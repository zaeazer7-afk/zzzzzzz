// bloodfx.dat editor checks. Runs against every bloodfx.dat in [bloodfx-packs] plus the built-in base.
// Usage: node test/test_dat.js [packs folder]
const fs = require('fs');
const path = require('path');
const core = require('../src/core/bfxcore.js');

const packsDir = process.argv[2] || path.join(__dirname, '..', '..', '[bloodfx-packs]');
const files = fs.readdirSync(packsDir).map((d) => path.join(packsDir, d, 'bloodfx.dat')).filter((f) => fs.existsSync(f));
files.push(path.join(__dirname, '..', 'src', 'base_bloodfx.dat'));

let failures = 0;
const fail = (m) => { failures++; console.log('  FAIL ' + m); };

for (const f of files) {
  const text = fs.readFileSync(f, 'latin1');
  const dat = new core.BloodDat(text);
  if (dat.toString() !== text) fail(`${f}: loading and saving changed the file`);
  if (dat.problems.length) fail(`${f}: ${dat.problems.join('; ')}`);
  const issues = dat.check();
  if (issues.length) fail(`${f}: ${issues.slice(0, 3).join('; ')}`);
  console.log(`${path.basename(path.dirname(f))}: ${dat.entry.size} entry rows, ${dat.exit.size} exit rows; head + guns play ${dat.summary().head.guns.join(', ')}`);
}

// Routing edits on the built-in base
const base = fs.readFileSync(path.join(__dirname, '..', 'src', 'base_bloodfx.dat'), 'latin1');
const dat = new core.BloodDat(base);
const before = dat.summary();
const r1 = dat.route(['head'], ['guns'], 'ent_brk_banknotes', { armour: true, size: 0.8 });
const s1 = dat.summary();
if (s1.head.guns.join() !== 'ent_brk_banknotes') fail('head + guns should be cash: ' + s1.head.guns);
if (s1.head.explosions.join() !== before.head.explosions.join()) fail('explosions changed by a guns rule');
if (s1.body.explosions.join() !== before.body.explosions.join()) fail('body explosions changed by a head rule');
if (s1.head.shotguns.join() !== before.head.shotguns.join()) fail('shotguns changed by a guns rule');
const r2 = dat.route(['body'], ['guns', 'shotguns'], 'liquid_splash_oil', {});
const s2 = dat.summary();
if (s2.body.guns.join() !== 'liquid_splash_oil' || s2.body.shotguns.join() !== 'liquid_splash_oil') fail('body should be oil: ' + JSON.stringify(s2.body));
if (s2.head.shotguns.join() !== before.head.shotguns.join()) fail('head shotguns changed by a body rule');
if (s2.head.guns.join() !== 'ent_brk_banknotes') fail('the head rule was lost');
const r3 = dat.route(['head'], ['guns'], 'ent_dst_gen_gobstop', { armour: true });
if (r3.clones.length) fail('editing the same hits again should not clone again: ' + JSON.stringify(r3.clones));
const alive = dat.cells(['head'], ['guns'], { dead: false }).length;
const all = dat.cells(['head'], ['guns']).length;
if (!(alive > 0 && all > alive)) fail('dead tables should be optional');
dat.tint([0, 40, 120]);
dat.splatter(1.5);

const out = dat.toString();
const again = new core.BloodDat(out);
if (again.toString() !== out) fail('edited file does not load and save cleanly');
const issues = again.check();
if (issues.length) fail('edited file: ' + issues.slice(0, 3).join('; '));
const crlf = (s) => (s.match(/\r\n/g) || []).length;
const added = out.split('\n').length - base.split('\n').length;
if (crlf(out) - crlf(base) !== added) fail('new lines must keep CRLF endings');
const ids = [...again.entry.keys()];
if (new Set(ids).size !== ids.length) fail('duplicate entry ids');
for (const [orig, clone] of [...r1.clones, ...r2.clones]) {
  if (again.exit.has(orig) !== again.exit.has(clone)) fail(`copy ${clone} of ${orig} lost its exit row`);
  if (clone > 999) fail('ids must stay at three digits');
}
const row = again.entry.get(105);
if (again.get(row, core.INFO.T_TINT) !== '0' || again.get(row, core.INFO.T_TINT + 2) !== '120') fail('splat tint not written');
console.log(`edits: ${added} rows added (${[...r1.clones, ...r2.clones].map((c) => c.join('->')).join(' ')})`);
console.log(failures ? `${failures} failures` : 'all good');
process.exit(failures ? 1 : 0);
