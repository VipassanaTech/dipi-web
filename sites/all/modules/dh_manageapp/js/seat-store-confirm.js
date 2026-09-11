// Section-aware "Store Seat Changes" confirm, shared by the seating and
// group-seating plans. A seating plan is often worked on by several people at
// once (each responsible for a different section), so before saving we show
// exactly which sections this person changed and let them uncheck any they did
// not mean to touch - unchecked sections are not sent, so they stay as they are.
//
// dhSectionSaveConfirm(sections, onConfirm):
//   sections : [{ id, label, count }]  - the changed sections
//   onConfirm(checkedIds)              - called on Save; checkedIds is a map
//                                        { sectionId: 1 } of the ticked sections
(function ($) {
  window.dhSectionSaveConfirm = function (sections, onConfirm) {
    var $ov = $('<div class="dh-modal-ov"></div>');
    var h = '<div class="dh-modal" role="dialog" aria-modal="true">' +
      '<h3>Save seat changes</h3>' +
      '<p>You changed seats in the section(s) below. Only <b>checked</b> sections will be saved. ' +
      'Uncheck any section you did not mean to change &ndash; it will be kept exactly as it is ' +
      '(useful when different people are arranging different sections at the same time).</p>' +
      '<div class="dh-modal-list">';
    $.each(sections, function (i, s) {
      h += '<label class="dh-modal-row"><input type="checkbox" class="dh-sec-cb" value="' + s.id + '" checked> ' +
        '<b>' + s.label + '</b> &ndash; ' + s.count + ' seat' + (s.count == 1 ? '' : 's') + ' changed</label>';
    });
    h += '</div><div class="dh-modal-btns">' +
      '<button type="button" class="dh-modal-cancel">Cancel</button> ' +
      '<button type="button" class="dh-modal-ok">Save checked sections</button>' +
      '</div></div>';
    $ov.html(h).appendTo('body');

    function close() { $ov.remove(); }
    $ov.on('click', function (e) { if (e.target === $ov[0]) close(); });   // click the backdrop = cancel
    $ov.find('.dh-modal-cancel').on('click', close);
    $ov.find('.dh-modal-ok').on('click', function () {
      var ids = {}, any = false;
      $ov.find('.dh-sec-cb:checked').each(function () { ids[$(this).val()] = 1; any = true; });
      if (!any) {
        if (!window.confirm('No section is checked, so nothing will be saved. Close this box?')) return;
        close();
        return;                       // nothing ticked -> save nothing, leave the plan untouched
      }
      close();
      onConfirm(ids);
    });
  };
})(jQuery);
