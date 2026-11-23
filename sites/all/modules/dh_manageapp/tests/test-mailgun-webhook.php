<?php
// Mailgun delivery reports (webhook): only reports really signed by Mailgun are used.
//  - An empty "Mailgun Webhook Signing Key" box (Mail Settings saves it empty) means "not set": the sending
//    key (mailgun_key) is used, as Mailgun signs this account's reports with it. Before, the empty box became
//    an empty key: real reports were refused and anyone could sign a fake one.
//  - No key at all: every report is refused.
//  - The application / teacher-assignment numbers in a report are used as plain numbers only.
// Run from the Drupal root: php7.4 sites/all/modules/dh_manageapp/tests/test-mailgun-webhook.php
require __DIR__ . '/lib.php';

global $conf;
$saved = array();
foreach (array('mailgun_webhook_signing_key', 'mailgun_key') as $k)
	$saved[$k] = array_key_exists($k, $conf) ? $conf[$k] : NULL;
$set = function ($box, $sending) {
	global $conf;
	if ($box === NULL) unset($conf['mailgun_webhook_signing_key']); else $conf['mailgun_webhook_signing_key'] = $box;
	if ($sending === NULL) unset($conf['mailgun_key']); else $conf['mailgun_key'] = $sending;
};
$sign = function ($key, $ts = '1700000000', $token = 'abcdef0123456789') {
	return array('timestamp' => $ts, 'token' => $token, 'signature' => hash_hmac('sha256', $ts . $token, $key));
};

echo "Which key is used\n";
$set('', 'SENDKEY');
t_eq(dh_mailgun_webhook_key(), 'SENDKEY', 'box saved empty: the sending key is used (this was the bug)');
$set(NULL, 'SENDKEY');
t_eq(dh_mailgun_webhook_key(), 'SENDKEY', 'box never saved: the sending key is used');
$set('BOXKEY', 'SENDKEY');
t_eq(dh_mailgun_webhook_key(), 'BOXKEY', 'box filled in: that key is used');
$set('', '');
t_eq(dh_mailgun_webhook_key(), '', 'no key at all');

echo "Signature check\n";
$set('', 'SENDKEY');
$s = $sign('SENDKEY');
t_ok(dh_mailgun_signature_ok($s['timestamp'], $s['token'], $s['signature']), 'real report (signed with the sending key) is accepted');
$f = $sign('');
t_ok(!dh_mailgun_signature_ok($f['timestamp'], $f['token'], $f['signature']), 'fake report signed with an EMPTY key is refused');
$f = $sign('guess');
t_ok(!dh_mailgun_signature_ok($f['timestamp'], $f['token'], $f['signature']), 'fake report signed with another key is refused');
t_ok(!dh_mailgun_signature_ok($s['timestamp'], $s['token'], ''), 'no signature is refused');
t_ok(!dh_mailgun_signature_ok(array('x'), $s['token'], $s['signature']), 'odd values are refused');
$set('', '');
$f = $sign('');
t_ok(!dh_mailgun_signature_ok($f['timestamp'], $f['token'], $f['signature']), 'no key at all: even an empty-key signature is refused');
$set('BOXKEY', 'SENDKEY');
$b = $sign('BOXKEY');
t_ok(dh_mailgun_signature_ok($b['timestamp'], $b['token'], $b['signature']), 'box filled in: report signed with it is accepted');
t_ok(!dh_mailgun_signature_ok($s['timestamp'], $s['token'], $s['signature']), 'box filled in: sending-key signature is refused');

echo "Whole report (current format), on a made-up application\n";
$set('', 'SENDKEY');
t_in_txn(function () use ($sign) {
	$course = t_make_course(array('c_combined_seat_course' => 0));
	$app = t_make_applicant($course, array('a_email' => 'zzq-bounce@example.invalid'));
	$centre = db_query("select a_center from dh_applicant where a_id = :a", array(':a' => $app))->fetchField();
	$tag = _dh_tag_system_id($centre, DH_EMAIL_ERROR_TAG, DH_EMAIL_ERROR_TAG_COLOR);
	$report = function ($key, $event, $uv) use ($sign) {
		return array('signature' => $sign($key), 'event-data' => array('event' => $event, 'recipient' => 'zzq-bounce@example.invalid',
			'user-variables' => $uv, 'delivery-status' => array('code' => 550, 'message' => 'No such user')));
	};
	t_ok(!_dh_mailgun_webhook3_handle($report('', 'failed', array('app-id' => (string) $app))), 'fake bounce (empty key) is refused');
	t_ok(!_dh_tag_is_active($tag, $app), 'fake bounce: no Email Error tag');
	t_ok(_dh_mailgun_webhook3_handle($report('SENDKEY', 'failed', array('app-id' => (string) $app))), 'real bounce is accepted');
	t_ok(_dh_tag_is_active($tag, $app), 'real bounce: Email Error tag added');
	t_ok(_dh_mailgun_webhook3_handle($report('SENDKEY', 'delivered', array('app-id' => (string) $app))), 'real delivered report is accepted');
	t_ok(!_dh_tag_is_active($tag, $app), 'real delivered: Email Error tag removed');
	$ok = TRUE;
	try {
		_dh_mailgun_webhook3_handle($report('SENDKEY', 'failed', array('at-app-id' => "0 union select 1 -- ")));
		_dh_mailgun_webhook3_handle($report('SENDKEY', 'failed', array('at-app-id' => "1 or 1=1")));
		_dh_mailgun_webhook3_handle($report('SENDKEY', 'failed', array('app-id' => "1' or '1'='1")));
	}
	catch (Throwable $e) {
		$ok = FALSE;
	}
	t_ok($ok, 'database text in the numbers does not reach the database (no error)');
	t_ok(!_dh_mailgun_webhook3_handle(array('signature' => $sign('SENDKEY'))), 'signed but no event: nothing done');
	t_ok(!_dh_mailgun_webhook3_handle(NULL), 'not JSON: nothing done');
});

echo "Code\n";
$src = file_get_contents(__DIR__ . '/../../dh_atportal/schedule.inc');
t_ok(strpos($src, 'ct_id=".$at_app_id') === FALSE, 'log_course_teacher no longer pastes the number into the question');

foreach ($saved as $k => $v)
	if ($v === NULL) unset($conf[$k]); else $conf[$k] = $v;
t_done();
