<?php
// Correcting a finalized course: new fields, student change log, permission.
// Run from the Drupal root:
//   local: php7.4 sites/all/modules/dh_manageapp/deploy/2026-11-finalize-sync-schema.php
//   prod:  sudo -u www-data php7.3 sites/all/modules/dh_manageapp/deploy/2026-11-finalize-sync-schema.php
// Re-runnable: anything already there is left alone.
if (php_sapi_name() !== 'cli') { echo "Command line only.\n"; exit(1); }
define('DRUPAL_ROOT', getcwd());
$_SERVER['REMOTE_ADDR'] = '127.0.0.1'; $_SERVER['HTTP_HOST'] = 'localhost'; $_SERVER['SCRIPT_NAME'] = '/index.php';
require_once DRUPAL_ROOT . '/includes/bootstrap.inc';
drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);

// dh_student keeps a 0000-00-00 default, which Drupal's session mode refuses on any ALTER.
db_query("SET SESSION sql_mode = 'STRICT_ALL_TABLES,NO_ENGINE_SUBSTITUTION'");
db_query("SET SESSION lock_wait_timeout = 15");

function fs_has_col($t, $c) {
  return (bool) db_query("select 1 from information_schema.columns where table_schema = database() and table_name = :t and column_name = :c", array(':t' => $t, ':c' => $c))->fetchField();
}
function fs_has_index($t, $i) {
  return (bool) db_query("select 1 from information_schema.statistics where table_schema = database() and table_name = :t and index_name = :i", array(':t' => $t, ':i' => $i))->fetchField();
}
function fs_run($sql, $label) { $t0 = microtime(TRUE); db_query($sql); echo "$label: done in " . round(microtime(TRUE) - $t0, 1) . "s\n"; }

$adds = array(
  array('dh_student_course', 'sc_applicant', 'ALTER TABLE dh_student_course ADD COLUMN sc_applicant BIGINT NULL DEFAULT NULL, ALGORITHM=INSTANT'),
  array('dh_applicant', 'a_attended_mail', 'ALTER TABLE dh_applicant ADD COLUMN a_attended_mail DATETIME NULL DEFAULT NULL, ALGORITHM=INSTANT'),
  array('dh_course', 'c_finalize_requested', 'ALTER TABLE dh_course ADD COLUMN c_finalize_requested TINYINT NOT NULL DEFAULT 0, ALGORITHM=INSTANT'),
  array('dh_course', 'c_finalize_mail', 'ALTER TABLE dh_course ADD COLUMN c_finalize_mail TINYINT NOT NULL DEFAULT 1, ALGORITHM=INSTANT'),
  array('dh_course', 'c_reopened', 'ALTER TABLE dh_course ADD COLUMN c_reopened DATETIME NULL DEFAULT NULL, ALGORITHM=INSTANT'),
  array('dh_course', 'c_reopened_by', 'ALTER TABLE dh_course ADD COLUMN c_reopened_by INT NULL DEFAULT NULL, ALGORITHM=INSTANT'),
);
foreach ($adds as $a)
  if (fs_has_col($a[0], $a[1])) echo "$a[0].$a[1]: already there\n"; else fs_run($a[2], "$a[0].$a[1] added");

if (fs_has_index('dh_student_course', 'sc_applicant')) echo "index sc_applicant: already there\n";
else fs_run('ALTER TABLE dh_student_course ADD INDEX sc_applicant (sc_applicant), ALGORITHM=INPLACE, LOCK=NONE', 'index sc_applicant added');

if (db_table_exists('dh_student_log')) echo "dh_student_log: already there\n";
else fs_run("CREATE TABLE dh_student_log (
  sl_id BIGINT NOT NULL AUTO_INCREMENT, sl_student BIGINT NULL, sl_applicant BIGINT NULL, sl_course BIGINT NULL,
  sl_action VARCHAR(30) NOT NULL, sl_field VARCHAR(64) NULL, sl_old MEDIUMTEXT NULL, sl_new MEDIUMTEXT NULL,
  sl_batch VARCHAR(40) NULL, sl_user INT NOT NULL DEFAULT 0, sl_tstamp DATETIME NOT NULL,
  PRIMARY KEY (sl_id), KEY sl_student (sl_student), KEY sl_course (sl_course), KEY sl_batch (sl_batch)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci", 'dh_student_log created');

// The administrator role gets the new permission; other roles are given it in People > Permissions.
$rid = db_query("select rid from role where name = 'administrator'")->fetchField();
if ($rid) { user_role_grant_permissions($rid, array('reopen any finalized course')); echo "permission granted to administrator\n"; }

// Past Attended emails: mark everyone who already got one, so a reopened course never re-sends to them.
$t0 = microtime(TRUE);
echo 'past Attended emails marked: ' . dh_fill_attended_mail_dates() . ' applicants in ' . round(microtime(TRUE) - $t0, 1) . "s\n";
