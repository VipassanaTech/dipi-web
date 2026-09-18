<?php
/**
 * @file
 * DCV bundle assembly: seat-config geometry, curated students, seats.
 */

/**
 * Parse cs_seat_config INI into structured sections for the app to render.
 * Emits only [RIGHT] (Male) and [LEFT] (Female). Group overrides use the
 * GROUP<n>-<Key> convention. Pure — unit tested.
 */
function dcv_parse_seat_config($ini_string) {
  $raw = parse_ini_string($ini_string, true, INI_SCANNER_RAW);
  if ($raw === false) { return array(); }
  $key_map = array(
    'SeatsPerRow'       => 'seats_per_row',
    'SeatsPerRowChowky' => 'seats_per_row_chowky',
    'SeatDirection'     => 'seat_direction',
    'ChowkyPosition'    => 'chowky_position',
    'EmptySeats'        => 'empty_seats',
  );
  $int_keys = array('seats_per_row', 'seats_per_row_chowky');
  $sides = array('RIGHT' => array('M', 'Male'), 'LEFT' => array('F', 'Female'));
  $out = array();
  foreach ($sides as $key => $gl) {
    if (!isset($raw[$key]) || !is_array($raw[$key])) { continue; }
    $s = $raw[$key];
    $sec = array(
      'key' => $key, 'gender' => $gl[0], 'label' => $gl[1],
      'seats_per_row' => 0, 'seats_per_row_chowky' => 0,
      'seat_direction' => '', 'chowky_position' => '', 'empty_seats' => '',
      'groups' => array(),
    );
    $groups = array();
    foreach ($s as $k => $v) {
      if (preg_match('/^GROUP(\d+)-(.+)$/', $k, $m)) {
        $n = (int) $m[1];
        if (!isset($groups[$n])) { $groups[$n] = array('n' => $n); }
        if (isset($key_map[$m[2]])) {
          $field = $key_map[$m[2]];
          $groups[$n][$field] = in_array($field, $int_keys) ? (int) $v : $v;
        }
      }
      elseif (isset($key_map[$k])) {
        $field = $key_map[$k];
        $sec[$field] = in_array($field, $int_keys) ? (int) $v : $v;
      }
    }
    ksort($groups);
    $sec['groups'] = array_values($groups);
    $out[] = $sec;
  }
  return $out;
}
