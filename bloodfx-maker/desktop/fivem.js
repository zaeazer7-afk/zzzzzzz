// FiveM's folders, the packs in mods, installing with backups, and putting backups back.
const fs = require('fs');
const os = require('os');
const path = require('path');
const { hasCoreYpt } = require('./rpfscan');

const OFF = '.disabled';
const plainName = (n) => typeof n === 'string' && n.length < 200 && n === path.basename(n) && /\.rpf$/i.test(n) && !/[<>:"|?*\\/\x00-\x1f]/.test(n);

// Windows keeps a running FiveM's files locked; say so plainly instead of EBUSY.
function friendly(e) {
  if (e && (e.code === 'EBUSY' || e.code === 'EPERM' || e.code === 'EACCES')) return new Error(`Windows won't let the maker change ${e.path ? path.basename(e.path) : 'a file'}. Close FiveM and try again.`);
  return e;
}

function createFiveM(dataDir) {
  const settingsFile = path.join(dataDir, 'fivem.json');
  const backupsDir = path.join(dataDir, 'backups');
  const readJson = (f, fallback) => { try { return JSON.parse(fs.readFileSync(f, 'utf8')); } catch { return fallback; } };
  const settings = readJson(settingsFile, {});
  const isApp = (dir) => { try { return fs.statSync(path.join(dir, 'citizen')).isDirectory(); } catch { return false; } };
  const defaultRoot = () => path.join(process.env.LOCALAPPDATA || path.join(os.homedir(), 'AppData', 'Local'), 'FiveM', 'FiveM.app');
  const root = () => (settings.root && isApp(settings.root) ? settings.root : defaultRoot());
  const dirs = (r) => ({ root: r, mods: path.join(r, 'mods'), effects: path.join(r, 'citizen', 'common', 'data', 'effects') });

  function detect() {
    const d = dirs(root());
    const found = isApp(d.root);
    return { found, ...d, datExists: found && fs.existsSync(path.join(d.effects, 'bloodfx.dat')), custom: !!settings.root };
  }

  // Accepts FiveM.app itself or a folder above it.
  function useFolder(dir) {
    const hit = [dir, path.join(dir, 'FiveM.app'), path.join(dir, 'FiveM', 'FiveM.app')].find(isApp);
    if (!hit) return { found: false, root: dir, error: `${dir} isn't FiveM.app. Pick the folder that has citizen, data and mods inside.` };
    settings.root = hit;
    fs.mkdirSync(dataDir, { recursive: true });
    fs.writeFileSync(settingsFile, JSON.stringify(settings));
    return detect();
  }

  const looked = new Map();
  function scan() {
    const d = detect();
    if (!d.found) return { found: false, mods: [] };
    let files = [];
    try { files = fs.readdirSync(d.mods); } catch { files = []; }
    const mods = [];
    for (const file of files) {
      const enabled = /\.rpf$/i.test(file);
      if (!enabled && !/\.rpf\.disabled$/i.test(file)) continue;
      const full = path.join(d.mods, file);
      let st;
      try { st = fs.statSync(full); } catch { continue; }
      if (!st.isFile()) continue;
      const key = `${full}|${st.size}|${st.mtimeMs}`;
      if (!looked.has(key)) looked.set(key, hasCoreYpt(full));
      mods.push({ name: enabled ? file : file.slice(0, -OFF.length), file, size: st.size, enabled, hasCore: looked.get(key), modified: st.mtimeMs });
    }
    mods.sort((a, b) => a.name.toLowerCase().localeCompare(b.name.toLowerCase()));
    return { found: true, mods };
  }

  function toggle(name, enable) {
    if (!plainName(name)) throw new Error(`${name} isn't a pack in the mods folder.`);
    const d = detect();
    const on = path.join(d.mods, name), off = on + OFF;
    try {
      if (enable && fs.existsSync(off)) {
        if (fs.existsSync(on)) throw new Error(`${name} is already on, and there's also a turned-off copy.`);
        fs.renameSync(off, on);
      } else if (!enable && fs.existsSync(on)) {
        if (fs.existsSync(off)) throw new Error(`There's already a turned-off copy of ${name}.`);
        fs.renameSync(on, off);
      }
    } catch (e) { throw friendly(e); }
    return scan();
  }

  function writeAtomic(file, bytes) {
    const tmp = file + '.bfxm-tmp';
    fs.writeFileSync(tmp, bytes);
    fs.renameSync(tmp, file);
  }

  // Writes the pack to mods and bloodfx.dat to citizen\common\data\effects, backing up what they replace,
  // and turns off the packs named in `disable` (renamed to .disabled).
  function install({ rpfName, rpf, dat, disable }) {
    if (!plainName(rpfName)) throw new Error('The tracer pack needs a plain .rpf file name.');
    if (!(rpf instanceof Uint8Array) || !(dat instanceof Uint8Array) || rpf.length < 16 || dat.length < 16) throw new Error('The files to install are missing.');
    const d = detect();
    if (!d.found) throw new Error('FiveM.app not found. Choose its folder on the Install page.');
    const id = new Date().toISOString().replace(/[:.]/g, '-');
    const dir = path.join(backupsDir, id);
    const manifest = { id, when: Date.now(), root: d.root, replaced: [], created: [], disabled: [] };
    const targets = [[path.join(d.mods, rpfName), rpf], [path.join(d.effects, 'bloodfx.dat'), dat]];
    try {
      fs.mkdirSync(d.mods, { recursive: true });
      fs.mkdirSync(d.effects, { recursive: true });
      fs.mkdirSync(dir, { recursive: true });
      targets.forEach(([dest], i) => {
        if (fs.existsSync(dest)) {
          const saved = path.join(dir, `${i}-${path.basename(dest)}`);
          fs.copyFileSync(dest, saved);
          manifest.replaced.push({ path: dest, saved });
        } else {
          manifest.created.push(dest);
        }
      });
      fs.writeFileSync(path.join(dir, 'manifest.json'), JSON.stringify(manifest, null, 2));
      for (const [dest, bytes] of targets) writeAtomic(dest, Buffer.from(bytes.buffer, bytes.byteOffset, bytes.byteLength));
      for (const name of Array.isArray(disable) ? disable : []) {
        if (!plainName(name) || name.toLowerCase() === rpfName.toLowerCase()) continue;
        const on = path.join(d.mods, name), off = on + OFF;
        if (fs.existsSync(on) && !fs.existsSync(off)) { fs.renameSync(on, off); manifest.disabled.push(name); }
      }
      fs.writeFileSync(path.join(dir, 'manifest.json'), JSON.stringify(manifest, null, 2));
    } catch (e) { throw friendly(e); }
    return { ok: true, wrote: targets.map(([p]) => p), backup: { id, when: manifest.when }, replaced: manifest.replaced.length, disabled: manifest.disabled };
  }

  function backups() {
    let ids = [];
    try { ids = fs.readdirSync(backupsDir); } catch { return []; }
    return ids.map((id) => readJson(path.join(backupsDir, id, 'manifest.json'), null)).filter(Boolean)
      .sort((a, b) => b.when - a.when)
      .map((m) => ({ id: m.id, when: m.when, files: [
        ...m.replaced.map((r) => `${path.basename(r.path)} (old copy)`),
        ...m.created.map((c) => `${path.basename(c)} (was new)`),
        ...m.disabled.map((n) => `${n} (turned off)`),
      ] }));
  }

  // Puts the replaced files back, removes files the install added, and turns the packs it turned off back on.
  function restore(id) {
    if (typeof id !== 'string' || id !== path.basename(id)) throw new Error('Unknown backup.');
    const m = readJson(path.join(backupsDir, id, 'manifest.json'), null);
    if (!m) throw new Error('That backup is missing.');
    try {
      for (const r of m.replaced) { fs.mkdirSync(path.dirname(r.path), { recursive: true }); fs.copyFileSync(r.saved, r.path); }
      for (const c of m.created) fs.rmSync(c, { force: true });
      const d = dirs(m.root);
      for (const name of m.disabled) {
        const on = path.join(d.mods, name), off = on + OFF;
        if (fs.existsSync(off) && !fs.existsSync(on)) fs.renameSync(off, on);
      }
    } catch (e) { throw friendly(e); }
    return { ok: true };
  }

  function folder(which) {
    const d = detect();
    const target = which === 'backups' ? backupsDir : which === 'effects' ? d.effects : d.mods;
    fs.mkdirSync(target, { recursive: true });
    return target;
  }

  return { detect, useFolder, scan, toggle, install, backups, restore, folder };
}

module.exports = { createFiveM };
