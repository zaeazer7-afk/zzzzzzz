// Builds ../index.html: inlines the core module, the built-in bloodfx.dat and GTA's core.ypt effect names into page.html.
// Usage: node src/build.js
const fs = require('fs');
const path = require('path');
const zlib = require('zlib');

const src = __dirname;
const read = (name) => fs.readFileSync(path.join(src, name));

const base = read('base_bloodfx.dat');
const baseB64 = zlib.deflateRawSync(base, { level: 9 }).toString('base64');
const effects = JSON.parse(read('core_effects.json').toString('utf8'));
const coreSrc = read('bfxcore.js').toString('utf8');
if (/<\/script/i.test(coreSrc)) throw new Error('bfxcore.js must not contain a closing script tag');

let page = read('page.html').toString('utf8');
const swap = (marker, value) => {
  if (!page.includes(marker)) throw new Error('page.html is missing ' + marker);
  page = page.split(marker).join(value);
};
swap('/*@CORE@*/', coreSrc);
swap('"@BASE_DAT@"', JSON.stringify(baseB64));
swap('"@EFFECTS@"', JSON.stringify(effects.join(' ')));

const out = path.join(src, '..', 'index.html');
fs.writeFileSync(out, page);
console.log(`wrote ${path.relative(process.cwd(), out)} (${(page.length / 1024).toFixed(1)} KB)`);
