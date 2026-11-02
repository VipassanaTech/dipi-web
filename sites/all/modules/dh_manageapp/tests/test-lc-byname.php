<?php
// Every by-name letter lookup goes through dh_letter_by_name() (active letters only, fixed order).
// Run: php7.4 sites/all/modules/dh_manageapp/tests/test-lc-byname.php
require __DIR__ . '/lib.php';

$files = array(
  'dh_manageapp.module' => DRUPAL_ROOT . '/sites/all/modules/dh_manageapp/dh_manageapp.module',
  'dh_atportal.module' => DRUPAL_ROOT . '/sites/all/modules/dh_atportal/dh_atportal.module',
);
foreach ($files as $label => $path) {
  $src = file_get_contents($path);
  t_eq(preg_match_all("/from dh_letter where l_name\s*=/i", $src), 0, "B1 $label has no direct by-name letter query left");
}
t_eq(substr_count(file_get_contents($files['dh_manageapp.module']), 'dh_letter_by_name('), 3, 'B2 dh_manageapp.module uses the helper 3 times');
t_eq(substr_count(file_get_contents($files['dh_atportal.module']), 'dh_letter_by_name('), 1, 'B2 dh_atportal.module uses the helper once');
// B3: when the centre has an LC admin email but no ACTIVE "LC Admin Notification" letter, the skip is logged like the other branches
$mod = file_get_contents($files['dh_manageapp.module']);
t_ok(strpos($mod, 'No LC Admin Notification letter found!') !== FALSE, 'B3 the module logs "No LC Admin Notification letter found!"');
t_ok((bool) preg_match("/dh_letter_by_name\(\\\$centre, 'LC Admin Notification'\);.*?else\s*\{\s*\\\$log = \"No LC Admin Notification letter found!\";\s*logit\(\\\$centre,\s*'Letter',\s*\\\$app_id,\s*\\\$log\s*\);/s", $mod), 'B3 and it is the else branch of that lookup, with the same logit call as its siblings');
t_done();
