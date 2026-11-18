<?php
// Step 1 move of referral person details (logic in inc/referral-move.inc). Add --dry to count only.
// Run from the Drupal root (DRUPAL_ROOT = the current directory):
//   local: php7.4 sites/all/modules/dh_manageapp/deploy/2026-11-referral-person-move.php details|links|rollback [--dry]
//   prod before the pull: copy deploy/ + inc/referral-person.inc + inc/referral-move.inc to /tmp/refmove/{deploy,inc}/ (mode 0700),
//     then: cd /dhamma/web/apps/dipi && sudo -u www-data php7.3 /tmp/refmove/deploy/2026-11-referral-person-move.php details
// Order: schema -> details -> pull the code -> links --dry (owner looks) -> links. Rollback only before step 2.
if (php_sapi_name() !== 'cli') { echo "Command line only.\n"; exit(1); }
define('DRUPAL_ROOT', getcwd());
$_SERVER['REMOTE_ADDR'] = '127.0.0.1'; $_SERVER['HTTP_HOST'] = 'localhost'; $_SERVER['SCRIPT_NAME'] = '/index.php';
require_once DRUPAL_ROOT . '/includes/bootstrap.inc';
drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);
// The new files next to this script; skipped when the module already loaded its own copy (after the pull).
if (!function_exists('dh_referral_person_map'))
	require_once __DIR__ . '/../inc/referral-person.inc';
require_once __DIR__ . '/../inc/referral-move.inc';

$mode = isset($argv[1]) ? $argv[1] : '';
$dry = in_array('--dry', $argv, TRUE);
if (!in_array($mode, array('details', 'links', 'rollback'), TRUE)) { echo "Usage: details|links|rollback [--dry]\n"; exit(1); }
if (!db_query("select 1 from information_schema.columns where table_schema = database() and table_name = 'dh_referral' and column_name = 'r_moved'")->fetchField()) { echo "Run deploy/2026-11-referral-person-schema.php first.\n"; exit(1); }
// links needs the new finalize match (dh_student_find with a skip list): the pulled module code.
if ($mode === 'links' && (new ReflectionFunction('dh_student_find'))->getNumberOfParameters() < 2) { echo "links needs the pulled code (dh_student_find skip list).\n"; exit(1); }
if (!$dry || $mode !== 'details')
	dh_refmove_prepare();
$t0 = microtime(TRUE);
$fn = 'dh_refmove_' . $mode;
$sum = $fn($dry);
ksort($sum);
foreach ($sum as $k => $v)
	echo str_pad($k, 24) . " $v\n";
echo ($dry ? 'DRY RUN, nothing written' : 'done') . ' in ' . round(microtime(TRUE) - $t0, 1) . "s\n";
