<?php
// LC letter locks on the write paths (inc/letters.inc): inactive/active, save, add, copy.
// Run: php7.4 sites/all/modules/dh_manageapp/tests/test-lc-writes.php
require __DIR__ . '/lib.php';

function tw2_letter($over) {
  $now = date('Y-m-d H:i:s');
  return (int) db_insert('dh_letter')->fields(array_merge(array('l_center' => 9900001, 'l_event' => 'Custom', 'l_course_type' => 0,
    'l_name' => 'Zzqtest Letter', 'l_subject' => 'Zzqtest subject', 'l_body' => '<p>Body</p>', 'l_sms' => '', 'l_deleted' => 0,
    'l_created' => $now, 'l_created_by' => 777110, 'l_updated' => $now, 'l_updated_by' => 777110), $over))->execute();
}
function tw2_row($id) { return db_query("select * from dh_letter where l_id = :l", array(':l' => $id))->fetchAssoc(); }

t_in_txn(function () {
  $lc = tw2_letter(array('l_name' => 'Area AT Rejected'));
  $lcOff = tw2_letter(array('l_name' => 'Recommending AT Rejected', 'l_deleted' => 1));
  $ord = tw2_letter(array('l_name' => 'Zzqtest Ordinary'));
  t_ok(!dh_letter_set_inactive(9900001, $lc, TRUE, 777111), 'W1 an LC letter cannot be made inactive');
  t_eq(tw2_row($lc)['l_deleted'], 0, 'W1 and stays active');
  t_ok(!dh_letter_set_inactive(9900001, $lcOff, FALSE, 777111), 'W1 an inactive LC letter cannot be made active either');
  t_eq(tw2_row($lcOff)['l_deleted'], 1, 'W1 and stays inactive');
  t_ok(dh_letter_set_inactive(9900001, $ord, TRUE, 777111) && tw2_row($ord)['l_deleted'] == 1, 'W2 an ordinary letter still can');

  $f = array('l_name' => 'Zzqtest Hacked', 'l_event' => 'Received', 'l_course_type' => 10, 'l_subject' => 'Zzqtest new subject', 'l_body' => '<p>New</p>', 'l_sms' => 'S');
  t_eq(dh_letter_save(9900001, 'edit', $lc, $f, 777112), $lc, 'W3 saving an LC letter works');
  $r = tw2_row($lc);
  t_eq(array($r['l_name'], $r['l_event'], $r['l_course_type'], $r['l_subject'], $r['l_updated_by']), array('Area AT Rejected', 'Custom', 0, 'Zzqtest new subject', 777112), 'W3 name, status and course type stay; subject changes; who is stamped');

  $n = (int) db_query("select count(*) from dh_letter where l_center = 9900001")->fetchField();
  t_ok(dh_letter_save(9900001, 'add', '', array('l_name' => 'LC Admin Notification', 'l_event' => 'Custom', 'l_course_type' => 0, 'l_subject' => 's', 'l_body' => 'b', 'l_sms' => ''), 777113) === FALSE, 'W4 adding a letter with a reserved name is refused');
  t_ok(dh_letter_save(9900001, 'copy', '', array('l_name' => 'Zzqtest Copy', 'l_event' => 'A-ATReview', 'l_course_type' => 0, 'l_subject' => 's', 'l_body' => '[ReviewLink]', 'l_sms' => ''), 777113) === FALSE, 'W4 copying into a review status is refused');
  t_eq((int) db_query("select count(*) from dh_letter where l_center = 9900001")->fetchField(), $n, 'W4 nothing was added');

  $rv = tw2_letter(array('l_name' => 'Zzqtest Review', 'l_event' => 'R-ATReview', 'l_body' => '<p>Click [ReviewLink]</p>'));
  t_ok(dh_letter_save(9900001, 'edit', $rv, array('l_name' => 'Zzqtest Review', 'l_event' => 'R-ATReview', 'l_course_type' => 0, 'l_subject' => 's', 'l_body' => '<p>link gone</p>', 'l_sms' => ''), 777114) === FALSE, 'W5 a review letter without [ReviewLink] is refused');
  t_eq(tw2_row($rv)['l_body'], '<p>Click [ReviewLink]</p>', 'W5 and is unchanged');

  $c = dh_letter_save(9900001, 'add', '', array('l_name' => 'Zzqtest Fine', 'l_event' => 'Received', 'l_course_type' => 0, 'l_subject' => 's', 'l_body' => 'b', 'l_sms' => ''), 777115);
  t_ok($c > 0 && tw2_row($c)['l_name'] === 'Zzqtest Fine', 'W6 an ordinary new letter is saved as before');
});
t_done();
