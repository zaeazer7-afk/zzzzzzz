// Tracer patch checks. Runs against every [bloodfx-packs]/*/stream/core.ypt, then round-trips one through a
// nested RPF laid out like the FiveM mods-folder packs (assembly.xml + content/<name>.rpf + x64/<name>.rpf).
// Usage: node test/test_tracer.js [packs folder]
const fs = require('fs');
const path = require('path');
const zlib = require('zlib');
const core = require('../src/core/bfxcore.js');

core.setZlib({
  inflateRaw: (u8) => new Uint8Array(zlib.inflateRawSync(u8)),
  deflateRaw: (u8, opts) => new Uint8Array(zlib.deflateRawSync(u8, opts)),
});

const packsDir = process.argv[2] || path.join(__dirname, '..', '..', '[bloodfx-packs]');
const only = process.env.PACKS ? process.env.PACKS.split(',') : null;
const ypts = fs.readdirSync(packsDir).filter((d) => !only || only.includes(d))
  .map((d) => path.join(packsDir, d, 'stream', 'core.ypt')).filter((f) => fs.existsSync(f));

const SETTINGS = {
  duration: [1.5, 1.25], effectRate: [0.3, 0.35], beamRate: [0.4, 0.45], zoom: [1.7, 1.8], farSize: 450, glow: 22,
  life: [0.08, 0.1], beamRgb: [1, 1, 1], smokeRgb: [1, 1, 1], tint: '#ffffff', smokeTint: '#ffffff',
  texture: { beam: { colour: '#ff2bd6', width: 6, style: 'neon' }, smoke: { mode: 'wavy', colour: '#7a5cff', width: 3 } },
};

let failures = 0;
const fail = (m) => { failures++; console.log('  FAIL ' + m); };
const near = (a, b) => Math.abs(a - b) < 1e-4;

function checkPatched(label, res, before) {
  const t = core.readTracer(new core.Ypt(res));
  const want = (name, got, value) => { if (!near(got, value)) fail(`${label} ${name}: ${got} instead of ${value}`); };
  want('duration', t.effect.duration[0], 1.5);
  want('effect speed', t.effect.rate[1], 0.35);
  want('beam speed', t.beam.rate[0], 0.4);
  want('thickness', t.beam.zoom[1], 1.8);
  if (t.beam.lodSize) want('far size', t.beam.lodSize.value[0], 450);
  want('glow', t.beamColour.emissive[0].v[0], 22);
  want('life', t.life.keyframes[0].v[1], 0.1);
  for (const k of t.beamColour.rgbaMin) want('beam colour', k.v[2], 1);
  if (t.beam.tint[0] !== 0xFFFFFFFF) fail(`${label}: beam tint not white`);
  if (t.texture.format !== core.textureTarget(before.texture)) fail(`${label}: texture format ${t.texture.formatName}`);
  const px = core.decodeTexture(new core.Ypt(res), t.texture);
  const drawn = core.drawTracerSheet(t.texture.width, t.texture.height, SETTINGS.texture);
  let bad = 0;
  for (let i = 0; i < drawn.length; i += 4) {
    const a = drawn[i + 3] > 0, b = px[i + 3] > 0;
    if (a !== b || (a && Math.abs(drawn[i] - px[i]) + Math.abs(drawn[i + 1] - px[i + 1]) + Math.abs(drawn[i + 2] - px[i + 2]) > 24)) bad++;
  }
  if (bad) fail(`${label}: ${bad} texture pixels differ from the drawing`);
}

let sample = null;
for (const file of ypts) {
  const label = path.basename(path.dirname(path.dirname(file)));
  const bytes = new Uint8Array(fs.readFileSync(file));
  const pack = core.openPack(bytes, 'core.ypt');
  const y = new core.Ypt(pack.res);
  const before = core.readTracer(y);
  const original = pack.res.data.slice();
  core.applyTracer(y, SETTINGS);

  // every changed byte must belong to a value we meant to change
  const allowed = [];
  const add = (o, n) => allowed.push([o, o + n]);
  add(before.effect.off + 0x78, 16);
  add(before.beam.off + 0x50, 24);
  if (before.smoke) add(before.smoke.off + 0x60, 8);
  for (const o of before.beam.lodSize ? before.beam.lodSize.offs : []) add(o + 16, 12);
  for (const c of [before.beamColour, before.smokeColour]) if (c) for (const k of [...c.rgbaMin, ...c.rgbaMax]) add(k.off + 16, 12);
  for (const k of before.beamColour.emissive) add(k.off + 16, 8);
  for (const k of before.life.keyframes) add(k.off + 16, 8);
  add(before.texture.data, before.texture.dataLength);
  add(before.texture.off + 0x58, 4);
  let stray = 0;
  for (let i = 0; i < original.length; i++) {
    if (original[i] !== pack.res.data[i] && !allowed.some(([a, b]) => i >= a && i < b)) stray++;
  }
  if (stray) fail(`${label}: ${stray} bytes changed outside the tracer values`);

  const saved = core.savePack(pack, 6);
  checkPatched(label, core.openPack(saved, 'core.ypt').res, before);
  console.log(`${label}: ${before.texture.formatName} sheet, glow ${before.beamColour.emissive[0].v[0]}, stays ${before.effect.duration[1].toFixed(2)} s -> patched OK`);
  if (!sample) sample = { label, bytes };

  const smokeOnly = { beam: { style: 'none' }, smoke: { mode: 'line', colour: '#ffc21f', width: 4 } };
  const again = core.openPack(bytes, 'core.ypt');
  const y2 = new core.Ypt(again.res);
  core.applyTracer(y2, { texture: smokeOnly });
  const t2 = core.readTracer(y2);
  const px2 = core.decodeTexture(y2, t2.texture);
  const want2 = core.drawTracerSheet(t2.texture.width, t2.texture.height, smokeOnly);
  const dxt1 = t2.texture.format === core.FMT.DXT1;
  let beamPixels = 0, alphaErr = 0, covered = 0, wanted = 0;
  for (let i = 0; i < want2.length; i += 4) {
    const x = (i / 4) % t2.texture.width;
    if (x >= t2.texture.width * 3 / 4 && px2[i + 3] > 0) beamPixels++;
    // DXT1 dithers soft alpha, so about alpha/255 of those pixels end up drawn
    if (want2[i + 3] > 0) wanted += dxt1 ? Math.min(255, want2[i + 3] * core.DXT1_BOOST) / 255 : 1;
    if (px2[i + 3] > 0) covered++;
    if (!dxt1) alphaErr = Math.max(alphaErr, Math.abs(want2[i + 3] - px2[i + 3]));
  }
  if (beamPixels) fail(`${label}: beam frame should be empty for smoke only (${beamPixels} pixels)`);
  if (alphaErr > 40) fail(`${label}: soft smoke alpha off by ${alphaErr}`);
  if (Math.abs(covered - wanted) > wanted * 0.35) fail(`${label}: smoke coverage ${covered} vs ${Math.round(wanted)}`);
}

// Nested RPF round trip, shaped like the FiveM mods-folder packs
if (sample) {
  const d = new DataView(sample.bytes.buffer, sample.bytes.byteOffset);
  const text = (s) => new TextEncoder().encode(s);
  const bin = (name, data, compress) => {
    const stored = compress ? new Uint8Array(zlib.deflateRawSync(data)) : data;
    return { kind: 'bin', name, stored, csize: compress ? stored.length : 0, usize: data.length, encrypted: 0 };
  };
  const dir = (name, children) => ({ kind: 'dir', name, children });
  const inner = { encryption: 0x4E45504F, root: dir('', [{ kind: 'rsc', name: 'core.ypt', stored: sample.bytes, sysf: d.getUint32(8, true), gfxf: d.getUint32(12, true) }]) };
  const dlc = { encryption: 0x4E45504F, root: dir('', [bin('content.xml', text('<CDataFileMgr__ContentsOfDataFileXml/>'), true), bin('setup2.xml', text('<SSetupData/>'), true),
    dir('x64', [{ ...bin('test_pack.rpf', core.writeRpf(inner), false), rpf: inner }])]) };
  const outer = { encryption: 0x4E45504F, root: dir('', [bin('assembly.xml', text('<package/>'), true), dir('content', [{ ...bin('test_pack.rpf', core.writeRpf(dlc), false), rpf: dlc }])]) };
  const rpfBytes = core.writeRpf(outer);
  const pack = core.openPack(rpfBytes, 'test.rpf');
  if (pack.paths.join() !== 'content/test_pack.rpf/x64/test_pack.rpf/core.ypt') fail('core.ypt path in nested rpf: ' + pack.paths);
  const names = core.listFiles(core.readRpf(rpfBytes)).map((f) => f.path);
  for (const want of ['assembly.xml', 'content/test_pack.rpf/content.xml', 'content/test_pack.rpf/setup2.xml']) if (!names.includes(want)) fail('missing ' + want);
  const xml = core.listFiles(core.readRpf(rpfBytes)).find((f) => f.path === 'content/test_pack.rpf/setup2.xml');
  if (new TextDecoder().decode(core.readBinary(xml.node)) !== '<SSetupData/>') fail('compressed file did not survive');
  const before = core.readTracer(new core.Ypt(pack.res));
  core.applyTracer(new core.Ypt(pack.res), SETTINGS);
  const saved = core.savePack(pack, 6);
  if (saved.length % 512) fail('rpf size is not a multiple of 512');
  checkPatched('nested rpf', core.openPack(saved, 'saved.rpf').res, before);
  console.log(`nested rpf round trip (${(saved.length / 1048576).toFixed(1)} MiB) OK`);
}
console.log(failures ? `${failures} failures` : 'all good');
process.exit(failures ? 1 : 0);
