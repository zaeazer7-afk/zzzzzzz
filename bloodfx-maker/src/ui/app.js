/* BloodFX Maker app: state, undo, loading, saving, and the frame (title bar, toolbar, tabs, status bar) around the pages. */
(function (root) {
  'use strict';
  const U = root.BfxUtil, D = root.BfxData, K = root.BfxControls, P = root.BfxPlatform, A = root.BfxArt;
  const { $, $$, h, clear, tick, plural } = U;
  const VERSION = '2.0';

  const TABS = [
    { id: 'home', label: 'Home', icon: 'home', title: 'Home', text: 'Open a tracer pack, pick a look and hit effects, then save or install.' },
    { id: 'tracer', label: 'Tracer', icon: 'tracer', title: 'Tracer', text: "Keep your pack's tracer, recolour it, or draw a new beam and smoke trail." },
    { id: 'blood', label: 'Blood', icon: 'drop', title: 'Blood', text: 'Pick what plays when a bullet hits someone, for each body part and weapon type.' },
    { id: 'muzzle', label: 'Muzzle flash', icon: 'muzzle', title: 'Muzzle flash', text: "Recolour the flash at the end of the gun. It's saved into the same tracer pack." },
    { id: 'explorer', label: 'Pack explorer', icon: 'explorer', title: 'Pack explorer', text: "Look inside the pack's core.ypt: every effect, texture and file." },
    { id: 'install', label: 'Install', icon: 'install', title: 'Install', text: 'Where the two files go, and a one-click install in the Windows app.' },
    { id: 'settings', label: 'Settings', icon: 'settings', title: 'Settings', text: 'Accent colour, preview and tips.' },
  ];
  const MOTION = ['glow', 'zoom', 'far', 'speed', 'stay', 'life'];
  const PLATFORM_LABEL = { desktop: 'Windows app', claude: 'Running in Claude', browser: 'Running in your browser' };

  function fatal(msg) {
    $('#fatal-text').textContent = msg;
    $('#fatal').hidden = false;
  }

  function start(pageFactories) {
    if (!root.pako || !root.BfxCore) {
      fatal('BloodFX Maker could not load its compression library. Check your internet connection and reload.');
      return;
    }
    const C = root.BfxCore;
    C.setZlib(root.pako);
    const BUILTIN_DAT = U.latin1(root.pako.inflateRaw(U.b64Bytes(D.BASE_DAT_B64)));
    const CORE_FX = new Set(D.CORE_FX.split(' '));
    const LONG_FX = new Set(D.LONG_FX.split(' '));

    // ------------------------------------------------------------ state
    const saved = U.store.get('settings', {});
    const state = {
      tab: 'home',
      pack: null,
      tracer: { style: 'solid', colour: '#ff2bd6', colour2: null, flip: false, core: '#ffffff', width: 3, tintOn: false, tint: '#ffffff',
        smoke: 'off', smokeColour: '#8a6bff', smokeColour2: null, smokeFlip: false, smokeWidth: 3, ...D.STOCK },
      packValues: { ...D.STOCK },
      touched: new Set(),
      lookTouched: false,
      tracerPreset: null,
      blood: { base: 'builtin', fileText: null, fileName: '', fileProblems: [], hits: {}, dead: true, splatOn: false, splat: '#00a0ff', strength: 50, splatter: 1, preset: null },
      muzzle: { enabled: false, match: 'muz', colour: '#3b82ff', glowOn: false, glow: 20, picked: null },
      selection: new Set(['head:guns']),
      settings: Object.assign({ accent: 'blue', scene: 'street', camera: 'side', auto: true, tips: true }, saved),
    };
    let packSeq = 0;

    // ------------------------------------------------------------ undo / redo
    const history = { past: [], future: [], current: null };
    let savedAt = null;
    function snap() {
      const b = state.blood;
      return JSON.stringify({
        tracer: state.tracer, touched: [...state.touched], lookTouched: state.lookTouched, tracerPreset: state.tracerPreset,
        blood: { base: b.base, hits: b.hits, dead: b.dead, splatOn: b.splatOn, splat: b.splat, strength: b.strength, splatter: b.splatter, preset: b.preset },
        muzzle: { ...state.muzzle, picked: state.muzzle.picked ? [...state.muzzle.picked] : null },
      });
    }
    function commit() {
      const now = snap();
      if (now === history.current) return;
      if (history.current !== null) history.past.push(history.current);
      if (history.past.length > 200) history.past.shift();
      history.current = now;
      history.future.length = 0;
      renderUndo();
      renderNote();
    }
    function restore(s) {
      const o = JSON.parse(s);
      Object.assign(state.tracer, o.tracer);
      state.touched = new Set(o.touched);
      state.lookTouched = o.lookTouched;
      state.tracerPreset = o.tracerPreset;
      Object.assign(state.blood, o.blood);
      Object.assign(state.muzzle, o.muzzle, { picked: o.muzzle.picked ? new Set(o.muzzle.picked) : null });
      enforceWhole();
      changed('all');
    }
    function undo() {
      if (!history.past.length) return;
      history.future.push(history.current);
      history.current = history.past.pop();
      restore(history.current);
      renderUndo();
      renderNote();
    }
    function redo() {
      if (!history.future.length) return;
      history.past.push(history.current);
      history.current = history.future.pop();
      restore(history.current);
      renderUndo();
      renderNote();
    }
    function renderUndo() {
      $('#tb-undo').disabled = !history.past.length;
      $('#tb-redo').disabled = !history.future.length;
    }

    // ------------------------------------------------------------ tracer
    const look = (t) => ({
      beam: { style: t.style, colour: t.colour, colour2: t.colour2 || null, flip: !!t.flip, core: t.core, width: t.width },
      smoke: { mode: t.smoke, colour: t.smokeColour, colour2: t.smokeColour2 || null, flip: !!t.smokeFlip, width: t.smokeWidth },
    });
    const drawnSmoke = (m) => m === 'line' || m === 'wavy';

    // A BC7 sheet can be kept whole or redrawn, but not kept or recoloured in part.
    function whole() { return !!state.pack && !state.pack.recolourable; }
    function enforceWhole(t) {
      t = t || state.tracer;
      if (!whole()) return;
      if (t.style === 'recolour') t.style = 'keep';
      if (t.style === 'keep') t.smoke = 'keep';
      else if (t.smoke === 'keep' || t.smoke === 'recolour') t.smoke = 'off';
    }

    // The tracer sheet a set of settings gives, for previews. Without a pack the pack's own beam and smoke are stand-ins.
    function sheetFor(t) {
      const p = state.pack;
      const orig = p && p.preview ? p.preview : null;
      if (!orig) {
        const L = look(t);
        const beam = t.style === 'keep' ? { style: 'solid', colour: t.tintOn ? t.tint : '#ffb070', width: 3 } : t.style === 'recolour' ? { style: 'solid', colour: t.colour, width: 3 } : L.beam;
        const smoke = t.smoke === 'keep' ? { mode: 'off' } : t.smoke === 'recolour' ? { mode: 'line', colour: t.smokeColour, width: 4 } : L.smoke;
        return { px: C.drawTracerSheet(128, 256, { beam, smoke }), w: 128, h: 256, standIn: true };
      }
      const w = p.texture.w, hh = p.texture.h;
      if (t.style === 'keep' && t.smoke === 'keep') {
        if (!t.tintOn) return { px: orig, w, h: hh };
        const px = orig.slice();
        const tint = U.rgbOf(t.tint);
        for (let i = 0; i < px.length; i += 4) for (let c = 0; c < 3; c++) px[i + c] = (px[i + c] * tint[c]) / 255;
        return { px, w, h: hh };
      }
      return { px: C.composeSheet(orig, w, hh, look(t)), w, h: hh };
    }
    let sheetKey = null, sheetVal = null;
    function sheet() {
      const t = state.tracer;
      const key = JSON.stringify([state.pack && state.pack.id, look(t), t.tintOn && t.tint]);
      if (key !== sheetKey) { sheetKey = key; sheetVal = sheetFor(t); }
      return sheetVal;
    }

    function tracerSettings() {
      const t = state.tracer, s = {};
      if (t.style === 'keep' && t.smoke === 'keep') {
        if (t.tintOn) s.beamRgb = U.rgb01(t.tint);
      } else {
        s.texture = look(t);
        // a new or recoloured beam shows its colour as picked; a recoloured smoke keeps the pack's own particle colour
        if (t.style !== 'keep') { s.beamRgb = [1, 1, 1]; s.tint = '#ffffff'; }
        if (drawnSmoke(t.smoke)) { s.smokeRgb = [1, 1, 1]; s.smokeTint = '#ffffff'; }
      }
      const T = state.touched;
      if (T.has('glow')) s.glow = t.glow;
      if (T.has('zoom')) s.zoom = [t.zoom, t.zoom];
      if (T.has('far')) s.farSize = t.far;
      if (T.has('speed')) { s.effectRate = [t.speed, t.speed]; s.beamRate = [1, 1]; }
      if (T.has('stay')) s.duration = [t.stay, t.stay];
      if (T.has('life')) s.life = [Math.round(t.life * 75) / 100, t.life];
      const m = muzzleChoice();
      if (m) s.muzzle = m;
      return s;
    }

    function describeBeam(t) {
      const grad = (a, b) => (b ? `${a.toUpperCase()} → ${b.toUpperCase()}` : a.toUpperCase());
      switch (t.style) {
        case 'keep': return state.pack ? "pack's beam" + (t.tintOn && t.smoke === 'keep' ? ` tinted ${t.tint.toUpperCase()}` : '') : 'stock beam';
        case 'recolour': return `pack's beam in ${t.colour.toUpperCase()}`;
        case 'none': return 'no beam';
        default: return `${t.style} beam ${t.width}px ${grad(t.colour, t.colour2)}`;
      }
    }
    function describeSmoke(t) {
      switch (t.smoke) {
        case 'keep': return state.pack ? "pack's smoke" : 'stock smoke';
        case 'recolour': return `pack's smoke in ${t.smokeColour.toUpperCase()}`;
        case 'off': return 'no smoke';
        default: return `${t.smoke === 'line' ? 'straight' : 'wavy'} smoke ${t.smokeColour2 ? t.smokeColour.toUpperCase() + ' → ' + t.smokeColour2.toUpperCase() : t.smokeColour.toUpperCase()}`;
      }
    }

    function applyTracerPreset(p) {
      const t = state.tracer;
      Object.assign(t, { colour2: null, flip: false, smokeColour2: null, smokeFlip: false, tintOn: false }, p.set);
      state.lookTouched = true;
      if (p.motion !== false) for (const k of MOTION) if (k in p.set) state.touched.add(k);
      state.tracerPreset = p.id;
      enforceWhole();
      changed('tracer');
      commit();
      preview.soon();
    }

    // ------------------------------------------------------------ blood
    const baseText = () => (state.blood.base === 'file' && state.blood.fileText ? state.blood.fileText : BUILTIN_DAT);
    let baseCache = { text: null, summary: null };
    function baseSummary() {
      const t = baseText();
      if (baseCache.text !== t) baseCache = { text: t, summary: new C.BloodDat(t).summary() };
      return baseCache.summary;
    }
    const validFx = (fx) => fx === '-' || /^[a-z0-9_]+$/i.test(fx || '');
    function splatTint() {
      const rgb = U.rgbOf(state.blood.splat);
      const max = Math.max(1, ...rgb);
      return rgb.map((c) => Math.round((c / max) * state.blood.strength));
    }
    let bloodKey = null, bloodVal = null;
    function blood() {
      const b = state.blood;
      const key = JSON.stringify([b.base, b.fileName, b.fileText && b.fileText.length, b.hits, b.dead, b.splatOn, b.splat, b.strength, b.splatter]);
      if (bloodVal && key === bloodKey) return bloodVal;
      bloodKey = key;
      try {
        const dat = new C.BloodDat(baseText());
        // Hits with the same settings are routed together, one call per rectangle of the hit map,
        // so shared rows in bloodfx.dat are copied as few times as possible.
        const groups = new Map();
        for (const [cell, a] of Object.entries(b.hits)) {
          if (!a || !validFx(a.fx)) continue;
          const [zone, weapon] = cell.split(':');
          const k = `${a.fx}|${a.armour ? 1 : 0}|${a.size === null || a.size === undefined ? '' : a.size}`;
          if (!groups.has(k)) groups.set(k, { a, zones: new Map() });
          const g = groups.get(k);
          if (!g.zones.has(zone)) g.zones.set(zone, new Set());
          g.zones.get(zone).add(weapon);
        }
        for (const g of groups.values()) {
          const rects = new Map();
          for (const [zone, ws] of g.zones) {
            const wk = [...ws].sort().join(',');
            if (!rects.has(wk)) rects.set(wk, []);
            rects.get(wk).push(zone);
          }
          for (const [wk, zones] of rects) {
            dat.route(zones, wk.split(','), g.a.fx, { armour: g.a.armour, underwater: g.a.armour, size: g.a.size, dead: b.dead });
          }
        }
        if (b.splatOn) dat.tint(splatTint());
        if (b.splatter !== 1) dat.splatter(b.splatter);
        const text = dat.toString();
        bloodVal = { text, summary: dat.summary(), base: baseSummary(), issues: dat.check(), changed: text !== baseText(), error: null };
      } catch (e) {
        bloodVal = { text: null, summary: null, base: baseSummary(), issues: [], changed: false, error: e.message || String(e) };
      }
      return bloodVal;
    }
    function hitCount() { return Object.keys(state.blood.hits).length; }

    function applyBloodPreset(p) {
      const b = state.blood;
      b.hits = {};
      for (const c of p.cells) for (const z of c.zones) for (const w of c.weapons) b.hits[z + ':' + w] = { fx: c.fx, armour: c.armour, size: c.size };
      if (p.id === 'normal') { b.splatOn = false; b.splatter = 1; }
      if (p.splat) { b.splatOn = true; b.splat = p.splat; }
      if (p.splatter) b.splatter = p.splatter;
      b.preset = p.id;
      changed('blood');
      commit();
      preview.soon();
    }
    // Sets the effect (and options) on the given hit cells.
    function assignHits(cells, patch) {
      const b = state.blood;
      const now = blood().summary || baseSummary();
      for (const cell of cells) {
        const [zone, weapon] = cell.split(':');
        const cur = b.hits[cell] || { fx: (now[zone][weapon] || [])[0] || 'blood_entry', armour: true, size: null };
        b.hits[cell] = { ...cur, ...patch };
      }
      b.preset = null;
      changed('blood');
      commit();
    }
    function resetHits(cells) {
      for (const cell of cells) delete state.blood.hits[cell];
      state.blood.preset = null;
      changed('blood');
      commit();
    }

    // ------------------------------------------------------------ effects catalogue
    function packContents() {
      const p = state.pack;
      if (!p) return null;
      if (!p.contents) {
        p.contents = C.packContents(p.y);
        p.effectMap = new Map(p.contents.effects.map((e) => [e.name, e]));
      }
      return p.contents;
    }
    function fxInfo(id) {
      const cur = D.EFFECT_BY_ID.get(id);
      const cat = id === '-' ? 'none' : D.categoryOf(id);
      let kind = cur ? cur.kind : D.effectKind(id);
      if (!cur && kind === 'puff') kind = D.CATEGORY_KIND[cat] || 'puff';
      const p = state.pack;
      let loops = false, inPack = null, rule = null;
      if (id !== '-') {
        if (p) {
          packContents();
          rule = p.effectMap.get(id) || null;
          inPack = !!rule;
          loops = rule ? rule.loops !== 0 || rule.duration > 4 : LONG_FX.has(id);
        } else loops = LONG_FX.has(id);
      }
      return { id, label: D.effectLabel(id), curated: !!cur, note: cur && cur.note, group: cur ? cur.group : null, kind, cat, loops, inPack, standard: CORE_FX.has(id), rule };
    }
    // Every effect a hit can play: the curated list first, then the pack's (or GTA's standard) effects.
    let catalogueKey = null, catalogueVal = null;
    function catalogue() {
      const key = state.pack ? state.pack.id : 0;
      if (catalogueKey === key && catalogueVal) return catalogueVal;
      const names = new Set(D.EFFECTS.map((e) => e.id));
      const source = state.pack ? [...state.pack.effects] : [...CORE_FX];
      for (const n of source.sort()) names.add(n);
      catalogueVal = [...names].map(fxInfo);
      catalogueKey = key;
      return catalogueVal;
    }
    // A warning for an effect on a hit, or ''.
    function fxWarning(info) {
      if (info.id === '-') return '';
      if (!validFx(info.id)) return 'Effect names use letters, numbers and underscores only.';
      if (state.pack && !info.inPack) return `Not in ${state.pack.fileName}, so it won't play with that pack.`;
      if (!state.pack && !info.standard) return "Not in GTA's standard core.ypt. It only plays with a pack that adds it.";
      if (info.loops) return 'This effect loops or runs for a long time, so it keeps playing where the bullet hit.';
      return '';
    }

    // ------------------------------------------------------------ muzzle flashes
    let muzzleKey = null, muzzleVal = [];
    function muzzleRules() {
      const p = state.pack;
      if (!p) return [];
      const key = p.id + '|' + state.muzzle.match;
      if (key !== muzzleKey) { muzzleKey = key; muzzleVal = C.colourRules(p.y, state.muzzle.match.trim()); }
      return muzzleVal;
    }
    function muzzlePicked() {
      const rules = muzzleRules();
      const m = state.muzzle;
      return m.picked ? rules.filter((r) => m.picked.has(r.name)) : rules;
    }
    function muzzleChoice() {
      const m = state.muzzle;
      if (!m.enabled || !state.pack) return null;
      const names = muzzlePicked().map((r) => r.name);
      if (!names.length) return null;
      return { names, rgb: U.rgb01(m.colour), emissive: m.glowOn ? m.glow : undefined };
    }

    // ------------------------------------------------------------ preview (one viewport, moved between pages)
    const canvas = h('canvas', { attrs: { 'aria-label': 'Preview of the tracer and the hit effect' } });
    const hud = h('div.vp-hud');
    const viewport = h('div.viewport', canvas, hud, h('span.vp-note', 'A sketch of the look, not GTA’s lighting'));
    const target = { zone: null, weapon: 'guns' };
    function previewHit(zone) {
      const b = blood();
      const s = b.summary || b.base;
      const z = target.zone === 'animals' && zone === 'body' ? 'animals' : zone;
      return ((s[z] || s.head)[target.weapon] || [])[0] || '-';
    }
    function timing() {
      const p = state.pack;
      if (!p || !p.tracer.smokeLife) return null;
      const k = p.tracer.smokeLife.keyframes[0];
      const alpha = p.tracer.smokeColour ? p.tracer.smokeColour.rgbaMax.map((x) => [x.t[0], x.v[3]]) : null;
      return { smokeLife: k ? [k.v[0], k.v[1]] : [0.6, 0.6], smokeAlpha: alpha && alpha.length ? alpha : null };
    }
    const preview = root.BfxPreview.create(canvas, {
      getTracer: () => state.tracer,
      getHit: previewHit,
      kindOf: (fx) => fxInfo(fx).kind,
      getTiming: timing,
    });
    preview.set('scene', state.settings.scene === 'image' ? 'street' : state.settings.scene);
    preview.set('camera', state.settings.camera);
    preview.set('auto', state.settings.auto);
    function renderHud() {
      const t = state.tracer;
      clear(hud);
      const chip = (label, value, colour) => h('span.hud-chip', colour ? h('i.sw', { style: { background: colour } }) : null, label + ' ', h('b', value));
      const beamColour = t.style === 'keep' ? (t.tintOn ? t.tint : null) : t.style === 'none' ? null : t.colour;
      const beamText = { keep: state.pack ? 'from the pack' : 'stock', recolour: 'pack, recoloured', none: 'none' }[t.style] || `${t.style} ${t.width}px`;
      hud.append(chip('beam', beamText, beamColour));
      const smokeText = { keep: state.pack ? 'from the pack' : 'stock', recolour: 'pack, recoloured', line: 'straight', wavy: 'wavy' }[t.smoke];
      if (t.smoke !== 'off') hud.append(chip('smoke', smokeText, drawnSmoke(t.smoke) || t.smoke === 'recolour' ? t.smokeColour : null));
      hud.append(chip('speed', '×' + t.speed.toFixed(2)), chip('stays', t.stay.toFixed(2) + ' s'));
      const zone = target.zone || 'head';
      const fx = previewHit(zone === 'animals' ? 'body' : zone);
      hud.append(chip({ head: 'headshot', body: 'body shot', animals: 'animal hit' }[zone], fx === '-' ? 'nothing' : D.effectLabel(fx)));
    }

    // ------------------------------------------------------------ loading
    async function openPackDialog() {
      let f;
      try { f = await P.openPack(); } catch (e) { toast('bad', "Couldn't open the file", e.message || String(e)); return false; }
      if (!f) return false;
      return loadPack(f);
    }
    async function loadPack(f) {
      busy('Reading ' + f.name + '…');
      await tick(40);
      try {
        const pack = C.openPack(f.bytes, f.name);
        const y = new C.Ypt(pack.res);
        const tr = C.readTracer(y);
        const tex = tr.texture;
        let px = null;
        try { px = tex ? C.decodeTexture(y, tex) : null; } catch { px = null; }
        state.pack = {
          id: ++packSeq, fileName: f.name, path: f.path || null, bytes: f.bytes, kind: pack.kind, paths: pack.paths || [],
          files: pack.files ? pack.files.map((x) => ({ path: x.path, kind: x.node.kind, size: x.node.stored ? x.node.stored.length : 0, nested: !!x.node.rpf })) : [],
          y, tracer: tr, effects: new Set(y.effectRules().keys()), preview: px, recolourable: C.canRecolour(tex),
          texture: tex ? { w: tex.width, h: tex.height, format: tex.formatName, levels: tex.levels } : null,
          size: f.bytes.length, contents: null, effectMap: null,
        };
        const v = {
          glow: tr.beamColour && tr.beamColour.emissive[0] ? tr.beamColour.emissive[0].v[0] : D.STOCK.glow,
          zoom: tr.beam.zoom[0],
          far: tr.beam.lodSize ? tr.beam.lodSize.value[0] : D.STOCK.far,
          speed: tr.effect.rate[0] * tr.beam.rate[0],
          stay: Math.max(tr.effect.duration[0], tr.effect.duration[1]),
          life: tr.life && tr.life.keyframes[0] ? tr.life.keyframes[0].v[1] : D.STOCK.life,
        };
        for (const k of MOTION) v[k] = Math.round(v[k] * 1000) / 1000;
        state.packValues = v;
        Object.assign(state.tracer, v);
        state.touched.clear();
        if (!state.lookTouched) { state.tracer.style = 'keep'; state.tracer.smoke = 'keep'; state.tracerPreset = null; }
        enforceWhole();
        muzzleKey = null;
        catalogueKey = null;
        if (f.path) P.addRecent({ path: f.path, name: f.name }).catch(() => {});
        changed('pack');
        // starting from a pack's own values isn't a change of yours
        const clean = history.current === savedAt;
        commit();
        if (clean) { savedAt = history.current; renderNote(); }
        preview.soon();
        const facts = [tex ? `${tex.formatName} ${tex.width}×${tex.height} tracer sheet` : 'no tracer sheet', plural(state.pack.effects.size, 'effect')];
        toast('ok', 'Opened ' + f.name, facts.join(' · ') + (whole() ? `. ${tex ? tex.formatName : 'This'} sheets can be kept whole or redrawn, not recoloured.` : ''));
        return true;
      } catch (e) {
        toast('bad', `Couldn't open ${f.name}`, e.message || String(e));
        return false;
      } finally {
        idle();
      }
    }
    async function openDatDialog() {
      let f;
      try { f = await P.openDat(); } catch (e) { toast('bad', "Couldn't open the file", e.message || String(e)); return false; }
      if (!f) return false;
      return loadDat(f);
    }
    function loadDat(f) {
      try {
        const text = U.latin1(f.bytes);
        const dat = new C.BloodDat(text);
        const b = state.blood;
        b.fileText = text;
        b.fileName = f.name;
        b.fileProblems = dat.problems;
        b.base = 'file';
        changed('blood');
        commit();
        toast('ok', 'Blood now starts from ' + f.name, dat.problems.length ? `${plural(dat.problems.length, 'row')} with an unusual layout will be left as they are.` : 'Your hit changes are applied on top of it.');
        return true;
      } catch (e) {
        toast('bad', `Couldn't use ${f.name}`, (e.message || String(e)) + '. Choose a bloodfx.dat file.');
        return false;
      }
    }
    function pickFile(accept) {
      return new Promise((resolve) => {
        const input = h('input', { type: 'file', accept, style: { display: 'none' } });
        document.body.append(input);
        input.addEventListener('change', () => { resolve(input.files[0] || null); input.remove(); });
        input.addEventListener('cancel', () => { resolve(null); input.remove(); });
        input.click();
      });
    }
    let screenshotUrl = null;
    function useScreenshot(file) {
      return new Promise((resolve) => {
        const url = URL.createObjectURL(file);
        const img = new Image();
        img.onload = () => {
          if (screenshotUrl) URL.revokeObjectURL(screenshotUrl);
          screenshotUrl = url;
          preview.set('image', img);
          preview.set('scene', 'image');
          changed('view');
          toast('ok', 'Preview now uses your screenshot', 'Aim the crosshair where a head would be. Nothing is uploaded.');
          resolve(true);
        };
        img.onerror = () => { URL.revokeObjectURL(url); toast('bad', "Couldn't read that image", 'Use a PNG or JPG screenshot.'); resolve(false); };
        img.src = url;
      });
    }
    async function pickScreenshot() {
      const file = await pickFile('image/png,image/jpeg,image/webp,image/bmp');
      return file ? useScreenshot(file) : false;
    }
    async function openAny(file) {
      const name = file.name.toLowerCase();
      if (/\.(png|jpe?g|webp|bmp|gif)$/.test(name)) return useScreenshot(file);
      const f = await P.fromFile(file);
      if (/\.(dat|txt)$/.test(name)) return loadDat(f);
      return loadPack(f);
    }

    // ------------------------------------------------------------ saving and installing
    async function buildFiles(why) {
      if (!state.pack) {
        toast('info', 'Pick your tracer pack', `The ${why} always holds both files, and the tracer is built from one of your packs.`);
        const ok = await openPackDialog();
        if (!ok) return null;
      }
      const b = blood();
      if (b.error) throw new Error(b.error);
      busy('Rebuilding ' + state.pack.fileName + '…');
      await tick(40);
      const pack = C.openPack(state.pack.bytes, state.pack.fileName);
      const log = C.applyTracer(new C.Ypt(pack.res), tracerSettings());
      const body = await U.deflateRaw(pack.res.data);
      const out = C.savePack(pack, 6, body);
      return { kind: pack.kind, rpfName: pack.kind === 'ypt' ? 'core.ypt' : state.pack.fileName, rpf: out, dat: U.latin1Bytes(b.text), log };
    }

    function installText(files) {
      const t = state.tracer;
      const L = [`BloodFX Maker ${VERSION}`, '', 'Close FiveM before copying files.', ''];
      if (files.kind === 'ypt') {
        L.push('TRACER: core.ypt');
        L.push("  A loose core.ypt. Put it in a resource's stream folder, or replace core.ypt inside your pack with OpenIV.");
      } else {
        L.push('TRACER: mods\\' + files.rpfName);
        L.push('  Copy it to %localappdata%\\FiveM\\FiveM.app\\mods');
        L.push('  Remove the old copy of the same pack. Keep only one tracer pack in that folder.');
      }
      L.push('  Look: ' + describeBeam(t) + '; ' + describeSmoke(t));
      const s = tracerSettings();
      const changes = [];
      if (s.glow !== undefined) changes.push('glow ' + s.glow);
      if (s.zoom) changes.push('thickness x' + s.zoom[0]);
      if (s.farSize !== undefined) changes.push('far size ' + s.farSize);
      if (s.effectRate) changes.push('speed x' + s.effectRate[0]);
      if (s.duration) changes.push('stays ' + s.duration[0] + ' s');
      if (s.life) changes.push('beam life ' + s.life[1] + ' s');
      L.push('  Timing and glow: ' + (changes.length ? changes.join(', ') : "as the pack had them"));
      if (s.muzzle) L.push(`  Muzzle flashes: ${plural(s.muzzle.names.length, 'particle')} recoloured ${state.muzzle.colour.toUpperCase()}`);
      L.push('');
      L.push('BLOOD: citizen\\common\\data\\effects\\bloodfx.dat');
      L.push('  Copy it to %localappdata%\\FiveM\\FiveM.app\\citizen\\common\\data\\effects');
      L.push('  Make the folders if they are missing. Back up any bloodfx.dat already there.');
      L.push('  Starts from: ' + (state.blood.base === 'file' && state.blood.fileText ? state.blood.fileName : 'the built-in file (normal blood everywhere)'));
      const byFx = new Map();
      for (const [cell, a] of Object.entries(state.blood.hits)) {
        const k = `${a.fx}${a.armour ? ' (also with armour and underwater)' : ''}`;
        if (!byFx.has(k)) byFx.set(k, []);
        const [z, w] = cell.split(':');
        byFx.get(k).push(`${z} ${D.WEAPONS.find((x) => x.id === w).label.toLowerCase()}`);
      }
      for (const [k, cells] of byFx) L.push(`  ${cells.join(', ')}: ${k}`);
      if (!byFx.size) L.push('  No hit changes.');
      if (state.blood.splatOn) L.push('  Splat colour: ' + splatTint().join(' '));
      if (state.blood.splatter !== 1) L.push('  Splatter size: x' + state.blood.splatter);
      L.push('');
      L.push('Servers that block client mods ignore these files.');
      return L.join('\r\n') + '\r\n';
    }

    const zipName = () => (state.pack ? state.pack.fileName.replace(/\.[^.]+$/, '').replace(/[^\w.-]+/g, '_') + '-bloodfx-maker.zip' : 'bloodfx-maker.zip');
    const SAVE_ERRORS = {
      declined: null,
      rejected_extension: "This view can't save zip files.",
      extension_not_enabled: "This view can't save zip files.",
      too_large: 'The zip is too large to save here.',
      rate_limited: 'A save prompt is already open. Finish that one first.',
      unavailable: 'Saving files only works when this page is open in Claude or a browser.',
    };
    let saving = false;
    async function save() {
      if (saving) return;
      saving = true;
      try {
        const files = await buildFiles('zip');
        if (!files) return;
        busy('Packing the zip…');
        await tick(20);
        const zip = U.makeZip([
          { name: files.kind === 'ypt' ? 'core.ypt' : 'mods/' + files.rpfName, data: files.rpf },
          { name: 'citizen/common/data/effects/bloodfx.dat', data: files.dat },
          { name: 'INSTALL.txt', data: new TextEncoder().encode(installText(files)) },
        ]);
        const name = zipName();
        idle();
        const res = await P.saveZip(name, zip);
        if (res && res.saved) {
          savedAt = history.current;
          renderNote();
          toast('ok', 'Saved ' + name, res.path ? res.path : 'Unzip it, then follow the steps on the Install page.',
            res.path && P.kind === 'desktop' ? [{ label: 'Show file', run: () => P.revealFile(res.path) }] : [{ label: 'Install steps', run: () => go('install') }]);
        }
      } catch (e) {
        const code = e && e.code;
        if (code === 'declined') toast('info', 'Not saved');
        else toast('bad', "Couldn't save", SAVE_ERRORS[code] || (e && e.message ? e.message : String(e)));
      } finally {
        saving = false;
        idle();
      }
    }

    async function install() {
      if (!P.canInstall) {
        go('install');
        toast('info', 'Save the zip, then copy two files', 'The Install page shows where each one goes. The Windows app can do it for you.');
        return;
      }
      if (saving) return;
      if (state.pack && state.pack.kind === 'ypt') {
        toast('bad', "A loose core.ypt can't go in the mods folder", "Open the pack's .rpf instead, or save the zip and use the core.ypt in a resource.");
        return;
      }
      saving = true;
      try {
        const where = await P.fivem.detect();
        if (!where || !where.found) {
          go('install');
          toast('bad', "Couldn't find FiveM", 'Choose your FiveM.app folder on the Install page.');
          return;
        }
        const files = await buildFiles('install');
        if (!files) return;
        idle();
        const scan = await P.fivem.scan();
        const others = (scan.mods || []).filter((m) => m.enabled && m.hasCore !== false && m.name.toLowerCase() !== files.rpfName.toLowerCase());
        let disable = [];
        if (others.length) {
          const pick = await confirmBox({
            title: 'Other packs also change the tracer',
            text: 'FiveM only uses one core.ypt pack at a time. Turn these off so yours is the one that plays? They are renamed to .disabled, and you can turn them back on from the Install page.',
            items: others.map((m) => m.name + (m.hasCore === null ? ' (couldn’t check inside)' : '')),
            buttons: [{ label: 'Cancel', value: null, ghost: true }, { label: 'Leave them on', value: 'keep' }, { label: 'Turn them off and install', value: 'off', primary: true }],
          });
          if (!pick) return;
          if (pick === 'off') disable = others.map((m) => m.name);
        }
        busy('Installing…');
        const res = await P.fivem.install({ rpfName: files.rpfName, rpf: files.rpf, dat: files.dat, disable });
        savedAt = history.current;
        renderNote();
        changed('install');
        toast('ok', 'Installed. Start FiveM to see it.', [`${files.rpfName} → mods`, 'bloodfx.dat → citizen\\common\\data\\effects', res.backup ? 'Old files backed up' : null, disable.length ? `Turned off ${plural(disable.length, 'pack')}` : null].filter(Boolean).join(' · '),
          [{ label: 'Install page', run: () => go('install') }]);
      } catch (e) {
        toast('bad', "Couldn't install", e && e.message ? e.message : String(e));
      } finally {
        saving = false;
        idle();
      }
    }

    // ------------------------------------------------------------ toasts, busy, dialogs
    function toast(kind, text, small, actions, ms) {
      const iconName = kind === 'ok' ? 'check' : kind === 'bad' ? 'warn' : 'info';
      const el = h('div.toast.' + kind, { role: kind === 'bad' ? 'alert' : 'status' },
        h('span', { html: D.icon(iconName) }),
        h('div.msg', text, small ? h('small', small) : null),
        h('div.acts', (actions || []).map((a) => h('button.link', { type: 'button', onclick: () => { a.run(); el.remove(); } }, a.label)),
          h('button.icon-btn', { type: 'button', ariaLabel: 'Dismiss', title: 'Dismiss', style: { width: '24px', height: '24px' }, onclick: () => el.remove(), html: D.icon('x') })));
      const box = $('#toasts');
      box.append(el);
      while (box.children.length > 4) box.firstChild.remove();
      setTimeout(() => el.remove(), ms || (kind === 'bad' ? 12000 : 6000));
      return el;
    }
    function busy(text) { $('#busy-text').textContent = text || 'Working…'; $('#busy').hidden = false; }
    function idle() { $('#busy').hidden = true; }
    function confirmBox(o) {
      return new Promise((resolve) => {
        const prevFocus = document.activeElement;
        const buttons = o.buttons.map((b) => h('button.btn' + (b.primary ? '.primary' : '') + (b.ghost ? '.ghost' : ''), { type: 'button', onclick: () => done(b.value) }, b.label));
        const box = h('div.dialog', { role: 'dialog', attrs: { 'aria-modal': 'true', 'aria-labelledby': 'dlg-title' } },
          h('h3#dlg-title', o.title), o.text ? h('p', o.text) : null, o.items ? h('ul.dlg-list', o.items.map((i) => h('li', i))) : null, h('div.dlg-actions', buttons));
        const ov = h('div.overlay', box);
        function done(v) { ov.remove(); document.removeEventListener('keydown', onKey, true); if (prevFocus && prevFocus.focus) prevFocus.focus(); resolve(v); }
        function onKey(e) { if (e.key === 'Escape') { e.preventDefault(); e.stopPropagation(); done(null); } }
        document.addEventListener('keydown', onKey, true);
        document.body.append(ov);
        (buttons.find((b) => b.classList.contains('primary')) || buttons[0]).focus();
      });
    }

    // ------------------------------------------------------------ frame
    function setIcons(scope) {
      for (const el of $$('[data-icon]', scope)) { el.innerHTML = D.icon(el.dataset.icon); el.style.display = 'contents'; el.removeAttribute('data-icon'); }
    }
    $('#logo').innerHTML = D.LOGO;
    $('#ver').textContent = 'v' + VERSION;
    setIcons(document);
    document.body.classList.add(P.kind === 'desktop' ? 'platform-desktop' : 'platform-web');
    if (!P.canInstall) $('#tb-install-label').textContent = 'How to install';

    const tabsEl = $('#tabs');
    const tabButtons = new Map();
    for (const t of TABS) {
      const b = h('button', { type: 'button', role: 'tab', id: 'tab-' + t.id, title: `${t.label} (${TABS.indexOf(t) + 1})`, attrs: { 'aria-selected': 'false', 'aria-controls': 'page-' + t.id } },
        h('span', { html: D.icon(t.icon), style: { display: 'contents' } }), t.label, h('span.badge', { hidden: true }));
      b.addEventListener('click', () => go(t.id));
      tabsEl.append(b);
      tabButtons.set(t.id, b);
    }
    tabsEl.addEventListener('keydown', (e) => {
      if (e.key !== 'ArrowRight' && e.key !== 'ArrowLeft') return;
      const i = TABS.findIndex((t) => t.id === state.tab);
      const next = TABS[(i + (e.key === 'ArrowRight' ? 1 : TABS.length - 1)) % TABS.length];
      go(next.id);
      tabButtons.get(next.id).focus();
    });

    function renderTabs() {
      for (const t of TABS) {
        const b = tabButtons.get(t.id);
        const on = t.id === state.tab;
        b.setAttribute('aria-selected', on ? 'true' : 'false');
        b.tabIndex = on ? 0 : -1;
        const badge = b.querySelector('.badge');
        let n = '';
        if (t.id === 'blood') n = hitCount() ? String(hitCount()) : '';
        if (t.id === 'muzzle') n = muzzleChoice() ? String(muzzleChoice().names.length) : '';
        badge.hidden = !n;
        badge.textContent = n;
      }
    }
    function renderHead() {
      const t = TABS.find((x) => x.id === state.tab);
      $('#ph-icon').innerHTML = D.icon(t.icon);
      $('#ph-title').textContent = t.title;
      $('#ph-text').textContent = t.text;
      const actions = clear($('#ph-actions'));
      const page = pages.get(t.id);
      if (page && page.actions) U.append(actions, [page.actions]);
    }
    function renderCrumbs() {
      const c = clear($('#crumbs'));
      const t = TABS.find((x) => x.id === state.tab);
      c.append(h('span', state.pack ? state.pack.fileName : 'No pack'), h('span', { html: D.icon('chevron'), style: { display: 'contents' } }), h('b', t.label));
    }
    function renderFileChip() {
      const chip = $('#file-chip');
      const p = state.pack;
      chip.classList.toggle('empty', !p);
      $('#file-chip-tag').textContent = p ? (p.kind === 'ypt' ? 'YPT' : 'RPF') : 'RPF';
      $('#file-chip-name').textContent = p ? p.fileName : 'No tracer pack open';
      chip.title = p ? `${p.fileName} (${U.fmtBytes(p.size)}). Click to open a different pack (Ctrl+O)` : 'Open a tracer pack (Ctrl+O)';
    }
    function renderStatus() {
      const s = clear($('#statusbar'));
      const p = state.pack;
      s.append(h('span.item', h('span.dot' + (p ? '.ok' : '')), p ? `${p.fileName}${p.texture ? ` · ${p.texture.format} ${p.texture.w}×${p.texture.h}` : ''}` : 'No tracer pack open'));
      s.append(h('span.item', 'Tracer: ' + describeBeam(state.tracer) + ', ' + describeSmoke(state.tracer)));
      const n = hitCount();
      s.append(h('span.item' + (n ? '.changes' : ''), n ? `Blood: ${plural(n, 'hit')} changed` : 'Blood: as the starting file'));
      const m = muzzleChoice();
      if (m) s.append(h('span.item.changes', `Muzzle: ${m.names.length} recoloured`));
      s.append(h('span.grow'));
      s.append(h('span.item', PLATFORM_LABEL[P.kind]));
      s.append(h('span.item', h('kbd', 'Space'), 'fire', h('kbd', 'Ctrl S'), 'save'));
    }
    function renderNote() {
      const dirty = history.current !== savedAt && history.past.length > 0;
      $('#tb-note').textContent = dirty ? 'Unsaved changes' : '';
    }

    // ------------------------------------------------------------ pages
    const pages = new Map();
    const pagesEl = $('#pages');
    function go(id) {
      if (!pages.has(id)) return;
      const prev = state.tab;
      state.tab = id;
      for (const [pid, p] of pages) p.el.hidden = pid !== id;
      const page = pages.get(id);
      if (page.viewSlot) {
        page.viewSlot.append(viewport);
        requestAnimationFrame(() => { preview.resize(); preview.start(); });
      }
      if (prev !== id && pages.get(prev) && pages.get(prev).left) pages.get(prev).left();
      if (page.shown) page.shown();
      renderTabs();
      renderHead();
      renderCrumbs();
      renderHud();
      U.store.set('tab', id);
    }

    function changed(what) {
      if (what === 'pack' || what === 'tracer' || what === 'all') {
        const s = sheet();
        preview.setSheet(s.px, s.w, s.h);
      }
      for (const p of pages.values()) if (p.refresh) p.refresh(what);
      renderTabs();
      renderStatus();
      renderFileChip();
      renderCrumbs();
      renderHud();
      renderUndo();
      if (what === 'pack') renderHead();
    }

    function applyAccent(id) {
      const a = D.ACCENTS.find((x) => x.id === id) || D.ACCENTS[0];
      const [r, g, b] = U.rgbOf(a.value);
      document.documentElement.style.setProperty('--accent', a.value);
      document.documentElement.style.setProperty('--accent-rgb', `${r}, ${g}, ${b}`);
    }
    function setSetting(key, value) {
      state.settings[key] = value;
      U.store.set('settings', state.settings);
      if (key === 'accent') applyAccent(value);
      if (key === 'tips') document.body.classList.toggle('no-tips', !value);
      if (key === 'auto') preview.set('auto', value);
      changed('settings');
    }
    applyAccent(state.settings.accent);
    document.body.classList.toggle('no-tips', !state.settings.tips);

    async function startOver() {
      const ok = await confirmBox({ title: 'Start over?', text: 'Every tracer, blood and muzzle change goes back to how it started. The open pack stays open.', buttons: [{ label: 'Cancel', value: false, ghost: true }, { label: 'Start over', value: true, primary: true }] });
      if (!ok) return;
      Object.assign(state.tracer, { style: state.pack ? 'keep' : 'solid', colour: '#ff2bd6', colour2: null, flip: false, core: '#ffffff', width: 3, tintOn: false, tint: '#ffffff',
        smoke: state.pack ? 'keep' : 'off', smokeColour: '#8a6bff', smokeColour2: null, smokeFlip: false, smokeWidth: 3 }, state.packValues);
      state.touched.clear();
      state.lookTouched = false;
      state.tracerPreset = null;
      Object.assign(state.blood, { base: 'builtin', hits: {}, dead: true, splatOn: false, splat: '#00a0ff', strength: 50, splatter: 1, preset: null });
      Object.assign(state.muzzle, { enabled: false, match: 'muz', colour: '#3b82ff', glowOn: false, glow: 20, picked: null });
      enforceWhole();
      changed('all');
      commit();
    }

    const app = {
      C, D, U, K, P, A, VERSION, TABS, MOTION, BUILTIN_DAT, CORE_FX, LONG_FX, state,
      changed, commit, go, toast, busy, idle, confirmBox, setSetting, startOver,
      openPackDialog, loadPack, openDatDialog, loadDat, pickScreenshot, useScreenshot, openAny, save, install,
      look, sheet, sheetFor, whole, enforceWhole, tracerSettings, describeBeam, describeSmoke, applyTracerPreset, drawnSmoke,
      blood, baseText, baseSummary, splatTint, applyBloodPreset, assignHits, resetHits, hitCount, fxInfo, fxWarning, catalogue, validFx,
      packContents, muzzleRules, muzzlePicked, muzzleChoice,
      preview, viewport, target, renderHud, installText, zipName,
    };

    for (const t of TABS) {
      const make = pageFactories[t.id];
      if (!make) continue;
      const page = make(app);
      page.el.id = 'page-' + t.id;
      page.el.setAttribute('role', 'tabpanel');
      page.el.setAttribute('aria-labelledby', 'tab-' + t.id);
      page.el.hidden = true;
      pagesEl.append(page.el);
      pages.set(t.id, page);
    }

    // ------------------------------------------------------------ toolbar, keys, drops
    $('#tb-open').addEventListener('click', openPackDialog);
    $('#file-chip').addEventListener('click', openPackDialog);
    $('#tb-dat').addEventListener('click', openDatDialog);
    $('#tb-undo').addEventListener('click', undo);
    $('#tb-redo').addEventListener('click', redo);
    $('#tb-fire').addEventListener('click', () => preview.fire());
    $('#tb-save').addEventListener('click', save);
    $('#tb-install').addEventListener('click', install);

    document.addEventListener('keydown', (e) => {
      if (!$('#busy').hidden || document.querySelector('.dialog')) return;
      const typing = e.target.closest && e.target.closest('input:not([type=range]):not([type=checkbox]):not([type=color]), textarea, select, [contenteditable]');
      const mod = e.ctrlKey || e.metaKey;
      const k = e.key.toLowerCase();
      if (mod && !e.altKey) {
        if (k === 'o') { e.preventDefault(); openPackDialog(); }
        else if (k === 's') { e.preventDefault(); save(); }
        else if (k === 'i') { e.preventDefault(); install(); }
        else if (k === 'z' && !typing) { e.preventDefault(); if (e.shiftKey) redo(); else undo(); }
        else if (k === 'y' && !typing) { e.preventDefault(); redo(); }
        return;
      }
      if (typing || e.altKey) return;
      if (e.key === ' ' && !e.target.closest('button, a, input, select, [role=tab], [tabindex]')) { e.preventDefault(); preview.fire(); }
      else if (/^[1-7]$/.test(e.key) && !e.target.closest('input')) go(TABS[Number(e.key) - 1].id);
    });

    let dragDepth = 0;
    const hasFiles = (e) => e.dataTransfer && Array.from(e.dataTransfer.types || []).includes('Files');
    window.addEventListener('dragenter', (e) => { if (!hasFiles(e)) return; dragDepth++; $('#drop-overlay').hidden = false; });
    window.addEventListener('dragleave', (e) => { if (!hasFiles(e)) return; dragDepth = Math.max(0, dragDepth - 1); if (!dragDepth) $('#drop-overlay').hidden = true; });
    window.addEventListener('dragover', (e) => { if (hasFiles(e)) e.preventDefault(); });
    window.addEventListener('drop', (e) => {
      if (!hasFiles(e)) return;
      e.preventDefault();
      dragDepth = 0;
      $('#drop-overlay').hidden = true;
      const f = e.dataTransfer.files[0];
      if (f) openAny(f);
    });
    P.onOpenPath(async (path) => {
      const f = await P.readRecent(path);
      if (f) loadPack(f); else toast('bad', `Couldn't open ${path}`);
    });

    // ------------------------------------------------------------ boot
    applyBloodPreset(D.BLOOD_PRESETS.find((p) => p.id === 'confetti'));
    history.past.length = 0;
    history.current = snap();
    savedAt = history.current;
    changed('all');
    const firstTab = U.store.get('tab', 'home');
    go(pages.has(firstTab) ? firstTab : 'home');
    preview.start();
    root.bfxApp = app; // for tests and the console
  }

  root.BfxApp = { start, VERSION };
})(typeof self !== 'undefined' ? self : this);
