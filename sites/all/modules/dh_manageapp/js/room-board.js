/* Room Board of a course: sections → rooms → places, who is where, the people not placed yet.
 * Tap a person, then a free place to place them; tap a placed person, then another place to move,
 * or another placed person to swap. Every action POSTs to saveUrl and redraws from the returned data. */
(function ($) {
  'use strict';
  // narrow = phone width (people list above the rooms): the pinned "Chosen" bar gets a button down to the rooms.
  // fPlace = the place filter: all | free | placed | other (another course at the same time).
  var S, D = null, side = null, open = {}, chosen = null, q = '', fToilet = '', fPlace = 'all', show = 'waiting';
  // Self-refresh state: busy = a save is running; lastAct = last click/key/touch in the board; loadedAt = data time.
  var busy = false, lastAct = 0, loadedAt = null, retryT = null;
  var narrow = window.innerWidth < 700;

  function esc(s) { return $('<div/>').text(s == null ? '' : String(s)).html().replace(/"/g, '&quot;').replace(/'/g, '&#39;'); }
  function key(sec, place) { return sec + '|' + place; }
  function holders() {
    var h = {};
    D.people.forEach(function (p) { if (p.gender === side && p.place !== '') (h[key(p.section, p.place)] = h[key(p.section, p.place)] || []).push(p); });
    return h;
  }
  function anyHit(list) { return list.some(hit); }
  function person(id) { return D.people.filter(function (p) { return p.a_id === id; })[0]; }
  // Another course of the centre at the same time holds this place: the only case that asks "use it anyway?".
  function otherCourse(sec, place) { var o = D.overlap[side] || {}; return (o[sec] && o[sec][place]) || ''; }
  // In a shared section the other side of THIS course holds it ("Female"/"Male"): simply taken, a place goes to one person.
  function otherSide(sec, place) { var x = (D.cross && D.cross[side]) || {}; return (x[sec] && x[sec][place]) || ''; }
  // A place for the counts and the filter: placed (this course, either side), other (another course) or free.
  function state(H, sec, place) { return (H[key(sec, place)] || otherSide(sec, place)) ? 'placed' : (otherCourse(sec, place) ? 'other' : 'free'); }
  function bySection(rooms) {
    var order = [], o = {};
    rooms.forEach(function (r) { if (!o[r.section]) { o[r.section] = []; order.push(r.section); } o[r.section].push(r); });
    return {order: order, o: o};
  }
  var FILTERS = [['all', 'All places'], ['free', 'Free'], ['placed', 'Placed'], ['other', 'Other course']];
  function onSide(r) { return r.gender === side || r.gender === 'B'; }
  // One line of what centres look at when giving a room: Old/New/Server, age, job, education, state + country, a different nationality.
  function details(p) {
    var where = [p.state, p.country].filter(Boolean).join(', ');
    return [p.server ? 'Server' : (p.old ? 'Old' : 'New'), p.age !== null ? p.age + ' yrs' : '', p.occupation, p.education, where,
      p.nationality ? 'Nationality: ' + p.nationality : ''].filter(Boolean).join(' · ');
  }
  function hintsHtml(p) { return p.hints.map(function (x) { return '<span class="dhrb-hint">' + esc(x) + '</span>'; }).join(''); }
  // Confirmation number + name, as staff call people: "AB123 Ram Kumar".
  function label(p) { return (p.conf ? p.conf + ' ' : '') + p.name; }
  function labelHtml(p) { return (p.conf ? '<span class="dhrb-conf">' + esc(p.conf) + '</span> ' : '') + esc(p.name); }
  // Where a person is, with the section (many sections have a place 2A): "R · 2A".
  function where(p) { return p.place ? (p.section ? p.section + ' · ' + p.place : p.place) : ''; }
  // Search: confirmation number, name, job, education, state, country, nationality or place ("2A", "R 2A", "R-2A" or "R · 2A").
  function hit(p) {
    if (!q) return true;
    var t = q.toLowerCase();
    return [p.conf, p.name, p.occupation, p.education, p.state, p.country, p.nationality, p.place, p.place && where(p), p.place && p.section + ' ' + p.place, p.place && p.section + '-' + p.place]
      .some(function (s) { return s && String(s).toLowerCase().indexOf(t) >= 0; });
  }
  // A room place as staff and the change log write it: section-place, "M-5A".
  function roomText(sec, place) { return place ? (sec ? sec + '-' + place : place) : ''; }
  // Every change asks first, naming the person(s) and the rooms (owner, 25 Nov 2026).
  function sure(text) { return window.confirm(text); }

  // Room changes log under the board (owner, 25 Nov 2026): every room change of this side (Room Board, Zero Day,
  // Edit Application; D.log from dh_room_board_log). Old room empty = placed fresh; new room empty = taken out.
  // Collapsed by default; a DataTables table (search, sort, pages) in its own box (#dh-rb-log), updated in place when
  // the log changes, so the board's redraws never reset its search, sort or page.
  var MONTHS = ['Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun', 'Jul', 'Aug', 'Sep', 'Oct', 'Nov', 'Dec'];
  var logOpen = false, logDT = null, logKey = null;
  function when(t) { return parseInt(t.slice(8, 10), 10) + ' ' + MONTHS[parseInt(t.slice(5, 7), 10) - 1] + ', ' + t.slice(11, 16); }
  function txt(d, type) { return type === 'display' ? esc(d) : d; }
  function renderLog() {
    var box = $('#dh-rb-log');
    if (!box.length) return;
    if (!D || !D.ok || !D.has_rooms) { box.empty(); logDT = null; logKey = null; return; }
    var rows = (D.log || []).filter(function (e) { return e.gender === side; });
    if (!box.children().length)
      box.html('<div class="dhrb-log"><h3 class="dhrb-loghead" role="button" tabindex="0"></h3><div class="dhrb-logbody">'
        + '<table class="dhrb-logt display"><thead><tr><th>Time</th><th>Conf. no</th><th>Name</th><th>Old room</th><th>New room</th><th>By</th></tr></thead></table></div></div>');
    box.find('.dhrb-loghead').text((logOpen ? '▾ ' : '▸ ') + 'Room changes · ' + (side === 'M' ? 'Male' : 'Female') + ' · ' + rows.length);
    box.find('.dhrb-logbody').toggle(logOpen);
    if (!logOpen) return;
    var key = side + '|' + JSON.stringify(rows);
    if (!logDT) {
      logDT = box.find('table').DataTable({
        data: rows, order: [[0, 'desc']], pageLength: 25, lengthMenu: [25, 50, 100, 300], autoWidth: false,
        columns: [
          {data: 't', render: function (d, type) { return type === 'display' || type === 'filter' ? esc(when(d)) : d; }},
          {data: 'conf', render: txt}, {data: 'name', render: txt}, {data: 'old', render: txt}, {data: 'new', render: txt}, {data: 'user', render: txt}],
        language: {search: 'Search:', emptyTable: 'No room changes yet.', zeroRecords: 'Nothing found.'}
      });
      logKey = key;
    }
    else if (key !== logKey) { logKey = key; logDT.clear().rows.add(rows).draw(false); }   // keeps search, sort and page
  }

  // Typing a search opens the sections where it finds someone, once; after that they close like any other section.
  function openHits() {
    if (!q) return;
    var H = holders();
    D.rooms.filter(onSide).forEach(function (r) {
      if (r.places.some(function (p) { var w = H[key(r.section, p.name)]; return w && anyHit(w); })) open[side + '|' + r.section] = true;
    });
  }

  function roomOf(p) {
    var r = D.rooms.filter(function (r) { return (r.gender === p.gender || r.gender === 'B') && r.section === p.section && r.places.some(function (x) { return x.name === p.place; }); })[0];
    return r ? r.name : '';
  }

  // Keep in place (owner, 25 Nov 2026): a redraw must not move what is on the screen. Before it, note where the last
  // tapped card / place / section / control is on the screen (and, as a fallback, the first card or place visible
  // under the Chosen bar); after it, scroll so that the same thing is at the same spot again.
  var lastKey = null;
  function keyOf(el) {
    var $e = $(el);
    function attr(n) { return '[' + n + '="' + String($e.attr(n)).replace(/(["\\])/g, '\\$1') + '"]'; }
    if ($e.hasClass('dhrb-pl')) return '.dhrb-pl' + attr('data-sec') + attr('data-place');
    if ($e.hasClass('dhrb-card')) return '.dhrb-card' + attr('data-a');
    if ($e.hasClass('dhrb-sec')) return '.dhrb-sec' + attr('data-sec');
    if ($e.hasClass('dhrb-count')) return '.dhrb-count' + attr('data-f');
    if ($e.hasClass('dhrb-show')) return '.dhrb-show' + attr('data-show');
    var cls = ['dhrb-q', 'dhrb-ft', 'dhrb-fp', 'dhrb-openall', 'dhrb-closeall'].filter(function (k) { return $e.hasClass(k); })[0];
    return cls ? '.' + cls : null;
  }
  var KEYED = '.dhrb-pl, .dhrb-card, .dhrb-sec, .dhrb-count, .dhrb-show, .dhrb-q, .dhrb-ft, .dhrb-fp, .dhrb-openall, .dhrb-closeall';
  function spot(sel) {
    var el = sel ? document.querySelector('#dh-rb-app ' + sel) : null;
    return el ? {sel: sel, top: el.getBoundingClientRect().top} : null;
  }
  function anchors(visibleOnly) {
    var app = document.getElementById('dh-rb-app'), out = [];
    if (!app) return out;
    if (!visibleOnly && lastKey) { var s = spot(lastKey); if (s) out.push(s); }
    var pin = app.querySelector('.dhrb-pin'), min = pin ? pin.getBoundingClientRect().bottom : 0;
    // Fallback: the first room place or section visible under the bar (the rooms move with the page), else a card.
    var visible = function (sel) { return [].filter.call(app.querySelectorAll(sel), function (x) { var r = x.getBoundingClientRect(); return r.top >= min && r.top < window.innerHeight; })[0]; };
    var vis = visible('.dhrb-pl, .dhrb-sec') || visible('.dhrb-card');
    if (vis && keyOf(vis)) out.push({sel: keyOf(vis), top: vis.getBoundingClientRect().top});
    return out;
  }
  function backInPlace(list) {
    for (var i = 0; i < list.length; i++) {
      var el = document.querySelector('#dh-rb-app ' + list[i].sel);
      if (!el) continue;
      var diff = el.getBoundingClientRect().top - list[i].top, box = $(el).closest('.dhrb-people')[0];
      // A card in the people list that scrolls on its own (computer): move the list, not the page.
      if (box && box.scrollHeight > box.clientHeight && window.getComputedStyle(box).overflowY !== 'visible') box.scrollTop += diff;
      else window.scrollBy(0, diff);
      return;
    }
  }

  // visibleOnly: keep the first visible thing in place (the self-refresh); otherwise the last tapped thing.
  function render(visibleOnly) {
    var keep = anchors(visibleOnly === true);
    draw();
    backInPlace(keep);
    renderLog();   // below the board: never moves what is on the screen above it
  }

  function draw() {
    if (!D.ok) { $('#dh-rb-app').html('<p>' + esc(D.msg) + '</p>'); return; }
    if (!D.has_rooms) { $('#dh-rb-app').html('<p>This centre has no rooms yet. Rooms are set up on the Rooms page.</p>'); return; }
    if (!side) side = D.sides[0] || 'M';
    var H = holders(), rooms = D.rooms.filter(onSide);
    var total = 0, placed = 0, otherN = 0, freeN = 0, shared = 0;
    rooms.forEach(function (r) { r.places.forEach(function (p) {
      var w = H[key(r.section, p.name)], st = state(H, r.section, p.name);
      total++;
      if (w) { placed += w.length; if (w.length > 1) shared++; }
      else if (st === 'placed') placed++;
      else if (st === 'other') otherN++;
      else freeN++;
    }); });
    var waiting = D.people.filter(function (p) { return p.gender === side && p.place === ''; });
    var h = '';
    if (D.finalized) h += '<div class="dhr-note">This course is finalized. The board is read-only.</div>';
    if (D.sides.length > 1) h += '<div class="dhr-tabs">' + D.sides.map(function (s) { return '<button class="dhr-tab' + (s === side ? ' on' : '') + '" data-side="' + esc(s) + '">' + (s === 'M' ? 'Male' : 'Female') + '</button>'; }).join('') + '</div>';
    // The count boxes are also the place filter (tap "Free" to see only free places).
    var box = function (f, title, n) { return '<div class="dhrb-count' + (fPlace === f ? ' on' : '') + '" data-f="' + f + '">' + title + '<b>' + n + '</b></div>'; };
    h += '<div class="dhrb-counts">' + box('all', 'Places', total) + box('placed', 'Placed', placed) + box('other', 'Other course', otherN) + box('free', 'Free', freeN) + '</div>'
      + '<small class="dhrb-updated">' + updatedText() + '</small>';
    if (shared) h += '<div class="dhr-note">' + (shared === 1 ? '1 place has' : shared + ' places have') + ' more than one person.</div>';
    // The chosen person sits in a bar over both columns that stays under the site header while the board scrolls.
    // The bar is ALWAYS there with one fixed height (css .dhrb-chosen), each part on one line, so choosing, clearing
    // or placing someone never changes the page height and the rooms do not jump (owner, 25 Nov 2026).
    h += '<div class="dhrb-wrap">';
    var c = chosen ? person(chosen) : null;
    if (c) {
      var status = c.place ? 'In place ' + c.place + ' (Room ' + roomOf(c) + ', section ' + c.section + '). Tap a free place to move, or a person to swap.' : 'Not placed yet. Tap a free place.';
      h += '<div class="dhrb-chosen dhrb-pin"><b class="dhrb-line">Chosen: ' + labelHtml(c) + '</b>'
        + '<small class="dhrb-det dhrb-line" title="' + esc(details(c)) + '">' + esc(details(c)) + '</small>'
        + '<span class="dhrb-line dhrb-hints" title="' + esc(c.hints.join(' · ')) + '">' + hintsHtml(c) + '</span>'
        + '<small class="dhrb-det dhrb-line" title="' + esc(status) + '">' + esc(status) + '</small>'
        + '<p class="dhrb-acts">' + (narrow ? '<button class="btn btn-primary btn-sm dhrb-torooms">Find a place ↓</button> ' : '')
        + (c.place ? '<button class="btn btn-default btn-sm dhrb-remove">Take out of the room</button> ' : '') + '<button class="btn btn-link btn-sm dhrb-unchoose">Clear</button></p></div>';
    }
    else h += '<div class="dhrb-chosen dhrb-pin dhrb-none"><b class="dhrb-line">Chosen: nobody</b>'
      + '<small class="dhrb-det dhrb-line">Tap a person in the list to choose them, then tap a free place.</small>'
      + '<span class="dhrb-line dhrb-hints"></span><small class="dhrb-det dhrb-line"></small><p class="dhrb-acts"></p></div>';
    h += '<div class="dhrb-main"><div class="dhrb-people">';
    var mine = D.people.filter(function (p) { return p.gender === side; }), placedN = mine.length - waiting.length;
    h += '<div class="dhr-tabs">' + [['waiting', 'Not placed · ' + waiting.length], ['placed', 'Placed · ' + placedN], ['all', 'All · ' + mine.length]].map(function (t) {
      return '<button class="dhr-tab dhrb-show' + (show === t[0] ? ' on' : '') + '" data-show="' + t[0] + '">' + t[1] + '</button>'; }).join('') + '</div>'
      + (D.optional ? '<small>Room is optional on this course.</small>' : '')
      + '<input type="text" class="form-control dhrb-q" placeholder="Find: conf. no., name, job, education, state, country, place" value="' + esc(q) + '" style="margin:6px 0">';
    var list = mine.filter(function (p) { return (show === 'all' || (show === 'placed') === (p.place !== '')) && hit(p); });
    if (q) h += '<small>' + list.length + ' found</small>';
    list.forEach(function (p) {
      h += '<div class="dhrb-card' + (chosen === p.a_id ? ' on' : '') + '" data-a="' + p.a_id + '"><b>' + labelHtml(p) + '</b>' + (p.place ? ' <span class="dhr-chip">' + esc(where(p)) + '</span>' : '')
        + '<br><small>' + esc(details(p)) + '</small><br>' + hintsHtml(p) + '</div>';
    });
    if (!list.length) h += '<p><small>' + (q ? 'Nobody found.' : (show === 'waiting' ? 'Everyone is placed.' : 'Nobody here yet.')) + '</small></p>';
    h += '</div><div><div class="dhr-bar"><select class="form-control dhrb-ft" style="width:auto"><option value="">All toilets</option>'
      + Object.keys(D.toilets).map(function (k) { return '<option value="' + esc(k) + '"' + (k === fToilet ? ' selected' : '') + '>' + esc(D.toilets[k]) + '</option>'; }).join('')
      + '</select><select class="form-control dhrb-fp" style="width:auto">' + FILTERS.map(function (f) { return '<option value="' + f[0] + '"' + (f[0] === fPlace ? ' selected' : '') + '>' + f[1] + '</option>'; }).join('') + '</select>'
      + '<button class="btn btn-default btn-sm dhrb-openall">Open all</button><button class="btn btn-default btn-sm dhrb-closeall">Close all</button>'
      + '<button class="btn btn-default btn-sm dhrb-print">Print</button></div>';
    var g = bySection(rooms), filtered = fPlace !== 'all' || fToilet, shownSecs = 0;
    g.order.forEach(function (sec) {
      var rs = g.o[sec], n = 0, pn = 0, on = 0, match = 0;
      rs.forEach(function (r) { r.places.forEach(function (p) {
        var st = state(H, sec, p.name);
        n++;
        if (st === 'placed') pn++; else if (st === 'other') on++;
        if ((fPlace === 'all' || st === fPlace) && (!fToilet || r.toilet === fToilet)) match++;
      }); });
      if (filtered && !match) return;   // a filter hides sections with nothing to show
      shownSecs++;
      // Open only by the user (header, Open all / Close all) or once by a search (openHits): never forced, so a
      // search can no longer keep a section from closing (owner, 25 Nov 2026).
      var isOpen = !!open[side + '|' + sec];
      h += '<div class="dhr-sec"><div class="dhr-sec-head dhrb-sec" data-sec="' + esc(sec) + '"><b>' + esc(sec) + (rs[0].section_name ? ' <span class="dhr-secname">— ' + esc(rs[0].section_name) + '</span>' : '') + '</b>' + (rs[0].gender === 'B' ? '<span class="dhr-shared">Male + Female</span>' : '') + '<span class="dhrb-fill"><i class="p" style="width:' + (n ? 100 * pn / n : 0) + '%"></i><i class="o" style="width:' + (n ? 100 * on / n : 0) + '%"></i></span>'
        + '<small>' + pn + ' placed · ' + on + ' other course · ' + (n - pn - on) + ' free</small><span style="flex:1"></span>' + (isOpen ? '▾' : '▸') + '</div>';
      if (isOpen) {
        h += '<div class="dhrb-rooms">';
        rs.forEach(function (r) {
          if (fToilet && r.toilet !== fToilet) return;
          var pl = r.places.map(function (p) {
            var who = H[key(sec, p.name)], st = state(H, sec, p.name), attrs = ' data-sec="' + esc(sec) + '" data-place="' + esc(p.name) + '"';
            if (fPlace !== 'all' && st !== fPlace) return '';
            if (who) return '<div class="dhrb-pl taken' + (who.some(function (x) { return chosen === x.a_id; }) ? ' on' : '') + (q && anyHit(who) ? ' hit' : '') + '" title="' + esc(who.map(function (x) { return label(x) + ' · ' + details(x); }).join(' / ')) + '"' + attrs + ' data-a="' + who[0].a_id + '"><span>' + esc(p.name) + '</span><span>' + esc(who.map(label).join(', ')) + (who.length > 1 ? ' <span class="dhr-t">' + who.length + ' people</span>' : '') + '</span></div>';
            if (st === 'placed') return '<div class="dhrb-pl taken xside"' + attrs + ' data-x="' + esc(otherSide(sec, p.name)) + '"><span>' + esc(p.name) + '</span><span>' + esc(otherSide(sec, p.name)) + ' · this course</span></div>';
            if (st === 'other') return '<div class="dhrb-pl other"' + attrs + '><span>' + esc(p.name) + '</span><span>' + esc(otherCourse(sec, p.name)) + '</span></div>';
            return '<div class="dhrb-pl free"' + attrs + '><span>' + esc(p.name) + '</span><span>Free</span></div>';
          }).join('');
          if (!pl) return;
          h += '<div class="dhrb-room"><b>' + esc(r.name) + '</b> <span class="dhr-t dhr-t-' + esc(r.toilet) + '">' + esc(D.toilets[r.toilet]) + '</span>' + pl + '</div>';
        });
        h += '</div>';
      }
      h += '</div>';
    });
    if (filtered && !shownSecs) h += '<p><small>No places match this filter.</small></p>';
    h += '</div></div></div>';
    var peopleTop = $('#dh-rb-app .dhrb-people').scrollTop() || 0;   // the people list scrolls on its own (computer)
    $('#dh-rb-app').html(h);
    $('#dh-rb-app .dhrb-people').scrollTop(peopleTop);
    window.dhRoomsCommon.viewSave('room-board:' + S.dataUrl, side, open);
  }

  // Print every section of this side, with the filters in use (rooms-common.js boardPrint).
  function printBoard() { window.dhRoomsCommon.boardPrint(D, side, fPlace, fToilet); }

  // Every board action (place, move, swap, take out) posts here and redraws from the returned data.
  function save(payload) {
    if (D.finalized) return;
    busy = true;
    $.post(S.saveUrl, $.extend({token: S.token}, payload), null, 'json').done(function (r) {
      if (r.confirm) { busy = false; if (confirm(r.msg)) save($.extend(payload, {confirm: 1})); return; }
      if (r.data) { D = r.data; loadedAt = new Date(); }
      if (!r.ok) alert(r.msg); else chosen = null;
      render();
    }).fail(function () { alert('Could not save. Please check the connection and try again.'); })
      .always(function () { busy = false; lastAct = Date.now(); });
  }

  function updatedText() { return loadedAt ? 'Updated ' + loadedAt.toTimeString().slice(0, 5) + ' \u00b7 the board refreshes by itself' : ''; }

  // Self-refresh: arrivals and rooms given in Zero Day (or by another person) show without a reload, every 30 s and
  // when the tab is shown again. Never while someone is working on the board: not during a save, not within 10 s of
  // a click, key or touch in it, not while the search box or a filter has the cursor; it tries again a bit later.
  // A redraw keeps the side, open sections, chosen person, search, filters and scroll position; no change = no redraw.
  function quiet() {
    var a = document.activeElement;
    var typing = a && /^(INPUT|SELECT|TEXTAREA)$/.test(a.tagName) && $(a).closest('#dh-rb-app').length > 0;
    return !busy && Date.now() - lastAct > 10000 && !typing;
  }
  function refresh() {
    if (!D || document.hidden) return;
    if (!quiet()) { clearTimeout(retryT); retryT = setTimeout(refresh, 11000); return; }
    $.getJSON(S.dataUrl).done(function (d) {
      if (!quiet()) { clearTimeout(retryT); retryT = setTimeout(refresh, 11000); return; }   // they started working meanwhile
      loadedAt = new Date();
      if (JSON.stringify(d) === JSON.stringify(D)) { $('#dh-rb-app .dhrb-updated').text(updatedText()); return; }
      D = d;
      if (chosen && !person(chosen)) chosen = null;
      render(true);   // keep what is on the screen where it is
    });
  }

  function setTop() { return window.dhRoomsCommon.fixedTop('dh-rb-app'); }

  $(function () {
    S = Drupal.settings.dhRoomBoard;
    setTop();
    var rt = null;
    $(window).on('resize', function () {
      clearTimeout(rt);
      rt = setTimeout(function () {
        setTop();
        var nw = window.innerWidth < 700;
        if (nw !== narrow) { narrow = nw; if (D) render(); }
      }, 200);
    });
    // A new '#male' / '#female:AT,B' typed in the address bar (or a link to one): show that view.
    $(window).on('hashchange', function () {
      if (!D) return;
      var v = window.dhRoomsCommon.viewLoad('room-board:' + S.dataUrl);
      if (v && D.sides && D.sides.indexOf(v.side) > -1 && v.side !== side) { side = v.side; chosen = null; }
      if (v && v.open) open = v.open;
      render();
    });
    $.getJSON(S.dataUrl).done(function (d) {
      D = d;
      loadedAt = new Date();
      // Back to the side and open sections this person had before the reload.
      var v = window.dhRoomsCommon.viewLoad('room-board:' + S.dataUrl);
      if (v && D.sides && D.sides.indexOf(v.side) > -1) side = v.side;
      if (v && v.open) open = v.open;
      render();
    }).fail(function () { $('#dh-rb-app').html('<p>Could not load the board. Please reload the page.</p>'); });
    var doc = $(document);
    doc.on('mousedown keydown touchstart', '#dh-rb-app', function () { lastAct = Date.now(); });
    // What was tapped last, for keep-in-place (capture phase: noted before the click handlers redraw the board).
    ['mousedown', 'touchstart', 'focusin', 'click'].forEach(function (ev) {
      document.addEventListener(ev, function (e) {
        var t = e.target && e.target.closest ? e.target.closest(KEYED) : null;
        if (t && t.closest('#dh-rb-app') && keyOf(t)) lastKey = keyOf(t);
      }, true);
    });
    setInterval(refresh, 30000);
    document.addEventListener('visibilitychange', function () { if (!document.hidden) refresh(); });
    doc.on('click', '#dh-rb-app .dhr-tab[data-side]', function () { side = $(this).data('side'); chosen = null; render(); });
    doc.on('click', '.dhrb-sec', function () { var k = side + '|' + $(this).attr('data-sec'); open[k] = !open[k]; render(); });
    doc.on('input', '.dhrb-q', function () { q = $(this).val(); var pos = this.selectionStart; openHits(); render(); var i = $('.dhrb-q')[0]; i.focus(); i.setSelectionRange(pos, pos); });
    doc.on('change', '.dhrb-ft', function () { fToilet = $(this).val(); render(); });
    // Place filter (list or count boxes): a filter other than "All places" opens every section so the places show.
    var setFilter = function (f) {
      fPlace = f;
      if (f !== 'all') bySection(D.rooms.filter(onSide)).order.forEach(function (sec) { open[side + '|' + sec] = true; });
      render();
    };
    doc.on('change', '.dhrb-fp', function () { setFilter($(this).val()); });
    doc.on('click', '.dhrb-count[data-f]', function () { setFilter(String($(this).attr('data-f'))); });
    doc.on('click', '.dhrb-openall', function () { bySection(D.rooms.filter(onSide)).order.forEach(function (sec) { open[side + '|' + sec] = true; }); render(); });
    doc.on('click', '.dhrb-closeall', function () { Object.keys(open).forEach(function (k) { if (k.indexOf(side + '|') === 0) open[k] = false; }); render(); });
    doc.on('click', '.dhrb-print', function () { printBoard(); });
    doc.on('click', '.dhrb-card', function () {
      var a = $(this).data('a'), p = person(a);
      if (!p) return;
      chosen = chosen === a ? null : a;
      if (chosen && p.place) open[side + '|' + p.section] = true;   // a placed person: open their section so the place shows
      render();
    });
    doc.on('click', '.dhrb-unchoose', function () { chosen = null; render(); });
    // Phone: jump from the people list down to the rooms, just below the pinned bar.
    doc.on('click', '.dhrb-torooms', function () {
      var col = $('.dhrb-main > div').eq(1)[0], pin = $('.dhrb-pin')[0];
      if (!col) return;
      window.scrollTo(0, col.getBoundingClientRect().top + window.pageYOffset - setTop() - (pin ? pin.offsetHeight : 0) - 6);
    });
    doc.on('click', '.dhrb-show', function () { show = $(this).data('show'); render(); });
    // Room changes log: open / close (click, or Enter / Space on the heading).
    doc.on('click keydown', '.dhrb-loghead', function (e) {
      if (e.type === 'keydown' && e.key !== 'Enter' && e.key !== ' ') return;
      e.preventDefault();
      logOpen = !logOpen;
      renderLog();
      if (logOpen && logDT) logDT.columns.adjust();
    });
    doc.on('click', '.dhrb-remove', function () {
      var c = person(chosen);
      if (!c || !sure('Take ' + label(c) + ' out of ' + roomText(c.section, c.place) + '?\n\nThe place becomes free.')) return;
      save({op: 'remove', a_id: chosen});
    });
    doc.on('click', '.dhrb-pl', function () {
      var el = $(this), sec = String(el.attr('data-sec')), place = String(el.attr('data-place')), who = el.data('a');
      if (el.hasClass('xside')) { alert('Place ' + place + ' is taken by ' + (el.attr('data-x') === 'Female' ? 'a female student' : 'a male student') + ' in this course.'); return; }
      if (who) {
        if (!chosen || chosen === who) { chosen = chosen === who ? null : who; render(); return; }
        var c = person(chosen);
        if (c.section === sec && c.place === place) { chosen = null; render(); return; }
        if (c.place === '') { alert('This place is taken. Choose a free place for ' + c.name + ', or take the other person out of the room first.'); return; }
        var o = person(who), a = roomText(c.section, c.place), b = roomText(o.section, o.place);
        if (!sure('Swap these two?\n\n' + label(c) + ': ' + a + ' → ' + b + '\n' + label(o) + ': ' + b + ' → ' + a)) return;
        save({op: 'swap', a_id: chosen, other: who});
        return;
      }
      if (!chosen) { alert('First tap a person, then tap a free place.'); return; }
      var p = person(chosen), to = roomText(sec, place);
      if (!sure(p.place ? 'Move ' + label(p) + ' from ' + roomText(p.section, p.place) + ' to ' + to + '?' : 'Place ' + label(p) + ' in ' + to + '?')) return;
      save({op: 'place', a_id: chosen, section: sec, place: place});
    });
  });
})(jQuery);
