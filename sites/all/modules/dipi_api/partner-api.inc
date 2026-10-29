<?php
/**
 * @file
 * Read-only sync APIs for partner portals (first user: the Pala team), so a partner can keep
 * its own copy of DIPI's upcoming courses and of the AT-Book, updated every few minutes.
 *
 * Endpoint 'api' (resource dipi, Services actions, JSON body), after /api/user/login.json:
 *   upcoming-courses-updated-last-n-minutes/<n>   permission 'api upcoming courses'
 *   at-book-updated-last-n-minutes/<n>            permission 'api at book'
 *   at-book-photo/<teacher id>                    permission 'api at book'
 * <n> = 0 returns everything (first load, or a periodic full resync).
 *
 * The AT-Book data is what a logged-in AT sees on the AT Address Book (dh_atportal
 * address-book.inc): every teacher except those in Training, with the same fields, plus each
 * teacher's CT / ACT centres and CAT / ACAT areas. A teacher who moved into Training comes
 * back as {ID, InAtBook: false} only, so the partner can remove the row.
 */

/** Largest <n> accepted (10 years in minutes); 0 means "everything". */
define('DIPI_PARTNER_API_MAX_MINUTES', 5256000);

/**
 * Checks <n> (digits only, 0..max). Returns the integer, or FALSE.
 */
function _api_partner_minutes($n) {
  if (!ctype_digit((string) $n)) {
    return FALSE;
  }
  $n = (int) $n;
  return ($n <= DIPI_PARTNER_API_MAX_MINUTES) ? $n : FALSE;
}

/** One line per call in the log, so usage can be seen (who, what, how many rows). */
function _api_partner_log($call, $n, $count) {
  global $user;
  watchdog('PartnerAPI', '@name (uid @uid) called @call (minutes @n): @count rows',
    array('@name' => isset($user->name) ? $user->name : '', '@uid' => isset($user->uid) ? $user->uid : 0,
      '@call' => $call, '@n' => $n, '@count' => $count), WATCHDOG_INFO);
}

/**
 * The Apply link of a course on the schedule site: the same form as the schedule site's Apply
 * button (CourseApplyLink.php on vri-3). Empty for centres that do not use VRI forms (they
 * apply on dhamma.org; DIPI does not hold that link). The schedule site shows the button only
 * while enrolment is open (see the guide); this returns the link itself.
 */
function _api_partner_apply_link($vri, $type_name, $vri_form, $centre, $course) {
  if ((string) $vri !== '1') {
    return '';
  }
  $prefix = '';
  if (in_array($type_name, array('10 Day Special Course', '20 Day Course', '30 Day Course', '45 Day Course', '60 Day Course', 'Gratitude Course'))) {
    $prefix = 'long-course-';
  }
  elseif ($type_name == '1 Day Course') {
    $prefix = 'one-day-';
  }
  elseif ($type_name == 'Teenager Course') {
    $prefix = 'teenager-';
  }
  elseif ($type_name == 'Children Course') {
    $prefix = 'children-';
  }
  elseif ($type_name == 'STP Course') {
    $prefix = 'stp-';
  }
  if ((string) $vri_form !== '') {
    $prefix = $vri_form;   // the centre's own form for this course type
  }
  $base = rtrim(variable_get('dipi_schedule_base_url', 'https://schedule.vridhamma.org'), '/');
  return $base . '/form/' . $prefix . 'application-form?centre=' . $centre . '&course=' . $course;
}

/**
 * upcoming-courses-updated-last-n-minutes/<n>: upcoming courses (start today or later) of all
 * centres that changed in the last <n> minutes; <n> = 0 returns all upcoming courses.
 * A course deleted in DIPI comes back with Deleted = true (only in the minutes form), so the
 * partner can remove it.
 */
function _api_partner_upcoming_courses($n) {
  $n = _api_partner_minutes($n);
  if ($n === FALSE) {
    return array('Error' => 'Invalid minutes (use 0 for all, or 1 to ' . DIPI_PARTNER_API_MAX_MINUTES . ')');
  }
  $args = array();
  $window = ' and co.c_deleted = 0 ';
  if ($n > 0) {
    $window = ' and co.c_updated >= NOW() - INTERVAL :n MINUTE ';
    $args[':n'] = $n;
  }
  $q = "select co.c_id, co.c_name, co.c_center, ce.c_name as centre_name, ce.c_subdomain, ce.c_type as centre_type, ce.c_vri,
      ce.c_city as centre_city_id, ci.c_name as centre_city, ce.c_state as centre_state_code, st.s_name as centre_state,
      ce.c_country as centre_country_code, cn.c_name as centre_country,
      td.td_key as type_key, td.td_val1 as type_name, td.td_val3 as type_group,
      co.c_start, co.c_end, co.c_enrol_date, co.c_description, co.c_comments, co.c_status,
      co.c_status_om, co.c_status_of, co.c_status_nm, co.c_status_nf, co.c_status_svr_m, co.c_status_svr_f,
      co.c_list_only, co.c_cancelled, co.c_date_change, co.c_deleted, co.c_form_langs, co.c_updated,
      (select max(cct.cct_vri_form) from dh_center_course_template cct where cct.cct_center = co.c_center and cct.cct_course_type = td.td_key) as vri_form
    from dh_course co
    join dh_center ce on ce.c_id = co.c_center
    left join dh_type_detail td on td.td_id = co.c_course_type
    left join dh_city ci on ci.c_id = ce.c_city
    left join dh_state st on (st.s_country = ce.c_country and st.s_code = ce.c_state)
    left join dh_country cn on cn.c_code = ce.c_country
    where co.c_center <> 0 and co.c_start >= CURDATE() $window
    order by co.c_start, co.c_id";
  $courses = array();
  foreach (db_query($q, $args) as $r) {
    $langs = array_values(array_filter(array_map('trim', explode(',', (string) $r->c_form_langs))));
    $courses[] = array(
      'CourseID' => (int) $r->c_id,
      'CourseName' => $r->c_name,
      'CourseType' => $r->type_name,
      'CourseTypeKey' => $r->type_key,
      'LongCourse' => (strtoupper(trim((string) $r->type_group)) === 'LC'),
      'Title' => (string) $r->c_description,
      'Comments' => (string) $r->c_comments,
      'StartDate' => $r->c_start,
      'EndDate' => $r->c_end,
      'EnrollmentDate' => $r->c_enrol_date,
      'CourseStatus' => $r->c_status,
      'StatusOldMale' => $r->c_status_om,
      'StatusOldFemale' => $r->c_status_of,
      'StatusNewMale' => $r->c_status_nm,
      'StatusNewFemale' => $r->c_status_nf,
      'StatusServerMale' => $r->c_status_svr_m,
      'StatusServerFemale' => $r->c_status_svr_f,
      'ListOnly' => (bool) $r->c_list_only,
      'Cancelled' => (bool) $r->c_cancelled,
      'DateChanged' => (bool) $r->c_date_change,
      'Deleted' => (bool) $r->c_deleted,
      'FormLanguages' => $langs,
      'VriForm' => ((string) $r->c_vri === '1'),
      'ApplyLink' => _api_partner_apply_link($r->c_vri, $r->type_name, $r->vri_form, $r->c_center, $r->c_id),
      'CentreID' => (int) $r->c_center,
      'CentreName' => $r->centre_name,
      'CentreSubdomain' => (string) $r->c_subdomain,
      'CentreType' => (string) $r->centre_type,
      'CentreCity' => (string) $r->centre_city,
      'CentreStateCode' => (string) $r->centre_state_code,
      'CentreState' => (string) $r->centre_state,
      'CentreCountryCode' => strtoupper((string) $r->centre_country_code),
      'CentreCountry' => (string) $r->centre_country,
      'Updated' => $r->c_updated,
    );
  }
  _api_partner_log('upcoming-courses-updated-last-n-minutes', $n, count($courses));
  return array(
    'ServerTime' => db_query("select NOW()")->fetchField(),
    'Minutes' => $n,
    'Count' => count($courses),
    'Courses' => $courses,
  );
}

/** A phone number the way the AT-Book shows it: "+<country code> <number>". */
function _api_partner_phone($cc, $ph) {
  $ph = trim((string) $ph);
  if ($ph === '') {
    return '';
  }
  if ($ph[0] === '+') {
    return $ph;   // already the full international number
  }
  $cc = trim((string) $cc);
  return (($cc !== '' && $cc !== '0') ? '+' . $cc . ' ' : '') . $ph;
}

/**
 * at-book-updated-last-n-minutes/<n>: AT-Book teachers whose record, or one of their
 * CT / ACT / CAT / ACAT assignments, changed in the last <n> minutes (<n> = 0: all teachers).
 * Each teacher carries the full current list of assignments (Roles), so the partner replaces
 * the teacher's row and roles as a whole.
 */
function _api_partner_at_book($n) {
  $n = _api_partner_minutes($n);
  if ($n === FALSE) {
    return array('Error' => 'Invalid minutes (use 0 for all, or 1 to ' . DIPI_PARTNER_API_MAX_MINUTES . ')');
  }
  $args = array();
  $window = '';
  if ($n > 0) {
    // the teacher's record, an assignment (added, removed), or the name of one of their areas
    $window = " and (t.t_updated >= NOW() - INTERVAL :n1 MINUTE
      or t.t_id in (select tr_teacher from dh_teacher_role where tr_updated >= NOW() - INTERVAL :n2 MINUTE or tr_created >= NOW() - INTERVAL :n3 MINUTE)
      or t.t_id in (select tr.tr_teacher from dh_teacher_role tr join dh_teacher_area_cluster tac on tac.tac_id = tr.tr_identifier
          where tr.tr_role in ('CAT', 'ACAT') and tr.tr_deleted = 0 and tac.tac_updated >= NOW() - INTERVAL :n4 MINUTE)) ";
    $args = array(':n1' => $n, ':n2' => $n, ':n3' => $n, ':n4' => $n);
  }
  else {
    $window = " and t.t_status <> 'Training' ";   // a full load needs only the AT-Book teachers
  }
  // Same columns as the AT Address Book (address-book.inc); its non-admin filter is t_status <> 'Training'.
  $q = "select t.t_id, t.t_code, t.t_gender, t.t_f_name, t.t_l_name, t.t_city, s.s_name, t.t_country, cn.c_name as country_name,
      t.t_mob_country, t.t_mob_phone, t.t_mob_country_alt, t.t_mob_phone_alt, t.t_email, t.t_email_alt,
      TIMESTAMPDIFF(YEAR, t.t_dob, CURDATE()) as age, t.t_responsibility, t.t_area,
      t.t_at_appointed, t.t_sat_appointed, t.t_full_t_appointed, t.t_bt_appointed, t.t_status,
      t.t_sat, t.t_ct, t.t_full_t, t.t_cat, t.t_bhante, (coalesce(t.t_photo, '') <> '') as has_photo, t.t_updated,
      (t.t_status <> 'Training') as in_at_book
    from dh_teacher t
    left join dh_state s on (t.t_state = s.s_code and t.t_country = s.s_country)
    left join dh_country cn on cn.c_code = t.t_country
    where 1 $window
    order by t.t_id";
  $rows = db_query($q, $args)->fetchAll();

  // Current assignments of these teachers (CT / ACT: teacher centre; CAT / ACAT: area cluster).
  $roles = array();
  $ids = array();
  foreach ($rows as $r) {
    if ($r->in_at_book) {
      $ids[] = (int) $r->t_id;
    }
  }
  foreach (array_chunk($ids, 500) as $chunk) {
    $rq = "select tr.tr_teacher, tr.tr_role, tr.tr_identifier, tc.tc_name, tc.tc_city, ts.s_name as tc_state, tc.tc_country, tcn.c_name as tc_country_name, tac.tac_name
      from dh_teacher_role tr
      left join dh_teacher_center tc on (tr.tr_role in ('CT', 'ACT') and tc.tc_id = tr.tr_identifier)
      left join dh_state ts on (tc.tc_country = ts.s_country and tc.tc_state = ts.s_code)
      left join dh_country tcn on tcn.c_code = tc.tc_country
      left join dh_teacher_area_cluster tac on (tr.tr_role in ('CAT', 'ACAT') and tac.tac_id = tr.tr_identifier)
      where tr.tr_deleted = 0 and tr.tr_teacher in (:ids)
      order by tr.tr_teacher, field(tr.tr_role, 'CT', 'ACT', 'CAT', 'ACAT'), coalesce(tc.tc_name, tac.tac_name)";
    foreach (db_query($rq, array(':ids' => $chunk)) as $x) {
      if (in_array($x->tr_role, array('CT', 'ACT'))) {
        $role = array('Role' => $x->tr_role, 'ID' => (int) $x->tr_identifier, 'Name' => (string) $x->tc_name,
          'City' => (string) $x->tc_city, 'State' => (string) $x->tc_state, 'CountryCode' => strtoupper((string) $x->tc_country),
          'Country' => (string) $x->tc_country_name);
      }
      else {
        $role = array('Role' => $x->tr_role, 'ID' => (int) $x->tr_identifier, 'Name' => (string) $x->tac_name);
      }
      $roles[(int) $x->tr_teacher][] = $role;
    }
  }

  $teachers = array();
  foreach ($rows as $r) {
    if (!$r->in_at_book) {
      // moved into Training (not shown on the AT-Book): just the id, so the partner removes it
      $teachers[] = array('ID' => (int) $r->t_id, 'InAtBook' => FALSE, 'Updated' => $r->t_updated);
      continue;
    }
    $teachers[] = array(
      'ID' => (int) $r->t_id,
      'InAtBook' => TRUE,
      'Code' => (string) $r->t_code,
      'Gender' => (string) $r->t_gender,
      'FirstName' => (string) $r->t_f_name,
      'LastName' => (string) $r->t_l_name,
      'Status' => (string) $r->t_status,
      'SAT' => (bool) $r->t_sat,
      'FullT' => (bool) $r->t_full_t,
      'CT' => (bool) $r->t_ct,
      'CAT' => (bool) $r->t_cat,
      'Bhante' => (bool) $r->t_bhante,
      'City' => (string) $r->t_city,
      'State' => (string) $r->s_name,
      'CountryCode' => strtoupper((string) $r->t_country),
      'Country' => (string) $r->country_name,
      'Mobile1' => _api_partner_phone($r->t_mob_country, $r->t_mob_phone),
      'Mobile2' => _api_partner_phone($r->t_mob_country_alt, $r->t_mob_phone_alt),
      'Email1' => (string) $r->t_email,
      'Email2' => (string) $r->t_email_alt,
      'Age' => ($r->age === NULL) ? NULL : (int) $r->age,
      'ATAppointed' => (string) $r->t_at_appointed,
      'SATAppointed' => (string) $r->t_sat_appointed,
      'FullTAppointed' => (string) $r->t_full_t_appointed,
      'BTAppointed' => (string) $r->t_bt_appointed,
      // the AT-Book shows "Area of Responsibility" for CATs only
      'AreaOfResponsibility' => $r->t_cat ? (string) $r->t_area : '',
      'OtherResponsibility' => (string) $r->t_responsibility,
      'Roles' => isset($roles[(int) $r->t_id]) ? $roles[(int) $r->t_id] : array(),
      'HasPhoto' => (bool) $r->has_photo,
      // changes whenever the teacher's record changes; fetch the photo again when it differs
      'PhotoVersion' => $r->has_photo ? substr(md5((string) $r->t_updated), 0, 8) : '',
      'Updated' => $r->t_updated,
    );
  }
  _api_partner_log('at-book-updated-last-n-minutes', $n, count($teachers));
  return array(
    'ServerTime' => db_query("select NOW()")->fetchField(),
    'Minutes' => $n,
    'Count' => count($teachers),
    'Teachers' => $teachers,
  );
}

/**
 * at-book-photo/<teacher id>: one AT-Book teacher's photo, base64 encoded. Only for teachers
 * on the AT-Book (not Training) who have a photo.
 */
function _api_partner_at_book_photo($id) {
  if (!ctype_digit((string) $id)) {
    return array('Error' => 'Invalid teacher id');
  }
  $t = db_query("select t_id, t_photo, t_status, t_updated from dh_teacher where t_id = :t", array(':t' => (int) $id))->fetchObject();
  if (!$t || $t->t_status == 'Training' || (string) $t->t_photo === '') {
    return array('Error' => 'No photo for this teacher');
  }
  $uri = $t->t_photo;
  $path = drupal_realpath($uri);
  if ($path && is_file($path)) {
    $data = file_get_contents($path);
  }
  else {
    $ret = s3_get_file(variable_get('s3_bucket_name', ''), str_replace('private://', '', $uri), 'stream');
    if (empty($ret['success'])) {
      return array('Error' => 'Photo could not be read');
    }
    $data = $ret['data'];
  }
  $ext = strtolower(pathinfo($uri, PATHINFO_EXTENSION));
  _api_partner_log('at-book-photo', (int) $id, 1);
  return array(
    'ID' => (int) $t->t_id,
    'ContentType' => ($ext == 'png') ? 'image/png' : 'image/jpeg',
    'PhotoVersion' => substr(md5((string) $t->t_updated), 0, 8),
    'Data' => base64_encode($data),
  );
}
