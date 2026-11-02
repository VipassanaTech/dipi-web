<?php
// The search results page (dh_manageapp_search_results) loads the age groups script and calls it on every redraw.
// Run from the Drupal root: php7.4 sites/all/modules/dh_manageapp/tests/test-search-age-render.php
require __DIR__ . '/lib.php';

$out = dh_manageapp_search_results('applicant', 'a_id = -1', 5);
t_ok(strpos($out, 'table-applicants') !== FALSE, 'results table rendered');

$js = drupal_add_js();
$file = drupal_get_path('module', 'dh_manageapp') . '/js/search-age-groups.js';
t_ok(isset($js[$file]), 'age groups script is loaded');
t_ok(file_exists(DRUPAL_ROOT . '/' . $file), 'age groups script exists');

$inline = '';
foreach ($js as $item) if (isset($item['type']) && $item['type'] == 'inline') $inline .= $item['data'];
$cb = strpos($inline, '"drawCallback": function (settings) {');
t_ok($cb !== FALSE, 'table has its redraw step');
$call = strpos($inline, 'if (window.dhAgeGroups) { window.dhAgeGroups.update(api); }');
t_ok($call !== FALSE && $call > $cb, 'redraw step updates the age groups');
t_eq(substr_count($inline, 'dhAgeGroups.update('), 1, 'called in one place only');

$css = file_get_contents(DRUPAL_ROOT . '/' . drupal_get_path('module', 'dh_manageapp') . '/css/tables.css');
t_ok(strpos($css, '.dh-age-groups') !== FALSE, 'age groups line is styled');
t_done();
