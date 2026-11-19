// Red "no letter" note under the status on the search page (js/search-letter-note.js).
// Run from the Drupal root: node sites/all/modules/dh_manageapp/tests/test-search-letter-note.js
var assert = require("assert");
var ln = require("../js/search-letter-note.js");
var pass = 0, fail = 0;
function t(label, fn) {
  try { fn(); pass++; console.log("  ok   " + label); }
  catch (e) { fail++; console.log("  FAIL " + label + " (" + e.message + ")"); }
}
var letters = {
  "Confirmed": [{ id: 1, status: "Acceptance 10 Day", type: 6 }, { id: 2, status: "Acceptance All", type: 0 }],
  "Cancelled": [{ id: 3, status: "Cancel 10 Day", type: 6 }],
  "waitlist ": [{ id: 4, status: "Wait", type: 0 }],
  "Regret": [{ id: 5, status: "Regret empty", type: 0, nosubj: 1 }],
  "Review": [{ id: 6, status: "Review letter", type: 0 }]
};
t("a letter for the course type or for All fits", function () {
  assert.deepStrictEqual(ln.fitting(letters, "Confirmed", 6).map(function (x) { return x.id; }), [1, 2]);
  assert.deepStrictEqual(ln.fitting(letters, "Confirmed", 10).map(function (x) { return x.id; }), [2]);
});
t("no letter for that course type: note with status and type name", function () {
  assert.strictEqual(ln.note(letters, "Cancelled", 10, "20 Day Course"), "No letter for Cancelled – 20 Day Course. The applicant will get no letter.");
});
t("a letter exists: no note", function () {
  assert.strictEqual(ln.note(letters, "Cancelled", 6, "10 Day Course"), "");
  assert.strictEqual(ln.note(letters, "Confirmed", 99, "X"), "");
});
t("status compared without case and end spaces, as the database does", function () {
  assert.strictEqual(ln.note(letters, "WaitList", 6, "10 Day Course"), "");
});
t("a letter with an empty subject counts as no letter", function () {
  assert.notStrictEqual(ln.note(letters, "Regret", 6, "10 Day Course"), "");
});
t("no status at all: note", function () {
  assert.notStrictEqual(ln.note(letters, "Duplicate", 6, "10 Day Course"), "");
});
t("Review, Custom and an empty choice never get a note", function () {
  assert.strictEqual(ln.note({}, "Review", 6, "x"), "");
  assert.strictEqual(ln.note({}, "Custom", 6, "x"), "");
  assert.strictEqual(ln.note({}, "", 6, "x"), "");
});
t("no course type on the row: no note", function () {
  assert.strictEqual(ln.note({}, "Confirmed", undefined, "x"), "");
});
t("no letters at all (PHP sends []): note", function () {
  assert.notStrictEqual(ln.note([], "Confirmed", 6, "10 Day Course"), "");
});
t("unknown course type name: generic words", function () {
  assert.ok(ln.note({}, "Confirmed", 77, undefined).indexOf("this course type") > 0);
});
t("Left and Clarification-Response are optional: no note (any case)", function () {
  assert.strictEqual(ln.note(letters, "Left", 6, "10 Day Course"), "");
  assert.strictEqual(ln.note(letters, "clarification-response ", 6, "10 Day Course"), "");
});
console.log("\n" + pass + " passed, " + fail + " failed");
process.exit(fail ? 1 : 0);
