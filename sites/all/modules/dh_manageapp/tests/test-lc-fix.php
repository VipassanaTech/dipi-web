<?php
// One-time LC letter fix (inc/lc-letters-fix.inc): plan, apply, second run. Made-up centres 9900001-9900003, rolled back.
// Run: php7.4 sites/all/modules/dh_manageapp/tests/test-lc-fix.php
require __DIR__ . '/lib.php';
require_once dirname(__DIR__) . '/inc/lc-letters-fix.inc';
$GLOBALS['conf']['dh_lc_review_link_r'] = 'https://zzqtest.invalid/r/';
$GLOBALS['conf']['dh_lc_review_link_a'] = 'https://zzqtest.invalid/a/';

t_eq(dh_lc_fix_link_body('<p>Go https://zzqtest.invalid/r/[RAuth] now</p>', 'R-ATReview'), array('<p>Go [ReviewLink] now</p>', 'replace'), 'K1 the typed recommending link becomes [ReviewLink]');
t_eq(dh_lc_fix_link_body('<a href="https://zzqtest.invalid/a/[AAuth]">https://zzqtest.invalid/a/[AAuth]</a>', 'A-ATReview'), array('<a href="[ReviewLink]">[ReviewLink]</a>', 'replace'), 'K1 every copy (link and text) is replaced');
t_eq(dh_lc_fix_link_body('<p>Hello</p><p>Thanks [Footer]</p>', 'R-ATReview'), array('<p>Hello</p>' . DH_LC_FIX_LINE . '<p>Thanks [Footer]</p>', 'add'), 'K2 no typed link: the line goes before the footer block');
t_eq(dh_lc_fix_link_body('<p>Hello</p>', 'A-ATReview'), array('<p>Hello</p>' . DH_LC_FIX_LINE, 'add'), 'K2 no footer: the line goes at the end');
t_eq(dh_lc_fix_link_body('<p>[ReviewLink]</p>', 'R-ATReview'), array('<p>[ReviewLink]</p>', ''), 'K3 already done: unchanged');
$cs = dh_lc_fix_centres();
t_ok($cs && !in_array(0, $cs) && !in_array(302, $cs), 'K4 active centres leave out the template and the test centre');
$GLOBALS['conf']['dh_lc_review_link_a'] = '';
t_eq(dh_lc_fix_plan(array(9900001)), array(array('do' => 'settings_missing')), 'K5 empty settings: the fix refuses');
$GLOBALS['conf']['dh_lc_review_link_a'] = 'https://zzqtest.invalid/a/';

function tx_letter($c, $name, $event, $body, $deleted = 0) {
  $now = date('Y-m-d H:i:s');
  return (int) db_insert('dh_letter')->fields(array('l_center' => $c, 'l_event' => $event, 'l_course_type' => 0, 'l_name' => $name,
    'l_subject' => 'Zzqtest subject', 'l_body' => $body, 'l_sms' => '', 'l_deleted' => $deleted,
    'l_created' => $now, 'l_created_by' => 777140, 'l_updated' => $now, 'l_updated_by' => 777140))->execute();
}
function tx_row($id) { return db_query("select * from dh_letter where l_id = :l", array(':l' => $id))->fetchAssoc(); }

t_in_txn(function () {
  // 9900002 is like Mitra Upakram: no R-ATReview letter, a "Recommending Review" with the area status; Area AT Rejected only inactive.
  $wrong = tx_letter(9900002, 'Recommending Review', 'A-ATReview', '<p>Open https://zzqtest.invalid/r/[RAuth]</p>');
  $area2 = tx_letter(9900002, 'Area Teacher Review', 'A-ATReview', '<p>Open https://zzqtest.invalid/a/[AAuth]</p>');
  foreach (array('Recommending AT Approved', 'Recommending AT Rejected', 'Area AT Approved', 'LC Admin Notification') as $n)
    tx_letter(9900002, $n, 'Custom', '<p>x</p>');
  $off = tx_letter(9900002, 'Area AT Rejected', 'Custom', '<p>x</p>', 1);
  // 9900003 is like Kamboja: two recommending review letters, the second without any link.
  $r3 = tx_letter(9900003, 'Recommending Review', 'R-ATReview', '<p>Open https://zzqtest.invalid/r/[RAuth]</p>');
  $km = tx_letter(9900003, 'km Recommending AT Review', 'R-ATReview', '<p>Khmer text</p><p>Thanks [Footer]</p>');
  tx_letter(9900003, 'Area Teacher Review', 'A-ATReview', '<p>Open https://zzqtest.invalid/a/[AAuth]</p>');
  foreach (array('Recommending AT Approved', 'Recommending AT Rejected', 'Area AT Approved', 'Area AT Rejected', 'LC Admin Notification') as $n)
    tx_letter(9900003, $n, 'Custom', '<p>x</p>');

  $plan = dh_lc_fix_plan(array(9900001, 9900002, 9900003));
  $count = function ($c, $do) use ($plan) { return count(array_filter($plan, function ($a) use ($c, $do) { return $a['centre'] == $c && $a['do'] == $do; })); };
  t_eq(array($count(9900001, 'add'), $count(9900001, 'activate'), $count(9900001, 'link')), array(7, 0, 0), 'F1 an empty centre gets the 7 LC letters added');
  t_eq(array($count(9900002, 'status'), $count(9900002, 'activate'), $count(9900002, 'add'), $count(9900002, 'link')), array(1, 1, 0, 2), 'F2 Mitra-like: status fixed, inactive copy made active, no extra review letter, 2 links');
  t_eq(array($count(9900003, 'add'), $count(9900003, 'link')), array(0, 3), 'F3 Kamboja-like: nothing added, 3 links');

  $n = dh_lc_fix_apply($plan, 15);
  t_eq($n, array('status' => 1, 'activate' => 1, 'link' => 5, 'add' => 7), 'F4 applied counts');
  $act = db_query("select count(*) from dh_letter where l_center = 9900001 and l_deleted = 0 and l_created_by = 15")->fetchField();
  t_eq($act, 7, 'F5 the 7 added letters are active and created by System');
  foreach (db_query("select l_body from dh_letter where l_center = 9900001 and l_event in ('R-ATReview', 'A-ATReview')")->fetchCol() as $b)
    t_ok(strpos($b, '[ReviewLink]') !== FALSE, 'F5 an added review letter carries [ReviewLink]');
  $w = tx_row($wrong);
  t_eq(array($w['l_event'], $w['l_body'], $w['l_updated_by']), array('R-ATReview', '<p>Open [ReviewLink]</p>', 15), 'F6 the wrong letter is now the recommending review letter, with [ReviewLink]');
  t_eq(array(tx_row($off)['l_deleted'], tx_row($area2)['l_body']), array(0, '<p>Open [ReviewLink]</p>'), 'F6 inactive copy now active; area link switched');
  t_eq(tx_row($km)['l_body'], '<p>Khmer text</p>' . DH_LC_FIX_LINE . '<p>Thanks [Footer]</p>', 'F7 the Khmer letter got the link line before its footer');
  t_eq(tx_row($r3)['l_body'], '<p>Open [ReviewLink]</p>', 'F7 the English one was switched');
  $logged = db_query("select count(distinct ll_record) from dh_letter_log where ll_area = 'letter' and ll_center in (9900001, 9900002, 9900003) and ll_user = 15")->fetchField();
  t_eq($logged, 7 + 1 + 1 + 1 + 3, 'F8 every changed letter is in the Change Log as System');
  t_eq(dh_lc_fix_plan(array(9900001, 9900002, 9900003)), array(), 'F9 a second run changes nothing');
});
t_done();
