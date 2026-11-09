<?php
// dh_format_mobile(): the mobile as staff see it, with its country code: '+91 98765 43210'.
// Run from the Drupal root: php7.4 sites/all/modules/dh_manageapp/tests/test-format-mobile.php
require __DIR__ . '/lib.php';

t_eq(dh_format_mobile(91, '9876543210'), '+91 98765 43210', 'India');
t_eq(dh_format_mobile('91', ' 98765 43210 '), '+91 98765 43210', 'spaces in the stored number');
t_eq(dh_format_mobile('+91', '09876543210'), '+91 98765 43210', 'cc with +, number with a leading 0');
t_eq(dh_format_mobile(91, '919876543210'), '+91 98765 43210', 'number stored with the cc in front');
t_eq(dh_format_mobile(91, '+91 9876543210'), '+91 98765 43210', 'number stored with +cc');
t_eq(dh_format_mobile(977, '9841234567'), '+977 984-1234567', 'Nepal, in its own style');
t_eq(dh_format_mobile(84, '912345678'), '+84 91 234 56 78', 'Vietnam');
t_eq(dh_format_mobile(1, '2025550123'), '+1 202-555-0123', 'USA');

// Not a valid number: still shown, as stored, after the code.
t_eq(dh_format_mobile(91, '12345'), '+91 12345', 'too short: shown as stored after +cc');
t_eq(dh_format_mobile(91, '98765 43210 / 98765 43211'), '+91 98765 43210 / 98765 43211', 'two numbers: shown as stored');
t_eq(dh_format_mobile(91, '+44 20 7946 0958'), '+44 20 7946 0958', 'own +code wins over the mobile country code');

// No country code: the number as stored.
t_eq(dh_format_mobile('', '9876543210'), '9876543210', 'no cc');
t_eq(dh_format_mobile(0, '9876543210'), '9876543210', 'cc 0');
t_eq(dh_format_mobile(NULL, '9876543210'), '9876543210', 'cc NULL');
t_eq(dh_format_mobile(999, '9876543210'), '+999 9876543210', 'unknown cc: shown after +cc');

// No number: nothing.
t_eq(dh_format_mobile(91, ''), '', 'empty number');
t_eq(dh_format_mobile(91, NULL), '', 'NULL number');
t_eq(dh_format_mobile(91, '   '), '', 'blank number');
t_done();
