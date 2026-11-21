/* Room Board of a course: sections → rooms → places, who is where, the people not placed yet.
 * Tap a person, then a free place to place them; tap a placed person, then another place to move,
 * or another placed person to swap. Every action POSTs to saveUrl and redraws from the returned data. */
(function ($) {
  'use strict';
  // narrow = phone width (people list above the rooms): the pinned "Chosen" bar gets a button down to the rooms.
  var S, D = null, side = null, open = {}, chosen = null, q = '', fToilet = '', freeOnly = false, show = 'waiting';
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
  // A place used by someone else: another course at the same time, or (in a shared section) the other side of this course.
  function other(sec, place) {
    var o = D.overlap[side] || {}, x = (D.cross && D.cross[side]) || {};
    return (o[sec] && o[sec][place]) || (x[sec] && x[sec][place]) || '';
  }
  function onSide(r) { return r.gender === side || r.gender === 'B'; }
  // One line of what centres look at when giving a room: Old/New/Server, age, job, education, state + country, a different nationality.
  function details(p) {
    var where = [p.state, p.country].filter(Boolean).join(', ');
    return [p.server ? 'Server' : (p.old ? 'Old' : 'New'), p.age !== null ? p.age + ' yrs' : '', p.occupation, p.education, where,
      p.nationality ? 'Nationality: ' + p.nationality : ''].filter(Boolean).join(' · ');
  }
  function hintsHtml(p) { return p.hints.map(function (x) { return '<span class="dhrb-hint">' + esc(x) + '</span>'; }).join(''); }
  // Where a person is, with the section (many sections have a place 2A): "R · 2A".
  function where(p) { return p.place ? (p.section ? p.section + ' · ' + p.place : p.place) : ''; }
  // Search: name, job, education, state, country, nationality or place ("2A", "R 2A", "R-2A" or "R · 2A").
  function hit(p) {
    if (!q) return true;
    var t = q.toLowerCase();
    return [p.name, p.occupation, p.education, p.state, p.country, p.nationality, p.place, p.place && where(p), p.place && p.section + ' ' + p.place, p.place && p.section + '-' + p.place]
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
    var total = 0, placed = 0, otherN = 0, shared = 0, usedPlaces = 0;
    rooms.forEach(function (r) { r.places.forEach(function (p) {
      var w = H[key(r.section, p.name)];
      total++;
      if (w) { placed += w.length; usedPlaces++; if (w.length > 1) shared++; } else if (other(r.section, p.name)) otherN++;
    }); });
    var waiting = D.people.filter(function (p) { return p.gender === side && p.place === ''; });
    var h = '';
    if (D.finalized) h += '<div class="dhr-note">This course is finalized. The board is read-only.</div>';
    if (D.sides.length > 1) h += '<div class="dhr-tabs">' + D.sides.map(function (s) { return '<button class="dhr-tab' + (s === side ? ' on' : '') + '" data-side="' + esc(s) + '">' + (s === 'M' ? 'Men' : 'Women') + '</button>'; }).join('') + '</div>';
    h += '<div class="dhrb-counts"><div class="dhrb-count">Places<b>' + total + '</b></div><div class="dhrb-count">Placed<b>' + placed + '</b></div>'
      + '<div class="dhrb-count">Used by others<b>' + otherN + '</b></div><div class="dhrb-count">Free<b>' + (total - usedPlaces - otherN) + '</b></div></div>';
    if (shared) h += '<div class="dhr-note">' + (shared === 1 ? '1 place has' : shared + ' places have') + ' more than one person.</div>';
    // The chosen person sits in a bar over both columns that stays under the site header while the board scrolls.
    h += '<div class="dhrb-wrap">';
    var c = chosen ? person(chosen) : null;
    if (c) h += '<div class="dhrb-chosen dhrb-pin"><b>Chosen: ' + esc(c.name) + '</b><small class="dhrb-det">' + esc(details(c)) + '</small>' + hintsHtml(c)
      + '<small class="dhrb-det">' + (c.place ? 'In place ' + esc(c.place) + ' (Room ' + esc(roomOf(c)) + ', section ' + esc(c.section) + '). Tap a free place to move, or a person to swap.' : 'Not placed yet. Tap a free place.') + '</small>'
      + '<p style="margin:6px 0 0">' + (narrow ? '<button class="btn btn-primary btn-sm dhrb-torooms">Find a place ↓</button> ' : '')
      + (c.place ? '<button class="btn btn-default btn-sm dhrb-remove">Take out of the room</button> ' : '') + '<button class="btn btn-link btn-sm dhrb-unchoose">Clear</button></p></div>';
    h += '<div class="dhrb-main"><div>';
    var mine = D.people.filter(function (p) { return p.gender === side; }), placedN = mine.length - waiting.length;
    h += '<div class="dhr-tabs">' + [['waiting', 'Not placed · ' + waiting.length], ['placed', 'Placed · ' + placedN], ['all', 'All · ' + mine.length]].map(function (t) {
      return '<button class="dhr-tab dhrb-show' + (show === t[0] ? ' on' : '') + '" data-show="' + t[0] + '">' + t[1] + '</button>'; }).join('') + '</div>'
      + (D.optional ? '<small>Room is optional on this course.</small>' : '')
      + '<input type="text" class="form-control dhrb-q" placeholder="Find: name, job, education, state, country, place" value="' + esc(q) + '" style="margin:6px 0">';
    var list = mine.filter(function (p) { return (show === 'all' || (show === 'placed') === (p.place !== '')) && hit(p); });
    if (q) h += '<small>' + list.length + ' found</small>';
    list.forEach(function (p) {
      h += '<div class="dhrb-card' + (chosen === p.a_id ? ' on' : '') + '" data-a="' + p.a_id + '"><b>' + esc(p.name) + '</b>' + (p.place ? ' <span class="dhr-chip">' + esc(where(p)) + '</span>' : '')
        + '<br><small>' + esc(details(p)) + '</small><br>' + hintsHtml(p) + '</div>';
    });
    if (!list.length) h += '<p><small>' + (q ? 'Nobody found.' : (show === 'waiting' ? 'Everyone is placed.' : 'Nobody here yet.')) + '</small></p>';
    h += '</div><div><div class="dhr-bar"><select class="form-control dhrb-ft" style="width:auto"><option value="">All toilets</option>'
      + Object.keys(D.toilets).map(function (k) { return '<option value="' + esc(k) + '"' + (k === fToilet ? ' selected' : '') + '>' + esc(D.toilets[k]) + '</option>'; }).join('')
      + '</select><label style="font-weight:normal"><input type="checkbox" class="dhrb-free"' + (freeOnly ? ' checked' : '') + '> Free only</label></div>';
    var order = [], bySec = {};
    rooms.forEach(function (r) { if (!bySec[r.section]) { bySec[r.section] = []; order.push(r.section); } bySec[r.section].push(r); });
    order.forEach(function (sec) {
      var rs = bySec[sec], n = 0, pn = 0, on = 0;
      rs.forEach(function (r) { r.places.forEach(function (p) { n++; if (H[key(sec, p.name)]) pn++; else if (other(sec, p.name)) on++; }); });
      var isOpen = open[side + '|' + sec] || (q && rs.some(function (r) { return r.places.some(function (p) { var w = H[key(sec, p.name)]; return w && anyHit(w); }); }));
      h += '<div class="dhr-sec"><div class="dhr-sec-head dhrb-sec" data-sec="' + esc(sec) + '"><b>' + esc(sec) + '</b>' + (rs[0].gender === 'B' ? '<span class="dhr-shared">Men + Women</span>' : '') + '<span class="dhrb-fill"><i class="p" style="width:' + (n ? 100 * pn / n : 0) + '%"></i><i class="o" style="width:' + (n ? 100 * on / n : 0) + '%"></i></span>'
        + '<small>' + pn + ' placed · ' + on + ' used by others · ' + (n - pn - on) + ' free</small><span style="flex:1"></span>' + (isOpen ? '▾' : '▸') + '</div>';
      if (isOpen) {
        h += '<div class="dhrb-rooms">';
        rs.forEach(function (r) {
          if (fToilet && r.toilet !== fToilet) return;
          var pl = r.places.map(function (p) {
            var who = H[key(sec, p.name)], o = other(sec, p.name);
            if (freeOnly && (who || o)) return '';
            if (who) return '<div class="dhrb-pl taken' + (who.some(function (x) { return chosen === x.a_id; }) ? ' on' : '') + (q && anyHit(who) ? ' hit' : '') + '" title="' + esc(who.map(details).join(' / ')) + '" data-sec="' + esc(sec) + '" data-place="' + esc(p.name) + '" data-a="' + who[0].a_id + '"><span>' + esc(p.name) + '</span><span>' + esc(who.map(function (x) { return x.name; }).join(', ')) + (who.length > 1 ? ' <span class="dhr-t">' + who.length + ' people</span>' : '') + '</span></div>';
            if (o) return '<div class="dhrb-pl other" data-sec="' + esc(sec) + '" data-place="' + esc(p.name) + '"><span>' + esc(p.name) + '</span><span>' + esc(o) + '</span></div>';
            return '<div class="dhrb-pl free" data-sec="' + esc(sec) + '" data-place="' + esc(p.name) + '"><span>' + esc(p.name) + '</span><span>Free</span></div>';
          }).join('');
          if (!pl) return;
          h += '<div class="dhrb-room"><b>' + esc(r.name) + '</b> <span class="dhr-t dhr-t-' + esc(r.toilet) + '">' + esc(D.toilets[r.toilet]) + '</span>' + pl + '</div>';
        });
        h += '</div>';
      }
      h += '</div>';
    });
    h += '</div></div></div>';
    $('#dh-rb-app').html(h);
  }

  function save(payload) {
    if (D.finalized) return;
    $.post(S.saveUrl, $.extend({token: S.token}, payload), null, 'json').done(function (r) {
      if (r.confirm) { if (confirm(r.msg)) save($.extend(payload, {confirm: 1})); return; }
      if (r.data) D = r.data;
      if (!r.ok) alert(r.msg); else chosen = null;
      render();
    }).fail(function () { alert('Could not save. Please check the connection and try again.'); });
  }

  // The site header is fixed at the top, so the pinned bar sits just below it (CSS --dhr-top).
  function setTop() {
    var t = 0, vh = window.innerHeight;
    $('body').find('> *, > * > *, > * > * > *').each(function () {
      if (this.offsetHeight === 0 || window.getComputedStyle(this).position !== 'fixed') return;
      var b = this.getBoundingClientRect();
      if (b.top < vh / 3 && b.height < vh / 2) t = Math.max(t, b.bottom);
    });
    var app = document.getElementById('dh-rb-app');
    if (app) app.style.setProperty('--dhr-top', Math.round(t) + 'px');
    return t;
  }

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
    doc.on('change', '.dhrb-free', function () { freeOnly = this.checked; render(); });
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
