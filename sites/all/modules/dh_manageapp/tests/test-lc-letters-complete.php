<?php
// One-time: every centre gets all ten LC letters, copied from the template centre 0 (owner, 27 Nov 2026;
// inc/lc-letters-complete.inc). Made-up centres, rolled back.
// Run from the Drupal root: php7.4 sites/all/modules/dh_manageapp/tests/test-lc-letters-complete.php
require __DIR__ . '/lib.php';
require_once __DIR__ . '/../inc/lc-letters-complete.inc';

function tc_letter($centre, $name, $event = 'Custom', $over = array()) {
  $now = date('Y-m-d H:i:s');
  return (int) db_insert('dh_letter')->fields(array_merge(array('l_center' => $centre, 'l_event' => $event, 'l_course_type' => 0,
    'l_name' => $name, 'l_subject' => "$name subject", 'l_body' => '<p>Body</p>', 'l_sms' => '', 'l_deleted' => 0,
    'l_created' => $now, 'l_created_by' => 0, 'l_updated' => $now, 'l_updated_by' => 0), $over))->execute();
}

t_in_txn(function () {
  $AUTO = DH_LETTER_LC_AUTO_STATUS;
  $a = t_make_centre();   // an older centre: some letters, one LC letter inactive
  $b = t_make_centre();   // a centre that already has everything it needs except the teacher-change letters
  $e = t_make_centre();   // a centre without any letters: not counted
  tc_letter($a, 'Welcome back');
  $off = tc_letter($a, 'Area AT Approved', $AUTO, array('l_deleted' => 1));
  foreach (array('Recommending AT Approved', 'Recommending AT Rejected', 'Area AT Approved', 'Area AT Rejected', 'LC Admin Notification') as $n) tc_letter($b, $n, $AUTO);
  tc_letter($b, 'Recommending Review', 'R-ATReview'); tc_letter($b, 'Area Teacher Review', 'A-ATReview');

  $plan = dh_lc_complete_plan();
  $for = function ($c) use ($plan) { return array_values(array_map(function ($p) { return $p['key']; }, array_filter($plan, function ($p) use ($c) { return $p['centre'] === $c; }))); };
  t_eq(count($for($a)), 9, 'a centre with no LC letters gets nine: all ten but the one it has (even though inactive)');
  t_ok(!in_array('a_approved', $for($a)), 'an inactive LC letter is never doubled');
  t_eq($for($b), array('r_changed', 'a_changed', 'r_returned'), 'a centre missing the three teacher-change letters gets just those');
  t_eq($for($e), array(), 'a centre without any letters is left out');
  t_eq(count($for(0)), 0, 'the template centre itself is never touched');

  $ids = dh_lc_complete_apply($plan, dh_system_uid());
  t_eq(count($ids), count($plan), 'one new letter per planned row');
  $rows = db_query("select l_name, l_event, l_deleted, l_created_by, l_subject <> '' has_subj from dh_letter where l_center = :c order by l_name", array(':c' => $b))->fetchAllAssoc('l_name');
  t_eq(array($rows['Recommending AT Changed']->l_event, $rows['Area AT Changed']->l_event, $rows['Recommending AT Returned']->l_event), array($AUTO, $AUTO, $AUTO), 'they get the status LC Automatic, as in the template');
  t_ok($rows['Recommending AT Changed']->l_created_by == dh_system_uid() && $rows['Recommending AT Changed']->has_subj && !$rows['Recommending AT Changed']->l_deleted, 'active, with the template text, added by System');
  $ra = db_query("select l_event from dh_letter where l_center = :c and l_name = 'Recommending Review'", array(':c' => $a))->fetchField();
  t_eq($ra, 'R-ATReview', 'a review letter keeps its review status');
  $tpl = db_query("select l_body from dh_letter where l_center = 0 and l_name = 'Recommending AT Returned' and l_deleted = 0")->fetchField();
  t_eq(db_query("select l_body from dh_letter where l_center = :c and l_name = 'Recommending AT Returned'", array(':c' => $a))->fetchField(), $tpl, 'the text is the template text');
  t_ok(dh_letter_by_name($a, 'LC Admin Notification') > 0 && dh_letter_by_name($b, 'Recommending AT Returned') > 0, 'the LC process now finds them by name');
  t_eq(db_query("select count(*) from dh_letter_log where ll_record in (:ids) and ll_action = 'create' and ll_field = 'l_name'", array(':ids' => $ids))->fetchField(), (string) count($ids), 'the Letter Change Log records each new letter');
  t_eq(array_values(array_filter(dh_lc_complete_plan(), function ($p) use ($a, $b) { return $p['centre'] === $a || $p['centre'] === $b; })), array(), 'a second run adds nothing');
  t_eq((int) db_query("select l_deleted from dh_letter where l_id = :l", array(':l' => $off))->fetchField(), 1, 'the inactive LC letter is left as it was');

  // undo: a letter a centre changed since stays
  $changed = $ids[0];
  db_update('dh_letter')->fields(array('l_subject' => 'Changed by the centre', 'l_updated' => date('Y-m-d H:i:s', time() + 5)))->condition('l_id', $changed)->execute();
  list($n, $kept) = dh_lc_complete_rollback($ids);
  t_eq(array($n, $kept), array(count($ids) - 1, 1), 'undo deletes the added letters, keeps one a centre changed');
  t_eq((int) db_query("select count(*) from dh_letter where l_id in (:ids)", array(':ids' => $ids))->fetchField(), 1, 'only the changed one is left');
});
t_done();
