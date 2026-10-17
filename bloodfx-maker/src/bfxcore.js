/* BloodFX Maker core: RPF7 archives, RSC7 resources, core.ypt tracer patching and texture drawing.
   Works in browsers and in Node. Call setZlib(pako) (or any object with inflateRaw/deflateRaw) first. */
(function (root, factory) {
  if (typeof module === 'object' && module.exports) module.exports = factory();
  else root.BfxCore = factory();
})(typeof self !== 'undefined' ? self : this, function () {
  'use strict';

  let zlib = null;
  function setZlib(z) { zlib = z; }

  const RPF7 = 0x52504637;
  const ENC_OPEN = 0x4E45504F;
  const DIR_MARK = 0x7FFFFF00;
  const RSC7 = 0x37435352;
  const BLOCK = 512;

  const dv = (u8) => new DataView(u8.buffer, u8.byteOffset, u8.byteLength);

  function joaat(str) {
    let h = 0;
    const s = str.toLowerCase();
    for (let i = 0; i < s.length; i++) {
      h = (h + s.charCodeAt(i)) >>> 0;
      h = (h + (h << 10)) >>> 0;
      h = (h ^ (h >>> 6)) >>> 0;
    }
    h = (h + (h << 3)) >>> 0;
    h = (h ^ (h >>> 11)) >>> 0;
    h = (h + (h << 15)) >>> 0;
    return h >>> 0;
  }

  function cstring(u8, off) {
    let end = off;
    while (end < u8.length && u8[end] !== 0) end++;
    let s = '';
    for (let i = off; i < end; i++) s += String.fromCharCode(u8[i]);
    return s;
  }

  // ---------------------------------------------------------------- RPF7

  function readRpf(bytes, path) {
    path = path || 'archive.rpf';
    if (bytes.length < 16) throw new Error(`${path} is too small to be an RPF archive`);
    const d = dv(bytes);
    if (d.getUint32(0, true) !== RPF7) throw new Error(`${path} is not an RPF7 archive`);
    const count = d.getUint32(4, true);
    const namesLen = d.getUint32(8, true);
    const encryption = d.getUint32(12, true);
    if (encryption !== ENC_OPEN && encryption !== 0) {
      throw new Error(`${path} is an encrypted RPF. Open it in OpenIV, switch it to "OpenIV" (unencrypted) and save, then try again.`);
    }
    const namesStart = 16 + count * 16;
    if (namesStart + namesLen > bytes.length) throw new Error(`${path} has a damaged header`);
    const names = bytes.subarray(namesStart, namesStart + namesLen);
    const raw = [];
    for (let i = 0; i < count; i++) {
      const o = 16 + i * 16;
      const second = d.getUint32(o + 4, true);
      let e;
      if (second === DIR_MARK) {
        e = { kind: 'dir', nameOff: d.getUint32(o, true), first: d.getUint32(o + 8, true), count: d.getUint32(o + 12, true) };
      } else if ((second & 0x80000000) === 0) {
        const lo = d.getUint32(o, true);
        const csize = (lo >>> 16) | ((second & 0xFF) << 16);
        const offset = second >>> 8;
        const usize = d.getUint32(o + 8, true);
        const encrypted = d.getUint32(o + 12, true);
        const len = csize || usize;
        e = { kind: 'bin', nameOff: lo & 0xFFFF, csize, usize, encrypted,
              stored: slice(bytes, offset * BLOCK, len, path) };
      } else {
        const nameOff = d.getUint16(o, true);
        let size = bytes[o + 2] | (bytes[o + 3] << 8) | (bytes[o + 4] << 16);
        const offset = (bytes[o + 5] | (bytes[o + 6] << 8) | (bytes[o + 7] << 16)) & 0x7FFFFF;
        if (size === 0xFFFFFF) {
          const h = offset * BLOCK;
          size = (bytes[h + 7] | (bytes[h + 14] << 8) | (bytes[h + 5] << 16) | (bytes[h + 2] << 24)) >>> 0;
        }
        e = { kind: 'rsc', nameOff, sysf: d.getUint32(o + 8, true), gfxf: d.getUint32(o + 12, true),
              stored: slice(bytes, offset * BLOCK, size, path) };
      }
      e.name = cstring(names, e.nameOff);
      raw.push(e);
    }
    if (!raw.length || raw[0].kind !== 'dir') throw new Error(`${path} has no root folder`);
    const seen = new Set();
    function build(i, prefix) {
      if (seen.has(i)) throw new Error(`${path} has a looping folder table`);
      seen.add(i);
      const e = raw[i];
      const node = { kind: 'dir', name: e.name, children: [] };
      for (let j = e.first; j < e.first + e.count; j++) {
        const c = raw[j];
        if (!c) throw new Error(`${path} has a broken folder table`);
        const p = prefix + c.name;
        if (c.kind === 'dir') {
          node.children.push(build(j, p + '/'));
        } else {
          const f = { kind: c.kind, name: c.name, stored: c.stored };
          if (c.kind === 'bin') {
            f.csize = c.csize; f.usize = c.usize; f.encrypted = c.encrypted;
            if (!c.csize && !c.encrypted && /\.rpf$/i.test(c.name)) {
              try { f.rpf = readRpf(c.stored, `${path}/${p}`); } catch { f.rpf = null; }
            }
          } else {
            f.sysf = c.sysf; f.gfxf = c.gfxf;
          }
          node.children.push(f);
        }
      }
      return node;
    }
    return { encryption, root: build(0, '') };
  }

  function slice(bytes, start, len, path) {
    if (start + len > bytes.length) throw new Error(`${path} is cut short (a file points past the end)`);
    return bytes.subarray(start, start + len);
  }

  function ordinal(a, b) {
    const n = Math.min(a.length, b.length);
    for (let i = 0; i < n; i++) {
      const d = a.charCodeAt(i) - b.charCodeAt(i);
      if (d) return d;
    }
    return a.length - b.length;
  }

  function writeRpf(archive) {
    const entries = [archive.root];
    const queue = [archive.root];
    while (queue.length) {
      const dir = queue.shift();
      const kids = dir.children.slice().sort((a, b) => ordinal(a.name, b.name));
      dir._first = entries.length;
      dir._count = kids.length;
      for (const k of kids) {
        entries.push(k);
        if (k.kind === 'dir') queue.push(k);
      }
    }
    const nameOffsets = new Map();
    const nameBytes = [];
    for (const e of entries) {
      if (nameOffsets.has(e.name)) continue;
      nameOffsets.set(e.name, nameBytes.length);
      for (let i = 0; i < e.name.length; i++) nameBytes.push(e.name.charCodeAt(i) & 0xFF);
      nameBytes.push(0);
    }
    while (nameBytes.length % 16) nameBytes.push(0);
    const headerLen = 16 + entries.length * 16 + nameBytes.length;
    let block = Math.ceil(headerLen / BLOCK);
    const payloads = [];
    for (const e of entries) {
      if (e.kind === 'dir') continue;
      let data = e.stored;
      if (e.kind === 'bin' && e.rpf) {
        data = writeRpf(e.rpf);
        e.csize = 0; e.usize = data.length; e.encrypted = 0;
      }
      if (e.kind === 'rsc' && data.length >= 0xFFFFFF) throw new Error(`${e.name} is too big for an RPF (16 MB limit per resource)`);
      if (e.kind === 'bin' && e.csize > 0xFFFFFF) throw new Error(`${e.name} is too big`);
      e._offset = block;
      e._data = data;
      payloads.push(e);
      block += Math.max(1, Math.ceil(data.length / BLOCK));
    }
    const out = new Uint8Array(block * BLOCK);
    const d = dv(out);
    d.setUint32(0, RPF7, true);
    d.setUint32(4, entries.length, true);
    d.setUint32(8, nameBytes.length, true);
    d.setUint32(12, archive.encryption === 0 ? 0 : ENC_OPEN, true);
    entries.forEach((e, i) => {
      const o = 16 + i * 16;
      const nameOff = nameOffsets.get(e.name);
      if (e.kind === 'dir') {
        d.setUint32(o, nameOff, true);
        d.setUint32(o + 4, DIR_MARK, true);
        d.setUint32(o + 8, e._first, true);
        d.setUint32(o + 12, e._count, true);
      } else if (e.kind === 'bin') {
        if (nameOff > 0xFFFF) throw new Error('Too many file names in one archive');
        d.setUint32(o, (nameOff | ((e.csize & 0xFFFF) << 16)) >>> 0, true);
        d.setUint32(o + 4, (((e.csize >>> 16) & 0xFF) | (e._offset << 8)) >>> 0, true);
        d.setUint32(o + 8, e.usize >>> 0, true);
        d.setUint32(o + 12, e.encrypted ? 1 : 0, true);
      } else {
        if (nameOff > 0xFFFF) throw new Error('Too many file names in one archive');
        const size = e._data.length;
        d.setUint16(o, nameOff, true);
        out[o + 2] = size & 0xFF; out[o + 3] = (size >>> 8) & 0xFF; out[o + 4] = (size >>> 16) & 0xFF;
        out[o + 5] = e._offset & 0xFF; out[o + 6] = (e._offset >>> 8) & 0xFF; out[o + 7] = ((e._offset >>> 16) & 0x7F) | 0x80;
        d.setUint32(o + 8, e.sysf >>> 0, true);
        d.setUint32(o + 12, e.gfxf >>> 0, true);
      }
    });
    out.set(nameBytes, 16 + entries.length * 16);
    for (const e of payloads) out.set(e._data, e._offset * BLOCK);
    for (const e of entries) { delete e._first; delete e._count; delete e._offset; delete e._data; }
    return out;
  }

  // Every file in the archive (descending into nested .rpf files), with its folder path.
  function listFiles(archive, prefix, out) {
    out = out || [];
    prefix = prefix || '';
    (function walk(dir, p) {
      for (const c of dir.children) {
        const path = p + c.name;
        if (c.kind === 'dir') walk(c, path + '/');
        else {
          out.push({ path, node: c });
          if (c.rpf) listFiles(c.rpf, path + '/', out);
        }
      }
    })(archive.root, prefix);
    return out;
  }

  function readBinary(node) {
    if (node.kind !== 'bin') throw new Error(`${node.name} is not a plain file`);
    if (node.encrypted) throw new Error(`${node.name} is encrypted inside the archive`);
    return node.csize ? zlib.inflateRaw(node.stored) : node.stored;
  }

  // ---------------------------------------------------------------- RSC7

  function pageSize(flags) {
    const counts = ((flags >>> 27) & 1) + (((flags >>> 26) & 1) << 1) + (((flags >>> 25) & 1) << 2) +
      (((flags >>> 24) & 1) << 3) + (((flags >>> 17) & 0x7F) << 4) + (((flags >>> 11) & 0x3F) << 5) +
      (((flags >>> 7) & 0xF) << 6) + (((flags >>> 5) & 0x3) << 7) + (((flags >>> 4) & 1) << 8);
    return (0x200 << (flags & 0xF)) * counts;
  }

  // Accepts a resource as stored in an RPF or as a standalone .ypt file (both start with the RSC7 header).
  function openResource(stored, sysf, gfxf) {
    const d = dv(stored);
    let version, body;
    if (stored.length >= 16 && d.getUint32(0, true) === RSC7) {
      version = d.getUint32(4, true);
      sysf = d.getUint32(8, true);
      gfxf = d.getUint32(12, true);
      body = stored.subarray(16);
    } else {
      if (sysf === undefined) throw new Error('This is not a GTA V resource (no RSC7 header)');
      version = (((sysf >>> 28) & 0xF) << 4) | ((gfxf >>> 28) & 0xF);
      body = stored.subarray(16);
    }
    const data = zlib.inflateRaw(body);
    const sysSize = pageSize(sysf);
    const gfxSize = pageSize(gfxf);
    if (data.length !== sysSize + gfxSize) {
      throw new Error(`Resource size mismatch (unpacked ${data.length} bytes, header says ${sysSize + gfxSize})`);
    }
    return { version, sysf, gfxf, data, sysSize, gfxSize };
  }

  function packResource(res, level, body) {
    body = body || zlib.deflateRaw(res.data, { level: level === undefined ? 6 : level });
    const out = new Uint8Array(16 + body.length);
    const d = dv(out);
    d.setUint32(0, RSC7, true);
    d.setUint32(4, res.version, true);
    d.setUint32(8, res.sysf >>> 0, true);
    d.setUint32(12, res.gfxf >>> 0, true);
    out.set(body, 16);
    return out;
  }

  // ---------------------------------------------------------------- core.ypt navigation

  const BEHAVIOUR_COLOUR = 0x164AEA72;
  const FMT = { DXT1: 0x31545844, DXT3: 0x33545844, DXT5: 0x35545844, BC7: 0x20374342, A8R8G8B8: 21 };
  const FMT_NAME = Object.fromEntries(Object.entries(FMT).map(([k, v]) => [v, k]));

  class Ypt {
    constructor(res) {
      this.res = res;
      this.u8 = res.data;
      this.d = dv(res.data);
    }
    u32(o) { return this.d.getUint32(o, true); }
    u16(o) { return this.d.getUint16(o, true); }
    f32(o) { return this.d.getFloat32(o, true); }
    setF32(o, v) { this.d.setFloat32(o, v, true); }
    setU32(o, v) { this.d.setUint32(o, v >>> 0, true); }
    ptr(o) {
      const lo = this.u32(o), hi = this.u32(o + 4);
      if (lo === 0 && hi === 0) return -1;
      if (hi !== 0) throw new Error(`Unexpected pointer at 0x${o.toString(16)}`);
      const tag = lo >>> 28, off = lo & 0x0FFFFFFF;
      let abs;
      if (tag === 5) abs = off;
      else if (tag === 6) abs = this.res.sysSize + off;
      else throw new Error(`Bad pointer 0x${lo.toString(16)} at 0x${o.toString(16)}`);
      if (abs >= this.u8.length) throw new Error(`Pointer 0x${lo.toString(16)} points outside the file`);
      return abs;
    }
    str(o) { const p = this.ptr(o); return p < 0 ? '' : cstring(this.u8, p); }
    // ResourcePointerList64 / ResourceSimpleList64: pointer at o, u16 count at o + 8
    pointers(o) {
      const base = this.ptr(o), n = this.u16(o + 8);
      const out = [];
      for (let i = 0; i < n; i++) out.push(this.ptr(base + i * 8));
      return out;
    }
    simple(o, stride) {
      const base = this.ptr(o), n = this.u16(o + 8);
      const out = [];
      for (let i = 0; i < n; i++) out.push(base + i * stride);
      return out;
    }
    rules(dictPtrOff, nameOff) {
      const dict = this.ptr(dictPtrOff);
      const map = new Map();
      for (const p of this.pointers(dict + 0x30)) if (p >= 0) map.set(this.str(p + nameOff), p);
      return map;
    }
    effectRules() { return this._eff || (this._eff = this.rules(0x48, 0x20)); }
    emitterRules() { return this._emi || (this._emi = this.rules(0x50, 0x20)); }
    particleRules() { return this._par || (this._par = this.rules(0x38, 0x120)); }
    textures() {
      if (this._tex) return this._tex;
      const dict = this.ptr(0x20);
      const map = new Map();
      if (dict >= 0) for (const p of this.pointers(dict + 0x30)) if (p >= 0) map.set(this.str(p + 0x28), p);
      return (this._tex = map);
    }
    keyframes(kfp) {
      return this.simple(kfp + 0x70, 0x20).map((o) => ({
        off: o,
        t: [this.f32(o), this.f32(o + 4), this.f32(o + 8), this.f32(o + 12)],
        v: [this.f32(o + 16), this.f32(o + 20), this.f32(o + 24), this.f32(o + 28)],
      }));
    }
    behaviours(rule) {
      return this.pointers(rule + 0x128).map((p) => ({ off: p, type: this.u32(p + 8) }));
    }
  }

  function readEmitter(y, p) {
    const em = {
      off: p,
      emitterRule: y.str(p + 0x30),
      particleRule: y.str(p + 0x38),
      rate: [y.f32(p + 0x50), y.f32(p + 0x54)],
      zoom: [y.f32(p + 0x58), y.f32(p + 0x5C)],
      tint: [y.u32(p + 0x60), y.u32(p + 0x64)],
      lodSize: null,
    };
    const evo = y.ptr(p + 0x18);
    if (evo >= 0) {
      const want = joaat('ptxEmitterRule:m_sizeScalarKFP');
      for (const item of y.simple(evo + 0x10, 24)) {
        if (y.u32(item + 0x10) !== want) continue;
        for (const ek of y.simple(item, 0x30)) {
          if (y.u8[ek + 0x24] !== 1) continue;
          const kfs = y.simple(ek, 0x20);
          if (kfs.length) em.lodSize = { offs: kfs, value: [y.f32(kfs[0] + 16), y.f32(kfs[0] + 20), y.f32(kfs[0] + 24), y.f32(kfs[0] + 28)] };
        }
      }
    }
    return em;
  }

  function readColour(y, rule) {
    const b = y.behaviours(rule).find((x) => x.type === BEHAVIOUR_COLOUR);
    if (!b) return null;
    return {
      off: b.off,
      canTint: y.u8[b.off + 0x1E6],
      rgbaMin: y.keyframes(b.off + 0x30),
      rgbaMax: y.keyframes(b.off + 0xC0),
      emissive: y.keyframes(b.off + 0x150),
    };
  }

  function readLife(y, emitterRule) {
    if (emitterRule < 0) return null;
    const want = joaat('ptxEmitterRule:m_particleLifeKFP');
    for (let i = 0; i < 10; i++) {
      const kfp = emitterRule + 0x78 + i * 0x90;
      if (y.u32(kfp + 0x68) === want) return { kfp, keyframes: y.keyframes(kfp) };
    }
    return null;
  }

  function readTexture(y, name) {
    const p = y.textures().get(name);
    if (p === undefined) return null;
    const t = {
      off: p, name,
      width: y.u16(p + 0x50), height: y.u16(p + 0x52), stride: y.u16(p + 0x56),
      format: y.u32(p + 0x58), levels: y.u8[p + 0x5D], data: y.ptr(p + 0x70),
    };
    t.formatName = FMT_NAME[t.format] || `0x${t.format.toString(16)}`;
    let len = 0, l = t.stride * t.height;
    for (let i = 0; i < t.levels; i++) { len += l; l = Math.floor(l / 4); }
    t.dataLength = len;
    if (t.data < y.res.sysSize || t.data + len > y.u8.length) throw new Error(`Texture ${name} data is out of range`);
    return t;
  }

  // Everything the maker shows and changes for the standard gun tracer.
  function readTracer(y) {
    const eff = y.effectRules().get('bullet_tracer');
    if (eff === undefined) throw new Error('No bullet_tracer effect in this core.ypt');
    const emitters = y.pointers(eff + 0x38).filter((p) => p >= 0).map((p) => readEmitter(y, p));
    const beam = emitters.find((e) => e.particleRule === 'bullet_tracer');
    const smoke = emitters.find((e) => e.particleRule === 'bullet_tracer_smoke');
    if (!beam) throw new Error('bullet_tracer has no tracer emitter');
    const pr = y.particleRules();
    const beamRule = pr.get('bullet_tracer');
    const smokeRule = pr.get('bullet_tracer_smoke');
    const out = {
      effect: { off: eff, duration: [y.f32(eff + 0x78), y.f32(eff + 0x7C)], rate: [y.f32(eff + 0x80), y.f32(eff + 0x84)] },
      beam,
      smoke: smoke || null,
      beamColour: beamRule === undefined ? null : readColour(y, beamRule),
      smokeColour: smokeRule === undefined ? null : readColour(y, smokeRule),
      beamFrames: beamRule === undefined ? null : [y.u32(beamRule + 0x118), y.u32(beamRule + 0x11C)],
      life: readLife(y, y.emitterRules().get('bullet_tracer') ?? -1),
      texture: readTexture(y, 'ptfx_bullet_tracer'),
    };
    return out;
  }

  // ---------------------------------------------------------------- patching

  const clamp = (v, lo, hi) => Math.min(hi, Math.max(lo, v));

  function argb(hex, alpha) {
    const n = parseInt(hex.replace('#', ''), 16);
    return ((((alpha === undefined ? 255 : alpha) & 0xFF) << 24) | n) >>> 0;
  }

  function setRgb(y, keyframes, rgb) {
    for (const k of keyframes) {
      y.setF32(k.off + 16, rgb[0]);
      y.setF32(k.off + 20, rgb[1]);
      y.setF32(k.off + 24, rgb[2]);
    }
  }

  // settings: any subset of
  //   duration [min,max] s, effectRate [min,max], beamRate [min,max], zoom [min,max], tint '#rrggbb',
  //   farSize number, glow number, life [min,max] s, beamRgb [r,g,b] 0-1, smokeRgb [r,g,b] 0-1,
  //   texture { beam: {colour, width, style}, smoke: {mode, colour, width} }
  function applyTracer(y, s) {
    const t = readTracer(y);
    const log = [];
    if (s.duration) {
      y.setF32(t.effect.off + 0x78, clamp(s.duration[0], 0.01, 30));
      y.setF32(t.effect.off + 0x7C, clamp(s.duration[1], 0.01, 30));
      log.push('effect duration');
    }
    if (s.effectRate) {
      y.setF32(t.effect.off + 0x80, clamp(s.effectRate[0], 0.01, 10));
      y.setF32(t.effect.off + 0x84, clamp(s.effectRate[1], 0.01, 10));
      log.push('effect speed');
    }
    if (s.beamRate) {
      y.setF32(t.beam.off + 0x50, clamp(s.beamRate[0], 0.01, 10));
      y.setF32(t.beam.off + 0x54, clamp(s.beamRate[1], 0.01, 10));
      log.push('beam speed');
    }
    if (s.zoom) {
      y.setF32(t.beam.off + 0x58, clamp(s.zoom[0], 0.05, 20));
      y.setF32(t.beam.off + 0x5C, clamp(s.zoom[1], 0.05, 20));
      log.push('beam thickness');
    }
    if (s.tint) {
      y.setU32(t.beam.off + 0x60, argb(s.tint));
      y.setU32(t.beam.off + 0x64, argb(s.tint));
      log.push('beam tint');
    }
    if (s.smokeTint && t.smoke) {
      y.setU32(t.smoke.off + 0x60, argb(s.smokeTint));
      y.setU32(t.smoke.off + 0x64, argb(s.smokeTint));
      log.push('smoke tint');
    }
    if (s.farSize !== undefined && t.beam.lodSize) {
      const v = clamp(s.farSize, 1, 5000);
      for (const o of t.beam.lodSize.offs) { y.setF32(o + 16, v); y.setF32(o + 20, v); y.setF32(o + 24, v); }
      log.push('far-away size');
    }
    if (s.glow !== undefined && t.beamColour) {
      const g = clamp(s.glow, 0, 200);
      for (const k of t.beamColour.emissive) { y.setF32(k.off + 16, g); y.setF32(k.off + 20, g * 16 / 15); }
      log.push('glow');
    }
    if (s.life && t.life) {
      for (const k of t.life.keyframes) {
        y.setF32(k.off + 16, clamp(s.life[0], 0.005, 10));
        y.setF32(k.off + 20, clamp(s.life[1], 0.005, 10));
      }
      log.push('beam life');
    }
    if (s.beamRgb && t.beamColour) {
      setRgb(y, t.beamColour.rgbaMin, s.beamRgb);
      setRgb(y, t.beamColour.rgbaMax, s.beamRgb);
      log.push('beam colour multiplier');
    }
    if (s.smokeRgb && t.smokeColour) {
      setRgb(y, t.smokeColour.rgbaMin, s.smokeRgb);
      setRgb(y, t.smokeColour.rgbaMax, s.smokeRgb);
      log.push('smoke colour multiplier');
    }
    if (s.texture) {
      if (!t.texture) throw new Error('This core.ypt has no ptfx_bullet_tracer texture to redraw');
      writeTracerTexture(y, t.texture, s.texture);
      log.push('tracer texture');
    }
    return log;
  }

  // ---------------------------------------------------------------- tracer texture drawing

  function hexRgb(hex) {
    const n = parseInt(String(hex || '#ffffff').replace('#', ''), 16);
    return [(n >> 16) & 255, (n >> 8) & 255, n & 255];
  }

  // The tracer sheet is 4 frames side by side: frames 0-2 animate the smoke trail, frame 3 is the beam.
  function drawTracerSheet(w, h, opts) {
    const px = new Uint8Array(w * h * 4);
    const frameW = w / 4;
    const scale = w / 128;
    const put = (x, y, rgb) => {
      if (x < 0 || x >= w) return;
      const i = (y * w + x) * 4;
      px[i] = rgb[0]; px[i + 1] = rgb[1]; px[i + 2] = rgb[2]; px[i + 3] = 255;
    };
    const beam = opts.beam || {};
    const style = beam.style || 'solid';
    if (style !== 'none') {
      const colour = hexRgb(beam.colour);
      const core = hexRgb(beam.core || '#ffffff');
      const width = Math.max(1, Math.round((beam.width || 3) * scale));
      const centre = Math.floor(frameW * 3 + frameW / 2 - 1);
      const x0 = centre - Math.floor((width - 1) / 2);
      const coreW = Math.max(1, Math.round(width / 3));
      const coreX0 = x0 + Math.floor((width - coreW) / 2);
      const lineW = Math.max(1, Math.floor(width / 3));
      for (let y = 0; y < h; y++) {
        for (let x = x0; x < x0 + width; x++) {
          if (style === 'solid') put(x, y, colour);
          else if (style === 'neon') put(x, y, x >= coreX0 && x < coreX0 + coreW ? core : colour);
          else if (style === 'twin') { if (x < x0 + lineW || x >= x0 + width - lineW) put(x, y, colour); }
        }
      }
    }
    const smoke = opts.smoke || {};
    if (smoke.mode === 'line' || smoke.mode === 'wavy') {
      const colour = hexRgb(smoke.colour);
      const width = Math.max(1, Math.round((smoke.width || 3) * scale));
      for (let f = 0; f < 3; f++) {
        const centre = Math.floor(frameW * f + frameW / 2 - 1);
        for (let y = 0; y < h; y++) {
          const wave = smoke.mode === 'wavy' ? Math.round(Math.sin((y / h) * Math.PI * 6 + f * 2.1) * 3 * scale) : 0;
          const x0 = centre - Math.floor((width - 1) / 2) + wave;
          for (let x = x0; x < x0 + width; x++) put(x, y, colour);
        }
      }
    }
    return px;
  }

  // Halve an RGBA image; colour is averaged over visible pixels so edges keep their colour.
  function halve(src, w, h) {
    const nw = Math.max(1, w >> 1), nh = Math.max(1, h >> 1);
    const out = new Uint8Array(nw * nh * 4);
    for (let y = 0; y < nh; y++) {
      for (let x = 0; x < nw; x++) {
        let r = 0, g = 0, b = 0, a = 0, n = 0;
        for (let dy = 0; dy < 2; dy++) {
          for (let dx = 0; dx < 2; dx++) {
            const sx = Math.min(w - 1, x * 2 + dx), sy = Math.min(h - 1, y * 2 + dy);
            const i = (sy * w + sx) * 4;
            const pa = src[i + 3];
            r += src[i] * pa; g += src[i + 1] * pa; b += src[i + 2] * pa; a += pa; n++;
          }
        }
        const o = (y * nw + x) * 4;
        if (a > 0) { out[o] = Math.round(r / a); out[o + 1] = Math.round(g / a); out[o + 2] = Math.round(b / a); }
        out[o + 3] = Math.round(a / n);
      }
    }
    return { px: out, w: nw, h: nh };
  }

  const to565 = (c) => ((Math.round(c[0] * 31 / 255) << 11) | (Math.round(c[1] * 63 / 255) << 5) | Math.round(c[2] * 31 / 255));
  const from565 = (v) => [((v >> 11) & 31) * 255 / 31, ((v >> 5) & 63) * 255 / 63, (v & 31) * 255 / 31];
  const dist2 = (a, b) => (a[0] - b[0]) ** 2 + (a[1] - b[1]) ** 2 + (a[2] - b[2]) ** 2;

  function blockPixels(img, bx, by) {
    const out = [];
    for (let y = 0; y < 4; y++) {
      for (let x = 0; x < 4; x++) {
        const sx = bx * 4 + x, sy = by * 4 + y;
        if (sx >= img.w || sy >= img.h) { out.push(null); continue; }
        const i = (sy * img.w + sx) * 4;
        out.push([img.px[i], img.px[i + 1], img.px[i + 2], img.px[i + 3]]);
      }
    }
    return out;
  }

  function endpoints(colours) {
    let best = [colours[0], colours[0]], bestD = -1;
    for (let i = 0; i < colours.length; i++) {
      for (let j = i; j < colours.length; j++) {
        const d = dist2(colours[i], colours[j]);
        if (d > bestD) { bestD = d; best = [colours[i], colours[j]]; }
      }
    }
    return best;
  }

  function nearest(palette, c) {
    let bi = 0, bd = Infinity;
    palette.forEach((p, i) => { if (p) { const d = dist2(p, c); if (d < bd) { bd = d; bi = i; } } });
    return bi;
  }

  // BC1 with 1-bit alpha. Any covered pixel is drawn, so thin lines survive in the small mip levels.
  function encodeDXT1(pixels, out, o) {
    const visible = pixels.map((p) => p && p[3] > 0);
    const colours = pixels.filter((p, i) => visible[i]).map((p) => p.slice(0, 3));
    const d = dv(out);
    if (!colours.length) {
      d.setUint16(o, 0, true); d.setUint16(o + 2, 0, true); d.setUint32(o + 4, 0xFFFFFFFF, true);
      return;
    }
    const [ea, eb] = endpoints(colours);
    let c0 = to565(ea), c1 = to565(eb);
    const anyHidden = pixels.some((p, i) => p && !visible[i]);
    let palette;
    if (anyHidden || c0 === c1) {
      if (c0 > c1) [c0, c1] = [c1, c0];
      const a = from565(c0), b = from565(c1);
      palette = [a, b, a.map((v, k) => (v + b[k]) / 2), null];
    } else {
      if (c0 < c1) [c0, c1] = [c1, c0];
      const a = from565(c0), b = from565(c1);
      palette = [a, b, a.map((v, k) => (2 * v + b[k]) / 3), a.map((v, k) => (v + 2 * b[k]) / 3)];
    }
    let bits = 0;
    pixels.forEach((p, i) => {
      const idx = !p ? 0 : visible[i] ? nearest(palette, p) : 3;
      bits = (bits | (idx << (i * 2))) >>> 0;
    });
    d.setUint16(o, c0, true); d.setUint16(o + 2, c1, true); d.setUint32(o + 4, bits, true);
  }

  // BC3: interpolated alpha block + four-colour block.
  function encodeDXT5(pixels, out, o) {
    const live = pixels.filter(Boolean);
    let a0 = 0, a1 = 255;
    for (const p of live) { a0 = Math.max(a0, p[3]); a1 = Math.min(a1, p[3]); }
    if (!live.length) { a0 = 0; a1 = 0; }
    out[o] = a0; out[o + 1] = a1;
    const alphas = [a0, a1];
    for (let k = 1; k <= 6; k++) alphas.push(((7 - k) * a0 + k * a1) / 7);
    // 48 index bits, little endian: pixels 0-7 in the first 3 bytes, 8-15 in the next 3
    let first = 0, second = 0;
    pixels.forEach((p, i) => {
      let idx = 0;
      if (p && a0 !== a1) {
        let bd = Infinity;
        alphas.forEach((a, k) => { const dd = Math.abs(a - p[3]); if (dd < bd) { bd = dd; idx = k; } });
      }
      if (i < 8) first |= idx << (i * 3); else second |= idx << ((i - 8) * 3);
    });
    out[o + 2] = first & 255; out[o + 3] = (first >>> 8) & 255; out[o + 4] = (first >>> 16) & 255;
    out[o + 5] = second & 255; out[o + 6] = (second >>> 8) & 255; out[o + 7] = (second >>> 16) & 255;
    const d = dv(out);
    const colours = pixels.filter((p) => p && p[3] > 0).map((p) => p.slice(0, 3));
    if (!colours.length) {
      d.setUint16(o + 8, 0, true); d.setUint16(o + 10, 0, true); d.setUint32(o + 12, 0, true);
      return;
    }
    const [ea, eb] = endpoints(colours);
    let c0 = to565(ea), c1 = to565(eb);
    if (c0 < c1) [c0, c1] = [c1, c0];
    const a = from565(c0), b = from565(c1);
    const palette = c0 === c1 ? [a, null, null, null]
      : [a, b, a.map((v, k) => (2 * v + b[k]) / 3), a.map((v, k) => (v + 2 * b[k]) / 3)];
    let bits = 0;
    pixels.forEach((p, i) => {
      const idx = p && p[3] > 0 ? nearest(palette, p) : 0;
      bits = (bits | (idx << (i * 2))) >>> 0;
    });
    d.setUint16(o + 8, c0, true); d.setUint16(o + 10, c1, true); d.setUint32(o + 12, bits, true);
  }

  // Encodes a full mip chain laid out the way the resource stores it (each level stride*height/4^n bytes).
  function encodeTexture(tex, format, opts) {
    const out = new Uint8Array(tex.dataLength);
    let img = { px: drawTracerSheet(tex.width, tex.height, opts), w: tex.width, h: tex.height };
    let pos = 0, len = tex.stride * tex.height;
    for (let level = 0; level < tex.levels; level++) {
      if (format === FMT.A8R8G8B8) {
        const n = Math.min(len, img.w * img.h * 4, out.length - pos);
        for (let i = 0; i + 3 < n; i += 4) {
          out[pos + i] = img.px[i + 2]; out[pos + i + 1] = img.px[i + 1]; out[pos + i + 2] = img.px[i]; out[pos + i + 3] = img.px[i + 3];
        }
      } else {
        const bsize = format === FMT.DXT1 ? 8 : 16;
        const bw = Math.max(1, Math.ceil(img.w / 4)), bh = Math.max(1, Math.ceil(img.h / 4));
        const tmp = new Uint8Array(bw * bh * bsize);
        for (let by = 0; by < bh; by++) {
          for (let bx = 0; bx < bw; bx++) {
            const pixels = blockPixels(img, bx, by);
            const o = (by * bw + bx) * bsize;
            if (format === FMT.DXT1) encodeDXT1(pixels, tmp, o); else encodeDXT5(pixels, tmp, o);
          }
        }
        out.set(tmp.subarray(0, Math.max(0, Math.min(tmp.length, out.length - pos))), pos);
      }
      pos += len;
      len = Math.floor(len / 4);
      if (pos >= out.length) break;
      img = halve(img.px, img.w, img.h);
    }
    return out;
  }

  // Formats we can write in place. BC7 and DXT3 sheets are rewritten as DXT5, which has the same size.
  function textureTarget(tex) {
    if (tex.format === FMT.DXT1) return FMT.DXT1;
    if (tex.format === FMT.DXT5 || tex.format === FMT.BC7 || tex.format === FMT.DXT3) return FMT.DXT5;
    if (tex.format === FMT.A8R8G8B8) return FMT.A8R8G8B8;
    return 0;
  }

  function writeTracerTexture(y, tex, opts) {
    const target = textureTarget(tex);
    if (!target) throw new Error(`The tracer texture uses ${tex.formatName}, which the maker can't redraw`);
    const data = encodeTexture(tex, target, opts);
    y.u8.set(data, tex.data);
    if (target !== tex.format) y.setU32(tex.off + 0x58, target);
  }

  // Decodes a DXT1/DXT5/A8R8G8B8 texture's top level to RGBA for previews. Returns null for other formats.
  function decodeTexture(y, tex) {
    const { width: w, height: h } = tex;
    const src = y.u8.subarray(tex.data, tex.data + tex.dataLength);
    const px = new Uint8Array(w * h * 4);
    const fmt = tex.format;
    if (fmt === FMT.A8R8G8B8) {
      for (let i = 0; i < w * h; i++) {
        px[i * 4] = src[i * 4 + 2]; px[i * 4 + 1] = src[i * 4 + 1]; px[i * 4 + 2] = src[i * 4]; px[i * 4 + 3] = src[i * 4 + 3];
      }
      return px;
    }
    if (fmt !== FMT.DXT1 && fmt !== FMT.DXT5) return null;
    const bsize = fmt === FMT.DXT1 ? 8 : 16;
    const bw = Math.ceil(w / 4);
    const d = dv(src);
    for (let by = 0; by < Math.ceil(h / 4); by++) {
      for (let bx = 0; bx < bw; bx++) {
        const o = (by * bw + bx) * bsize;
        const co = fmt === FMT.DXT1 ? o : o + 8;
        const c0 = d.getUint16(co, true), c1 = d.getUint16(co + 2, true), bits = d.getUint32(co + 4, true);
        const a = from565(c0), b = from565(c1);
        const four = fmt === FMT.DXT5 || c0 > c1;
        const pal = four
          ? [a, b, a.map((v, k) => (2 * v + b[k]) / 3), a.map((v, k) => (v + 2 * b[k]) / 3)]
          : [a, b, a.map((v, k) => (v + b[k]) / 2), [0, 0, 0]];
        let alphas = null, abits = 0n;
        if (fmt === FMT.DXT5) {
          const a0 = src[o], a1 = src[o + 1];
          alphas = [a0, a1];
          if (a0 > a1) for (let k = 1; k <= 6; k++) alphas.push(((7 - k) * a0 + k * a1) / 7);
          else { for (let k = 1; k <= 4; k++) alphas.push(((5 - k) * a0 + k * a1) / 5); alphas.push(0, 255); }
          for (let k = 5; k >= 0; k--) abits = (abits << 8n) | BigInt(src[o + 2 + k]);
        }
        for (let i = 0; i < 16; i++) {
          const x = bx * 4 + (i & 3), yy = by * 4 + (i >> 2);
          if (x >= w || yy >= h) continue;
          const idx = (bits >>> (i * 2)) & 3;
          const p = (yy * w + x) * 4;
          const c = pal[idx];
          px[p] = c[0]; px[p + 1] = c[1]; px[p + 2] = c[2];
          if (fmt === FMT.DXT5) px[p + 3] = Math.round(alphas[Number((abits >> BigInt(i * 3)) & 7n)]);
          else px[p + 3] = !four && idx === 3 ? 0 : 255;
        }
      }
    }
    return px;
  }

  // ---------------------------------------------------------------- packages

  // Finds core.ypt inside a pack (any depth). Accepts a standalone .ypt too.
  function openPack(bytes, fileName) {
    const d = dv(bytes);
    if (bytes.length >= 4 && d.getUint32(0, true) === RSC7) {
      return { kind: 'ypt', fileName, res: openResource(bytes) };
    }
    const archive = readRpf(bytes, fileName);
    const files = listFiles(archive);
    const hits = files.filter((f) => f.node.kind === 'rsc' && /(^|\/)core\.ypt$/i.test(f.path));
    if (!hits.length) {
      const ypts = files.filter((f) => /\.ypt$/i.test(f.path)).map((f) => f.path);
      throw new Error(ypts.length ? `No core.ypt in this pack (found ${ypts.join(', ')}). Tracers live in core.ypt.` : 'No core.ypt in this pack. Tracers live in core.ypt.');
    }
    const first = hits[0].node;
    if (hits.some((h) => !sameBytes(h.node.stored, first.stored))) {
      throw new Error(`This pack has ${hits.length} different core.ypt files (${hits.map((h) => h.path).join(', ')}), so the maker can't tell which one the game uses.`);
    }
    return { kind: 'rpf', fileName, archive, paths: hits.map((h) => h.path), nodes: hits.map((h) => h.node),
             res: openResource(first.stored, first.sysf, first.gfxf), files };
  }

  function sameBytes(a, b) {
    if (a.length !== b.length) return false;
    for (let i = 0; i < a.length; i++) if (a[i] !== b[i]) return false;
    return true;
  }

  function savePack(pack, level, body) {
    const stored = packResource(pack.res, level, body);
    if (pack.kind === 'ypt') return stored;
    for (const n of pack.nodes) n.stored = stored;
    return writeRpf(pack.archive);
  }

  // ---------------------------------------------------------------- bloodfx.dat

  const BLOOD_COLUMNS = ['PUNCH_KICK', 'MELEE_WOOD', 'MELEE_METAL', 'MELEE_SHARP', 'MELEE_GENERIC', 'PISTOL_SMALL', 'PISTOL_LARGE',
    'PISTOL_SILENCED', 'RUBBER', 'SMG', 'SHOTGUN', 'RIFLE_ASSAULT', 'RIFLE_SNIPER', 'ROCKET', 'GRENADE', 'MOLOTOV',
    'FIRE', 'EXPLOSION', 'LASER', 'STUNGUN', 'HEAVY_MG', 'VEHICLE_MG', 'CLOWN', 'ALIEN'];
  const WEAPONS = {
    guns: ['PISTOL_SMALL', 'PISTOL_LARGE', 'PISTOL_SILENCED', 'SMG', 'RIFLE_ASSAULT', 'HEAVY_MG', 'VEHICLE_MG'],
    shotguns: ['SHOTGUN'],
    snipers: ['RIFLE_SNIPER'],
    melee: ['PUNCH_KICK', 'MELEE_WOOD', 'MELEE_METAL', 'MELEE_SHARP', 'MELEE_GENERIC'],
    explosions: ['ROCKET', 'GRENADE', 'EXPLOSION'],
  };
  const ZONE_OF = (name) => (name === 'HEAD' || name === 'NECK' ? 'head' : name === 'ANIMAL_DEFAULT' ? 'animals' : 'body');
  // Info rows: id, 33 numbers, 4 effect names (normal, body armour, heavy armour, underwater),
  // size evo, damage chance, 3 ped damage names.
  const T_TINT = 8, T_SIZES = 20, T_FX = 34, T_EVO = 38, INFO_TOKENS = 43;

  function fmtNum(v) {
    const s = String(Number(v.toFixed(3)));
    return s.includes('.') ? s : s + '.0';
  }

  class BloodDat {
    constructor(text) {
      if (!/BLOODFX_TABLE_\w+_START/.test(text)) throw new Error('This is not a bloodfx.dat file');
      this.items = [];
      this.tables = {};
      this.entry = new Map();
      this.exit = new Map();
      this.problems = [];
      let section = null;
      for (const raw of text.split('\n')) {
        const item = { raw };
        this.items.push(item);
        const eol = raw.endsWith('\r') ? '\r' : '';
        const body = eol ? raw.slice(0, -1) : raw;
        const hash = body.indexOf('#');
        const code = hash >= 0 ? body.slice(0, hash) : body;
        const trimmed = code.trim();
        if (!trimmed) continue;
        const m = /^BLOODFX_(TABLE_\w+|ENTRY_INFO|EXIT_INFO)_(START|END)$/.exec(trimmed);
        if (m) { section = m[2] === 'START' ? m[1] : null; continue; }
        if (!section) continue;
        const parts = code.split(/(\s+)/);
        const tok = [];
        parts.forEach((p, i) => { if (i % 2 === 0 && p !== '') tok.push(i); });
        const row = { parts, tok, comment: hash >= 0 ? body.slice(hash) : '', eol, section };
        if (section.startsWith('TABLE')) {
          row.name = parts[tok[0]];
          if (tok.length !== BLOOD_COLUMNS.length + 1) { this.problems.push(`${section} ${row.name}: ${tok.length - 1} columns`); continue; }
          (this.tables[section] = this.tables[section] || []).push(row);
        } else {
          row.id = parseInt(parts[tok[0]], 10);
          const numeric = tok.slice(1, T_FX).every((i) => /^-?\d+(\.\d+)?$/.test(parts[i]));
          if (tok.length < INFO_TOKENS || !numeric || Number.isNaN(row.id)) { this.problems.push(`${section} row ${parts[tok[0]]} has an unexpected layout`); continue; }
          (section === 'ENTRY_INFO' ? this.entry : this.exit).set(row.id, row);
        }
        item.row = row;
      }
      if (!Object.keys(this.tables).length || !this.entry.size) throw new Error('No blood tables or entry rows found in this bloodfx.dat');
    }

    get(row, k) { return row.parts[row.tok[k]]; }
    set(row, k, v) { row.parts[row.tok[k]] = String(v); row.dirty = true; }

    toString() {
      return this.items.map((it) => (it.row ? it.row.parts.join('') + it.row.comment + it.row.eol : it.raw)).join('\n');
    }

    // Table cells for the given zones and weapon groups, skipping empty (0) cells.
    cells(zones, weapons, opts) {
      opts = opts || {};
      const cols = new Set(weapons.flatMap((w) => WEAPONS[w] || []));
      const out = [];
      for (const [section, rows] of Object.entries(this.tables)) {
        if (opts.dead === false && /DEAD/.test(section)) continue;
        for (const row of rows) {
          if (!zones.includes(ZONE_OF(row.name))) continue;
          BLOOD_COLUMNS.forEach((col, c) => {
            if (!cols.has(col)) return;
            const id = parseInt(this.get(row, c + 1), 10);
            if (id) out.push({ row, k: c + 1, id });
          });
        }
      }
      return out;
    }

    usage(id) {
      const out = [];
      for (const rows of Object.values(this.tables)) {
        for (const row of rows) for (let k = 1; k <= BLOOD_COLUMNS.length; k++) if (parseInt(this.get(row, k), 10) === id) out.push({ row, k });
      }
      return out;
    }

    freeId(near) {
      const used = new Set([...this.entry.keys(), ...this.exit.keys()]);
      for (const rows of Object.values(this.tables)) for (const row of rows) for (let k = 1; k <= BLOOD_COLUMNS.length; k++) used.add(parseInt(this.get(row, k), 10));
      const start = Math.floor(near / 100) * 100;
      for (let id = start + 10; id < start + 100; id++) if (!used.has(id)) return id;
      for (let id = 100; id < 1000; id++) if (!used.has(id)) return id;
      throw new Error('No free ids left in bloodfx.dat');
    }

    // Copies an id's entry and exit rows to a new id (placed right after the originals).
    clone(id) {
      const fresh = this.freeId(id);
      const label = String(fresh).padStart(3, '0');
      for (const map of [this.entry, this.exit]) {
        const src = map.get(id);
        if (!src) continue;
        const row = { ...src, parts: src.parts.slice(), dirty: true, id: fresh };
        row.parts[row.tok[0]] = label;
        const at = this.items.findIndex((it) => it.row === src);
        this.items.splice(at + 1, 0, { row });
        map.set(fresh, row);
      }
      return fresh;
    }

    // Points the chosen zone/weapon cells at info rows playing `fx`. Ids shared with cells outside the
    // selection are cloned first, so nothing else changes.
    route(zones, weapons, fx, opts) {
      opts = opts || {};
      const cells = this.cells(zones, weapons, opts);
      const byId = new Map();
      for (const c of cells) {
        if (!this.entry.has(c.id)) continue;
        if (!byId.has(c.id)) byId.set(c.id, []);
        byId.get(c.id).push(c);
      }
      const clones = [];
      const edited = [];
      for (const [id, picked] of byId) {
        let target = id;
        if (this.usage(id).some((c) => !picked.some((p) => p.row === c.row && p.k === c.k))) {
          target = this.clone(id);
          clones.push([id, target]);
          const label = String(target).padStart(3, '0');
          for (const c of picked) this.set(c.row, c.k, label);
        }
        const row = this.entry.get(target);
        if (fx) {
          this.set(row, T_FX, fx);
          if (opts.armour) { this.set(row, T_FX + 1, fx); this.set(row, T_FX + 2, fx); }
          if (opts.underwater) this.set(row, T_FX + 3, fx);
        }
        if (opts.size !== undefined && opts.size !== null) this.set(row, T_EVO, fmtNum(clamp(opts.size, 0, 1)));
        edited.push(target);
      }
      return { cells: cells.length, edited, clones };
    }

    // Blood decal colour on every info row (game default is 50 0 0).
    tint(rgb) {
      for (const row of [...this.entry.values(), ...this.exit.values()]) {
        for (let i = 0; i < 3; i++) this.set(row, T_TINT + i, Math.round(clamp(rgb[i], 0, 255)));
      }
    }

    // Scales the splat/spray decal sizes on every info row.
    splatter(factor) {
      for (const row of [...this.entry.values(), ...this.exit.values()]) {
        for (let k = T_SIZES; k < T_SIZES + 8; k++) {
          const v = parseFloat(this.get(row, k));
          if (v > 0) this.set(row, k, fmtNum(v * factor));
        }
      }
    }

    // What plays where: zone -> weapon group -> effect names (normal column), from the alive tables.
    summary() {
      const out = {};
      for (const zone of ['head', 'body', 'animals']) {
        out[zone] = {};
        for (const w of Object.keys(WEAPONS)) {
          const names = new Set();
          for (const c of this.cells([zone], [w], { dead: false })) {
            const row = this.entry.get(c.id);
            names.add(row ? this.get(row, T_FX) : `missing ${c.id}`);
          }
          out[zone][w] = [...names];
        }
      }
      return out;
    }

    // Every id a table points at must have an entry row; ids are unique.
    check() {
      const issues = [...this.problems];
      for (const [section, rows] of Object.entries(this.tables)) {
        for (const row of rows) {
          for (let k = 1; k <= BLOOD_COLUMNS.length; k++) {
            const id = parseInt(this.get(row, k), 10);
            if (id && !this.entry.has(id)) issues.push(`${section} ${row.name} ${BLOOD_COLUMNS[k - 1]} points at ${id}, which has no entry row`);
          }
        }
      }
      return issues;
    }

    effects() {
      const names = new Set();
      for (const row of [...this.entry.values(), ...this.exit.values()]) {
        for (let k = T_FX; k < T_FX + 4; k++) { const n = this.get(row, k); if (n !== '-') names.add(n); }
      }
      return [...names].sort();
    }
  }

  return {
    setZlib, joaat, readRpf, writeRpf, listFiles, readBinary, pageSize, openResource, packResource,
    Ypt, readTracer, applyTracer, drawTracerSheet, encodeTexture, decodeTexture, textureTarget, openPack, savePack,
    FMT, FMT_NAME, argb, BloodDat, WEAPONS, BLOOD_COLUMNS, INFO: { T_TINT, T_SIZES, T_FX, T_EVO },
  };
});
