<?php
// Application form (Add Application), the Male/Female reload: the form reloads its Section and Room lists through
// system/ajax, where the page address has no course. The posted course is used only when it is a plain number of a
// course this user may open; the centre is that course's own centre (the posted centre is ignored) (hole 7, 24 Nov 2026).
// Run from the Drupal root: php7.4 sites/all/modules/dh_manageapp/tests/test-app-form-course.php
require __DIR__ . '/lib.php';

// The form as the browser sees it for page $q, with $fs as the form state (by reference, like Drupal).
function t_app_form($q, &$fs, $edit = array())
{
	$_GET['q'] = $q;
	drupal_static_reset('arg');
	drupal_static_reset('drupal_set_message');
	unset($_SESSION['messages']);
	return dh_ma_applicant_form(array(), $fs, $edit);
}

t_in_txn(function () {
	$a = t_make_centre();
	$b = t_make_centre();
	$ka = t_make_course(array('c_center' => $a, 'c_combined_seat_course' => 0));
	$ka2 = t_make_course(array('c_center' => $a, 'c_combined_seat_course' => 0));
	$kb = t_make_course(array('c_center' => $b, 'c_combined_seat_course' => 0));
	$name_a = db_query("select c_name from dh_course where c_id = :c", array(':c' => $ka))->fetchField();
	$rid = db_query("select rid from role where name = 'Centre Admin'")->fetchField();
	$u = user_save(drupal_anonymous_user(), array('name' => 'zzq-app-form-' . mt_rand(1000, 9999), 'mail' => 'zzq-app-form@example.invalid', 'status' => 1,
		'roles' => array(DRUPAL_AUTHENTICATED_RID => 'authenticated user', $rid => 'Centre Admin')));
	$cols = db_query("desc dh_user_center")->fetchCol();
	db_insert('dh_user_center')->fields(array_intersect_key(array('uc_user' => $u->uid, 'uc_center' => $a, 'uc_deleted' => 0, 'uc_created_by' => 1, 'uc_updated_by' => 1, 'uc_created' => date('Y-m-d H:i:s'), 'uc_updated' => date('Y-m-d H:i:s')), array_flip($cols)))->execute();
	$saved_user = $GLOBALS['user'];
	$saved_q = isset($_GET['q']) ? $_GET['q'] : '';
	$GLOBALS['user'] = user_load($u->uid, TRUE);
	drupal_static_reset('user_access');
	t_ok(!user_access('access all centres'), '(test user: centre A only)');

	echo "Opening the form\n";
	$fs = array('input' => array());
	$form = t_app_form("app/add/$a/$ka", $fs);
	t_eq(array($form['course']['#value'], $form['centre']['#value']), array($ka, $a), 'course and centre from the page address');
	t_ok(strpos($form['Personal Information']['#prefix'], check_plain($name_a)) !== FALSE, 'heading names the opened course');

	echo "The Male/Female reload\n";
	$re = function ($in) { $fs = array('input' => $in + array('a_gender' => 'F')); $t0 = microtime(TRUE); $form = t_app_form('system/ajax', $fs); return array($form, microtime(TRUE) - $t0); };
	list($form) = $re(array('course' => (string) $ka, 'centre' => (string) $a));
	t_eq(array($form['course']['#value'], $form['centre']['#value']), array($ka, $a), 'own course: used');
	list($form) = $re(array('course' => (string) $ka2, 'centre' => (string) $a));
	t_eq($form['course']['#value'], $ka2, 'another course of my own centre: allowed (I may open it anyway)');
	list($form) = $re(array('course' => (string) $ka, 'centre' => (string) $b));
	t_eq(array($form['course']['#value'], $form['centre']['#value']), array($ka, $a), 'posted centre ignored: the course\'s own centre is used');
	foreach (array(
		'course of a centre I may not open' => array('course' => (string) $kb, 'centre' => (string) $b),
		'database text' => array('course' => "$ka and sleep(3)", 'centre' => (string) $a),
		'not a plain number' => array('course' => '1e3', 'centre' => (string) $a),
		'not text' => array('course' => array($ka), 'centre' => (string) $a),
		'nothing' => array(),
		'no such course' => array('course' => '999999999', 'centre' => (string) $a),
	) as $label => $in)
	{
		list($form, $secs) = $re($in);
		t_ok(!isset($form['course']) && isset($form['Attending']['aa_section']['#markup']) && strpos($form['Attending']['aa_section']['#markup'], 'open the form again') !== FALSE && $secs < 2,
			"$label: refused (\"Please open the form again\", nothing run)");
	}

	echo "Edit Application (course from the saved application)\n";
	$app = t_make_applicant($ka, array('a_gender' => 'M'));
	$row = db_query("select * from dh_applicant where a_id = :a", array(':a' => $app))->fetchAssoc();
	$fs = array('input' => array());
	$form = t_app_form("app/$app/edit", $fs, $row);
	t_eq($form['course']['#value'], (string) $ka, 'edit: course of the application');
	$re = $fs;
	$re['input'] = array('course' => (string) $kb, 'centre' => (string) $b, 'a_gender' => 'F');
	$form = t_app_form('system/ajax', $re, $row);
	t_eq($form['course']['#value'], (string) $ka, 'edit reload: posted course ignored');

	$GLOBALS['user'] = $saved_user;
	drupal_static_reset('user_access');
	$_GET['q'] = $saved_q;
	drupal_static_reset('arg');
});

$src = file_get_contents(__DIR__ . '/../inc/application.inc');
t_ok(strpos($src, "\$form_state['input']['centre']") === FALSE, 'the posted centre is not used');
t_ok(strpos($src, "\$course_id = \$form_state['input']['course'];") === FALSE && strpos($src, '(ctype_digit($posted) && course_id_load($posted))') !== FALSE, 'the posted course is used only after the number + access check');
t_ok(strpos($src, 'c_id=$course_id') === FALSE, 'the course number is no longer pasted into a query');
t_done();
