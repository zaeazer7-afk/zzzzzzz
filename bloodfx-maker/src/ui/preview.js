/* BloodFX Maker preview stage: a canvas sketch of the tracer and the hit effect. Not a render of GTA's lighting. */
(function (root) {
  'use strict';

  function rng(seed) {
    return () => {
      seed |= 0; seed = (seed + 0x6D2B79F5) | 0;
      let t = Math.imul(seed ^ (seed >>> 15), 1 | seed);
      t = (t + Math.imul(t ^ (t >>> 7), 61 | t)) ^ t;
      return ((t ^ (t >>> 14)) >>> 0) / 4294967296;
    };
  }
  const PALETTES = {
    confetti: ['#ff3d8b', '#ffd23f', '#3ddcff', '#7dff6a', '#b26bff', '#ff8c3d'],
    paint: ['#ff3355', '#33aaff', '#ffdd33', '#55ee77', '#cc66ff'],
    firework: ['#ffcf4a', '#ff5ea8', '#7ee8ff', '#ffffff'],
  };

  function create(canvas, opts) {
    const ctx = canvas.getContext('2d');
    const sheet = document.createElement('canvas');
    sheet.width = 128; sheet.height = 256;
    const sctx = sheet.getContext('2d');
    const reduce = root.matchMedia && root.matchMedia('(prefers-reduced-motion: reduce)').matches;
    const view = { scene: 'street', camera: 'side', slowmo: false, paused: false, auto: true, focus: null, image: null };
    let W = 0, H = 0, dpr = 1, backdrop = null;
    const shots = [], parts = [];
    let clock = 0, lastReal = 0, nextShot = 0.4, shotCount = 0, running = false, visible = true;

    function setSheet(px, w, h) {
      if (sheet.width !== w || sheet.height !== h) { sheet.width = w; sheet.height = h; }
      const img = sctx.createImageData(w, h);
      img.data.set(px);
      sctx.putImageData(img, 0, 0);
      if (!running) still();
    }

    // ------------------------------------------------------------ backdrops
    function paintStreet(b) {
      const sky = b.createLinearGradient(0, 0, 0, H);
      sky.addColorStop(0, '#04060b'); sky.addColorStop(0.55, '#111a2c'); sky.addColorStop(0.78, '#2b2236'); sky.addColorStop(1, '#0a0d14');
      b.fillStyle = sky; b.fillRect(0, 0, W, H);
      const rand = rng(7);
      const horizon = H * 0.74;
      for (let layer = 0; layer < 2; layer++) {
        let x = -10;
        while (x < W) {
          const bw = (30 + rand() * 70) * dpr * (layer ? 1.2 : 0.8);
          const bh = H * (layer ? 0.12 + rand() * 0.3 : 0.2 + rand() * 0.38);
          b.fillStyle = layer ? '#0a0e17' : '#131a28';
          b.fillRect(x, horizon - bh, bw, bh);
          if (layer) {
            for (let wy = horizon - bh + 8 * dpr; wy < horizon - 6 * dpr; wy += 9 * dpr) {
              for (let wx = x + 5 * dpr; wx < x + bw - 6 * dpr; wx += 8 * dpr) {
                if (rand() < 0.16) { b.fillStyle = rand() < 0.7 ? 'rgba(255,190,120,0.55)' : 'rgba(170,210,255,0.45)'; b.fillRect(wx, wy, 3 * dpr, 4 * dpr); }
              }
            }
          }
          x += bw + rand() * 6 * dpr;
        }
      }
      const ground = b.createLinearGradient(0, horizon, 0, H);
      ground.addColorStop(0, '#10141c'); ground.addColorStop(1, '#06080d');
      b.fillStyle = ground; b.fillRect(0, horizon, W, H - horizon);
      b.strokeStyle = 'rgba(120,140,170,0.08)'; b.lineWidth = dpr;
      for (let i = -12; i <= 12; i++) { b.beginPath(); b.moveTo(W / 2 + i * W * 0.02, horizon); b.lineTo(W / 2 + i * W * 0.16, H); b.stroke(); }
      for (let j = 1; j < 6; j++) { const yy = horizon + (H - horizon) * (j / 6) ** 1.6; b.beginPath(); b.moveTo(0, yy); b.lineTo(W, yy); b.stroke(); }
      const lamp = b.createRadialGradient(W * 0.55, horizon - H * 0.2, 0, W * 0.55, horizon - H * 0.2, H * 0.7);
      lamp.addColorStop(0, 'rgba(255,170,80,0.10)'); lamp.addColorStop(1, 'rgba(255,170,80,0)');
      b.fillStyle = lamp; b.fillRect(0, 0, W, H);
    }

    function paintRange(b) {
      const wall = b.createLinearGradient(0, 0, 0, H);
      wall.addColorStop(0, '#0b0d12'); wall.addColorStop(0.5, '#141922'); wall.addColorStop(1, '#07090d');
      b.fillStyle = wall; b.fillRect(0, 0, W, H);
      const vx = W * 0.55, vy = H * 0.5;
      // ceiling baffles and floor lanes converging on the target wall
      b.strokeStyle = 'rgba(150,165,190,0.10)'; b.lineWidth = dpr;
      for (let i = -8; i <= 8; i++) {
        b.beginPath(); b.moveTo(vx + i * W * 0.012, vy + H * 0.06); b.lineTo(vx + i * W * 0.2, H); b.stroke();
        b.beginPath(); b.moveTo(vx + i * W * 0.012, vy - H * 0.06); b.lineTo(vx + i * W * 0.2, 0); b.stroke();
      }
      for (let j = 1; j < 7; j++) {
        const k = (j / 7) ** 1.7;
        const yb = vy + H * 0.06 + (H - vy - H * 0.06) * k, yt = vy - H * 0.06 - (vy - H * 0.06) * k;
        b.beginPath(); b.moveTo(0, yb); b.lineTo(W, yb); b.stroke();
        b.fillStyle = `rgba(255,236,200,${0.05 + 0.1 * k})`;
        b.fillRect(vx - W * (0.03 + 0.28 * k), yt - 2 * dpr, W * (0.06 + 0.56 * k), 3 * dpr);
      }
      b.fillStyle = '#1a2130';
      b.fillRect(vx - W * 0.09, vy - H * 0.06, W * 0.18, H * 0.12);
      const glow = b.createRadialGradient(vx, vy, 0, vx, vy, H * 0.6);
      glow.addColorStop(0, 'rgba(255,214,150,0.12)'); glow.addColorStop(1, 'rgba(255,214,150,0)');
      b.fillStyle = glow; b.fillRect(0, 0, W, H);
    }

    // A screenshot from the player's own game, cropped to fill the stage.
    function paintImage(b, img) {
      const k = Math.max(W / img.naturalWidth, H / img.naturalHeight);
      const w = img.naturalWidth * k, h = img.naturalHeight * k;
      b.fillStyle = '#000'; b.fillRect(0, 0, W, H);
      b.drawImage(img, (W - w) / 2, (H - h) / 2, w, h);
      b.fillStyle = 'rgba(0,0,0,0.12)'; b.fillRect(0, 0, W, H);
    }

    function resize() {
      const r = canvas.getBoundingClientRect();
      dpr = Math.min(2, root.devicePixelRatio || 1);
      W = Math.max(1, Math.round(r.width * dpr)); H = Math.max(1, Math.round(r.height * dpr));
      canvas.width = W; canvas.height = H;
      backdrop = document.createElement('canvas');
      backdrop.width = W; backdrop.height = H;
      const b = backdrop.getContext('2d');
      if (view.scene === 'image' && view.image) paintImage(b, view.image);
      else if (view.scene === 'range') paintRange(b);
      else paintStreet(b);
      if (!running) still();
    }

    // ------------------------------------------------------------ geometry per camera
    function rig() {
      if (view.camera === 'shooter') {
        const s = H / 400;
        return { muzzle: [W * 0.66, H * 0.97], dummy: { x: W * 0.52, headY: H * 0.43, r: 9 * s, top: H * 0.455, bottom: H * 0.6, w: 20 * s }, perspective: true, scale: 0.45 };
      }
      return { muzzle: [W * 0.07, H * 0.66], dummy: { x: W * 0.8, headY: H * 0.33, r: H * 0.045, top: H * 0.39, bottom: H * 0.74, w: H * 0.1 }, perspective: false, scale: 1 };
    }

    function drawDummy(c, g) {
      const d = g.dummy;
      if (view.scene === 'image' && view.image) {
        c.save(); c.strokeStyle = 'rgba(255,255,255,0.85)'; c.lineWidth = 1.5 * dpr;
        const x = d.x, y = d.headY, r = 6 * dpr;
        c.beginPath(); c.moveTo(x - r * 2, y); c.lineTo(x - r * 0.6, y); c.moveTo(x + r * 0.6, y); c.lineTo(x + r * 2, y);
        c.moveTo(x, y - r * 2); c.lineTo(x, y - r * 0.6); c.moveTo(x, y + r * 0.6); c.lineTo(x, y + r * 2); c.stroke(); c.restore();
        return;
      }
      c.save();
      c.fillStyle = '#273042'; c.strokeStyle = 'rgba(160,185,220,0.38)'; c.lineWidth = 1.5 * dpr;
      c.beginPath(); c.arc(d.x, d.headY, d.r, 0, Math.PI * 2); c.fill(); c.stroke();
      c.beginPath();
      if (c.roundRect) c.roundRect(d.x - d.w / 2, d.top, d.w, d.bottom - d.top, d.w * 0.35); else c.rect(d.x - d.w / 2, d.top, d.w, d.bottom - d.top);
      c.fill(); c.stroke();
      c.fillStyle = '#1b2230';
      c.fillRect(d.x - d.w * 0.04, d.bottom, d.w * 0.08, (d.bottom - d.top) * 0.18);
      c.restore();
      if (!g.perspective) {
        const [mx, my] = g.muzzle;
        c.save(); c.fillStyle = '#1d2433'; c.strokeStyle = 'rgba(160,185,220,0.3)'; c.lineWidth = dpr;
        c.beginPath(); c.moveTo(mx - W * 0.06, my + H * 0.015); c.lineTo(mx, my - H * 0.005); c.lineTo(mx, my + H * 0.02); c.lineTo(mx - W * 0.05, my + H * 0.06); c.closePath(); c.fill(); c.stroke();
        c.restore();
      } else {
        // first-person gun silhouette at the bottom right
        const [mx, my] = g.muzzle;
        c.save(); c.fillStyle = '#121722'; c.strokeStyle = 'rgba(160,185,220,0.25)'; c.lineWidth = dpr;
        c.beginPath(); c.moveTo(mx - W * 0.02, my - H * 0.02); c.lineTo(mx + W * 0.02, my - H * 0.03); c.lineTo(mx + W * 0.2, H); c.lineTo(mx + W * 0.04, H); c.closePath(); c.fill(); c.stroke();
        c.restore();
      }
    }

    // ------------------------------------------------------------ tracer
    const speedNow = () => Math.max(0.1, opts.getTracer().speed);

    function fire(zone) {
      shotCount++;
      const g = rig();
      const z = zone || view.focus || (shotCount % 2 ? 'head' : 'body');
      const d = g.dummy;
      const to = z === 'head' ? [d.x - d.r * 0.3, d.headY] : [d.x - d.w * 0.2, d.top + (d.bottom - d.top) * 0.35];
      const t = opts.getTracer();
      const timing = opts.getTiming ? opts.getTiming() : null;
      const lerp = (range) => range[0] + (range[1] - range[0]) * Math.random();
      shots.push({ born: clock, from: g.muzzle.slice(), to, zone: z, perspective: g.perspective, scale: g.scale,
        travel: Math.max(0.05, 0.14 / speedNow()), linger: Math.min(3, t.stay / speedNow()), hit: false,
        smoke: t.smoke !== 'off', smokeLife: Math.min(4, (timing ? lerp(timing.smokeLife) : 0.6) / speedNow()),
        smokeAlpha: timing ? timing.smokeAlpha : null });
      if (shots.length > 8) shots.shift();
    }

    // Piecewise-linear alpha over life from the pack's keyframes ([time, alpha] pairs); a soft fade when there are none.
    function envelope(keys, f) {
      if (!keys || !keys.length) return f < 0.1 ? f / 0.1 : 1 - (f - 0.1) / 0.9;
      if (f <= keys[0][0]) return keys[0][1];
      for (let i = 1; i < keys.length; i++) {
        if (f <= keys[i][0]) {
          const [t0, a0] = keys[i - 1], [t1, a1] = keys[i];
          return a0 + (a1 - a0) * ((f - t0) / Math.max(1e-6, t1 - t0));
        }
      }
      return keys[keys.length - 1][1];
    }

    function strip(c, index, x0, y0, x1, y1, w0, w1, alpha, v0, v1) {
      const dx = x1 - x0, dy = y1 - y0, len = Math.hypot(dx, dy);
      if (len < 1 || alpha <= 0) return;
      const fw = sheet.width / 4, sh = sheet.height;
      c.save();
      c.globalAlpha = Math.min(1, alpha);
      c.translate(x0, y0);
      c.rotate(Math.atan2(dy, dx) - Math.PI / 2);
      if (w0 === w1) {
        c.drawImage(sheet, index * fw, sh * v0, fw, sh * (v1 - v0), -w0 / 2, 0, w0, len);
      } else {
        const n = 14;
        for (let i = 0; i < n; i++) {
          const a = i / n, b = (i + 1) / n;
          const w = w0 + (w1 - w0) * (a + b) / 2;
          c.drawImage(sheet, index * fw, sh * (v0 + (v1 - v0) * a), fw, Math.max(1, sh * (v1 - v0) / n), -w / 2, len * a, w, len / n + 0.5);
        }
      }
      c.restore();
    }

    function drawShots(c) {
      const t = opts.getTracer();
      const base = 30 * dpr * t.zoom;
      c.save();
      c.globalCompositeOperation = 'lighter';
      for (const s of shots) {
        const age = clock - s.born;
        const k = Math.min(1, age / s.travel);
        const [x0, y0] = s.from, [x1, y1] = s.to;
        const hx = x0 + (x1 - x0) * k, hy = y0 + (y1 - y0) * k;
        if (k >= 1 && !s.hit) { s.hit = true; spawn(x1, y1, s.zone, s.scale); }
        const streak = Math.min(1, 0.22 + t.life * 4.5);
        const tk = Math.max(0, k - streak);
        const tx = x0 + (x1 - x0) * tk, ty = y0 + (y1 - y0) * tk;
        const fade = k < 1 ? 1 : Math.max(0, 1 - (age - s.travel) / Math.max(0.06, s.linger));
        // width shrinks with distance in the shooter view
        const wAt = (p) => (s.perspective ? base * (1.6 - 1.45 * p) : base);
        if (s.smoke && age < s.smokeLife) {
          // the game steps through smoke frames 0-2 over the smoke's life and blends between them
          const f = age / s.smokeLife;
          const pos = Math.min(1.999, f * 2);
          const fr = Math.floor(pos), mix = pos - fr;
          const env = envelope(s.smokeAlpha, f);
          strip(c, fr, x0, y0, hx, hy, wAt(0) * 1.1, wAt(k) * 1.1, env * (1 - mix) * 0.95, 0, 1);
          strip(c, fr + 1, x0, y0, hx, hy, wAt(0) * 1.1, wAt(k) * 1.1, env * mix * 0.95, 0, 1);
        }
        if (fade > 0) {
          c.shadowColor = t.style === 'keep' ? (t.tintOn ? t.tint : '#ffb070') : t.colour;
          c.shadowBlur = (4 + t.glow * 0.9) * dpr * (s.perspective ? 0.7 : 1);
          strip(c, 3, tx, ty, hx, hy, wAt(tk), wAt(k), fade * (0.55 + Math.min(t.glow, 40) / 60), 0, 1);
          c.shadowBlur = 0;
          strip(c, 3, tx, ty, hx, hy, wAt(tk), wAt(k), fade, 0, 1);
        }
        if (age < 0.06) {
          const r = (s.perspective ? 40 : 22) * dpr;
          const gr = c.createRadialGradient(x0, y0, 0, x0, y0, r);
          gr.addColorStop(0, 'rgba(255,220,160,0.9)'); gr.addColorStop(1, 'rgba(255,160,60,0)');
          c.fillStyle = gr; c.beginPath(); c.arc(x0, y0, r, 0, Math.PI * 2); c.fill();
        }
      }
      c.restore();
    }

    // ------------------------------------------------------------ hit particles
    function spawn(x, y, zone, scale) {
      const fx = opts.getHit(zone);
      const kind = opts.kindOf(fx);
      const u = (H / 400) * (scale || 1);
      const r = Math.random;
      const add = (o) => parts.push({ x, y, vx: 0, vy: 0, g: 600 * u, life: 1.2, age: 0, size: 3 * u, shape: 'dot', colour: '#fff', spin: 0, rot: r() * 6, add: false, drag: 0.6, floor: y + 120 * u, ...o });
      const burst = (n, speed, make) => {
        for (let i = 0; i < n; i++) {
          const a = -Math.PI * (0.05 + r() * 1.1) + Math.PI * 0.6 * (r() - 0.5);
          const s = speed * (0.3 + r() * 0.9) * u;
          add({ vx: Math.cos(a) * s - 40 * u * r(), vy: Math.sin(a) * s, ...make(i) });
        }
      };
      const pick = (list) => list[Math.floor(r() * list.length)];
      parts.push({ x, y, ring: true, age: 0, life: 0.25, colour: kind === 'none' ? 'rgba(255,255,255,0.45)' : '#ffffff', size: 18 * u });
      switch (kind) {
        case 'confetti': burst(46, 260, () => ({ shape: 'rect', size: 4 * u, colour: pick(PALETTES.confetti), spin: (r() - 0.5) * 18, g: 380 * u, life: 1.8, drag: 1.4 })); break;
        case 'balloon': burst(10, 200, () => ({ shape: 'rect', size: 5 * u, colour: '#e8344a', spin: (r() - 0.5) * 10, life: 1 })); parts.push({ x, y, ring: true, age: 0, life: 0.35, colour: '#ff5a6e', size: 34 * u }); break;
        case 'gumballs': burst(20, 220, () => ({ shape: 'ball', size: 5 * u, colour: pick(PALETTES.confetti), g: 700 * u, life: 2, bounce: true })); break;
        case 'cash': burst(14, 170, () => ({ shape: 'bill', size: 7 * u, colour: r() < 0.5 ? '#4caf50' : '#7fcf73', spin: (r() - 0.5) * 6, g: 120 * u, life: 2.2, drag: 1.8, flutter: true })); break;
        case 'coins': burst(16, 240, () => ({ shape: 'ball', size: 3.5 * u, colour: '#f5c542', g: 800 * u, life: 1.6, bounce: true })); break;
        case 'water': burst(34, 280, () => ({ shape: 'drop', size: 2.6 * u, colour: '#8fd8ff', life: 0.9 })); break;
        case 'oil': burst(26, 230, () => ({ shape: 'drop', size: 3 * u, colour: '#1c1a17', edge: '#6b6458', life: 1.1 })); break;
        case 'paint': burst(30, 250, () => ({ shape: 'drop', size: 3 * u, colour: pick(PALETTES.paint), life: 1 })); break;
        case 'gloop': burst(22, 200, () => ({ shape: 'drop', size: 3.6 * u, colour: '#8bd450', life: 1.1 })); break;
        case 'gore': burst(48, 300, () => ({ shape: 'drop', size: 3.2 * u, colour: r() < 0.6 ? '#8a0d14' : '#5c070c', life: 1.2 })); parts.push({ x, y, mist: true, age: 0, life: 0.6, colour: '150,10,20', size: 40 * u }); break;
        case 'mist': parts.push({ x, y, mist: true, age: 0, life: 0.9, colour: '170,20,30', size: 46 * u }); break;
        case 'blood': burst(24, 230, () => ({ shape: 'drop', size: 2.6 * u, colour: r() < 0.6 ? '#9b0d14' : '#6a0a10', life: 0.9 })); parts.push({ x, y, mist: true, age: 0, life: 0.4, colour: '150,10,20', size: 22 * u }); break;
        case 'feathers': burst(14, 140, () => ({ shape: 'feather', size: 6 * u, colour: '#f2f2f2', g: 60 * u, life: 2.4, drag: 2.2, flutter: true, spin: (r() - 0.5) * 4 })); break;
        case 'paper': burst(12, 160, () => ({ shape: 'bill', size: 7 * u, colour: '#f4f4ef', g: 90 * u, life: 2.2, drag: 2, flutter: true, spin: (r() - 0.5) * 5 })); break;
        case 'glass': burst(22, 260, () => ({ shape: 'shard', size: 4 * u, colour: 'rgba(190,235,255,0.85)', spin: (r() - 0.5) * 16, life: 1 })); break;
        case 'sparks': burst(30, 420, () => ({ shape: 'spark', size: 2 * u, colour: '#fff3b0', g: 500 * u, life: 0.5, add: true })); break;
        case 'foam': burst(14, 120, () => ({ shape: 'puff', size: 7 * u, colour: 'rgba(240,244,250,0.8)', g: 40 * u, life: 1, drag: 2 })); break;
        case 'mud': burst(26, 220, () => ({ shape: 'drop', size: 3.2 * u, colour: '#6b4a2b', life: 1 })); break;
        case 'snow': burst(30, 160, () => ({ shape: 'ball', size: 2.4 * u, colour: '#ffffff', g: 90 * u, life: 2, drag: 1.6 })); break;
        case 'champagne': burst(30, 220, () => ({ shape: 'ball', size: 2.2 * u, colour: r() < 0.5 ? '#f6e7a1' : '#ffffff', g: 260 * u, life: 1.3 })); break;
        case 'star': parts.push({ x, y, star: true, age: 0, life: 0.3, colour: '#fff6c2', size: 30 * u }); break;
        case 'firework': burst(40, 330, () => ({ shape: 'spark', size: 2 * u, colour: pick(PALETTES.firework), g: 160 * u, life: 1.1, add: true })); break;
        case 'none': break;
        default: parts.push({ x, y, mist: true, age: 0, life: 0.6, colour: '200,205,215', size: 30 * u });
      }
      if (parts.length > 700) parts.splice(0, parts.length - 700);
    }

    function drawParts(c, dt) {
      for (let i = parts.length - 1; i >= 0; i--) {
        const p = parts[i];
        p.age += dt;
        if (p.age >= p.life) { parts.splice(i, 1); continue; }
        const a = 1 - p.age / p.life;
        if (p.ring) {
          c.save(); c.globalAlpha = a; c.strokeStyle = p.colour; c.lineWidth = 2 * dpr;
          c.beginPath(); c.arc(p.x, p.y, p.size * (0.4 + (1 - a)), 0, Math.PI * 2); c.stroke(); c.restore();
          continue;
        }
        if (p.mist) {
          const rad = p.size * (0.6 + (1 - a) * 0.9);
          const g = c.createRadialGradient(p.x, p.y, 0, p.x, p.y, rad);
          g.addColorStop(0, `rgba(${p.colour},${0.55 * a})`); g.addColorStop(1, `rgba(${p.colour},0)`);
          c.fillStyle = g; c.beginPath(); c.arc(p.x, p.y, rad, 0, Math.PI * 2); c.fill();
          continue;
        }
        if (p.star) {
          c.save(); c.globalCompositeOperation = 'lighter'; c.globalAlpha = a; c.fillStyle = p.colour; c.translate(p.x, p.y);
          const s = p.size * (0.6 + (1 - a));
          c.beginPath();
          for (let k = 0; k < 8; k++) { const rr = k % 2 ? s * 0.18 : s; const an = (k / 8) * Math.PI * 2; c.lineTo(Math.cos(an) * rr, Math.sin(an) * rr); }
          c.closePath(); c.fill(); c.restore();
          continue;
        }
        p.vy += p.g * dt;
        p.vx *= Math.exp(-p.drag * dt);
        if (p.flutter) { p.vy *= Math.exp(-2.5 * dt); p.vx += Math.sin(p.age * 7 + p.rot) * 30 * dt * dpr; }
        p.x += p.vx * dt; p.y += p.vy * dt;
        p.rot += p.spin * dt;
        if (p.bounce && p.y > p.floor && p.vy > 0) { p.y = p.floor; p.vy *= -0.45; p.vx *= 0.7; }
        c.save();
        c.globalAlpha = Math.min(1, a * 1.4);
        if (p.add) c.globalCompositeOperation = 'lighter';
        c.translate(p.x, p.y);
        c.fillStyle = p.colour;
        switch (p.shape) {
          case 'rect': c.rotate(p.rot); c.fillRect(-p.size / 2, -p.size / 4, p.size, p.size / 2); break;
          case 'bill': c.rotate(p.rot); c.fillRect(-p.size, -p.size / 2, p.size * 2, p.size); c.fillStyle = 'rgba(0,0,0,0.18)'; c.fillRect(-p.size * 0.3, -p.size * 0.3, p.size * 0.6, p.size * 0.6); break;
          case 'feather': c.rotate(p.rot); c.beginPath(); c.ellipse(0, 0, p.size, p.size * 0.32, 0, 0, Math.PI * 2); c.fill(); break;
          case 'ball': c.beginPath(); c.arc(0, 0, p.size, 0, Math.PI * 2); c.fill(); c.fillStyle = 'rgba(255,255,255,0.45)'; c.beginPath(); c.arc(-p.size * 0.3, -p.size * 0.3, p.size * 0.35, 0, Math.PI * 2); c.fill(); break;
          case 'shard': c.rotate(p.rot); c.beginPath(); c.moveTo(0, -p.size); c.lineTo(p.size * 0.6, p.size * 0.7); c.lineTo(-p.size * 0.5, p.size * 0.4); c.closePath(); c.fill(); break;
          case 'spark': { const vl = Math.hypot(p.vx, p.vy) || 1; c.strokeStyle = p.colour; c.lineWidth = p.size; c.beginPath(); c.moveTo(0, 0); c.lineTo(-p.vx / vl * p.size * 5, -p.vy / vl * p.size * 5); c.stroke(); break; }
          case 'puff': { const g = c.createRadialGradient(0, 0, 0, 0, 0, p.size * 1.6); g.addColorStop(0, p.colour); g.addColorStop(1, 'rgba(240,244,250,0)'); c.fillStyle = g; c.beginPath(); c.arc(0, 0, p.size * 1.6, 0, Math.PI * 2); c.fill(); break; }
          default: {
            c.rotate(Math.atan2(p.vy, p.vx));
            c.beginPath(); c.ellipse(0, 0, p.size * 1.8, p.size, 0, 0, Math.PI * 2); c.fill();
            if (p.edge) { c.strokeStyle = p.edge; c.lineWidth = dpr; c.stroke(); }
          }
        }
        c.restore();
      }
    }

    // ------------------------------------------------------------ loop
    function draw(dt) {
      if (!backdrop) return;
      const g = rig();
      ctx.drawImage(backdrop, 0, 0);
      drawDummy(ctx, g);
      drawShots(ctx);
      drawParts(ctx, dt);
    }

    function frame(ts) {
      if (!running) return;
      const real = ts / 1000;
      const dtReal = Math.min(0.05, lastReal ? real - lastReal : 0.016);
      lastReal = real;
      const dt = view.paused ? 0 : dtReal * (view.slowmo ? 0.25 : 1);
      clock += dt;
      if (view.auto && !view.paused && clock >= nextShot) {
        fire();
        const t = opts.getTracer();
        nextShot = clock + Math.max(1.5, 0.14 / speedNow() + Math.min(3, t.stay / speedNow()) + 0.9);
      }
      draw(dt);
      root.requestAnimationFrame(frame);
    }

    // A still frame for reduced motion and while hidden: a beam halfway to the head.
    function still() {
      if (!backdrop || running) return;
      const saveShots = shots.splice(0), saveParts = parts.splice(0);
      const g = rig();
      shots.push({ born: clock - 0.1, from: g.muzzle.slice(), to: [g.dummy.x - g.dummy.r * 0.3, g.dummy.headY], zone: 'head', perspective: g.perspective, scale: g.scale,
        travel: 0.2, linger: 1, hit: true, smoke: opts.getTracer().smoke !== 'off', smokeLife: 0.6, smokeAlpha: null });
      draw(0);
      shots.splice(0); shots.push(...saveShots); parts.push(...saveParts);
    }

    function start() {
      if (running || reduce || !visible || document.hidden) return;
      running = true; lastReal = 0;
      root.requestAnimationFrame(frame);
    }
    function stop() { running = false; still(); }

    new root.ResizeObserver(() => resize()).observe(canvas);
    if ('IntersectionObserver' in root) {
      new root.IntersectionObserver((entries) => { visible = entries[0].isIntersecting; if (visible) start(); else stop(); }).observe(canvas);
    }
    document.addEventListener('visibilitychange', () => { if (document.hidden) stop(); else start(); });

    return {
      setSheet,
      resize,
      start,
      still,
      fire(zone) {
        if (reduce && !running) { running = true; lastReal = 0; view.auto = false; fire(zone); root.requestAnimationFrame(frame); setTimeout(() => { running = false; still(); }, 2500); return; }
        fire(zone);
      },
      soon() { nextShot = Math.min(nextShot, clock + 0.15); if (!running) still(); },
      set(key, value) {
        view[key] = value;
        if (key === 'scene' || key === 'camera' || key === 'image') { shots.splice(0); parts.splice(0); resize(); }
        if (!running) still();
      },
      get: (key) => view[key],
      reduced: reduce,
    };
  }

  root.BfxPreview = { create };
})(typeof self !== 'undefined' ? self : this);
