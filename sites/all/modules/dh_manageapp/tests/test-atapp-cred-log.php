<?php
// AT app login check (dipi_api _check_cred): a failed login is logged WITHOUT the password the AT typed.
// Before, the whole decrypted "username|||password" went into the log (watchdog type "atapp cred").
// Uses a throwaway key pair and made-up logins; everything is rolled back.
// Run from the Drupal root: php7.4 sites/all/modules/dh_manageapp/tests/test-atapp-cred-log.php
require __DIR__ . '/lib.php';

global $conf;
$saved = array_key_exists('sodium_key_pair_base64', $conf) ? $conf['sodium_key_pair_base64'] : NULL;
$pair = sodium_crypto_box_keypair();
$conf['sodium_key_pair_base64'] = sodium_bin2base64($pair, SODIUM_BASE64_VARIANT_ORIGINAL);
$seal = function ($plain) use ($pair) {
	return array('data' => sodium_bin2base64(sodium_crypto_box_seal($plain, sodium_crypto_box_publickey($pair)), SODIUM_BASE64_VARIANT_ORIGINAL));
};
$logs = function ($from) {
	return db_query("select message, variables from watchdog where type = 'atapp cred' and wid > :w", array(':w' => $from))->fetchAll();
};
$has = function ($rows, $text) {
	foreach ($rows as $r)
		if (strpos($r->message . $r->variables, $text) !== FALSE)
			return TRUE;
	return FALSE;
};

t_in_txn(function () use ($seal, $logs, $has) {
	$from = (int) db_query("select max(wid) from watchdog")->fetchField();
	$t = db_query("select concat(t_code, '.', t_gender) from dh_teacher where t_status = 'Active' and t_code <> '' limit 1")->fetchField();

	$r = _check_cred($seal('ZZQNOBODY.M|||Secret-Typed-1'));
	t_eq($r['Success'], FALSE, 'unknown login: refused');
	$r = _check_cred($seal($t . '|||Secret-Typed-2'));
	t_eq($r['Success'], FALSE, 'Active AT with a wrong password: refused');
	$r = _check_cred($seal('no-separator-Secret-Typed-3'));
	t_eq($r['Success'], FALSE, 'bad format: refused');
	$r = _check_cred($seal('A|||B|||Secret-Typed-4'));
	t_eq($r['Success'], FALSE, 'too many parts: refused');

	$rows = $logs($from);
	t_ok(count($rows) >= 3, 'failed logins are still logged (' . count($rows) . ' rows)');
	foreach (array(1, 2, 3, 4) as $i)
		t_ok(!$has($rows, 'Secret-Typed-' . $i), "typed password $i is NOT in the log");
	t_ok($has($rows, 'ZZQNOBODY.M'), 'the username is in the log (for support)');
	t_ok(!$has($rows, 'cred-string'), 'the old "cred-string" text is gone');

	// A correct login still works (made-up Active AT ZZQ9.M with a made-up password).
	$cols = db_query("desc dh_teacher")->fetchCol();
	$row = array('t_code' => 'ZZQ9', 't_gender' => 'M', 't_status' => 'Active', 't_f_name' => 'Zzqtest', 't_l_name' => 'Teacher', 't_created_by' => 1, 't_updated_by' => 1);
	db_insert('dh_teacher')->fields(array_intersect_key($row, array_flip($cols)))->execute();
	user_save(drupal_anonymous_user(), array('name' => 'ZZQ9.M', 'mail' => 'zzq9@example.invalid', 'pass' => 'Made-Up-Pass-9', 'status' => 1));
	$before = (int) db_query("select max(wid) from watchdog")->fetchField();
	$r = _check_cred($seal('ZZQ9.M|||Made-Up-Pass-9'));
	t_eq($r['Success'], TRUE, 'correct login still accepted');
	$r = _check_cred($seal('ZZQ9.M|||Made-Up-Wrong-9'));
	t_eq($r['Success'], FALSE, 'same AT, wrong password: refused');
	$rows = $logs($before);
	t_ok(!$has($rows, 'Made-Up-Pass-9') && !$has($rows, 'Made-Up-Wrong-9'), 'neither password is in the log');
	t_ok($has($rows, 'wrong password'), 'the log says why (wrong password)');
});

if ($saved === NULL) unset($conf['sodium_key_pair_base64']); else $conf['sodium_key_pair_base64'] = $saved;
t_done();
