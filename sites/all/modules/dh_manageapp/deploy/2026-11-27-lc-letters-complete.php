<?php
// One-time: every centre gets all ten LC letters, copied from the template centre 0 (owner, 27 Nov 2026;
// inc/lc-letters-complete.inc). Dry run by default (centre ids and counts only). Run from the Drupal root:
//   dry run:  php7.3 sites/all/modules/dh_manageapp/deploy/2026-11-27-lc-letters-complete.php
//   apply:    php7.3 sites/all/modules/dh_manageapp/deploy/2026-11-27-lc-letters-complete.php --apply --backup=/root/backups/<dir>/lc-letters-added.json
//   undo:     php7.3 sites/all/modules/dh_manageapp/deploy/2026-11-27-lc-letters-complete.php --undo --backup=/root/backups/<dir>/lc-letters-added.json
// --apply adds the letters as System in one transaction, then writes the new letter ids to the backup file (never
// over an existing file; the ids are printed too if the file cannot be written). --undo deletes those letters unless a
// centre changed one since. A second --apply adds nothing.
if (php_sapi_name() !== 'cli') { echo "Command line only.\n"; exit(1); }
define('DRUPAL_ROOT', getcwd());
$_SERVER['REMOTE_ADDR'] = '127.0.0.1'; $_SERVER['HTTP_HOST'] = 'localhost'; $_SERVER['SCRIPT_NAME'] = '/index.php';
require_once DRUPAL_ROOT . '/includes/bootstrap.inc';
drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);
require_once dirname(__DIR__) . '/inc/lc-letters-complete.inc';   // one-time code: the site itself does not load it

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

if ($mode == 'undo')
{
	$ids = is_readable($backup) ? json_decode(file_get_contents($backup), TRUE) : NULL;
	if (!is_array($ids))
	{
		echo "Cannot read the backup $backup. Nothing was done.\n";
		exit(1);
	}
	list($n, $kept) = dh_lc_complete_rollback($ids);
	echo "Undo: $n of " . count($ids) . " added letters deleted; $kept kept because a centre changed them since.\n";
	exit(0);
}

try
{
	$plan = dh_lc_complete_plan();
}
catch (Exception $e)
{
	echo $e->getMessage() . " Nothing was done.\n";
	exit(1);
}
$by_centre = array(); $by_letter = array();
$roles = dh_letter_lc_roles();
foreach ($plan as $p)
{
	$by_centre[$p['centre']][] = $p['key'];
	@$by_letter[$roles[$p['key']]['name']]++;
}
echo "Letters to add: " . count($plan) . " at " . count($by_centre) . " centres\n";
foreach ($by_letter as $n => $c)
	echo "  $n: $c\n";
$groups = array();
foreach ($by_centre as $c => $keys)
	$groups[count($keys) . ' letters'][] = $c;
foreach ($groups as $g => $cs)
	echo "  centres getting $g: " . implode(',', $cs) . "\n";
if ($mode == 'dry')
{
	echo "Dry run: nothing was changed.\n";
	exit(0);
}
if (!$plan)
{
	echo "Nothing to add.\n";
	exit(0);
}
if (file_exists($backup))
{
	echo "The backup file $backup already exists. Choose a new file. Nothing was done.\n";
	exit(1);
}
if (@file_put_contents($backup, '[]') === FALSE)
{
	echo "Cannot write the backup $backup. Nothing was done.\n";
	exit(1);
}
$ids = dh_lc_complete_apply($plan, dh_system_uid());
if (@file_put_contents($backup, json_encode($ids)) === FALSE)
	echo "WARNING: could not write the ids to $backup. The new letter ids: " . json_encode($ids) . "\n";
echo "Added: " . count($ids) . " letters. New letter ids saved in $backup\n";
echo "Centres still missing an LC letter: " . count(array_unique(array_map(function ($p) { return $p['centre']; }, dh_lc_complete_plan()))) . "\n";
echo "Undo: php7.3 sites/all/modules/dh_manageapp/deploy/" . basename(__FILE__) . " --undo --backup=$backup\n";
