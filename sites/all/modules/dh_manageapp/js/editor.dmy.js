/**
 * DIPI: a date as three dropdowns (Day / Month / Year) for DataTables Editor 1.7 — field type "dmy".
 *
 * Why: on a phone, Editor's popup hides the rest of the page (it moves it into a hidden box), and the
 * jQuery UI calendar is moved with it, so a "date" field cannot be picked. Dropdowns need no popup and
 * work the same on every phone and computer; the month is shown as a name, so day and month cannot be
 * mixed up.
 *
 * The field reads and sends the same text the server already uses: "dd Mon yyyy" (e.g. "01 Dec 2026"),
 * or "" when day, month or year is not chosen (the server then fills its default, as before).
 *
 * Options (in the field definition):
 *   years: years shown before and after the current year (default 4). A saved year outside that range
 *          is added to the list, so editing an old course keeps its date.
 * An empty field shows Day, Month and Year all blank, so nothing is chosen by accident.
 */
(function ($, DataTable) {
	'use strict';

	var MON = ['Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun', 'Jul', 'Aug', 'Sep', 'Oct', 'Nov', 'Dec'];

	function pad(n) { return (n < 10 ? '0' : '') + n; }

	/* Days in month m (1-12) of year y. */
	function daysIn(m, y) { return new Date(y, m, 0).getDate(); }

	/* "01 Dec 2026", "1 Dec 2026" or "2026-12-01" -> {d, m, y}; anything else, or an impossible date -> null. */
	function parse(v) {
		v = $.trim ? $.trim(String(v == null ? '' : v)) : String(v == null ? '' : v).trim();
		var d, m, y, a;
		if ((a = /^(\d{1,2}) ([A-Za-z]{3}) (\d{4})$/.exec(v))) {
			d = +a[1]; y = +a[3];
			m = MON.map(function (s) { return s.toLowerCase(); }).indexOf(a[2].toLowerCase()) + 1;
		}
		else if ((a = /^(\d{4})-(\d{2})-(\d{2})/.exec(v))) {
			y = +a[1]; m = +a[2]; d = +a[3];
		}
		else return null;
		if (!m || m > 12 || !d || d > daysIn(m, y)) return null;
		return { d: d, m: m, y: y };
	}

	/* The server's form: "01 Dec 2026". */
	function format(d, m, y) { return pad(d) + ' ' + MON[m - 1] + ' ' + y; }

	/* The year list: span years each side of the current year, plus a saved year outside it. */
	function years(current, span, keep) {
		var out = [];
		for (var y = current - span; y <= current + span; y++) out.push(y);
		if (keep && out.indexOf(keep) < 0) { out.push(keep); out.sort(function (a, b) { return a - b; }); }
		return out;
	}

	if (typeof window !== 'undefined') window.dhDmy = { parse: parse, format: format, daysIn: daysIn, years: years };

	if (!DataTable.ext.editorFields) DataTable.ext.editorFields = {};
	var types = DataTable.Editor ? DataTable.Editor.fieldTypes : DataTable.ext.editorFields;

	function fillDays(conf) {
		var keep = +conf._d.val(), m = +conf._m.val(), y = +conf._y.val();
		var n = (m && y) ? daysIn(m, y) : 31;
		var opts = conf._d[0].options;
		opts.length = 0;
		opts[0] = new Option('Day', '');
		for (var i = 1; i <= n; i++) opts[i] = new Option(String(i), String(i));
		conf._d.val(keep && keep <= n ? String(keep) : '');
	}

	function fillYears(conf, keep) {
		var opts = conf._y[0].options, list = years(new Date().getFullYear(), conf.years || 4, keep);
		opts.length = 0;
		opts[0] = new Option('Year', '');
		for (var i = 0; i < list.length; i++) opts[i + 1] = new Option(String(list[i]), String(list[i]));
	}

	types.dmy = $.extend(true, {}, DataTable.Editor ? DataTable.Editor.models.fieldType : {}, {
		create: function (conf) {
			var id = DataTable.Editor.safeId(conf.id);
			conf._d = $('<select/>').attr({ id: id, 'aria-label': 'Day', 'class': 'dh-dmy-day' });
			conf._m = $('<select/>').attr({ id: id + '-month', 'aria-label': 'Month', 'class': 'dh-dmy-month' });
			conf._y = $('<select/>').attr({ id: id + '-year', 'aria-label': 'Year', 'class': 'dh-dmy-year' });
			var mo = conf._m[0].options;
			mo[0] = new Option('Month', '');
			for (var i = 0; i < 12; i++) mo[i + 1] = new Option(MON[i], String(i + 1));
			fillYears(conf, null);
			fillDays(conf);
			conf._m.add(conf._y).on('change', function () { fillDays(conf); });
			conf._input = $('<div class="dh-dmy"/>').append(conf._d, conf._m, conf._y);
			return conf._input[0];
		},

		get: function (conf) {
			var d = +conf._d.val(), m = +conf._m.val(), y = +conf._y.val();
			return (d && m && y) ? format(d, m, y) : '';
		},

		set: function (conf, val) {
			var p = parse(val);
			fillYears(conf, p ? p.y : null);
			conf._y.val(p ? String(p.y) : '');
			conf._m.val(p ? String(p.m) : '');
			conf._d.val('');
			fillDays(conf);
			if (p) conf._d.val(String(p.d));
		},

		enable: function (conf) { conf._input.find('select').prop('disabled', false); },
		disable: function (conf) { conf._input.find('select').prop('disabled', true); },
		focus: function (conf) { conf._d.focus(); },
		canReturnSubmit: function () { return false; }
	});
}(jQuery, jQuery.fn.dataTable));
