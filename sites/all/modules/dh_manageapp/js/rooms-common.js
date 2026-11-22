/* Shared by the Rooms page (rooms-editor.js) and the Room Board (room-board.js). */

/* The room chart: one row per room, a box per bed, two boxes on a line. Pure functions (no page needed), so they
 * can be tested with node. A room is {name, toilet (the label), beds: [{name, text}]}; text is what to show after
 * the bed name ('' = an empty bed, so staff can write in it). */
(function (root) {
  'use strict';

  // Rooms in their order -> blocks, one per tbody. A room with 2 or more beds is a 'multi' block: its beds go two per
  // line, [bed, bed|null] per line. Two single-bed rooms next to each other share one line: a 'single' block with a
  // left and a right half (right = null for an odd last room). A room with more beds in between breaks the pairing.
  function layout(rooms) {
    var out = [], pending = null;
    function flush() { if (pending) { out.push({kind: 'single', left: pending, right: null}); pending = null; } }
    rooms.forEach(function (r) {
      if (!r.beds || !r.beds.length) return;
      if (r.beds.length === 1) {
        var half = {name: r.name, toilet: r.toilet, bed: r.beds[0]};
        if (pending) { out.push({kind: 'single', left: pending, right: half}); pending = null; } else pending = half;
        return;
      }
      flush();
      var lines = [];
      for (var i = 0; i < r.beds.length; i += 2) lines.push([r.beds[i], r.beds[i + 1] || null]);
      out.push({kind: 'multi', name: r.name, toilet: r.toilet, lines: lines});
    });
    flush();
    return out;
  }

  function bedCell(b, esc, span) {
    if (!b) return '<td class="bed none"' + (span ? ' colspan="' + span + '"' : '') + '></td>';
    return '<td class="bed"' + (span ? ' colspan="' + span + '"' : '') + '><b>' + esc(b.name) + '</b>' + (b.text ? ' ' + esc(b.text) : '') + '</td>';
  }

  // Six columns (Room, Toilet, Bed, Room, Toilet, Bed): a room's two bed boxes each cover three of them minus the
  // room cells, so both are the same width.
  function tableHtml(rooms, esc) {
    var h = '<table class="chart"><thead><tr><th>Room</th><th>Toilet</th><th>Bed</th><th>Room</th><th>Toilet</th><th>Bed</th></tr></thead>';
    layout(rooms).forEach(function (b) {
      h += '<tbody>';
      if (b.kind === 'multi') {
        b.lines.forEach(function (ln, i) {
          h += '<tr>';
          if (i === 0) h += '<td class="room" rowspan="' + b.lines.length + '">' + esc(b.name) + '</td><td class="toilet" rowspan="' + b.lines.length + '">' + esc(b.toilet) + '</td>';
          h += bedCell(ln[0], esc, 2) + bedCell(ln[1], esc, 2) + '</tr>';
        });
      } else {
        h += '<tr><td class="room">' + esc(b.left.name) + '</td><td class="toilet">' + esc(b.left.toilet) + '</td>' + bedCell(b.left.bed, esc);
        if (b.right) h += '<td class="room">' + esc(b.right.name) + '</td><td class="toilet">' + esc(b.right.toilet) + '</td>' + bedCell(b.right.bed, esc);
        else h += '<td class="none" colspan="3"></td>';
        h += '</tr>';
      }
      h += '</tbody>';
    });
    return h + '</table>';
  }

  // "Section M — Male Common Block (Men + Women) · 5 of 8 beds taken" (the name only when the section has one).
  function heading(code, name, shared, count, esc) {
    return 'Section ' + esc(code) + (name ? ' — ' + esc(name) : '') + (shared ? ' (Men + Women)' : '') + (count ? ' · ' + esc(count) : '');
  }

  // sections: [{code, name, shared, count, rooms}] -> a heading and a table each; a section with no rooms is skipped.
  function sectionsHtml(sections, esc) {
    return sections.filter(function (s) { return layout(s.rooms).length; }).map(function (s) {
      return '<h2>' + heading(s.code, s.name, s.shared, s.count, esc) + '</h2>' + tableHtml(s.rooms, esc);
    }).join('');
  }

  var api = {layout: layout, tableHtml: tableHtml, heading: heading, sectionsHtml: sectionsHtml};
  if (typeof module !== 'undefined' && module.exports) module.exports = api;
  else root.dhRoomChart = api;
})(typeof window !== 'undefined' ? window : this);

if (typeof jQuery !== 'undefined') (function ($) {
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
        // The room chart: equal columns, every room in its own tbody that stays on one page, tall boxes to write in.
        + 'table.chart{table-layout:fixed;font-size:12px}table.chart th:nth-child(1),table.chart th:nth-child(4){width:9%}table.chart th:nth-child(2),table.chart th:nth-child(5){width:9%}'
        + 'table.chart tbody{page-break-inside:avoid;break-inside:avoid}table.chart tbody tr:first-child td{border-top:2px solid #333}'
        + 'table.chart td{height:30px;vertical-align:top}table.chart td.room{font-weight:bold}table.chart td.toilet{color:#444}table.chart td.none{background:#f4f4f4}'
        + '</style></head><body><h1>' + esc(title) + '</h1><p class="n">Printed ' + esc(new Date().toLocaleString()) + (note ? ' · ' + esc(note) : '') + '</p>'
        + (body || '<p>Nothing to print.</p>') + '</body></html>');
      w.document.close();
      w.focus();
      setTimeout(function () { w.print(); }, 300);
    },

    // The Room Board print of one side: the room chart, a table per section. A bed box shows the bed name, then the
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
            return {st: st, bed: {name: p.name, text: who ? who.map(label).join(', ') : (x ? x + ' (this course)' : oc)}};
          });
          if (fPlace && fPlace !== 'all' && !list.some(function (b) { return b.st === fPlace; })) return;
          list.forEach(function (b) { beds++; if (b.st !== 'free') used++; });
          rooms.push({name: r.name, toilet: d.toilets[r.toilet] || r.toilet, beds: list.map(function (b) { return b.bed; })});
        });
        sections.push({code: sec, name: o[sec][0].section_name || '', shared: o[sec][0].gender === 'B', count: used + ' of ' + beds + ' beds taken', rooms: rooms});
      });
      var names = {free: 'Free beds', placed: 'Taken beds', other: 'Beds of the other course'};
      var note = [names[fPlace] ? 'Only: ' + names[fPlace] : '', fToilet ? 'Toilet: ' + (d.toilets[fToilet] || fToilet) : ''].filter(Boolean).join(' · ');
      window.dhRoomsCommon.printDoc('Room Board - ' + d.course + ' - ' + (side === 'M' ? 'Men' : 'Women'), note,
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
          rooms: rs.map(function (r) { return {name: r.name, toilet: toilets[r.toilet] || r.toilet, beds: r.places.map(function (p) { return {name: p.name, text: ''}; })}; })};
      });
      window.dhRoomsCommon.printDoc(title, note, window.dhRoomChart.sectionsHtml(sections, esc));
    }
  };
})(jQuery);
