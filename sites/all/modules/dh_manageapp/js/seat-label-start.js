/*
 * "Seat labels..." box on the seating plans (inc/seat-label-start.inc).
 * Each section button carries its plan, section and saved starts. The box saves
 * both grids of that section, then reloads the page, so the plan and the labels
 * saved on each student are drawn again with the new start.
 */
(function ($) {
  var cfg = window.dhSeatLabelCfg || {};
  var conv = parseInt(cfg.conv || 0, 10);
  function letter(n) { var s = ""; n = parseInt(n, 10); do { s = String.fromCharCode(65 + (n % 26)) + s; n = Math.floor(n / 26) - 1; } while (n >= 0); return s; }
  function trim(t) { return String(t).replace(/^\s+|\s+$/g, ""); }
  function letterIndex(t) { t = trim(t).toUpperCase(); if (!/^[A-Z]{1,3}$/.test(t)) return -1; var n = 0; for (var i = 0; i < t.length; i++) n = n * 26 + (t.charCodeAt(i) - 64); return n - 1; }
  /* Typed text to the offset: -1 = not allowed, empty = 0. Same rules as dh_seat_label_start_parse(). */
  function parse(t) { t = trim(t); if (t === "") return 0; if (conv === 1) return letterIndex(t); if (!/^[0-9]{1,5}$/.test(t) || parseInt(t, 10) < 1) return -1; return parseInt(t, 10) - 1; }
  /* The first three seats of the front row with this start. */
  function sample(start) { var out = []; for (var c = 0; c < 3; c++) out.push(conv === 1 ? letter(c + start) + "1" : (conv === 2 ? "A" + (c + 1 + start) : String(c + 1 + start))); return out.join(", ") + " ..."; }
  var hint = conv === 1 ? "Type the letter of the first column, for example I." : (conv === 2 ? "Type the number of the first column, for example 7." : "Type the first seat number, for example 101.");
  function esc(t) { return $("<div>").text(t).html(); }
  $("head").append("<style>.dh-sls-box{position:fixed;top:80px;left:50%;transform:translateX(-50%);z-index:10000;background:#fff;border:2px solid #2e7d32;border-radius:6px;padding:12px 16px;box-shadow:0 4px 16px rgba(0,0,0,.3);max-width:92vw;font:14px sans-serif}.dh-sls-box h4{margin:0 0 6px}.dh-sls-row{margin:8px 0}.dh-sls-sample{color:#555;margin-left:6px}.dh-sls-bad,.dh-sls-err{color:#b00020}</style>");
  function field(name, title, value) {
    return "<div class=\"dh-sls-row\"><label>" + title + " <input type=\"text\" size=\"6\" maxlength=\"5\" name=\"" + name + "\" value=\"" + esc(value) + "\"></label><span class=\"dh-sls-sample\"></span></div>";
  }
  function refresh($box) {
    var ok = true;
    $box.find("input").each(function () {
      var s = parse(this.value), $smp = $(this).closest(".dh-sls-row").find(".dh-sls-sample");
      if (s < 0) { ok = false; $smp.text("Not valid. " + hint).addClass("dh-sls-bad"); }
      else $smp.text("First seats: " + sample(s)).removeClass("dh-sls-bad");
    });
    $box.find(".dh-sls-save").prop("disabled", !ok);
  }
  function close() { $(".dh-sls-box").remove(); }
  function save($box, $btn, main, chowky) {
    var n = window.dhSeatPendingCount ? window.dhSeatPendingCount() : 0;
    if (n > 0 && !confirm("You have " + n + (n == 1 ? " unsaved seat change" : " unsaved seat changes") + " on this page. The page reloads after saving, so they will be lost.\n\nContinue?")) return;
    $box.find("button").prop("disabled", true);
    $.post(cfg.url, { token: cfg.token, plan: $btn.attr("data-plan"), sec: $btn.attr("data-sec"), main: main, chowky: chowky }, null, "json")
      .done(function (r) {
        if (r && r.status === "OK") { window.location.hash = $btn.attr("data-anchor"); window.location.reload(); return; }
        $box.find(".dh-sls-err").text(r && r.msg ? r.msg : "Not saved. Please try again.");
        $box.find("button").prop("disabled", false);
      })
      .fail(function () { $box.find(".dh-sls-err").text("Not saved: no reply from DIPI. Please try again."); $box.find("button").prop("disabled", false); });
  }
  $(document).on("click", ".dh-sls-btn", function () {
    close();
    var $btn = $(this);
    var $box = $("<div class=\"dh-sls-box no-print\" role=\"dialog\"></div>")
      .append("<h4>Seat labels: " + esc($btn.attr("data-label")) + "</h4>")
      .append("<p>" + hint + " Leave it empty for the normal start. This course only.</p>")
      .append(field("main", "Main grid starts at", $btn.attr("data-main") || ""))
      .append(field("chowky", "Chowky grid starts at", $btn.attr("data-chowky") || ""))
      .append("<p class=\"dh-sls-err\"></p>")
      .append("<button type=\"button\" class=\"dh-sls-save\">Save</button> <button type=\"button\" class=\"dh-sls-reset\">Reset to normal</button> <button type=\"button\" class=\"dh-sls-cancel\">Cancel</button>");
    $("body").append($box);
    refresh($box);
    $box.on("input", "input", function () { refresh($box); });
    $box.on("click", ".dh-sls-cancel", close);
    $box.on("click", ".dh-sls-save", function () { save($box, $btn, $box.find("input[name=main]").val(), $box.find("input[name=chowky]").val()); });
    $box.on("click", ".dh-sls-reset", function () { save($box, $btn, "", ""); });
    $box.find("input[name=main]").focus();
  });
  $(document).on("keydown", function (e) { if (e.keyCode === 27) close(); });
})(jQuery);
