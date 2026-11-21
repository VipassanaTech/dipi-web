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

    // A window for a print, opened at once while the click still counts (a window opened later, after loading data,
    // is blocked as a pop-up). null when blocked.
    printWindow: function () {
      var w = window.open('', '_blank');
      if (!w) { alert('Please allow pop-ups for this site, then press Print again.'); return null; }
      w.document.write('<p style="font-family:Arial,sans-serif">Getting the list ready…</p>');
      return w;
    },

    // A clean printable page (no site menus or buttons) in a new window, then the browser's print dialog.
    // The title is also the file name when saved as PDF. body = trusted HTML built with esc(). w = a window from
    // printWindow(), or none to open one now.
    printDoc: function (title, note, body, w) {
      w = w || window.dhRoomsCommon.printWindow();
      if (!w || w.closed) return;
      w.document.open();
      w.document.write('<!doctype html><html><head><meta charset="utf-8"><title>' + esc(title) + '</title><style>'
        + 'body{font-family:Arial,sans-serif;font-size:11px;color:#000;margin:16px}h1{font-size:16px;margin:0 0 2px}p.n{margin:0 0 10px;color:#444}'
        + 'h2{font-size:13px;margin:14px 0 4px;page-break-after:avoid}table{border-collapse:collapse;width:100%}'
        + 'th,td{border:1px solid #999;padding:2px 5px;text-align:left;vertical-align:top}th{background:#eee}'
        + 'thead{display:table-header-group}tr{page-break-inside:avoid}tr.rs td{border-top:2px solid #333}tr.free td{color:#666}td{height:16px}td.who{width:50%}'
        + '</style></head><body><h1>' + esc(title) + '</h1><p class="n">Printed ' + esc(new Date().toLocaleString()) + (note ? ' · ' + esc(note) : '') + '</p>'
        + (body || '<p>Nothing to print.</p>') + '</body></html>');
      w.document.close();
      w.focus();
      setTimeout(function () { w.print(); }, 300);
    },

    // The Room Board print of one side: a table per section, Room · Bed · Student · Toilet. An empty bed's Student
    // cell stays blank so names can be written in. d = Room Board data (room-board/{centre}/{course}/data);
    // fPlace = all | free | placed | other; fToilet = a toilet key or ''. w = an open print window or none.
    boardPrint: function (d, side, fPlace, fToilet, w) {
      var key = function (s, p) { return s + '|' + p; };
      var get = function (m, s, p) { return (m && m[s] && m[s][p]) || ''; };
      var label = function (p) { return (p.conf ? p.conf + ' ' : '') + p.name; };
      var H = {}, ov = d.overlap[side] || {}, cr = (d.cross && d.cross[side]) || {}, order = [], o = {}, body = '';
      d.people.forEach(function (p) { if (p.gender === side && p.place !== '') (H[key(p.section, p.place)] = H[key(p.section, p.place)] || []).push(p); });
      d.rooms.filter(function (r) { return r.gender === side || r.gender === 'B'; })
        .forEach(function (r) { if (!o[r.section]) { o[r.section] = []; order.push(r.section); } o[r.section].push(r); });
      order.forEach(function (sec) {
        var rows = '', beds = 0, used = 0;
        o[sec].forEach(function (r) {
          if (fToilet && r.toilet !== fToilet) return;
          var first = true;
          r.places.forEach(function (p) {
            var who = H[key(sec, p.name)], x = get(cr, sec, p.name), oc = get(ov, sec, p.name);
            var st = (who || x) ? 'placed' : (oc ? 'other' : 'free');
            if (fPlace && fPlace !== 'all' && st !== fPlace) return;
            var text = who ? who.map(label).join(', ') : (x ? x + ' (this course)' : oc);
            rows += '<tr class="' + (first ? 'rs' : '') + (st === 'free' ? ' free' : '') + '"><td>' + (first ? esc(r.name) : '') + '</td><td>' + esc(p.name) + '</td><td class="who">' + esc(text)
              + '</td><td>' + (first ? esc(d.toilets[r.toilet] || r.toilet) : '') + '</td></tr>';
            first = false;
            beds++;
            if (st !== 'free') used++;
          });
        });
        if (rows) body += '<h2>Section ' + esc(sec) + (o[sec][0].gender === 'B' ? ' (Men + Women)' : '') + ' · ' + used + ' of ' + beds + ' beds taken</h2>'
          + '<table><thead><tr><th>Room</th><th>Bed</th><th>Student</th><th>Toilet</th></tr></thead><tbody>' + rows + '</tbody></table>';
      });
      var names = {free: 'Free beds', placed: 'Taken beds', other: 'Beds of the other course'};
      var note = [names[fPlace] ? 'Only: ' + names[fPlace] : '', fToilet ? 'Toilet: ' + (d.toilets[fToilet] || fToilet) : ''].filter(Boolean).join(' · ');
      window.dhRoomsCommon.printDoc('Room Board - ' + d.course + ' - ' + (side === 'M' ? 'Men' : 'Women'), note, body, w);
    }
  };
})(jQuery);
