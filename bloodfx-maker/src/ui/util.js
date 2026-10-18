/* BloodFX Maker helpers: DOM building, colours, zips and storage. */
(function (root) {
  'use strict';

  const $ = (s, el) => (el || document).querySelector(s);
  const $$ = (s, el) => Array.from((el || document).querySelectorAll(s));

  // h('div.panel#x', { onclick, hidden, dataset: {}, style: {}, attrs: {} }, ...children)
  function h(spec, props, ...kids) {
    const m = /^([a-z0-9-]+)?((?:[.#][\w-]+)*)$/i.exec(spec);
    const el = document.createElement((m && m[1]) || 'div');
    if (m && m[2]) {
      for (const part of m[2].match(/[.#][\w-]+/g)) {
        if (part[0] === '.') el.classList.add(part.slice(1)); else el.id = part.slice(1);
      }
    }
    if (props && (typeof props !== 'object' || props instanceof Node || Array.isArray(props))) { kids.unshift(props); props = null; }
    if (props) {
      for (const [k, v] of Object.entries(props)) {
        if (v === undefined || v === null) continue;
        if (v === false) { if (k in el && typeof el[k] === 'boolean') el[k] = false; continue; }
        if (k === 'role' || k.startsWith('aria')) el.setAttribute(k === 'role' ? k : k.replace(/^aria([A-Z])/, (_, c) => 'aria-' + c.toLowerCase()), v);
        else if (k === 'dataset') Object.assign(el.dataset, v);
        else if (k === 'style') { for (const [sk, sv] of Object.entries(v)) { if (sk.startsWith('--')) el.style.setProperty(sk, sv); else el.style[sk] = sv; } }
        else if (k === 'attrs') { for (const [ak, av] of Object.entries(v)) if (av !== undefined && av !== null && av !== false) el.setAttribute(ak, av === true ? '' : av); }
        else if (k === 'html') el.innerHTML = v; // static markup only (icons)
        else if (k.startsWith('on')) el.addEventListener(k.slice(2), v);
        else if (k === 'class') el.className = v;
        else if (k in el) el[k] = v;
        else el.setAttribute(k, v === true ? '' : v);
      }
    }
    append(el, kids);
    return el;
  }
  function append(el, kids) {
    for (const k of kids.flat(Infinity)) {
      if (k === null || k === undefined || k === false) continue;
      el.append(k instanceof Node ? k : String(k));
    }
    return el;
  }
  const clear = (el) => { el.textContent = ''; return el; };
  const tick = (ms) => new Promise((r) => setTimeout(r, ms || 0));

  // ---------------------------------------------------------------- colours
  const hexOk = (s) => /^#?[0-9a-f]{6}$/i.test(String(s || '').trim());
  const normHex = (s) => '#' + String(s).trim().replace('#', '').toLowerCase();
  const rgbOf = (hex) => { const n = parseInt(String(hex).replace('#', ''), 16); return [(n >> 16) & 255, (n >> 8) & 255, n & 255]; };
  const toHex = (rgb) => '#' + rgb.map((c) => Math.round(Math.min(255, Math.max(0, c))).toString(16).padStart(2, '0')).join('');
  const rgb01 = (hex) => rgbOf(hex).map((c) => Math.round((c / 255) * 1000) / 1000);
  const cssRgb01 = (v) => toHex(v.slice(0, 3).map((c) => Math.min(1, Math.max(0, c)) * 255));
  const luma = (hex) => { const [r, g, b] = rgbOf(hex); return 0.299 * r + 0.587 * g + 0.114 * b; };

  // ---------------------------------------------------------------- bytes
  const latin1 = (u8) => { let s = ''; for (let i = 0; i < u8.length; i += 8192) s += String.fromCharCode.apply(null, u8.subarray(i, i + 8192)); return s; };
  const latin1Bytes = (s) => { const u = new Uint8Array(s.length); for (let i = 0; i < s.length; i++) u[i] = s.charCodeAt(i) & 0xFF; return u; };
  const b64Bytes = (b) => { const s = atob(b); const u = new Uint8Array(s.length); for (let i = 0; i < s.length; i++) u[i] = s.charCodeAt(i); return u; };
  const fmtBytes = (n) => (n >= 1048576 ? (n / 1048576).toFixed(1) + ' MB' : n >= 1024 ? Math.round(n / 1024) + ' KB' : n + ' B');

  const CRC = (() => { const t = new Uint32Array(256); for (let n = 0; n < 256; n++) { let c = n; for (let k = 0; k < 8; k++) c = c & 1 ? 0xEDB88320 ^ (c >>> 1) : c >>> 1; t[n] = c >>> 0; } return t; })();
  function crc32(u8) { let c = 0xFFFFFFFF; for (let i = 0; i < u8.length; i++) c = CRC[(c ^ u8[i]) & 0xFF] ^ (c >>> 8); return (c ^ 0xFFFFFFFF) >>> 0; }

  // A stored (uncompressed) zip; the rpf inside is already compressed.
  function makeZip(files) {
    const d = new Date();
    const time = (d.getHours() << 11) | (d.getMinutes() << 5) | (d.getSeconds() >> 1);
    const date = ((d.getFullYear() - 1980) << 9) | ((d.getMonth() + 1) << 5) | d.getDate();
    const enc = new TextEncoder();
    const chunks = [], central = [];
    let offset = 0;
    for (const f of files) {
      const name = enc.encode(f.name);
      const crc = crc32(f.data);
      const local = new DataView(new ArrayBuffer(30));
      local.setUint32(0, 0x04034b50, true); local.setUint16(4, 20, true); local.setUint16(6, 0x0800, true); local.setUint16(8, 0, true);
      local.setUint16(10, time, true); local.setUint16(12, date, true); local.setUint32(14, crc, true);
      local.setUint32(18, f.data.length, true); local.setUint32(22, f.data.length, true); local.setUint16(26, name.length, true); local.setUint16(28, 0, true);
      chunks.push(new Uint8Array(local.buffer), name, f.data);
      const cen = new DataView(new ArrayBuffer(46));
      cen.setUint32(0, 0x02014b50, true); cen.setUint16(4, 20, true); cen.setUint16(6, 20, true); cen.setUint16(8, 0x0800, true); cen.setUint16(10, 0, true);
      cen.setUint16(12, time, true); cen.setUint16(14, date, true); cen.setUint32(16, crc, true); cen.setUint32(20, f.data.length, true); cen.setUint32(24, f.data.length, true);
      cen.setUint16(28, name.length, true); cen.setUint32(42, offset, true);
      central.push(new Uint8Array(cen.buffer), name);
      offset += 30 + name.length + f.data.length;
    }
    const cenSize = central.reduce((n, c) => n + c.length, 0);
    const end = new DataView(new ArrayBuffer(22));
    end.setUint32(0, 0x06054b50, true); end.setUint16(8, files.length, true); end.setUint16(10, files.length, true);
    end.setUint32(12, cenSize, true); end.setUint32(16, offset, true);
    const parts = [...chunks, ...central, new Uint8Array(end.buffer)];
    const out = new Uint8Array(parts.reduce((n, p) => n + p.length, 0));
    let pos = 0;
    for (const p of parts) { out.set(p, pos); pos += p.length; }
    return out;
  }

  async function deflateRaw(u8) {
    if (typeof CompressionStream === 'function') {
      try {
        const stream = new Blob([u8]).stream().pipeThrough(new CompressionStream('deflate-raw'));
        return new Uint8Array(await new Response(stream).arrayBuffer());
      } catch { /* fall back to pako */ }
    }
    return root.pako.deflateRaw(u8, { level: 6 });
  }

  // ---------------------------------------------------------------- per-viewer settings
  const store = {
    get(key, fallback) {
      try { const v = root.localStorage.getItem('bfxm:' + key); return v === null ? fallback : JSON.parse(v); } catch { return fallback; }
    },
    set(key, value) {
      try { root.localStorage.setItem('bfxm:' + key, JSON.stringify(value)); } catch { /* storage unavailable */ }
    },
  };

  const plural = (n, one, many) => `${n} ${n === 1 ? one : many || one + 's'}`;
  const round = (v, step) => Math.round(v / step) * step;

  root.BfxUtil = {
    $, $$, h, append, clear, tick, hexOk, normHex, rgbOf, toHex, rgb01, cssRgb01, luma,
    latin1, latin1Bytes, b64Bytes, fmtBytes, crc32, makeZip, deflateRaw, store, plural, round,
  };
})(typeof self !== 'undefined' ? self : this);
