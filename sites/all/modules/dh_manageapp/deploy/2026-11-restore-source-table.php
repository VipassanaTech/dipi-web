<?php
// Makes the temporary table dh_restore_source for the restore of the 2020 migrated students (safe to run twice).
//   php7.3 sites/all/modules/dh_manageapp/deploy/2026-11-restore-source-table.php          make it if missing, print its row count
//   php7.3 sites/all/modules/dh_manageapp/deploy/2026-11-restore-source-table.php --drop   drop it (only once the restore batch is final)
if (php_sapi_name() !== 'cli') { echo "Command line only.\n"; exit(1); }
define('DRUPAL_ROOT', getcwd());
$_SERVER['REMOTE_ADDR'] = '127.0.0.1'; $_SERVER['HTTP_HOST'] = 'localhost'; $_SERVER['SCRIPT_NAME'] = '/index.php';
require_once DRUPAL_ROOT . '/includes/bootstrap.inc';
drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);
if (in_array('--drop', $argv)) { db_query('DROP TABLE IF EXISTS dh_restore_source'); echo "dh_restore_source dropped\n"; exit(0); }
dh_restore_source_create();
echo 'dh_restore_source: ' . db_query('select count(*) from dh_restore_source')->fetchField() . " rows\n";
