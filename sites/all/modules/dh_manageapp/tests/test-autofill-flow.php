<?php
// Codes, tokens, limits and the audit log of the autofill API (inc/autofill.inc), without HTTP.
// Run from the Drupal root: php7.4 sites/all/modules/dh_manageapp/tests/test-autofill-flow.php
require __DIR__ . '/lib.php';

$GLOBALS['af_sent'] = array();
function af_mail($email, $code) { $GLOBALS['af_sent'][] = array($email, $code); return TRUE; }
function af_mail_fail($email, $code) { return FALSE; }
function af_last_code() { $s = end($GLOBALS['af_sent']); return $s[1]; }
function af_type($key)
{
	return (int) db_query("select td_id from dh_type_detail where td_type = 'COURSE-TYPE' and td_key = :k order by td_id limit 1", array(':k' => $key))->fetchField();
}
function af_start($email, $ip = '198.51.100.7', $fn = 'af_mail')
{
	return dh_autofill_email_start(array('email' => $email, 'form' => 'application_form', 'client_ip' => $ip, 'request_id' => 'zzq'), $fn);
}
function af_code_row($otp_id)
{
	return db_query("select * from dh_autofill_code where afc_otp_id = :id", array(':id' => $otp_id))->fetchAssoc();
}
$shape = array('otp_id', 'expires_in', 'resend_after');

t_in_txn(function () use ($shape) {
	$ten = t_make_course(array('c_center' => 5, 'c_course_type' => af_type('10-Day')));
	$a = t_make_applicant($ten, array('a_email' => 'zzq-flow-1@example.invalid', 'a_f_name' => 'Neha', 'a_l_name' => 'Bansal', 'a_gender' => 'F'));
	db_update('dh_applicant')->fields(array('a_created' => '2025-05-01 10:00:00'))->condition('a_id', $a)->execute();
	t_make_applicant($ten, array('a_email' => 'zzq-flow-2@example.invalid', 'a_f_name' => 'Neha', 'a_l_name' => 'Bansal', 'a_gender' => 'F'));
	t_make_applicant($ten, array('a_email' => 'zzq-flow-2@example.invalid', 'a_f_name' => 'Ravi', 'a_l_name' => 'Bansal', 'a_gender' => 'M'));

	// start: the same answer for a known and an unknown email, and a code emailed to each.
	$s = af_start(' Zzq-Flow-1@Example.Invalid ', '203.0.113.5');
	t_eq(array_keys($s), $shape, 'start answer');
	t_ok(preg_match('/^[0-9a-f]{32}$/', $s['otp_id']) === 1, 'otp_id is 128-bit hex');
	t_eq(array($s['expires_in'], $s['resend_after']), array(600, 60), '10 minutes, resend after 60 s');
	t_eq($GLOBALS['af_sent'][0][0], 'zzq-flow-1@example.invalid', 'code emailed to the trimmed, lowercased email');
	t_ok(preg_match('/^\d{6}$/', $GLOBALS['af_sent'][0][1]) === 1, '6-digit code');
	$row = af_code_row($s['otp_id']);
	t_ok($row && strlen($row['afc_code_hash']) == 64 && strpos($row['afc_code_hash'], $GLOBALS['af_sent'][0][1]) === FALSE, 'only a hash of the code is stored');
	t_eq(array_keys(af_start('zzq-nobody@example.invalid', '203.0.113.5')), $shape, 'unknown email: the same answer');
	t_eq(count($GLOBALS['af_sent']), 2, 'unknown email: a code is emailed too');

	// verify: wrong codes; the 5th wrong try ends the code.
	$code = $GLOBALS['af_sent'][0][1];
	$wrong = ($code === '000000') ? '111111' : '000000';
	for ($i = 1; $i <= 4; $i++)
		t_eq(dh_autofill_email_verify(array('otp_id' => $s['otp_id'], 'code' => $wrong)), array('status' => 'wrong_code'), "wrong code $i");
	t_eq(dh_autofill_email_verify(array('otp_id' => $s['otp_id'], 'code' => $wrong)), array('status' => 'too_many_attempts'), '5th wrong code ends it');
	t_eq(dh_autofill_email_verify(array('otp_id' => $s['otp_id'], 'code' => $code)), array('status' => 'too_many_attempts'), 'then even the right code is refused');
	t_eq(af_code_row($s['otp_id'])['afc_email'], '', 'after the 5th wrong try the plaintext email is blanked');

	// A fresh code (after the 60 s wait), verified right.
	db_query("update dh_autofill_code set afc_created = afc_created - 120");
	$s = af_start('zzq-flow-1@example.invalid', '203.0.113.5');
	$code = af_last_code();
	$v = dh_autofill_email_verify(array('otp_id' => $s['otp_id'], 'code' => $code));
	t_eq(array($v['status'], $v['result']), array('verified', 'single'), 'right code: verified, one person');
	t_eq($v['person'], array('masked_name' => 'N*** B*****', 'last_applied_month' => '2025-05'), 'masked name and month');
	t_ok(preg_match('/^[A-Za-z0-9_-]{43}$/', $v['token']) === 1, '256-bit token');
	$row = af_code_row($s['otp_id']);
	t_ok($row['afc_used'] > 0 && $row['afc_email'] === '', 'code used; email blanked');
	t_eq(dh_autofill_email_verify(array('otp_id' => $s['otp_id'], 'code' => $code)), array('status' => 'expired'), 'a used code answers expired');
	t_eq(dh_autofill_email_verify(array('otp_id' => str_repeat('a', 32), 'code' => '123456')), array('status' => 'expired'), 'an unknown otp_id answers expired');
	t_eq((int) db_query("select count(*) from dh_autofill_token where aft_hash = :t", array(':t' => $v['token']))->fetchField(), 0, 'token not stored in clear');

	// fetch: once, for the same form only.
	t_eq(dh_autofill_fetch(array('token' => $v['token'], 'form' => 'stp_application_form')), array('status' => 'invalid_token'), 'another form: refused');
	$f = dh_autofill_fetch(array('token' => $v['token'], 'form' => 'application_form'));
	t_eq($f['source']['application_id'], (int) $a, 'source application id');
	t_eq(array_keys($f['source']), array('application_id', 'created', 'centre_name', 'course_type'), 'source block');
	t_eq($f['fields']['a_f_name'], 'Neha', 'fields returned');
	t_eq(dh_autofill_fetch(array('token' => $v['token'], 'form' => 'application_form')), array('status' => 'invalid_token'), 'a second fetch is refused');

	// Several people: verified, no person, and the token cannot fetch.
	$s = af_start('zzq-flow-2@example.invalid');
	$v = dh_autofill_email_verify(array('otp_id' => $s['otp_id'], 'code' => af_last_code()));
	t_eq(array($v['status'], $v['result'], isset($v['person'])), array('verified', 'multiple', FALSE), 'several people');
	t_eq(dh_autofill_fetch(array('token' => $v['token'], 'form' => 'application_form')), array('status' => 'invalid_token'), 'several: no fetch');

	// Expiry: a code after 10 minutes, a token after 15.
	$s = af_start('zzq-flow-3@example.invalid');
	db_query("update dh_autofill_code set afc_expires = :t where afc_otp_id = :id", array(':t' => REQUEST_TIME - 1, ':id' => $s['otp_id']));
	t_eq(dh_autofill_email_verify(array('otp_id' => $s['otp_id'], 'code' => af_last_code())), array('status' => 'expired'), 'expired code');
	db_query("update dh_autofill_code set afc_created = afc_created - 120");
	$s = af_start('zzq-flow-1@example.invalid', '203.0.113.5');
	$v = dh_autofill_email_verify(array('otp_id' => $s['otp_id'], 'code' => af_last_code()));
	db_query("update dh_autofill_token set aft_expires = :t where aft_hash = :h", array(':t' => REQUEST_TIME - 1, ':h' => hash('sha256', $v['token'])));
	t_eq(dh_autofill_fetch(array('token' => $v['token'], 'form' => 'application_form')), array('status' => 'invalid_token'), 'expired token');

	// Limits.
	db_query("delete from dh_autofill_code");
	af_start('zzq-lim@example.invalid');
	$n = count($GLOBALS['af_sent']);
	$r = af_start('zzq-lim@example.invalid');
	t_eq($r['status'], 'limited', 'again within 60 s: limited');
	t_ok($r['retry_after'] >= 1 && $r['retry_after'] <= 60, 'retry after the rest of the minute');
	t_eq(count($GLOBALS['af_sent']), $n, 'no email when limited');
	for ($i = 0; $i < 2; $i++)
	{
		db_query("update dh_autofill_code set afc_created = afc_created - 70");
		af_start('zzq-lim@example.invalid');
	}
	db_query("update dh_autofill_code set afc_created = afc_created - 70");
	$r = af_start('zzq-lim@example.invalid');
	t_eq($r['status'], 'limited', '4th in an hour: limited');
	t_ok($r['retry_after'] > 60, 'wait for the hour');
	db_query("delete from dh_autofill_code");
	$eh = dh_autofill_hmac('zzq-day@example.invalid');
	for ($i = 0; $i < 10; $i++)
		db_insert('dh_autofill_code')->fields(array('afc_otp_id' => bin2hex(random_bytes(16)), 'afc_email_hash' => $eh,
			'afc_code_hash' => str_repeat('0', 64), 'afc_form' => 'application_form', 'afc_created' => REQUEST_TIME - 4000 - $i * 100, 'afc_expires' => 0))->execute();
	$r = af_start('zzq-day@example.invalid');
	t_eq($r['status'], 'limited', '11th in a day: limited');
	for ($i = 0; $i < 60; $i++)
		db_insert('dh_autofill_code')->fields(array('afc_otp_id' => bin2hex(random_bytes(16)), 'afc_email_hash' => dh_autofill_hmac("zzq-ip-$i@example.invalid"),
			'afc_code_hash' => str_repeat('0', 64), 'afc_form' => 'application_form', 'afc_client_ip' => '192.0.2.44', 'afc_created' => REQUEST_TIME - 100, 'afc_expires' => 0))->execute();
	$r = af_start('zzq-fresh@example.invalid', '192.0.2.44');
	t_eq($r['status'], 'limited', '61st from one IP in an hour: limited');
	t_eq(array_keys(af_start('zzq-fresh2@example.invalid', '192.0.2.45')), $shape, 'another IP is not limited');

	// A bad email: the same answer, nothing sent, and its otp_id leads nowhere.
	$n = count($GLOBALS['af_sent']);
	$r = af_start('not-an-email');
	t_eq(array_keys($r), $shape, 'bad email: the same answer');
	t_eq(count($GLOBALS['af_sent']), $n, 'bad email: nothing sent');
	t_eq(dh_autofill_email_verify(array('otp_id' => $r['otp_id'], 'code' => '123456')), array('status' => 'expired'), 'bad email: otp_id unknown');

	// The email fails: the same answer, logged as mail_failed.
	$r = af_start('zzq-mf@example.invalid', '198.51.100.8', 'af_mail_fail');
	t_eq(array_keys($r), $shape, 'mail failure: the same answer');
	t_eq(db_query("select afl_result from dh_autofill_log order by afl_id desc limit 1")->fetchField(), 'mail_failed', 'mail failure logged');

	// The log never holds codes, tokens or emails.
	$dump = json_encode(db_query("select * from dh_autofill_log")->fetchAll());
	t_ok(strpos($dump, 'example.invalid') === FALSE, 'no email in the log');
	$leak = 0;
	foreach ($GLOBALS['af_sent'] as $sent)
		if (strpos($dump, '"'.$sent[1].'"') !== FALSE)
			$leak++;
	t_eq($leak, 0, 'no code in the log');
	t_ok(db_query("select count(*) from dh_autofill_log where afl_endpoint = 'email_verify' and afl_result = 'single'")->fetchField() > 0, 'verify results logged');

	// Unknown form: start throws and stores/sends nothing; verify answers expired and burns nothing.
	$n = count($GLOBALS['af_sent']);
	$threw = FALSE;
	try { dh_autofill_email_start(array('email' => 'zzq-uf@example.invalid', 'form' => 'no_such_form', 'client_ip' => '198.51.100.20'), 'af_mail'); }
	catch (InvalidArgumentException $e) { $threw = TRUE; }
	t_ok($threw, 'unknown form: start throws');
	t_eq(count($GLOBALS['af_sent']), $n, 'unknown form: nothing sent');
	t_eq((int) db_query("select count(*) from dh_autofill_code where afc_email_hash = :h", array(':h' => dh_autofill_hmac('zzq-uf@example.invalid')))->fetchField(), 0, 'unknown form: no code row');
	db_insert('dh_autofill_code')->fields(array('afc_otp_id' => str_repeat('d', 32), 'afc_email' => 'zzq-uf@example.invalid', 'afc_email_hash' => dh_autofill_hmac('zzq-uf@example.invalid'),
		'afc_code_hash' => dh_autofill_hmac(str_repeat('d', 32).'|123456'), 'afc_form' => 'no_such_form', 'afc_created' => REQUEST_TIME, 'afc_expires' => REQUEST_TIME + 600))->execute();
	t_eq(dh_autofill_email_verify(array('otp_id' => str_repeat('d', 32), 'code' => '123456')), array('status' => 'expired'), 'stored unknown form: expired');
	$row = af_code_row(str_repeat('d', 32));
	t_ok($row['afc_tries'] == 0 && $row['afc_used'] === NULL, 'stored unknown form: no try taken, not used');

	// The try counter is the gate, even for the right code.
	db_insert('dh_autofill_code')->fields(array('afc_otp_id' => str_repeat('e', 32), 'afc_email' => 'zzq-uf@example.invalid', 'afc_email_hash' => dh_autofill_hmac('zzq-uf@example.invalid'),
		'afc_code_hash' => dh_autofill_hmac(str_repeat('e', 32).'|123456'), 'afc_form' => 'application_form', 'afc_tries' => 5, 'afc_created' => REQUEST_TIME, 'afc_expires' => REQUEST_TIME + 600))->execute();
	t_eq(dh_autofill_email_verify(array('otp_id' => str_repeat('e', 32), 'code' => '123456')), array('status' => 'too_many_attempts'), '5 tries taken: the right code is refused');
	t_ok(af_code_row(str_repeat('e', 32))['afc_used'] === NULL, 'and the code stays unused');

	// Global hourly cap: over it, limited with retry_after 300, nothing sent, one warning without email/IP/code.
	$old_cap = variable_get('autofill_global_hourly_cap', NULL);
	variable_set('autofill_global_hourly_cap', 3);
	db_query("delete from dh_autofill_code");
	for ($i = 0; $i < 3; $i++)
		db_insert('dh_autofill_code')->fields(array('afc_otp_id' => bin2hex(random_bytes(16)), 'afc_email_hash' => dh_autofill_hmac("zzq-cap-$i@example.invalid"),
			'afc_code_hash' => str_repeat('0', 64), 'afc_form' => 'application_form', 'afc_client_ip' => "192.0.2.1$i", 'afc_created' => REQUEST_TIME - 100, 'afc_expires' => 0))->execute();
	$n = count($GLOBALS['af_sent']);
	$w0 = (int) db_query("select count(*) from watchdog where type = 'autofill' and message = 'Global hourly code cap reached'")->fetchField();
	$r = af_start('zzq-cap-fresh@example.invalid', '198.51.100.99');
	t_eq($r, array('status' => 'limited', 'retry_after' => 300), 'global cap: limited, retry after 300');
	t_eq(count($GLOBALS['af_sent']), $n, 'global cap: nothing sent');
	t_eq(db_query("select afl_result from dh_autofill_log order by afl_id desc limit 1")->fetchField(), 'limited', 'global cap: logged as limited');
	t_eq((int) db_query("select count(*) from watchdog where type = 'autofill' and message = 'Global hourly code cap reached'")->fetchField(), $w0 + 1, 'global cap: one watchdog warning');
	t_eq((int) db_query("select count(*) from dh_autofill_code where afc_email_hash = :h", array(':h' => dh_autofill_hmac('zzq-cap-fresh@example.invalid')))->fetchField(), 0, 'global cap: no code row');
	if ($old_cap === NULL)
		variable_del('autofill_global_hourly_cap');
	else
		variable_set('autofill_global_hourly_cap', $old_cap);
	db_query("delete from dh_autofill_code");

	// A very long email (valid format, over 100 characters) is treated like a bad one.
	$n = count($GLOBALS['af_sent']);
	$long = str_repeat('a', 60).'@'.str_repeat('b', 40).'.example.invalid';
	t_ok(strlen($long) > 100 && filter_var($long, FILTER_VALIDATE_EMAIL), 'long email: valid format, over 100 characters');
	$r = af_start($long, '198.51.100.40');
	t_eq(array_keys($r), $shape, 'long email: the same answer shape');
	t_eq(count($GLOBALS['af_sent']), $n, 'long email: nothing sent');
	t_eq(db_query("select afl_result from dh_autofill_log order by afl_id desc limit 1")->fetchField(), 'bad_email', 'long email: logged as bad_email');
	t_eq((int) db_query("select count(*) from dh_autofill_code where afc_email_hash = :h", array(':h' => dh_autofill_hmac($long)))->fetchField(), 0, 'long email: no code row');

	// The source application is gone between verify and fetch: invalid_token, logged, never empty fields.
	$gone = t_make_applicant($ten, array('a_email' => 'zzq-gone@example.invalid', 'a_f_name' => 'Gita', 'a_l_name' => 'Gone', 'a_gender' => 'F'));
	$s = af_start('zzq-gone@example.invalid', '198.51.100.41');
	$v = dh_autofill_email_verify(array('otp_id' => $s['otp_id'], 'code' => af_last_code()));
	t_eq($v['result'], 'single', 'gone source: verified single first');
	db_delete('dh_applicant_attended')->condition('aa_applicant', $gone)->execute();
	db_delete('dh_applicant')->condition('a_id', $gone)->execute();
	t_eq(dh_autofill_fetch(array('token' => $v['token'], 'form' => 'application_form')), array('status' => 'invalid_token'), 'gone source: fetch answers invalid_token');
	t_eq(db_query("select afl_result from dh_autofill_log where afl_endpoint = 'fetch' order by afl_id desc limit 1")->fetchField(), 'invalid_token', 'gone source: logged');

	// The start lock is held by someone else: limited, retry after 5, nothing sent.
	db_insert('semaphore')->fields(array('name' => 'dh_autofill_start', 'value' => 'someone-else', 'expire' => microtime(TRUE) + 30))->execute();
	$n = count($GLOBALS['af_sent']);
	t_eq(af_start('zzq-locked@example.invalid', '198.51.100.42'), array('status' => 'limited', 'retry_after' => 5), 'lock held by another: limited, retry after 5');
	t_eq(count($GLOBALS['af_sent']), $n, 'lock held by another: nothing sent');
	db_delete('semaphore')->condition('name', 'dh_autofill_start')->execute();

	// A parent's email with the parent's 10-Day and one child's Child-Teen application, end to end.
	$kid = t_make_course(array('c_center' => 5, 'c_course_type' => af_type('Child-Teen')));
	t_make_applicant($ten, array('a_email' => 'zzq-parent@example.invalid', 'a_f_name' => 'Neha', 'a_l_name' => 'Bansal', 'a_gender' => 'F'));
	$child = t_make_applicant($kid, array('a_email' => 'zzq-parent@example.invalid', 'a_f_name' => 'Aarav', 'a_l_name' => 'Bansal', 'a_gender' => 'M'));
	db_insert('dh_applicant_extra')->fields(array('ae_applicant' => $child, 'ae_father' => 'Zzqfather', 'ae_created_by' => 0, 'ae_updated_by' => 0, 'ae_updated' => date('Y-m-d H:i:s')))->execute();
	$cs = dh_autofill_email_start(array('email' => 'zzq-parent@example.invalid', 'form' => 'children_application_form', 'client_ip' => '198.51.100.43'), 'af_mail');
	$v = dh_autofill_email_verify(array('otp_id' => $cs['otp_id'], 'code' => af_last_code()));
	t_eq(array($v['status'], $v['result'], $v['person']['masked_name']), array('verified', 'single', 'A**** B*****'), 'child form: single, the child masked name');
	$f = dh_autofill_fetch(array('token' => $v['token'], 'form' => 'children_application_form'));
	t_eq(array($f['source']['application_id'], $f['fields']['a_f_name'], $f['fields']['ae_father']), array($child, 'Aarav', 'Zzqfather'), 'child form: fetch returns the child, with ae_father');
	db_query("update dh_autofill_code set afc_created = afc_created - 120");
	$as = af_start('zzq-parent@example.invalid', '198.51.100.43');
	$av = dh_autofill_email_verify(array('otp_id' => $as['otp_id'], 'code' => af_last_code()));
	t_eq(array($av['status'], $av['result']), array('verified', 'multiple'), 'adult form on the same email: multiple');

	// The start lock is released (not testable under contention in one request).
	af_start('zzq-lock@example.invalid', '198.51.100.30');
	t_ok(lock_may_be_available('dh_autofill_start'), 'start lock released');

	// Clean-up: codes and tokens after 24 hours, the log after 180 days.
	db_insert('dh_autofill_code')->fields(array('afc_otp_id' => str_repeat('b', 32), 'afc_email_hash' => str_repeat('0', 64),
		'afc_code_hash' => str_repeat('0', 64), 'afc_form' => 'application_form', 'afc_created' => REQUEST_TIME - 90000, 'afc_expires' => 0))->execute();
	db_insert('dh_autofill_token')->fields(array('aft_hash' => str_repeat('c', 64), 'aft_email_hash' => str_repeat('0', 64),
		'aft_form' => 'application_form', 'aft_result' => 'none', 'aft_created' => REQUEST_TIME - 90000, 'aft_expires' => 0))->execute();
	db_insert('dh_autofill_log')->fields(array('afl_tstamp' => REQUEST_TIME - 181 * 86400, 'afl_endpoint' => 'fetch', 'afl_result' => 'zzq-old'))->execute();
	af_start('zzq-clean@example.invalid', '198.51.100.9');
	t_eq((int) db_query("select count(*) from dh_autofill_code where afc_otp_id = :id", array(':id' => str_repeat('b', 32)))->fetchField(), 0, 'old code deleted');
	t_eq((int) db_query("select count(*) from dh_autofill_token where aft_hash = :h", array(':h' => str_repeat('c', 64)))->fetchField(), 0, 'old token deleted');
	t_eq((int) db_query("select count(*) from dh_autofill_log where afl_result = 'zzq-old'")->fetchField(), 0, 'old log row deleted');
});
t_done();
