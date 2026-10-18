// Builds BloodFX Maker from src/.
//   node src/build.js            writes index.html: the single-file web page (fonts from Google Fonts, pako from cdnjs)
//   node src/build.js --desktop  writes desktop/app/: the Windows app's window, with every file local so it works offline
const fs = require('fs');
const path = require('path');
const zlib = require('zlib');

const src = __dirname;
const root = path.join(src, '..');
const read = (name) => fs.readFileSync(path.join(src, name), 'utf8');

const SCRIPTS = [
  'core/bfxcore.js', 'ui/data.js', 'ui/util.js', 'ui/controls.js', 'ui/art.js', 'ui/preview.js', 'ui/platform.js', 'ui/app.js',
  'ui/pages/home.js', 'ui/pages/tracer.js', 'ui/pages/blood.js', 'ui/pages/muzzle.js', 'ui/pages/explorer.js', 'ui/pages/install.js',
  'ui/pages/settings.js', 'ui/boot.js',
];
const FONTS_URL = 'https://fonts.googleapis.com/css2?family=Barlow:wght@400;500;600;700&family=Big+Shoulders+Display:wght@700;800&family=IBM+Plex+Mono:wght@400;500;600&display=swap';
const PAKO_CDN = 'https://cdnjs.cloudflare.com/ajax/libs/pako/2.1.0/pako.min.js';
const FONTS = [
  ['Barlow', 'barlow', [400, 500, 600, 700]],
  ['Big Shoulders Display', 'big-shoulders-display', [700, 800]],
  ['IBM Plex Mono', 'ibm-plex-mono', [400, 500, 600]],
];

// The built-in bloodfx.dat and GTA's standard effect names, inlined into data.js.
function dataSwaps() {
  const base = fs.readFileSync(path.join(src, 'base_bloodfx.dat'));
  return {
    "'@BASE_DAT@'": JSON.stringify(zlib.deflateRawSync(base, { level: 9 }).toString('base64')),
    "'@EFFECTS@'": JSON.stringify(JSON.parse(read('core_effects.json')).join(' ')),
    "'@LONG@'": JSON.stringify(JSON.parse(read('long_effects.json')).join(' ')),
  };
}

function scripts() {
  const swaps = dataSwaps();
  return SCRIPTS.map((name) => {
    let code = read(name);
    if (name === 'ui/data.js') {
      for (const [k, v] of Object.entries(swaps)) {
        if (!code.includes(k)) throw new Error(`${name} is missing ${k}`);
        code = code.split(k).join(v);
      }
    }
    if (/<\/script/i.test(code)) throw new Error(`${name} must not contain a closing script tag`);
    return { name, code };
  });
}

function page(head, scriptTags) {
  let html = read('ui/index.html');
  const swap = (marker, value) => {
    if (!html.includes(marker)) throw new Error('ui/index.html is missing ' + marker);
    html = html.split(marker).join(value);
  };
  swap('<!--@HEAD@-->', head);
  swap('<!--@STYLE@-->', `<style>\n${read('ui/app.css')}</style>`);
  swap('<!--@SCRIPTS@-->', scriptTags);
  return html;
}

function web() {
  const head = [
    '<meta name="claude:viewport-fit-sides" content="cover">',
    '<link rel="preconnect" href="https://fonts.googleapis.com">',
    '<link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>',
    `<link rel="stylesheet" href="${FONTS_URL}">`,
  ].join('\n');
  const tags = [`<script src="${PAKO_CDN}"></script>`, ...scripts().map((s) => `<script>/* ${s.name} */\n${s.code}</script>`)].join('\n');
  const html = page(head, tags);
  const out = path.join(root, 'index.html');
  fs.writeFileSync(out, html);
  console.log(`wrote ${path.relative(process.cwd(), out)} (${(html.length / 1024).toFixed(1)} KB)`);
}

function desktop() {
  const app = path.join(root, 'desktop', 'app');
  const nm = path.join(root, 'desktop', 'node_modules');
  if (!fs.existsSync(path.join(nm, 'pako'))) throw new Error('Run npm install in desktop/ first');
  fs.rmSync(app, { recursive: true, force: true });
  fs.mkdirSync(path.join(app, 'fonts'), { recursive: true });
  fs.copyFileSync(path.join(nm, 'pako', 'dist', 'pako.min.js'), path.join(app, 'pako.min.js'));
  const faces = [];
  for (const [family, pkg, weights] of FONTS) {
    for (const w of weights) {
      for (const subset of ['latin', 'latin-ext']) {
        const file = `${pkg}-${subset}-${w}-normal.woff2`;
        const from = path.join(nm, '@fontsource', pkg, 'files', file);
        if (!fs.existsSync(from)) continue;
        fs.copyFileSync(from, path.join(app, 'fonts', file));
        faces.push(`@font-face { font-family: "${family}"; font-style: normal; font-weight: ${w}; font-display: swap; src: url("fonts/${file}") format("woff2"); }`);
      }
    }
  }
  fs.writeFileSync(path.join(app, 'fonts.css'), faces.join('\n') + '\n');
  const tags = ['<script src="pako.min.js"></script>'];
  for (const s of scripts()) {
    const rel = path.join('js', s.name);
    fs.mkdirSync(path.join(app, path.dirname(rel)), { recursive: true });
    fs.writeFileSync(path.join(app, rel), s.code);
    tags.push(`<script src="${rel.split(path.sep).join('/')}"></script>`);
  }
  const head = '<link rel="stylesheet" href="fonts.css">';
  const csp = "default-src 'self'; script-src 'self'; style-src 'self' 'unsafe-inline'; img-src 'self' data: blob:; font-src 'self'; connect-src 'self' blob:";
  const html = `<!doctype html>\n<html lang="en">\n<head>\n<meta charset="utf-8">\n<meta name="viewport" content="width=device-width, initial-scale=1">\n` +
    `<meta http-equiv="Content-Security-Policy" content="${csp}">\n${page(head, tags.join('\n'))}</html>\n`;
  fs.writeFileSync(path.join(app, 'index.html'), html);
  console.log(`wrote ${path.relative(process.cwd(), app)} (${faces.length} font files)`);
}

if (process.argv.includes('--desktop')) desktop();
else web();
