<?php
// The drag "drop here" placeholder (.dh-seat-ph) must be exactly the size of a seat card
// (.ui-state-default), or the floated grid reflows while dragging (a card slips into the gap
// beside a shorter placeholder and every later card drops a row). css/seating.css only.
// Run from the Drupal root: php7.4 sites/all/modules/dh_manageapp/tests/test-seat-placeholder-size.php
require __DIR__ . '/lib.php';

$css = file_get_contents(DRUPAL_ROOT . '/' . drupal_get_path('module', 'dh_manageapp') . '/css/seating.css');
$css = preg_replace('#/\*.*?\*/#s', '', $css);   // drop comments
function ps_rule($css, $selector) {
  t_ok(preg_match('/(^|\})\s*' . preg_quote($selector, '/') . '\s*\{([^}]*)\}/', $css, $m) === 1, "rule $selector found");
  $out = array();
  foreach (explode(';', isset($m[2]) ? $m[2] : '') as $decl)
    if (strpos($decl, ':') !== FALSE) { list($k, $v) = explode(':', $decl, 2); $out[trim($k)] = trim($v); }
  return $out;
}
function ps_px($v) { return (int) preg_replace('/[^0-9].*$/', '', trim((string) $v)); }
function ps_border($v) { return preg_match('/(\d+)px/', (string) $v, $m) ? (int) $m[1] : 0; }

// Seat card: content-box, so its outer size adds padding and border.
$card = ps_rule($css, '.ui-state-default');
$card_w = ps_px($card['width']) + 2 * ps_border($card['border']);
$card_h = ps_px($card['height']) + ps_px(isset($card['padding-top']) ? $card['padding-top'] : 0) + 2 * ps_border($card['border']);
t_eq(array($card_w, $card_h), array(142, 83), 'seat card outer size');

// Placeholder: border-box, so its width/height are already the outer size.
$ph = ps_rule($css, '.dh-seat-ph');
t_eq(isset($ph['box-sizing']) ? $ph['box-sizing'] : '', 'border-box', 'placeholder is border-box');
t_eq(array(ps_px($ph['width']), ps_px($ph['height'])), array($card_w, $card_h), 'placeholder is exactly a card in size');
t_done();
