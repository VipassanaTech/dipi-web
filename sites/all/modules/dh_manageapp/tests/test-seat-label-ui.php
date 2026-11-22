<?php
// The "Seat labels" button and its script (inc/seat-label-start.inc).
// Run from the Drupal root: php7.4 sites/all/modules/dh_manageapp/tests/test-seat-label-ui.php
require __DIR__ . '/lib.php';

$starts = array('M' => array('main' => 8, 'chowky' => 0), 'g1_F' => array('main' => 6, 'chowky' => 100));
$b = dh_seat_label_start_button('main', 'M', 'Male', 1, $starts);
t_ok(strpos($b, 'class="dh-sls-btn no-print"') !== FALSE, 'button class, never printed');
t_ok(strpos($b, 'data-plan="main" data-sec="M" data-label="Male" data-anchor="dh-sec-M" data-main="I" data-chowky=""') !== FALSE, 'button carries the section and its current starts');
t_ok(strpos($b, 'Seat labels') !== FALSE, 'button text');
t_ok(strpos($b, "'") === FALSE, 'no single quote in the button HTML');
$g = dh_seat_label_start_button('group', 'g1_F', 'Group 1 - Female', 2, $starts);
t_ok(strpos($g, 'data-sec="g1_F"') !== FALSE && strpos($g, 'data-main="7" data-chowky="101"') !== FALSE, 'group section, style 2 texts');
$n = dh_seat_label_start_button('main', 'F', 'Female', 0, $starts);
t_ok(strpos($n, 'data-main="" data-chowky=""') !== FALSE, 'no saved start: empty fields');

drupal_static_reset('dh_seat_label_start_js');
$js = dh_seat_label_start_js(5, 123, 2);
t_ok(strpos($js, 'seat-label-start\\/5\\/123') !== FALSE, 'script settings carry the save URL');
t_ok(strpos($js, '"conv":2') !== FALSE, 'script settings carry the naming style');
t_ok(preg_match('/"token":"[^"]+"/', $js) === 1, 'script settings carry a token');
t_ok(strpos($js, 'js/seat-label-start.js?v=') !== FALSE, 'the box script is loaded');
t_eq(dh_seat_label_start_js(5, 123, 2), '', 'printed once per page');
t_ok(file_exists('sites/all/modules/dh_manageapp/js/seat-label-start.js'), 'the script file exists');
t_done();
