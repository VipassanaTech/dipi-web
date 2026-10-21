<?php
/**
 * @file
 * DCV API helpers + login/courses endpoints.
 */

/** Build the standard response envelope (pure — unit tested). */
function dcv_envelope($status, $data = array(), $msg = '') {
  // An empty PHP array JSON-encodes as [], which breaks typed clients that
  // expect data to be an object or null. Normalize empty data to null so the
  // envelope's data is always an object (success) or null (error/empty).
  if ($data === array()) {
    $data = NULL;
  }
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
 * Access callback for the two download endpoints: centre users ('access zero day' with at
 * least one of Dipi's gender permissions, see dcv_centre_sides()) and teachers ('dcv teacher
 * access'). What each may load is narrowed per course by dcv_build_course_list() (the list)
 * and dcv_bundle_scope() / dcv_course_access() (the download).
 *
 * Teachers have their own permission, separate from the AT Portal's 'at view courses',
 * so the AT course view on the website can be on while the app stays off for teachers.
 * The app needs both on for a teacher (see dcv_teacher_id()).
 */
function dcv_api_access() {
  return (user_access('access zero day') && dcv_centre_sides())
    || user_access('dcv teacher access');
}

/**
 * Centres the caller may load courses from as a centre user: dcv_user_centres(),
 * but only with 'access zero day' — a teacher's own centre links grant nothing here.
 */
function dcv_scope_centres() {
  return user_access('access zero day') ? dcv_user_centres() : array();
}

/** The sides Dipi's gender permissions allow (array('M','F'), array('M'), array('F') or array()). Pure. */
function dcv_gender_sides($male, $female) {
  $sides = array();
  if ($male) { $sides[] = 'M'; }
  if ($female) { $sides[] = 'F'; }
  return $sides;
}

/**
 * A centre user's sides: Dipi's 'access male' / 'access female' ("Access Male / Female
 * Applications"), which limit the website's applicant pages and lists the same way. With both,
 * a course of their centre comes whole; with one, only that side; with neither, nothing.
 * These are the user's effective permissions over all their roles, as the website uses them —
 * e.g. the AT Portal role itself holds both, so a centre login that is also an AT gets both.
 */
function dcv_centre_sides() {
  return dcv_gender_sides(user_access('access male'), user_access('access female'));
}

/**
 * The sides a caller gets of one course: NULL = the whole course (a centre user of its centre
 * with both genders); else their centre sides (if it is their centre) plus their teacher
 * sides ($teacher_sides: an array, or FALSE when not a teacher of it), in M, F order —
 * array() = nothing. Pure.
 */
function dcv_course_sides($in_centre, array $centre_sides, $teacher_sides) {
  if ($in_centre && in_array('M', $centre_sides, TRUE) && in_array('F', $centre_sides, TRUE)) {
    return NULL;
  }
  $sides = $in_centre ? $centre_sides : array();
  if (is_array($teacher_sides)) { $sides = array_merge($sides, $teacher_sides); }
  return array_values(array_intersect(array('M', 'F'), $sides));
}

/**
 * The caller's active teacher id when they may use the app as a teacher, else
 * FALSE. The app serves teachers only while Dipi's AT course view is on
 * ('at view courses') AND the app switch ('dcv teacher access') is on: switching either
 * off stops teachers in the app. Same identity as Dipi's AT portal
 * (_at_view_teacher_id(): username = t_code.t_gender), so no AT portal module → no
 * teacher access.
 */
function dcv_teacher_id() {
  if (!user_access('dcv teacher access') || !user_access('at view courses')
      || !function_exists('_at_view_teacher_id')) {
    return FALSE;
  }
  $tid = _at_view_teacher_id();
  return $tid ? (int) $tid : FALSE;
}

/**
 * SQL for the courses a teacher serves, as in Dipi's AT portal: Confirmed and
 * Conducting or Assisting. Trainee, cancelled and unconfirmed mappings grant nothing.
 */
function dcv_teacher_courses_sql() {
  return "select ct_course from dh_course_teacher
          where ct_teacher = :tid and ct_status = 'Confirmed'
            and ct_type in ('Conducting','Assisting')";
}

/** True if teacher $tid serves course $course_id (and it isn't deleted or cancelled). */
function dcv_teacher_mapped($tid, $course_id) {
  if (!$tid) { return false; }
  $sql = "select count(*) from dh_course
          where c_id = :c and c_deleted = 0 and c_cancelled = 0
            and c_id in (" . dcv_teacher_courses_sql() . ")";
  return db_query($sql, array(':c' => (int) $course_id, ':tid' => (int) $tid))->fetchField() > 0;
}

/**
 * A teacher's sides for a course — array('M'), array('F') or array('M','F'), always in M, F
 * order — from their Confirmed Conducting/Assisting mapping(s) and the centre's "Sees"
 * (ct_sees: M / F / B, empty = the teacher's own gender), by Dipi's own AT course view rule
 * _at_view_sides(). Several mappings → only the sides common to all of them (never more than
 * the website, which reads one). array() = none; FALSE = not a teacher of this course (or no
 * AT portal module).
 */
function dcv_teacher_sides($tid, $course_id) {
  if (!$tid || !function_exists('_at_view_sides')) { return FALSE; }
  $rows = db_query("select ct.ct_sees as sees, t.t_gender as gender
      from dh_course_teacher ct
        join dh_teacher t on t.t_id = ct.ct_teacher and t.t_status = 'Active'
        join dh_course c on c.c_id = ct.ct_course
      where ct.ct_teacher = :tid and ct.ct_course = :c and ct.ct_status = 'Confirmed'
        and ct.ct_type in ('Conducting','Assisting') and c.c_deleted = 0 and c.c_cancelled = 0",
    array(':tid' => (int) $tid, ':c' => (int) $course_id))->fetchAll();
  if (!$rows) { return FALSE; }
  $sides = array('M', 'F');
  foreach ($rows as $r) {
    $sides = array_values(array_intersect($sides, _at_view_sides($r->sees, $r->gender)));
  }
  return $sides;
}

/**
 * What the caller may load of a course: 'full' (a centre user of the course's centre with
 * both gender permissions, as on Dipi's centre pages), their sides (a non-empty array: their
 * centre side and/or their teacher side, see dcv_course_sides()), or FALSE.
 */
function dcv_course_access($course_id) {
  $course_centre = db_query("select c_center from dh_course where c_id = :c",
    array(':c' => (int) $course_id))->fetchField();
  $sides = dcv_course_sides(dcv_centres_allow(dcv_scope_centres(), $course_centre),
    dcv_centre_sides(), dcv_teacher_sides(dcv_teacher_id(), $course_id));
  if ($sides === NULL) { return 'full'; }
  return $sides ? $sides : FALSE;
}

/**
 * The plans and sides a bundle request gets, or the error message to send. Full access:
 * the requested plans (NULL = the whole course). Limited to sides (a teacher, or a centre
 * user with only one gender permission): only plans on their sides — none asked (apps before
 * 1.1.4) = all their plans, never the whole course — and every query also limited to their
 * sides' genders.
 */
function dcv_bundle_scope($course_id, $raw) {
  $access = dcv_course_access($course_id);
  if ($access === FALSE) { return 'Not authorised for this course.'; }
  $plans = dcv_requested_plans($course_id, $raw);
  if ($plans === FALSE) { return 'Unknown plan.'; }
  if ($access === 'full') { return array($plans, NULL); }
  if ($plans === NULL) {
    $plans = array();
    foreach (dcv_course_plans($course_id, $access) as $p) { $plans[] = $p['id']; }
  }
  foreach ($plans as $id) {
    if (!dcv_plan_on_sides($id, $access)) { return 'Unknown plan.'; }
  }
  return array($plans, $access);
}

/**
 * Build the loadable-course list: the running courses of the given centres plus
 * those teacher $teacher_id serves (each listed once).
 * A course lists the plans of the caller's sides there (dcv_course_sides(): all of them for a
 * centre user with both gender permissions; else their centre side and/or teacher side) and
 * is skipped when their sides come to nothing.
 * $centres is an array of ints, or array('*') for all centres; $teacher_id is a
 * dh_teacher id or FALSE; $today defaults to the real date (tests pass one);
 * $centre_sides: the caller's sides as a centre user; NULL (default) = the current user's own
 * dcv_centre_sides(), so a caller that leaves it out never gets more than that user may see.
 * Excludes deleted/cancelled courses. Read-only, parameterized.
 */
function dcv_build_course_list($centres, $teacher_id = FALSE, $today = NULL, $centre_sides = NULL) {
  if ($centre_sides === NULL) { $centre_sides = dcv_centre_sides(); }
  // Who may see what: centre courses and/or the teacher's own courses.
  $scope = array();
  $args = array();
  if ($centres === array('*')) {
    $scope[] = "1=1";
  }
  elseif (!empty($centres)) {
    $scope[] = "c_center in (:centres)";
    $args[':centres'] = $centres;
  }
  if ($teacher_id) {
    $scope[] = "c_id in (" . dcv_teacher_courses_sql() . ")";
    $args[':tid'] = (int) $teacher_id;
  }
  if (empty($scope)) {
    return array();
  }
  // Only currently-running courses: start_date <= today <= end_date. The start
  // date is Day 0, so a course can be loaded from Day 0 to its last day. DCV is a
  // viewer for the course a teacher is running now, not the full history.
  $args[':today'] = $today ? $today : date('Y-m-d');
  $where = "c_deleted=0 and c_cancelled=0 and c_start <= :today and c_end >= :today"
    . " and (" . implode(" or ", $scope) . ")";
  $sql = "select c_id, c_name, c_center, c_start, c_end
          from dh_course
          where $where
          order by c_start desc";
  $out = array();
  foreach (db_query($sql, $args) as $r) {
    // The caller's sides of this course (NULL = all of it); a course whose sides come to
    // nothing is not listed.
    $sides = dcv_course_sides(dcv_centres_allow($centres, (int) $r->c_center), $centre_sides,
      dcv_teacher_sides($teacher_id, (int) $r->c_id));
    if ($sides !== NULL && !$sides) { continue; }
    $out[] = array(
      'id'         => (int) $r->c_id,
      'name'       => $r->c_name,
      'centre'     => (int) $r->c_center,
      'start_date' => $r->c_start,
      'end_date'   => $r->c_end,
      // The plans the app can choose from (spec: choose which plans to load).
      'plans'      => dcv_course_plans((int) $r->c_id, $sides),
    );
  }
  return $out;
}

/** GET dcv/courses — courses the logged-in caller may load (centre and/or teacher). */
function dcv_courses() {
  $list = dcv_build_course_list(dcv_scope_centres(), dcv_teacher_id(), NULL, dcv_centre_sides());
  dcv_json('OK', array('courses' => $list));
}

/**
 * Decide whether a caller's centre scope grants access to a course's centre.
 * Pure (no DB, no globals) — unit tested. $centres is dcv_user_centres()'s
 * return: an int list, or array('*') for the all-centres bypass. $course_centre
 * is the course's c_center, or false/null when the course does not exist.
 */
function dcv_centres_allow($centres, $course_centre) {
  if ($centres === array('*')) { return true; }
  if (empty($centres)) { return false; }
  if ($course_centre === false || $course_centre === null) { return false; }
  return in_array((int) $course_centre, $centres, true);
}

/**
 * True if the current user may load at least one side of this course (dcv_course_access()).
 */
function dcv_user_can_access_course($course_id) {
  return dcv_course_access($course_id) !== FALSE;
}

/**
 * The plans a bundle request asks for: NULL = the whole course (no ?plans=, as 1.1.x
 * apps send); FALSE = refused (empty, malformed, or of the wrong kind for this
 * course's seating plan); else the plan ids.
 */
function dcv_requested_plans($course_id, $raw) {
  $ids = dcv_parse_plans($raw);
  if ($ids === NULL || $ids === FALSE) { return $ids; }
  $kind = function_exists('dh_course_plan') ? dh_course_plan((int) $course_id, 'seat') : 'main';
  return dcv_plans_fit_kind($ids, $kind) ? $ids : FALSE;
}

/** GET dcv/course/%/bundle[?plans=RIGHT,LEFT-2] */
function dcv_course_bundle($course_id) {
  if (!ctype_digit((string) $course_id)) {
    dcv_json('FAIL', array(), 'Invalid course id.');
  }
  $raw = isset($_GET['plans']) ? $_GET['plans'] : NULL;
  $scope = dcv_bundle_scope($course_id, (is_string($raw) || $raw === NULL) ? $raw : '');
  if (is_string($scope)) {
    dcv_json('FAIL', array(), $scope);
  }
  list($plans, $sides) = $scope;
  // Building a bundle fetches every student photo from S3 (up to ~450 in a big
  // course), which can exceed the default 30s max_execution_time — raise it here.
  drupal_set_time_limit(300);
  $bundle = dcv_build_bundle($course_id, $plans, $sides);
  if ($bundle === null) {
    dcv_json('FAIL', array(), 'Course not found.');
  }
  dcv_json('OK', $bundle);
}
