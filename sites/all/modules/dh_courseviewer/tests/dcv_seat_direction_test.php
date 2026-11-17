<?php
// sites/all/modules/dh_courseviewer/tests/dcv_seat_direction_test.php
//
// Seat direction as Dipi's seating plan draws it (zero-day.inc dh_seat_direction): 'right' =
// left to right, 'left' = right to left, and anything else (most centres leave it unset) = the
// section's natural side — the RIGHT (Male) section left to right, the LEFT (Female) section
// right to left. The bundle sends the resolved value, so every app version draws the same order
// (and puts an unset chowky block on the same side) as Dipi. Runs against THIS worktree's code;
// the fixture centre's seat config is changed only inside a transaction that is rolled back.
if (PHP_SAPI !== 'cli') { exit; }
define('DRUPAL_ROOT', realpath(__DIR__ . '/../../../../..')); chdir(DRUPAL_ROOT);
require_once DRUPAL_ROOT . '/includes/bootstrap.inc';
drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);
foreach (array('dcv-api.inc', 'dcv-bundle.inc', 'dcv-plans.inc') as $f) {
  require_once DRUPAL_ROOT . '/sites/all/modules/dh_courseviewer/inc/' . $f;
}
function ok($c, $m) { echo ($c ? "PASS" : "FAIL") . " - $m\n"; if (!$c) { $GLOBALS['fail'] = 1; } }
function dirs($b) { $o = array(); foreach ($b['sections'] as $s) { $o[$s['key']] = $s['seat_direction']; } return $o; }
const COURSE = 9900002;   // centre 5

// --- the rule (pure)
ok(dcv_seat_direction('', 'RIGHT') === 'right' && dcv_seat_direction('', 'LEFT') === 'left', 'unset: RIGHT (Male) left to right, LEFT (Female) right to left');
ok(dcv_seat_direction('left', 'RIGHT') === 'left' && dcv_seat_direction('right', 'LEFT') === 'right', 'a set direction wins');
ok(dcv_seat_direction(' Right ', 'LEFT') === 'right' && dcv_seat_direction('LEFT', 'RIGHT') === 'left', 'set values are trimmed and lower-cased');
ok(dcv_seat_direction('ltr', 'LEFT') === 'left' && dcv_seat_direction('0', 'RIGHT') === 'right', 'an unknown value counts as unset, as in Dipi');
ok(dcv_chowky_position('', 'right') === 'right' && dcv_chowky_position('', 'left') === 'left', 'unset chowky: on the side the seats run to, as Dipi (dh_chowky_position)');
ok(dcv_chowky_position(' Back ', 'right') === 'back' && dcv_chowky_position('left', 'right') === 'left' && dcv_chowky_position('top', 'left') === 'left', 'a set chowky side wins; unknown counts as unset');

$cfg = db_query("select cs_seat_config from dh_center_setting where cs_center = 5")->fetchField();
ok($cfg !== FALSE && $cfg !== '', 'fixture: centre 5 has a seat config');
$txn = db_transaction();
try {
  $set = function ($ini) {
    db_update('dh_center_setting')->fields(array('cs_seat_config' => $ini))->condition('cs_center', 5)->execute();
  };
  // No SeatDirection at all (as in 112 live centres).
  $none = preg_replace('/^\s*SeatDirection\s*=.*$\R?/mi', '', $cfg);
  $set($none);
  ok(!preg_match('/^\s*SeatDirection\s*=/mi', $none), 'fixture: the sections\' SeatDirection removed for this check (group lines stay)');
  ok(dirs(dcv_build_bundle(COURSE)) === array('RIGHT' => 'right', 'LEFT' => 'left'), 'unset: the bundle sends RIGHT left to right, LEFT right to left');
  ok(dirs(dcv_build_bundle(COURSE, array('LEFT'))) === array('LEFT' => 'left'), 'a one-plan bundle too');

  // Both set to the opposite of the natural side.
  $flipped = preg_replace('/^(\[RIGHT\]\s*\R)/mi', "$1SeatDirection = left\n", $none);
  $flipped = preg_replace('/^(\[LEFT\]\s*\R)/mi', "$1SeatDirection = right\n", $flipped);
  $set($flipped);
  ok(dirs(dcv_build_bundle(COURSE)) === array('RIGHT' => 'left', 'LEFT' => 'right'), 'set: the bundle keeps the centre\'s choice');

  // Group-wise plans (Dipi dh_group_dims): a group's own direction / chowky side wins, else the
  // section's, else the natural side. The fixture keeps "GROUP1-SeatDirection = right" in RIGHT.
  $grp = preg_replace('/^(\[RIGHT\]\s*\R)/mi', "$1SeatDirection = right\nGROUP2-SeatDirection = left\nGROUP3-ChowkyPosition = back\n", $none);
  $set($grp);
  $right = NULL; $left = NULL;
  foreach (dcv_build_bundle(COURSE)['sections'] as $sec) { if ($sec['key'] === 'RIGHT') { $right = $sec; } else { $left = $sec; } }
  $g = array(); foreach ($right['groups'] as $x) { $g[$x['n']] = array($x['seat_direction'], $x['chowky_position']); }
  ok($right['chowky_position'] === 'right' && $left['chowky_position'] === 'left', 'sections: an unset chowky is sent resolved (Male right, Female left)');
  ok(isset($g[2]) && $g[2] === array('left', 'left'), 'group 2: its own right-to-left, chowky on the left');
  ok(isset($g[3]) && $g[3] === array('right', 'back'), "group 3: the section's direction, its own chowky at the back");
  ok(isset($g[1]) && $g[1] === array('right', 'right'), 'group 1: its own left-to-right, chowky on the right');
}
finally {
  $txn->rollback();
}
ok(db_query("select cs_seat_config from dh_center_setting where cs_center = 5")->fetchField() === $cfg, 'rolled back: centre 5 seat config as before');

echo empty($GLOBALS['fail']) ? "ALL PASS\n" : "FAILURES\n";
exit(empty($GLOBALS['fail']) ? 0 : 1);
