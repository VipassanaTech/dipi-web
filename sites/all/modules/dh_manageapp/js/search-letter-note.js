/* Search page: a red note under the status when the chosen status has no letter for the
 * applicant's course type, e.g. "No letter for Cancelled – 10 Day Course. The applicant will
 * get no letter." Same rule as the server (dh_get_letter): an active letter of that status for
 * the course type or for All, with a subject. The page lists only active letters. Status names
 * are compared without upper/lower case and end spaces, as the database does.
 */
(function (root) {
  "use strict";

  function norm(s) {
    return String(s === null || s === undefined ? "" : s).replace(/\s+$/, "").toLowerCase();
  }

  // letters: {event: [{id, status, type, nosubj}]}. The letters DIPI could send for status + course type ct.
  function fitting(letters, status, ct) {
    var out = [], want = norm(status);
    if (!letters || typeof letters !== "object" || want === "") return out;
    for (var ev in letters) {
      if (!Object.prototype.hasOwnProperty.call(letters, ev) || norm(ev) !== want) continue;
      var list = letters[ev];
      if (!list || typeof list.length !== "number") continue;
      for (var i = 0; i < list.length; i++) {
        var x = list[i];
        if (x && (x.type == 0 || x.type == ct) && !x.nosubj) out.push(x);
      }
    }
    return out;
  }

  // The note text, or "" when no note is due. Review and Custom never get one.
  function note(letters, status, ct, typeName) {
    var s = norm(status);
    if (s === "" || s === "review" || s === "custom") return "";
    if (ct === undefined || ct === null || ct === "") return "";
    if (fitting(letters, status, ct).length) return "";
    return "No letter for " + String(status) + " – " + (typeName ? String(typeName) : "this course type") +
      ". The applicant will get no letter.";
  }

  var dhLetterNote = { norm: norm, fitting: fitting, note: note };
  if (typeof module !== "undefined" && module.exports) module.exports = dhLetterNote;
  else root.dhLetterNote = dhLetterNote;
})(typeof window !== "undefined" ? window : this);
