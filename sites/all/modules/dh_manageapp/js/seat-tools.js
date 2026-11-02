/*
 * Seating plan tools, shared by the Seating Plan and the Group-wise Seating
 * Plan (loaded by dh_seat_tools_js() in inc/seat-special.inc):
 *  - "Swap / Move seats": a click mode per section. Click a student, then a seat
 *    in the same grid: the two swap, or the student moves into an empty seat.
 *    Only the page changes; "Store Seat Changes" saves it.
 *  - Student menu (the small button on each card): backrest on/off in the main
 *    grid (saved at once), and a move to the other grid (saved at once, then
 *    the page reloads).
 * Settings: window.dhSeatToolsCfg = { url: save URL, token: form token }.
 */
(function ($) {
  var MSG_ON = "Swap / Move is on: click a student, then a seat.";
  var MSG_GRID = "Pick a seat in the same grid. To move between the main and the chowky grid, use the student's menu.";
  var $sel = null, $menu = null;

  function cfg() { return window.dhSeatToolsCfg || {}; }
  function appId($li) { return $li.find(".s-app-id").first().attr("data-s-app-id") || ""; }
  function gridOf($li) { return $li.closest(".chowky-div").length ? "chowky" : "main"; }
  function nameOf($li) { var $n = $li.find(".s-name").first(); return $n.attr("title") || $.trim($n.text()); }
  function status($sec, text) { $sec.find(".dh-swap-status").text(text); }
  function clearSel() { if ($sel) $sel.removeClass("dh-swap-sel"); $sel = null; }
  function closeMenu() { if ($menu) { $menu.remove(); $menu = null; } }

  /* Exchange two cards; works for neighbours too. */
  function swapLi(a, b) {
    var tmp = document.createElement("li");
    a.parentNode.insertBefore(tmp, a);
    b.parentNode.insertBefore(a, b);
    tmp.parentNode.insertBefore(b, tmp);
    tmp.parentNode.removeChild(tmp);
  }

  /* ---- Swap / Move mode, one section at a time ---- */

  $(document).on("click", ".dh-swap-toggle", function () {
    var $sec = $(this).closest(".seat-section"), on = !$sec.hasClass("dh-swap-on");
    clearSel();
    $sec.toggleClass("dh-swap-on", on);
    $(this).toggleClass("dh-active", on);
    $sec.find("ul.ui-sortable").sortable(on ? "disable" : "enable");
    status($sec, on ? MSG_ON : "");
  });

  $(document).on("click", ".dh-swap-on ul.ui-sortable > li", function (e) {
    if ($(e.target).closest(".s-aid-btn, .s-special-btn").length) return;   // they open their own menus
    var $li = $(this), $sec = $li.closest(".seat-section");
    if (!$sel) {
      if (appId($li)) $sel = $li.addClass("dh-swap-sel");    // the first click must be on a student
      return;
    }
    if ($li[0] === $sel[0]) { clearSel(); return; }           // the same card again = cancel
    if ($li.parent()[0] !== $sel.parent()[0]) { status($sec, MSG_GRID); return; }
    swapLi($sel[0], $li[0]);
    clearSel();
    status($sec, MSG_ON);
    if (window.dhAidSync) window.dhAidSync();                  // CW/CH mark: copy the new seat number
  });

  /* ---- Student menu ---- */

  function ask(title, text, okLabel, cancelLabel, onOk) {
    var $ov = $('<div class="dh-modal-ov"></div>');
    var $box = $('<div class="dh-modal" role="dialog" aria-modal="true"></div>').appendTo($ov);
    $("<h3></h3>").text(title).appendTo($box);
    $("<p></p>").text(text).appendTo($box);
    var $btns = $('<div class="dh-modal-btns"></div>').appendTo($box);
    $('<button type="button" class="dh-modal-cancel"></button>').text(cancelLabel).appendTo($btns)
      .on("click", function () { $ov.remove(); });
    $btns.append(" ");
    $('<button type="button" class="dh-modal-ok"></button>').text(okLabel).appendTo($btns)
      .on("click", function () { $ov.remove(); onOk(); });
    $ov.on("click", function (e) { if (e.target === $ov[0]) $ov.remove(); });
    $ov.appendTo("body");
  }

  function save($li, set, done) {
    $.post(cfg().url, { a: appId($li), set: set, grid: gridOf($li), token: cfg().token }, function (j) {
      if (j && j.status === "OK") done(j);
      else alert("Could not save: " + ((j && j.msg) || "unknown error"));
    }, "json").fail(function () { alert("Could not save. Please try again."); });
  }

  function setBackrest($li, set) {
    save($li, set, function () {
      $li.find(".s-app-id").first().attr("data-special", set);
      $li.find(".s-backrest").first().toggleClass("s-backrest-yes", set === "backrest");
    });
  }

  function moveGrid($li, set, choice) {
    var n = window.dhSeatPendingCount ? window.dhSeatPendingCount() : 0;
    var to = (set === "chowky" || set === "chair") ? "the chowky grid" : "the main grid";
    var go = function () {
      // Path only: a reload with ?r=1 would regenerate the plan.
      save($li, set, function () { window.location.href = window.location.pathname; });
    };
    if (n > 0)
      ask("Move to " + to, "You have " + n + " seat change" + (n == 1 ? "" : "s") +
        " not stored yet. The page will reload and they will be lost.", "Move anyway", "Cancel, I will store them first", go);
    else
      ask("Move to " + to, "Move " + nameOf($li) + " to " + to + " (" + choice + ")? The page will reload.", "Move", "Cancel", go);
  }

  $(document).on("click", ".s-special-btn", function (e) {
    e.preventDefault(); e.stopPropagation();
    closeMenu();
    var $btn = $(this), $li = $btn.closest("li");
    if (!appId($li)) return;
    var sp = $li.find(".s-app-id").first().attr("data-special") || "none", items = [];
    if (gridOf($li) === "main") {
      if (sp === "backrest") items.push(["Remove backrest", function () { setBackrest($li, "none"); }]);
      else items.push(["Add backrest", function () { setBackrest($li, "backrest"); }]);
      items.push(["Move to chowky grid: Chowky", function () { moveGrid($li, "chowky", "Chowky"); }]);
      items.push(["Move to chowky grid: Chair", function () { moveGrid($li, "chair", "Chair"); }]);
    } else {
      items.push(["Move to main grid: with backrest", function () { moveGrid($li, "backrest", "with backrest"); }]);
      items.push(["Move to main grid: without backrest", function () { moveGrid($li, "none", "without backrest"); }]);
    }
    $menu = $('<div class="s-aid-menu no-print"></div>');
    $.each(items, function (i, it) {
      $('<button type="button"></button>').text(it[0]).appendTo($menu)
        .on("click", function (ev) { ev.stopPropagation(); closeMenu(); it[1](); });
    });
    var off = $btn.offset();
    $menu.css({ top: off.top + $btn.outerHeight() + 2, left: off.left }).appendTo("body");
  });

  $(document).on("click", function () { closeMenu(); });
  $(document).on("keydown", function (e) { if (e.key === "Escape") { clearSel(); closeMenu(); } });
})(jQuery);
