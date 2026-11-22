// Room chart print (js/rooms-common.js, dhRoomChart): rooms -> room cards like the Room Board, four in a row,
// each card = room name + toilet + a box per bed. Pure functions, no browser needed.
// Run from the Drupal root: node sites/all/modules/dh_manageapp/tests/test-rooms-chart.js
var assert = require("assert");
var chart = require("../js/rooms-common.js");
var pass = 0, fail = 0;
function t(label, fn) {
  try { fn(); pass++; console.log("  ok   " + label); }
  catch (e) { fail++; console.log("  FAIL " + label + " (" + e.message + ")"); }
}
function room(name, beds, toilet) {
  return {name: name, toilet: toilet === undefined ? "Indian" : toilet, beds: beds.map(function (b) { return typeof b === "string" ? {name: b, text: ""} : b; })};
}
function rooms(n) { var a = []; for (var i = 1; i <= n; i++) a.push(room(String(i), [String(i)])); return a; }
function count(s, re) { return (s.match(re) || []).length; }
function esc(s) { return String(s).replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;").replace(/"/g, "&quot;"); }

t("four room cards in a row", function () {
  var h = chart.cardsHtml(rooms(4), esc);
  assert.strictEqual(chart.COLS, 4);
  assert.strictEqual(count(h, /<tr>/g), 1);
  assert.strictEqual(count(h, /<td class="card">/g), 4);
  assert.strictEqual(count(h, /class="nc"/g), 0);
});
t("5 rooms are 2 rows; the last row is filled up with empty cells", function () {
  var h = chart.cardsHtml(rooms(5), esc);
  assert.strictEqual(count(h, /<tr>/g), 2);
  assert.strictEqual(count(h, /<td class="card">/g), 5);
  assert.strictEqual(count(h, /<td class="nc"><\/td>/g), 3);
});
t("rooms keep their order (single and double rooms mixed, as on the Room Board)", function () {
  var h = chart.cardsHtml([room("1", ["1"]), room("2", ["2A", "2B"]), room("3", ["3"])], esc);
  assert.ok(h.indexOf("<b>1</b>") < h.indexOf("<b>2</b>") && h.indexOf("<b>2</b>") < h.indexOf("<b>3</b>"));
});
t("a room with no beds is left out", function () {
  assert.strictEqual(chart.cardsHtml([room("1", [])], esc), "");
  assert.strictEqual(count(chart.cardsHtml([room("1", []), room("2", ["2"])], esc), /<td class="card">/g), 1);
});
t("every bed is its own box in the card", function () {
  var h = chart.cardsHtml([room("7", ["7A", "7B", "7C"])], esc);
  assert.strictEqual(count(h, /<div class="pl /g), 3);
});
t("a free bed shows only its bed name (room to write by hand)", function () {
  var h = chart.cardsHtml([room("12", ["12A", "12B"])], esc);
  assert.ok(h.indexOf('<div class="pl free"><b>12A</b></div>') >= 0);
});
t("a filled bed shows the bed name, then confirmation number and name", function () {
  var h = chart.cardsHtml([room("12", [{name: "12A", text: "OM12 Ramesh Patil"}, "12B"])], esc);
  assert.ok(h.indexOf('<div class="pl taken"><b>12A</b> OM12 Ramesh Patil</div>') >= 0);
});
t("a bed of another course is marked as such", function () {
  var h = chart.cardsHtml([room("12", [{name: "12A", text: "Course 9", st: "other"}])], esc);
  assert.ok(h.indexOf('<div class="pl other"><b>12A</b> Course 9</div>') >= 0);
});
t("several people in one bed are comma separated", function () {
  var h = chart.cardsHtml([room("12", [{name: "12A", text: "OM1 A B, OM2 C D"}, "12B"])], esc);
  assert.ok(h.indexOf("OM1 A B, OM2 C D") >= 0);
});
t("the card head: room name, then the toilet when it is set", function () {
  assert.ok(chart.cardsHtml([room("12", ["12A"], "Western")], esc).indexOf('<div class="rh"><b>12</b> <span class="tl">Western</span></div>') >= 0);
  assert.ok(chart.cardsHtml([room("12", ["12A"], "")], esc).indexOf('<div class="rh"><b>12</b></div>') >= 0);
});
t("text is escaped", function () {
  var h = chart.cardsHtml([room("<b>", [{name: "A&B", text: "<i>x</i>"}, "2"], "<u>")], esc);
  assert.ok(h.indexOf("<i>") < 0 && h.indexOf("<u>") < 0 && h.indexOf("A&amp;B") >= 0 && h.indexOf("&lt;i&gt;x") >= 0);
});

t("section heading: code, name when set, shared, counts", function () {
  assert.strictEqual(chart.heading("M", "Male Common Block", false, "5 of 8 beds taken", esc), "Section M — Male Common Block · 5 of 8 beds taken");
  assert.strictEqual(chart.heading("M", "", false, "2 rooms · 4 beds", esc), "Section M · 2 rooms · 4 beds");
  assert.strictEqual(chart.heading("S", "Hall", true, "1 of 2 beds taken", esc), "Section S — Hall (Male + Female) · 1 of 2 beds taken");
});
t("sectionsHtml: one heading and one card table per section; sections with no beds are skipped", function () {
  var h = chart.sectionsHtml([
    {code: "M", name: "Block", shared: false, count: "1 room · 2 beds", rooms: [room("1", ["1A", "1B"])]},
    {code: "X", name: "", shared: false, count: "", rooms: [room("9", [])]}
  ], esc);
  assert.strictEqual(count(h, /<h2>/g), 1);
  assert.strictEqual(count(h, /<table class="cards"/g), 1);
  assert.ok(h.indexOf("Section M — Block") >= 0);
});

console.log("\n" + pass + " passed, " + fail + " failed");
process.exit(fail ? 1 : 0);
