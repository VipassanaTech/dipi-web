<?php
// The search results page loads the "no letter" note script, the course type names and the empty-subject flag.
// Run from the Drupal root: php7.4 sites/all/modules/dh_manageapp/tests/test-search-letter-note-render.php
require __DIR__ . '/lib.php';

t_in_txn(function () {
  $now = date('Y-m-d H:i:s');
  $id = (int) db_insert('dh_letter')->fields(array('l_center' => 5, 'l_event' => 'Regret', 'l_course_type' => 0, 'l_name' => 'Zzq no subject',
    'l_subject' => '', 'l_body' => '<p>x</p>', 'l_sms' => '', 'l_deleted' => 0, 'l_created' => $now, 'l_created_by' => 0,
    'l_updated' => $now, 'l_updated_by' => 0))->execute();
  $out = dh_manageapp_search_results('applicant', 'a_id = -1', 5);
  t_ok(strpos($out, 'table-applicants') !== FALSE, 'results table rendered');
  $js = drupal_add_js();
  $file = drupal_get_path('module', 'dh_manageapp') . '/js/search-letter-note.js';
  t_ok(isset($js[$file]) && file_exists(DRUPAL_ROOT . '/' . $file), 'note script is loaded and exists');
  $inline = '';
  foreach ($js as $item) if (isset($item['type']) && $item['type'] == 'inline') $inline .= $item['data'];
  t_ok(strpos($inline, 'var courseTypeNames = {') !== FALSE, 'course type names are on the page');
  t_ok(strpos($inline, '"id":"' . $id . '","status":"Zzq no subject","type":0,"nosubj":1') !== FALSE, 'a letter with no subject is flagged');
  t_eq(substr_count($inline, 'dhLetterNote.note('), 1, 'the note is worked out in one place');
  $h = strpos($inline, '.on("change", ".status-select"');
  $n = strpos($inline, 'dhLetterNote.note(');
  $old = strpos($inline, 'if ( (letters[tmp] === null)');
  t_ok($h !== FALSE && $n > $h && $n < $old, 'the note comes in the status handler, before the old early return');
});
t_done();
