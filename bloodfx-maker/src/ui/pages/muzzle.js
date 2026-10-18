/* Muzzle flash workspace: recolour the pack's muzzle flash particles. Saved into the same tracer pack. */
(function (root) {
  'use strict';
  const pages = (root.BfxPages = root.BfxPages || {});

  pages.muzzle = function (app) {
    const { U, D, K, state } = app;
    const { h, clear, plural } = U;
    const m = state.muzzle;
    const ic = (name) => h('span', { html: D.icon(name), style: { display: 'contents' } });
    const change = () => { app.changed('muzzle'); };
    const done = () => { app.commit(); };

    // ------------------------------------------------------------ settings
    const enabled = K.toggle({ label: 'Recolour muzzle flashes', sub: 'Saved into the tracer pack with everything else', get: () => m.enabled, set: (v) => { m.enabled = v; }, onChange: () => { change(); done(); } });
    const rgbInputs = ['R', 'G', 'B'].map((c, i) => {
      const input = h('input', { type: 'number', min: 0, max: 255, step: 1, ariaLabel: c + ' (0 to 255)' });
      input.addEventListener('input', () => {
        const v = Math.max(0, Math.min(255, Math.round(Number(input.value) || 0)));
        const rgb = U.rgbOf(m.colour);
        rgb[i] = v;
        m.colour = U.toHex(rgb);
        colour.update();
        change();
      });
      input.addEventListener('change', done);
      return { input, el: h('label', h('i', { style: { background: ['#ff4d5e', '#2fd38a', '#3b82ff'][i] } }), input) };
    });
    const colour = K.colour({ label: 'Flash colour', get: () => m.colour, set: (v) => { m.colour = v; m.enabled = true; }, onInput: () => { syncRgb(); enabled.update(); change(); }, onCommit: done,
      swatches: ['#3b82ff', '#39e6ff', '#7dff6a', '#ff2bd6', '#8a6bff', '#ff2a3d', '#ffd23f', '#ffffff'] });
    const rgbRow = h('div.rgb-inputs', rgbInputs.map((x) => x.el), h('span'));
    function syncRgb() { const rgb = U.rgbOf(m.colour); rgbInputs.forEach((x, i) => { if (document.activeElement !== x.input) x.input.value = rgb[i]; }); }
    const glowOn = K.toggle({ label: 'Also set the glow', sub: 'Leave off to keep each flash’s own brightness', get: () => m.glowOn, set: (v) => { m.glowOn = v; }, onChange: () => { sync(); change(); done(); } });
    const glow = K.slider({ label: 'Glow', min: 0, max: 100, step: 1, fmt: (v) => v.toFixed(0), get: () => m.glow, set: (v) => { m.glow = v; }, onInput: change, onChange: done });
    const match = h('input.input.mono', { type: 'text', value: m.match, spellcheck: false, id: 'mz-match' });
    match.addEventListener('input', () => { m.match = match.value; m.picked = null; change(); });
    match.addEventListener('change', done);
    const noPack = h('div.note.info', 'Open your tracer pack to list its muzzle flashes. ', h('button.link', { type: 'button', onclick: () => app.openPackDialog() }, 'Open a pack'));
    const summary = h('div.field-help');
    const settings = h('div',
      h('div.section', noPack, enabled.el, colour.el, rgbRow, summary),
      h('div.section', h('h3', 'Glow'), glowOn.el, glow.el),
      h('div.section', h('h3', 'Which particles'), h('div.field', h('div.field-head', h('label', { htmlFor: 'mz-match' }, 'Names containing')), match,
        h('div.field-help', 'Muzzle flash particles start with muz_. Untick any you want to leave alone.'))),
      h('div.section', h('div.tip', ic('info'), h('span', 'Each flash keeps its own fade and transparency. Only the colour changes, so a flash that fades out still fades out.'))));
    const leftPanel = K.panel({ title: 'Muzzle flash colour', headId: 'mz-h', cls: 'area-insp', flush: true, scroll: true, body: [settings] });

    // ------------------------------------------------------------ particle table
    const tbody = h('tbody');
    const allBox = h('input.check', { type: 'checkbox', ariaLabel: 'Select every particle' });
    allBox.addEventListener('change', () => { m.picked = allBox.checked ? null : new Set(); m.enabled = m.enabled || allBox.checked; change(); done(); });
    const table = h('table.table', h('thead', h('tr', h('th', allBox), h('th', 'Particle'), h('th', 'Now'), h('th', 'After'), h('th', 'Keyframes'), h('th', 'Glow'))), tbody);
    const empty = h('div.empty-state', h('span', { html: D.icon('muzzle') }), h('b', 'No pack open'), h('span', 'Open your tracer pack to see its muzzle flashes.'),
      h('button.btn.primary', { type: 'button', onclick: () => app.openPackDialog() }, ic('open'), 'Open a pack'));
    const count = h('span.count');
    const listPanel = K.panel({ title: 'Particles', headId: 'mz-list-h', cls: 'area-list', flush: true, scroll: true, body: [table, empty],
      tools: [h('button.btn.ghost.small', { type: 'button', onclick: () => { m.picked = null; change(); done(); } }, 'Select all'),
        h('button.btn.ghost.small', { type: 'button', onclick: () => { m.picked = new Set(); change(); done(); } }, 'Select none')] });
    listPanel.head.querySelector('h2').after(count);

    function renderTable() {
      clear(tbody);
      const rules = app.muzzleRules();
      const picked = new Set(app.muzzlePicked().map((r) => r.name));
      const after = U.rgbOf(m.colour).map((c) => c / 255);
      table.hidden = !state.pack;
      empty.hidden = !!state.pack;
      count.textContent = state.pack ? `${picked.size} of ${rules.length}` : '';
      allBox.checked = !!rules.length && picked.size === rules.length;
      allBox.indeterminate = picked.size > 0 && picked.size < rules.length;
      for (const r of rules) {
        const on = picked.has(r.name);
        const box = h('input.check', { type: 'checkbox', checked: on, ariaLabel: 'Recolour ' + r.name });
        box.addEventListener('change', () => {
          const set = m.picked ? new Set(m.picked) : new Set(rules.map((x) => x.name));
          if (box.checked) set.add(r.name); else set.delete(r.name);
          m.picked = set;
          if (box.checked) m.enabled = true;
          change(); done();
        });
        const k = Math.max(r.now[0], r.now[1], r.now[2]);
        const afterCss = U.toHex(after.map((c) => c * k * 255));
        tbody.append(h('tr', h('td', box), h('td.mono', r.name),
          h('td', h('span.sw', { style: { display: 'inline-block', background: U.cssRgb01(r.now) }, title: r.now.map((x) => x.toFixed(2)).join(' ') })),
          h('td', m.enabled && on ? h('span.sw', { style: { display: 'inline-block', background: afterCss } }) : h('span.muted', 'unchanged')),
          h('td.mono', String(r.keyframes)),
          h('td.mono', m.glowOn && m.enabled && on ? `${r.emissive === null ? '–' : r.emissive.toFixed(1)} → ${m.glow}` : r.emissive === null ? '–' : r.emissive.toFixed(1))));
      }
      if (state.pack && !rules.length) tbody.append(h('tr', h('td', { colSpan: 6 }, h('span.muted', `No particles with colours match “${m.match}”.`))));
    }

    function sync() {
      enabled.update(); colour.update(); syncRgb(); glowOn.update(); glow.update();
      if (document.activeElement !== match) match.value = m.match;
      glow.el.hidden = !m.glowOn;
      noPack.hidden = !!state.pack;
      const n = app.muzzlePicked().length;
      summary.textContent = !state.pack ? '' : m.enabled ? `${plural(n, 'particle')} will be recoloured when you save or install.` : 'Off: the flashes stay as the pack has them.';
    }

    const el = h('section.page.ws-muzzle', leftPanel.el, listPanel.el);
    let dirty = true;
    function render() { sync(); renderTable(); dirty = false; }
    function refresh(what) {
      if (!['muzzle', 'pack', 'all'].includes(what)) return;
      dirty = true;
      if (!el.hidden) render();
    }
    return { el, actions: [], refresh, shown: () => { if (dirty) render(); } };
  };
})(typeof self !== 'undefined' ? self : this);
