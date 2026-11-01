<?php
// Letter writes (inc/letters.inc): who is recorded, and a letter of another centre is refused.
// Run: php7.4 sites/all/modules/dh_manageapp/tests/test-letter-writes.php
require __DIR__ . '/lib.php';

function tw_letter($centre, $over = array()) {
  $now = date('Y-m-d H:i:s');
  return (int) db_insert('dh_letter')->fields(array_merge(array('l_center' => $centre, 'l_event' => 'Received', 'l_course_type' => 0,
    'l_name' => 'Zzqtest Letter', 'l_subject' => 'Zzqtest subject', 'l_body' => '<p>Line one</p>', 'l_sms' => '',
    'l_deleted' => 0, 'l_created' => $now, 'l_created_by' => 777030, 'l_updated' => $now, 'l_updated_by' => 777030), $over))->execute();
}
function tw_log($id) {
  return db_query("select ll_action, ll_field, ll_old, ll_new, ll_user from dh_letter_log where ll_area = 'letter' and ll_record = :r order by ll_id", array(':r' => $id))->fetchAll(PDO::FETCH_ASSOC);
}
function tw_last($id) { $log = tw_log($id); return end($log); }
function tw_row($id) { return db_query("select * from dh_letter where l_id = :l", array(':l' => $id))->fetchAssoc(); }
// The Letters page as if the browser asked for $path (dh_manage_letters reads arg()).
function tw_page($path) { $_GET['q'] = $path; return dh_manage_letters(); }

t_in_txn(function () {
  $a = tw_letter(9900001);
  $b = tw_letter(9900002, array('l_attachment' => 'private://letters/zzqtest.pdf'));
  t_ok(dh_letter_in_centre($a, 9900001) && !dh_letter_in_centre($a, 9900002), 'W1 dh_letter_in_centre');

  t_ok(dh_letter_set_inactive(9900001, $a, TRUE, 777031), 'W2 own letter made inactive');
  t_eq(array(tw_row($a)['l_deleted'], tw_row($a)['l_updated_by']), array(1, 777031), 'W2 inactive, updated by who did it');
  t_eq(array(tw_last($a)['ll_action'], tw_last($a)['ll_user']), array('inactive', 777031), 'W2 logged as inactive, by who did it');
  t_ok(dh_letter_set_inactive(9900001, $a, FALSE, 777032), 'W3 made active again');
  t_eq(array(tw_last($a)['ll_action'], tw_last($a)['ll_user']), array('active', 777032), 'W3 logged as active');

  $n = count(tw_log($b));
  t_ok(!dh_letter_set_inactive(9900001, $b, TRUE, 777033), "W4 another centre's letter is refused");
  t_eq(array(tw_row($b)['l_deleted'], count(tw_log($b))), array(0, $n), 'W4 and stays unchanged');
  t_ok(!dh_letter_remove_attachment(9900001, $b, 777034), "W5 another centre's attachment is refused");
  t_eq(tw_row($b)['l_attachment'], 'private://letters/zzqtest.pdf', 'W5 attachment kept');
  t_ok(dh_letter_remove_attachment(9900002, $b, 777035), 'W6 own attachment removed');
  t_eq(array(tw_last($b)['ll_field'], tw_last($b)['ll_old'], tw_last($b)['ll_user']), array('l_attachment', 'private://letters/zzqtest.pdf', 777035), 'W6 logged with who');
  t_ok(tw_row($b)['l_attachment'] === NULL, 'W6 attachment is empty');

  $fields = array('l_name' => 'Zzqtest New', 'l_event' => 'Confirmed', 'l_course_type' => 0, 'l_subject' => 'Zzqtest s', 'l_body' => '<p>B</p>', 'l_sms' => 'S');
  $c = dh_letter_save(9900001, 'add', '', $fields, 777036);
  $r = tw_row($c);
  t_eq(array($r['l_center'], $r['l_name'], $r['l_event'], $r['l_subject'], $r['l_body'], $r['l_sms'], $r['l_created_by'], $r['l_updated_by']),
    array(9900001, 'Zzqtest New', 'Confirmed', 'Zzqtest s', '<p>B</p>', 'S', 777036, 777036), 'W7 add stores the form fields as before, with who');
  t_eq(array_values(array_unique(array_column(tw_log($c), 'll_user'))), array(777036), 'W7 created rows by who added it');
  t_eq(dh_letter_save(9900001, 'edit', $c, array('l_subject' => 'Zzqtest s2') + $fields, 777037), $c, 'W8 edit of own letter');
  t_eq(array(tw_last($c)['ll_field'], tw_last($c)['ll_new'], tw_last($c)['ll_user']), array('l_subject', 'Zzqtest s2', 777037), 'W8 logged by the editor');
  t_ok(dh_letter_save(9900002, 'edit', $c, array('l_subject' => 'Zzqtest hacked') + $fields, 777038) === FALSE, 'W9 an edit through another centre is refused');
  t_eq(tw_row($c)['l_subject'], 'Zzqtest s2', 'W9 letter unchanged');

  foreach (array('view', 'preview', 'edit', 'copy', 'delete', 'restore', 'delattach') as $act)
    t_eq(tw_page("letters/9900002/$act/$a"), MENU_NOT_FOUND, "W10 letters/<other centre>/$act/<letter> is Page not found");
  t_eq(tw_row($a)['l_deleted'], 0, 'W10 the delete URL changed nothing');
  t_ok(is_string(tw_page("letters/9900001/view/$a")), 'W11 the own letter view page still works');
});
t_done();
