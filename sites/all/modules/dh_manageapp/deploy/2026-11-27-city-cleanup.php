<?php
// One-time city clean-up (owner, 27 Nov 2026; inc/city-cleanup.inc): merge the copies of each city that the old
// apostrophe bug made, and clean names saved with HTML codes or a backslash. Dry run by default (city names, ids and
// counts only; no personal data). Run from the Drupal root:
//   dry run:  php7.3 sites/all/modules/dh_manageapp/deploy/2026-11-27-city-cleanup.php
//   apply:    php7.3 sites/all/modules/dh_manageapp/deploy/2026-11-27-city-cleanup.php --apply --backup=/root/backups/<dir>/city-cleanup-before.json
//   undo:     php7.3 sites/all/modules/dh_manageapp/deploy/2026-11-27-city-cleanup.php --undo --backup=/root/backups/<dir>/city-cleanup-before.json
// --apply first writes the backup (the plan, the city rows and every record that points at a copy; never over an
// existing file), then makes the change as System in one transaction. --undo puts back what the backup holds, except
// records changed since. A second --apply finds nothing.
if (php_sapi_name() !== 'cli') { echo "Command line only.\n"; exit(1); }
define('DRUPAL_ROOT', getcwd());
$_SERVER['REMOTE_ADDR'] = '127.0.0.1'; $_SERVER['HTTP_HOST'] = 'localhost'; $_SERVER['SCRIPT_NAME'] = '/index.php';
require_once DRUPAL_ROOT . '/includes/bootstrap.inc';
drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);
require_once dirname(__DIR__) . '/inc/city-cleanup.inc';   // one-time code: the site itself does not load it

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
	$b = is_readable($backup) ? json_decode(file_get_contents($backup), TRUE) : NULL;
	if (!is_array($b) || !isset($b['plan'], $b['snap']))
	{
		echo "Cannot read the backup $backup. Nothing was done.\n";
		exit(1);
	}
	$r = dh_city_cleanup_undo($b);
	echo "Undo: $r[cities_back] cities back, $r[names_back] names back, $r[refs_back] records back; $r[kept] left as they are because they changed since.\n";
	exit(0);
}

$plan = dh_city_cleanup_plan();
$snap = dh_city_cleanup_snapshot($plan);
$drop = 0; $rename = 0;
foreach ($plan as $g)
{
	$drop += count($g['drop']);
	if ($g['name'] !== $g['old_name'])
		$rename++;
}
echo "Cities to fix: " . count($plan) . " (copies to remove: $drop, names to change: $rename)\n";
foreach ($snap['refs'] as $t => $list)
	echo "  $t: " . count($list) . " records point at a copy\n";
foreach ($plan as $g)
	echo "  keep $g[keep] $g[country]/$g[state] [$g[old_name]]" . ($g['name'] !== $g['old_name'] ? " -> [$g[name]]" : '') .
		($g['drop'] ? '; copies removed: ' . implode(',', $g['drop']) : '') . "\n";
if ($mode == 'dry')
{
	echo "Dry run: nothing was changed.\n";
	exit(0);
}
if (!$plan)
{
	echo "Nothing to do.\n";
	exit(0);
}
if (file_exists($backup))
{
	echo "The backup file $backup already exists. Choose a new file. Nothing was done.\n";
	exit(1);
}
$json = json_encode(array('plan' => $plan, 'snap' => $snap, 'made' => date('Y-m-d H:i:s')));
if ($json === FALSE || @file_put_contents($backup, $json) === FALSE || file_get_contents($backup) !== $json)
{
	echo "Cannot write the backup $backup. Nothing was done.\n";
	exit(1);
}
$done = dh_city_cleanup_apply($plan, dh_system_uid());
echo "Done: $done[deleted] copies removed, $done[renamed] names changed. Records moved to the kept city:\n";
foreach ($done['refs'] as $t => $n)
	echo "  $t: $n\n";
echo "Backup: $backup\n";
echo "Cities still to fix: " . count(dh_city_cleanup_plan()) . "\n";
echo "Undo: php7.3 sites/all/modules/dh_manageapp/deploy/" . basename(__FILE__) . " --undo --backup=$backup\n";
