/* BloodFX Maker artwork: template thumbnails, seg-button art and the home hero, drawn from real tracer sheets. */
(function (root) {
  'use strict';
  const { h } = root.BfxUtil;
  const D = root.BfxData;

  // Paints an RGBA tracer sheet onto a canvas so it can be stretched along a path.
  function sheetCanvas(sheet) {
    const c = document.createElement('canvas');
    c.width = sheet.w; c.height = sheet.h;
    const img = c.getContext('2d').createImageData(sheet.w, sheet.h);
    img.data.set(sheet.px);
    c.getContext('2d').putImageData(img, 0, 0);
    return c;
  }

  // One frame of the sheet stretched from (x0,y0) to (x1,y1), `width` pixels across.
  function strip(ctx, src, frame, x0, y0, x1, y1, width, alpha) {
    const len = Math.hypot(x1 - x0, y1 - y0);
    if (len < 1 || alpha <= 0) return;
    const fw = src.width / 4;
    ctx.save();
    ctx.globalAlpha = Math.min(1, alpha);
    ctx.translate(x0, y0);
    ctx.rotate(Math.atan2(y1 - y0, x1 - x0) - Math.PI / 2);
    ctx.drawImage(src, frame * fw, 0, fw, src.height, -width / 2, 0, width, len);
    ctx.restore();
  }

  function backdrop(ctx, w, hgt, tone) {
    const g = ctx.createLinearGradient(0, 0, w, hgt);
    g.addColorStop(0, tone ? '#0b1322' : '#070a11');
    g.addColorStop(1, '#03050a');
    ctx.fillStyle = g;
    ctx.fillRect(0, 0, w, hgt);
  }

  // A tracer crossing a small canvas: smoke frame first, then the beam with its glow.
  // look: { sheet: {px,w,h}, glowColour, glow, smoke: bool, beam: bool }
  function tracerThumb(canvas, look, opts) {
    opts = opts || {};
    const dpr = 2;
    const w = (opts.w || 74) * dpr, hgt = (opts.h || 38) * dpr;
    canvas.width = w; canvas.height = hgt;
    const ctx = canvas.getContext('2d');
    backdrop(ctx, w, hgt, true);
    const src = sheetCanvas(look.sheet);
    const x0 = w * 0.06, y0 = hgt * 0.84, x1 = w * 0.94, y1 = hgt * 0.2;
    ctx.globalCompositeOperation = 'lighter';
    const width = (opts.width || 15) * dpr * Math.max(0.6, Math.min(1.6, look.zoom || 1));
    if (look.smoke) {
      strip(ctx, src, 0, x0, y0, x1, y1, width * 1.1, 0.75);
      strip(ctx, src, 1, x0, y0, x1, y1, width * 1.1, 0.35);
    }
    if (look.beam !== false) {
      ctx.shadowColor = look.glowColour || '#ffffff';
      ctx.shadowBlur = Math.min(26, 4 + (look.glow || 15) * 0.5) * (dpr / 2) * 2;
      strip(ctx, src, 3, x0 + (x1 - x0) * 0.25, y0 + (y1 - y0) * 0.25, x1, y1, width, 0.8);
      ctx.shadowBlur = 0;
      strip(ctx, src, 3, x0 + (x1 - x0) * 0.25, y0 + (y1 - y0) * 0.25, x1, y1, width, 1);
    }
    ctx.globalCompositeOperation = 'source-over';
  }

  function fxThumb(kind) {
    return h('span.fx', { html: D.fxIcon(kind) });
  }

  // Seg-button art: tiny pictures of each beam and smoke mode.
  const bar = (left, width, bg, extra) => h('i', { style: Object.assign({ left: left + 'px', width: width + 'px', background: bg }, extra || {}) });
  const BEAM_ART = {
    keep: () => [bar(12, 4, 'linear-gradient(#ffd9a0,#ff9a4d)'), bar(3, 3, 'rgba(180,190,210,.35)')],
    recolour: () => [bar(12, 4, 'linear-gradient(#5ec4ff,#9b6bff)'), bar(3, 3, 'rgba(94,196,255,.35)')],
    none: () => [h('i', { style: { left: '4px', right: '4px', top: '7px', bottom: 'auto', height: '1px', background: '#6f7a8e', transform: 'rotate(-28deg)' } })],
    solid: () => [bar(11, 6, '#ff2bd6')],
    neon: () => [bar(10, 8, '#ff2bd6', { boxShadow: '0 0 6px #ff2bd6' }), bar(13, 2, '#fff')],
    twin: () => [bar(9, 2, '#7dff6a'), bar(17, 2, '#7dff6a')],
  };
  const SMOKE_ART = {
    keep: () => [bar(6, 4, 'linear-gradient(rgba(200,205,215,.8),rgba(200,205,215,.2))'), bar(18, 3, 'rgba(200,205,215,.45)')],
    recolour: () => [bar(6, 4, 'linear-gradient(rgba(255,194,31,.9),rgba(255,194,31,.25))'), bar(18, 3, 'rgba(255,194,31,.5)')],
    off: () => [h('i', { style: { left: '4px', right: '4px', top: '7px', bottom: 'auto', height: '1px', background: '#6f7a8e', transform: 'rotate(-28deg)' } })],
    line: () => [bar(12, 4, '#cfd6e4', { clipPath: 'polygon(0 0,100% 0,100% 30%,75% 30%,75% 60%,100% 60%,100% 100%,0 100%)' })],
    wavy: () => [h('i', { style: { left: '6px', right: '6px', top: '2px', bottom: '2px', border: '0', background: 'repeating-linear-gradient(120deg, transparent 0 3px, #cfd6e4 3px 5px, transparent 5px 8px)' } })],
  };

  // The home page hero: a fan of real drawn tracers and a hit burst.
  function hero(canvas, draw) {
    const r = canvas.getBoundingClientRect();
    const dpr = Math.min(2, root.devicePixelRatio || 1);
    const w = Math.max(1, Math.round(r.width * dpr)), hgt = Math.max(1, Math.round(r.height * dpr));
    canvas.width = w; canvas.height = hgt;
    const ctx = canvas.getContext('2d');
    ctx.clearRect(0, 0, w, hgt);
    const looks = [
      { beam: { style: 'neon', colour: '#ff3dd6', colour2: '#38d9ff', core: '#ffffff', width: 4 }, smoke: { mode: 'off' }, glow: '#ff3dd6', from: [0.05, 1.02], to: [0.82, 0.3] },
      { beam: { style: 'solid', colour: '#39e6ff', width: 3 }, smoke: { mode: 'line', colour: '#39e6ff', width: 2 }, glow: '#39e6ff', from: [0.2, 1.08], to: [0.86, 0.42] },
      { beam: { style: 'none' }, smoke: { mode: 'wavy', colour: '#ffc21f', width: 4 }, glow: '#ffc21f', from: [0.38, 1.1], to: [0.9, 0.52] },
      { beam: { style: 'neon', colour: '#ff2a3d', core: '#ffffff', width: 3 }, smoke: { mode: 'off' }, glow: '#ff2a3d', from: [0.0, 0.72], to: [0.8, 0.36] },
    ];
    ctx.globalCompositeOperation = 'lighter';
    for (const l of looks) {
      const src = sheetCanvas({ px: draw(128, 256, { beam: l.beam, smoke: l.smoke }), w: 128, h: 256 });
      const [x0, y0] = [l.from[0] * w, l.from[1] * hgt], [x1, y1] = [l.to[0] * w, l.to[1] * hgt];
      const width = 26 * dpr;
      strip(ctx, src, 0, x0, y0, x1, y1, width, 0.8);
      ctx.shadowColor = l.glow; ctx.shadowBlur = 18 * dpr;
      strip(ctx, src, 3, x0, y0, x1, y1, width, 0.9);
      ctx.shadowBlur = 0;
      strip(ctx, src, 3, x0, y0, x1, y1, width, 1);
    }
    // confetti burst where the tracers meet
    const cx = 0.84 * w, cy = 0.36 * hgt;
    let seed = 11;
    const rand = () => { seed = (seed * 16807) % 2147483647; return seed / 2147483647; };
    const colours = ['#ff3d8b', '#ffd23f', '#3ddcff', '#7dff6a', '#b26bff', '#ff8c3d'];
    ctx.globalCompositeOperation = 'source-over';
    for (let i = 0; i < 70; i++) {
      const a = rand() * Math.PI * 2, d = (12 + rand() * 90) * dpr;
      ctx.save();
      ctx.translate(cx + Math.cos(a) * d * 1.3, cy + Math.sin(a) * d);
      ctx.rotate(rand() * 6);
      ctx.globalAlpha = 0.5 + rand() * 0.5;
      ctx.fillStyle = colours[i % colours.length];
      ctx.fillRect(-3 * dpr, -1.5 * dpr, 6 * dpr, 3 * dpr);
      ctx.restore();
    }
    const glow = ctx.createRadialGradient(cx, cy, 0, cx, cy, 70 * dpr);
    glow.addColorStop(0, 'rgba(255,255,255,0.55)'); glow.addColorStop(1, 'rgba(255,255,255,0)');
    ctx.fillStyle = glow; ctx.beginPath(); ctx.arc(cx, cy, 70 * dpr, 0, Math.PI * 2); ctx.fill();
  }

  // A decoded texture drawn to fit a box (for the pack explorer).
  function textureCanvas(px, w, hgt, max) {
    const c = document.createElement('canvas');
    const k = Math.min(1, max / Math.max(w, hgt));
    c.width = Math.max(1, Math.round(w * k)); c.height = Math.max(1, Math.round(hgt * k));
    const full = sheetCanvas({ px, w, h: hgt });
    const ctx = c.getContext('2d');
    ctx.imageSmoothingQuality = 'high';
    ctx.drawImage(full, 0, 0, c.width, c.height);
    return c;
  }

  root.BfxArt = { sheetCanvas, strip, tracerThumb, fxThumb, BEAM_ART, SMOKE_ART, hero, textureCanvas };
})(typeof self !== 'undefined' ? self : this);
