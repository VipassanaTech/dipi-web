<?php
// Referral lists and application flags: capture them to a file, or compare two captures. Changes nothing.
// A capture holds real names, so it is written OUTSIDE the repository ($DIPI_CAPTURE_DIR, default /tmp/dipi-ref-capture, mode 0700).
// Run from the Drupal root:
//   php7.4 sites/all/modules/dh_manageapp/tests/tool-referral-capture.php capture <name>
//   php7.4 sites/all/modules/dh_manageapp/tests/tool-referral-capture.php compare <a> <b> [--allow=photo,course_*] [--main-active-only]
if (php_sapi_name() !== 'cli') { echo "Command line only.\n"; exit(1); }
define('DRUPAL_ROOT', getcwd());
$_SERVER['REMOTE_ADDR'] = '127.0.0.1'; $_SERVER['HTTP_HOST'] = 'localhost'; $_SERVER['SCRIPT_NAME'] = '/index.php';
$_SERVER['HTTP_USER_AGENT'] = 'referral-capture';
require_once DRUPAL_ROOT . '/includes/bootstrap.inc';
drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);
drupal_save_session(FALSE);

$dir = getenv('DIPI_CAPTURE_DIR') ?: '/tmp/dipi-ref-capture';
if (!is_dir($dir)) mkdir($dir, 0700, TRUE);
chmod($dir, 0700);

// The dataset rows a referral list page builds (the inline 'var dataset = [...]'), keyed by referral id.
// Action cells (del, update) and the student id are dropped; every value is compared as plain text.
function rc_rows($uid, $path, $query = array()) {
  global $user;
  $user = user_load($uid);
  $_GET = $query + array('q' => $path);
  $_REQUEST = $_GET;
  $_SERVER['REQUEST_URI'] = '/' . $path;
  drupal_static_reset();
  $page = menu_execute_active_handler($path, FALSE);
  if (is_int($page)) return NULL;                       // MENU_ACCESS_DENIED or MENU_NOT_FOUND
  foreach (drupal_add_js() as $item) {
    if (!is_array($item) || $item['type'] !== 'inline' || strpos($item['data'], 'var dataset = ') === FALSE) continue;
    $s = substr($item['data'], strpos($item['data'], 'var dataset = ') + 14);
    for ($end = strpos($s, '];'); $end !== FALSE; $end = strpos($s, '];', $end + 1)) {
      $rows = json_decode(substr($s, 0, $end + 1), TRUE);
      if (!is_array($rows)) continue;
      $out = array();
      foreach ($rows as $r) {
        unset($r['del'], $r['update'], $r['sid']);
        foreach ($r as $k => $v) $r[$k] = trim(strip_tags((string) $v));
        $out[$r['ref_id']] = $r;
      }
      ksort($out);
      return $out;
    }
  }
  return array();
}

// Before Task 6 there is no dh_referral_pick(): what the OLD set_referral() would store now (a read-only copy of its query).
function rc_old_pick($a_id) {
  $res = db_query('select a_f_name, a_l_name, a_center, a_email, a_phone_mobile, a_passport, a_voter_id, a_pancard, a_aadhar, a_referral
    from dh_applicant where a_id = :a', array(':a' => $a_id))->fetchAssoc();
  if (!$res) return 0;
  $m = dh_referral_match_sql(dh_referral_who($res, 'a_'), 's.s_');
  if (!$m) return 0;
  list($cond, $args, $id_first, $order_args) = $m;
  $args += $order_args + array(':centre' => (int) $res['a_center'], ':current' => (int) $res['a_referral']);
  return (int) db_query("select r.r_id from dh_referral r join dh_student s on s.s_id = r.r_student
    where r.r_deleted = 0 and r.r_center in (0, :centre) and $cond
    order by (r.r_id = :current) desc, " . ($id_first !== '' ? "$id_first, " : '') . "r.r_id limit 1", $args)->fetchField();
}

function rc_capture($name, $dir) {
  $cap = array('today' => db_query('select curdate()')->fetchField(), 'lists' => array(), 'flags' => array());
  $c = (int) db_query('select min(c_id) from dh_center where c_id > 0')->fetchField();
  $cap['lists']['main'] = rc_rows(1, "referral/$c");
  foreach (db_query('select distinct r_center from dh_referral where r_center > 0 order by r_center')->fetchCol() as $rc)
    foreach (array('active', 'inactive', 'deleted') as $tab)
      $cap['lists']["centre-$rc-$tab"] = rc_rows(1, "center-referral/$rc", $tab == 'active' ? array() : array('show' => $tab));
  // AT lists: Active + Inactive together equal the list before Task 12 (before the tabs, ?show is ignored).
  $teachers = db_query("select u.uid, t.t_id from dh_teacher t join users u on u.name = concat(t.t_code, '.', t.t_gender)
      where t.t_status = 'Active' and u.status = 1 and exists (select 1 from dh_referral r where r.r_teacher = t.t_id and r.r_center = 0)
      order by t.t_id")->fetchAllKeyed();
  foreach ($teachers as $uid => $tid) {
    $all = (array) rc_rows($uid, 'at-portal/my-referral');
    $all += (array) rc_rows($uid, 'at-portal/my-referral', array('show' => 'inactive'));
    ksort($all);
    $cap['lists']["at-$tid"] = $all;
  }
  // Flags on applications of courses that have not ended; 'pick' = what set_referral would choose now (dh_referral_pick from Task 6 on, else its old query).
  $pick = function_exists('dh_referral_pick');
  foreach (db_query('select a.a_id, a.a_referral from dh_applicant a join dh_course c on c.c_id = a.a_course where c.c_end >= curdate() order by a.a_id') as $a)
    $cap['flags'][$a->a_id] = array('flag' => (int) $a->a_referral, 'pick' => $pick ? (int) dh_referral_pick((int) $a->a_id) : rc_old_pick((int) $a->a_id));
  file_put_contents("$dir/$name.json", json_encode($cap));
  chmod("$dir/$name.json", 0600);
  echo count($cap['lists']) . ' lists, ' . count($cap['flags']) . " applications -> $dir/$name.json\n";
}

function rc_allowed($field, $allow) {
  foreach ($allow as $p)
    if ($p !== '' && ($p === $field || (substr($p, -1) === '*' && strpos($field, substr($p, 0, -1)) === 0))) return TRUE;
  return FALSE;
}

function rc_compare($a, $b, $allow, $main_active_only) {
  $fail = 0;
  foreach (array_unique(array_merge(array_keys($a['lists']), array_keys($b['lists']))) as $list) {
    $la = isset($a['lists'][$list]) ? (array) $a['lists'][$list] : array();
    $lb = isset($b['lists'][$list]) ? (array) $b['lists'][$list] : array();
    $only_a = array_diff_key($la, $lb);
    $only_b = array_diff_key($lb, $la);
    if ($main_active_only && $list === 'main')
      foreach ($only_a as $id => $r)
        if ($r['ref_end'] !== '' && $r['ref_end'] < $b['today']) unset($only_a[$id]);
    $changed = array();
    foreach (array_intersect_key($la, $lb) as $id => $r)
      foreach (array_unique(array_merge(array_keys($r), array_keys($lb[$id]))) as $f) {
        $va = isset($r[$f]) ? $r[$f] : NULL;
        $vb = isset($lb[$id][$f]) ? $lb[$id][$f] : NULL;
        if ($va !== $vb && !rc_allowed($f, $allow)) $changed[$f][] = $id;
      }
    if ($only_a || $only_b || $changed) {
      $fail++;
      echo "$list: only in A [" . implode(',', array_keys($only_a)) . '] only in B [' . implode(',', array_keys($only_b)) . "]\n";
      foreach ($changed as $f => $ids) echo "  $f differs for " . count($ids) . ' referrals: ' . implode(',', array_slice($ids, 0, 20)) . "\n";
    }
  }
  $lost = $added = $switched = array();
  foreach ($a['flags'] as $aid => $fa) {
    if (!isset($b['flags'][$aid])) continue;
    $fb = $b['flags'][$aid];
    $was = ($fa['pick'] !== NULL) ? $fa['pick'] : $fa['flag'];
    $now = ($fb['pick'] !== NULL) ? $fb['pick'] : $fb['flag'];
    if ($was && !$now) $lost[] = $aid;
    elseif (!$was && $now) $added[] = $aid;
    elseif ($was && $now && $now != $was) $switched[] = $aid;
  }
  echo 'flags: lost ' . count($lost) . ' [' . implode(',', $lost) . '] added ' . count($added) . ' [' . implode(',', $added) . '] switched ' . count($switched) . ' [' . implode(',', $switched) . "]\n";
  if ($lost || $switched) $fail++;
  echo $fail ? "DIFFERENT ($fail)\n" : "SAME\n";
  return $fail;
}

$cmd = isset($argv[1]) ? $argv[1] : '';
if ($cmd === 'capture' && !empty($argv[2])) { rc_capture(basename($argv[2]), $dir); exit(0); }
if ($cmd === 'compare' && !empty($argv[2]) && !empty($argv[3])) {
  $allow = array(); $main = FALSE;
  foreach (array_slice($argv, 4) as $o) {
    if (strpos($o, '--allow=') === 0) $allow = explode(',', substr($o, 8));
    if ($o === '--main-active-only') $main = TRUE;
  }
  $a = json_decode(file_get_contents("$dir/" . basename($argv[2]) . '.json'), TRUE);
  $b = json_decode(file_get_contents("$dir/" . basename($argv[3]) . '.json'), TRUE);
  exit(rc_compare($a, $b, $allow, $main) ? 1 : 0);
}
echo "Usage: capture <name> | compare <a> <b> [--allow=f1,prefix*] [--main-active-only]\n";
exit(1);
