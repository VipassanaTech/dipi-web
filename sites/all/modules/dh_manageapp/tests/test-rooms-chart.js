// Room chart layout (js/rooms-common.js, dhRoomChart): rooms -> printable lines, two bed boxes per line,
// single rooms paired side by side. Pure functions, no browser needed.
// Run from the Drupal root: node sites/all/modules/dh_manageapp/tests/test-rooms-chart.js
var assert = require("assert");
var chart = require("../js/rooms-common.js");
var pass = 0, fail = 0;
function t(label, fn) {
  try { fn(); pass++; console.log("  ok   " + label); }
  catch (e) { fail++; console.log("  FAIL " + label + " (" + e.message + ")"); }
}
function room(name, beds, toilet) {
  return {name: name, toilet: toilet || "Indian", beds: beds.map(function (b) { return typeof b === "string" ? {name: b, text: ""} : b; })};
}
function lines(blocks) { return blocks.reduce(function (n, b) { return n + (b.kind === "multi" ? b.lines.length : 1); }, 0); }
function count(s, re) { return (s.match(re) || []).length; }
function esc(s) { return String(s).replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;").replace(/"/g, "&quot;"); }

t("a room with 2 beds is one line, two boxes", function () {
  var b = chart.layout([room("12", ["12A", "12B"])]);
  assert.strictEqual(b.length, 1);
  assert.strictEqual(b[0].kind, "multi");
  assert.strictEqual(lines(b), 1);
  assert.strictEqual(b[0].lines[0][0].name, "12A");
  assert.strictEqual(b[0].lines[0][1].name, "12B");
});
t("a room with 3 beds is 2 lines, the second has one empty box", function () {
  var b = chart.layout([room("7", ["7A", "7B", "7C"])]);
  assert.strictEqual(lines(b), 2);
  assert.strictEqual(b[0].lines[1][0].name, "7C");
  assert.strictEqual(b[0].lines[1][1], null);
});
t("a room with 5 beds is 3 lines", function () {
  assert.strictEqual(lines(chart.layout([room("9", ["a", "b", "c", "d", "e"])])), 3);
});
t("5 single rooms are 3 lines, the last has an empty right half", function () {
  var b = chart.layout([1, 2, 3, 4, 5].map(function (n) { return room(String(n), [String(n)]); }));
  assert.strictEqual(lines(b), 3);
  assert.deepStrictEqual(b.map(function (x) { return x.kind; }), ["single", "single", "single"]);
  assert.strictEqual(b[0].left.name, "1");
  assert.strictEqual(b[0].right.name, "2");
  assert.strictEqual(b[2].left.name, "5");
  assert.strictEqual(b[2].right, null);
});
t("single, double, single keeps the order and does not pair across the double", function () {
  var b = chart.layout([room("1", ["1"]), room("2", ["2A", "2B"]), room("3", ["3"])]);
  assert.deepStrictEqual(b.map(function (x) { return x.kind; }), ["single", "multi", "single"]);
  assert.strictEqual(b[0].right, null);
  assert.strictEqual(b[2].right, null);
  assert.strictEqual(b[1].name, "2");
});
t("two singles, a double, two singles: pairs on each side", function () {
  var b = chart.layout([room("1", ["1"]), room("2", ["2"]), room("3", ["3A", "3B"]), room("4", ["4"]), room("5", ["5"])]);
  assert.deepStrictEqual(b.map(function (x) { return x.kind; }), ["single", "multi", "single"]);
  assert.strictEqual(b[0].right.name, "2");
  assert.strictEqual(b[2].right.name, "5");
});
t("a room with no beds is left out", function () {
  assert.strictEqual(chart.layout([room("1", [])]).length, 0);
});
t("each block keeps its room name and toilet", function () {
  var b = chart.layout([room("12", ["12A", "12B"], "Western"), room("1", ["1"], "Both")]);
  assert.strictEqual(b[0].name, "12");
  assert.strictEqual(b[0].toilet, "Western");
  assert.strictEqual(b[1].left.name, "1");
  assert.strictEqual(b[1].left.toilet, "Both");
});

t("a 3-bed room: room name and toilet once, spanning its 2 lines", function () {
  var h = chart.tableHtml([room("7", ["7A", "7B", "7C"])], esc);
  assert.strictEqual(count(h, /rowspan="2"/g), 2);
  assert.strictEqual(count(h, /<tbody>/g), 1);
  assert.strictEqual(count(h, /<tr>/g), 2);   // 2 lines, no header row
});
t("each room is its own tbody (a room never splits across pages)", function () {
  var h = chart.tableHtml([room("1", ["1A", "1B"]), room("2", ["2A", "2B"]), room("3", ["3"]), room("4", ["4"])], esc);
  assert.strictEqual(count(h, /<tbody/g), 3);
  assert.strictEqual(count(h, /<thead/g), 0);   // no column header row
});
t("a free bed shows only its bed name, in bold", function () {
  var h = chart.tableHtml([room("12", ["12A", "12B"])], esc);
  assert.ok(h.indexOf('<b>12A</b></td>') >= 0);
});
t("a filled bed shows the bed name, then confirmation number and name", function () {
  var h = chart.tableHtml([room("12", [{name: "12A", text: "OM12 Ramesh Patil"}, "12B"])], esc);
  assert.ok(h.indexOf('<b>12A</b> OM12 Ramesh Patil</td>') >= 0);
});
t("several people in one bed are comma separated", function () {
  var h = chart.tableHtml([room("12", [{name: "12A", text: "OM1 A B, OM2 C D"}, "12B"])], esc);
  assert.ok(h.indexOf("OM1 A B, OM2 C D") >= 0);
});
t("text is escaped", function () {
  var h = chart.tableHtml([room("<b>", [{name: "A&B", text: "<i>x</i>"}, "2"])], esc);
  assert.ok(h.indexOf("<i>") < 0 && h.indexOf("A&amp;B") >= 0 && h.indexOf("&lt;i&gt;x") >= 0);
});
t("an odd last single room leaves its right half empty", function () {
  var h = chart.tableHtml([room("1", ["1"])], esc);
  assert.ok(h.indexOf('class="none"') >= 0);
});

t("section heading: code, name when set, shared, counts", function () {
  assert.strictEqual(chart.heading("M", "Male Common Block", false, "5 of 8 beds taken", esc), "Section M — Male Common Block · 5 of 8 beds taken");
  assert.strictEqual(chart.heading("M", "", false, "2 rooms · 4 beds", esc), "Section M · 2 rooms · 4 beds");
  assert.strictEqual(chart.heading("S", "Hall", true, "1 of 2 beds taken", esc), "Section S — Hall (Men + Women) · 1 of 2 beds taken");
});
t("sectionsHtml: one heading and one table per section; sections with no rooms are skipped", function () {
  var h = chart.sectionsHtml([
    {code: "M", name: "Block", shared: false, count: "1 room · 2 beds", rooms: [room("1", ["1A", "1B"])]},
    {code: "X", name: "", shared: false, count: "", rooms: []}
  ], esc);
  assert.strictEqual(count(h, /<h2>/g), 1);
  assert.strictEqual(count(h, /<table/g), 1);
  assert.ok(h.indexOf("Section M — Block") >= 0);
});

console.log("\n" + pass + " passed, " + fail + " failed");
process.exit(fail ? 1 : 0);
