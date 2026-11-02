// Age groups for the search results table (js/search-age-groups.js).
// Run from the Drupal root: node sites/all/modules/dh_manageapp/tests/test-search-age-groups.js
var assert = require("assert");
var ag = require("../js/search-age-groups.js");
var pass = 0, fail = 0;
function t(label, fn) {
  try { fn(); pass++; console.log("  ok   " + label); }
  catch (e) { fail++; console.log("  FAIL " + label + " (" + e.message + ")"); }
}
function labels(list) { return list.map(function (g) { return g.label + ":" + g.count; }).join(" "); }

t("ten-year groups, 1-10 then 11-20", function () {
  assert.strictEqual(labels(ag.groups([1, 10, 11, 20, 21])), "1-10:2 11-20:2 21-30:1");
});
t("only groups that have people, in age order", function () {
  assert.strictEqual(labels(ag.groups([65, 34, 31, 72, 40])), "31-40:3 61-70:1 71-80:1");
});
t("ages as text (as the page data has them)", function () {
  assert.strictEqual(labels(ag.groups(["34", " 35 ", "8"])), "1-10:1 31-40:2");
});
t("missing, empty, zero, negative and junk ages are Unknown, listed last", function () {
  assert.strictEqual(labels(ag.groups([null, undefined, "", 0, "0", -3, "-3", "abc", "3.5", 25])), "21-30:1 Unknown:9");
});
t("odd high ages keep their own group", function () {
  assert.strictEqual(labels(ag.groups([125, 101, 100])), "91-100:1 101-110:1 121-130:1");
});
t("no rows gives no groups", function () {
  assert.strictEqual(ag.groups([]).length, 0);
});
t("update never throws, even with a broken table", function () {
  ag.update(null);
  ag.update({ rows: function () { throw new Error("boom"); } });
});

console.log("\n" + pass + " passed, " + fail + " failed");
process.exit(fail ? 1 : 0);
