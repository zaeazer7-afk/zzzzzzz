/* Home: what the maker does, the three steps, quick starts, recent packs and the open pack's facts. */
(function (root) {
  'use strict';
  const pages = (root.BfxPages = root.BfxPages || {});

  pages.home = function (app) {
    const { U, D, A, C, P, state } = app;
    const { h, clear, plural } = U;
    const ic = (name) => h('span', { html: D.icon(name), style: { display: 'contents' } });

    const heroArt = h('canvas.hero-art', { attrs: { 'aria-hidden': 'true' } });
    const openBtn = h('button.btn.primary.big', { type: 'button', onclick: () => app.openPackDialog().then((ok) => { if (ok) app.go('tracer'); }) }, ic('open'), 'Open a tracer pack');
    const hero = h('div.hero', heroArt,
      h('h2', 'Custom tracers and hit effects for FiveM'),
      h('p', 'Start from a tracer pack you already have. Recolour its own beam and smoke, or draw new ones, then choose what plays when a bullet hits: confetti, cash, sparks, gore or anything in the game. Your files never leave this computer.'),
      h('div.actions', openBtn,
        h('button.btn.big', { type: 'button', onclick: () => app.go('tracer') }, ic('tracer'), 'Tracer templates'),
        h('button.btn.big', { type: 'button', onclick: () => app.go('blood') }, ic('drop'), 'Blood effects')));

    const step = (n, title, text) => h('div.step', h('span.num', 'STEP ' + n), h('h3', title), h('p', text));
    const steps = h('div.steps-row',
      step(1, 'Open your tracer pack', 'The .rpf from your FiveM mods folder, like bluetrace.rpf. The maker finds core.ypt inside it.'),
      step(2, 'Pick a look and hit effects', 'Use a template or set the beam, smoke, speed and glow yourself. Then pick what plays on each body part.'),
      step(3, app.P.canInstall ? 'Install or save' : 'Save the zip', app.P.canInstall ? 'Install to FiveM copies both files into place and backs up the old ones. Or save a zip.' : 'One zip holds the new pack and bloodfx.dat, with the steps to install them.'));

    // quick starts: four tracer templates and four blood presets
    const quickGrid = h('div.quick-grid');
    const QUICK_T = ['pack-recolour', 'laser', 'slow-beam', 'nebula'];
    const QUICK_B = ['confetti', 'cash', 'gore', 'party'];
    const quickCanvases = [];
    for (const id of QUICK_T) {
      const p = D.TRACER_PRESETS.find((x) => x.id === id);
      const canvas = h('canvas');
      quickCanvases.push({ canvas, p });
      quickGrid.append(h('button.quick-card', { type: 'button', onclick: () => { app.applyTracerPreset(p); app.go('tracer'); } }, h('span.thumb', canvas), h('b', p.name), h('span', p.sub)));
    }
    for (const id of QUICK_B) {
      const p = D.BLOOD_PRESETS.find((x) => x.id === id);
      quickGrid.append(h('button.quick-card', { type: 'button', onclick: () => { app.applyBloodPreset(p); app.go('blood'); } }, h('span.thumb', h('span.fx', { html: D.fxIcon(app.fxInfo(p.fx).kind) })), h('b', p.name), h('span', p.sub)));
    }
    function drawQuick() {
      for (const { canvas, p } of quickCanvases) {
        const tt = { ...state.tracer, colour2: null, flip: false, smokeColour2: null, smokeFlip: false, tintOn: false, ...p.set };
        app.enforceWhole(tt);
        A.tracerThumb(canvas, { sheet: app.sheetFor(tt), glowColour: tt.style === 'keep' ? '#ffb070' : tt.colour, glow: tt.glow, zoom: tt.zoom, smoke: tt.smoke !== 'off', beam: tt.style !== 'none' }, { w: 160, h: 56, width: 20 });
      }
    }

    const recentBox = h('div.recent-list');
    const recentPanel = h('section.panel', h('div.panel-head', h('h2', 'Recent packs')), h('div.panel-body', recentBox));
    async function renderRecents() {
      const list = await P.recents().catch(() => []);
      clear(recentBox);
      recentPanel.hidden = !P.canInstall;
      if (!list.length) { recentBox.append(h('p.field-help', 'Packs you open show up here.')); return; }
      for (const r of list.slice(0, 6)) {
        recentBox.append(h('button', { type: 'button', title: r.path, onclick: async () => { const f = await P.readRecent(r.path); if (f) { if (await app.loadPack(f)) app.go('tracer'); } else app.toast('bad', `Couldn't open ${r.name}`, 'It may have been moved or deleted.'); } },
          ic('folder'), h('span', { style: { minWidth: '0' } }, h('div.nm', r.name), h('div.pt', r.path)), h('span.muted', { style: { fontSize: '11px' } }, r.when ? new Date(r.when).toLocaleDateString() : '')));
      }
    }

    const factsBox = h('div');
    const factsPanel = h('section.panel', h('div.panel-head', h('h2', 'Open pack')), h('div.panel-body', factsBox));
    function renderFacts() {
      clear(factsBox);
      const p = state.pack;
      if (!p) {
        factsBox.append(h('div.empty-state.compact', h('span', { html: D.icon('open') }), h('b', 'No pack open yet'), h('span', 'Drop a .rpf anywhere on this window, or use Open a tracer pack.')));
        return;
      }
      const v = state.packValues;
      const rows = [
        ['File', `${p.fileName} · ${U.fmtBytes(p.size)}`],
        ['core.ypt', p.kind === 'ypt' ? 'loose file' : p.paths.join(', ')],
        ['Tracer sheet', p.texture ? `${p.texture.format} ${p.texture.w}×${p.texture.h}${p.recolourable ? ' · can be recoloured' : ' · keep whole or redraw'}` : 'none'],
        ['Beam', `glow ${v.glow} · thickness ×${v.zoom} · speed ×${v.speed}`],
        ['Timing', `stays ${v.stay} s · beam life ${v.life} s · far size ${v.far}`],
        ['Effects', plural(p.effects.size, 'effect')],
      ];
      factsBox.append(h('dl.facts', rows.flatMap(([k, val]) => [h('dt', k), h('dd', val)])));
    }

    const lower = h('div.home-cols', factsPanel, recentPanel);
    const home = h('div.home', hero, steps, h('div.home-section', h('h3.home-h', 'Quick starts'), quickGrid), lower);
    const el = h('section.page-scroll.scroll', home);
    let dirty = true;
    function render() { drawQuick(); renderFacts(); renderRecents(); dirty = false; }
    new ResizeObserver(() => { if (!el.hidden) A.hero(heroArt, C.drawTracerSheet); }).observe(heroArt);
    function refresh(what) {
      if (!['pack', 'all'].includes(what)) return;
      dirty = true;
      if (!el.hidden) render();
    }
    return { el, actions: [], refresh, shown: () => { A.hero(heroArt, C.drawTracerSheet); if (dirty) render(); else renderRecents(); } };
  };
})(typeof self !== 'undefined' ? self : this);
