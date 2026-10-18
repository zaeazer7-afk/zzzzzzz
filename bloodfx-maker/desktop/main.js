// BloodFX Maker for Windows: one window running the same interface as the web page, plus file dialogs,
// recent packs and a one-click FiveM install with backups.
const { app, BrowserWindow, ipcMain, dialog, shell, Menu, session } = require('electron');
const fs = require('fs');
const path = require('path');
const { createFiveM } = require('./fivem');

const OPENABLE = /\.(rpf|ypt|dat|txt)$/i;
const MAX_FILE = 1024 * 1024 * 1024;
let win = null;
let fivem = null;
let pendingPath = null;

const userFile = (name) => path.join(app.getPath('userData'), name);
const readJson = (file, fallback) => { try { return JSON.parse(fs.readFileSync(file, 'utf8')); } catch { return fallback; } };
const writeJson = (file, value) => { fs.mkdirSync(path.dirname(file), { recursive: true }); fs.writeFileSync(file, JSON.stringify(value, null, 2)); };
const argPath = (argv) => argv.slice(1).find((a) => !a.startsWith('-') && OPENABLE.test(a) && fs.existsSync(a)) || null;

function readEntry(file) {
  const st = fs.statSync(file);
  if (!st.isFile()) throw new Error(`${path.basename(file)} isn't a file.`);
  if (st.size > MAX_FILE) throw new Error(`${path.basename(file)} is too big to be a tracer pack.`);
  return { name: path.basename(file), path: file, bytes: new Uint8Array(fs.readFileSync(file)) };
}

// Errors cross to the window as plain messages, without Electron's "Error invoking remote method" prefix.
function handle(channel, fn) {
  ipcMain.handle(channel, async (event, ...args) => {
    if (!win || event.sender !== win.webContents) return { __error: 'Unknown window.' };
    try { return await fn(...args); } catch (e) { return { __error: e && e.message ? e.message : String(e) }; }
  });
}

async function pick(filters) {
  const r = await dialog.showOpenDialog(win, { properties: ['openFile'], filters: [...filters, { name: 'All files', extensions: ['*'] }] });
  if (r.canceled || !r.filePaths[0]) return null;
  return readEntry(r.filePaths[0]);
}

function wire() {
  handle('open-pack', () => pick([{ name: 'Tracer packs', extensions: ['rpf', 'ypt'] }]));
  handle('open-dat', () => pick([{ name: 'bloodfx.dat', extensions: ['dat', 'txt'] }]));
  handle('read-recent', (file) => (typeof file === 'string' && OPENABLE.test(file) && fs.existsSync(file) ? readEntry(file) : null));
  handle('recents', () => readJson(userFile('recents.json'), []).filter((r) => r && typeof r.path === 'string' && fs.existsSync(r.path)));
  handle('add-recent', (entry) => {
    if (!entry || typeof entry.path !== 'string' || !OPENABLE.test(entry.path)) return false;
    const list = readJson(userFile('recents.json'), []).filter((r) => r && r.path !== entry.path);
    list.unshift({ path: entry.path, name: path.basename(entry.path), when: Date.now() });
    writeJson(userFile('recents.json'), list.slice(0, 10));
    return true;
  });
  handle('save-zip', async (name, bytes) => {
    const safe = String(name || '').replace(/[<>:"/\\|?*\x00-\x1f]/g, '_').slice(0, 120) || 'bloodfx-maker.zip';
    const r = await dialog.showSaveDialog(win, { defaultPath: path.join(app.getPath('downloads'), safe), filters: [{ name: 'Zip files', extensions: ['zip'] }] });
    if (r.canceled || !r.filePath) return null;
    fs.writeFileSync(r.filePath, Buffer.from(bytes.buffer, bytes.byteOffset, bytes.byteLength));
    return r.filePath;
  });
  handle('reveal', (file) => { if (typeof file === 'string' && fs.existsSync(file)) shell.showItemInFolder(file); return true; });
  handle('fivem-detect', () => fivem.detect());
  handle('fivem-choose', async () => {
    const start = fivem.detect();
    const r = await dialog.showOpenDialog(win, { title: 'Choose your FiveM.app folder', defaultPath: start.found ? start.root : path.dirname(start.root), properties: ['openDirectory'] });
    if (r.canceled || !r.filePaths[0]) return null;
    return fivem.useFolder(r.filePaths[0]);
  });
  handle('fivem-scan', () => fivem.scan());
  handle('fivem-toggle', (name, on) => fivem.toggle(name, !!on));
  handle('fivem-install', (o) => fivem.install({ rpfName: o && o.rpfName, rpf: o && o.rpf, dat: o && o.dat, disable: o && o.disable }));
  handle('fivem-backups', () => fivem.backups());
  handle('fivem-restore', (id) => fivem.restore(id));
  handle('fivem-open', async (which) => { const err = await shell.openPath(fivem.folder(which)); if (err) throw new Error(err); return true; });
}

function createWindow() {
  const saved = readJson(userFile('window.json'), {});
  win = new BrowserWindow({
    width: saved.width || 1480,
    height: saved.height || 920,
    x: saved.x,
    y: saved.y,
    minWidth: 1024,
    minHeight: 680,
    show: false,
    backgroundColor: '#06090f',
    title: 'BloodFX Maker',
    icon: path.join(__dirname, 'build', 'icon.png'),
    titleBarStyle: 'hidden',
    titleBarOverlay: { color: '#06090f', symbolColor: '#9ba7bd', height: 42 },
    webPreferences: { preload: path.join(__dirname, 'preload.js'), contextIsolation: true, nodeIntegration: false, sandbox: true, spellcheck: false },
  });
  if (saved.maximized) win.maximize();
  win.loadFile(path.join(__dirname, 'app', 'index.html'));
  win.once('ready-to-show', () => win.show());
  win.webContents.on('did-finish-load', () => {
    if (pendingPath) { win.webContents.send('open-path', pendingPath); pendingPath = null; }
  });
  // links open in the browser; the window itself never navigates away from the app
  win.webContents.setWindowOpenHandler(({ url }) => { if (/^https:\/\//.test(url)) shell.openExternal(url); return { action: 'deny' }; });
  win.webContents.on('will-navigate', (e) => e.preventDefault());
  win.on('close', () => {
    try { writeJson(userFile('window.json'), { ...win.getNormalBounds(), maximized: win.isMaximized() }); } catch { /* not worth failing a close over */ }
  });
  win.on('closed', () => { win = null; });
}

if (!app.requestSingleInstanceLock()) {
  app.quit();
} else {
  app.on('second-instance', (_e, argv) => {
    if (!win) return;
    if (win.isMinimized()) win.restore();
    win.focus();
    const file = argPath(argv);
    if (file) win.webContents.send('open-path', file);
  });
  app.whenReady().then(() => {
    fivem = createFiveM(app.getPath('userData'));
    Menu.setApplicationMenu(null);
    session.defaultSession.setPermissionRequestHandler((_wc, _permission, done) => done(false));
    pendingPath = argPath(process.argv);
    wire();
    createWindow();
  });
  app.on('window-all-closed', () => app.quit());
}
