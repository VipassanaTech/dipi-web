/* Visual "hall diagram" editor for one seating section on the centre form.
   Progressive enhancement: each .dh-seatviz container is bound (via data-prefix)
   to a set of plain seatcfg_<...>_* fields. It draws an interactive diagram, hides
   those raw fields, and writes the values back on every change so the normal form
   save/validation is unchanged. Vanilla JS, no jQuery.

   Model (matches dh_generate_seating_plan / dh_allocate_seats in zero-day.inc):
   - "Number of columns" (SeatsPerRow) = width of the MAIN block. Students fill it
     row by row; rows grow with attendance, so the rows drawn are a preview. The
     plan prints row 1 (front) at the BOTTOM, teacher's dais below, so we draw it
     front-at-the-bottom. The front row shows its seat numbers so the direction is
     clear (seat 1 on the direction side).
   - "Number of chowky columns" (SeatsPerRowChowky) = width of a SEPARATE chowky
     block (low seats/chairs), placed left / right / back per "Chowky position".
     At the BACK it is a full-width band whose columns match (and follow) the main
     grid and line up with it.
   - Empty seats: click any main OR chowky seat to leave it empty (a gap). Stored as
     row-col pairs (row 1 = front): EmptySeats for the main grid, EmptyChowky for
     the chowky grid. Config only for now; the allocator will honour them later. */
(function () {
  var CHO_PREVIEW_ROWS = 2;   // chowky rows are attendance-driven; show a hint

  function initOne(host) {
    if (!host || host.dataset.dhInit) return;
    host.dataset.dhInit = '1';
    var prefix = host.dataset.prefix || 'seatcfg_male_';
    var nat = host.dataset.nat || (prefix.indexOf('female') >= 0 ? 'left' : 'right');
    var st = {};

    function f(suffix) { return document.querySelector('[name="' + prefix + suffix + '"]'); }
    function gv(el) { return el ? String(el.value).trim() : ''; }
    function setf(suffix, v) { var el = f(suffix); if (el && String(el.value) !== String(v)) { el.value = v; } }
    function key(r, c) { return r + '-' + c; }
    function parseEmpty(t) {
      var o = {};
      (t || '').split(',').forEach(function (p) { var m = p.match(/^\s*(\d+)\s*-\s*(\d+)\s*$/); if (m) o[key(+m[1], +m[2])] = true; });
      return o;
    }
    function emptyText(o) {
      return Object.keys(o).map(function (k) { var a = k.split('-'); return [+a[0], +a[1]]; })
        .sort(function (a, b) { return a[0] - b[0] || a[1] - b[1]; })
        .map(function (a) { return a[0] + '-' + a[1]; }).join(', ');
    }

    function load() {
      var empties = parseEmpty(gv(f('empty')));
      var choEmpties = parseEmpty(gv(f('empty_cho')));
      var show = 4; for (var k in empties) { var r = +k.split('-')[0]; if (r > show) show = r; }
      var choShow = CHO_PREVIEW_ROWS; for (var k2 in choEmpties) { var r2 = +k2.split('-')[0]; if (r2 > choShow) choShow = r2; }
      st = {
        spr:  parseInt(gv(f('spr')), 10)  || 5,
        sprc: parseInt(gv(f('sprc')), 10) || 1,   // minimum 1 chowky column
        dir:  gv(f('dir')) || nat,   // no "default" option; empty falls to the natural side
        pos:  gv(f('pos')),
        empties: empties,
        choEmpties: choEmpties,
        show: show,
        choShow: choShow
      };
    }
    function dirOf() { return st.dir || nat; }
    function posOf() { return st.pos || dirOf(); }
    // Chowky column count: at the back it spans the full width (matches the main
    // grid and follows it); on a side it is its own SeatsPerRowChowky.
    function choCols() { return posOf() === 'back' ? st.spr : st.sprc; }
    function pruneEmpties() {
      for (var k in st.empties) if (+k.split('-')[1] > st.spr) delete st.empties[k];
      var cc = choCols();
      for (var k2 in st.choEmpties) if (+k2.split('-')[1] > cc) delete st.choEmpties[k2];
    }
    function save() {
      setf('spr', st.spr);
      setf('sprc', st.sprc);
      setf('dir', st.dir);
      setf('pos', st.pos);
      setf('empty', emptyText(st.empties));
      setf('empty_cho', emptyText(st.choEmpties));
    }
    // Seat number counted from the direction side, for a row `cols` wide. DOM cells
    // are always laid out left->right, so this maps a DOM index to its seat number.
    function colNum(c, cols) { return (dirOf() === 'right') ? c : (cols - c + 1); }

    function el(tag, cls, html) { var e = document.createElement(tag); if (cls) e.className = cls; if (html != null) e.innerHTML = html; return e; }

    // One clickable seat. Empty seats draw a full corner-to-corner cross (CSS);
    // front-row main seats show their number. Empties are stored by seat number.
    function seatCell(cols, r, c, chowky, showNum, onToggle, emptyMap) {
      var cn = colNum(c, cols);
      var isEmpty = !!emptyMap[key(r, cn)];
      var cls = 'dhv-seat dhv-click' + (chowky ? ' dhv-chowky' : '') + (isEmpty ? ' dhv-empty' : (showNum ? ' dhv-num' : ''));
      var seat = el('div', cls, (!isEmpty && showNum) ? String(cn) : null);
      seat.title = isEmpty ? ('Seat ' + cn + ' - empty (click to restore)') : ('Seat ' + cn + ' - click to leave empty');
      seat.onclick = function () { onToggle(r, cn); };
      return seat;
    }

    // Main block: one line per row, row 1 first in the DOM; CSS column-reverse
    // paints row 1 at the bottom (front).
    function drawMain() {
      var main = el('div', 'dhv-rows');
      for (var r = 1; r <= st.show; r++) {
        var line = el('div', 'dhv-rowline dhv-' + dirOf());
        line.appendChild(el('div', 'dhv-rlabel', 'row ' + r));
        var row = el('div', 'dhv-row');
        for (var c = 1; c <= st.spr; c++) row.appendChild(seatCell(st.spr, r, c, false, r === 1, toggleEmpty, st.empties));
        line.appendChild(row);
        main.appendChild(line);
      }
      return main;
    }
    // Chowky on a side: its own bordered block, cols = sprc, header on top so the
    // rows sit at the bottom and align with the main rows.
    function drawChowkySide() {
      var wrap = el('div', 'dhv-cho');
      var head = el('div', 'dhv-cho-head');
      head.appendChild(el('span', 'dhv-cho-cap', 'Chowky'));
      var add = el('button', 'dhv-cho-add', '+ row'); add.type = 'button';
      add.onclick = function () { st.choShow++; apply(); };
      head.appendChild(add);
      wrap.appendChild(head);
      var rows = el('div', 'dhv-cho-rows');
      for (var r = 1; r <= st.choShow; r++) {
        var line = el('div', 'dhv-rowline dhv-' + dirOf());
        var row = el('div', 'dhv-row');
        for (var c = 1; c <= st.sprc; c++) row.appendChild(seatCell(st.sprc, r, c, true, false, toggleChoEmpty, st.choEmpties));
        line.appendChild(row);
        rows.appendChild(line);
      }
      wrap.appendChild(rows);
      return wrap;
    }
    // Chowky at the back: a full-width band above the seats. Columns match the main
    // grid (st.spr) and an empty row-label spacer keeps them lined up with it.
    function drawChowkyBack() {
      var wrap = el('div', 'dhv-choback');
      var head = el('div', 'dhv-choback-head');
      head.appendChild(el('span', 'dhv-cho-cap', 'Chowky (back – full width)'));
      var add = el('button', 'dhv-cho-add', '+ row'); add.type = 'button';
      add.onclick = function () { st.choShow++; apply(); };
      head.appendChild(add);
      wrap.appendChild(head);
      var rows = el('div', 'dhv-rows');
      for (var r = 1; r <= st.choShow; r++) {
        var line = el('div', 'dhv-rowline dhv-' + dirOf());
        line.appendChild(el('div', 'dhv-rlabel', ''));   // spacer -> align columns with the main grid
        var row = el('div', 'dhv-row');
        for (var c = 1; c <= st.spr; c++) row.appendChild(seatCell(st.spr, r, c, true, false, toggleChoEmpty, st.choEmpties));
        line.appendChild(row);
        rows.appendChild(line);
      }
      wrap.appendChild(rows);
      return wrap;
    }
    function drawSide() {
      var area = host.querySelector('.dhv-area');
      if (!area) return;
      area.innerHTML = '';
      var main = drawMain();
      var pos = posOf();
      if (pos === 'back') {                       // choCols = spr, always > 0
        area.className = 'dhv-area dhv-stack';
        area.appendChild(drawChowkyBack());
        area.appendChild(main);
      } else if (st.sprc > 0 && pos === 'left') {
        area.className = 'dhv-area dhv-lr';
        area.appendChild(drawChowkySide()); area.appendChild(main);
      } else if (st.sprc > 0) {                   // right (default)
        area.className = 'dhv-area dhv-lr';
        area.appendChild(main); area.appendChild(drawChowkySide());
      } else {
        area.className = 'dhv-area';
        area.appendChild(main);
      }
    }
    function toggleEmpty(r, cn) { var k = key(r, cn); if (st.empties[k]) delete st.empties[k]; else st.empties[k] = true; apply(); }
    function toggleChoEmpty(r, cn) { var k = key(r, cn); if (st.choEmpties[k]) delete st.choEmpties[k]; else st.choEmpties[k] = true; apply(); }

    function stepper(kkey, label, min) {
      var wrap = el('div', 'dhv-fld');
      wrap.appendChild(el('span', null, label));
      var box = el('span', 'dhv-step');
      var minus = el('button', null, '−'); minus.type = 'button';
      var val = el('span', 'dhv-v', String(st[kkey]));
      var plus = el('button', null, '+'); plus.type = 'button';
      minus.onclick = function () { st[kkey] = Math.max(min, st[kkey] - 1); apply(); };
      plus.onclick = function () { st[kkey] = st[kkey] + 1; apply(); };
      box.appendChild(minus); box.appendChild(val); box.appendChild(plus);
      wrap.appendChild(box);
      return wrap;
    }
    function select(kkey, label, opts) {
      var wrap = el('div', 'dhv-fld');
      wrap.appendChild(el('span', null, label));
      var sel = el('select');
      opts.forEach(function (o) { var op = el('option'); op.value = o[0]; op.textContent = o[1]; if (st[kkey] === o[0]) op.selected = true; sel.appendChild(op); });
      sel.onchange = function () { st[kkey] = sel.value; apply(); };
      wrap.appendChild(sel);
      return wrap;
    }

    function build() {
      host.innerHTML = '';
      var hall = el('div', 'dhv-hall');
      var area = el('div', 'dhv-area');
      hall.appendChild(area);
      var add = el('button', 'dhv-addrow', '+ show more rows'); add.type = 'button';
      add.onclick = function () { st.show++; apply(); };
      hall.appendChild(add);
      hall.appendChild(el('div', 'dhv-front', '▼ FRONT &ndash; Teacher\'s Seat'));
      var ctrls = el('div', 'dhv-ctrls');
      var c = el('div', 'dhv-ctrl');
      c.appendChild(stepper('spr', 'Number of columns', 1));
      c.appendChild(stepper('sprc', 'Number of chowky columns', 1));
      c.appendChild(select('dir', 'Seat direction', [['right', 'Left to right'], ['left', 'Right to left']]));
      c.appendChild(select('pos', 'Chowky position', [['', 'Default (' + nat + ')'], ['left', 'Left'], ['right', 'Right'], ['back', 'Back']]));
      var ef = el('div', 'dhv-fld');
      ef.appendChild(el('span', null, 'Empty seats'));
      var er = el('span', 'dhv-empty-info');
      er.appendChild(el('span', 'dhv-empty-n', '0'));
      var clr = el('a', 'dhv-empty-clr', 'clear'); clr.href = '#';
      clr.onclick = function (ev) { ev.preventDefault(); st.empties = {}; st.choEmpties = {}; apply(); };
      er.appendChild(clr);
      ef.appendChild(er);
      c.appendChild(ef);
      ctrls.appendChild(c);
      hall.appendChild(ctrls);
      host.appendChild(hall);
      apply();
    }
    function apply() {
      // At the back the chowky spans the full width, so its stored column count
      // follows the main grid (and updates when the main grid does).
      if (posOf() === 'back') st.sprc = st.spr;
      pruneEmpties();
      drawSide();
      var flds = host.querySelectorAll('.dhv-ctrl .dhv-fld');   // 0=spr 1=sprc 2=dir 3=pos 4=empty
      if (flds[0]) { var v0 = flds[0].querySelector('.dhv-v'); if (v0) v0.textContent = st.spr; }
      var back = posOf() === 'back';
      if (flds[1]) {
        var v1 = flds[1].querySelector('.dhv-v'); if (v1) v1.textContent = back ? st.spr : st.sprc;
        var btns = flds[1].querySelectorAll('.dhv-step button');
        for (var i = 0; i < btns.length; i++) btns[i].disabled = back;
        flds[1].style.opacity = back ? '.5' : '';
        flds[1].title = back ? 'Chowky is at the back, so it spans the full width - its columns match the main grid.' : '';
      }
      var n = host.querySelector('.dhv-empty-n');
      if (n) n.textContent = Object.keys(st.empties).length + Object.keys(st.choEmpties).length;
      save();
    }

    // Hide the raw seatcfg_* form items this section owns.
    function setRawHidden(hidden) {
      ['spr', 'sprc', 'dir', 'pos', 'empty', 'empty_cho'].forEach(function (suffix) {
        var input = f(suffix);
        if (!input) return;
        var item = input.closest('.form-item') || input.parentNode;
        if (item) item.style.display = hidden ? 'none' : '';
      });
    }

    load();
    build();
    setRawHidden(true);
  }

  // ---------- Plan manager: create / delete the Group sections ----------
  // Main Plan sections (data-plan="main") are always present. Group sections
  // (data-plan="group") are all rendered but hidden until they hold data; the
  // operator adds one from a dropdown (seeded from the same-gender Main Plan) and
  // deletes it with a link inside the section.
  var _groups = [], _addBox = null;

  function q(name) { return document.querySelector('[name="' + name + '"]'); }
  function sectionOf(host) { return host.closest('.dh-plan-section'); }
  function hasData(host) { var el = q(host.dataset.prefix + 'spr'); return !!(el && String(el.value).trim() !== ''); }
  function showSection(host, on) { var fs = sectionOf(host); if (fs) fs.style.display = on ? '' : 'none'; }
  function initEditor(host) { host.dataset.dhInit = ''; host.innerHTML = ''; initOne(host); }

  function addDeleteLink(host) {
    var fs = sectionOf(host); if (!fs || fs.querySelector('.dhv-plan-del')) return;
    // Put it in the heading/legend (Bootstrap .panel-heading, core Seven <legend>)
    // so it stays visible even when the section is collapsed.
    var head = fs.querySelector('.panel-heading') || fs.querySelector('legend') || fs;
    var del = document.createElement('a');
    del.href = '#'; del.className = 'dhv-plan-del no-print'; del.textContent = '✕ Delete plan';
    del.onclick = function (ev) {
      ev.preventDefault(); ev.stopPropagation();
      if (window.confirm('Delete the "' + host.dataset.label + '" plan? Its layout will be removed.')) deletePlan(host);
    };
    head.appendChild(del);
  }
  function removeDeleteLink(host) {
    var fs = sectionOf(host); if (!fs) return;
    var del = fs.querySelector('.dhv-plan-del'); if (del && del.parentNode) del.parentNode.removeChild(del);
  }

  function activatePlan(host) {
    // Seed the new plan from the same-gender Main Plan so it starts real & editable.
    var mainPfx = 'seatcfg_' + host.dataset.gender + '_';
    ['spr', 'sprc', 'dir', 'pos'].forEach(function (k) { var m = q(mainPfx + k), t = q(host.dataset.prefix + k); if (m && t) t.value = m.value; });
    ['empty', 'empty_cho'].forEach(function (k) { var t = q(host.dataset.prefix + k); if (t) t.value = ''; });
    showSection(host, true);
    initEditor(host);
    addDeleteLink(host);
    renderAddUI();
  }
  function deletePlan(host) {
    ['spr', 'sprc', 'dir', 'pos', 'empty', 'empty_cho'].forEach(function (k) { var t = q(host.dataset.prefix + k); if (t) t.value = ''; });
    removeDeleteLink(host);
    host.dataset.dhInit = ''; host.innerHTML = '';
    showSection(host, false);
    renderAddUI();
  }

  function renderAddUI() {
    if (!_addBox) return;
    var inactive = _groups.filter(function (h) { return !hasData(h); });
    _addBox.innerHTML = '';
    var wrap = document.createElement('div'); wrap.className = 'dhv-planadd';
    var label = document.createElement('span'); label.className = 'dhv-planadd-lbl'; label.textContent = 'Add a group plan:';
    var sel = document.createElement('select');
    var def = document.createElement('option'); def.value = '';
    def.textContent = inactive.length ? '— choose —' : '(all group plans created)';
    sel.appendChild(def);
    inactive.forEach(function (h) { var o = document.createElement('option'); o.value = h.dataset.prefix; o.textContent = h.dataset.label; sel.appendChild(o); });
    if (!inactive.length) sel.disabled = true;
    sel.onchange = function () {
      var pfx = sel.value; if (!pfx) return;
      var host = _groups.filter(function (h) { return h.dataset.prefix === pfx; })[0];
      if (host) activatePlan(host);
    };
    wrap.appendChild(label); wrap.appendChild(sel);
    _addBox.appendChild(wrap);
  }

  function run() {
    var hosts = document.querySelectorAll('.dh-seatviz');
    _groups = [];
    for (var i = 0; i < hosts.length; i++) {
      var host = hosts[i];
      if (host.dataset.plan === 'group') _groups.push(host);
      else initOne(host);   // Main Plan: always present + active
    }
    _addBox = document.getElementById('dh-plan-add');
    _groups.forEach(function (host) {
      if (hasData(host)) { showSection(host, true); initOne(host); addDeleteLink(host); }
      else { showSection(host, false); }
    });
    renderAddUI();
  }

  // Drupal adds this file in <head> by default, so run at DOM-ready.
  if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', run);
  } else {
    run();
  }
})();
