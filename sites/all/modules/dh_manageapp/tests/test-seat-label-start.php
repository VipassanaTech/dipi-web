<?php
// Seat label start: letters, typed text, labels with a start (inc/seat-label-start.inc, dh_seat_label).
// Run from the Drupal root: php7.4 sites/all/modules/dh_manageapp/tests/test-seat-label-start.php
require __DIR__ . '/lib.php';

// Letters <-> index, the inverse of dh_seat_letter().
foreach (array('A' => 0, 'I' => 8, 'Z' => 25, 'AA' => 26, 'AZ' => 51, 'BA' => 52, 'i' => 8, ' j ' => 9) as $t => $n)
  t_eq(dh_seat_letter_index($t), $n, "letter index of '$t'");
foreach (array('', '1', 'A1', 'AAAA', '-', 'É') as $t)
  t_eq(dh_seat_letter_index($t), FALSE, "not letters: '$t'");
$bad = -1;
for ($n = 0; $n < 800; $n++) if (dh_seat_letter_index(dh_seat_letter($n)) !== $n) { $bad = $n; break; }
t_eq($bad, -1, 'letters round-trip for 0..799');

// Typed text -> offset. Empty = 0 (normal start). FALSE = not allowed.
$cases = array(
  array(1, 'I', 8), array(1, 'a', 0), array(1, 'AA', 26), array(1, '', 0), array(1, '7', FALSE), array(1, 'I1', FALSE),
  array(2, '7', 6), array(2, '1', 0), array(2, '', 0), array(2, '0', FALSE), array(2, 'G', FALSE), array(2, '-3', FALSE), array(2, '2.5', FALSE),
  array(0, '101', 100), array(0, '1', 0), array(0, ' 12 ', 11), array(0, '0', FALSE), array(0, '100000', FALSE),
);
foreach ($cases as $c)
  t_eq(dh_seat_label_start_parse($c[0], $c[1]), $c[2], "style {$c[0]}: '{$c[1]}'");

// Offset -> the text shown in the box ('' = normal start).
foreach (array(array(1, 8, 'I'), array(1, 26, 'AA'), array(2, 6, '7'), array(0, 100, '101'), array(1, 0, ''), array(2, 0, ''), array(0, 0, '')) as $c)
  t_eq(dh_seat_label_start_text($c[0], $c[1]), $c[2], "style {$c[0]} offset {$c[1]} shows '{$c[2]}'");

// The hint used in error messages.
t_eq(dh_seat_label_start_hint(1), 'type a letter, for example I. Leave it empty for the normal start.', 'hint, style 1');
t_eq(dh_seat_label_start_hint(2), 'type a column number of 1 or more, for example 7. Leave it empty for the normal start.', 'hint, style 2');
t_eq(dh_seat_label_start_hint(0), 'type a seat number of 1 or more, for example 101. Leave it empty for the normal start.', 'hint, numbered');

// dh_seat_label with a start; without one it is unchanged.
t_eq(dh_seat_label(0, 0, 1), 'A1', 'style 1, no start');
t_eq(dh_seat_label(0, 0, 1, 8), 'I1', 'style 1 from I: first seat I1');
t_eq(dh_seat_label(2, 3, 1, 8), 'K4', 'style 1 from I: third column, fourth row');
t_eq(dh_seat_label(0, 0, 1, 25), 'Z1', 'style 1 from Z');
t_eq(dh_seat_label(1, 0, 1, 25), 'AA1', 'style 1 past Z goes on to AA');
t_eq(dh_seat_label(0, 0, 2), 'A1', 'style 2, no start');
t_eq(dh_seat_label(0, 0, 2, 6), 'A7', 'style 2 from 7: first seat A7');
t_eq(dh_seat_label(3, 1, 2, 6), 'B10', 'style 2 from 7: fourth column, second row');
t_done();
