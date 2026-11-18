<?php
// LC email review links: when a step's AT changes, wipe that step's review code, so the former AT's email link
// stops working. Adds two checks to the end of trigger dh_lc_bu (BEFORE UPDATE ON dh_applicant_lc); the rest of the
// trigger (teacher ids) stays as it is. DIPI makes a fresh code with the next review email (letters.inc
// dh_get_letter, which must be deployed FIRST). Existing codes are not touched, so open reviews keep working.
//   - recommending AT name changes -> al_recommending_auth = NULL
//   - area AT name changes         -> al_area_auth = NULL
//   Names are compared by the column collation (letter case and trailing spaces do not count). A code written in the
//   same save is kept. NULL, never '': an empty code must not match any link.
// Run from the Drupal root:
//   local: php7.4 sites/all/modules/dh_manageapp/deploy/2026-11-18-lc-review-code-reset.php
//   prod:  sudo -u www-data php7.3 sites/all/modules/dh_manageapp/deploy/2026-11-18-lc-review-code-reset.php
// Re-runnable: does nothing when the checks are already there. Refuses (exit 1, nothing changed) when dh_lc_bu is not
// the version this script knows. Rollback: the same command with --rollback puts back the exact original trigger.
if (php_sapi_name() !== 'cli') { echo "Command line only.\n"; exit(1); }
define('DRUPAL_ROOT', getcwd());
$_SERVER['REMOTE_ADDR'] = '127.0.0.1'; $_SERVER['HTTP_HOST'] = 'localhost'; $_SERVER['SCRIPT_NAME'] = '/index.php';
require_once DRUPAL_ROOT . '/includes/bootstrap.inc';
drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);

// The trigger body on local and prod before this change (CRLF line ends, md5 below).
$original = str_replace("\n", "\r\n", <<<'SQL'
BEGIN

    DECLARE recommending_id INT;
    DECLARE area_at_id INT;

    IF (new.al_recommending <> '') THEN
      SELECT t_id into recommending_id from dh_teacher where CONCAT(t_f_name, ' ', t_l_name) = new.al_recommending limit 1;
      IF (recommending_id <> '') THEN
        SET new.al_recommending_id = recommending_id;
      ELSE
        SET new.al_recommending_id = null;
      END IF;
    END IF;

    IF (new.al_area_at <> '') THEN
      SELECT t_id into area_at_id from dh_teacher where CONCAT(t_f_name, ' ', t_l_name) = new.al_area_at limit 1;
      IF (area_at_id <> '') THEN
        SET new.al_area_at_id = area_at_id;
      ELSE
        SET new.al_area_at_id = null;
      END IF;
    END IF;

END
SQL
);
$original_md5 = '6245a9e51b45facaebc963bff5c2520d';

$checks = "    IF NOT (new.al_recommending <=> old.al_recommending) AND (new.al_recommending_auth <=> old.al_recommending_auth) THEN\r\n"
	. "      SET new.al_recommending_auth = NULL;\r\n"
	. "    END IF;\r\n"
	. "\r\n"
	. "    IF NOT (new.al_area_at <=> old.al_area_at) AND (new.al_area_auth <=> old.al_area_auth) THEN\r\n"
	. "      SET new.al_area_auth = NULL;\r\n"
	. "    END IF;\r\n"
	. "\r\n";
$changed = substr($original, 0, strrpos($original, 'END')) . $checks . 'END';

try {
	if (md5($original) !== $original_md5) { echo "FAILED: the original text in this script is damaged\n"; exit(1); }
	$conn = Database::getConnection();
	// The session mode is stored with the trigger: prod's mode, on both servers. A short lock wait gives up instead
	// of queueing application saves.
	$conn->exec("SET SESSION sql_mode = 'ERROR_FOR_DIVISION_BY_ZERO,NO_AUTO_CREATE_USER'");
	$conn->exec("SET SESSION lock_wait_timeout = 5");
	$now = db_query("select action_statement from information_schema.triggers where trigger_schema = database() and trigger_name = 'dh_lc_bu'")->fetchField();
	$rollback = in_array('--rollback', $argv);
	$want = $rollback ? $original : $changed;

	if ($now === $want) { echo "dh_lc_bu: already " . ($rollback ? 'the original' : 'has the review-code checks') . "; nothing to do\n"; exit(0); }
	if ($now !== ($rollback ? $changed : $original)) {
		echo "dh_lc_bu: REFUSED, it is not the version this script knows (md5 " . ($now === FALSE ? 'none, trigger missing' : md5($now)) . "); nothing changed\n";
		exit(1);
	}
	$conn->exec("CREATE OR REPLACE TRIGGER dh_lc_bu BEFORE UPDATE ON dh_applicant_lc FOR EACH ROW\n" . $want);
	$got = db_query("select action_statement s, sql_mode m from information_schema.triggers where trigger_schema = database() and trigger_name = 'dh_lc_bu'")->fetchAssoc();
	$ok = $got && $got['s'] === $want;
	echo "dh_lc_bu: " . ($ok ? ($rollback ? 'original put back' : 'review-code checks added') . ", sql_mode {$got['m']}, md5 " . md5($got['s']) : 'FAILED: not as expected after the change') . "\n";
	exit($ok ? 0 : 1);
} catch (Throwable $e) {
	echo "FAILED: " . $e->getMessage() . "\n";
	exit(1);
}
