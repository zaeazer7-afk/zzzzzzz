// Tells whether an .rpf on disk holds a core.ypt (so it changes the tracer), reading only the folder tables,
// nested packs included. Returns true, false, or null when the pack can't be read (encrypted or damaged).
const fs = require('fs');

const RPF7 = 0x52504637;
const ENC_OPEN = 0x4E45504F;
const DIR_MARK = 0x7FFFFF00;
const BLOCK = 512;

function readAt(fd, pos, len) {
  const buf = Buffer.alloc(len);
  const n = fs.readSync(fd, buf, 0, len, pos);
  return n === len ? buf : buf.subarray(0, n);
}

function scan(fd, base, depth, hits) {
  const head = readAt(fd, base, 16);
  if (head.length < 16 || head.readUInt32LE(0) !== RPF7) throw new Error('not an rpf');
  const count = head.readUInt32LE(4), namesLen = head.readUInt32LE(8), enc = head.readUInt32LE(12);
  if (enc !== ENC_OPEN && enc !== 0) throw Object.assign(new Error('encrypted'), { encrypted: true });
  if (count > 200000 || namesLen > 1 << 24) throw new Error('damaged header');
  const toc = readAt(fd, base + 16, count * 16 + namesLen);
  if (toc.length < count * 16 + namesLen) throw new Error('cut short');
  const names = toc.subarray(count * 16);
  const nameAt = (off) => { let e = off; while (e < names.length && names[e]) e++; return names.subarray(off, e).toString('latin1'); };
  for (let i = 0; i < count; i++) {
    const o = i * 16;
    const second = toc.readUInt32LE(o + 4);
    if (second === DIR_MARK) continue;
    if ((second & 0x80000000) === 0) {
      const lo = toc.readUInt32LE(o);
      const csize = (lo >>> 16) | ((second & 0xFF) << 16);
      if (depth < 4 && !csize && /\.rpf$/i.test(nameAt(lo & 0xFFFF))) {
        try { scan(fd, base + (second >>> 8) * BLOCK, depth + 1, hits); } catch (e) { if (e.encrypted) throw e; }
      }
    } else if (/^core\.ypt$/i.test(nameAt(toc.readUInt16LE(o)))) {
      hits.push(true);
    }
  }
}

function hasCoreYpt(file) {
  let fd;
  try {
    fd = fs.openSync(file, 'r');
    const hits = [];
    scan(fd, 0, 0, hits);
    return hits.length > 0;
  } catch {
    return null;
  } finally {
    if (fd !== undefined) fs.closeSync(fd);
  }
}

module.exports = { hasCoreYpt };
