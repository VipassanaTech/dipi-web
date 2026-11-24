<?php
// "Add" from an earlier application (app/add/<centre>/<course>?ref=<application>): the earlier application is used
// only when it is a plain number of an application this user may open (app_id_load: own centres or all-centre
// access, and gender access). Otherwise the new form opens empty.
// Run from the Drupal root: php7.4 sites/all/modules/dh_manageapp/tests/test-add-ref.php
require __DIR__ . '/lib.php';

t_in_txn(function () {
	$a = t_make_centre();
	$b = t_make_centre();
	$ca = t_make_course(array('c_center' => $a, 'c_combined_seat_course' => 0));
	$cb = t_make_course(array('c_center' => $b, 'c_combined_seat_course' => 0));
	$app_a = t_make_applicant($ca, array('a_gender' => 'M'));
	$app_b = t_make_applicant($cb, array('a_gender' => 'M'));
	$rid = db_query("select rid from role where name = 'Centre Admin'")->fetchField();
	$u = user_save(drupal_anonymous_user(), array('name' => 'zzq-add-ref-' . mt_rand(1000, 9999), 'mail' => 'zzq-add-ref@example.invalid', 'status' => 1,
		'roles' => array(DRUPAL_AUTHENTICATED_RID => 'authenticated user', $rid => 'Centre Admin')));
	$cols = db_query("desc dh_user_center")->fetchCol();
	db_insert('dh_user_center')->fields(array_intersect_key(array('uc_user' => $u->uid, 'uc_center' => $a, 'uc_deleted' => 0, 'uc_created_by' => 1, 'uc_updated_by' => 1, 'uc_created' => date('Y-m-d H:i:s'), 'uc_updated' => date('Y-m-d H:i:s')), array_flip($cols)))->execute();
	$saved = $GLOBALS['user'];
	$GLOBALS['user'] = user_load($u->uid, TRUE);
	drupal_static_reset('user_access');
	t_ok(!user_access('access all centres'), '(test user: one centre only)');

	echo "Earlier application\n";
	t_eq(dh_add_ref_application((string) $app_a), $app_a, 'from my own centre: used');
	t_eq(dh_add_ref_application((string) $app_b), 0, 'from a centre I have no access to: not used');
	$ok = TRUE;
	try {
		t_eq(dh_add_ref_application("$app_a' or '1'='1"), 0, 'database text: not used');
		t_eq(dh_add_ref_application("0 union select 1"), 0, 'database text (union): not used');
		t_eq(dh_add_ref_application('1e3'), 0, 'not a plain number: not used');
		t_eq(dh_add_ref_application(''), 0, 'nothing: not used');
		t_eq(dh_add_ref_application(array($app_a)), 0, 'not text: not used');
		t_eq(dh_add_ref_application('999999999'), 0, 'no such application: not used');
	}
	catch (Throwable $e) {
		$ok = FALSE;
	}
	t_ok($ok, 'no database error from any of these');

	echo "All-centre access\n";
	$GLOBALS['user'] = user_load(1, TRUE);
	drupal_static_reset('user_access');
	t_eq(dh_add_ref_application((string) $app_b), $app_b, 'administrator: any centre');
	$GLOBALS['user'] = $saved;
	drupal_static_reset('user_access');
});

$src = file_get_contents(__DIR__ . '/../inc/zero-day.inc');
t_ok(strpos($src, "a_id='\".\$_REQUEST['ref'].\"'") === FALSE, 'the ref number is no longer pasted into the query');
t_ok(strpos($src, 'dh_add_ref_application(') !== FALSE, 'the Add page uses the check');
t_done();
