/* Settings: accent colour, preview defaults, tips, start over, and what the maker can and can't do. */
(function (root) {
  'use strict';
  const pages = (root.BfxPages = root.BfxPages || {});

  pages.settings = function (app) {
    const { U, D, K, P, state } = app;
    const { h } = U;
    const ic = (name) => h('span', { html: D.icon(name), style: { display: 'contents' } });
    const s = state.settings;
    const row = (label, sub, ...ctrl) => h('div.set-row', h('div.k', label, sub ? h('span', sub) : null), h('div', ctrl));
    const panel = (title, ...body) => h('section.panel', h('div.panel-head', h('h2', title)), h('div.panel-body', body));

    const accents = h('div.accent-row', { role: 'group', ariaLabel: 'Accent colour' }, D.ACCENTS.map((a) => h('button', {
      type: 'button', title: a.label, ariaLabel: a.label, dataset: { v: a.id }, style: { background: a.value },
      onclick: () => { app.setSetting('accent', a.id); sync(); },
    })));
    const scene = K.miniSeg([['street', 'Street'], ['range', 'Range']], () => (s.scene === 'image' ? 'street' : s.scene), (v) => { app.setSetting('scene', v); app.preview.set('scene', v); }, 'Default scene');
    const camera = K.miniSeg([['side', 'Side'], ['shooter', 'Shooter']], () => s.camera, (v) => { app.setSetting('camera', v); app.preview.set('camera', v); }, 'Default camera');
    const auto = K.toggle({ label: 'Fire on its own', sub: 'Shoots every couple of seconds while the preview is on screen', get: () => s.auto, set: (v) => app.setSetting('auto', v) });
    const tips = K.toggle({ label: 'Show tips', sub: 'Short explanations next to the controls', get: () => s.tips, set: (v) => app.setSetting('tips', v) });
    const keys = [['Ctrl O', 'Open a tracer pack'], ['Ctrl S', 'Save the zip'], ['Ctrl I', P.canInstall ? 'Install to FiveM' : 'How to install'], ['Ctrl Z / Ctrl Y', 'Undo / redo'], ['Space', 'Fire a test shot'], ['1 – 7', 'Switch page']];

    const box = h('div.settings',
      panel('Look', row('Accent colour', 'Buttons, tabs and highlights', accents)),
      panel('Preview', row('Scene', 'Or drop a GTA screenshot on the window', scene.el), row('Camera', null, camera.el), h('div', { style: { paddingTop: '12px' } }, auto.el)),
      panel('Help', tips.el, h('table.table', { style: { marginTop: '12px' } }, h('tbody', keys.map(([k, v]) => h('tr', h('td', h('kbd', k)), h('td', v)))))),
      panel('Start over', h('div.set-row', h('div.k', 'Undo every change', h('span', 'Tracer, blood and muzzle flash go back to how they started. The open pack stays open.')),
        h('div', h('button.btn', { type: 'button', onclick: () => app.startOver() }, ic('reset'), 'Start over')))),
      panel('About', h('dl.facts',
        h('dt', 'Version'), h('dd', `BloodFX Maker ${app.VERSION} · ${P.kind === 'desktop' ? 'Windows app' : 'web page'}`),
        h('dt', 'Your files'), h('dd', 'Read and rebuilt on this computer. Nothing is uploaded.'),
        h('dt', 'Preview'), h('dd', "A sketch of the tracer and hit, drawn from the pack's real tracer sheet and timing. It isn't GTA's renderer: CodeWalker and OpenIV can't play particles either, so the only exact preview is the game."),
        h('dt', 'Recolouring'), h('dd', 'DXT1 and DXT5 tracer sheets keep their exact shape and transparency when recoloured. BC7 sheets (sleep, slow confetti, white confetti, xans) can be kept or redrawn, not recoloured.'),
        h('dt', 'Not tested in game'), h('dd', 'Every output is checked by reading it back, and was checked with CodeWalker on 12 real packs. Tell the maker’s author what you see in game.'))));
    const el = h('section.page-scroll.scroll', box);
    function sync() {
      for (const b of accents.children) b.setAttribute('aria-pressed', b.dataset.v === s.accent ? 'true' : 'false');
      scene.update(); camera.update(); auto.update(); tips.update();
    }
    sync();
    return { el, actions: [], refresh: (what) => { if (what === 'settings' || what === 'all') sync(); }, shown: sync };
  };
})(typeof self !== 'undefined' ? self : this);
