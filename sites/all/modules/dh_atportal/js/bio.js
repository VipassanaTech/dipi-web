/**
 * Teacher profile bio data (bio.inc): the family members table and the languages picker.
 * The family rows live in the hidden #dh-family-json field, which is posted with the form;
 * the server cleans them again (dh_teacher_family_clean).
 */
(function ($, Drupal) {
  'use strict';

  function esc(s) {
    return String(s == null ? '' : s).replace(/[&<>"']/g, function (c) {
      return {'&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;'}[c];
    });
  }

  Drupal.behaviors.dhTeacherBio = {
    attach: function (context) {
      $(context).find('#dh-family').once('dh-family').each(function () {
        var $box = $(this), $json = $('#dh-family-json');
        var relations = $box.data('relations') || [];
        var rows = [];
        try { rows = JSON.parse($json.val() || '[]') || []; } catch (e) { rows = []; }

        function save() {
          $json.val(JSON.stringify(rows.filter(function (r) { return $.trim(r.name || '') !== ''; })));
        }
        function relationSelect(v) {
          var h = '<select data-k="relation"><option value="">- relation -</option>';
          relations.forEach(function (r) { h += '<option' + (r === v ? ' selected' : '') + '>' + esc(r) + '</option>'; });
          return h + '</select>';
        }
        function draw() {
          var h = '<table class="dh-family-table"><thead><tr><th>Name</th><th>Relation</th><th>Year of birth</th>' +
            '<th>Meditator</th><th>Courses</th><th></th></tr></thead><tbody>';
          rows.forEach(function (r, i) {
            h += '<tr data-i="' + i + '">' +
              '<td data-label="Name"><input type="text" data-k="name" maxlength="100" value="' + esc(r.name) + '"></td>' +
              '<td data-label="Relation">' + relationSelect(r.relation) + '</td>' +
              '<td data-label="Year of birth"><input type="number" data-k="birth_year" min="1900" max="' + new Date().getFullYear() + '" value="' + esc(r.birth_year) + '"></td>' +
              '<td data-label="Meditator"><input type="checkbox" data-k="meditator"' + (r.meditator ? ' checked' : '') + '></td>' +
              '<td data-label="Courses"><input type="number" data-k="courses" min="0" max="999" value="' + esc(r.courses) + '"' + (r.meditator ? '' : ' disabled') + '></td>' +
              '<td><a href="#" class="dh-family-remove">Remove</a></td></tr>';
          });
          if (!rows.length) {
            h += '<tr class="dh-family-empty"><td colspan="6"><em>No family members added.</em></td></tr>';
          }
          h += '</tbody></table><a href="#" class="dh-family-add">+ Add a family member</a>';
          $box.html(h);
          save();
        }
        $box.on('input change', 'input, select', function () {
          var $f = $(this), i = $f.closest('tr').data('i'), k = $f.data('k');
          if (i === undefined || !k) { return; }
          rows[i][k] = (this.type === 'checkbox') ? (this.checked ? 1 : 0) : $f.val();
          if (k === 'meditator') {
            $f.closest('tr').find('[data-k=courses]').prop('disabled', !this.checked);
          }
          save();
        });
        $box.on('click', '.dh-family-add', function (e) {
          e.preventDefault();
          rows.push({name: '', relation: '', birth_year: '', meditator: 0, courses: ''});
          draw();
          $box.find('tr[data-i=' + (rows.length - 1) + '] [data-k=name]').focus();
        });
        $box.on('click', '.dh-family-remove', function (e) {
          e.preventDefault();
          rows.splice($(this).closest('tr').data('i'), 1);
          draw();
        });
        $box.closest('form').on('submit', save);
        draw();
      });

      if ($.fn.select2) {
        $(context).find('#edit-tb-languages').once('dh-langs').select2({width: '100%', placeholder: 'Type to find a language'});
      }
    }
  };
})(jQuery, Drupal);
