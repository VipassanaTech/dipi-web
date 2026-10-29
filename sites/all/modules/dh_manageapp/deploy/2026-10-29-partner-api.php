<?php
// ============================================================================
// Partner portal sync APIs (dipi_api/partner-api.inc), first user: the Pala team.
//   1. role "API Pala" with only the two new permissions ('api upcoming courses', 'api at book')
//   2. enable the three new actions on the Services endpoint 'external_app' (path /api)
//      (saving the endpoint also rebuilds the menu and clears the Services cache)
// The login itself (e.g. api_pala) is created by hand in DIPI (People -> Add user) and given
// the role, so no password passes through this script.
//
// Run from the Drupal root, on the command line only:
//     sudo -u www-data php7.3 sites/all/modules/dh_manageapp/deploy/2026-10-29-partner-api.php
// Re-runnable: nothing is added twice.
// ============================================================================
if (php_sapi_name() !== 'cli') { echo "Command line only.\n"; exit(1); }
define('DRUPAL_ROOT', getcwd());
$_SERVER['REMOTE_ADDR'] = '127.0.0.1'; $_SERVER['HTTP_HOST'] = 'localhost'; $_SERVER['SCRIPT_NAME'] = '/index.php';
require_once DRUPAL_ROOT . '/includes/bootstrap.inc';
drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);

$role_name = 'API Pala';
$perms = array('api upcoming courses', 'api at book');
$actions = array('upcoming-courses-updated-last-n-minutes', 'at-book-updated-last-n-minutes', 'at-book-photo');

// 1. role + permissions
$role = user_role_load_by_name($role_name);
if (!$role) {
  $role = (object) array('name' => $role_name);
  user_role_save($role);
  $role = user_role_load_by_name($role_name);
  echo "role '$role_name' created (rid {$role->rid})\n";
}
else {
  echo "role '$role_name' exists (rid {$role->rid})\n";
}
user_role_grant_permissions($role->rid, $perms);
$have = db_query("select permission from role_permission where rid = :r order by permission", array(':r' => $role->rid))->fetchCol();
echo "role permissions: " . implode(', ', $have) . "\n";

// 2. endpoint actions
$endpoint = services_endpoint_load('external_app');
if (!$endpoint) { echo "ERROR: Services endpoint 'external_app' not found\n"; exit(1); }
$changed = FALSE;
foreach ($actions as $a) {
  if (empty($endpoint->resources['dipi']['actions'][$a]['enabled'])) {
    $endpoint->resources['dipi']['actions'][$a] = array('enabled' => '1');
    $changed = TRUE;
    echo "action '$a' enabled\n";
  }
  else {
    echo "action '$a' already enabled\n";
  }
}
if ($changed) {
  services_endpoint_save($endpoint);
  echo "endpoint saved (menu rebuilt, Services cache cleared)\n";
}
echo "done. Next: create the API login in DIPI (People -> Add user) and give it the role '$role_name'.\n";
