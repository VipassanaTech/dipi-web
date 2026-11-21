/* Rooms page: a centre's sections → rooms → places, with toilet type. Loads Drupal.settings.dhRooms.dataUrl;
 * every change POSTs {op, token, ...} to saveUrl and redraws from the rooms it returns. */
(function ($) {
  'use strict';
  var S, rooms = [], oldList = false, inUse = {}, side = 'M', open = {}, picked = {}, q = '', fToilet = '', onlyReview = false, lastMsg = '';

  function esc(s) { return $('<div/>').text(s == null ? '' : String(s)).html().replace(/"/g, '&quot;').replace(/'/g, '&#39;'); }
  function badge(t) { return '<span class="dhr-t dhr-t-' + esc(t) + '">' + esc(S.toilets[t] || t) + '</span>'; }
  function needsLook(r) { return r.review || r.toilet === 'unset'; }
  // A shared section (gender "B") is used by men and women, so it shows on both tabs.
  function onSide(r, s) { return r.gender === s || r.gender === 'B'; }
  function sideRooms() { return rooms.filter(function (r) { return onSide(r, side); }); }
  function otherSide() { return side === 'M' ? 'F' : 'M'; }
  var WHO = {M: 'men', F: 'women', B: 'men and women'};
  function places(list) { return list.reduce(function (n, r) { return n + r.places.length; }, 0); }
  function n(count, word) { return count + ' ' + word + (count === 1 ? '' : 's'); }
  function matches(r) {
    if (fToilet && r.toilet !== fToilet) return false;
    if (onlyReview && !needsLook(r)) return false;
    if (!q) return true;
    var t = q.toLowerCase();
    return r.name.toLowerCase().indexOf(t) >= 0 || r.places.some(function (p) { return p.name.toLowerCase().indexOf(t) >= 0; });
  }
  function bySection(list) {
    var o = {}, order = [];
    list.forEach(function (r) { if (!o[r.section]) { o[r.section] = []; order.push(r.section); } o[r.section].push(r); });
    return {o: o, order: order};
  }
  function toiletOptions(sel, withAll) {
    var h = withAll ? '<option value="">All toilets</option>' : '';
    $.each(S.toilets, function (k, v) { h += '<option value="' + esc(k) + '"' + (k === sel ? ' selected' : '') + '>' + esc(v) + '</option>'; });
    return h;
  }

  function render() {
    var men = rooms.filter(function (r) { return onSide(r, 'M'); }), women = rooms.filter(function (r) { return onSide(r, 'F'); });
    var list = sideRooms(), look = list.filter(needsLook).length, nPicked = Object.keys(picked).length;
    var h = '<div class="dhr-tabs"><button class="dhr-tab' + (side === 'M' ? ' on' : '') + '" data-side="M">Men · ' + places(men) + ' places</button>'
      + '<button class="dhr-tab' + (side === 'F' ? ' on' : '') + '" data-side="F">Women · ' + places(women) + ' places</button></div>';
    if (look) h += '<div class="dhr-note">' + (look === 1 ? '1 room needs' : look + ' rooms need') + ' a look: toilet "Not set", or added from a course. '
      + '<a href="#" class="dhr-only">' + (onlyReview ? 'Show all rooms' : 'Show only these') + '</a></div>';
    h += '<div class="dhr-bar"><input type="text" class="form-control dhr-q" placeholder="Find room or place, e.g. 112B" value="' + esc(q) + '">'
      + '<select class="form-control dhr-ft" style="width:auto">' + toiletOptions(fToilet, true) + '</select></div>';
    h += '<div class="dhr-bar"><button class="btn btn-default dhr-add">Add room</button><button class="btn btn-default dhr-many">Add many rooms</button>'
      + (nPicked ? '<select class="form-control dhr-set" style="width:auto"><option value="">Set toilet for ' + nPicked + ' selected…</option>' + toiletOptions('', false) + '</select>'
        + (nPicked > 1 ? '<button class="btn btn-default dhr-join">Join into one room</button>' : '')
        + (nPicked === 1 ? '<button class="btn btn-default dhr-edit">Edit</button><button class="btn btn-default dhr-split">Split</button><button class="btn btn-default dhr-del">Remove</button>' : '')
        + '<button class="btn btn-default dhr-ok">Mark as checked</button><button class="btn btn-link dhr-none">Clear selection</button>' : '')
      + '</div><div class="dhr-formbox"></div>';
    var g = bySection(list);
    if (!g.order.length) {
      if (!rooms.length && oldList) h += '<div class="dhr-note">This centre still uses its old room list. Bring it in here to see and edit its rooms.</div><p><button class="btn btn-primary dhr-import">Bring in the old room list</button></p>';
      else h += '<p>No rooms yet. Use "Add room" or "Add many rooms".</p>';
    }
    g.order.forEach(function (sec) {
      var rs = g.o[sec], shown = rs.filter(matches), isOpen = open[side + '|' + sec] || (q && shown.length), used = rs[0].gender;
      if ((q || fToilet || onlyReview) && !shown.length) return;
      var counts = {};
      rs.forEach(function (r) { counts[r.toilet] = (counts[r.toilet] || 0) + 1; });
      var sum = Object.keys(counts).map(function (t) { return '<span class="dhr-t dhr-t-' + esc(t) + '">' + esc(S.toilets[t]) + ' ' + counts[t] + '</span>'; }).join(' ');
      h += '<div class="dhr-sec"><div class="dhr-sec-head" data-sec="' + esc(sec) + '"><b>' + esc(sec) + '</b>' + (used === 'B' ? '<span class="dhr-shared">Men + Women</span>' : '')
        + '<span>' + n(rs.length, 'room') + ' · ' + n(places(rs), 'place') + '</span>'
        + sum + '<span style="flex:1"></span><a href="#" class="dhr-ren" data-sec="' + esc(sec) + '">Rename</a> <span>' + (isOpen ? '▾' : '▸') + '</span></div>';
      if (isOpen) {
        h += '<div class="dhr-bar" style="padding:8px 10px 0"><label style="font-weight:normal">Used by <select class="form-control dhr-used" data-sec="' + esc(sec) + '" style="width:auto;display:inline-block">'
          + [['M', 'Men only'], ['F', 'Women only'], ['B', 'Men and Women']].map(function (o) { return '<option value="' + o[0] + '"' + (o[0] === used ? ' selected' : '') + '>' + o[1] + '</option>'; }).join('')
          + '</select></label><select class="form-control dhr-setsec" data-sec="' + esc(sec) + '" style="width:auto"><option value="">Set toilet for whole section…</option>'
          + toiletOptions('', false) + '</select><button class="btn btn-default btn-sm dhr-manyhere" data-sec="' + esc(sec) + '">Add rooms here</button>'
          + '<button class="btn btn-default btn-sm dhr-delsec" data-sec="' + esc(sec) + '">Delete section</button></div><div class="dhr-tiles">';
        shown.forEach(function (r) {
          h += '<div class="dhr-tile' + (picked[r.id] ? ' sel' : '') + (needsLook(r) ? ' rev' : '') + '" data-id="' + r.id + '"><b>' + esc(r.name) + '</b><br>'
            + r.places.map(function (p) { return '<span class="dhr-chip">' + esc(p.name) + '</span>'; }).join('') + '<br>' + badge(r.toilet) + '</div>';
        });
        h += '</div>';
      }
      h += '</div>';
    });
    $('#dh-rooms-app').html(h);
  }

  function post(op, payload, after) {
    $.post(S.saveUrl, $.extend({op: op, token: S.token, gender: side}, payload), null, 'json')
      .done(function (r) {
        if (r.rooms) rooms = r.rooms;
        if (typeof r.old_list !== 'undefined') oldList = !!r.old_list;
        if (r.in_use) { inUse = {}; r.in_use.forEach(function (k) { inUse[k] = 1; }); }
        if (!r.ok) { alert(r.msg); render(); return; }
        lastMsg = r.msg; picked = {}; render(); if (after) after();
      })
      .fail(function () { alert('Could not save. Please check the connection and try again.'); });
  }

  function roomForm(r) {
    var sec = r ? r.section : (Object.keys(open).filter(function (k) { return open[k] && k.indexOf(side + '|') === 0; }).map(function (k) { return k.slice(2); })[0] || '');
    var h = '<div class="dhr-form"><b>' + (r ? 'Edit room ' + esc(r.name) : 'Add a room') + '</b>'
      + '<label>Section<input type="text" class="form-control f-sec" maxlength="10" value="' + esc(sec) + '"></label>'
      + '<label>Room name<input type="text" class="form-control f-name" maxlength="40" value="' + esc(r ? r.name : '') + '"></label>'
      + '<label>Places in this room (separate with commas)<input type="text" class="form-control f-places" value="' + esc(r ? r.places.map(function (p) { return p.name; }).join(', ') : '') + '"></label>'
      + '<label>Toilet<select class="form-control f-toilet">' + toiletOptions(r ? r.toilet : 'unset', false) + '</select></label>'
      + '<p><button class="btn btn-primary f-save">Save</button> <button class="btn btn-default f-cancel">Cancel</button></p></div>';
    $('.dhr-formbox').html(h).find('.f-save').on('click', function () {
      var f = $(this).closest('.dhr-form');
      post('save_room', {id: r ? r.id : 0, section: f.find('.f-sec').val(), name: f.find('.f-name').val(), toilet: f.find('.f-toilet').val(),
        places: f.find('.f-places').val().split(',').map(function (s) { return $.trim(s); }).filter(Boolean)});
    });
  }

  function manyForm(sec) {
    var h = '<div class="dhr-form"><b>Add many rooms</b>'
      + '<label>Section<input type="text" class="form-control f-sec" maxlength="10" value="' + esc(sec || '') + '"></label>'
      + '<label>Rooms from<input type="number" class="form-control f-from" min="0"></label><label>to<input type="number" class="form-control f-to" min="0"></label>'
      + '<label>Places in each room, e.g. A, B (leave empty for one place per room)<input type="text" class="form-control f-suf"></label>'
      + '<label>Toilet<select class="form-control f-toilet">' + toiletOptions('unset', false) + '</select></label>'
      + '<p><button class="btn btn-primary f-save">Add</button> <button class="btn btn-default f-cancel">Cancel</button></p></div>';
    $('.dhr-formbox').html(h).find('.f-save').on('click', function () {
      var f = $(this).closest('.dhr-form');
      post('add_many', {section: f.find('.f-sec').val(), from: f.find('.f-from').val(), to: f.find('.f-to').val(), toilet: f.find('.f-toilet').val(),
        suffixes: f.find('.f-suf').val().split(',').map(function (s) { return $.trim(s); }).filter(Boolean)});
    });
  }

  function pickedIds() { return Object.keys(picked).map(Number); }
  function roomById(id) { return rooms.filter(function (r) { return r.id === id; })[0]; }

  $(function () {
    S = Drupal.settings.dhRooms;
    $.getJSON(S.dataUrl).done(function (d) { rooms = d.rooms; oldList = !!d.old_list; d.in_use.forEach(function (k) { inUse[k] = 1; }); render(); })
      .fail(function () { $('#dh-rooms-app').html('<p>Could not load the rooms. Please reload the page.</p>'); });
    var app = $(document);
    app.on('click', '.dhr-tab', function () { side = $(this).data('side'); picked = {}; render(); });
    app.on('click', '.dhr-only', function (e) { e.preventDefault(); onlyReview = !onlyReview; render(); });
    app.on('input', '.dhr-q', function () { q = $(this).val(); var pos = this.selectionStart; render(); var i = $('.dhr-q')[0]; i.focus(); i.setSelectionRange(pos, pos); });
    app.on('change', '.dhr-ft', function () { fToilet = $(this).val(); render(); });
    app.on('click', '.dhr-sec-head', function (e) { if ($(e.target).hasClass('dhr-ren')) return; var k = side + '|' + $(this).attr('data-sec'); open[k] = !open[k]; render(); });
    app.on('click', '.dhr-tile', function () { var id = $(this).data('id'); if (picked[id]) delete picked[id]; else picked[id] = 1; render(); });
    app.on('click', '.dhr-import', function () { if (confirm('Bring in the old room list now?')) post('import_old', {}, function () { if (lastMsg) alert(lastMsg); }); });
    app.on('click', '.dhr-none', function () { picked = {}; render(); });
    app.on('click', '.dhr-add', function () { roomForm(null); });
    app.on('click', '.dhr-edit', function () { roomForm(roomById(pickedIds()[0])); });
    app.on('click', '.dhr-many', function () { manyForm(''); });
    app.on('click', '.dhr-manyhere', function () { manyForm($(this).attr('data-sec')); });
    app.on('click', '.f-cancel', function () { $('.dhr-formbox').empty(); });
    app.on('change', '.dhr-set', function () { if ($(this).val()) post('set_toilet', {ids: pickedIds(), toilet: $(this).val()}); });
    app.on('change', '.dhr-setsec', function () {
      var sec = $(this).attr('data-sec'), t = $(this).val();
      if (!t) return;
      var ids = sideRooms().filter(function (r) { return r.section === String(sec); }).map(function (r) { return r.id; });
      if (confirm('Set toilet "' + S.toilets[t] + '" for all ' + ids.length + ' rooms of section ' + sec + '?')) post('set_toilet', {ids: ids, toilet: t}); else render();
    });
    app.on('click', '.dhr-join', function () { if (confirm('Join the ' + pickedIds().length + ' selected rooms into one room?')) post('join', {ids: pickedIds()}); });
    app.on('click', '.dhr-split', function () { post('split', {id: pickedIds()[0]}); });
    app.on('click', '.dhr-del', function () { var r = roomById(pickedIds()[0]); if (confirm('Remove room ' + r.name + '?')) post('remove', {id: r.id}); });
    app.on('click', '.dhr-ok', function () { post('clear_review', {ids: pickedIds()}); });
    app.on('change', '.dhr-used', function () {
      var sec = String($(this).attr('data-sec')), to = $(this).val(), oth = otherSide();
      var twin = rooms.some(function (r) { return r.gender === oth && r.section.toUpperCase() === sec.toUpperCase(); });
      var ask = (to === 'B' && twin)
        ? ({M: 'Men', F: 'Women'})[oth] + ' also have a section ' + sec + '. Join them into one section for men and women? The ' + WHO[oth] + "'s rooms of " + sec + ' will be replaced by these rooms.'
        : 'Section ' + sec + ' will be used by ' + WHO[to] + '. OK?';
      if (confirm(ask)) post('set_used_by', {section: sec, to: to}, function () { if (lastMsg) alert(lastMsg); }); else render();
    });
    app.on('click', '.dhr-delsec', function () {
      var sec = String($(this).attr('data-sec')), rs = sideRooms().filter(function (r) { return r.section === sec; });
      if (confirm('Delete section ' + sec + ': ' + n(rs.length, 'room') + ', ' + n(places(rs), 'place') + '?' + (rs.length && rs[0].gender === 'B' ? ' It is used by men and women.' : '')))
        post('delete_section', {section: sec}, function () { if (lastMsg) alert(lastMsg); });
    });
    app.on('click', '.dhr-ren', function (e) {
      e.preventDefault();
      var from = String($(this).attr('data-sec')), to = prompt('New name for section ' + from + ':', from);
      if (to && $.trim(to) !== from) post('rename_section', {from: from, to: $.trim(to)});
    });
  });
})(jQuery);
