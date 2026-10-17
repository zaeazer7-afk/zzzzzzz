/* Procedural blood splatter renderer. No image assets: every splat is drawn from a seed. */
(function (root) {
  'use strict';

  const TAU = Math.PI * 2;

  function mulberry32(seed) {
    let a = seed >>> 0;
    return function () {
      a = (a + 0x6d2b79f5) >>> 0;
      let t = a;
      t = Math.imul(t ^ (t >>> 15), t | 1);
      t ^= t + Math.imul(t ^ (t >>> 7), t | 61);
      return ((t ^ (t >>> 14)) >>> 0) / 4294967296;
    };
  }

  function hexToRgb(hex) {
    let h = String(hex || '').trim().replace(/^#/, '');
    if (h.length === 3) h = h.split('').map((c) => c + c).join('');
    if (!/^[0-9a-f]{6}$/i.test(h)) return { r: 94, g: 0, b: 0 };
    const n = parseInt(h, 16);
    return { r: (n >> 16) & 255, g: (n >> 8) & 255, b: n & 255 };
  }

  function toHex(c) {
    const p = (v) => Math.max(0, Math.min(255, Math.round(v))).toString(16).padStart(2, '0');
    return `#${p(c.r)}${p(c.g)}${p(c.b)}`;
  }

  function mix(a, b, t) {
    return { r: a.r + (b.r - a.r) * t, g: a.g + (b.g - a.g) * t, b: a.b + (b.b - a.b) * t };
  }

  function rgba(c, alpha) {
    return `rgba(${Math.round(c.r)},${Math.round(c.g)},${Math.round(c.b)},${alpha})`;
  }

  function luminance(c) {
    const f = (v) => {
      v /= 255;
      return v <= 0.03928 ? v / 12.92 : Math.pow((v + 0.055) / 1.055, 2.4);
    };
    return 0.2126 * f(c.r) + 0.7152 * f(c.g) + 0.0722 * f(c.b);
  }

  // A version of the colour light enough to read as an accent on a dark UI.
  function accent(hex) {
    let c = hexToRgb(hex);
    for (let i = 0; i < 12 && luminance(c) < 0.11; i++) c = mix(c, { r: 255, g: 70, b: 80 }, 0.22);
    return toHex(c);
  }

  const WHITE = { r: 255, g: 255, b: 255 };
  const BLACK = { r: 0, g: 0, b: 0 };

  function angleDiff(a, b) {
    const d = a - b;
    return Math.atan2(Math.sin(d), Math.cos(d));
  }

  function makeProfile(rnd, opts) {
    const waves = [];
    for (let i = 0; i < 5; i++) {
      waves.push({ f: 2 + Math.floor(rnd() * 7), a: 0.025 + rnd() * 0.05, p: rnd() * TAU });
    }
    const spikes = [];
    const count = opts.spikes != null ? opts.spikes : 5 + Math.floor(rnd() * 9);
    const reach = opts.spikeLength != null ? opts.spikeLength : 1;
    for (let i = 0; i < count; i++) {
      spikes.push({ at: rnd() * TAU, len: (0.2 + rnd() * 0.85) * reach, w: 0.045 + rnd() * 0.08 });
    }
    return { waves, spikes };
  }

  // Outline radius (as a multiple of R) at angle theta.
  function radiusAt(profile, theta, dir, stretch) {
    let r = 1;
    for (const w of profile.waves) r += w.a * Math.sin(w.f * theta + w.p);
    for (const s of profile.spikes) {
      const d = angleDiff(theta, s.at);
      r += s.len * Math.exp(-(d * d) / (2 * s.w * s.w));
    }
    if (dir != null) {
      const c = Math.cos(angleDiff(theta, dir));
      if (c > 0) r *= 1 + stretch * c * c;
    }
    return r;
  }

  function droplet(ctx, x, y, r, angle, stretch) {
    ctx.beginPath();
    ctx.ellipse(x, y, r * stretch, r, angle, 0, TAU);
    ctx.fill();
  }

  /**
   * Renders one splat onto its own canvas and returns { canvas, cx, cy }.
   * opts: dir (radians, spray direction), stretch, spikes, spikeLength, satellites, drips,
   *       dripLength, mist (fine spray dots), gloss (wet highlight, default true)
   */
  function render(color, R, seed, opts) {
    opts = opts || {};
    const rnd = mulberry32(seed || 1);
    const base = typeof color === 'string' ? hexToRgb(color) : color;
    const dir = opts.dir != null ? opts.dir : null;
    const stretch = opts.stretch != null ? opts.stretch : 0.7;
    const drips = opts.drips || 0;
    const dripLength = opts.dripLength || 1.6;

    const pad = R * 4.2;
    const width = Math.ceil(pad * 2);
    const height = Math.ceil(pad * 2 + (drips ? R * (dripLength + 1) : 0));
    const canvas = document.createElement('canvas');
    canvas.width = width;
    canvas.height = height;
    const ctx = canvas.getContext('2d');
    const cx = pad;
    const cy = pad;
    const profile = makeProfile(rnd, opts);

    ctx.fillStyle = rgba(base, 1);

    // Fine mist, drawn first so it sits under everything else.
    const mist = opts.mist || 0;
    for (let i = 0; i < mist; i++) {
      let a = rnd() * TAU;
      if (dir != null && rnd() < 0.8) a = dir + (rnd() - 0.5) * 1.3;
      const d = R * (1.0 + rnd() * 2.9);
      ctx.globalAlpha = 0.25 + rnd() * 0.5;
      droplet(ctx, cx + Math.cos(a) * d, cy + Math.sin(a) * d, Math.max(0.5, R * (0.006 + rnd() * 0.02)), a, 1);
    }
    ctx.globalAlpha = 1;

    // Main body.
    ctx.beginPath();
    const N = 180;
    for (let i = 0; i <= N; i++) {
      const t = (i / N) * TAU;
      const r = R * radiusAt(profile, t, dir, stretch);
      const x = cx + Math.cos(t) * r;
      const y = cy + Math.sin(t) * r;
      if (i === 0) ctx.moveTo(x, y);
      else ctx.lineTo(x, y);
    }
    ctx.closePath();
    ctx.fill();

    // Beads at the tips of the spikes.
    for (const s of profile.spikes) {
      if (rnd() < 0.75) {
        const r = R * radiusAt(profile, s.at, dir, stretch) + R * (0.06 + rnd() * 0.22);
        const size = R * (0.035 + rnd() * 0.06);
        droplet(ctx, cx + Math.cos(s.at) * r, cy + Math.sin(s.at) * r, size, s.at, 1.15 + rnd() * 0.9);
      }
    }

    // Satellite droplets, elongated away from the centre.
    const satellites = opts.satellites != null ? opts.satellites : 12 + Math.floor(rnd() * 22);
    for (let i = 0; i < satellites; i++) {
      let a = rnd() * TAU;
      const forward = dir != null && rnd() < 0.65;
      if (forward) a = dir + (rnd() - 0.5) * 1.5;
      const reach = forward ? 1.35 : 1.0;
      const d = R * (1.12 + Math.pow(rnd(), 1.5) * 2.0 * reach);
      const falloff = Math.max(0.35, 1.45 - d / (R * 3.2));
      const size = R * (0.012 + Math.pow(rnd(), 2.3) * 0.1) * falloff;
      droplet(ctx, cx + Math.cos(a) * d, cy + Math.sin(a) * d, Math.max(0.6, size), a, 1 + rnd() * 2.4);
    }

    // Drips running down (screen blood).
    for (let i = 0; i < drips; i++) {
      const x = cx + (rnd() - 0.5) * R * 1.2;
      const top = cy + R * (0.2 + rnd() * 0.3);
      const len = R * (0.4 + rnd() * dripLength);
      const w = R * (0.05 + rnd() * 0.07);
      ctx.beginPath();
      ctx.moveTo(x - w / 2, top);
      ctx.bezierCurveTo(x - w / 2, top + len * 0.6, x - w * 0.3, top + len * 0.8, x - w * 0.55, top + len);
      ctx.arc(x, top + len, w * 0.6, Math.PI, 0, true);
      ctx.bezierCurveTo(x + w * 0.3, top + len * 0.8, x + w / 2, top + len * 0.6, x + w / 2, top);
      ctx.closePath();
      ctx.fill();
    }

    // Shading, kept inside the shape.
    ctx.globalCompositeOperation = 'source-atop';
    const shade = ctx.createRadialGradient(cx, cy, R * 0.15, cx, cy, R * 3.4);
    shade.addColorStop(0, rgba(BLACK, 0.0));
    shade.addColorStop(0.32, rgba(BLACK, 0.16));
    shade.addColorStop(1, rgba(BLACK, 0.55));
    ctx.fillStyle = shade;
    ctx.fillRect(0, 0, width, height);

    if (opts.gloss !== false) {
      const hx = cx - R * 0.3;
      const hy = cy - R * 0.34;
      const gloss = ctx.createRadialGradient(hx, hy, 0, hx, hy, R * 0.7);
      gloss.addColorStop(0, rgba(mix(base, WHITE, 0.45), 0.3));
      gloss.addColorStop(0.35, rgba(mix(base, WHITE, 0.2), 0.1));
      gloss.addColorStop(1, rgba(base, 0));
      ctx.fillStyle = gloss;
      ctx.fillRect(0, 0, width, height);
    }
    ctx.globalCompositeOperation = 'source-over';

    return { canvas, cx, cy };
  }

  root.Splat = { render, hexToRgb, toHex, mix, accent, luminance, mulberry32 };
})(window);
