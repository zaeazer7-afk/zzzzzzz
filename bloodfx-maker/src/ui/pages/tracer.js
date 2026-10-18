/* Tracer workspace: templates, live preview, the tracer sheet, and the beam / smoke / timing inspector. */
(function (root) {
  'use strict';
  const pages = (root.BfxPages = root.BfxPages || {});

  pages.tracer = function (app) {
    const { U, D, K, A, state } = app;
    const { h, clear } = U;
    const t = state.tracer;
    const DRAWN = ['solid', 'neon', 'twin'];

    // ------------------------------------------------------------ templates
    const listBody = h('div');
    const search = h('input', { type: 'search', placeholder: 'Search templates', ariaLabel: 'Search templates', spellcheck: false });
    const listPanel = K.panel({ title: 'Templates', headId: 'tpl-h', count: D.TRACER_PRESETS.length, cls: 'area-list', flush: true, scroll: true,
      body: [h('div', { style: { padding: '10px 10px 0' } }, h('label.search', h('span', { html: D.icon('search'), style: { display: 'contents' } }), search)), listBody] });
    const cards = new Map();
    const groupEls = [];
    {
      const groups = new Map();
      for (const p of D.TRACER_PRESETS) { if (!groups.has(p.group)) groups.set(p.group, []); groups.get(p.group).push(p); }
      for (const [g, items] of groups) {
        const grid = h('div.card-list');
        for (const p of items) {
          const canvas = h('canvas');
          const b = h('button.tpl', { type: 'button', title: p.sub, onclick: () => app.applyTracerPreset(p) },
            h('span.thumb', canvas), h('span', h('div.nm', p.name), h('div.sb', p.sub), p.from ? h('div.from', 'Like ' + p.from) : null));
          cards.set(p.id, { b, canvas, p, text: `${p.name} ${p.sub} ${p.group} ${p.from || ''}`.toLowerCase() });
          grid.append(b);
        }
        const el = h('div.list-group', h('h4', g), grid);
        groupEls.push({ el, items });
        listBody.append(el);
      }
    }
    search.addEventListener('input', () => {
      const q = search.value.trim().toLowerCase();
      for (const c of cards.values()) c.b.hidden = !!q && !c.text.includes(q);
      for (const g of groupEls) g.el.hidden = g.items.every((p) => cards.get(p.id).b.hidden);
    });
    function drawThumbs() {
      for (const { canvas, p } of cards.values()) {
        const tt = { ...state.tracer, colour2: null, flip: false, smokeColour2: null, smokeFlip: false, tintOn: false, ...p.set };
        app.enforceWhole(tt);
        A.tracerThumb(canvas, { sheet: app.sheetFor(tt), glowColour: tt.style === 'keep' ? '#ffb070' : tt.colour, glow: tt.glow, zoom: tt.zoom, smoke: tt.smoke !== 'off', beam: tt.style !== 'none' });
      }
    }
    function markTemplate() {
      for (const c of cards.values()) c.b.classList.toggle('on', state.tracerPreset === c.p.id);
    }

    // ------------------------------------------------------------ viewport
    const sceneSeg = K.miniSeg([['street', 'Street'], ['range', 'Range'], ['image', 'Screenshot', 'Use your own GTA screenshot as the background']],
      () => app.preview.get('scene'), (v) => setScene(v), 'Scene');
    function setScene(v) {
      if (v === 'image' && !app.preview.get('image')) {
        app.pickScreenshot().then(() => sceneSeg.update());
        return;
      }
      app.preview.set('scene', v);
      if (v !== 'image') app.setSetting('scene', v);
    }
    const cameraSeg = K.miniSeg([['side', 'Side'], ['shooter', 'Shooter']], () => app.preview.get('camera'), (v) => { app.preview.set('camera', v); app.setSetting('camera', v); }, 'Camera');
    const slow = K.miniToggle('Slow-mo', 'slow', () => app.preview.get('slowmo'), (v) => app.preview.set('slowmo', v), 'Play at a quarter speed');
    const pause = K.miniToggle('Pause', 'pause', () => app.preview.get('paused'), (v) => app.preview.set('paused', v), 'Freeze the preview');
    const fire = h('button.fire-btn', { type: 'button', title: 'Fire a shot', onclick: () => app.preview.fire() }, h('span', { html: D.icon('fire'), style: { display: 'contents' } }), 'Fire', h('kbd', 'Space'));
    const viewPanel = K.panel({ title: 'Preview', headId: 'tr-view-h', cls: 'area-view', tools: [h('div.vp-tools', sceneSeg.el, cameraSeg.el, slow.el, pause.el, fire)] });
    viewPanel.body.remove();

    // ------------------------------------------------------------ tracer sheet
    const sheetBody = h('div.sheets');
    const sheetCount = h('span.count');
    const sheetPanel = K.panel({ title: 'Tracer sheet', headId: 'tr-sheet-h', cls: 'area-sheet', scroll: true, body: [sheetBody],
      tools: [h('span.muted', { style: { fontSize: '11.5px' } }, 'ptfx_bullet_tracer')] });
    sheetPanel.head.querySelector('h2').after(sheetCount);
    function strips(title, sub, px, w, hh) {
      const full = A.sheetCanvas({ px, w, h: hh });
      const fw = w / 4;
      const rows = [0, 1, 2, 3].map((f) => {
        const c = h('canvas', { width: hh, height: fw });
        const x = c.getContext('2d');
        // the sheet's height runs along the bullet's path, so each frame is drawn lying down
        x.translate(0, fw);
        x.rotate(-Math.PI / 2);
        x.drawImage(full, f * fw, 0, fw, hh, 0, 0, fw, hh);
        return h('div.strip' + (f === 3 ? '.beam' : ''), h('span', f === 3 ? 'Beam' : 'Smoke ' + (f + 1)), c);
      });
      return h('div.sheet', h('div.sheet-title', title, sub ? h('span.mono', sub) : null), h('div.strips', rows));
    }
    function renderSheets() {
      clear(sheetBody);
      const p = state.pack;
      const s = app.sheet();
      sheetCount.textContent = p && p.texture ? `${p.texture.format} ${p.texture.w}×${p.texture.h}` : '128×256';
      if (p && p.preview) sheetBody.append(strips('In the pack', p.fileName, p.preview, p.texture.w, p.texture.h));
      else if (p) sheetBody.append(h('div.sheet-empty', p.texture ? `${p.texture.format} sheets can't be previewed. Pick a drawn beam and smoke to replace it.` : 'This pack has no tracer sheet.'));
      const unchanged = p && p.preview && s.px === p.preview;
      sheetBody.append(strips(p ? (unchanged ? 'After (unchanged)' : 'After your changes') : 'Your sheet', s.standIn ? 'stand-in until a pack is open' : '', s.px, s.w, s.h));
    }

    // ------------------------------------------------------------ inspector
    const changedLook = () => { state.lookTouched = true; state.tracerPreset = null; };
    const live = () => app.changed('tracer');
    const done = () => { app.commit(); app.preview.soon(); };
    const pick = (mutate) => () => { mutate(); changedLook(); app.enforceWhole(); sync(); live(); done(); };

    const styleSeg = K.seg({
      label: 'Look', wrap: true,
      options: [
        { v: 'keep', label: "Pack's", art: A.BEAM_ART.keep, title: "Keep the pack's beam" },
        { v: 'recolour', label: 'Recolour', art: A.BEAM_ART.recolour, title: "The pack's beam in your colour", disabled: () => app.whole() },
        { v: 'none', label: 'None', art: A.BEAM_ART.none, title: 'No beam, smoke only' },
        { v: 'solid', label: 'Solid', art: A.BEAM_ART.solid, title: 'A flat line' },
        { v: 'neon', label: 'Neon', art: A.BEAM_ART.neon, title: 'Colour around a thin bright core' },
        { v: 'twin', label: 'Twin', art: A.BEAM_ART.twin, title: 'Two thin lines' },
      ],
      get: () => t.style,
      set: (v) => { t.style = v; },
      onPick: pick(() => {}),
      help: (v) => ({
        keep: state.pack ? "Keeps the pack's beam exactly as it is." : "Keeps the pack's beam. Open a pack to see it.",
        recolour: "Keeps the pack's beam shape and fade, painted in your colour.",
        none: 'No beam. Only the smoke trail shows.',
        solid: 'A flat line in your colour.',
        neon: 'Your colour around a thin bright core.',
        twin: 'Two thin lines side by side.',
      })[v],
    });
    const colour = K.colour({ label: 'Colour', get: () => t.colour, set: (v) => { t.colour = v; changedLook(); }, onInput: live, onCommit: done });
    const fadeToggle = K.toggle({ label: 'Fade to a second colour', sub: 'Changes colour along the tracer', get: () => !!t.colour2, set: (v) => { t.colour2 = v ? t.colour2Last || '#38d9ff' : null; if (!v) t.colour2Last = t.colour2; changedLook(); }, onChange: () => { sync(); live(); done(); } });
    const flipBtn = h('button.btn.small', { type: 'button', title: 'Swap which end gets which colour' }, h('span', { html: D.icon('flip'), style: { display: 'contents' } }), 'Flip');
    flipBtn.addEventListener('click', () => { t.flip = !t.flip; changedLook(); live(); done(); });
    const colour2 = K.colour({ label: 'Second colour', get: () => t.colour2 || '#38d9ff', set: (v) => { t.colour2 = v; changedLook(); }, onInput: live, onCommit: done, extra: flipBtn, noQuick: true });
    const core = K.colour({ label: 'Core colour', help: 'The thin centre line. White looks hottest.', get: () => t.core, set: (v) => { t.core = v; changedLook(); }, onInput: live, onCommit: done, swatches: ['#ffffff', '#fff6d6', '#e6f6ff', '#ffe2f6'] });
    const width = K.slider({ label: 'Width', min: 1, max: 16, step: 1, fmt: (v) => v + ' px', get: () => t.width, set: (v) => { t.width = v; changedLook(); }, onInput: live, onChange: done, help: 'Pixels across the 32-pixel frame.' });
    const tintToggle = K.toggle({ label: "Tint the pack's colours", sub: 'Multiplies the beam and smoke by a colour', get: () => t.tintOn, set: (v) => { t.tintOn = v; changedLook(); }, onChange: () => { sync(); live(); done(); } });
    const tint = K.colour({ label: 'Tint', get: () => t.tint, set: (v) => { t.tint = v; t.tintOn = true; changedLook(); }, onInput: live, onCommit: done });

    const smokeSeg = K.seg({
      label: 'Look', wrap: true,
      options: [
        { v: 'keep', label: "Pack's", art: A.SMOKE_ART.keep, title: "Keep the pack's smoke", disabled: () => app.whole() },
        { v: 'recolour', label: 'Recolour', art: A.SMOKE_ART.recolour, title: "The pack's smoke in your colour", disabled: () => app.whole() },
        { v: 'off', label: 'Off', art: A.SMOKE_ART.off, title: 'No smoke trail' },
        { v: 'line', label: 'Straight', art: A.SMOKE_ART.line, title: 'A crisp line with a ragged edge' },
        { v: 'wavy', label: 'Wavy', art: A.SMOKE_ART.wavy, title: 'A wavy line' },
      ],
      get: () => t.smoke,
      set: (v) => { t.smoke = v; },
      onPick: pick(() => {}),
      help: (v) => ({
        keep: state.pack ? "Keeps the pack's smoke exactly as it is." : "Keeps the pack's smoke. Open a pack to see it.",
        recolour: "Keeps the pack's smoke shape and fade, painted in your colour. Nothing else about it changes.",
        off: 'No smoke trail.',
        line: 'A crisp line with a ragged edge that flickers like smoke.',
        wavy: 'A wavy line that drifts as it fades.',
      })[v],
    });
    const smokeColour = K.colour({ label: 'Smoke colour', get: () => t.smokeColour, set: (v) => { t.smokeColour = v; changedLook(); }, onInput: live, onCommit: done });
    const smokeFade = K.toggle({ label: 'Fade to a second colour', sub: 'Changes colour along the trail', get: () => !!t.smokeColour2, set: (v) => { t.smokeColour2 = v ? t.smokeColour2Last || '#ffffff' : null; if (!v) t.smokeColour2Last = t.smokeColour2; changedLook(); }, onChange: () => { sync(); live(); done(); } });
    const smokeFlip = h('button.btn.small', { type: 'button', title: 'Swap which end gets which colour' }, h('span', { html: D.icon('flip'), style: { display: 'contents' } }), 'Flip');
    smokeFlip.addEventListener('click', () => { t.smokeFlip = !t.smokeFlip; changedLook(); live(); done(); });
    const smokeColour2 = K.colour({ label: 'Second colour', get: () => t.smokeColour2 || '#ffffff', set: (v) => { t.smokeColour2 = v; changedLook(); }, onInput: live, onCommit: done, extra: smokeFlip, noQuick: true });
    const smokeWidth = K.slider({ label: 'Width', min: 1, max: 12, step: 1, fmt: (v) => v + ' px', get: () => t.smokeWidth, set: (v) => { t.smokeWidth = v; changedLook(); }, onInput: live, onChange: done });

    const SLIDERS = {
      glow: { label: 'Glow', min: 0, max: 60, step: 1, fmt: (v) => v.toFixed(0), help: 'How brightly the beam lights up. GTA uses 15.' },
      zoom: { label: 'Thickness', min: 0.2, max: 4, step: 0.05, fmt: (v) => '×' + v.toFixed(2), help: 'Scales the whole tracer up or down.' },
      far: { label: 'Size far away', min: 50, max: 1500, step: 10, fmt: (v) => v.toFixed(0), help: 'How big the tracer stays at long range. GTA uses 200.' },
      speed: { label: 'Speed', min: 0.1, max: 2, step: 0.05, fmt: (v) => '×' + v.toFixed(2), help: 'Lower is a slower beam. ×0.5 is half speed.' },
      stay: { label: 'Stays for', min: 0.05, max: 3, step: 0.05, fmt: (v) => v.toFixed(2) + ' s', help: 'How long the tracer effect lasts. GTA uses 0.1 s.' },
      life: { label: 'Beam life', min: 0.02, max: 0.5, step: 0.01, fmt: (v) => v.toFixed(2) + ' s', help: 'How long each beam particle lives. Longer is a longer streak.' },
    };
    const sliders = Object.entries(SLIDERS).map(([k, o]) => K.slider({
      ...o,
      get: () => t[k],
      set: (v) => { t[k] = v; state.touched.add(k); },
      changed: () => state.touched.has(k),
      marks: () => [{ v: D.STOCK[k], title: 'GTA stock' }, state.pack ? { v: state.packValues[k], kind: 'pack', title: "The pack's value" } : null].filter(Boolean),
      onInput: live,
      onChange: done,
    }));
    const resetMotion = h('button.link', { type: 'button', onclick: () => { Object.assign(t, state.packValues); state.touched.clear(); state.tracerPreset = null; sync(); live(); done(); } }, 'Reset');

    const bc7Note = h('div.note');
    const noPack = h('div.note.info', "Open your tracer pack to keep or recolour its own beam and smoke. Until then you're drawing a new sheet. ",
      h('button.link', { type: 'button', onclick: () => app.openPackDialog() }, 'Open a pack'));
    const nothingNote = h('div.note', 'No beam and no smoke: the tracer will be invisible.');
    const smokeKeptWhole = h('div.note.info', "The smoke is kept with the pack's beam, because this pack's sheet can only be kept whole.");

    const beamSection = K.section('Beam', null, styleSeg.el, colour.el, fadeToggle.el, colour2.el, core.el, width.el, tintToggle.el, tint.el);
    const smokeSection = K.section('Smoke trail', null, smokeSeg.el, smokeKeptWhole, smokeColour.el, smokeFade.el, smokeColour2.el, smokeWidth.el, nothingNote);
    const motionSection = K.section(null, null, h('h3', 'Timing and glow', resetMotion), ...sliders.map((s) => s.el),
      h('div.marks-legend', h('span', h('i'), 'GTA stock'), h('span', h('i.pack'), "pack's value")));
    const notes = h('div.section', { style: { gap: '8px' } }, noPack, bc7Note);
    const inspBody = h('div', notes, beamSection, smokeSection, motionSection);
    const resetAll = h('button.btn.ghost.small', { type: 'button', title: "Back to the pack's own tracer" }, h('span', { html: D.icon('reset'), style: { display: 'contents' } }), 'Reset');
    resetAll.addEventListener('click', () => {
      Object.assign(t, { style: state.pack ? 'keep' : 'solid', smoke: state.pack ? 'keep' : 'off', colour2: null, smokeColour2: null, flip: false, smokeFlip: false, tintOn: false }, state.packValues);
      state.touched.clear();
      state.lookTouched = false;
      state.tracerPreset = null;
      app.enforceWhole();
      sync(); live(); done();
    });
    const inspPanel = K.panel({ title: 'Tracer settings', headId: 'tr-insp-h', cls: 'area-insp', flush: true, scroll: true, body: [inspBody], tools: [resetAll] });

    function sync() {
      const whole = app.whole();
      styleSeg.update(); smokeSeg.update();
      colour.update(); colour2.update(); core.update(); width.update(); tint.update(); fadeToggle.update(); tintToggle.update();
      smokeColour.update(); smokeColour2.update(); smokeWidth.update(); smokeFade.update();
      for (const s of sliders) s.update();
      const drawn = DRAWN.includes(t.style), smokeDrawn = app.drawnSmoke(t.smoke);
      colour.el.hidden = !(drawn || t.style === 'recolour');
      fadeToggle.el.hidden = !drawn;
      colour2.el.hidden = !(drawn && t.colour2);
      flipBtn.setAttribute('aria-pressed', t.flip ? 'true' : 'false');
      core.el.hidden = t.style !== 'neon';
      width.el.hidden = !drawn;
      tintToggle.el.hidden = !(t.style === 'keep' && t.smoke === 'keep');
      tint.el.hidden = tintToggle.el.hidden || !t.tintOn;
      smokeSeg.el.hidden = whole && t.style === 'keep';
      smokeKeptWhole.hidden = !(whole && t.style === 'keep');
      smokeColour.el.hidden = !(smokeDrawn || t.smoke === 'recolour');
      smokeFade.el.hidden = !smokeDrawn;
      smokeColour2.el.hidden = !(smokeDrawn && t.smokeColour2);
      smokeFlip.setAttribute('aria-pressed', t.smokeFlip ? 'true' : 'false');
      smokeWidth.el.hidden = !smokeDrawn;
      nothingNote.hidden = !(t.style === 'none' && t.smoke === 'off');
      noPack.hidden = !!state.pack;
      bc7Note.hidden = !whole;
      if (whole) bc7Note.textContent = `${state.pack.fileName} stores its tracer sheet as ${state.pack.texture ? state.pack.texture.format : 'an unknown format'}, so it can be kept as it is or redrawn, but not recoloured in part.`;
      notes.hidden = noPack.hidden && bc7Note.hidden;
    }

    // ------------------------------------------------------------ page
    const el = h('section.page.ws-tracer', listPanel.el, viewPanel.el, sheetPanel.el, inspPanel.el);
    const actions = [
      h('button.btn.ghost.small', { type: 'button', onclick: () => app.pickScreenshot().then(() => sceneSeg.update()) }, h('span', { html: D.icon('image'), style: { display: 'contents' } }), 'Use a screenshot'),
    ];
    let dirty = { sheets: true, thumbs: true };
    function refresh(what) {
      if (what === 'view' || what === 'settings') { sceneSeg.update(); cameraSeg.update(); return; }
      if (!['tracer', 'pack', 'all'].includes(what)) return;
      sync();
      markTemplate();
      dirty.sheets = true;
      if (what !== 'tracer') dirty.thumbs = true;
      if (!el.hidden) flush();
    }
    function flush() {
      if (dirty.thumbs) { drawThumbs(); dirty.thumbs = false; }
      if (dirty.sheets) { renderSheets(); dirty.sheets = false; }
    }
    sync();
    markTemplate();
    return { el, actions, viewSlot: viewPanel.el, refresh, shown: () => { sceneSeg.update(); cameraSeg.update(); slow.update(); pause.update(); flush(); } };
  };
})(typeof self !== 'undefined' ? self : this);
