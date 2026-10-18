/* Pack explorer: what's inside the pack's core.ypt (effects, textures, files) and the tracer's own numbers. */
(function (root) {
  'use strict';
  const pages = (root.BfxPages = root.BfxPages || {});

  pages.explorer = function (app) {
    const { U, D, K, A, C, state } = app;
    const { h, clear, plural } = U;
    const ic = (name) => h('span', { html: D.icon(name), style: { display: 'contents' } });

    const stats = h('div.stats.area-stats');
    let view = 'effects';
    const VIEWS = [['effects', 'Effects', 'list'], ['textures', 'Textures', 'image'], ['files', 'Files', 'folder'], ['tracer', 'Tracer details', 'tracer']];
    const navButtons = VIEWS.map(([id, label, icon]) => {
      const b = h('button.nav-item', { type: 'button', onclick: () => { view = id; render(); } }, ic(icon), h('span', label), h('span.n'));
      b.dataset.v = id;
      return b;
    });
    const navPanel = K.panel({ title: 'Browse', headId: 'ex-nav-h', cls: 'area-nav', body: [h('div.nav-list', navButtons)] });
    const mainBody = h('div');
    const mainTools = h('div.tools');
    const mainPanel = K.panel({ title: 'Effects', headId: 'ex-main-h', cls: 'area-main', flush: true, scroll: true, body: [mainBody] });
    mainPanel.head.append(mainTools);
    const title = mainPanel.head.querySelector('h2');

    const empty = h('div.empty-state.area-main', h('span', { html: D.icon('explorer') }), h('b', 'No pack open'),
      h('span', 'Open a tracer pack to look inside its core.ypt.'),
      h('button.btn.primary', { type: 'button', onclick: () => app.openPackDialog() }, ic('open'), 'Open a pack'));

    // ------------------------------------------------------------ effects
    let q = '', cat = 'all', limit = 250;
    const search = h('input', { type: 'search', placeholder: 'Search effects', ariaLabel: 'Search effects', spellcheck: false });
    search.addEventListener('input', () => { q = search.value.trim().toLowerCase(); limit = 250; renderMain(); });
    const catSel = h('select.input', { ariaLabel: 'Category', style: { width: 'auto' } });
    catSel.addEventListener('change', () => { cat = catSel.value; limit = 250; renderMain(); });
    function renderEffects() {
      const c = app.packContents();
      if (!catSel.options.length || catSel.dataset.pack !== String(state.pack.id)) {
        clear(catSel);
        const counts = new Map();
        for (const e of c.effects) { const k = D.categoryOf(e.name); counts.set(k, (counts.get(k) || 0) + 1); }
        catSel.append(h('option', { value: 'all' }, `Every category (${c.effects.length})`));
        for (const [k, label] of [...D.CATEGORIES.map(([k2, l]) => [k2, l]), ['other', D.CATEGORY_LABEL.other]]) if (counts.get(k)) catSel.append(h('option', { value: k }, `${label} (${counts.get(k)})`));
        catSel.dataset.pack = String(state.pack.id);
        catSel.value = cat;
      }
      mainTools.append(h('label.search', { style: { width: '220px' } }, ic('search'), search), catSel);
      let list = c.effects;
      if (cat !== 'all') list = list.filter((e) => D.categoryOf(e.name) === cat);
      if (q) list = list.filter((e) => e.name.toLowerCase().includes(q));
      const sel = [...state.selection];
      const target = sel.length === 1 ? sel[0].replace(':', ' · ') : plural(sel.length, 'selected hit');
      const rows = list.slice(0, limit).map((e) => {
        const loops = e.loops !== 0 || e.duration > 4;
        return h('tr', h('td.mono', e.name), h('td', h('span.tag', D.CATEGORY_LABEL[D.categoryOf(e.name)])),
          h('td', loops ? h('span.tag.warn', e.loops < 0 ? 'Loops' : e.loops > 0 ? `Repeats ${e.loops}×` : 'Long') : h('span.tag.ok', 'Once')),
          h('td.mono', e.duration.toFixed(2) + ' s'), h('td.mono', String(e.emitters)),
          h('td', h('button.btn.small', { type: 'button', title: `Play ${e.name} on ${target} (Blood page selection)`, onclick: () => useOnHits(e.name) }, 'Use on hits')));
      });
      mainBody.append(h('table.table', h('thead', h('tr', h('th', 'Effect'), h('th', 'Category'), h('th', 'Plays'), h('th', 'Duration'), h('th', 'Emitters'), h('th', ''))), h('tbody', rows)));
      if (list.length > limit) mainBody.append(h('div', { style: { padding: '12px' } }, h('button.btn.small', { type: 'button', onclick: () => { limit += 500; renderMain(); } }, `Show more (${list.length - limit} left)`)));
      if (!list.length) mainBody.append(h('p.field-help', { style: { padding: '14px' } }, 'No effects match.'));
      title.textContent = 'Effects';
    }
    function useOnHits(name) {
      const sel = [...state.selection];
      app.assignHits(sel, { fx: name });
      const info = app.fxInfo(name);
      app.toast('ok', `${name} now plays on ${sel.length === 1 ? sel[0].replace(':', ' · ') : plural(sel.length, 'hit')}`, app.fxWarning(info) || 'Pick which hits on the Blood page.', [{ label: 'Go to Blood', run: () => app.go('blood') }]);
    }

    // ------------------------------------------------------------ textures
    let observer = null;
    function renderTextures() {
      const c = app.packContents();
      title.textContent = 'Textures';
      const grid = h('div.tex-grid', { style: { padding: '12px' } });
      if (observer) observer.disconnect();
      observer = 'IntersectionObserver' in root ? new IntersectionObserver((entries) => {
        for (const en of entries) if (en.isIntersecting) { observer.unobserve(en.target); paint(en.target); }
      }, { root: mainPanel.body, rootMargin: '200px' }) : null;
      for (const t of c.textures) {
        const box = h('div.img');
        box._tex = t;
        const card = h('div.tex', box, h('div.cap', h('b', { title: t.name }, t.name), h('span', t.broken ? 'unreadable' : `${t.formatName} ${t.width}×${t.height}`)));
        grid.append(card);
        if (observer) observer.observe(box); else paint(box);
      }
      mainBody.append(grid);
    }
    function paint(box) {
      const t = box._tex;
      if (t.broken) { box.append(h('span.none', 'Unreadable')); return; }
      let px = null;
      try { px = C.decodeTexture(state.pack.y, t); } catch { px = null; }
      if (!px) { box.append(h('span.none', `${t.formatName}: no preview`)); return; }
      box.append(A.textureCanvas(px, t.width, t.height, 132));
    }

    // ------------------------------------------------------------ files
    function renderFiles() {
      const p = state.pack;
      title.textContent = 'Files';
      if (p.kind === 'ypt') { mainBody.append(h('p.field-help', { style: { padding: '14px' } }, `${p.fileName} is a loose core.ypt, not a pack.`)); return; }
      const rows = p.files.map((f) => h('tr', h('td.mono', f.path), h('td', h('span.tag' + (/core\.ypt$/i.test(f.path) ? '.accent' : ''), f.nested ? 'pack inside the pack' : f.kind === 'rsc' ? 'resource' : 'file')), h('td.mono', U.fmtBytes(f.size))));
      mainBody.append(h('table.table', h('thead', h('tr', h('th', 'Path'), h('th', 'Kind'), h('th', 'Stored size'))), h('tbody', rows)));
      mainBody.append(h('p.field-help', { style: { padding: '12px 14px' } }, `The maker rewrites ${p.paths.join(', ')} and copies everything else unchanged.`));
    }

    // ------------------------------------------------------------ tracer details
    function renderTracer() {
      const p = state.pack, t = p.tracer;
      title.textContent = 'Tracer details';
      const hex = (v) => '0x' + (v >>> 0).toString(16).toUpperCase().padStart(8, '0');
      const kf = (list) => (list && list.length ? list.map((k) => `${k.t[0].toFixed(2)}: ${k.v.map((x) => x.toFixed(2)).join(' ')}`).join(' · ') : '–');
      const rows = [
        ['Effect', `bullet_tracer · duration ${t.effect.duration.map((x) => x.toFixed(2)).join('–')} s · playback ×${t.effect.rate.map((x) => x.toFixed(2)).join('–')}`],
        ['Beam emitter', `${t.beam.emitterRule} → ${t.beam.particleRule} · rate ×${t.beam.rate[0].toFixed(2)} · thickness ×${t.beam.zoom[0].toFixed(2)} · tint ${hex(t.beam.tint[0])}`],
        ['Beam far size', t.beam.lodSize ? t.beam.lodSize.value.slice(0, 3).map((x) => x.toFixed(0)).join(' ') : '–'],
        ['Beam life', t.life ? kf(t.life.keyframes) : '–'],
        ['Beam colour', t.beamColour ? kf(t.beamColour.rgbaMax) : '–'],
        ['Beam glow', t.beamColour ? kf(t.beamColour.emissive) : '–'],
        ['Beam frames', t.beamFrames ? t.beamFrames.join('–') : '–'],
        ['Smoke emitter', t.smoke ? `${t.smoke.emitterRule} → ${t.smoke.particleRule} · tint ${hex(t.smoke.tint[0])}` : 'none'],
        ['Smoke life', t.smokeLife ? kf(t.smokeLife.keyframes) : '–'],
        ['Smoke colour', t.smokeColour ? kf(t.smokeColour.rgbaMax) : '–'],
        ['Sheet', p.texture ? `ptfx_bullet_tracer · ${p.texture.format} ${p.texture.w}×${p.texture.h} · ${p.texture.levels} mip levels` : 'missing'],
        ['core.ypt', p.kind === 'ypt' ? 'loose file' : p.paths.join(', ')],
      ];
      mainBody.append(h('dl.facts', { style: { padding: '14px' } }, rows.flatMap(([k, v]) => [h('dt', k), h('dd.mono', v)])));
    }

    function renderStats() {
      clear(stats);
      const c = app.packContents();
      const loops = c.effects.filter((e) => e.loops !== 0).length;
      const tile = (n, label) => h('div.stat', h('b', String(n)), h('span', label));
      stats.append(tile(c.effects.length, 'effects'), tile(loops, 'loop forever'), tile(c.particles, `particle rules · ${c.emitters} emitters`), tile(c.textures.length, 'textures'));
      const counts = { effects: c.effects.length, textures: c.textures.length, files: state.pack.files.length || 1, tracer: '' };
      for (const b of navButtons) {
        b.setAttribute('aria-pressed', b.dataset.v === view ? 'true' : 'false');
        b.querySelector('.n').textContent = String(counts[b.dataset.v]);
      }
    }
    function renderMain() {
      clear(mainBody);
      clear(mainTools);
      mainPanel.body.scrollTop = 0;
      if (view === 'effects') renderEffects();
      else if (view === 'textures') renderTextures();
      else if (view === 'files') renderFiles();
      else renderTracer();
    }

    const el = h('section.page.ws-explorer', stats, navPanel.el, mainPanel.el, empty);
    let dirty = true;
    function render() {
      const has = !!state.pack;
      stats.hidden = navPanel.el.hidden = mainPanel.el.hidden = !has;
      empty.hidden = has;
      if (has) { renderStats(); renderMain(); }
      dirty = false;
    }
    function refresh(what) {
      if (what !== 'pack' && what !== 'all' && !(what === 'blood' && view === 'effects')) return;
      dirty = true;
      if (!el.hidden && what !== 'blood') render();
    }
    return { el, actions: [], refresh, shown: () => { if (dirty) render(); } };
  };
})(typeof self !== 'undefined' ? self : this);
