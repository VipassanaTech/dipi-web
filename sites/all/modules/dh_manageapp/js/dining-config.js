/* Visual editor for the centre "Dining Configuration" — a table over the raw
   INI textarea (cs_dining_config), mirroring the Course Configuration editor.

   Rows = a "Default" block (always present) + group blocks added via a "+"
   dropdown; columns = Male / Female. Each block has:
     - Dining seats  (the combined range; greyed when "split" is on)
     - Keep aside    (held back from auto-assignment; manual use; no reverse)
     - Sevak         (servers - always old, one range; always shown)
     - split -> New / Old (student split; shown only when "split" is ticked)
   Every auto-assigned range cell has a small "reverse" toggle that writes a
   "<Key>Rev = 1" flag (engine assigns that range high->low). On every change the
   exact INI the engine reads is written back into the hidden textarea, so the
   backend (inc/allocate.inc) is unchanged. Vanilla-ish jQuery. */
(function ($) {
  var GEN = [{ k: 'M', ini: 'MALE', label: 'Male' }, { k: 'F', ini: 'FEMALE', label: 'Female' }];
  // row key -> INI key it maps to (in the student section, except 'sevak').
  var ROWS = {
    main:     { label: 'Dining seats',   key: 'Cells',    rev: true,  server: false, cls: 'dc-row-main' },
    reserved: { label: 'Keep aside',     key: 'Reserved', rev: false, server: false, cls: 'dc-row-res' },
    sevak:    { label: 'Sevak',          key: 'Cells',    rev: true,  server: true,  cls: 'dc-row-sevak' },
    New:      { label: 'New',            key: 'New',      rev: true,  server: false, cls: 'dc-row-split' },
    Old:      { label: 'Old',            key: 'Old',      rev: true,  server: false, cls: 'dc-row-split' }
  };

  function parseINI(text) {
    var out = {}, cur = null;
    (text || '').split(/\r?\n/).forEach(function (line) {
      line = line.trim();
      if (!line || line[0] === ';' || line[0] === '#') { return; }
      var m = line.match(/^\[(.+)\]$/);
      if (m) { cur = m[1].trim(); out[cur] = out[cur] || {}; return; }
      var eq = line.indexOf('=');
      if (cur && eq > 0) { out[cur][line.slice(0, eq).trim()] = line.slice(eq + 1).trim(); }
    });
    return out;
  }

  function secKey(genIni, gid, server) { return genIni + (server ? '_SERVER' : '') + (gid ? '_' + gid : ''); }
  function esc(s) { return String(s == null ? '' : s).replace(/&/g, '&amp;').replace(/"/g, '&quot;').replace(/</g, '&lt;'); }

  function blockHasData(ini, gid) {
    return GEN.some(function (g) {
      var s = ini[secKey(g.ini, gid, false)] || {}, sv = ini[secKey(g.ini, gid, true)] || {};
      return s.Cells || s.Old || s.New || s.Reserved || sv.Cells;
    });
  }
  function blockSplit(ini, gid) {
    return GEN.some(function (g) { var s = ini[secKey(g.ini, gid, false)] || {}; return s.Old || s.New; });
  }

  // one range cell: text input (+ optional reverse toggle)
  function cell(gid, row, gk, v, rev) {
    var r = ROWS[row];
    var h = '<td><div class="dc-cell"><input type="text" class="dc-in" data-gid="' + esc(gid) + '" data-row="' + row +
      '" data-g="' + gk + '" value="' + esc(v) + '" placeholder="e.g. 1-40, 55" size="12">';
    if (r.rev) {
      h += '<label class="dc-rev" title="assign this range high→low"><input type="checkbox" class="dc-revcb" data-gid="' +
        esc(gid) + '" data-row="' + row + '" data-g="' + gk + '"' + (rev ? ' checked' : '') + '>⟲</label>';
    }
    return h + '</div></td>';
  }

  function rowTr(gid, row, ini) {
    var r = ROWS[row];
    var cls = r.cls + (row === 'New' || row === 'Old' ? ' dc-split' : '');
    var lead = (row === 'main')
      ? ''
      : '<span class="dc-sub">└ ' + r.label + '</span>';
    // main row's label cell is the block header (handled separately); other rows show the sub-label.
    var html = '<tr class="' + cls + '" data-gid="' + esc(gid) + '"><td class="dc-rowlabel">' + lead + '</td>';
    GEN.forEach(function (g) {
      var sec = ini[secKey(g.ini, gid, r.server)] || {};
      var v = sec[r.key] || '';
      var rev = r.rev && sec[r.key + 'Rev'];
      html += cell(gid, row, g.k, v, rev);
    });
    html += '</tr>';
    return html;
  }

  // a whole block (Default or one group) = a <tbody> with its rows.
  function blockHtml(gid, label, ini) {
    var split = blockSplit(ini, gid);
    var rm = gid ? '<a class="dc-rm" title="Remove this group">×</a> ' : '';
    var note = gid ? '' : ' <span class="dc-defnote">— used by any group without its own range</span>';
    // header row (main): block label in the first cell, main range cells, split toggle
    var head = '<tr class="dc-row-main dc-head" data-gid="' + esc(gid) + '">' +
      '<td class="dc-rowlabel dc-blocklabel">' + rm + '<b>' + esc(label) + '</b>' + note + '</td>';
    GEN.forEach(function (g) {
      var sec = ini[secKey(g.ini, gid, false)] || {};
      head += cell(gid, 'main', g.k, sec.Cells || '', sec.CellsRev);
    });
    head += '<td class="dc-splitcell"><label><input type="checkbox" class="dc-split-cb" data-gid="' + esc(gid) + '"' +
      (split ? ' checked' : '') + '> split</label></td></tr>';

    return '<tbody class="dc-block" data-gid="' + esc(gid) + '">' +
      head +
      rowTr(gid, 'reserved', ini) +
      rowTr(gid, 'sevak', ini) +
      rowTr(gid, 'New', ini) +
      rowTr(gid, 'Old', ini) +
      '</tbody>';
  }

  function scaffold(host) {
    var help =
      '<div class="dc-help">' +
      '<div class="dc-help-title">How to type a range</div>' +
      '<p>Type a list separated by commas. Each part is one seat, or a <b>start-end</b> range:</p>' +
      '<table class="dc-help-eg">' +
      '<tr><td><code>1-40</code></td><td>1, 2, 3 … 40</td></tr>' +
      '<tr><td><code>1-40, 55, 60-70</code></td><td>1…40, then 55, then 60…70</td></tr>' +
      '<tr><td><code>55</code></td><td>just that one seat</td></tr>' +
      '<tr><td><code>A1-A5</code></td><td>A1, A2, A3, A4, A5 (letters are kept)</td></tr>' +
      '</table>' +
      '<ul>' +
      '<li>Ranges go <b>low to high</b> only. For high-to-low, tick the <b>⟲ reverse</b> box (1-100 → 100, 99 … 1).</li>' +
      '<li>Do not mix letters in one range (write <code>A1-A5, B1-B5</code>, not <code>A1-B5</code>). Leave a box blank for none.</li>' +
      '<li><b>Keep aside</b> — seats held back from auto-assignment (assign them by hand).</li>' +
      '<li>Tick <b>split</b> to seat <b>New</b> and <b>Old</b> students on different seats (the combined "Dining seats" is then not used).</li>' +
      '<li><b>Sevak</b> — separate seats for course servers (always treated as old). Leave blank to not seat servers.</li>' +
      '<li><b>Group</b> rows override the Default for that group only; groups you do not add use the Default range.</li>' +
      '</ul></div>';
    host.html(
      '<table class="dc-table"><thead>' +
      '<tr><th class="dc-rowlabel"></th><th>Male</th><th>Female</th><th></th></tr>' +
      '</thead></table>' +
      '<div class="dc-add"><select class="dc-add-sel"></select></div>' +
      help
    );
  }

  function addedGids(host) {
    var g = [];
    host.find('tbody.dc-block').each(function () { var v = String($(this).data('gid') || ''); if (v) { g.push(v); } });
    return g;
  }

  function rebuildAddSelect(host) {
    var groups = host.data('groups') || [];
    var added = addedGids(host);
    var opts = '<option value="">+ Add group…</option>';
    groups.forEach(function (t) {
      if (added.indexOf(String(t.key)) === -1) { opts += '<option value="' + esc(t.key) + '">' + esc(t.label) + '</option>'; }
    });
    var $sel = host.find('.dc-add-sel').html(opts);
    host.find('.dc-add').toggle($sel.find('option').length > 1);
  }

  function applyBlock($b) {
    var split = $b.find('.dc-split-cb').is(':checked');
    $b.find('.dc-row-main .dc-in').prop('disabled', split).toggleClass('dc-off', split);
    $b.find('.dc-row-main .dc-revcb').prop('disabled', split);
    $b.find('.dc-row-split').toggle(split);
  }

  function assemble(host) {
    var out = [];
    function val(gid, row, g) { var e = host.find('.dc-in[data-gid="' + gid + '"][data-row="' + row + '"][data-g="' + g + '"]'); return e.length ? String(e.val()).trim() : ''; }
    function rev(gid, row, g) { var e = host.find('.dc-revcb[data-gid="' + gid + '"][data-row="' + row + '"][data-g="' + g + '"]'); return e.length && e.is(':checked'); }
    host.find('tbody.dc-block').each(function () {
      var gid = String($(this).data('gid') || '');
      var split = $(this).find('.dc-split-cb').is(':checked');
      GEN.forEach(function (g) {
        var lines = [];
        if (split) {
          var nv = val(gid, 'New', g.k), ov = val(gid, 'Old', g.k);
          if (nv) { lines.push('New = ' + nv); if (rev(gid, 'New', g.k)) { lines.push('NewRev = 1'); } }
          if (ov) { lines.push('Old = ' + ov); if (rev(gid, 'Old', g.k)) { lines.push('OldRev = 1'); } }
        } else {
          var cv = val(gid, 'main', g.k);
          if (cv) { lines.push('Cells = ' + cv); if (rev(gid, 'main', g.k)) { lines.push('CellsRev = 1'); } }
        }
        var rv = val(gid, 'reserved', g.k);
        if (rv) { lines.push('Reserved = ' + rv); }
        if (lines.length) { out.push('[' + secKey(g.ini, gid, false) + ']'); out = out.concat(lines); out.push(''); }
        // Sevak (server) section - single range, optional reverse.
        var sv = val(gid, 'sevak', g.k);
        if (sv) { out.push('[' + secKey(g.ini, gid, true) + ']'); out.push('Cells = ' + sv); if (rev(gid, 'sevak', g.k)) { out.push('CellsRev = 1'); } out.push(''); }
      });
    });
    return out.join('\n').replace(/\n{3,}/g, '\n\n').replace(/^\n+/, '').replace(/\n+$/, '\n');
  }

  $(document).ready(function () {
    var host = $('#dh-diningcfg');
    if (!host.length) { return; }
    var $ta = $('[name="cs_dining_config"]');
    if (!$ta.length) { return; }

    scaffold(host);
    var ini = parseINI($ta.val());
    var groups = host.data('groups') || [];
    var byKey = {}; groups.forEach(function (t) { byKey[String(t.key)] = t; });
    var $table = host.find('.dc-table');

    // Default block is always present.
    $table.append(blockHtml('', 'Default', ini));
    // Group blocks that already have saved data.
    groups.forEach(function (t) {
      if (blockHasData(ini, String(t.key))) { $table.append(blockHtml(String(t.key), t.label, ini)); }
    });

    host.find('tbody.dc-block').each(function () { applyBlock($(this)); });
    rebuildAddSelect(host);

    // Hide the raw INI textarea (kept in the DOM so it still submits).
    $ta.closest('.form-item, .form-group').hide();

    function sync() { $ta.val(assemble(host)); }

    host.on('input', '.dc-in', sync);
    host.on('change', '.dc-revcb', sync);
    host.on('change', '.dc-split-cb', function () { applyBlock($(this).closest('tbody.dc-block')); sync(); });
    host.on('change', '.dc-add-sel', function () {
      var k = this.value; if (!k) { return; }
      var t = byKey[k] || { key: k, label: 'Group ' + k };
      $table.append(blockHtml(String(t.key), t.label, {}));
      var $b = host.find('tbody.dc-block[data-gid="' + k + '"]');
      applyBlock($b);
      rebuildAddSelect(host); sync();
    });
    host.on('click', '.dc-rm', function () {
      var $b = $(this).closest('tbody.dc-block');
      var gid = String($b.data('gid'));
      var hasVal = false;
      $b.find('.dc-in').each(function () { if (String(this.value).trim() !== '') { hasVal = true; } });
      if (hasVal && !window.confirm('Remove this group range? Its dining seats will be cleared when you save.')) { return; }
      $b.remove();
      rebuildAddSelect(host); sync();
    });

    sync();
  });
})(jQuery);
