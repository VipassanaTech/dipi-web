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
t_eq(dh_lc_fix_link_body('<p><a href="http://zzqother.invalid/x/[RAuth]">http://zzqother.invalid/x/[RAuth]</a></p>', 'R-ATReview'), array('<p><a href="[ReviewLink]">[ReviewLink]</a></p>', 'replace-other'), 'K6 a typed link on another host is replaced too');
t_eq(dh_lc_fix_link_body('<p>One https://zzqtest.invalid/r/[RAuth] two HTTP://zzqother.invalid/old/[RAuth].</p>', 'R-ATReview'), array('<p>One [ReviewLink] two [ReviewLink].</p>', 'replace-other'), 'K6 setting link and other link together: replace-other, both gone, the full stop stays');
t_eq(dh_lc_fix_link_body('<p>Open https://zzqtest.invalid/a/[AAuth]</p>', 'R-ATReview'), array('<p>Open https://zzqtest.invalid/a/[AAuth]</p>' . DH_LC_FIX_LINE, 'add-stale'), 'K6 a recommending letter leaves the area link alone and gets the line; the other token is still there, so add-stale');
t_eq(dh_lc_fix_link_body('<p>Open https://zzqtest.invalid/r/[RAuth]</p>', 'A-ATReview'), array('<p>Open https://zzqtest.invalid/r/[RAuth]</p>' . DH_LC_FIX_LINE, 'add-stale'), 'K6 an area letter leaves the recommending link alone and gets the line; the other token is still there, so add-stale');
t_eq(dh_lc_fix_link_body('<p>Code: [AAuth]</p>', 'R-ATReview'), array('<p>Code: [AAuth]</p>' . DH_LC_FIX_LINE, 'add-stale'), 'K7 a recommending letter with a bare [AAuth] is add-stale');
t_eq(dh_lc_fix_link_body('<p>Code: [RAuth]</p>', 'A-ATReview'), array('<p>Code: [RAuth]</p>' . DH_LC_FIX_LINE, 'add-stale'), 'K7 an area letter with a bare [RAuth] is add-stale');
t_eq(dh_lc_fix_link_body('<p>Code: [RAuth]</p>', 'R-ATReview'), array('<p>Code: [RAuth]</p>' . DH_LC_FIX_LINE, 'add-stale'), 'K7 a token without an address in front: line added, add-stale');
t_eq(dh_lc_fix_link_body('<p>Open https://zzqtest.invalid/r/[RAuth]/more</p>', 'R-ATReview'), array('<p>Open https://zzqtest.invalid/r/[RAuth]/more</p>' . DH_LC_FIX_LINE, 'add-stale'), 'K7 an address that goes on after the token is not cut: line added, add-stale');
// K8: the hosts shown in the dry run for the addresses that are replaced (host only: no path, no port, no user)
t_eq(dh_lc_fix_replaced_hosts('<p>One https://zzqtest.invalid/r/[RAuth] two HTTP://ZzqOther.invalid/old/[RAuth]. <a href="http://zzqother.invalid:8080/y/[RAuth]">http://zzqother.invalid:8080/y/[RAuth]</a></p>', 'R-ATReview'),
	array('zzqtest.invalid', 'zzqother.invalid'), 'K8 the distinct hosts, lower case, in order, no path and no port');
t_eq(dh_lc_fix_replaced_hosts('<p>Open https://zzqtest.invalid/a/[AAuth] and https://zzqthird.invalid/r/[RAuth]</p>', 'A-ATReview'), array('zzqtest.invalid'), 'K8 an area letter looks at [AAuth] addresses only');
t_eq(dh_lc_fix_replaced_hosts('<p>Open https://user:secret@zzqother.invalid/x/[RAuth]</p>', 'R-ATReview'), array('zzqother.invalid'), 'K8 a user name and password in the address are not shown');
t_eq(dh_lc_fix_replaced_hosts('<p>Code: [RAuth] and no address</p>', 'R-ATReview'), array(), 'K8 no address, no host');
$ctl = dh_lc_fix_replaced_hosts("<p>https://zzq\x1bhost.invalid/x/[RAuth] and https://zzq\xc3\xa9.invalid/x/[RAuth]</p>", 'R-ATReview');
t_ok(count($ctl) >= 1 && count(array_filter($ctl, function ($h) { return preg_match('~^[\x21-\x7e]+$~', $h); })) == count($ctl), 'K8 a control or non-ASCII character in a host is never printed as it is');
// K9: each source row says which letter it is (l_id, l_center) for the dry run, and keeps the keys the apply step uses
$srcs = dh_lc_fix_sources();
t_eq(array_keys($srcs), array_keys(dh_letter_lc_roles()), 'K9 one source entry per LC role');
$okk = TRUE;
foreach ($srcs as $key => $sr)
	if ($sr !== NULL)
	{
		$have = array_keys($sr);
		foreach (array('l_id', 'l_center', 'l_name', 'l_event', 'l_course_type', 'l_subject', 'l_body', 'l_sms') as $k)
			$okk = $okk && in_array($k, $have);
		$okk = $okk && (int) $sr['l_id'] > 0 && db_query("select l_name from dh_letter where l_id = :l and l_center = :c", array(':l' => $sr['l_id'], ':c' => $sr['l_center']))->fetchField() === $sr['l_name'];
	}
t_ok($okk, 'K9 every source row has l_id and l_center of its letter, and the keys apply uses');
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

$five = array('Recommending AT Approved', 'Recommending AT Rejected', 'Area AT Approved', 'Area AT Rejected', 'LC Admin Notification');
$cnt = function ($plan, $c, $do, $key = NULL) {
  return count(array_filter($plan, function ($a) use ($c, $do, $key) { return $a['centre'] == $c && $a['do'] == $do && ($key === NULL || (isset($a['key']) && $a['key'] == $key)); }));
};

t_in_txn(function () use ($five, $cnt) {
  // 9900004: the only review letter is a "Recommending Review" with the area status (and a typed recommending link).
  $wrong = tx_letter(9900004, 'Recommending Review', 'A-ATReview', '<p>Open https://zzqtest.invalid/r/[RAuth]</p>');
  foreach ($five as $n)
    tx_letter(9900004, $n, 'Custom', '<p>x</p>');
  $plan = dh_lc_fix_plan(array(9900004));
  t_eq(array($cnt($plan, 9900004, 'status'), $cnt($plan, 9900004, 'add', 'a_review'), $cnt($plan, 9900004, 'add'), $cnt($plan, 9900004, 'link'), $cnt($plan, 9900004, 'activate')), array(1, 1, 1, 1, 0), 'F10 only a wrong letter: 1 status, 1 add (the area letter), 1 link');
  t_eq(dh_lc_fix_apply($plan, 15), array('status' => 1, 'activate' => 0, 'link' => 1, 'add' => 1), 'F10 applied counts');
  $w = tx_row($wrong);
  t_eq(array($w['l_event'], $w['l_body']), array('R-ATReview', '<p>Open [ReviewLink]</p>'), 'F10 the wrong letter is now the recommending review letter');
  t_eq(db_query("select count(*) from dh_letter where l_center = 9900004 and l_deleted = 0 and l_event = 'A-ATReview' and l_body like '%[ReviewLink]%'")->fetchField(), 1, 'F10 one active area review letter was added, with [ReviewLink]');
  t_eq(dh_lc_fix_plan(array(9900004)), array(), 'F10 a second run for that centre changes nothing');
});

t_in_txn(function () use ($five, $cnt) {
  // 9900005: a real recommending review letter AND a "Recommending Review" that has the area status: nothing to correct.
  tx_letter(9900005, 'Recommending Review', 'R-ATReview', '<p>Open https://zzqtest.invalid/r/[RAuth]</p>');
  tx_letter(9900005, 'Recommending Review', 'A-ATReview', '<p>Open https://zzqtest.invalid/r/[RAuth]</p>');
  foreach ($five as $n)
    tx_letter(9900005, $n, 'Custom', '<p>x</p>');
  $plan = dh_lc_fix_plan(array(9900005));
  t_eq($cnt($plan, 9900005, 'status'), 0, 'F11 an active recommending review letter exists: no status change');
  t_eq(array($cnt($plan, 9900005, 'add'), $cnt($plan, 9900005, 'activate')), array(0, 0), 'F11 nothing added or activated');
  dh_lc_fix_apply($plan, 15);
  t_eq(dh_lc_fix_plan(array(9900005)), array(), 'F11 a second run changes nothing');
});

t_in_txn(function () use ($five, $cnt) {
  // 9900006: the "Recommending Review" with the area status is INACTIVE: not corrected, not made the area letter.
  $off = tx_letter(9900006, 'Recommending Review', 'A-ATReview', '<p>Open https://zzqtest.invalid/r/[RAuth]</p>', 1);
  foreach ($five as $n)
    tx_letter(9900006, $n, 'Custom', '<p>x</p>');
  $plan = dh_lc_fix_plan(array(9900006));
  t_eq($cnt($plan, 9900006, 'status'), 0, 'F12 an inactive wrong letter: no status change');
  t_eq(array($cnt($plan, 9900006, 'add', 'r_review'), $cnt($plan, 9900006, 'add', 'a_review'), $cnt($plan, 9900006, 'activate')), array(1, 1, 0), 'F12 the recommending and area letters are added; the inactive one is not activated');
  dh_lc_fix_apply($plan, 15);
  $o = tx_row($off);
  t_eq(array($o['l_deleted'], $o['l_event']), array(1, 'A-ATReview'), 'F12 the inactive letter stays inactive with its status');
  t_eq(dh_lc_fix_plan(array(9900006)), array(), 'F12 a second run changes nothing');
});
t_done();
