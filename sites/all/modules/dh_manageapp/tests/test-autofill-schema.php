<?php
// Tables, indexes and settings made by deploy/2026-11-06-form-autofill.php.
// Run from the Drupal root: php7.4 sites/all/modules/dh_manageapp/tests/test-autofill-schema.php
require __DIR__ . '/lib.php';

foreach (array('dh_autofill_code', 'dh_autofill_token', 'dh_autofill_log') as $t)
{
	t_ok(db_table_exists($t), "$t exists");
	t_eq(db_query("select engine from information_schema.tables where table_schema = database() and table_name = :t", array(':t' => $t))->fetchField(), 'InnoDB', "$t is InnoDB");
}
$cols = db_query("select column_name from information_schema.columns where table_schema = database() and table_name = 'dh_autofill_code' order by ordinal_position")->fetchCol();
t_eq($cols, array('afc_otp_id', 'afc_email', 'afc_email_hash', 'afc_code_hash', 'afc_form', 'afc_client_ip', 'afc_tries', 'afc_created', 'afc_expires', 'afc_used'), 'code table columns');
$cols = db_query("select column_name from information_schema.columns where table_schema = database() and table_name = 'dh_autofill_token' order by ordinal_position")->fetchCol();
t_eq($cols, array('aft_hash', 'aft_email_hash', 'aft_form', 'aft_result', 'aft_source_app', 'aft_client_ip', 'aft_created', 'aft_expires', 'aft_used'), 'token table columns');
$cols = db_query("select column_name from information_schema.columns where table_schema = database() and table_name = 'dh_autofill_log' order by ordinal_position")->fetchCol();
t_eq($cols, array('afl_id', 'afl_tstamp', 'afl_endpoint', 'afl_email_hash', 'afl_form', 'afl_result', 'afl_client_ip', 'afl_request_id'), 'log table columns');
foreach (array('a_email', 'a_created') as $i)
	t_ok((bool) db_query("select count(*) from information_schema.statistics where table_schema = database() and table_name = 'dh_applicant' and index_name = :n and column_name = :n", array(':n' => $i))->fetchField(), "index $i on dh_applicant");
t_ok(preg_match('/^[0-9a-f]{64}$/', variable_get('autofill_hash_secret', '')) === 1, 'hash secret set');
t_eq((string) variable_get('autofill_enabled', 'unset'), '0', 'switch is off by default');
t_done();
