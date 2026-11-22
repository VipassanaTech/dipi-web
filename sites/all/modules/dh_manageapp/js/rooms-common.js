/* Shared by the Rooms page (rooms-editor.js) and the Room Board (room-board.js). */

/* The room chart print: room cards like the Room Board, four in a row; a card = the room name, its toilet and a box
 * per bed. Pure functions (no page needed), so they can be tested with node. A room is {name, toilet (the label, ''
 * when not set), beds: [{name, text, st}]}; text is what to show after the bed name ('' = an empty bed, so staff can
 * write in it); st = free | taken | other (when left out: taken when there is text, else free). */
(function (root) {
  'use strict';
  var COLS = 4;

  function card(r, esc) {
    var h = '<div class="rh"><b>' + esc(r.name) + '</b>' + (r.toilet ? ' <span class="tl">' + esc(r.toilet) + '</span>' : '') + '</div>';
    r.beds.forEach(function (b) {
      var st = b.st || (b.text ? 'taken' : 'free');
      h += '<div class="pl ' + esc(st) + '"><b>' + esc(b.name) + '</b>' + (b.text ? ' ' + esc(b.text) : '') + '</div>';
    });
    return '<td class="card">' + h + '</td>';
  }

  // Rooms in their order -> a table of cards, COLS in a row (a row never splits across pages); the last row is filled
  // up with empty cells so every card has the same width. A room with no beds is left out; no rooms = ''.
  function cardsHtml(rooms, esc) {
    var list = rooms.filter(function (r) { return r.beds && r.beds.length; });
    if (!list.length) return '';
    var h = '<table class="cards">';
    for (var i = 0; i < list.length; i += COLS) {
      h += '<tr>';
      for (var j = i; j < i + COLS; j++) h += j < list.length ? card(list[j], esc) : '<td class="nc"></td>';
      h += '</tr>';
    }
    return h + '</table>';
  }

  // "Section M — Male Common Block (Male + Female) · 5 of 8 beds taken" (the name only when the section has one).
  function heading(code, name, shared, count, esc) {
    return 'Section ' + esc(code) + (name ? ' — ' + esc(name) : '') + (shared ? ' (Male + Female)' : '') + (count ? ' · ' + esc(count) : '');
  }

  // sections: [{code, name, shared, count, rooms}] -> a heading and a card table each; a section with no beds is skipped.
  function sectionsHtml(sections, esc) {
    return sections.map(function (s) {
      var cards = cardsHtml(s.rooms, esc);
      return cards ? '<h2>' + heading(s.code, s.name, s.shared, s.count, esc) + '</h2>' + cards : '';
    }).join('');
  }

  var api = {COLS: COLS, cardsHtml: cardsHtml, heading: heading, sectionsHtml: sectionsHtml};
  if (typeof module !== 'undefined' && module.exports) module.exports = api;
  else root.dhRoomChart = api;
})(typeof window !== 'undefined' ? window : this);

if (typeof jQuery !== 'undefined') (function ($) {
  'use strict';
  function esc(s) { return $('<div/>').text(s == null ? '' : String(s)).html().replace(/"/g, '&quot;').replace(/'/g, '&#39;'); }
  // The toilet as printed on a room card: nothing when it is not set (it would only fill the page).
  function toiletLabel(toilets, key) { return key === 'unset' ? '' : (toilets[key] || key); }

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
        // The room chart: room cards like the Room Board, four in a row of equal width; a row stays on one page;
        // bed boxes tall enough to write a name in. Print the box colours too.
        + 'body{-webkit-print-color-adjust:exact;print-color-adjust:exact}'
        + 'table.cards{table-layout:fixed;border-collapse:separate;border-spacing:5px;width:calc(100% + 10px);margin:0 -5px}'
        + 'table.cards tr{page-break-inside:avoid;break-inside:avoid}'
        + 'table.cards td{border:1px solid #888;border-radius:5px;padding:4px 5px;height:auto}table.cards td.nc{border:0}'
        + '.rh{margin-bottom:2px}.rh b{font-size:13px}.tl{background:#faeeda;border-radius:3px;padding:0 4px;font-size:10px;margin-left:3px}'
        + '.pl{margin-top:3px;padding:3px 5px;border-radius:3px;min-height:26px;font-size:11px;box-sizing:border-box}'
        + '.pl.taken{background:#e6f1fb}.pl.free{border:1px dashed #555}.pl.other{background:#eeeeee;color:#444}'
        + '</style></head><body><h1>' + esc(title) + '</h1><p class="n">Printed ' + esc(new Date().toLocaleString()) + (note ? ' · ' + esc(note) : '') + '</p>'
        + (body || '<p>Nothing to print.</p>') + '</body></html>');
      w.document.close();
      w.focus();
      setTimeout(function () { w.print(); }, 300);
    },

    // The Room Board print of one side: the room chart, room cards per section. A bed box shows the bed name, then the
    // confirmation number and name of who is in it (blank when free, so a name can be written in). d = Room Board data
    // (room-board/{centre}/{course}/data); fPlace = all | free | placed | other; fToilet = a toilet key or ''.
    // A room is kept when its toilet matches and at least one of its beds matches the place filter; then all its beds show.
    // w = an open print window or none.
    boardPrint: function (d, side, fPlace, fToilet, w) {
      var key = function (s, p) { return s + '|' + p; };
      var get = function (m, s, p) { return (m && m[s] && m[s][p]) || ''; };
      var label = function (p) { return (p.conf ? p.conf + ' ' : '') + p.name; };
      var H = {}, ov = d.overlap[side] || {}, cr = (d.cross && d.cross[side]) || {}, order = [], o = {};
      d.people.forEach(function (p) { if (p.gender === side && p.place !== '') (H[key(p.section, p.place)] = H[key(p.section, p.place)] || []).push(p); });
      d.rooms.filter(function (r) { return r.gender === side || r.gender === 'B'; })
        .forEach(function (r) { if (!o[r.section]) { o[r.section] = []; order.push(r.section); } o[r.section].push(r); });
      var sections = [];
      order.forEach(function (sec) {
        var beds = 0, used = 0, rooms = [];
        o[sec].forEach(function (r) {
          if (fToilet && r.toilet !== fToilet) return;
          var list = r.places.map(function (p) {
            var who = H[key(sec, p.name)], x = get(cr, sec, p.name), oc = get(ov, sec, p.name);
            var st = (who || x) ? 'placed' : (oc ? 'other' : 'free');
            return {st: st, bed: {name: p.name, text: who ? who.map(label).join(', ') : (x ? x + ' (this course)' : oc), st: st === 'placed' ? 'taken' : st}};
          });
          if (fPlace && fPlace !== 'all' && !list.some(function (b) { return b.st === fPlace; })) return;
          list.forEach(function (b) { beds++; if (b.st !== 'free') used++; });
          rooms.push({name: r.name, toilet: toiletLabel(d.toilets, r.toilet), beds: list.map(function (b) { return b.bed; })});
        });
        sections.push({code: sec, name: o[sec][0].section_name || '', shared: o[sec][0].gender === 'B', count: used + ' of ' + beds + ' beds taken', rooms: rooms});
      });
      var names = {free: 'Free beds', placed: 'Taken beds', other: 'Beds of the other course'};
      var note = [names[fPlace] ? 'Only: ' + names[fPlace] : '', fToilet ? 'Toilet: ' + (d.toilets[fToilet] || fToilet) : ''].filter(Boolean).join(' · ');
      window.dhRoomsCommon.printDoc('Room Board - ' + d.course + ' - ' + (side === 'M' ? 'Male' : 'Female'), note,
        window.dhRoomChart.sectionsHtml(sections, esc), w);
    },

    // The same chart for the room list alone: every bed box shows only the bed name (a blank chart to write on).
    // rooms = the rooms to print (one side, filters already applied) as the Rooms page holds them; toilets = key -> label.
    roomsPrint: function (rooms, toilets, title, note) {
      var order = [], o = {};
      rooms.forEach(function (r) { if (!o[r.section]) { o[r.section] = []; order.push(r.section); } o[r.section].push(r); });
      var sections = order.map(function (sec) {
        var rs = o[sec], nb = rs.reduce(function (n, r) { return n + r.places.length; }, 0);
        return {code: sec, name: rs[0].section_name || '', shared: rs[0].gender === 'B', count: rs.length + ' room' + (rs.length === 1 ? '' : 's') + ' · ' + nb + ' bed' + (nb === 1 ? '' : 's'),
          rooms: rs.map(function (r) { return {name: r.name, toilet: toiletLabel(toilets, r.toilet), beds: r.places.map(function (p) { return {name: p.name, text: ''}; })}; })};
      });
      window.dhRoomsCommon.printDoc(title, note, window.dhRoomChart.sectionsHtml(sections, esc));
    }
  };
})(jQuery);
