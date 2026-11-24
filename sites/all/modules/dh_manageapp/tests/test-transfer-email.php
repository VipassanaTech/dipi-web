<?php
// Transfer email (owner, 24 Nov 2026): after an application moves to another course (same or another centre),
// the applicant gets a short automatic email from DIPI Automation <noreply@vridhamma.org>, with no reply address.
// Sent for the registrar's Transfer and for the TRANSFER keyword (WhatsApp / SMS / partner API). Nothing is sent
// by this test: the email is captured (dh_letter_capture), and everything is rolled back.
// Run from the Drupal root: php7.4 sites/all/modules/dh_manageapp/tests/test-transfer-email.php
require __DIR__ . '/lib.php';

global $conf;
$saved = array('mode_test' => variable_get('mode_test', '0'), 'mode_test_emails' => variable_get('mode_test_emails', ''));

t_in_txn(function () use (&$conf) {
	$c1 = t_make_centre();
	$c2 = t_make_centre();
	db_update('dh_center')->fields(array('c_email' => 'newcentre@example.invalid', 'c_phone' => '0000 111 222'))->condition('c_id', $c2)->execute();
	$a_course = t_make_course(array('c_center' => $c1, 'c_combined_seat_course' => 0, 'c_name' => 'ZZQ Old Course'));
	$b_course = t_make_course(array('c_center' => $c2, 'c_combined_seat_course' => 0, 'c_name' => 'ZZQ New Course'));
	$a2_course = t_make_course(array('c_center' => $c1, 'c_combined_seat_course' => 0, 'c_name' => 'ZZQ Other Course Same Centre'));
	$app = t_make_applicant($a_course, array('a_f_name' => 'Zzqtest', 'a_m_name' => '', 'a_l_name' => 'Mover', 'a_email' => 'applicant@example.invalid'));
	$c1n = db_query("select c_name from dh_center where c_id = :c", array(':c' => $c1))->fetchField();
	$c2n = db_query("select c_name from dh_center where c_id = :c", array(':c' => $c2))->fetchField();
	// the move itself happened before the email (as in both move paths)
	db_update('dh_applicant')->fields(array('a_course' => $b_course, 'a_center' => $c2, 'a_updated_by' => 1))->condition('a_id', $app)->execute();

	echo "Another centre\n";
	$conf['mode_test'] = '0';
	$GLOBALS['dh_letter_capture'] = array();
	t_ok(dh_send_transfer_email($app, $a_course, $b_course), 'email sent');
	$m = $GLOBALS['dh_letter_capture'];
	t_eq(count($m), 1, 'exactly one email');
	$m = $m ? $m[0] : array();
	t_eq(isset($m['from']) ? $m['from'] : NULL, 'DIPI Automation <noreply@vridhamma.org>', 'from DIPI Automation <noreply@vridhamma.org>');
	t_eq(isset($m['to']) ? $m['to'] : NULL, 'applicant@example.invalid', 'to the applicant');
	t_ok(!isset($m['h:Reply-To']) && !isset($m['reply_to']), 'no reply address');
	t_eq(isset($m['subject']) ? $m['subject'] : NULL, 'Your Vipassana course application has been transferred', 'subject');
	$text = isset($m['text']) ? $m['text'] : '';
	t_ok(strpos($text, 'Dear Zzqtest Mover,') === 0, 'greets the applicant by name');
	$an = db_query("select c_name from dh_course where c_id = :c", array(':c' => $a_course))->fetchField();
	$bn = db_query("select c_name from dh_course where c_id = :c", array(':c' => $b_course))->fetchField();
	t_ok(strpos($an, $c1n) === 0 && strpos($bn, $c2n) === 0, '(DIPI names courses "<centre> / <type> / <year> / <dates>")');
	t_ok(strpos($text, "Your application for $an has been transferred to $bn.") !== FALSE, 'from old course to new course (the names already include the centres)');
	t_ok(strpos($text, "please contact $c2n: newcentre@example.invalid, 0000 111 222") !== FALSE, 'new centre contact (email, phone)');
	t_ok(strpos($text, 'Please do not reply') !== FALSE, 'says not to reply');
	t_eq((int) db_query("select count(*) from dh_log where l_identifier = :a and l_msg like 'Transfer email sent%'", array(':a' => $app))->fetchField(), 1, 'noted in the application activity');

	echo "Same centre, other course\n";
	$GLOBALS['dh_letter_capture'] = array();
	t_ok(dh_send_transfer_email($app, $a_course, $a2_course), 'email sent for a course change in the same centre');
	$text = $GLOBALS['dh_letter_capture'] ? $GLOBALS['dh_letter_capture'][0]['text'] : '';
	$a2n = db_query("select c_name from dh_course where c_id = :c", array(':c' => $a2_course))->fetchField();
	t_ok(strpos($text, "Your application for $an has been transferred to $a2n.") !== FALSE, 'same centre: old course to new course');
	t_eq(dh_transfer_course_label('Dhamma Giri / 10 Day / 2026 / 3rd-Dec to 14th-Dec', 'Dhamma Giri'), 'Dhamma Giri / 10 Day / 2026 / 3rd-Dec to 14th-Dec', 'course name with the centre: as it is');
	t_eq(dh_transfer_course_label('Special Retreat', 'Dhamma Giri'), 'Special Retreat at Dhamma Giri', 'course name without the centre: "<course> at <centre>"');

	echo "Test mode\n";
	$conf['mode_test'] = '1';
	$conf['mode_test_emails'] = 'tester@example.invalid';
	$GLOBALS['dh_letter_capture'] = array();
	dh_send_transfer_email($app, $a_course, $b_course);
	t_eq($GLOBALS['dh_letter_capture'] ? $GLOBALS['dh_letter_capture'][0]['to'] : NULL, 'tester@example.invalid', 'test mode: only to the test address');
	$conf['mode_test'] = '0';

	echo "Not sent\n";
	$GLOBALS['dh_letter_capture'] = array();
	t_ok(!dh_send_transfer_email($app, $a_course, $a_course), 'same course: nothing');
	db_update('dh_applicant')->fields(array('a_email' => '', 'a_updated_by' => 1))->condition('a_id', $app)->execute();
	t_ok(!dh_send_transfer_email($app, $a_course, $b_course), 'no email address: nothing');
	t_ok(!dh_send_transfer_email($app, $a_course, 999999999), 'unknown course: nothing, no error');
	t_ok(!dh_send_transfer_email(0, $a_course, $b_course), 'no application: nothing');
	t_eq(count($GLOBALS['dh_letter_capture']), 0, 'no email captured in these cases');
	unset($GLOBALS['dh_letter_capture']);
});
foreach ($saved as $k => $v) $conf[$k] = $v;

echo "Both move paths send it\n";
$src = file_get_contents(__DIR__ . '/../dh_manageapp.module');
preg_match('/function _move_to_centre_course\(\).*?\n}\n/s', $src, $m);
t_ok(!empty($m[0]) && preg_match('/db_update\(.dh_applicant.\).*dh_send_transfer_email\(\$app_id, \$rs\[.a_course.\], \$new_course\)/s', $m[0]), 'registrar Transfer: email after the move');
$src = file_get_contents(__DIR__ . '/../../dipi_api/dipi_api.module');
preg_match('/function _api_sms_keyword_transfer\(.*?\n}\n/s', $src, $m);
t_ok(!empty($m[0]) && preg_match('/a_transferred_course.*dh_send_transfer_email\(\$r\[.a_id.\], \$r\[.a_course.\], \$new_course\[.c_id.\]\)/s', $m[0]), 'TRANSFER keyword (WhatsApp / SMS / API): email after the move');
t_done();
