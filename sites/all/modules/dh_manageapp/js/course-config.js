/* Visual editor (prototype) for the centre "Course Configuration".
   Progressive enhancement over the raw INI textarea (cs_course_config).

   Instead of listing every course type, the centre ADDS the types it needs
   (a dropdown of the remaining types). Each added type has Waitlist/Full limits
   per gender, plus an optional New/Old/Sevak breakdown ("split"). On every change
   the exact same INI is written back into the hidden textarea, so the backend
   (dh_course_status_check) is unchanged.

   Rules enforced (blocks Save): Full must be greater than Waitlist wherever both
   are set. Leave a box blank for "no limit"; 0 is not allowed (auto-cleared to
   blank, and never written to the config). Vanilla-ish jQuery. */
(function ($) {
  var GENDERS = [{ k: 'Male', label: 'Male' }, { k: 'Female', label: 'Female' }];
  var ATYPES = [{ k: 'New', label: 'New' }, { k: 'Old', label: 'Old' }, { k: 'Server', label: 'Sevak' }];

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

  function num(el) { return el ? String(el.value).replace(/[^0-9]/g, '') : ''; }

  // a course type is worth showing/storing only if it has at least one real limit
  function hasData(sec) {
    return Object.keys(sec).some(function (k) {
      return /^(MaxApps|Waitlist)-/.test(k) && parseInt(sec[k], 10) > 0;
    });
  }

  function hasSplit(sec) {
    var f = false;
    ['Male', 'Female'].forEach(function (G) {
      ['New', 'Old', 'Server'].forEach(function (tp) {
        if (sec['MaxApps-' + G + '-' + tp] || sec['Waitlist-' + G + '-' + tp]) { f = true; }
      });
    });
    return f;
  }

  function inp(g, tp, kind, v) {
    return '<input type="text" inputmode="numeric" class="cc-in" data-g="' + g + '" data-tp="' + tp +
      '" data-kind="' + kind + '" value="' + (v || '') + '" size="4" />';
  }

  // Build the <tr> group (main row + 3 New/Old/Sevak sub-rows) for one course type.
  function rowHtml(t, sec) {
    sec = sec || {};
    var adv = hasSplit(sec);
    var html = '<tr class="cc-type" data-tk="' + t.key + '">' +
      '<td class="cc-tname"><a class="cc-rm" title="Remove this course type">×</a> ' + t.label + '</td>';
    GENDERS.forEach(function (g, gi) {
      html += '<td class="cc-basic">' + inp(g.k, '', 'Waitlist', sec['Waitlist-' + g.k]) + '</td>' +
        '<td class="cc-basic">' + inp(g.k, '', 'MaxApps', sec['MaxApps-' + g.k]) + '</td>';
      if (gi === 0) { html += '<td class="cc-gap"></td>'; }
    });
    html += '<td class="cc-advcell"><label><input type="checkbox" class="cc-adv-cb"' + (adv ? ' checked' : '') + '> split</label></td></tr>';
    ATYPES.forEach(function (a) {
      html += '<tr class="cc-adv-row" data-tk="' + t.key + '"' + (adv ? '' : ' style="display:none"') + '>' +
        '<td class="cc-atype">└ ' + a.label + '</td>';
      GENDERS.forEach(function (g, gi) {
        html += '<td>' + inp(g.k, a.k, 'Waitlist', sec['Waitlist-' + g.k + '-' + a.k]) + '</td>' +
          '<td>' + inp(g.k, a.k, 'MaxApps', sec['MaxApps-' + g.k + '-' + a.k]) + '</td>';
        if (gi === 0) { html += '<td class="cc-gap"></td>'; }
      });
      html += '<td></td></tr>';
    });
    return html;
  }

  function scaffold(host) {
    var ex =
      '<div class="cc-examples"><b>Example</b> &mdash; 10 Day, Male: <i>Waitlist at</i> 30, <i>Full at</i> 40. ' +
      'The Male section becomes <b>Wait List</b> at 30 applications and <b>Course Full</b> at 40 applications. Leave a box blank for no limit.' +
      '<div class="cc-note"><b>Full</b> must be greater than <b>Waitlist</b>.<br>' +
      'Both numbers are the <b>total</b> applications received (counted from zero) &mdash; <b>Full is not added on top of Waitlist</b>. ' +
      'So <i>Waitlist 30, Full 40</i> means it closes at 40 in total, not at 30+40.<br>' +
      'Tick <b>split</b> to set separate limits for New / Old / Sevak.<br>' +
      'The count does not include applications with status <b>Cancelled</b>, <b>Rejected</b> or <b>Duplicate</b>.<br>' +
      'These settings only move a course to <b>Wait List</b> or <b>Course Full</b>. Once a course is waitlisted or closed, ' +
      'changing these settings will <b>not</b> re-open it &mdash; to re-open, do it manually from <b>Manage Courses</b>.</div>' +
      '</div>';
    host.html(
      '<table class="cc-table"><thead>' +
      '<tr><th class="cc-tname"></th><th colspan="2">Male</th><th class="cc-gap"></th><th colspan="2">Female</th><th></th></tr>' +
      '<tr><th></th><th>Waitlist at</th><th>Full at</th><th class="cc-gap"></th><th>Waitlist at</th><th>Full at</th><th></th></tr>' +
      '</thead><tbody></tbody></table>' +
      '<div class="cc-empty">No limits set yet. Add a course type below to set fill / waitlist limits.</div>' +
      '<div class="cc-add"><select class="cc-add-sel"></select></div>' +
      '<div class="cc-msg" role="alert"></div>' +
      ex
    );
  }

  function addedKeys(host) {
    var keys = [];
    host.find('tbody .cc-type').each(function () { keys.push(String($(this).data('tk'))); });
    return keys;
  }

  function rebuildAddSelect(host) {
    var types = host.data('types') || [];
    var added = addedKeys(host);
    var opts = '<option value="">+ Add course type…</option>';
    types.forEach(function (t) {
      if (added.indexOf(String(t.key)) === -1) {
        opts += '<option value="' + t.key + '">' + t.label + '</option>';
      }
    });
    var $sel = host.find('.cc-add-sel').html(opts);
    // hide the add control when everything is already added
    host.find('.cc-add').toggle($sel.find('option').length > 1);
  }

  function restripe(host) {
    var i = 0;
    host.find('tbody .cc-type').each(function () {
      var tk = String($(this).data('tk')), alt = (i % 2) === 1;
      $(this).toggleClass('cc-alt', alt);
      host.find('tbody .cc-adv-row[data-tk="' + tk + '"]').toggleClass('cc-alt', alt);
      i++;
    });
    host.find('.cc-empty').toggle(i === 0);
  }

  function applyAdvState(host) {
    host.find('.cc-type').each(function () {
      var $t = $(this), tk = $t.data('tk'), adv = $t.find('.cc-adv-cb').is(':checked');
      $t.find('.cc-basic .cc-in').prop('disabled', adv).toggleClass('cc-off', adv);
      host.find('.cc-adv-row[data-tk="' + tk + '"]').toggle(adv);
    });
  }

  function assemble(host) {
    var out = [];
    host.find('tbody .cc-type').each(function () {
      var $t = $(this), tk = $t.data('tk'), adv = $t.find('.cc-adv-cb').is(':checked');
      var sec = [];
      GENDERS.forEach(function (g) {
        function v(tp, kind) {
          var sel = adv ? host.find('.cc-adv-row[data-tk="' + tk + '"] .cc-in[data-g="' + g.k + '"][data-tp="' + tp + '"][data-kind="' + kind + '"]')
            : $t.find('.cc-in[data-g="' + g.k + '"][data-tp="' + tp + '"][data-kind="' + kind + '"]');
          return sel.length ? num(sel[0]) : '';
        }
        if (adv) {
          ['New', 'Old', 'Server'].forEach(function (tp) {
            var wi = parseInt(v(tp, 'Waitlist'), 10), fi = parseInt(v(tp, 'MaxApps'), 10);
            if (wi > 0) { sec.push('Waitlist-' + g.k + '-' + tp + ' = ' + wi); }
            if (fi > 0) { sec.push('MaxApps-' + g.k + '-' + tp + ' = ' + fi); }
          });
        } else {
          var wi = parseInt(v('', 'Waitlist'), 10), fi = parseInt(v('', 'MaxApps'), 10);
          if (wi > 0) { sec.push('Waitlist-' + g.k + ' = ' + wi); }
          if (fi > 0) { sec.push('MaxApps-' + g.k + ' = ' + fi); }
        }
      });
      if (sec.length) { out.push('[' + tk + ']'); out = out.concat(sec); out.push(''); }
    });
    return out.join('\n');
  }

  // Full must be > Waitlist wherever both are real limits (>0). 0/blank = no limit.
  function validate(host) {
    var errs = [];
    host.find('.cc-in').removeClass('cc-err');
    host.find('tbody .cc-type').each(function () {
      var $t = $(this), tk = $t.data('tk'), adv = $t.find('.cc-adv-cb').is(':checked');
      var label = $t.find('.cc-tname').text().replace(/^\s*×\s*/, '').trim();
      GENDERS.forEach(function (g) {
        function pair(tp, subLabel) {
          var scope = adv ? host.find('.cc-adv-row[data-tk="' + tk + '"]') : $t;
          var $w = scope.find('.cc-in[data-g="' + g.k + '"][data-tp="' + tp + '"][data-kind="Waitlist"]');
          var $f = scope.find('.cc-in[data-g="' + g.k + '"][data-tp="' + tp + '"][data-kind="MaxApps"]');
          var w = num($w[0]), f = num($f[0]);
          var wi = parseInt(w, 10), fi = parseInt(f, 10);
          if (w !== '' && f !== '' && wi > 0 && fi > 0 && fi <= wi) {
            $w.addClass('cc-err'); $f.addClass('cc-err');
            errs.push(label + ' – ' + g.label + subLabel + ': Full (' + f + ') must be greater than Waitlist (' + w + ').');
          }
        }
        if (adv) { ATYPES.forEach(function (a) { pair(a.k, ' ' + a.label); }); }
        else { pair('', ''); }
      });
    });
    var $msg = host.find('.cc-msg');
    if (errs.length) { $msg.html(errs.join('<br>')).show(); }
    else { $msg.hide().empty(); }
    return errs;
  }

  $(document).ready(function () {
    var host = $('#dh-coursecfg');
    if (!host.length) { return; }
    var $ta = $('[name="cs_course_config"]');
    if (!$ta.length) { return; }

    scaffold(host);
    // Pre-load rows for the types already present in the saved config.
    var ini = parseINI($ta.val());
    var types = host.data('types') || [];
    var byKey = {}; types.forEach(function (t) { byKey[String(t.key)] = t; });
    Object.keys(ini).forEach(function (k) {
      if (!hasData(ini[k])) { return; }   // skip empty course types
      var t = byKey[k] || { key: k, label: k };
      host.find('tbody').append(rowHtml(t, ini[k]));
    });
    applyAdvState(host);
    restripe(host);
    rebuildAddSelect(host);

    // Hide the raw INI textarea (kept in the DOM so it still submits).
    $ta.closest('.form-item, .form-group').hide();

    function sync() { $ta.val(assemble(host)); validate(host); }

    host.on('input', '.cc-in', function () { this.value = this.value.replace(/[^0-9]/g, ''); sync(); });
    // On blur: strip leading zeros and clear a bare 0 (0 is not a valid limit).
    host.on('change', '.cc-in', function () { var n = parseInt(this.value, 10); this.value = (n > 0) ? String(n) : ''; sync(); });
    host.on('change', '.cc-adv-cb', function () { applyAdvState(host); sync(); });
    host.on('change', '.cc-add-sel', function () {
      var k = this.value; if (!k) { return; }
      var t = byKey[k] || { key: k, label: k };
      host.find('tbody').append(rowHtml(t, {}));
      restripe(host); rebuildAddSelect(host); sync();
    });
    host.on('click', '.cc-rm', function () {
      var $type = $(this).closest('.cc-type');
      var tk = String($type.data('tk'));
      var $rows = host.find('tbody tr[data-tk="' + tk + '"]');
      // Only confirm when the type actually has limits set (removing an empty
      // just-added row is harmless and needs no prompt).
      var hasVal = false;
      $rows.find('.cc-in').each(function () { if (num(this) !== '') { hasVal = true; } });
      if (hasVal) {
        var label = $type.find('.cc-tname').text().replace(/^\s*×\s*/, '').trim();
        if (!window.confirm('Remove "' + label + '"? Its Waitlist / Full limits will be cleared when you save.')) {
          return;
        }
      }
      $rows.remove();
      restripe(host); rebuildAddSelect(host); sync();
    });

    // Backstop: block the form submit if Full <= Waitlist anywhere.
    $ta.closest('form').on('submit', function (e) {
      $ta.val(assemble(host));
      if (validate(host).length) {
        e.preventDefault();
        $('html, body').animate({ scrollTop: host.offset().top - 80 }, 200);
        return false;
      }
    });

    sync();
  });
})(jQuery);
