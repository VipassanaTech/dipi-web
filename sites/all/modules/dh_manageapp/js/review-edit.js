/* Edit an internal review comment in place. Self-contained + delegated on
   document, so it works wherever the reviews list is shown (applicant search
   list AND the standalone application view) as long as jquery-confirm + CKEditor
   are loaded. The "Edit" link is rendered by _get_applicant_reviews() only for
   the comment's author; the server re-checks author-only on save. */
(function ($) {
  function ckCfg() {
    var cfg = { removePlugins: "elementspath", resize_enabled: false };
    var w = window.innerWidth || document.documentElement.clientWidth || 0;
    if (w > 0 && w <= 768) {
      cfg.toolbar = [
        { name: "basicstyles", items: ["Bold", "Italic", "Underline"] },
        { name: "paragraph", items: ["NumberedList", "BulletedList"] },
        { name: "colors", items: ["TextColor"] },
        { name: "cleanup", items: ["RemoveFormat"] }
      ];
    } else {
      cfg.toolbar = [
        { name: "basicstyles", items: ["Bold", "Italic", "Underline", "Strike"] },
        { name: "colors", items: ["TextColor", "BGColor"] },
        { name: "paragraph", items: ["NumberedList", "BulletedList", "Blockquote", "JustifyLeft", "JustifyCenter", "JustifyRight"] },
        { name: "links", items: ["Link", "Unlink"] },
        { name: "clipboard", items: ["Undo", "Redo"] },
        { name: "cleanup", items: ["RemoveFormat"] }
      ];
    }
    /* Tighten paragraph spacing inside the editor so Enter doesn't leave a big gap. */
    cfg.on = { instanceReady: function () { this.document.appendStyleText("p{margin:0 0 6px}ul,ol{margin:0 0 6px}"); } };
    return cfg;
  }

  $(document).on("click", ".edit-review-btn", function (e) {
    e.preventDefault();
    var $btn = $(this);
    var arId = $btn.data("ar-id");
    var aid = $btn.data("aid");
    // The comment lives in the Review cell — a sibling <td> in the same row.
    var existing = $btn.closest("tr").find(".review-body").html();
    if (existing == null) { existing = ""; }
    var edId = "review-editor-edit-" + arId;

    if (!$.confirm) { alert("Editor not available on this page."); return; }
    $.confirm({
      title: "Edit Internal Review Comment",
      columnClass: "large",
      content: "<form class=\"formName\"><textarea id=\"" + edId + "\" name=\"review\" rows=\"10\" style=\"width:100%\"></textarea></form>",
      onContentReady: function () {
        document.getElementById(edId).value = existing;
        if (window.CKEDITOR) {
          if (CKEDITOR.instances[edId]) { CKEDITOR.instances[edId].destroy(true); }
          CKEDITOR.replace(edId, ckCfg());
        }
      },
      onClose: function () {
        if (window.CKEDITOR && CKEDITOR.instances[edId]) { CKEDITOR.instances[edId].destroy(true); }
      },
      buttons: {
        save: {
          text: "Save Changes",
          btnClass: "btn-blue",
          action: function () {
            var html = (window.CKEDITOR && CKEDITOR.instances[edId]) ? CKEDITOR.instances[edId].getData() : $("#" + edId).val();
            if ($.trim(html.replace(/<[^>]*>/g, "")) === "") { $.alert("Please enter a review note."); return false; }
            $.post("/edit-review/" + arId, { r: html }, function (json) {
              if (json && json.status) {
                $.alert("Review updated.");
                $(".internal-reviews-" + aid).load("/applicant-reviews/" + aid);
              } else {
                $.alert((json && json.msg) ? json.msg : "Failed to update review.");
              }
            }, "json").fail(function () { $.alert("Failed to update review."); });
          }
        },
        cancel: { text: "Cancel" }
      }
    });
  });
})(jQuery);
