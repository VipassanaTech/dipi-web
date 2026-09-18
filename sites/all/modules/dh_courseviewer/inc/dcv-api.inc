<?php
/**
 * @file
 * DCV API helpers + login/courses endpoints.
 */

/** Build the standard response envelope (pure — unit tested). */
function dcv_envelope($status, $data = array(), $msg = '') {
  return array('status' => $status, 'msg' => $msg, 'data' => $data);
}

/** Emit the envelope as JSON and stop. */
function dcv_json($status, $data = array(), $msg = '') {
  drupal_json_output(dcv_envelope($status, $data, $msg));
  exit;
}

/** GET dcv/ping — liveness check. */
function dcv_ping() {
  dcv_json('OK', array('pong' => true), 'dcv');
}

/**
 * POST dcv/login — validate Dipi credentials, establish a Drupal session.
 * Accepts form-encoded 'username' and 'password'. On success the response
 * sets the session cookie the app carries for the two download calls.
 * Applies core's login flood control (brute-force throttling), mirroring
 * user_login_authenticate_validate() in user.module.
 */
function dcv_login() {
  $name = isset($_POST['username']) ? trim($_POST['username']) : '';
  $pass = isset($_POST['password']) ? $_POST['password'] : '';
  if ($name === '' || $pass === '') {
    dcv_json('FAIL', array(), 'Username and password are required.');
  }

  // Brute-force protection, mirroring core user_login flood control.
  $ip_limit    = variable_get('user_failed_login_ip_limit', 50);
  $ip_window   = variable_get('user_failed_login_ip_window', 3600);
  $user_limit  = variable_get('user_failed_login_user_limit', 5);
  $user_window = variable_get('user_failed_login_user_window', 21600);

  // Per-IP gate (identifier defaults to ip_address()).
  if (!flood_is_allowed('failed_login_attempt_ip', $ip_limit, $ip_window)) {
    dcv_json('FAIL', array(), 'Too many failed login attempts from your network. Please try again later.');
  }

  $uid = user_authenticate($name, $pass);

  // Build the per-user flood identifier (uid + IP), when we can resolve the account.
  $identifier = '';
  if ($uid) {
    $identifier = $uid . '-' . ip_address();
  }
  else {
    $account = user_load_by_name($name);
    if ($account) {
      $identifier = $account->uid . '-' . ip_address();
    }
  }

  if (!$uid) {
    // Register the failed attempt against IP and (when known) the account.
    flood_register_event('failed_login_attempt_ip', $ip_window);
    if ($identifier !== '') {
      flood_register_event('failed_login_attempt_user', $user_window, $identifier);
    }
    dcv_json('FAIL', array(), 'Invalid username or password.');
  }

  // Correct password, but block if this account is over its failed-attempt limit.
  if ($identifier !== '' && !flood_is_allowed('failed_login_attempt_user', $user_limit, $user_window, $identifier)) {
    dcv_json('FAIL', array(), 'Too many failed login attempts for this account. Please try again later.');
  }

  // Success — clear the per-user counter and establish the session.
  if ($identifier !== '') {
    flood_clear_event('failed_login_attempt_user', $identifier);
  }
  global $user;
  $user = user_load($uid);
  $login = array('name' => $user->name, 'pass' => $pass);
  user_login_finalize($login); // sets up the session + Set-Cookie
  dcv_json('OK', array('uid' => (int) $uid, 'name' => $user->name), 'Logged in.');
}

/**
 * Centre ids the current user may access via dh_user_center.
 * Returns array('*') when the user has the 'access all centres' bypass.
 */
function dcv_user_centres() {
  global $user;
  if (user_access('access all centres')) {
    return array('*');
  }
  $centres = array();
  $res = db_query("select uc_center from dh_user_center where uc_deleted=0 and uc_user=:u",
    array(':u' => $user->uid));
  foreach ($res as $r) { $centres[] = (int) $r->uc_center; }
  return $centres;
}

/**
 * Build the loadable-course list for a set of centre ids.
 * $centres is an array of ints, or array('*') for all centres.
 * Excludes deleted/cancelled courses. Read-only, parameterized.
 */
function dcv_build_course_list($centres) {
  if (empty($centres)) {
    return array();
  }
  $where = "c_deleted=0 and c_cancelled=0";
  $args = array();
  if ($centres !== array('*')) {
    $where .= " and c_center in (:centres)";
    $args[':centres'] = $centres;
  }
  $sql = "select c_id, c_name, c_center, c_start, c_end
          from dh_course
          where $where
          order by c_start desc";
  $out = array();
  foreach (db_query($sql, $args) as $r) {
    $out[] = array(
      'id'         => (int) $r->c_id,
      'name'       => $r->c_name,
      'centre'     => (int) $r->c_center,
      'start_date' => $r->c_start,
      'end_date'   => $r->c_end,
    );
  }
  return $out;
}

/** GET dcv/courses — courses the logged-in caller may load. */
function dcv_courses() {
  $list = dcv_build_course_list(dcv_user_centres());
  dcv_json('OK', array('courses' => $list));
}
