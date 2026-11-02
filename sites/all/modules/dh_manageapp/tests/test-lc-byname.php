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
t_done();
