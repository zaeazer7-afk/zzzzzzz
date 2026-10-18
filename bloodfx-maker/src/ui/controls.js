/* BloodFX Maker controls: panels, sliders, segmented pickers, colour pickers and switches bound to state. */
(function (root) {
  'use strict';
  const { h, hexOk, normHex } = root.BfxUtil;
  const D = root.BfxData;

  let seq = 0;
  const uid = (p) => `${p}-${++seq}`;

  const icon = (name, cls) => h('span' + (cls ? '.' + cls : ''), { html: D.icon(name), attrs: { 'aria-hidden': 'true' }, style: { display: 'contents' } });

  function panel(o) {
    const head = h('div.panel-head', h('h2', { id: o.headId }, o.title), o.count !== undefined ? h('span.count', o.count) : null, o.tools ? h('div.tools', o.tools) : null);
    const body = h('div.panel-body' + (o.flush ? '.flush' : '') + (o.scroll ? '.scroll' : ''), o.body || []);
    const el = h('section.panel' + (o.cls ? '.' + o.cls.split(' ').join('.') : ''), { attrs: { 'aria-labelledby': o.headId } }, head, body);
    return { el, head, body };
  }

  function section(title, explain, ...kids) {
    const el = h('div.section', title ? h('h3', title) : null, explain ? h('p.explain', explain) : null, ...kids);
    return el;
  }

  function fieldHead(label, forId, extra) {
    return h('div.field-head', h('label', { htmlFor: forId }, label), extra || null);
  }

  // A slider with a value pill and optional stock / pack marks under the track.
  function slider(o) {
    const id = uid('r');
    const input = h('input', { type: 'range', id, min: o.min, max: o.max, step: o.step });
    const pill = h('output.pill', { htmlFor: id, attrs: { 'aria-live': 'off' } });
    const track = h('div.track', input);
    const help = o.help ? h('div.field-help', o.help) : null;
    const el = h('div.field', fieldHead(o.label, id, pill), track, help);
    const fill = () => {
      const v = Number(input.value), lo = Number(o.min), hi = Number(o.max);
      input.style.setProperty('--fill', `${((v - lo) / (hi - lo)) * 100}%`);
    };
    const show = (v) => { pill.textContent = o.fmt ? o.fmt(v) : String(v); fill(); };
    input.addEventListener('input', () => { const v = Number(input.value); o.set(v); show(v); pill.classList.toggle('changed', !!(o.changed && o.changed())); if (o.onInput) o.onInput(v); });
    input.addEventListener('change', () => { if (o.onChange) o.onChange(Number(input.value)); });
    const marks = [];
    function update() {
      const v = o.get();
      input.value = v;
      show(v);
      pill.classList.toggle('changed', !!(o.changed && o.changed()));
      for (const m of marks) m.remove();
      marks.length = 0;
      if (o.marks) {
        const lo = Number(o.min), hi = Number(o.max);
        for (const m of o.marks()) {
          if (m.v === undefined || m.v === null || m.v < lo || m.v > hi) continue;
          // the thumb travels between its own radius at either end
          const k = (m.v - lo) / (hi - lo);
          const mk = h('i.mark' + (m.kind === 'pack' ? '.pack' : ''), { title: m.title || '', style: { left: `calc(${k * 100}% + ${7.5 - k * 15}px)` } });
          marks.push(mk);
          track.append(mk);
        }
      }
    }
    update();
    return { el, input, update };
  }

  // Segmented picker. options: [{ v, label, art, title }]
  function seg(o) {
    const id = uid('s');
    const box = h('div.seg' + (o.wrap ? '.wrap' : ''), { role: 'group', attrs: { 'aria-labelledby': id } });
    const buttons = o.options.map((opt) => {
      const b = h('button', { type: 'button', title: opt.title || '', dataset: { v: opt.v } }, opt.art ? h('span.art', opt.art()) : null, h('span', opt.label));
      b.addEventListener('click', () => { if (b.disabled) return; o.set(opt.v); update(); if (o.onPick) o.onPick(opt.v); });
      box.append(b);
      return { b, opt };
    });
    const label = o.label ? h('div.field-head', h('span.lbl', { id }, o.label), o.extra || null) : null;
    const help = o.help ? h('div.field-help') : null;
    const el = h('div.field', label, box, help);
    function update() {
      const v = o.get();
      for (const { b, opt } of buttons) {
        b.setAttribute('aria-pressed', opt.v === v ? 'true' : 'false');
        b.disabled = !!(opt.disabled && opt.disabled());
      }
      if (help) help.textContent = typeof o.help === 'function' ? o.help(v) : o.help;
    }
    update();
    return { el, update, buttons };
  }

  // Colour picker: swatch (native picker), hex box and quick swatches.
  function colour(o) {
    const id = uid('c');
    const picker = h('input', { type: 'color', id, attrs: { 'aria-label': o.label + ' picker' } });
    const chip = h('span');
    const sw = h('label.swatch-btn', { title: 'Pick a colour' }, chip, picker);
    const hex = h('input.hex', { type: 'text', maxLength: 7, spellcheck: false, attrs: { 'aria-label': o.label + ' hex code' } });
    const quick = h('div.quick', (o.swatches || D.SWATCHES).map((c) => h('button', { type: 'button', title: c, style: { background: c }, attrs: { 'aria-label': 'Use ' + c }, onclick: () => { o.set(c); update(); if (o.onInput) o.onInput(c); if (o.onCommit) o.onCommit(c); } })));
    picker.addEventListener('input', () => { o.set(picker.value); sync(picker.value, picker); if (o.onInput) o.onInput(picker.value); });
    picker.addEventListener('change', () => { if (o.onCommit) o.onCommit(picker.value); });
    hex.addEventListener('input', () => {
      if (!hexOk(hex.value)) return;
      const v = normHex(hex.value);
      o.set(v); sync(v, hex); if (o.onInput) o.onInput(v);
    });
    hex.addEventListener('change', () => { if (hexOk(hex.value) && o.onCommit) o.onCommit(normHex(hex.value)); update(); });
    const row = h('div.colour-row', sw, hex, o.extra || null, o.noQuick ? null : quick);
    const el = o.bare ? row : h('div.field', fieldHead(o.label, id, o.headExtra), row, o.help ? h('div.field-help', o.help) : null);
    function sync(v, from) {
      chip.style.background = v;
      if (from !== picker) picker.value = v;
      if (from !== hex) hex.value = v.toUpperCase();
    }
    function update() { sync(o.get() || '#ffffff'); }
    update();
    return { el, update };
  }

  function toggle(o) {
    const input = h('input', { type: 'checkbox', role: 'switch' });
    const el = h('label.switch', h('span.txt', h('b', o.label), o.sub ? h('span', o.sub) : null), input);
    input.addEventListener('change', () => { o.set(input.checked); if (o.onChange) o.onChange(input.checked); });
    function update() { input.checked = !!o.get(); }
    update();
    return { el, input, update };
  }

  // Small pressed/unpressed buttons for viewport tool rows.
  function miniSeg(options, get, set, label) {
    const el = h('div.mini-seg', { role: 'group', attrs: { 'aria-label': label } });
    const bs = options.map(([v, text, title]) => {
      const b = h('button', { type: 'button', title: title || '', dataset: { v } }, text);
      b.addEventListener('click', () => { set(v); update(); });
      el.append(b);
      return b;
    });
    function update() { const v = get(); for (const b of bs) b.setAttribute('aria-pressed', b.dataset.v === v ? 'true' : 'false'); }
    update();
    return { el, update };
  }
  function miniToggle(text, iconName, get, set, title) {
    const b = h('button.mini-toggle', { type: 'button', title: title || '' }, iconName ? icon(iconName) : null, text);
    b.addEventListener('click', () => { set(!get()); update(); });
    function update() { b.setAttribute('aria-pressed', get() ? 'true' : 'false'); }
    update();
    return { el: b, update };
  }

  root.BfxControls = { icon, panel, section, slider, seg, colour, toggle, miniSeg, miniToggle, uid };
})(typeof self !== 'undefined' ? self : this);
