/* BloodFX Studio UI and screen blood overlay. Talks to client/studio.lua through NUI callbacks. */
(() => {
  'use strict';

  const RESOURCE = typeof window.GetParentResourceName === 'function' ? window.GetParentResourceName() : 'bloodfx';
  const $ = (sel, root = document) => root.querySelector(sel);

  const ICONS = {
    look: '<path d="M12 3.5c3.4 4.7 6.6 8.6 6.6 12.2a6.6 6.6 0 0 1-13.2 0c0-3.6 3.2-7.5 6.6-12.2z"/>',
    hits: '<circle cx="12" cy="12" r="6.5"/><circle cx="12" cy="12" r="1.6"/><path d="M12 2.5v3.5M12 18v3.5M2.5 12H6M18 12h3.5"/>',
    splatter: '<path d="M11 7.5c2.8-.6 5.4 1.6 4.9 4.6-.4 2.7-3.3 4.4-5.9 3.5-2.2-.8-3.3-3.5-2.1-5.6.6-1.2 1.8-2.2 3.1-2.5z"/><circle cx="18.6" cy="6" r="1.3"/><circle cx="19.4" cy="15.8" r="0.9"/><circle cx="5" cy="5.6" r="1.1"/><circle cx="5.8" cy="18.6" r="1.4"/>',
    pools: '<path d="M3.5 16.5c0-2 3.8-3.6 8.5-3.6s8.5 1.6 8.5 3.6-3.8 3.6-8.5 3.6-8.5-1.6-8.5-3.6z"/><path d="M12 3.5c1.9 2.6 3.3 4.4 3.3 6a3.3 3.3 0 0 1-6.6 0c0-1.6 1.4-3.4 3.3-6z"/>',
    screen: '<rect x="3" y="4.5" width="18" height="12" rx="2"/><path d="M8.5 20.5h7M12 16.5v4"/>',
    share: '<circle cx="6.5" cy="12" r="2.4"/><circle cx="17.5" cy="6" r="2.4"/><circle cx="17.5" cy="18" r="2.4"/><path d="M8.6 10.9l6.8-3.7M8.6 13.1l6.8 3.7"/>',
    target: '<circle cx="12" cy="12" r="7"/><circle cx="12" cy="12" r="3"/><path d="M12 2v3M12 19v3M2 12h3M19 12h3"/>',
    burst: '<path d="M12 2.5l1.6 5.1 4.9-2.4-2.4 4.9 5.4 1.9-5.4 1.9 2.4 4.9-4.9-2.4L12 21.5l-1.6-5.1-4.9 2.4 2.4-4.9L2.5 12l5.4-1.9-2.4-4.9 4.9 2.4z"/>',
    dummy: '<circle cx="12" cy="5.5" r="2.5"/><path d="M12 8.5v6.5M7.5 11h9M9 21l3-6 3 6"/>',
    clean: '<path d="M14.5 3.5l6 6M4 20l7.5-7.5M11.5 12.5l2-2 3.5 3.5-2 2zM5.5 4.5v3M4 6h3M18.5 16.5v3M17 18h3"/>',
    brush: '<path d="M19.5 4.5c-3 1-8.5 6-10 8.5l2 2c2.5-1.5 7.5-7 8.5-10z"/><path d="M9 14.5c-2 0-3.5 1.5-3.5 3.5 0 1-.7 1.6-1.5 2 3.5.8 7-1.1 7-3.5z"/>',
    close: '<path d="M6 6l12 12M18 6L6 18"/>',
    detach: '<path d="M2.5 12s3.5-6.5 9.5-6.5 9.5 6.5 9.5 6.5-3.5 6.5-9.5 6.5S2.5 12 2.5 12z"/><circle cx="12" cy="12" r="2.8"/>',
    reroll: '<path d="M20 12a8 8 0 1 1-2.4-5.7"/><path d="M20 4v4.5h-4.5"/>',
    lock: '<rect x="5" y="11" width="14" height="9.5" rx="2"/><path d="M8.5 11V8a3.5 3.5 0 0 1 7 0v3"/>',
  };

  function icon(name) {
    return `<svg class="ico" viewBox="0 0 24 24" aria-hidden="true">${ICONS[name] || ''}</svg>`;
  }

  function el(tag, cls, text) {
    const node = document.createElement(tag);
    if (cls) node.className = cls;
    if (text != null) node.textContent = text;
    return node;
  }

  function button(label, cls, onClick) {
    const b = el('button', cls, label);
    b.type = 'button';
    b.addEventListener('click', onClick);
    return b;
  }

  async function post(name, data) {
    try {
      const res = await fetch(`https://${RESOURCE}/${name}`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json; charset=UTF-8' },
        body: JSON.stringify(data || {}),
      });
      const text = await res.text();
      return text ? JSON.parse(text) : {};
    } catch {
      return null;
    }
  }

  const clamp01 = (v) => Math.max(0, Math.min(1, v));

  function hashString(s) {
    let h = 2166136261;
    for (let i = 0; i < s.length; i++) h = Math.imul(h ^ s.charCodeAt(i), 16777619);
    return h >>> 0;
  }

  // ------------------------------------------------------------------ colour maths

  function normalizeHex(value) {
    let h = String(value || '').trim().replace(/^#/, '');
    if (h.length === 3) h = h.split('').map((c) => c + c).join('');
    return /^[0-9a-f]{6}$/i.test(h) ? `#${h.toLowerCase()}` : null;
  }

  function hsvToHex(h, s, v) {
    const i = Math.floor(h * 6);
    const f = h * 6 - i;
    const p = v * (1 - s);
    const q = v * (1 - f * s);
    const t = v * (1 - (1 - f) * s);
    const [r, g, b] = [[v, t, p], [q, v, p], [p, v, t], [p, q, v], [t, p, v], [v, p, q]][((i % 6) + 6) % 6];
    return Splat.toHex({ r: r * 255, g: g * 255, b: b * 255 });
  }

  function hexToHsv(hex) {
    const { r, g, b } = Splat.hexToRgb(hex);
    const R = r / 255, G = g / 255, B = b / 255;
    const max = Math.max(R, G, B), min = Math.min(R, G, B), d = max - min;
    let h = 0;
    if (d) {
      if (max === R) h = ((G - B) / d) % 6;
      else if (max === G) h = (B - R) / d + 2;
      else h = (R - G) / d + 4;
      h /= 6;
      if (h < 0) h += 1;
    }
    return { h, s: max ? d / max : 0, v: max };
  }

  // ------------------------------------------------------------------ state

  const state = {
    open: false,
    detached: false,
    builtKey: '',
    settings: {},
    fields: [],
    sections: [],
    presets: [],
    customizable: true,
    canPaint: false,
    dummy: false,
    activePreset: null,
    tab: 'look',
    seed: 1 + Math.floor(Math.random() * 1e6),
    seq: 0,
    sendTimer: 0,
  };

  const controls = new Map(); // key -> { field, row, set(value, opts) }
  const els = {
    studio: $('#studio'),
    tabs: $('#tabs'),
    panels: $('#panels'),
    preview: $('#preview'),
    previewBox: $('.preview'),
    previewHex: $('#preview-hex'),
    version: $('#version'),
    lock: $('#lock-banner'),
    toast: $('#toast'),
    keyHint: $('#key-hint'),
    painter: $('#btn-painter'),
  };
  let shareOut = null;

  for (const node of document.querySelectorAll('[data-icon]')) {
    node.insertAdjacentHTML('afterbegin', icon(node.dataset.icon));
  }

  // ------------------------------------------------------------------ formatting

  function decimals(step) {
    const s = String(step == null ? 1 : step);
    return s.includes('.') ? s.split('.')[1].length : 0;
  }

  function formatValue(field, v) {
    if (typeof v !== 'number' || Number.isNaN(v)) return '';
    if (field.unit === '%') return `${Math.round(v * 100)}%`;
    const n = v.toFixed(decimals(field.step));
    if (field.unit === 'x') return `${n}×`;
    if (field.unit === 's') return `${n}s`;
    if (field.unit === 'm') return `${n} m`;
    return n;
  }

  // ------------------------------------------------------------------ controls

  function boolControl(field, head) {
    const input = document.createElement('input');
    input.type = 'checkbox';
    input.setAttribute('role', 'switch');
    input.setAttribute('aria-label', field.label);
    input.disabled = !!field.locked;
    const sw = el('label', 'switch');
    sw.append(input, el('span', 'switch-track'));
    input.addEventListener('change', () => commit(field.key, input.checked));
    head.append(sw);
    return { set: (v) => { input.checked = !!v; } };
  }

  function numberControl(field, head, row) {
    const value = el('span', 'field-value');
    head.append(value);
    const input = document.createElement('input');
    input.type = 'range';
    input.className = 'range';
    input.min = field.min;
    input.max = field.max;
    input.step = field.step || 0.01;
    input.disabled = !!field.locked;
    input.setAttribute('aria-label', field.label);
    const paint = (v) => {
      value.textContent = formatValue(field, v);
      const span = field.max - field.min;
      input.style.setProperty('--fill', `${span > 0 ? ((v - field.min) / span) * 100 : 0}%`);
    };
    input.addEventListener('input', () => commit(field.key, parseFloat(input.value)));
    row.append(input);
    return {
      set: (v) => {
        const n = Number(v);
        if (String(input.value) !== String(n)) input.value = n;
        paint(n);
      },
    };
  }

  function selectControl(field, row) {
    const options = Array.isArray(field.options) ? field.options : [];
    const wrap = el('div', options.length <= 3 ? 'segmented' : 'chips');
    const buttons = options.map((option) => {
      const b = button(option.label, 'seg', () => commit(field.key, option.value));
      b.disabled = !!field.locked;
      wrap.append(b);
      return { b, value: option.value };
    });
    row.append(wrap);
    return { set: (v) => buttons.forEach(({ b, value }) => b.classList.toggle('is-active', value === v)) };
  }

  const SWATCHES = ['#320000', '#5e0000', '#8a0303', '#d40000', '#ff1fd1', '#1fb4ff', '#5cff1f', '#ffb21f', '#7a2bff', '#f2f2f2'];

  // Hand-made picker: <input type="color"> opens an OS dialog that FiveM's browser cannot show.
  function colorControl(field, row) {
    let hsv = { h: 0, s: 1, v: 0.37 };
    const picker = el('div', 'picker');
    const sv = el('div', 'picker-sv');
    const svHandle = el('span', 'picker-handle');
    sv.append(svHandle);
    const hue = el('div', 'picker-hue');
    const hueHandle = el('span', 'picker-handle');
    hue.append(hueHandle);

    const line = el('div', 'picker-row');
    const chip = el('span', 'picker-chip');
    const hex = document.createElement('input');
    hex.type = 'text';
    hex.className = 'hex-input';
    hex.maxLength = 7;
    hex.spellcheck = false;
    hex.disabled = !!field.locked;
    hex.setAttribute('aria-label', 'Hex colour');
    line.append(chip, hex);

    const swatches = el('div', 'swatches');
    const swatchButtons = SWATCHES.map((c) => {
      const b = button('', 'swatch', () => commit(field.key, c));
      b.style.setProperty('--c', c);
      b.title = c;
      b.disabled = !!field.locked;
      swatches.append(b);
      return { b, c };
    });

    picker.append(sv, hue, line, swatches);
    row.append(picker);

    function paint(current) {
      sv.style.setProperty('--hue', hsvToHex(hsv.h, 1, 1));
      svHandle.style.left = `${hsv.s * 100}%`;
      svHandle.style.top = `${(1 - hsv.v) * 100}%`;
      hueHandle.style.left = `${hsv.h * 100}%`;
      chip.style.background = current;
      if (document.activeElement !== hex) hex.value = current.toUpperCase();
      swatchButtons.forEach(({ b, c }) => b.classList.toggle('is-active', c === current));
    }

    const emit = () => commit(field.key, hsvToHex(hsv.h, hsv.s, hsv.v), { fromPicker: true });
    drag(sv, (x, y) => { hsv.s = x; hsv.v = 1 - y; emit(); });
    drag(hue, (x) => { hsv.h = Math.min(x, 0.9999); emit(); });
    hex.addEventListener('change', () => {
      const v = normalizeHex(hex.value);
      if (v) commit(field.key, v);
      else hex.value = String(state.settings[field.key] || '').toUpperCase();
    });

    return {
      set: (v, opts) => {
        const current = normalizeHex(v) || '#5e0000';
        // Keep the hue while dragging; converting back from hex loses it at black and grey.
        const same = hsvToHex(hsv.h, hsv.s, hsv.v) === current;
        if (!(opts && opts.fromPicker) && !same) hsv = hexToHsv(current);
        paint(current);
      },
    };
  }

  function drag(node, onMove) {
    node.addEventListener('pointerdown', (e) => {
      if (node.closest('.is-locked')) return;
      node.setPointerCapture(e.pointerId);
      const move = (ev) => {
        const r = node.getBoundingClientRect();
        onMove(clamp01((ev.clientX - r.left) / r.width), clamp01((ev.clientY - r.top) / r.height));
      };
      const up = () => {
        node.removeEventListener('pointermove', move);
        node.removeEventListener('pointerup', up);
        node.removeEventListener('pointercancel', up);
      };
      move(e);
      node.addEventListener('pointermove', move);
      node.addEventListener('pointerup', up);
      node.addEventListener('pointercancel', up);
    });
  }

  function buildField(field) {
    const row = el('div', 'field');
    row.dataset.key = field.key;
    const head = el('div', 'field-head');
    const label = el('span', 'field-label', field.label);
    if (field.locked) {
      const lock = el('span', 'lock');
      lock.innerHTML = icon('lock');
      lock.title = 'Locked by the server';
      label.append(lock);
    }
    head.append(label);
    row.append(head);

    let api = null;
    if (field.type === 'bool') api = boolControl(field, head);
    else if (field.type === 'number') api = numberControl(field, head, row);
    else if (field.type === 'select') api = selectControl(field, row);
    else if (field.type === 'color') api = colorControl(field, row);
    if (!api) return row;

    if (field.hint) row.append(el('p', 'hint', field.hint));
    if (field.locked) row.classList.add('is-locked');
    controls.set(field.key, { field, row, set: api.set });
    return row;
  }

  // ------------------------------------------------------------------ presets

  function buildPresets() {
    const block = el('div', 'block');
    block.append(el('div', 'block-title', 'Presets'));
    const grid = el('div', 'presets');
    for (const preset of state.presets) {
      const b = button('', `preset${preset.rainbow ? ' is-rainbow' : ''}`, () => applyPreset(preset));
      b.dataset.preset = preset.key;
      b.title = preset.label;
      b.disabled = !state.customizable;
      const canvas = document.createElement('canvas');
      canvas.width = 88;
      canvas.height = 88;
      const splat = Splat.render(preset.rainbow ? '#ff2a2a' : preset.color, 15, hashString(preset.key), { satellites: 9, spikes: 6 });
      canvas.getContext('2d').drawImage(splat.canvas, 44 - splat.cx, 44 - splat.cy);
      b.append(canvas, el('span', '', preset.label));
      grid.append(b);
    }
    block.append(grid);
    return block;
  }

  function refreshPresetActive() {
    for (const b of document.querySelectorAll('.preset')) {
      b.classList.toggle('is-active', b.dataset.preset === state.activePreset);
    }
  }

  async function applyPreset(preset) {
    clearTimeout(state.sendTimer);
    state.seq += 1;
    const res = await post('preset', { key: preset.key });
    if (res && res.ok && res.settings) {
      applySettings(res.settings);
      state.activePreset = preset.key;
      refreshPresetActive();
      toast(`${preset.label} applied`);
    } else {
      toast('Could not reach the game');
    }
  }

  // ------------------------------------------------------------------ share codes

  function encodeCode(settings) {
    return `BFX1-${btoa(JSON.stringify(settings))}`;
  }

  function decodeCode(text) {
    const raw = String(text || '').replace(/\s+/g, '');
    if (!raw.startsWith('BFX1-')) throw new Error('BloodFX codes start with BFX1-');
    let parsed;
    try {
      parsed = JSON.parse(atob(raw.slice(5)));
    } catch {
      throw new Error('That code is damaged or incomplete');
    }
    if (!parsed || typeof parsed !== 'object' || Array.isArray(parsed)) throw new Error('That code is not a BloodFX setup');
    return parsed;
  }

  async function copyText(area) {
    area.focus();
    area.select();
    let ok = false;
    try { ok = document.execCommand('copy'); } catch { ok = false; }
    if (!ok && navigator.clipboard) {
      try { await navigator.clipboard.writeText(area.value); ok = true; } catch { ok = false; }
    }
    area.scrollTop = 0;
    toast(ok ? 'Code copied' : 'Select the code and press Ctrl+C');
  }

  function buildShare() {
    const wrap = el('div', 'share');
    wrap.append(el('div', 'block-title', 'Your setup'));
    wrap.append(el('p', 'hint', 'Send this code to a friend so they can load your exact look.'));
    shareOut = el('textarea', 'code');
    shareOut.readOnly = true;
    shareOut.rows = 4;
    shareOut.spellcheck = false;
    shareOut.setAttribute('aria-label', 'Your BloodFX code');
    const copyRow = el('div', 'row-btns');
    copyRow.append(button('Copy code', 'btn', () => copyText(shareOut)));
    wrap.append(shareOut, copyRow);

    wrap.append(el('div', 'block-title', 'Load a setup'));
    const input = el('textarea', 'code');
    input.rows = 3;
    input.spellcheck = false;
    input.placeholder = 'Paste a BFX1- code here';
    input.setAttribute('aria-label', 'Paste a BloodFX code');
    const apply = button('Apply code', 'btn primary', async () => {
      let values;
      try {
        values = decodeCode(input.value);
      } catch (err) {
        toast(err.message);
        return;
      }
      const res = await post('import', { settings: values });
      if (res && res.ok && res.settings) {
        applySettings(res.settings);
        input.value = '';
        toast('Setup loaded');
      } else {
        toast('Could not reach the game');
      }
    });
    apply.disabled = !state.customizable;
    const applyRow = el('div', 'row-btns');
    applyRow.append(apply);
    wrap.append(input, applyRow);

    wrap.append(el('div', 'block-title', 'Start over'));
    const reset = button('Reset to server defaults', 'btn danger', async () => {
      const res = await post('reset');
      if (res && res.settings) {
        applySettings(res.settings);
        state.activePreset = null;
        refreshPresetActive();
        toast('Settings reset');
      }
    });
    reset.disabled = !state.customizable;
    const resetRow = el('div', 'row-btns');
    resetRow.append(reset);
    wrap.append(resetRow);
    return wrap;
  }

  function refreshShare() {
    if (!shareOut) return;
    try { shareOut.value = encodeCode(state.settings); } catch { shareOut.value = ''; }
  }

  // ------------------------------------------------------------------ build

  function selectTab(id) {
    state.tab = id;
    for (const t of els.tabs.children) {
      const on = t.dataset.tab === id;
      t.classList.toggle('is-active', on);
      t.setAttribute('aria-selected', on ? 'true' : 'false');
    }
    for (const p of els.panels.children) p.classList.toggle('is-active', p.dataset.panel === id);
    els.panels.scrollTop = 0;
  }

  function build() {
    controls.clear();
    shareOut = null;
    els.tabs.textContent = '';
    els.panels.textContent = '';
    const sections = [...state.sections, { id: 'share', label: 'Share' }];
    for (const section of sections) {
      const tab = el('button', 'tab');
      tab.type = 'button';
      tab.dataset.tab = section.id;
      tab.setAttribute('role', 'tab');
      tab.innerHTML = `${icon(section.id)}<span></span>`;
      tab.lastChild.textContent = section.label;
      tab.addEventListener('click', () => selectTab(section.id));
      els.tabs.append(tab);

      const panel = el('div', 'panel');
      panel.dataset.panel = section.id;
      panel.setAttribute('role', 'tabpanel');
      if (section.id === 'look') panel.append(buildPresets());
      if (section.id === 'share') panel.append(buildShare());
      for (const field of state.fields.filter((f) => f.section === section.id)) panel.append(buildField(field));
      els.panels.append(panel);
    }
    selectTab(sections.some((s) => s.id === state.tab) ? state.tab : 'look');
  }

  // ------------------------------------------------------------------ settings flow

  function refreshDependencies() {
    for (const { field, row } of controls.values()) {
      row.classList.toggle('is-dim', !!field.depends && !state.settings[field.depends]);
    }
    els.studio.classList.toggle('is-off', state.settings.enabled === false);
  }

  function applyAccent() {
    const accent = state.settings.rainbow ? '#ff4d6d' : Splat.accent(state.settings.color || '#5e0000');
    const rgb = Splat.hexToRgb(accent);
    const root = document.documentElement.style;
    root.setProperty('--accent', accent);
    root.setProperty('--accent-soft', `rgba(${rgb.r},${rgb.g},${rgb.b},0.16)`);
  }

  function refreshLook() {
    refreshDependencies();
    renderPreview();
    applyAccent();
    refreshShare();
  }

  function applySettings(settings, fromServer) {
    if (!settings || typeof settings !== 'object') return;
    state.settings = Object.assign({}, settings);
    for (const [key, control] of controls) {
      if (key in settings) control.set(settings[key], { fromServer: !!fromServer });
    }
    refreshLook();
  }

  function scheduleSend() {
    clearTimeout(state.sendTimer);
    state.sendTimer = setTimeout(async () => {
      const seq = ++state.seq;
      const res = await post('update', { settings: state.settings });
      if (res && res.settings && seq === state.seq) applySettings(res.settings, true);
    }, 90);
  }

  function commit(key, value, opts) {
    state.settings[key] = value;
    const control = controls.get(key);
    if (control) control.set(value, opts);
    if (state.activePreset) {
      state.activePreset = null;
      refreshPresetActive();
    }
    refreshLook();
    scheduleSend();
  }

  // ------------------------------------------------------------------ preview

  let concrete = null;

  function makeConcrete(w, h) {
    const c = document.createElement('canvas');
    c.width = w;
    c.height = h;
    const ctx = c.getContext('2d');
    const grad = ctx.createLinearGradient(0, 0, 0, h);
    grad.addColorStop(0, '#2c2629');
    grad.addColorStop(1, '#1a1618');
    ctx.fillStyle = grad;
    ctx.fillRect(0, 0, w, h);
    const img = ctx.getImageData(0, 0, w, h);
    const rnd = Splat.mulberry32(99);
    for (let i = 0; i < img.data.length; i += 4) {
      const n = (rnd() - 0.5) * 16;
      img.data[i] += n;
      img.data[i + 1] += n;
      img.data[i + 2] += n;
    }
    ctx.putImageData(img, 0, 0);
    ctx.strokeStyle = 'rgba(0,0,0,0.35)';
    ctx.lineWidth = Math.max(1, h / 90);
    for (const y of [0.33, 0.66]) {
      ctx.beginPath();
      ctx.moveTo(0, h * y);
      ctx.lineTo(w, h * y);
      ctx.stroke();
    }
    const vignette = ctx.createRadialGradient(w / 2, h / 2, h * 0.2, w / 2, h / 2, w * 0.7);
    vignette.addColorStop(0, 'rgba(0,0,0,0)');
    vignette.addColorStop(1, 'rgba(0,0,0,0.5)');
    ctx.fillStyle = vignette;
    ctx.fillRect(0, 0, w, h);
    return c;
  }

  function stamp(ctx, color, R, seed, opts, x, y, dpr) {
    const s = Splat.render(color, R * dpr, seed, opts);
    ctx.drawImage(s.canvas, x - s.cx / dpr, y - s.cy / dpr, s.canvas.width / dpr, s.canvas.height / dpr);
  }

  function renderPreview() {
    const canvas = els.preview;
    const w = canvas.clientWidth;
    const h = canvas.clientHeight;
    if (!w || !h) return;
    const dpr = Math.min(window.devicePixelRatio || 1, 2);
    if (canvas.width !== Math.round(w * dpr) || canvas.height !== Math.round(h * dpr)) {
      canvas.width = Math.round(w * dpr);
      canvas.height = Math.round(h * dpr);
      concrete = null;
    }
    if (!concrete) concrete = makeConcrete(canvas.width, canvas.height);
    const ctx = canvas.getContext('2d');
    ctx.setTransform(1, 0, 0, 1, 0, 0);
    ctx.drawImage(concrete, 0, 0);
    ctx.setTransform(dpr, 0, 0, dpr, 0, 0);

    const rainbow = !!state.settings.rainbow;
    const color = rainbow ? '#ff2a2a' : state.settings.color || '#5e0000';
    ctx.globalAlpha = typeof state.settings.opacity === 'number' ? state.settings.opacity : 1;
    stamp(ctx, color, h * 0.19, state.seed, { dir: -0.12, stretch: 0.6, mist: 260, satellites: 34 }, w * 0.31, h * 0.52, dpr);
    stamp(ctx, color, h * 0.085, state.seed + 7, { satellites: 10 }, w * 0.79, h * 0.3, dpr);
    stamp(ctx, color, h * 0.055, state.seed + 13, { satellites: 6, spikes: 4 }, w * 0.88, h * 0.72, dpr);
    ctx.globalAlpha = 1;

    els.previewBox.classList.toggle('is-rainbow', rainbow);
    els.previewBox.style.setProperty('--hue-speed', `${4 / (state.settings.rainbowSpeed || 1)}s`);
    els.previewHex.textContent = rainbow ? 'RAINBOW' : color.toUpperCase();
  }

  // ------------------------------------------------------------------ toast, panel state

  let toastTimer = 0;
  function toast(message) {
    els.toast.textContent = message;
    els.toast.classList.add('is-on');
    clearTimeout(toastTimer);
    toastTimer = setTimeout(() => els.toast.classList.remove('is-on'), 1900);
  }

  function setDetached(on) {
    state.detached = on;
    els.studio.classList.toggle('is-detached', on);
  }

  function setCanPaint(on) {
    state.canPaint = on;
    els.painter.classList.toggle('is-hidden', !on);
  }

  function setDummy(on) {
    state.dummy = on;
    const label = $('[data-test="dummy"] span');
    if (label) label.textContent = on ? 'Remove dummy' : 'Dummy';
  }

  function open(d) {
    state.fields = Array.isArray(d.schema) ? d.schema : [];
    state.sections = Array.isArray(d.sections) ? d.sections : [];
    state.presets = Array.isArray(d.presets) ? d.presets : [];
    state.customizable = d.customizable !== false;
    const key = JSON.stringify([state.fields, state.sections, state.presets, state.customizable]);
    if (key !== state.builtKey) {
      build();
      state.builtKey = key;
    }
    els.version.textContent = d.version ? `v${d.version}` : '';
    els.keyHint.textContent = d.keyHint || '/bloodfx';
    els.lock.classList.toggle('is-hidden', state.customizable);
    setCanPaint(!!d.canPaint);
    setDummy(!!d.dummy);
    state.open = true;
    setDetached(false);
    els.studio.classList.remove('is-hidden');
    applySettings(d.settings || {});
    requestAnimationFrame(renderPreview);
  }

  function hide() {
    state.open = false;
    setDetached(false);
    els.studio.classList.add('is-hidden');
  }

  function close() {
    hide();
    post('close');
  }

  $('#btn-close').addEventListener('click', close);
  $('#btn-detach').addEventListener('click', () => {
    setDetached(true);
    post('detach');
  });
  $('#btn-reroll').addEventListener('click', () => {
    state.seed = 1 + Math.floor(Math.random() * 1e6);
    renderPreview();
  });
  els.painter.addEventListener('click', () => {
    hide();
    post('painter');
  });
  for (const b of document.querySelectorAll('[data-test]')) {
    b.addEventListener('click', async () => {
      const res = await post('test', { kind: b.dataset.test });
      if (res && typeof res.dummy === 'boolean') setDummy(res.dummy);
    });
  }

  window.addEventListener('keydown', (e) => {
    if (state.open && !state.detached && e.key === 'Escape') {
      e.preventDefault();
      close();
    }
  });

  // ------------------------------------------------------------------ screen blood

  const Screen = (() => {
    const canvas = $('#screen-fx');
    const ctx = canvas.getContext('2d');
    const low = $('#low-health');
    let splats = [];
    let raf = 0;

    function resize() {
      const dpr = Math.min(window.devicePixelRatio || 1, 1.5);
      canvas.width = Math.round(window.innerWidth * dpr);
      canvas.height = Math.round(window.innerHeight * dpr);
    }
    resize();
    window.addEventListener('resize', resize);

    function spawn(color, intensity, duration, headshot) {
      const W = canvas.width;
      const H = canvas.height;
      const R = Math.min(W, H) * (0.035 + Math.random() * 0.05) * (0.7 + intensity * 0.5) * (headshot ? 1.3 : 1);
      const edge = Math.floor(Math.random() * 4);
      const t = 0.08 + Math.random() * 0.84;
      const inset = R * (0.4 + Math.random() * 1.4);
      let x;
      let y;
      if (edge === 0) { x = t * W; y = inset; }
      else if (edge === 1) { x = W - inset; y = t * H; }
      else if (edge === 2) { x = t * W; y = H - inset; }
      else { x = inset; y = t * H; }
      const s = Splat.render(color, R, (Math.random() * 4294967295) >>> 0, {
        drips: 1 + Math.floor(Math.random() * 4),
        dripLength: 2.4,
        spikes: 4 + Math.floor(Math.random() * 5),
        spikeLength: 0.55,
        satellites: 14 + Math.floor(Math.random() * 16),
        mist: 50,
      });
      return {
        img: s.canvas,
        x: x - s.cx,
        y: y - s.cy,
        born: performance.now(),
        life: duration * 1000 * (0.75 + Math.random() * 0.5),
        alpha: Math.min(0.9, 0.45 + intensity * 0.35),
        slide: R * (0.2 + Math.random() * 0.6),
      };
    }

    function tick(now) {
      ctx.clearRect(0, 0, canvas.width, canvas.height);
      splats = splats.filter((s) => now - s.born < s.life);
      for (const s of splats) {
        const t = (now - s.born) / s.life;
        const fadeIn = Math.min(1, t / 0.04);
        const fadeOut = t < 0.35 ? 1 : Math.pow(1 - (t - 0.35) / 0.65, 1.4);
        ctx.globalAlpha = Math.max(0, s.alpha * fadeIn * fadeOut);
        ctx.drawImage(s.img, s.x, s.y + s.slide * Math.pow(t, 1.3));
      }
      ctx.globalAlpha = 1;
      raf = splats.length ? requestAnimationFrame(tick) : 0;
    }

    function hit(d) {
      const intensity = Math.max(0.1, Number(d.intensity) || 0.8);
      const duration = Math.max(0.3, Number(d.duration) || 2.5);
      const count = Math.max(1, Math.round(1 + intensity * (d.headshot ? 3.2 : 2)));
      for (let i = 0; i < count; i++) splats.push(spawn(d.color || '#6a0000', intensity, duration, !!d.headshot));
      if (splats.length > 26) splats.splice(0, splats.length - 26);
      if (!raf) raf = requestAnimationFrame(tick);
    }

    function lowHealth(level, color) {
      const lvl = clamp01(Number(level) || 0);
      const rgb = Splat.hexToRgb(color || '#8a0000');
      low.style.setProperty('--lh-color', `rgba(${rgb.r},${rgb.g},${rgb.b},${0.35 + lvl * 0.5})`);
      low.style.setProperty('--lh-speed', `${(1.6 - lvl * 0.8).toFixed(2)}s`);
      low.style.opacity = lvl > 0 ? String(0.35 + lvl * 0.65) : '0';
      low.classList.toggle('is-pulsing', lvl > 0);
    }

    function clear() {
      splats = [];
      ctx.clearRect(0, 0, canvas.width, canvas.height);
    }

    return { hit, lowHealth, clear };
  })();

  // ------------------------------------------------------------------ messages from Lua

  window.addEventListener('message', (event) => {
    const d = event.data || {};
    switch (d.type) {
      case 'studio:open': open(d); break;
      case 'studio:focus': setDetached(false); break;
      case 'studio:close': hide(); break;
      case 'studio:canPaint': setCanPaint(!!d.value); break;
      case 'screen:hit': Screen.hit(d); break;
      case 'screen:lowHealth': Screen.lowHealth(d.level, d.color); break;
      case 'screen:clear': Screen.clear(); break;
      default: break;
    }
  });
})();
