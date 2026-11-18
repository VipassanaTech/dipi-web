<?php
// Schema for referral person details (deploy/2026-11-referral-person-schema.php).
// Run: php7.4 sites/all/modules/dh_manageapp/tests/test-referral-schema.php
require __DIR__ . '/lib.php';

function t_col($t, $c) {
  return db_query('select column_type, is_nullable, column_default from information_schema.columns where table_schema = database() and table_name = :t and column_name = :c',
    array(':t' => $t, ':c' => $c))->fetchAssoc();
}
$want = array('r_f_name' => 'varchar(50)', 'r_m_name' => 'varchar(50)', 'r_l_name' => 'varchar(50)', 'r_gender' => 'char(1)', 'r_dob' => 'date',
  'r_email' => 'varchar(100)', 'r_phone_home' => 'varchar(20)', 'r_phone_mobile' => 'varchar(18)', 'r_phone_office' => 'varchar(20)',
  'r_address' => 'varchar(255)', 'r_city' => 'bigint(20)', 'r_state' => 'varchar(10)', 'r_country' => 'varchar(15)', 'r_zip' => 'varchar(20)',
  'r_education' => 'varchar(150)', 'r_occupation' => 'varchar(100)', 'r_company' => 'varchar(150)', 'r_department' => 'varchar(100)',
  'r_designation' => 'varchar(100)', 'r_aadhar' => 'varchar(20)', 'r_pancard' => 'varchar(20)', 'r_voter_id' => 'varchar(20)',
  'r_passport' => 'varchar(20)', 'r_photo' => 'varchar(150)', 'r_moved' => 'datetime');
foreach ($want as $c => $type) {
  $got = t_col('dh_referral', $c);
  t_ok($got && $got['column_type'] === $type && $got['is_nullable'] === 'YES', "S1 dh_referral.$c is $type NULL");
}
foreach (array('teen', '10d', 'stp', 'spl', '20d', '30d', '45d', '60d', 'tsc') as $k) {
  $got = t_col('dh_referral', "r_cnt_$k");
  t_ok($got && $got['column_type'] === 'smallint(6)' && $got['is_nullable'] === 'NO' && (string) $got['column_default'] === '0', "S2 r_cnt_$k is smallint NOT NULL default 0");
}
$got = t_col('dh_referral', 'r_cnt_service');
t_ok($got && $got['column_type'] === 'int(4)' && $got['is_nullable'] === 'NO', 'S3 r_cnt_service is int(4) NOT NULL');
t_ok((bool) db_query("select 1 from information_schema.statistics where table_schema = database() and table_name = 'dh_referral_log' and column_name = 'rl_referral' and seq_in_index = 1")->fetchField(), 'S4 dh_referral_log has an index on rl_referral');
foreach (array('rc_student', 'rc_referral', 'rc_group', 'rc_status', 'rc_batch', 'rc_created', 'rc_updated') as $c)
  t_ok((bool) t_col('dh_referral_copy', $c), "S5 dh_referral_copy.$c exists");
t_eq(db_query("select column_name from information_schema.statistics where table_schema = database() and table_name = 'dh_referral_copy' and index_name = 'PRIMARY'")->fetchField(), 'rc_student', 'S6 rc_student is the primary key');
t_done();
