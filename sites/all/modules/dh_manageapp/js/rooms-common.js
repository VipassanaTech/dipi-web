/* Shared by the Rooms page (rooms-editor.js) and the Room Board (room-board.js). */
(function ($) {
  'use strict';
  function esc(s) { return $('<div/>').text(s == null ? '' : String(s)).html().replace(/"/g, '&quot;').replace(/'/g, '&#39;'); }

  window.dhRoomsCommon = {
    esc: esc,

    // The site header is fixed at the top: its bottom edge in px, also set as CSS --dhr-top on the app element so a
    // pinned (sticky) bar sits just below it.
    fixedTop: function (appId) {
      var t = 0, vh = window.innerHeight;
      $('body').find('> *, > * > *, > * > * > *').each(function () {
        if (this.offsetHeight === 0 || window.getComputedStyle(this).position !== 'fixed') return;
        var b = this.getBoundingClientRect();
        if (b.top < vh / 3 && b.height < vh / 2) t = Math.max(t, b.bottom);
      });
      var app = document.getElementById(appId);
      if (app) app.style.setProperty('--dhr-top', Math.round(t) + 'px');
      return t;
    },

    // A clean printable page (no site menus or buttons) in a new window, then the browser's print dialog.
    // The title is also the file name when saved as PDF. body = trusted HTML built with esc().
    printDoc: function (title, note, body) {
      var w = window.open('', '_blank');
      if (!w) { alert('Please allow pop-ups for this site, then press Print again.'); return; }
      w.document.write('<!doctype html><html><head><meta charset="utf-8"><title>' + esc(title) + '</title><style>'
        + 'body{font-family:Arial,sans-serif;font-size:11px;color:#000;margin:16px}h1{font-size:16px;margin:0 0 2px}p.n{margin:0 0 10px;color:#444}'
        + 'h2{font-size:13px;margin:14px 0 4px;page-break-after:avoid}table{border-collapse:collapse;width:100%}'
        + 'th,td{border:1px solid #999;padding:2px 5px;text-align:left;vertical-align:top}th{background:#eee}'
        + 'thead{display:table-header-group}tr{page-break-inside:avoid}tr.rs td{border-top:2px solid #333}tr.free td{color:#666}'
        + '</style></head><body><h1>' + esc(title) + '</h1><p class="n">Printed ' + esc(new Date().toLocaleString()) + (note ? ' · ' + esc(note) : '') + '</p>'
        + (body || '<p>Nothing to print.</p>') + '</body></html>');
      w.document.close();
      w.focus();
      setTimeout(function () { w.print(); }, 300);
    }
  };
})(jQuery);
