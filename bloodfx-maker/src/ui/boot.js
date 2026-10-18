/* Starts BloodFX Maker once every module has loaded. */
(function (root) {
  'use strict';
  try {
    root.BfxApp.start(root.BfxPages || {});
  } catch (e) {
    const box = document.getElementById('fatal');
    document.getElementById('fatal-text').textContent = 'BloodFX Maker hit an error while starting: ' + (e && e.message ? e.message : String(e));
    box.hidden = false;
    throw e;
  }
})(typeof self !== 'undefined' ? self : this);
