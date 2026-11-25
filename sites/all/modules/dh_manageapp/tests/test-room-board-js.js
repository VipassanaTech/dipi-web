// Room Board script (js/room-board.js): every function the board calls must be defined in the file.
// Guards the 21 Nov 2026 slip where save() was deleted with the old print code, so no board action worked.
// Run from the Drupal root: node sites/all/modules/dh_manageapp/tests/test-room-board-js.js
var assert = require("assert");
var src = require("fs").readFileSync(__dirname + "/../js/room-board.js", "utf8");
var pass = 0, fail = 0;
function t(label, fn) {
  try { fn(); pass++; console.log("  ok   " + label); }
  catch (e) { fail++; console.log("  FAIL " + label + " (" + e.message + ")"); }
}
t("the script parses", function () { new Function(src); });
t("save() is defined", function () { assert.ok(/function save\(payload\)/.test(src)); });
["place", "swap", "remove"].forEach(function (op) {
  t("the '" + op + "' action calls save()", function () { assert.ok(new RegExp("save\\(\\{op: '" + op + "'").test(src)); });
});
t("every function called by name is defined in the file", function () {
  var called = {}, defined = {};
  (src.match(/\b([a-zA-Z_]\w*)\(/g) || []).forEach(function (m) { called[m.slice(0, -1)] = 1; });
  (src.match(/function ([a-zA-Z_]\w*)\(/g) || []).forEach(function (m) { defined[m.slice(9, -1)] = 1; });
  (src.match(/var ([a-zA-Z_]\w*) = function/g) || []).forEach(function (m) { defined[m.slice(4).split(" ")[0]] = 1; });
  // built-ins, jQuery, browser and Drupal names the script uses without defining them
  var known = "function if for while switch return typeof catch $ jQuery alert confirm setTimeout clearTimeout setInterval Date parseInt String Number Object Array JSON Math encodeURIComponent".split(" ");
  var missing = Object.keys(called).filter(function (n) {
    if (defined[n] || known.indexOf(n) >= 0) return false;
    // method calls (.name() are not free functions
    return new RegExp("(^|[^.\\w])" + n + "\\(").test(src);
  });
  assert.deepStrictEqual(missing, []);
});
// No jumping (owner, 25 Nov 2026): the Chosen bar is always drawn (fixed height in css), and a redraw keeps the
// last tapped thing (or the first visible one) at the same spot on the screen.
t("the Chosen bar is always drawn, also when nobody is chosen", function () {
  assert.ok(/dhrb-chosen dhrb-pin dhrb-none/.test(src));
  var css = require("fs").readFileSync(__dirname + "/../css/rooms.css", "utf8");
  assert.ok(/\.dhrb-chosen \{ height: \d+px;/.test(css));
});
t("every redraw keeps what is on the screen in place", function () {
  assert.ok(/function render\(visibleOnly\) \{\s*var keep = anchors\(visibleOnly === true\);\s*draw\(\);\s*backInPlace\(keep\);/.test(src));
  assert.ok(/render\(true\);/.test(src), "the self-refresh keeps the visible part in place");
});
// Confirm every change (owner, 25 Nov 2026): place / move, swap and take out each ask first, naming person + room.
["place", "swap", "remove"].forEach(function (op) {
  t("the '" + op + "' action asks for a yes before saving", function () {
    var at = src.indexOf("save({op: '" + op + "'");
    assert.ok(at > 0);
    var before = src.slice(Math.max(0, at - 400), at);
    assert.ok(/if \(!(c \|\| !)?sure\(/.test(before) || /\|\| !sure\(/.test(before), "no sure(...) right before save " + op);
  });
});
t("the room changes log is under the board, collapsed by default, searchable (DataTables)", function () {
  assert.ok(/draw\(\);\s*backInPlace\(keep\);\s*renderLog\(\);/.test(src), "drawn after every board redraw");
  assert.ok(/var logOpen = false/.test(src), "collapsed by default");
  assert.ok(/\$\('#dh-rb-log'\)/.test(src) && !/#dh-rb-app[^\n]*dhrb-log\b/.test(src), "in its own box, outside the board");
  assert.ok(/\.DataTable\(\{/.test(src) && /logDT\.clear\(\)\.rows\.add\(rows\)\.draw\(false\)/.test(src), "DataTables, updated in place");
  assert.ok(/<th>Old room<\/th><th>New room<\/th><th>By<\/th>/.test(src));
  assert.ok(/render: txt/.test(src) && /function txt\(d, type\) \{ return type === 'display' \? esc\(d\) : d; \}/.test(src), "names shown as plain text");
});
// Placing many people (owner, 25 Nov 2026): the people list stays in view on a computer (the automatic next person
// was tried and left out by the owner).
t("the people list stays in view and scrolls on its own (computer)", function () {
  assert.ok(/<div class="dhrb-main"><div class="dhrb-people">/.test(src));
  var css = require("fs").readFileSync(__dirname + "/../css/rooms.css", "utf8");
  assert.ok(/@media \(min-width: 701px\) \{\s*\.dhrb-people \{ position: sticky;[^}]*overflow-y: auto;/.test(css));
  assert.ok(/var peopleTop = \$\('#dh-rb-app \.dhrb-people'\)\.scrollTop\(\)[\s\S]*\.html\(h\);\s*\$\('#dh-rb-app \.dhrb-people'\)\.scrollTop\(peopleTop\);/.test(src), "its scroll survives redraws");
});
// A search opens matching sections once; it must never keep a section from closing (owner, 25 Nov 2026).
t("a search never forces a section open (Close all and the header still close it)", function () {
  assert.ok(/var isOpen = !!open\[side \+ '\|' \+ sec\];/.test(src));
  assert.ok(/q = \$\(this\)\.val\(\); var pos = this\.selectionStart; openHits\(\); render\(\);/.test(src));
});
console.log("\n" + pass + " passed, " + fail + " failed");
process.exit(fail ? 1 : 0);
