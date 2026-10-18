/* Install: what gets saved and where it goes. In the Windows app it also finds FiveM, installs, lists mods and restores backups. */
(function (root) {
  'use strict';
  const pages = (root.BfxPages = root.BfxPages || {});

  pages.install = function (app) {
    const { U, D, A, P, state } = app;
    const { h, clear, plural } = U;
    const ic = (name) => h('span', { html: D.icon(name), style: { display: 'contents' } });
    const MODS = '%localappdata%\\FiveM\\FiveM.app\\mods';
    const EFFECTS = '%localappdata%\\FiveM\\FiveM.app\\citizen\\common\\data\\effects';
    const panel = (title, ...body) => h('section.panel', h('div.panel-head', h('h2', title)), h('div.panel-body', { style: { display: 'grid', gap: '12px' } }, body));

    // ------------------------------------------------------------ what gets saved
    const tracerArt = h('canvas');
    const tracerName = h('b'), tracerTo = h('div.to'), tracerWhat = h('div.what');
    const bloodName = h('b', 'bloodfx.dat'), bloodTo = h('div.to', EFFECTS), bloodWhat = h('div.what');
    const files = h('div', { style: { display: 'grid', gap: '8px' } },
      h('div.file-row', h('span.art', tracerArt), h('div', tracerName, tracerTo, tracerWhat)),
      h('div.file-row', h('span.art', { html: D.fxIcon('blood') }), h('div', bloodName, bloodTo, bloodWhat)),
      h('div.file-row', h('span.art', ic('info')), h('div', h('b', 'INSTALL.txt'), h('div.what', 'These steps, plus a note of every change. Only in the zip.'))));
    const saveBtn = h('button.btn.big', { type: 'button', onclick: () => app.save() }, ic('save'), 'Save .zip');
    const installBtn = h('button.btn.primary.big', { type: 'button', onclick: () => app.install() }, ic('install'), 'Install to FiveM');
    installBtn.hidden = !P.canInstall;
    const whatPanel = panel('What gets saved', h('p.explain', 'Saving always gives you both files. The tracer is rebuilt from your pack, so you need one open.'), files, h('div', { style: { display: 'flex', gap: '10px', flexWrap: 'wrap' } }, installBtn, saveBtn));

    const tips = panel('Good to know',
      h('div.tip', ic('info'), h('span', 'FiveM uses one core.ypt pack at a time. If two packs in mods both change the tracer, only one of them shows.')),
      h('div.tip', ic('info'), h('span', 'Servers with strict client-mod rules ignore files in mods and citizen.')),
      h('div.tip', ic('info'), h('span', 'Encrypted packs can’t be opened. Open the pack in OpenIV, switch it to OpenIV (unencrypted) and save it first.')),
      h('div.tip', ic('warn'), h('span', 'Nothing here has been tested in game by the maker itself. Every file is checked by reading it back, and was checked with CodeWalker on 12 real packs.')));

    // ------------------------------------------------------------ by hand (web)
    const copyBtn = (text) => {
      const b = h('button.btn.small', { type: 'button' }, ic('copy'), 'Copy');
      b.addEventListener('click', () => {
        try { navigator.clipboard.writeText(text).then(() => { b.lastChild.textContent = 'Copied'; setTimeout(() => { b.lastChild.textContent = 'Copy'; }, 1500); }, () => {}); } catch { /* the path is selectable */ }
      });
      return b;
    };
    const handPanel = panel('Install by hand',
      h('ol.steps',
        h('li', 'Close FiveM.'),
        h('li', 'Unzip the file you saved.'),
        h('li', 'Put the .rpf from the zip’s ', h('code', 'mods'), ' folder in your FiveM mods folder. Remove the old copy of that pack, and any other pack that changes the tracer.', h('div.path-box', { style: { marginTop: '6px' } }, h('span', { style: { flex: '1' } }, MODS), copyBtn(MODS))),
        h('li', 'Put ', h('code', 'bloodfx.dat'), ' here. Make the folders if they’re missing, and back up any bloodfx.dat already there.', h('div.path-box', { style: { marginTop: '6px' } }, h('span', { style: { flex: '1' } }, EFFECTS), copyBtn(EFFECTS))),
        h('li', 'Start FiveM.')),
      P.canInstall ? null : h('div.note.info', 'The Windows app version of BloodFX Maker does these steps for you, keeps backups, and can turn other tracer packs off.'));

    // ------------------------------------------------------------ desktop: FiveM folder, mods, backups
    const fivemDot = h('span.state-dot'), fivemPath = h('span', { style: { flex: '1' } }, 'Looking for FiveM…');
    const fivemPanel = panel('FiveM',
      h('div.path-box', fivemDot, fivemPath),
      h('div', { style: { display: 'flex', gap: '8px', flexWrap: 'wrap' } },
        h('button.btn.small', { type: 'button', onclick: () => detect(true) }, ic('search'), 'Find again'),
        h('button.btn.small', { type: 'button', onclick: async () => { const r = await P.fivem.choose(); if (r) { where = r; renderWhere(); scan(); } } }, ic('folder'), 'Choose FiveM.app…'),
        h('button.btn.small', { type: 'button', onclick: () => P.fivem.open('mods') }, ic('open'), 'Open mods folder'),
        h('button.btn.small', { type: 'button', onclick: () => P.fivem.open('effects') }, ic('open'), 'Open effects folder')));
    const modsBox = h('div', { style: { display: 'grid', gap: '6px' } });
    const modsPanel = panel('Packs in your mods folder', h('p.explain', 'Turning a pack off renames it to .disabled, so FiveM skips it. Turn it back on any time.'), modsBox);
    const backupsBox = h('div', { style: { display: 'grid', gap: '6px' } });
    const backupsPanel = panel('Backups', h('p.explain', 'Every install backs up the files it replaces. Restore puts them back and turns the packs it turned off back on.'), backupsBox);
    let where = null;
    async function detect(again) {
      try { where = await P.fivem.detect(); } catch (e) { where = { found: false, error: e.message }; }
      renderWhere();
      if (where && where.found) { scan(); backups(); } else { clear(modsBox).append(h('p.field-help', 'FiveM not found yet.')); }
      if (again && where && where.found) app.toast('ok', 'Found FiveM', where.root);
    }
    function renderWhere() {
      fivemDot.className = 'state-dot ' + (where && where.found ? 'ok' : 'bad');
      fivemPath.textContent = where && where.found ? where.root : (where && where.error) || 'FiveM.app not found. Choose its folder; it is usually in %localappdata%\\FiveM.';
    }
    async function scan() {
      let res;
      try { res = await P.fivem.scan(); } catch (e) { clear(modsBox).append(h('p.note.bad', e.message)); return; }
      clear(modsBox);
      if (!res.mods.length) { modsBox.append(h('p.field-help', 'Your mods folder is empty.')); return; }
      for (const m of res.mods) {
        const input = h('input', { type: 'checkbox', role: 'switch', checked: m.enabled, ariaLabel: (m.enabled ? 'Turn off ' : 'Turn on ') + m.name });
        input.addEventListener('change', async () => {
          try { await P.fivem.toggle(m.name, input.checked); scan(); } catch (e) { app.toast('bad', "Couldn't change " + m.name, e.message); scan(); }
        });
        modsBox.append(h('div.mod-row', h('div', h('div.nm', m.name), h('div.meta', h('span', U.fmtBytes(m.size)),
          m.hasCore ? h('span.tag.accent', 'changes the tracer') : m.hasCore === null ? h('span.tag', 'couldn’t look inside') : null,
          m.enabled ? null : h('span.tag.off', 'off'))), h('label.switch', input)));
      }
      const on = res.mods.filter((m) => m.enabled && m.hasCore);
      if (on.length > 1) modsBox.prepend(h('div.note', `${plural(on.length, 'pack')} that change the tracer are on. Only one of them will show in game.`));
    }
    async function backups() {
      let list = [];
      try { list = await P.fivem.backups(); } catch { list = []; }
      clear(backupsBox);
      if (!list.length) { backupsBox.append(h('p.field-help', 'No backups yet.')); return; }
      for (const b of list.slice(0, 8)) {
        backupsBox.append(h('div.backup-row', h('div', h('div.when', new Date(b.when).toLocaleString()), h('div.files', b.files.join(' · ') || 'nothing replaced')),
          h('button.btn.small', { type: 'button', onclick: async () => {
            const ok = await app.confirmBox({ title: 'Restore this backup?', text: 'The files it holds are copied back, and packs that install turned off are turned on again.', items: b.files,
              buttons: [{ label: 'Cancel', value: false, ghost: true }, { label: 'Restore', value: true, primary: true }] });
            if (!ok) return;
            try { await P.fivem.restore(b.id); app.toast('ok', 'Restored'); scan(); backups(); } catch (e) { app.toast('bad', "Couldn't restore", e.message); }
          } }, ic('restore'), 'Restore')));
      }
    }

    function renderSaved() {
      const p = state.pack;
      tracerName.textContent = p ? (p.kind === 'ypt' ? 'core.ypt' : p.fileName) : 'Your tracer pack (.rpf)';
      tracerTo.textContent = p && p.kind === 'ypt' ? "A loose core.ypt: put it in a resource's stream folder or inside your pack with OpenIV" : MODS;
      tracerWhat.textContent = (p ? '' : 'Open a pack first. ') + 'Tracer: ' + app.describeBeam(state.tracer) + ', ' + app.describeSmoke(state.tracer) + (app.muzzleChoice() ? `; ${app.muzzleChoice().names.length} muzzle flashes recoloured` : '');
      const n = app.hitCount();
      bloodWhat.textContent = (n ? `${plural(n, 'hit')} changed` : 'No hit changes') + (state.blood.splatOn ? ' · new splat colour' : '') + (state.blood.splatter !== 1 ? ` · splatter ×${state.blood.splatter}` : '');
      const t = state.tracer;
      A.tracerThumb(tracerArt, { sheet: app.sheet(), glowColour: t.style === 'keep' ? '#ffb070' : t.colour, glow: t.glow, zoom: t.zoom, smoke: t.smoke !== 'off', beam: t.style !== 'none' }, { w: 52, h: 52, width: 14 });
    }

    const cols = P.canInstall
      ? [h('div.col', whatPanel, fivemPanel, tips), h('div.col', modsPanel, backupsPanel)]
      : [h('div.col', whatPanel, tips), h('div.col', handPanel)];
    const el = h('section.page-scroll.scroll', h('div.install', cols));
    let first = true;
    function refresh(what) {
      if (what === 'install') { if (P.canInstall) { scan(); backups(); } return; }
      if (!el.hidden) renderSaved();
    }
    return { el, actions: [], refresh, shown: () => { renderSaved(); if (P.canInstall) { if (first) detect(false); else { scan(); backups(); } first = false; } } };
  };
})(typeof self !== 'undefined' ? self : this);
