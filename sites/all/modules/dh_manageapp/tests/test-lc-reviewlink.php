<?php
// [ReviewLink] (inc/letter-protect.inc, letters.inc dh_get_letter, letter-check.inc, Manage Fields guard, admin settings).
// Made-up link starts only (no real address). Run: php7.4 sites/all/modules/dh_manageapp/tests/test-lc-reviewlink.php
require __DIR__ . '/lib.php';
include_once 'sites/all/libraries/editor/php/DataTables.php';   // $db for the Manage Fields check

$GLOBALS['conf']['dh_lc_review_link_r'] = 'https://zzqtest.invalid/r/';
$GLOBALS['conf']['dh_lc_review_link_a'] = 'https://zzqtest.invalid/a/';
t_eq(dh_letter_review_link('r', 'CODE1'), 'https://zzqtest.invalid/r/CODE1', 'L1 recommending link = setting + code');
t_eq(dh_letter_review_link('a', 'CODE2'), 'https://zzqtest.invalid/a/CODE2', 'L1 area link = setting + code');

// The merge-field check
$p = dh_letter_check(5, array('l_event' => 'R-ATReview', 'l_body' => '<p>no link</p>'));
t_ok((bool) array_filter($p, function ($x) { return $x['level'] == 'error' && $x['raw'] == '[ReviewLink]'; }), 'C1 a review letter without [ReviewLink] is an error');
$p = dh_letter_check(5, array('l_event' => 'A-ATReview', 'l_body' => '<p>Click [ReviewLink]</p>'));
t_ok(!array_filter($p, function ($x) { return $x['level'] == 'error'; }), 'C2 with [ReviewLink] there is no error');
$p = dh_letter_check(5, array('l_event' => 'R-ATReview', 'l_body' => '<p>[ReviewLink] old [RAuth]</p>'));
t_ok((bool) array_filter($p, function ($x) { return $x['level'] == 'warning' && $x['raw'] == '[RAuth]'; }), 'C3 [RAuth] gets a "use [ReviewLink]" warning');
$p = dh_letter_check(5, array('l_event' => 'Received', 'l_body' => '<p>[ReviewLink]</p>'));
t_ok((bool) array_filter($p, function ($x) { return $x['level'] == 'error' && $x['raw'] == '[ReviewLink]'; }), 'C4 [ReviewLink] in a letter that does not go to an AT is an error');

// Admin settings: both required fields exist
$form = drupal_get_form('dh_manageapp_admin_form');
$found = array();
foreach (element_children($form) as $g)
  foreach (array('dh_lc_review_link_r', 'dh_lc_review_link_a') as $k)
    if (isset($form[$g][$k]))
      $found[$k] = !empty($form[$g][$k]['#required']);
t_eq($found, array('dh_lc_review_link_r' => TRUE, 'dh_lc_review_link_a' => TRUE), 'S1 the two settings are on the admin settings page and required');

// dh_get_letter fills [ReviewLink] for the right AT
t_in_txn(function () {
  $course = t_make_course();
  $aid = t_make_applicant($course, array('a_email' => 'zzqtest@example.invalid'));
  db_insert('dh_applicant_lc')->fields(array('al_applicant' => $aid, 'al_recommending' => 'Zzqtest Rat', 'al_area_at' => 'Zzqtest Aat',
    'al_recommending_auth' => 'RCODE9', 'al_area_auth' => 'ACODE9'))->execute();
  $c = db_query("select c_center, c_course_type from dh_course where c_id = :c", array(':c' => $course))->fetchAssoc();
  $now = date('Y-m-d H:i:s');
  foreach (array('R-ATReview' => 'Zzqtest RLetter', 'A-ATReview' => 'Zzqtest ALetter') as $ev => $nm)
    db_insert('dh_letter')->fields(array('l_center' => $c['c_center'], 'l_event' => $ev, 'l_course_type' => $c['c_course_type'], 'l_name' => $nm,
      'l_subject' => 'Zzqtest review', 'l_body' => '<p>Open [ReviewLink] now</p>', 'l_sms' => '', 'l_deleted' => 0,
      'l_created' => $now, 'l_created_by' => 777120, 'l_updated' => $now, 'l_updated_by' => 777120))->execute();
  $r = dh_get_letter('r-teacher', $aid, 'R-ATReview');
  t_ok(!empty($r['result']) && strpos($r['body'], 'https://zzqtest.invalid/r/RCODE9') !== FALSE, 'G1 the recommending AT gets the recommending link with its code');
  $a = dh_get_letter('a-teacher', $aid, 'A-ATReview');
  t_ok(!empty($a['result']) && strpos($a['body'], 'https://zzqtest.invalid/a/ACODE9') !== FALSE, 'G2 the area AT gets the area link with its code');
  // G4: a centre field named ReviewLink (put in directly, as if it got past Manage Fields) never overrides DIPI's own value
  $fid = db_insert('dh_letter_fields')->fields(array('lf_center' => $c['c_center'], 'lf_name' => 'ReviewLink',
    'lf_value' => 'https://zzqother.invalid/x/[RAuth]', 'lf_deleted' => 0, 'lf_updated_by' => 777122))->execute();
  $o = dh_get_letter('r-teacher', $aid, 'R-ATReview');
  t_ok(!empty($o['result']) && strpos($o['body'], 'https://zzqtest.invalid/r/RCODE9') !== FALSE, 'G4 a centre field named ReviewLink does not override: the setting + the AT code is still used');
  t_ok(!empty($o['body']) && strpos($o['body'], 'zzqother.invalid') === FALSE, 'G4 and the centre field text is not in the letter');
  db_delete('dh_letter_fields')->condition('lf_id', $fid)->execute();
  $GLOBALS['conf']['dh_lc_review_link_r'] = '';
  $e = dh_get_letter('r-teacher', $aid, 'R-ATReview');
  t_ok(empty($e['result']), 'G3 an empty setting: the letter is not sent');
  t_ok((bool) db_query("select 1 from dh_log where l_module = 'Applicant' and l_identifier = :a and l_msg like 'Letter not sent: the review link%'", array(':a' => $aid))->fetchField(), 'G3 and the activity log says why');
  $GLOBALS['conf']['dh_lc_review_link_r'] = 'https://zzqtest.invalid/r/';
});

// Manage Fields refuses a centre field named like DIPI's review-only fields
$db->transaction();
try {
  $res = dh_letter_fields_editor($db, 9900001, 777121)->transaction(false)->process(array('action' => 'create',
    'data' => array(0 => array('dh_letter_fields' => array('lf_name' => 'ReviewLink', 'lf_value' => 'x', 'lf_deleted' => 0)))))->data();
  t_ok(!empty($res['fieldErrors']), 'M1 a centre field cannot be named ReviewLink');

  // M2: a crafted lf_deleted ('00', ' 0', '0.0', an array) must not count as "deleting" and skip the name rule
  $reserved = 0;
  foreach (array('00', ' 0', '0.0', '', array('x')) as $odd) {
    foreach (array('ReviewLink', 'RAuth', 'AAuth') as $nm) {
      $res = dh_letter_fields_editor($db, 9900001, 777121)->transaction(false)->process(array('action' => 'create',
        'data' => array(0 => array('dh_letter_fields' => array('lf_name' => $nm, 'lf_value' => 'x', 'lf_deleted' => $odd)))))->data();
      $names = array_column(isset($res['fieldErrors']) ? $res['fieldErrors'] : array(), 'name');
      t_ok(in_array('dh_letter_fields.lf_name', $names), 'M2 create ' . $nm . ' with lf_deleted ' . json_encode($odd) . ' is refused on the name');
    }
  }
  $left = $db->sql("select count(*) as n from dh_letter_fields where lf_center = 9900001 and lf_name in ('ReviewLink','RAuth','AAuth')")->fetch();
  t_eq($left['n'], 0, 'M2 and no row is left');
  // the same crafted values on an edit of an ordinary field of this centre
  $okid = (int) $db->insert('dh_letter_fields', array('lf_center' => 9900001, 'lf_name' => 'Zzqtest Plain', 'lf_value' => 'v', 'lf_deleted' => 0, 'lf_updated_by' => 777121), 'lf_id')->insertId();
  foreach (array('00', ' 0', '0.0') as $odd) {
    $res = dh_letter_fields_editor($db, 9900001, 777121)->transaction(false)->process(array('action' => 'edit',
      'data' => array("row_$okid" => array('dh_letter_fields' => array('lf_name' => 'ReviewLink', 'lf_value' => 'x', 'lf_deleted' => $odd)))))->data();
    $names = array_column(isset($res['fieldErrors']) ? $res['fieldErrors'] : array(), 'name');
    t_ok(in_array('dh_letter_fields.lf_name', $names), 'M2 edit to ReviewLink with lf_deleted ' . json_encode($odd) . ' is refused on the name');
  }
  $row = $db->sql("select lf_name, lf_deleted from dh_letter_fields where lf_id = $okid")->fetch();
  t_eq(array($row['lf_name'], $row['lf_deleted']), array('Zzqtest Plain', 0), 'M2 and the edited field is unchanged');
  // Remove (an edit with lf_deleted 1) still works, and is stored as 1
  $res = dh_letter_fields_editor($db, 9900001, 777121)->transaction(false)->process(array('action' => 'edit',
    'data' => array("row_$okid" => array('dh_letter_fields' => array('lf_name' => 'Zzqtest Plain', 'lf_value' => 'v', 'lf_deleted' => '1')))))->data();
  $row = $db->sql("select lf_deleted from dh_letter_fields where lf_id = $okid")->fetch();
  t_eq($row['lf_deleted'], 1, 'M2 Remove (lf_deleted 1 on an edit) still deletes');

  // M3: an ordinary create still saves, and lf_deleted is stored as 0
  foreach (array('0', ' 0', '00', 0) as $odd) {
    $nm = 'Zzqtest Fine ' . md5(json_encode($odd));
    $res = dh_letter_fields_editor($db, 9900001, 777121)->transaction(false)->process(array('action' => 'create',
      'data' => array(0 => array('dh_letter_fields' => array('lf_name' => $nm, 'lf_value' => 'ok', 'lf_deleted' => $odd)))))->data();
    $row = $db->sql("select lf_deleted from dh_letter_fields where lf_center = 9900001 and lf_name = " . $db->quote($nm))->fetch();
    t_ok(empty($res['fieldErrors']) && $row && (string) $row['lf_deleted'] === '0', 'M3 an ordinary create with lf_deleted ' . json_encode($odd) . ' saves, stored as 0');
  }
} catch (Throwable $e) { t_ok(FALSE, 'unexpected error: ' . $e->getMessage()); }
$db->rollback();
t_done();
