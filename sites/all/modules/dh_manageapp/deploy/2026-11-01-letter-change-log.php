<?php
// Letter change log: table dh_letter_log, column dh_letter_fields.lf_updated_by, and 5 triggers that
// write one row per changed letter or merge field (who, when, old -> new).
// Run from the Drupal root:
//   local: php7.4 sites/all/modules/dh_manageapp/deploy/2026-11-01-letter-change-log.php
//   prod:  sudo -u www-data php7.3 sites/all/modules/dh_manageapp/deploy/2026-11-01-letter-change-log.php
// Re-runnable: the table and the column are added only when missing; the triggers are replaced.
// Rollback: the same command with --rollback drops the 5 triggers (the table and the column stay).
// A failing statement prints FAILED and exits 1.
if (php_sapi_name() !== 'cli') { echo "Command line only.\n"; exit(1); }
define('DRUPAL_ROOT', getcwd());
$_SERVER['REMOTE_ADDR'] = '127.0.0.1'; $_SERVER['HTTP_HOST'] = 'localhost'; $_SERVER['SCRIPT_NAME'] = '/index.php';
require_once DRUPAL_ROOT . '/includes/bootstrap.inc';
drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);

// One log INSERT. Arguments are SQL expressions; $f is the column name written to ll_field.
function ll_ins($area, $c, $r, $n, $a, $f, $o, $v, $who = 'who')
{
	return "INSERT INTO dh_letter_log (ll_center, ll_area, ll_record, ll_name, ll_action, ll_field, ll_old, ll_new, ll_user, ll_tstamp) "
		."VALUES (IFNULL($c, 0), '$area', $r, IFNULL($n, ''), $a, '$f', $o, $v, $who, ts);";
}

try {

	$triggers = array('dh_letter_ai', 'dh_letter_au', 'dh_letter_ad', 'dh_letter_fields_ai', 'dh_letter_fields_au');
	$conn = Database::getConnection();
	// The session mode is stored with each trigger: prod's mode, on both servers. A short lock wait
	// gives up instead of queueing letter queries.
	$conn->exec("SET SESSION sql_mode = 'ERROR_FOR_DIVISION_BY_ZERO,NO_AUTO_CREATE_USER'");
	$conn->exec("SET SESSION lock_wait_timeout = 5");

	if (in_array('--rollback', $argv))
	{
		foreach ($triggers as $t)
		{
			$conn->exec("DROP TRIGGER IF EXISTS $t");
			echo "$t dropped\n";
		}
		exit(0);
	}

	$conn->exec("CREATE TABLE IF NOT EXISTS dh_letter_log (
  ll_id      bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  ll_center  bigint(20) NOT NULL,
  ll_area    varchar(10) NOT NULL,
  ll_record  bigint(20) NOT NULL,
  ll_name    varchar(100) NOT NULL DEFAULT '',
  ll_action  varchar(10) NOT NULL,
  ll_field   varchar(64) NOT NULL,
  ll_old     longtext,
  ll_new     longtext,
  ll_user    bigint(20) NOT NULL,
  ll_tstamp  datetime NOT NULL,
  PRIMARY KEY (ll_id),
  KEY ll_center_tstamp (ll_center, ll_tstamp),
  KEY ll_record_tstamp (ll_area, ll_record, ll_tstamp)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci");
	echo "dh_letter_log: ready\n";

	if (db_query("select 1 from information_schema.columns where table_schema = database() and table_name = 'dh_letter_fields' and column_name = 'lf_updated_by'")->fetchField())
		echo "dh_letter_fields.lf_updated_by: already there\n";
	else
	{
		$conn->exec("ALTER TABLE dh_letter_fields ADD COLUMN lf_updated_by BIGINT NULL DEFAULT NULL");
		echo "dh_letter_fields.lf_updated_by: added\n";
	}

	// Text columns compare exactly (BINARY, so a capital-letters change counts); NULL equals ''.
	$ltext = array('l_name', 'l_event', 'l_subject', 'l_body', 'l_sms', 'l_attachment');
	$lnum = array('l_course_type');

	$sql = array();
	$s = "CREATE OR REPLACE TRIGGER dh_letter_ai AFTER INSERT ON dh_letter FOR EACH ROW\nBEGIN\n  DECLARE who BIGINT;\n  DECLARE ts DATETIME;\n"
		."  SET who = IFNULL(NEW.l_created_by, IFNULL(NEW.l_updated_by, 0));\n  SET ts = NOW();\n";
	foreach ($ltext as $f)
		$s .= "  IF IFNULL(NEW.$f, '') <> '' THEN\n    ".ll_ins('letter', 'NEW.l_center', 'NEW.l_id', 'NEW.l_name', "'create'", $f, 'NULL', "NEW.$f")."\n  END IF;\n";
	foreach (array_merge($lnum, array('l_deleted')) as $f)
		$s .= "  IF IFNULL(NEW.$f, 0) <> 0 THEN\n    ".ll_ins('letter', 'NEW.l_center', 'NEW.l_id', 'NEW.l_name', "'create'", $f, 'NULL', "NEW.$f")."\n  END IF;\n";
	$sql['dh_letter_ai'] = $s."END";

	$s = "CREATE OR REPLACE TRIGGER dh_letter_au AFTER UPDATE ON dh_letter FOR EACH ROW\nBEGIN\n  DECLARE act VARCHAR(10) DEFAULT 'update';\n  DECLARE who BIGINT;\n  DECLARE ts DATETIME;\n"
		."  SET who = IFNULL(NEW.l_updated_by, 0);\n  SET ts = NOW();\n"
		."  IF NOT (IFNULL(OLD.l_deleted, 0) <=> IFNULL(NEW.l_deleted, 0)) THEN\n"
		."    SET act = IF(IFNULL(NEW.l_deleted, 0) = 1, 'inactive', 'active');\n"
		."    ".ll_ins('letter', 'NEW.l_center', 'NEW.l_id', 'NEW.l_name', 'act', 'l_deleted', 'IFNULL(OLD.l_deleted, 0)', 'IFNULL(NEW.l_deleted, 0)')."\n  END IF;\n";
	foreach ($ltext as $f)
		$s .= "  IF BINARY IFNULL(OLD.$f, '') <> BINARY IFNULL(NEW.$f, '') THEN\n    ".ll_ins('letter', 'NEW.l_center', 'NEW.l_id', 'NEW.l_name', 'act', $f, "OLD.$f", "NEW.$f")."\n  END IF;\n";
	foreach ($lnum as $f)
		$s .= "  IF NOT (IFNULL(OLD.$f, 0) <=> IFNULL(NEW.$f, 0)) THEN\n    ".ll_ins('letter', 'NEW.l_center', 'NEW.l_id', 'NEW.l_name', 'act', $f, "OLD.$f", "NEW.$f")."\n  END IF;\n";
	// A letter moved to another centre: a row in the log of both centres.
	$s .= "  IF NOT (IFNULL(OLD.l_center, 0) <=> IFNULL(NEW.l_center, 0)) THEN\n"
		."    ".ll_ins('letter', 'OLD.l_center', 'NEW.l_id', 'NEW.l_name', 'act', 'l_center', 'OLD.l_center', 'NEW.l_center')."\n"
		."    ".ll_ins('letter', 'NEW.l_center', 'NEW.l_id', 'NEW.l_name', 'act', 'l_center', 'OLD.l_center', 'NEW.l_center')."\n  END IF;\n";
	$sql['dh_letter_au'] = $s."END";

	// Removed from the database (only direct SQL does this): every filled field, so it can be rebuilt. Nobody to credit.
	$s = "CREATE OR REPLACE TRIGGER dh_letter_ad AFTER DELETE ON dh_letter FOR EACH ROW\nBEGIN\n  DECLARE ts DATETIME;\n  SET ts = NOW();\n";
	foreach ($ltext as $f)
		$s .= "  IF IFNULL(OLD.$f, '') <> '' THEN\n    ".ll_ins('letter', 'OLD.l_center', 'OLD.l_id', 'OLD.l_name', "'delete'", $f, "OLD.$f", 'NULL', '0')."\n  END IF;\n";
	foreach (array_merge($lnum, array('l_deleted')) as $f)
		$s .= "  IF IFNULL(OLD.$f, 0) <> 0 THEN\n    ".ll_ins('letter', 'OLD.l_center', 'OLD.l_id', 'OLD.l_name', "'delete'", $f, "OLD.$f", 'NULL', '0')."\n  END IF;\n";
	$sql['dh_letter_ad'] = $s."END";

	$ftext = array('lf_name', 'lf_value');
	$s = "CREATE OR REPLACE TRIGGER dh_letter_fields_ai AFTER INSERT ON dh_letter_fields FOR EACH ROW\nBEGIN\n  DECLARE who BIGINT;\n  DECLARE ts DATETIME;\n"
		."  SET who = IFNULL(NEW.lf_updated_by, 0);\n  SET ts = NOW();\n";
	foreach ($ftext as $f)
		$s .= "  IF IFNULL(NEW.$f, '') <> '' THEN\n    ".ll_ins('field', 'NEW.lf_center', 'NEW.lf_id', 'NEW.lf_name', "'create'", $f, 'NULL', "NEW.$f")."\n  END IF;\n";
	$s .= "  IF IFNULL(NEW.lf_deleted, 0) <> 0 THEN\n    ".ll_ins('field', 'NEW.lf_center', 'NEW.lf_id', 'NEW.lf_name', "'create'", 'lf_deleted', 'NULL', 'NEW.lf_deleted')."\n  END IF;\n";
	$sql['dh_letter_fields_ai'] = $s."END";

	$s = "CREATE OR REPLACE TRIGGER dh_letter_fields_au AFTER UPDATE ON dh_letter_fields FOR EACH ROW\nBEGIN\n  DECLARE act VARCHAR(10) DEFAULT 'update';\n  DECLARE who BIGINT;\n  DECLARE ts DATETIME;\n"
		."  SET who = IFNULL(NEW.lf_updated_by, 0);\n  SET ts = NOW();\n"
		."  IF NOT (IFNULL(OLD.lf_deleted, 0) <=> IFNULL(NEW.lf_deleted, 0)) THEN\n"
		."    SET act = IF(IFNULL(NEW.lf_deleted, 0) = 1, 'delete', 'active');\n"
		."    ".ll_ins('field', 'NEW.lf_center', 'NEW.lf_id', 'NEW.lf_name', 'act', 'lf_deleted', 'IFNULL(OLD.lf_deleted, 0)', 'IFNULL(NEW.lf_deleted, 0)')."\n  END IF;\n";
	foreach ($ftext as $f)
		$s .= "  IF BINARY IFNULL(OLD.$f, '') <> BINARY IFNULL(NEW.$f, '') THEN\n    ".ll_ins('field', 'NEW.lf_center', 'NEW.lf_id', 'NEW.lf_name', 'act', $f, "OLD.$f", "NEW.$f")."\n  END IF;\n";
	$s .= "  IF NOT (IFNULL(OLD.lf_center, 0) <=> IFNULL(NEW.lf_center, 0)) THEN\n"
		."    ".ll_ins('field', 'OLD.lf_center', 'NEW.lf_id', 'NEW.lf_name', 'act', 'lf_center', 'OLD.lf_center', 'NEW.lf_center')."\n"
		."    ".ll_ins('field', 'NEW.lf_center', 'NEW.lf_id', 'NEW.lf_name', 'act', 'lf_center', 'OLD.lf_center', 'NEW.lf_center')."\n  END IF;\n";
	$sql['dh_letter_fields_au'] = $s."END";

	foreach ($sql as $t => $s)
		$conn->exec($s);
	$got = db_query("select trigger_name, sql_mode from information_schema.triggers where trigger_schema = database() and trigger_name in (:t)", array(':t' => $triggers))->fetchAllKeyed();
	foreach ($triggers as $t)
		echo "$t: ".(isset($got[$t]) ? 'installed, sql_mode '.$got[$t] : 'MISSING')."\n";
	exit(count($got) == 5 ? 0 : 1);
} catch (Exception $e) {
	echo "FAILED: ".$e->getMessage()."\n";
	exit(1);
}
