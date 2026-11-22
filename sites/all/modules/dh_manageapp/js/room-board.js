/* Room Board of a course: sections → rooms → places, who is where, the people not placed yet.
 * Tap a person, then a free place to place them; tap a placed person, then another place to move,
 * or another placed person to swap. Every action POSTs to saveUrl and redraws from the returned data. */
(function ($) {
  'use strict';
  // narrow = phone width (people list above the rooms): the pinned "Chosen" bar gets a button down to the rooms.
  // fPlace = the place filter: all | free | placed | other (another course at the same time).
  var S, D = null, side = null, open = {}, chosen = null, q = '', fToilet = '', fPlace = 'all', show = 'waiting';
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
  function roomOf(p) {
    var r = D.rooms.filter(function (r) { return (r.gender === p.gender || r.gender === 'B') && r.section === p.section && r.places.some(function (x) { return x.name === p.place; }); })[0];
    return r ? r.name : '';
  }

  function render() {
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
    h += '<div class="dhrb-counts">' + box('all', 'Places', total) + box('placed', 'Placed', placed) + box('other', 'Other course', otherN) + box('free', 'Free', freeN) + '</div>';
    if (shared) h += '<div class="dhr-note">' + (shared === 1 ? '1 place has' : shared + ' places have') + ' more than one person.</div>';
    // The chosen person sits in a bar over both columns that stays under the site header while the board scrolls.
    h += '<div class="dhrb-wrap">';
    var c = chosen ? person(chosen) : null;
    if (c) h += '<div class="dhrb-chosen dhrb-pin"><b>Chosen: ' + labelHtml(c) + '</b><small class="dhrb-det">' + esc(details(c)) + '</small>' + hintsHtml(c)
      + '<small class="dhrb-det">' + (c.place ? 'In place ' + esc(c.place) + ' (Room ' + esc(roomOf(c)) + ', section ' + esc(c.section) + '). Tap a free place to move, or a person to swap.' : 'Not placed yet. Tap a free place.') + '</small>'
      + '<p style="margin:6px 0 0">' + (narrow ? '<button class="btn btn-primary btn-sm dhrb-torooms">Find a place ↓</button> ' : '')
      + (c.place ? '<button class="btn btn-default btn-sm dhrb-remove">Take out of the room</button> ' : '') + '<button class="btn btn-link btn-sm dhrb-unchoose">Clear</button></p></div>';
    h += '<div class="dhrb-main"><div>';
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
      var isOpen = open[side + '|' + sec] || (q && rs.some(function (r) { return r.places.some(function (p) { var w = H[key(sec, p.name)]; return w && anyHit(w); }); }));
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
    $('#dh-rb-app').html(h);
  }

  // Print every section of this side, with the filters in use (rooms-common.js boardPrint).
  function printBoard() { window.dhRoomsCommon.boardPrint(D, side, fPlace, fToilet); }

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
    $.getJSON(S.dataUrl).done(function (d) { D = d; render(); }).fail(function () { $('#dh-rb-app').html('<p>Could not load the board. Please reload the page.</p>'); });
    var doc = $(document);
    doc.on('click', '#dh-rb-app .dhr-tab[data-side]', function () { side = $(this).data('side'); chosen = null; render(); });
    doc.on('click', '.dhrb-sec', function () { var k = side + '|' + $(this).attr('data-sec'); open[k] = !open[k]; render(); });
    doc.on('input', '.dhrb-q', function () { q = $(this).val(); var pos = this.selectionStart; render(); var i = $('.dhrb-q')[0]; i.focus(); i.setSelectionRange(pos, pos); });
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
    doc.on('click', '.dhrb-remove', function () { save({op: 'remove', a_id: chosen}); });
    doc.on('click', '.dhrb-pl', function () {
      var el = $(this), sec = String(el.attr('data-sec')), place = String(el.attr('data-place')), who = el.data('a');
      if (el.hasClass('xside')) { alert('Place ' + place + ' is taken by ' + (el.attr('data-x') === 'Female' ? 'a female student' : 'a male student') + ' in this course.'); return; }
      if (who) {
        if (!chosen || chosen === who) { chosen = chosen === who ? null : who; render(); return; }
        var c = person(chosen);
        if (c.section === sec && c.place === place) { chosen = null; render(); return; }
        if (c.place === '') { alert('This place is taken. Choose a free place for ' + c.name + ', or take the other person out of the room first.'); return; }
        save({op: 'swap', a_id: chosen, other: who});
        return;
      }
      if (!chosen) { alert('First tap a person, then tap a free place.'); return; }
      save({op: 'place', a_id: chosen, section: sec, place: place});
    });
  });
})(jQuery);
