<?php
// AT Portal long-course review lists: only "Reviews Assigned to me" (lc-review/me), "All Reviews" (lc-review/all)
// and the review page (lc-review/app/N) exist. The old 2021 address lc-review/<any AT code> - replaced by
// lc-review/me two days after it was written, never access-checked, and a database hole - is gone.
// Run from the Drupal root: php7.4 sites/all/modules/dh_manageapp/tests/test-lc-review-routes.php
require __DIR__ . '/lib.php';

echo "Addresses\n";
$items = dh_atportal_menu();
t_ok(!isset($items['lc-review/%atcode']), 'lc-review/<AT code> is gone');
foreach (array('lc-review/me' => 'me', 'lc-review/all' => 'all') as $path => $type) {
	t_ok(isset($items[$path]) && $items[$path]['page callback'] === 'dh_atportal_show_apps' && $items[$path]['page arguments'] === array($type),
		"$path still shows the list ($type)");
	t_eq(isset($items[$path]) ? $items[$path]['access arguments'] : NULL, array('access at portal'), "$path: same access as before");
}
t_ok(isset($items['lc-review/app/%lcappid']), 'the review page lc-review/app/N is still there');
$other = 0;
foreach ($items as $path => $item)
	if (isset($item['page arguments']) && in_array('other', $item['page arguments'], TRUE))
		$other++;
t_eq($other, 0, 'no address shows "another AT" lists');

echo "The list page\n";
$src = file_get_contents(__DIR__ . '/../../dh_atportal/dh_atportal.module');
preg_match('/function dh_atportal_apps_form\(.*?\n}\n/s', $src, $m);
t_ok(!empty($m[0]) && strpos($m[0], 'arg(1)') === FALSE, 'the list never takes an AT code from the address');
t_ok(!empty($m[0]) && strpos($m[0], "= '\$at_code'") === FALSE, 'the AT code is passed to the database safely');
t_in_txn(function () {
	$saved = $GLOBALS['user'];
	$GLOBALS['user'] = (object) array('uid' => 999999, 'name' => "x' or '1'='1", 'roles' => array(DRUPAL_AUTHENTICATED_RID => 'authenticated user'));
	drupal_static_reset('user_access');
	$ok = TRUE;
	try {
		$form = dh_atportal_apps_form(array(), $fs, 'me');
	}
	catch (Throwable $e) {
		$ok = FALSE;
	}
	t_ok($ok && isset($form['a']['#markup']) && strpos($form['a']['#markup'], 'Unable to find') !== FALSE,
		'a login name with a quote finds nothing and causes no database error');
	$GLOBALS['user'] = $saved;
	drupal_static_reset('user_access');
});
t_done();
