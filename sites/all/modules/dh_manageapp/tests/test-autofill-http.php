<?php
// The autofill routes over HTTP on the LOCAL site (inc/autofill.inc). It makes its own made-up
// course and applicant and deletes them again, and sets the autofill variables only while it runs.
// Run from the Drupal root: php7.4 sites/all/modules/dh_manageapp/tests/test-autofill-http.php
require __DIR__ . '/lib.php';

$BASE = getenv('DIPI_TEST_BASE') ? getenv('DIPI_TEST_BASE') : 'http://dipi.localhost';
$KEY = 'zzq-test-key-'.bin2hex(random_bytes(8));
$EMAIL = 'zzq-af-http@example.invalid';
$vars = array('autofill_enabled', 'autofill_api_key_hash', 'autofill_allowed_ips', 'autofill_mail_off');
$saved = array();
foreach ($vars as $v)
	$saved[$v] = variable_get($v, NULL);

function af_post($path, $body, $key, $method = 'POST')
{
	global $BASE;
	$payload = ($method == 'POST') ? (is_string($body) ? $body : json_encode($body)) : '';
	if (function_exists('curl_init'))
	{
		$ch = curl_init($BASE.'/'.$path);
		curl_setopt_array($ch, array(CURLOPT_RETURNTRANSFER => TRUE, CURLOPT_HEADER => TRUE, CURLOPT_FOLLOWLOCATION => FALSE,
			CURLOPT_TIMEOUT => 20, CURLOPT_CUSTOMREQUEST => $method,
			CURLOPT_HTTPHEADER => array('Content-Type: application/json', 'X-Api-Key: '.$key)));
		if ($method == 'POST')
			curl_setopt($ch, CURLOPT_POSTFIELDS, $payload);
		$raw = curl_exec($ch);
		$code = curl_getinfo($ch, CURLINFO_HTTP_CODE);
		$hs = curl_getinfo($ch, CURLINFO_HEADER_SIZE);
		curl_close($ch);
	}
	else
	{
		// php-cli without the curl extension: use the curl program, same request
		$cmd = 'curl -s -i --max-time 20 -X '.escapeshellarg($method).' -H '.escapeshellarg('Content-Type: application/json')
			.' -H '.escapeshellarg('X-Api-Key: '.$key).($method == 'POST' ? ' --data-binary @-' : '').' '.escapeshellarg($BASE.'/'.$path);
		$p = proc_open($cmd, array(0 => array('pipe', 'r'), 1 => array('pipe', 'w')), $pipes);
		fwrite($pipes[0], $payload);
		fclose($pipes[0]);
		$raw = stream_get_contents($pipes[1]);
		proc_close($p);
		$parts = explode("\r\n\r\n", $raw, 2);
		$hs = strlen($parts[0]) + 4;
		$code = (int) substr($raw, 9, 3);
	}
	return array('code' => $code, 'head' => substr($raw, 0, $hs), 'json' => json_decode(substr($raw, $hs), TRUE));
}
function af_type($key)
{
	return (int) db_query("select td_id from dh_type_detail where td_type = 'COURSE-TYPE' and td_key = :k order by td_id limit 1", array(':k' => $key))->fetchField();
}

$course = 0;
$app = 0;
try
{
	$course = t_make_course(array('c_center' => 5, 'c_course_type' => af_type('10-Day')));
	$app = t_make_applicant($course, array('a_email' => $EMAIL, 'a_f_name' => 'Zzqhttp', 'a_l_name' => 'Person', 'a_gender' => 'M'));
	variable_set('autofill_api_key_hash', hash('sha256', $KEY));
	variable_set('autofill_allowed_ips', '127.0.0.1,::1');
	variable_set('autofill_mail_off', 1);
	$start = array('email' => $EMAIL, 'form' => 'application_form', 'client_ip' => '203.0.113.9', 'request_id' => 'zzq-1');

	variable_set('autofill_enabled', 0);
	$r = af_post('autofill/email/start', $start, $KEY);
	t_eq(array($r['code'], $r['json']['status']), array(503, 'disabled'), 'switch off: 503 + JSON');
	variable_set('autofill_enabled', 1);
	t_eq(af_post('autofill/email/start', '', $KEY, 'GET')['code'], 405, 'GET: 405');
	t_eq(af_post('autofill/email/start', $start, 'wrong-key')['code'], 403, 'wrong key: 403');
	t_eq(af_post('autofill/email/start', $start, '')['code'], 403, 'no key: 403');
	variable_set('autofill_allowed_ips', '10.9.9.9');
	t_eq(af_post('autofill/email/start', $start, $KEY)['code'], 403, 'caller not allowed: 403');
	variable_set('autofill_allowed_ips', '127.0.0.1,::1');
	t_eq(af_post('autofill/email/start', '{not json', $KEY)['code'], 400, 'not JSON: 400');
	t_eq(af_post('autofill/email/start', array('email' => $EMAIL, 'form' => 'no_such_form'), $KEY)['code'], 400, 'unknown form: 400');
	t_eq(af_post('autofill/email/verify', array('otp_id' => 'x'), $KEY)['code'], 400, 'missing code: 400');
	t_eq(af_post('autofill/email/start', array('email' => $EMAIL, 'form' => 'application_form'), $KEY)['code'], 400, 'start without client_ip: 400');

	$r = af_post('autofill/email/start', $start, $KEY);
	t_eq($r['code'], 200, 'start: 200');
	t_ok(preg_match('/^[0-9a-f]{32}$/', (string) $r['json']['otp_id']) === 1, 'start: otp_id');
	t_ok(stripos($r['head'], 'application/json') !== FALSE, 'JSON content type');
	t_ok(stripos($r['head'], 'no-store') !== FALSE, 'not cached');
	$otp = $r['json']['otp_id'];
	db_update('dh_autofill_code')->fields(array('afc_code_hash' => dh_autofill_hmac($otp.'|123456')))->condition('afc_otp_id', $otp)->execute();

	$r = af_post('autofill/email/verify', array('otp_id' => $otp, 'code' => '000000'), $KEY);
	t_eq(array($r['code'], $r['json']), array(200, array('status' => 'wrong_code')), 'wrong code: 200 wrong_code');
	$r = af_post('autofill/email/verify', array('otp_id' => $otp, 'code' => '123456'), $KEY);
	t_eq(array($r['code'], $r['json']['status'], $r['json']['result'], $r['json']['person']['masked_name']),
		array(200, 'verified', 'single', 'Z****** P*****'), 'right code: verified, one person');
	$tok = $r['json']['token'];

	$r = af_post('autofill/fetch', array('token' => $tok, 'form' => 'application_form'), $KEY);
	t_eq(array($r['code'], $r['json']['source']['application_id'], $r['json']['fields']['a_f_name']), array(200, $app, 'Zzqhttp'), 'fetch: the fields');
	$r = af_post('autofill/fetch', array('token' => $tok, 'form' => 'application_form'), $KEY);
	t_eq(array($r['code'], $r['json']), array(200, array('status' => 'invalid_token')), 'fetch again: 200 invalid_token');
	$r = af_post('autofill/email/start', $start, $KEY);
	t_eq(array($r['code'], $r['json']['status']), array(200, 'limited'), 'start again within 60 s: 200 limited');
}
catch (Throwable $e)
{
	t_ok(FALSE, 'unexpected error: '.$e->getMessage());
}
finally
{
	foreach ($saved as $v => $val)
		if ($val === NULL)
			variable_del($v);
		else
			variable_set($v, $val);
	$eh = dh_autofill_hmac(dh_autofill_email_norm($EMAIL));
	db_delete('dh_autofill_code')->condition('afc_email_hash', $eh)->execute();
	db_delete('dh_autofill_token')->condition('aft_email_hash', $eh)->execute();
	db_delete('dh_autofill_log')->condition('afl_email_hash', $eh)->execute();
	if ($app)
	{
		db_delete('dh_applicant_attended')->condition('aa_applicant', $app)->execute();
		db_delete('dh_applicant')->condition('a_id', $app)->execute();
	}
	if ($course)
		db_delete('dh_course')->condition('c_id', $course)->execute();
}
t_done();
