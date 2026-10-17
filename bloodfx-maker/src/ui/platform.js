/* Where BloodFX Maker is running: the Windows app (Electron), a Claude artifact, or a plain browser tab. */
(function (root) {
  'use strict';
  const desktop = root.bfxDesktop || null;
  const kind = desktop ? 'desktop' : root.claude ? 'claude' : 'browser';

  // A hidden file input, opened from a click. Resolves null when the dialog is cancelled.
  function pickFile(accept) {
    return new Promise((resolve) => {
      const input = document.createElement('input');
      input.type = 'file';
      input.accept = accept;
      input.style.display = 'none';
      document.body.append(input);
      input.addEventListener('change', () => { resolve(input.files[0] || null); input.remove(); });
      input.addEventListener('cancel', () => { resolve(null); input.remove(); });
      input.click();
    });
  }
  const fromFile = async (file) => (file ? { name: file.name, path: desktop ? desktop.pathForFile(file) : null, bytes: new Uint8Array(await file.arrayBuffer()) } : null);

  let claudeDownloads;
  async function claudeSave(filename, blob) {
    if (claudeDownloads === undefined) {
      try { claudeDownloads = await root.claude.use('downloads'); } catch { claudeDownloads = null; }
    }
    if (!claudeDownloads) throw Object.assign(new Error('Saving is not available in this view.'), { code: 'unavailable' });
    await claudeDownloads.save({ filename, data: blob });
    return { saved: true, where: 'your downloads' };
  }

  function browserSave(filename, blob) {
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url; a.download = filename;
    document.body.append(a); a.click(); a.remove();
    setTimeout(() => URL.revokeObjectURL(url), 60000);
    return { saved: true, where: 'your downloads' };
  }

  const Platform = {
    kind,
    canInstall: !!desktop,
    async openPack() {
      if (desktop) return desktop.openPack();
      return fromFile(await pickFile('.rpf,.ypt'));
    },
    async openDat() {
      if (desktop) return desktop.openDat();
      return fromFile(await pickFile('.dat,.txt'));
    },
    fromFile,
    async readRecent(path) { return desktop ? desktop.readRecent(path) : null; },
    async recents() { return desktop ? desktop.recents() : []; },
    async addRecent(entry) { if (desktop && entry.path) await desktop.addRecent(entry); },
    async saveZip(filename, bytes) {
      if (desktop) {
        const path = await desktop.saveZip(filename, bytes);
        return path ? { saved: true, where: path, path } : { saved: false };
      }
      const blob = new Blob([bytes], { type: 'application/zip' });
      return kind === 'claude' ? claudeSave(filename, blob) : browserSave(filename, blob);
    },
    fivem: desktop ? desktop.fivem : null,
    onOpenPath(cb) { if (desktop && desktop.onOpenPath) desktop.onOpenPath(cb); },
    revealFile(path) { if (desktop) desktop.reveal(path); },
  };
  root.BfxPlatform = Platform;
})(typeof self !== 'undefined' ? self : this);
