<?php
// Referral person details: columns on dh_referral, the move marker, the log index, the copy table.
// Run from the Drupal root:
//   local: php7.4 sites/all/modules/dh_manageapp/deploy/2026-11-referral-person-schema.php
//   prod:  sudo -u www-data php7.3 sites/all/modules/dh_manageapp/deploy/2026-11-referral-person-schema.php
// Re-runnable: anything already there is left alone.
if (php_sapi_name() !== 'cli') { echo "Command line only.\n"; exit(1); }
define('DRUPAL_ROOT', getcwd());
$_SERVER['REMOTE_ADDR'] = '127.0.0.1'; $_SERVER['HTTP_HOST'] = 'localhost'; $_SERVER['SCRIPT_NAME'] = '/index.php';
require_once DRUPAL_ROOT . '/includes/bootstrap.inc';
drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);

// dh_referral has legacy zero dates, which Drupal's session mode refuses on any ALTER.
db_query("SET SESSION sql_mode = 'STRICT_ALL_TABLES,NO_ENGINE_SUBSTITUTION'");
db_query('SET SESSION lock_wait_timeout = 15');

function rp_has_col($t, $c) {
  return (bool) db_query('select 1 from information_schema.columns where table_schema = database() and table_name = :t and column_name = :c', array(':t' => $t, ':c' => $c))->fetchField();
}

// Same types as the dh_student / dh_student_course_input columns they replace.
$cols = array(
  'r_f_name' => 'VARCHAR(50) NULL DEFAULT NULL', 'r_m_name' => 'VARCHAR(50) NULL DEFAULT NULL', 'r_l_name' => 'VARCHAR(50) NULL DEFAULT NULL',
  'r_gender' => 'CHAR(1) NULL DEFAULT NULL', 'r_dob' => 'DATE NULL DEFAULT NULL', 'r_email' => 'VARCHAR(100) NULL DEFAULT NULL',
  'r_phone_home' => 'VARCHAR(20) NULL DEFAULT NULL', 'r_phone_mobile' => 'VARCHAR(18) NULL DEFAULT NULL', 'r_phone_office' => 'VARCHAR(20) NULL DEFAULT NULL',
  'r_address' => 'VARCHAR(255) NULL DEFAULT NULL', 'r_city' => 'BIGINT(20) NULL DEFAULT NULL', 'r_state' => 'VARCHAR(10) NULL DEFAULT NULL',
  'r_country' => 'VARCHAR(15) NULL DEFAULT NULL', 'r_zip' => 'VARCHAR(20) NULL DEFAULT NULL', 'r_education' => 'VARCHAR(150) NULL DEFAULT NULL',
  'r_occupation' => 'VARCHAR(100) NULL DEFAULT NULL', 'r_company' => 'VARCHAR(150) NULL DEFAULT NULL', 'r_department' => 'VARCHAR(100) NULL DEFAULT NULL',
  'r_designation' => 'VARCHAR(100) NULL DEFAULT NULL', 'r_aadhar' => 'VARCHAR(20) NULL DEFAULT NULL', 'r_pancard' => 'VARCHAR(20) NULL DEFAULT NULL',
  'r_voter_id' => 'VARCHAR(20) NULL DEFAULT NULL', 'r_passport' => 'VARCHAR(20) NULL DEFAULT NULL', 'r_photo' => 'VARCHAR(150) NULL DEFAULT NULL',
  'r_cnt_teen' => 'SMALLINT(6) NOT NULL DEFAULT 0', 'r_cnt_10d' => 'SMALLINT(6) NOT NULL DEFAULT 0', 'r_cnt_stp' => 'SMALLINT(6) NOT NULL DEFAULT 0',
  'r_cnt_spl' => 'SMALLINT(6) NOT NULL DEFAULT 0', 'r_cnt_20d' => 'SMALLINT(6) NOT NULL DEFAULT 0', 'r_cnt_30d' => 'SMALLINT(6) NOT NULL DEFAULT 0',
  'r_cnt_45d' => 'SMALLINT(6) NOT NULL DEFAULT 0', 'r_cnt_60d' => 'SMALLINT(6) NOT NULL DEFAULT 0', 'r_cnt_tsc' => 'SMALLINT(6) NOT NULL DEFAULT 0',
  'r_cnt_service' => 'INT(4) NOT NULL DEFAULT 0', 'r_moved' => 'DATETIME NULL DEFAULT NULL');
foreach ($cols as $c => $def) {
  if (rp_has_col('dh_referral', $c)) { echo "dh_referral.$c: already there\n"; continue; }
  db_query("ALTER TABLE dh_referral ADD COLUMN $c $def");
  echo "dh_referral.$c: added\n";
}

if (db_query("select 1 from information_schema.statistics where table_schema = database() and table_name = 'dh_referral_log' and column_name = 'rl_referral' and seq_in_index = 1")->fetchField())
  echo "index rl_referral: already there\n";
else { db_query('ALTER TABLE dh_referral_log ADD INDEX rl_referral (rl_referral)'); echo "index rl_referral: added\n"; }

// The old referral copies in dh_student until step 2 merges or removes them. rc_status: open (finalize skips it),
// merged, removed, kept. rc_group: twin, own, none.
db_query("CREATE TABLE IF NOT EXISTS dh_referral_copy (
  rc_student BIGINT(20) NOT NULL,
  rc_referral BIGINT(20) NOT NULL,
  rc_group VARCHAR(10) NOT NULL DEFAULT '',
  rc_status VARCHAR(10) NOT NULL DEFAULT 'open',
  rc_batch VARCHAR(40) NULL DEFAULT NULL,
  rc_created DATETIME NOT NULL,
  rc_updated DATETIME NULL DEFAULT NULL,
  PRIMARY KEY (rc_student),
  KEY rc_status (rc_status),
  KEY rc_referral (rc_referral)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci");
echo "dh_referral_copy: ready\n";
