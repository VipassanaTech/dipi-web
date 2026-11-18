<?php
// Step 2: merge or remove the old referral copies (logic in inc/referral-cleanup.inc).
// Run from the Drupal root:
//   php7.4 sites/all/modules/dh_manageapp/deploy/2026-11-referral-copy-cleanup.php dry      (counts only)
//   php7.4 sites/all/modules/dh_manageapp/deploy/2026-11-referral-copy-cleanup.php apply    (prints the batch id: keep it)
//   php7.4 sites/all/modules/dh_manageapp/deploy/2026-11-referral-copy-cleanup.php undo refcopy-YYYYMMDD-HHMMSS
//   prod: sudo -u www-data php7.3 ... (back up first: see the plan's rollout)
if (php_sapi_name() !== 'cli') { echo "Command line only.\n"; exit(1); }
define('DRUPAL_ROOT', getcwd());
$_SERVER['REMOTE_ADDR'] = '127.0.0.1'; $_SERVER['HTTP_HOST'] = 'localhost'; $_SERVER['SCRIPT_NAME'] = '/index.php';
require_once DRUPAL_ROOT . '/includes/bootstrap.inc';
drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);
require_once __DIR__ . '/../inc/referral-cleanup.inc';

$mode = isset($argv[1]) ? $argv[1] : '';
$t0 = microtime(TRUE);
if ($mode === 'dry' || $mode === 'apply')
	$sum = dh_refclean_run($mode === 'apply');
elseif ($mode === 'undo' && !empty($argv[2]) && preg_match('/^refcopy-\d{8}-\d{6}$/', $argv[2]))
	$sum = dh_refclean_undo($argv[2]);
else
{
	echo "Usage: dry | apply | undo refcopy-YYYYMMDD-HHMMSS\n";
	exit(1);
}
foreach ($sum as $k => $v)
	echo str_pad($k, 20) . ' ' . (is_array($v) ? count($v) : $v) . "\n";
echo 'done in ' . round(microtime(TRUE) - $t0, 1) . "s\n";
