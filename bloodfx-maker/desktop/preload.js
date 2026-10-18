// The only bridge between the window and the computer: file dialogs, recent packs, saving, and the FiveM install.
const { contextBridge, ipcRenderer, webUtils } = require('electron');

async function call(channel, ...args) {
  const r = await ipcRenderer.invoke(channel, ...args);
  if (r && typeof r === 'object' && typeof r.__error === 'string') throw new Error(r.__error);
  return r;
}

contextBridge.exposeInMainWorld('bfxDesktop', {
  openPack: () => call('open-pack'),
  openDat: () => call('open-dat'),
  readRecent: (file) => call('read-recent', file),
  pathForFile: (file) => { try { return webUtils.getPathForFile(file) || null; } catch { return null; } },
  recents: () => call('recents'),
  addRecent: (entry) => call('add-recent', { path: entry && entry.path, name: entry && entry.name }),
  saveZip: (name, bytes) => call('save-zip', name, bytes),
  reveal: (file) => call('reveal', file),
  onOpenPath: (cb) => { ipcRenderer.on('open-path', (_e, file) => cb(file)); },
  fivem: {
    detect: () => call('fivem-detect'),
    choose: () => call('fivem-choose'),
    scan: () => call('fivem-scan'),
    toggle: (name, on) => call('fivem-toggle', name, on),
    install: (o) => call('fivem-install', { rpfName: o.rpfName, rpf: o.rpf, dat: o.dat, disable: o.disable }),
    backups: () => call('fivem-backups'),
    restore: (id) => call('fivem-restore', id),
    open: (which) => call('fivem-open', which),
  },
});
