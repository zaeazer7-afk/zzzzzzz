/* Blood workspace: presets, the hit map (body part × weapon), and the effect browser for the selected hits. */
(function (root) {
  'use strict';
  const pages = (root.BfxPages = root.BfxPages || {});

  pages.blood = function (app) {
    const { U, D, K, A, state } = app;
    const { h, clear, plural } = U;
    const b = state.blood;
    const sel = state.selection;
    const ZONES = D.ZONES, WEAPONS = D.WEAPONS;
    const cellId = (z, w) => z + ':' + w;
    const ALL = ZONES.flatMap((z) => WEAPONS.map((w) => cellId(z.id, w.id)));
    const ic = (name) => h('span', { html: D.icon(name), style: { display: 'contents' } });
    const zoneLabel = (z) => ZONES.find((x) => x.id === z).label;
    const weaponLabel = (w) => WEAPONS.find((x) => x.id === w).label;

    // ------------------------------------------------------------ presets
    const presetCards = new Map();
    const presetBody = h('div');
    {
      const groups = new Map();
      for (const p of D.BLOOD_PRESETS) { if (!groups.has(p.group)) groups.set(p.group, []); groups.get(p.group).push(p); }
      for (const [g, items] of groups) {
        presetBody.append(h('div.list-group', h('h4', g), h('div.card-list', items.map((p) => {
          const kind = app.fxInfo(p.fx).kind;
          const btn = h('button.tpl', { type: 'button', title: p.sub, onclick: () => { app.applyBloodPreset(p); app.preview.fire('head'); } },
            h('span.thumb', h('span.fx', { html: D.fxIcon(kind) })), h('span', h('div.nm', p.name), h('div.sb', p.sub)));
          presetCards.set(p.id, btn);
          return btn;
        }))));
      }
    }
    const listPanel = K.panel({ title: 'Presets', headId: 'bl-list-h', count: D.BLOOD_PRESETS.length, cls: 'area-list', flush: true, scroll: true, body: [presetBody] });

    // ------------------------------------------------------------ viewport
    const sceneSeg = K.miniSeg([['street', 'Street'], ['range', 'Range'], ['image', 'Screenshot', 'Use your own GTA screenshot as the background']],
      () => app.preview.get('scene'), (v) => {
        if (v === 'image' && !app.preview.get('image')) { app.pickScreenshot().then(() => sceneSeg.update()); return; }
        app.preview.set('scene', v);
        if (v !== 'image') app.setSetting('scene', v);
      }, 'Scene');
    const fire = h('button.fire-btn', { type: 'button', title: 'Shoot the selected body part', onclick: () => shoot() }, ic('fire'), 'Fire', h('kbd', 'Space'));
    const viewPanel = K.panel({ title: 'Preview', headId: 'bl-view-h', cls: 'area-view', tools: [h('div.vp-tools', sceneSeg.el, fire)] });
    viewPanel.body.remove();
    function aim() {
      const first = [...sel][0];
      const [zone, weapon] = first ? first.split(':') : ['head', 'guns'];
      app.target.zone = zone;
      app.target.weapon = weapon;
      app.preview.set('focus', zone === 'animals' ? 'body' : zone);
      app.renderHud();
    }
    function shoot() { aim(); app.preview.fire(app.target.zone === 'animals' ? 'body' : app.target.zone || 'head'); }

    // ------------------------------------------------------------ hit map
    const map = h('div.hitmap', { role: 'grid', ariaLabel: 'Hits: body part by weapon type' });
    const cellEls = new Map();
    const corner = h('button.hm-col', { type: 'button', title: 'Select every hit', onclick: (e) => selectCells(ALL, e) }, 'All');
    map.append(h('div.hm-corner', corner));
    for (const w of WEAPONS) {
      map.append(h('button.hm-col', { type: 'button', title: `Select ${w.label.toLowerCase()} (${w.sub})`, onclick: (e) => selectCells(ZONES.map((z) => cellId(z.id, w.id)), e) }, ic(w.id), w.label));
    }
    for (const z of ZONES) {
      map.append(h('button.hm-row', { type: 'button', title: `Select every ${z.label.toLowerCase()} hit (${z.sub})`, onclick: (e) => selectCells(WEAPONS.map((w) => cellId(z.id, w.id)), e) }, ic(z.id), z.label));
      for (const w of WEAPONS) {
        const id = cellId(z.id, w.id);
        const icon = h('span.ic');
        const lb = h('span.lb'), more = h('span.more');
        const cell = h('button.hm-cell', { type: 'button', dataset: { cell: id }, attrs: { 'aria-pressed': 'false' } }, icon, h('span.txt', lb, more));
        cellEls.set(id, { cell, icon, lb, more });
        map.append(cell);
      }
    }
    // Click selects one hit; Ctrl-click adds or removes; Shift-click or a drag selects a rectangle.
    let anchor = 'head:guns', dragging = false, pressing = false;
    const rect = (a, c) => {
      const [za, wa] = a.split(':'), [zc, wc] = c.split(':');
      const zi = [ZONES.findIndex((z) => z.id === za), ZONES.findIndex((z) => z.id === zc)].sort();
      const wi = [WEAPONS.findIndex((w) => w.id === wa), WEAPONS.findIndex((w) => w.id === wc)].sort();
      const out = [];
      for (let i = zi[0]; i <= zi[1]; i++) for (let j = wi[0]; j <= wi[1]; j++) out.push(cellId(ZONES[i].id, WEAPONS[j].id));
      return out;
    };
    const setSel = (ids) => { sel.clear(); for (const id of ids) sel.add(id); };
    map.addEventListener('pointerdown', (e) => {
      const c = e.target.closest('.hm-cell');
      if (!c || e.button !== 0) return;
      if (e.target.hasPointerCapture && e.target.hasPointerCapture(e.pointerId)) e.target.releasePointerCapture(e.pointerId);
      e.preventDefault();
      c.focus();
      pressing = true;
      const id = c.dataset.cell;
      if (e.ctrlKey || e.metaKey) { if (sel.has(id) && sel.size > 1) sel.delete(id); else sel.add(id); anchor = id; }
      else if (e.shiftKey) setSel(rect(anchor, id));
      else { setSel([id]); anchor = id; dragging = true; }
      selectionChanged(false);
    });
    map.addEventListener('pointermove', (e) => {
      if (!dragging) return;
      const over = document.elementFromPoint(e.clientX, e.clientY);
      const c = over && over.closest('.hm-cell');
      if (!c) return;
      const next = rect(anchor, c.dataset.cell);
      if (next.join() !== [...sel].join()) { setSel(next); selectionChanged(false); }
    });
    window.addEventListener('pointerup', () => {
      if (!pressing) return;
      pressing = false;
      dragging = false;
      selectionChanged(true);
    });
    map.addEventListener('keydown', (e) => {
      const c = e.target.closest('.hm-cell');
      if (!c) return;
      const [z, w] = c.dataset.cell.split(':');
      let zi = ZONES.findIndex((x) => x.id === z), wi = WEAPONS.findIndex((x) => x.id === w);
      if (e.key === 'ArrowRight') wi = Math.min(WEAPONS.length - 1, wi + 1);
      else if (e.key === 'ArrowLeft') wi = Math.max(0, wi - 1);
      else if (e.key === 'ArrowDown') zi = Math.min(ZONES.length - 1, zi + 1);
      else if (e.key === 'ArrowUp') zi = Math.max(0, zi - 1);
      else if (e.key === 'Enter' || e.key === ' ') {
        e.preventDefault();
        const id = c.dataset.cell;
        if (e.ctrlKey || e.metaKey) { if (sel.has(id) && sel.size > 1) sel.delete(id); else sel.add(id); } else setSel([id]);
        anchor = id;
        selectionChanged(true);
        return;
      } else return;
      e.preventDefault();
      const next = cellId(ZONES[zi].id, WEAPONS[wi].id);
      cellEls.get(next).cell.focus();
      if (e.shiftKey) { setSel(rect(anchor, next)); selectionChanged(true); }
    });
    function selectCells(ids, e) {
      if (e && (e.ctrlKey || e.metaKey)) for (const id of ids) sel.add(id); else setSel(ids);
      anchor = ids[0];
      selectionChanged(true);
    }
    function selectionChanged(final) {
      renderMap();
      renderSelection();
      aim();
      if (final) shoot();
    }
    const selCount = h('span.count');
    const mapPanel = K.panel({ title: 'Hits', headId: 'bl-map-h', cls: 'area-map', scroll: true, body: [h('div.map-scroll', map)],
      tools: [h('span.muted', { style: { fontSize: '11.5px' } }, 'Click a hit, drag for several. Ctrl-click to add.')] });
    mapPanel.head.querySelector('h2').after(selCount);

    function renderMap() {
      const built = app.blood();
      const now = built.summary || built.base;
      for (const z of ZONES) {
        for (const w of WEAPONS) {
          const id = cellId(z.id, w.id);
          const { cell, icon, lb, more } = cellEls.get(id);
          const names = now[z.id][w.id] || [];
          const first = names[0] || '-';
          const info = app.fxInfo(first);
          icon.innerHTML = D.fxIcon(names.length ? info.kind : 'none');
          lb.textContent = names.length ? (first === '-' ? 'Nothing' : info.label) : 'Not used';
          const a = b.hits[id];
          more.textContent = names.length > 1 ? `+${names.length - 1} more` : a ? (a.armour ? 'changed' : 'changed · not armour') : names.length ? first : '';
          cell.title = `${z.label} hit by ${w.label.toLowerCase()}: ${names.join(', ') || 'no effect in this file'}`;
          cell.classList.toggle('changed', !!a);
          cell.setAttribute('aria-pressed', sel.has(id) ? 'true' : 'false');
        }
      }
      selCount.textContent = sel.size > 1 ? `${sel.size} selected` : '';
    }

    // ------------------------------------------------------------ inspector: effect tab
    const selHead = h('div.sel-head', h('b'), h('span'));
    const selNote = h('div.note');
    const query = h('input', { type: 'search', placeholder: 'Search effects, e.g. confetti or bul_', ariaLabel: 'Search effects', spellcheck: false });
    const chips = h('div.chips', { role: 'group', ariaLabel: 'Effect category' });
    const fxList = h('div.fx-list');
    const moreBtn = h('button.btn.small.wide', { type: 'button', hidden: true });
    const custom = h('input.input.mono', { type: 'text', placeholder: 'any_effect_name', ariaLabel: 'Effect name', spellcheck: false });
    const customBtn = h('button.btn.small', { type: 'button' }, 'Use');
    let filter = 'suggested', limit = 120;
    query.addEventListener('input', () => { limit = 120; renderList(); });
    moreBtn.addEventListener('click', () => { limit += 300; renderList(); });
    const useCustom = () => {
      const v = custom.value.trim();
      if (!app.validFx(v)) { app.toast('bad', 'Effect names use letters, numbers and underscores only'); return; }
      pickFx(v);
    };
    customBtn.addEventListener('click', useCustom);
    custom.addEventListener('keydown', (e) => { if (e.key === 'Enter') useCustom(); });

    function pickFx(id) {
      app.assignHits([...sel], { fx: id });
      const info = app.fxInfo(id);
      const warn = app.fxWarning(info);
      if (warn) app.toast('info', `${info.label} on ${describeSel()}`, warn);
      shoot();
    }
    function describeSel() {
      if (!sel.size) return 'nothing';
      if (sel.size === 1) { const [z, w] = [...sel][0].split(':'); return `${zoneLabel(z)} · ${weaponLabel(w)}`; }
      if (sel.size === ALL.length) return 'every hit';
      const zones = new Set([...sel].map((c) => c.split(':')[0])), weapons = new Set([...sel].map((c) => c.split(':')[1]));
      if (zones.size === 1 && sel.size === WEAPONS.length) return `${zoneLabel([...zones][0])} · every weapon`;
      if (weapons.size === 1 && sel.size === ZONES.length) return `every body part · ${weaponLabel([...weapons][0])}`;
      return plural(sel.size, 'hit');
    }
    function selFx() {
      const built = app.blood();
      const now = built.summary || built.base;
      const names = new Set();
      for (const c of sel) { const [z, w] = c.split(':'); for (const n of now[z][w] || []) names.add(n); }
      return [...names];
    }

    function renderChips() {
      clear(chips);
      const cat = app.catalogue();
      const counts = new Map();
      for (const f of cat) counts.set(f.cat, (counts.get(f.cat) || 0) + 1);
      const make = (id, label, n) => h('button.chip', { type: 'button', attrs: { 'aria-pressed': filter === id ? 'true' : 'false' }, onclick: () => { filter = id; limit = 120; renderChips(); renderList(); } }, label, n !== undefined ? h('span.n', String(n)) : null);
      chips.append(make('suggested', 'Suggested', D.EFFECTS.length), make('all', state.pack ? 'All in pack' : 'All', cat.length));
      for (const [key] of D.CATEGORIES) if (counts.get(key)) chips.append(make(key, D.CATEGORY_LABEL[key], counts.get(key)));
      if (counts.get('other')) chips.append(make('other', D.CATEGORY_LABEL.other, counts.get('other')));
    }
    function renderList() {
      clear(fxList);
      const q = query.value.trim().toLowerCase();
      const current = selFx();
      let items = app.catalogue();
      if (!q && filter === 'suggested') items = items.filter((f) => f.curated);
      else if (!q && filter !== 'all') items = items.filter((f) => f.cat === filter);
      if (q) items = items.filter((f) => f.id.toLowerCase().includes(q) || f.label.toLowerCase().includes(q));
      let lastGroup = null;
      for (const f of items.slice(0, limit)) {
        if (!q && filter === 'suggested' && f.group !== lastGroup) {
          lastGroup = f.group;
          fxList.append(h('div.fx-group', D.GROUPS[f.group] || ''));
        }
        const warn = app.fxWarning(f);
        let badge = null;
        if (f.id !== '-' && state.pack && !f.inPack) badge = h('span.badge', 'Not in pack');
        else if (f.id !== '-' && !state.pack && !f.standard) badge = h('span.badge', 'Needs a pack');
        else if (f.loops) badge = h('span.badge', 'Loops');
        else if (f.note) badge = h('span.badge.ok', f.note);
        fxList.append(h('button.fx-item' + (current.length === 1 && current[0] === f.id ? '.on' : ''), { type: 'button', title: warn || f.id, onclick: () => pickFx(f.id) },
          h('span.ic', { html: D.fxIcon(f.kind) }),
          h('span', { style: { minWidth: '0' } }, h('div.nm', f.curated || f.id === '-' ? f.label : f.id), h('div.id', f.curated ? f.id : D.CATEGORY_LABEL[f.cat] || '')),
          badge));
      }
      if (!items.length) fxList.append(h('p.field-help', q ? `No effect matches “${query.value.trim()}”. Type its exact name below to use it anyway.` : 'Nothing in this category.'));
      moreBtn.hidden = items.length <= limit;
      moreBtn.textContent = `Show ${Math.min(300, items.length - limit)} more of ${items.length}`;
    }

    const armour = K.toggle({ label: 'Same with body armour and underwater', sub: 'Otherwise armour and water hits keep their own effect',
      get: () => { const a = [...sel].map((c) => b.hits[c]).filter(Boolean); return a.length ? a.every((x) => x.armour) : true; },
      set: () => {}, onChange: (v) => { app.assignHits([...sel], { armour: v }); } });
    const SIZES = [[null, 'Keep'], [0.2, 'Small'], [0.5, 'Medium'], [0.8, 'Large'], [1, 'Biggest']];
    const size = K.seg({
      label: 'Effect size', options: SIZES.map(([v, label]) => ({ v: String(v), label })),
      get: () => { const a = [...sel].map((c) => b.hits[c]).filter(Boolean); const s = new Set(a.map((x) => String(x.size))); return s.size === 1 ? [...s][0] : a.length ? '' : 'null'; },
      set: (v) => { app.assignHits([...sel], { size: v === 'null' ? null : Number(v) }); },
      help: 'How big the effect plays. Keep leaves the size the starting file has.',
    });
    const resetSel = h('button.btn.small', { type: 'button', onclick: () => { app.resetHits([...sel]); shoot(); } }, ic('reset'), 'Back to the starting file');
    const effectTab = h('div',
      h('div.section', selHead, selNote),
      h('div.section', h('h3', 'Play this effect'), h('label.search', ic('search'), query), chips, fxList, moreBtn,
        h('div.field', h('div.field-head', h('label', { htmlFor: 'bl-custom' }, 'Or type any effect name')), h('div.colour-row', Object.assign(custom, { id: 'bl-custom' }), customBtn))),
      h('div.section', h('h3', 'Options'), armour.el, size.el, h('div', resetSel)));

    function renderSelection() {
      const names = selFx();
      selHead.firstChild.textContent = describeSel();
      selHead.lastChild.textContent = !sel.size ? 'Pick a hit on the map' : names.length === 0 ? 'No effect plays here in this file' : names.length === 1 ? `Plays ${names[0] === '-' ? 'nothing' : app.fxInfo(names[0]).label} (${names[0]})` : `Plays ${plural(names.length, 'different effect')}`;
      const warns = names.map((n) => app.fxWarning(app.fxInfo(n))).filter(Boolean);
      selNote.hidden = !warns.length;
      selNote.textContent = warns[0] || '';
      armour.update();
      size.update();
      resetSel.disabled = ![...sel].some((c) => b.hits[c]);
      renderList();
    }

    // ------------------------------------------------------------ inspector: splats tab
    const splatOn = K.toggle({ label: 'Change the splat colour', sub: 'The blood decals left on walls and the ground', get: () => b.splatOn, set: (v) => { b.splatOn = v; }, onChange: () => { syncSplats(); app.changed('blood'); app.commit(); } });
    const splat = K.colour({ label: 'Splat colour', get: () => b.splat, set: (v) => { b.splat = v; b.splatOn = true; }, onInput: () => { splatOn.update(); syncSplats(); app.changed('blood'); }, onCommit: () => app.commit(),
      swatches: ['#00a0ff', '#7dff6a', '#ff2bd6', '#ffd23f', '#ffffff', '#8a6bff', '#ff9a4d', '#1c1a17'] });
    const strength = K.slider({ label: 'Strength', min: 10, max: 255, step: 1, fmt: (v) => v.toFixed(0), get: () => b.strength, set: (v) => { b.strength = v; },
      help: 'GTA uses 50 (dark red 50 0 0). Higher is brighter.', onInput: () => { syncSplats(); app.changed('blood'); }, onChange: () => app.commit() });
    const splatOut = h('div.field-help.mono');
    const splatter = K.slider({ label: 'Splatter size', min: 0.25, max: 3, step: 0.05, fmt: (v) => '×' + v.toFixed(2), get: () => b.splatter, set: (v) => { b.splatter = v; },
      changed: () => b.splatter !== 1, marks: () => [{ v: 1, title: 'GTA stock' }], help: 'Scales the splats and sprays every hit leaves.', onInput: () => app.changed('blood'), onChange: () => app.commit() });
    const dead = K.toggle({ label: 'Also on dead bodies', sub: 'Shots at a body that is already down use the same effects', get: () => b.dead, set: (v) => { b.dead = v; }, onChange: () => { app.changed('blood'); app.commit(); } });
    function syncSplats() {
      splatOn.update(); splat.update(); strength.update(); splatter.update(); dead.update();
      splat.el.hidden = strength.el.hidden = splatOut.hidden = !b.splatOn;
      splatOut.textContent = b.splatOn ? 'Written as ' + app.splatTint().join(' ') : '';
    }
    const splatTab = h('div', h('div.section', h('h3', 'Splats'), splatOn.el, splat.el, strength.el, splatOut, splatter.el), h('div.section', h('h3', 'Dead bodies'), dead.el));

    // ------------------------------------------------------------ inspector: file tab
    const baseSeg = K.seg({
      label: 'Start from', options: [{ v: 'builtin', label: 'Built-in file' }, { v: 'file', label: 'My bloodfx.dat' }],
      get: () => b.base, set: (v) => { if (v === 'file' && !b.fileText) { app.openDatDialog().then(() => syncFile()); return; } b.base = v; app.changed('blood'); app.commit(); },
    });
    const fileInfo = h('div.field-help');
    const loadBtn = h('button.btn.small', { type: 'button', onclick: () => app.openDatDialog() }, ic('open'), 'Load a bloodfx.dat');
    const issues = h('div.note.bad');
    function syncFile() {
      baseSeg.update();
      clear(fileInfo);
      if (b.base === 'file' && b.fileText) fileInfo.append(`Using ${b.fileName}. Your hit changes are applied on top of it.`, b.fileProblems.length ? ` ${plural(b.fileProblems.length, 'row')} with an unusual layout are left as they are.` : '');
      else fileInfo.append('The built-in file has the layout every one of your packs uses, with normal blood everywhere.');
      const built = app.blood();
      const list = built.error ? [built.error] : built.issues;
      issues.hidden = !list.length;
      issues.textContent = list.length ? list[0] + (list.length > 1 ? ` (and ${list.length - 1} more)` : '') : '';
    }
    const fileTab = h('div', h('div.section', h('h3', 'Starting file'), baseSeg.el, fileInfo, h('div', loadBtn), issues),
      h('div.section', h('h3', 'How it works'), h('p.explain', 'bloodfx.dat picks an effect for every body part and weapon type. Rows in it are shared between hits, so when you change only some of them the maker copies that row first. Nothing else changes.')));

    // ------------------------------------------------------------ inspector frame
    let tab = 'effect';
    const tabs = K.miniSeg([['effect', 'Effect'], ['splats', 'Splats'], ['file', 'File']], () => tab, (v) => { tab = v; showTab(); }, 'Hit settings');
    const inspPanel = K.panel({ title: 'Hit settings', headId: 'bl-insp-h', cls: 'area-insp', flush: true, scroll: true, body: [effectTab, splatTab, fileTab], tools: [tabs.el] });
    function showTab() { effectTab.hidden = tab !== 'effect'; splatTab.hidden = tab !== 'splats'; fileTab.hidden = tab !== 'file'; inspPanel.body.scrollTop = 0; }

    function markPreset() { for (const [id, btn] of presetCards) btn.classList.toggle('on', b.preset === id); }

    // ------------------------------------------------------------ page
    const el = h('section.page.ws-blood', listPanel.el, viewPanel.el, mapPanel.el, inspPanel.el);
    const actions = [
      h('button.btn.ghost.small', { type: 'button', onclick: () => app.openDatDialog() }, ic('open'), 'Open bloodfx.dat'),
      h('button.btn.ghost.small', { type: 'button', onclick: () => { app.applyBloodPreset(D.BLOOD_PRESETS.find((p) => p.id === 'normal')); } }, ic('reset'), 'Reset every hit'),
    ];
    let dirty = true;
    function render() {
      renderMap(); renderChips(); renderSelection(); syncSplats(); syncFile(); markPreset();
      dirty = false;
    }
    function refresh(what) {
      if (what === 'view' || what === 'settings') { sceneSeg.update(); return; }
      if (!['blood', 'pack', 'all'].includes(what)) return;
      dirty = true;
      if (!el.hidden) render();
    }
    showTab();
    return { el, actions, viewSlot: viewPanel.el, refresh, shown: () => { sceneSeg.update(); aim(); if (dirty) render(); }, left: () => { app.target.zone = null; app.target.weapon = 'guns'; app.preview.set('focus', null); } };
  };
})(typeof self !== 'undefined' ? self : this);
