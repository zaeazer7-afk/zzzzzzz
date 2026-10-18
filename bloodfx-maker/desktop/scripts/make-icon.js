// Renders the logo from src/ui/data.js into build/icon.png (256 px) and build/icon.ico (16 to 256 px).
// Run with: npx electron scripts/make-icon.js
const { app, BrowserWindow } = require('electron');
const fs = require('fs');
const path = require('path');
const { LOGO } = require('../../src/ui/data.js').BfxData;

const SIZES = [16, 24, 32, 48, 64, 128, 256];
const out = path.join(__dirname, '..', 'build');

// An .ico holding PNG images, which Windows Vista and later read at every size.
function ico(pngs) {
  const head = Buffer.alloc(6 + 16 * pngs.length);
  head.writeUInt16LE(0, 0);
  head.writeUInt16LE(1, 2);
  head.writeUInt16LE(pngs.length, 4);
  let offset = head.length;
  pngs.forEach(({ size, data }, i) => {
    const o = 6 + i * 16;
    head.writeUInt8(size >= 256 ? 0 : size, o);
    head.writeUInt8(size >= 256 ? 0 : size, o + 1);
    head.writeUInt16LE(1, o + 4);
    head.writeUInt16LE(32, o + 6);
    head.writeUInt32LE(data.length, o + 8);
    head.writeUInt32LE(offset, o + 12);
    offset += data.length;
  });
  return Buffer.concat([head, ...pngs.map((p) => p.data)]);
}

app.disableHardwareAcceleration();
app.whenReady().then(async () => {
  const pngs = [];
  const page = path.join(app.getPath('temp'), 'bfxm-icon.html');
  fs.writeFileSync(page, '<!doctype html><body style="margin:0;background:transparent;overflow:hidden"></body>');
  const win = new BrowserWindow({ width: 256, height: 256, useContentSize: true, show: false, frame: false, transparent: true, webPreferences: { offscreen: true } });
  await win.loadFile(page);
  for (const size of SIZES) {
    // each size is drawn from the vector logo, not scaled down from the biggest
    const svg = LOGO.replace('<svg ', `<svg width="${size}" height="${size}" style="display:block" `);
    await win.webContents.executeJavaScript(`document.body.innerHTML = ${JSON.stringify(svg)}; new Promise((r) => requestAnimationFrame(() => requestAnimationFrame(r)))`);
    await new Promise((r) => setTimeout(r, 150));
    const img = await win.webContents.capturePage({ x: 0, y: 0, width: size, height: size });
    pngs.push({ size, data: img.resize({ width: size, height: size, quality: 'best' }).toPNG() });
  }
  win.destroy();
  fs.rmSync(page, { force: true });
  fs.mkdirSync(out, { recursive: true });
  fs.writeFileSync(path.join(out, 'icon.png'), pngs[pngs.length - 1].data);
  fs.writeFileSync(path.join(out, 'icon.ico'), ico(pngs));
  console.log(`wrote build/icon.png and build/icon.ico (${SIZES.join(', ')} px)`);
  app.quit();
});
