/* Age groups line above the search results table (#table-applicants).
 * Counts the rows the table shows after its search box and filters (all pages, not only
 * the current one), in groups of ten years: 1-10, 11-20, ... Only groups with people are
 * listed. The age is the table's Age column (age today). No age, or an age below 1, is "Unknown".
 */
(function (root) {
  "use strict";

  // ages: the Age values of the shown rows. Returns [{label, count}] in age order, "Unknown" last.
  function groups(ages) {
    var by = {}, unknown = 0;
    for (var i = 0; i < ages.length; i++) {
      var s = (ages[i] === null || ages[i] === undefined) ? "" : String(ages[i]).trim();
      var a = /^\d+$/.test(s) ? parseInt(s, 10) : 0;
      if (a < 1) { unknown++; continue; }
      var g = Math.floor((a - 1) / 10);
      by[g] = (by[g] || 0) + 1;
    }
    var out = Object.keys(by).map(Number).sort(function (x, y) { return x - y; }).map(function (g) {
      return { label: (g * 10 + 1) + "-" + (g * 10 + 10), count: by[g] };
    });
    if (unknown) out.push({ label: "Unknown", count: unknown });
    return out;
  }

  // Called from the table's redraw step with its DataTables API. Never throws, so the table
  // keeps working even if something here goes wrong.
  function update(api) {
    try {
      var $ = root.jQuery;
      var ages = api.rows({ search: "applied" }).data().pluck("age").toArray();
      var $box = $("#dh-age-groups");
      if (!$box.length) {
        $box = $("<div id=\"dh-age-groups\" class=\"dh-age-groups\"></div>");
        $(api.table().node()).before($box);
      }
      if (!ages.length) { $box.hide().empty(); return; }
      $box.empty().append($("<b></b>").text("Age-wise count:"));
      groups(ages).forEach(function (g) {
        $box.append(" ").append($("<span class=\"dh-age-group\"></span>").text(g.label + ": " + g.count));
      });
      $box.show();
    } catch (e) {
      if (root.console && root.console.warn) root.console.warn("Age groups:", e);
    }
  }

  var dhAgeGroups = { groups: groups, update: update };
  if (typeof module !== "undefined" && module.exports) module.exports = dhAgeGroups;
  else root.dhAgeGroups = dhAgeGroups;
})(typeof window !== "undefined" ? window : this);
