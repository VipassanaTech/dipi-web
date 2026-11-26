<?php
// One-time: the automatic LC letters get their own status, LC Automatic (owner, 26 Nov 2026; inc/lc-auto-status.inc).
// Dry run by default (counts only). Run from the Drupal root:
//   dry run: php7.3 sites/all/modules/dh_manageapp/deploy/2026-11-26-lc-automatic-status.php
//   apply:   php7.3 sites/all/modules/dh_manageapp/deploy/2026-11-26-lc-automatic-status.php --apply --backup=/root/backups/<dir>/lc-auto-before.json
//   undo:    php7.3 sites/all/modules/dh_manageapp/deploy/2026-11-26-lc-automatic-status.php --undo --backup=/root/backups/<dir>/lc-auto-before.json
// --apply first writes every letter's id and old status to the backup file (never over an existing file), then
// changes them as System in one transaction; the Letter Change Log records each change. A second run changes nothing.
// Prints counts only.
if (php_sapi_name() !== 'cli') { echo "Command line only.\n"; exit(1); }
define('DRUPAL_ROOT', getcwd());
$_SERVER['REMOTE_ADDR'] = '127.0.0.1'; $_SERVER['HTTP_HOST'] = 'localhost'; $_SERVER['SCRIPT_NAME'] = '/index.php';
require_once DRUPAL_ROOT . '/includes/bootstrap.inc';
drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);
require_once dirname(__DIR__) . '/inc/lc-auto-status.inc';   // one-time code: the site itself does not load it

$mode = 'dry'; $backup = '';
foreach (array_slice($argv, 1) as $arg)
{
	if ($arg === '--apply' || $arg === '--undo')
		$mode = substr($arg, 2);
	elseif (strpos($arg, '--backup=') === 0)
		$backup = substr($arg, 9);
	else
	{
		echo "Unknown option: $arg (nothing was done)\n";
		exit(1);
	}
}
if ($mode != 'dry' && $backup === '')
{
	echo "--$mode needs --backup=<file>. Nothing was done.\n";
	exit(1);
}
$uid = dh_system_uid();

if ($mode == 'undo')
{
	$rows = is_readable($backup) ? json_decode(file_get_contents($backup), TRUE) : NULL;
	if (!is_array($rows))
	{
		echo "Cannot read the backup $backup. Nothing was done.\n";
		exit(1);
	}
	$n = dh_lc_auto_undo($rows, $uid);
	echo "Undo: $n of " . count($rows) . " letters have their old status again.\n";
	exit(0);
}

$rows = dh_lc_auto_targets();
$by = array();
foreach ($rows as $r)
	@$by[$r['l_event']]++;
$centres = count(array_unique(array_map(function ($r) { return $r['l_center']; }, $rows)));
$tpl = count(array_filter($rows, function ($r) { return (int) $r['l_center'] === 0; }));
echo "Letters to move to " . DH_LETTER_LC_AUTO_STATUS . ": " . count($rows) . " at $centres centres (template centre 0: $tpl)\n";
foreach ($by as $ev => $c)
	echo "  now $ev: $c\n";
echo "Already " . DH_LETTER_LC_AUTO_STATUS . ": " . db_query("select count(*) from dh_letter where l_event = :s", array(':s' => DH_LETTER_LC_AUTO_STATUS))->fetchField() . "\n";
if ($mode == 'dry')
{
	echo "Dry run: nothing was changed.\n";
	exit(0);
}
if (!$rows)
{
	echo "Nothing to move.\n";
	exit(0);
}
if (file_exists($backup))
{
	echo "The backup file $backup already exists. Choose a new file. Nothing was done.\n";
	exit(1);
}
if (@file_put_contents($backup, json_encode($rows)) === FALSE || json_decode(file_get_contents($backup), TRUE) != $rows)
{
	echo "Cannot write the backup $backup. Nothing was done.\n";
	exit(1);
}
$n = dh_lc_auto_apply($rows, $uid);
echo "Moved: $n letters. Backup (id + old status): $backup\n";
echo "Undo: php7.3 " . 'sites/all/modules/dh_manageapp/deploy/' . basename(__FILE__) . " --undo --backup=$backup\n";
