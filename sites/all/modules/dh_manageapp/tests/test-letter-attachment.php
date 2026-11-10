<?php
// Letter attachments (inc/letters.inc): a renamed upload is stored as private://letters/… (two slashes)
// and must still be found and attached in full; a missing file is never attached empty.
// Run: php7.4 sites/all/modules/dh_manageapp/tests/test-letter-attachment.php
require __DIR__ . '/lib.php';

$parent = drupal_realpath('private://').'/letters';
$made_parent = !is_dir($parent);
$dir = 'private:///letters/zzqtest-attach';
file_prepare_directory($dir, FILE_CREATE_DIRECTORY);
$real_dir = drupal_realpath($dir);
$body = str_repeat('%PDF-zzq ', 137);
file_put_contents($real_dir.'/Waiver_RAK.pdf', $body);
file_put_contents($real_dir.'/Waiver_RAK_0.pdf', $body);
file_put_contents($real_dir.'/Empty_0.pdf', '');   // an upload that failed: the file is there but has nothing in it
$want = $real_dir.'/Waiver_RAK_0.pdf';
// Remove the made-up files even when a check crashes.
$cleanup = function () use ($real_dir, $parent, $made_parent) {
	@unlink($real_dir.'/Waiver_RAK.pdf');
	@unlink($real_dir.'/Waiver_RAK_0.pdf');
	@unlink($real_dir.'/Empty_0.pdf');
	@rmdir($real_dir);
	if ($made_parent)
		@rmdir($parent);
};
register_shutdown_function($cleanup);

// A: finding the file
t_eq(dh_letter_attachment_path('private:///letters/zzqtest-attach/Waiver_RAK.pdf'), $real_dir.'/Waiver_RAK.pdf', 'A1 three-slash address (first upload of a name)');
t_eq(dh_letter_attachment_path('private://letters/zzqtest-attach/Waiver_RAK_0.pdf'), $want, 'A2 two-slash address (renamed upload) is found too');
t_eq(dh_letter_attachment_path('private://letters/zzqtest-attach/Nope.pdf'), '', 'A3 a missing file gives nothing');
t_eq(dh_letter_attachment_path(''), '', 'A4 no attachment gives nothing');
t_eq(dh_letter_attachment_path(NULL), '', 'A5 NULL gives nothing');
t_eq(dh_letter_attachment_path('private://letters/zzqtest-attach/Empty_0.pdf'), '', 'A6 an empty (0-byte) file counts as missing');

// M: turning files into Mailgun attachments
$missing = array();
$opts = dh_letter_mail_attachments(array($want, $real_dir.'/Nope.pdf', ''), $missing);
t_eq($opts, array(array('filePath' => $want, 'filename' => 'Waiver_RAK_0.pdf')), 'M1 only the file that is there is attached, with its name');
t_eq($missing, array('Nope.pdf', 'application PDF'), 'M2 the missing ones are reported');
$missing = array();
t_eq(dh_letter_mail_attachments(array(), $missing), array(), 'M3 no files, no attachments');
t_eq($missing, array(), 'M4 and nothing missing');
$missing = array();
t_eq(dh_letter_mail_attachments(array($real_dir.'/Empty_0.pdf'), $missing), array(), 'M5 an empty file is not attached');
t_eq($missing, array('Empty_0.pdf'), 'M6 and is reported missing');

// G: dh_get_letter() with a made-up letter
t_in_txn(function () use ($want) {
	$course = t_make_course();
	$aid = t_make_applicant($course, array('a_email' => 'zzqtest@example.invalid'));
	$c = db_query("select c_center, c_course_type from dh_course where c_id = :c", array(':c' => $course))->fetchAssoc();
	$now = date('Y-m-d H:i:s');
	$mk = function ($uri) use ($c, $now) {
		return db_insert('dh_letter')->fields(array('l_center' => $c['c_center'], 'l_event' => 'ReConfirmation', 'l_course_type' => $c['c_course_type'],
			'l_name' => 'Zzqtest attach', 'l_subject' => 'Zzqtest', 'l_body' => '<p>Hi</p>', 'l_sms' => '', 'l_deleted' => 0, 'l_attachment' => $uri,
			'l_created' => $now, 'l_created_by' => 777130, 'l_updated' => $now, 'l_updated_by' => 777130))->execute();
	};
	$r = dh_get_letter('applicant', $aid, 'ReConfirmation', $mk('private://letters/zzqtest-attach/Waiver_RAK_0.pdf'));
	t_eq($r['attachment'], $want, 'G1 a renamed (two-slash) attachment gives the real file');
	t_eq($r['attachment_missing'], '', 'G2 and nothing is missing');
	$r = dh_get_letter('applicant', $aid, 'ReConfirmation', $mk('private://letters/zzqtest-attach/Gone_1.pdf'));
	t_eq($r['attachment'], '', 'G3 a file that is not there gives no attachment');
	t_eq($r['attachment_missing'], 'Gone_1.pdf', 'G4 and its name is reported as missing');
	$r = dh_get_letter('applicant', $aid, 'ReConfirmation', $mk('private://letters/zzqtest-attach/Empty_0.pdf'));
	t_eq(array($r['attachment'], $r['attachment_missing']), array('', 'Empty_0.pdf'), 'G6 a letter whose file is empty: nothing attached, reported missing');
	$r = dh_get_letter('applicant', $aid, 'ReConfirmation', $mk(NULL));
	t_eq(array($r['attachment'], $r['attachment_missing']), array('', ''), 'G5 a letter without an attachment: nothing attached, nothing missing');
});

$cleanup();
t_done();
