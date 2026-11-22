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
console.log("\n" + pass + " passed, " + fail + " failed");
process.exit(fail ? 1 : 0);
