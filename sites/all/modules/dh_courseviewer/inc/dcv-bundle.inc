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

/**
 * Curated, teacher-focused student records for a course, keyed by student id.
 * Read-only, parameterized. Fields chosen for course guidance (spec §6, decision C).
 */
function dcv_course_students($course_id) {
  $sql = "select a_id,
            concat(a_f_name,' ',a_m_name,' ',a_l_name) as name,
            a_gender as gender,
            TIMESTAMPDIFF(YEAR, a_dob, CURDATE()) as age,
            a_old, a_type, a_langs, a_lang_discourse,
            ac_teacher,
            ae_desc_physical, ae_desc_mental, ae_desc_medication,
            ae_desc_special_requirement, ae_pregnant,
            ae_desc_other_technique, ae_desc_other_technique_old,
            aa_comment, a_internal_note, a_extra, a_purpose,
            aa_section, aa_acco, aa_cell,
            aa_backrest, aa_chowky, aa_chair, a_monk, a_alist
          from dh_applicant
            left join dh_applicant_course   on a_id = ac_applicant
            left join dh_applicant_extra    on a_id = ae_applicant
            left join dh_applicant_attended on a_id = aa_applicant
          where a_course = :c and a_attended = 1 and a_type = 'Student'";
  $out = array();
  foreach (db_query($sql, array(':c' => (int) $course_id)) as $r) {
    $acco = trim($r->aa_section . '-' . $r->aa_acco, '-');
    if (!empty($r->aa_cell)) { $acco .= ' / ' . $r->aa_cell; }
    $language = trim($r->a_langs);
    if ($language === '' && !empty($r->a_lang_discourse)) { $language = $r->a_lang_discourse; }
    $meditation = trim(implode(' | ', array_filter(array(
      $r->ae_desc_other_technique_old ? 'Previous: ' . $r->ae_desc_other_technique_old : '',
      $r->ae_desc_other_technique     ? 'Current: '  . $r->ae_desc_other_technique     : '',
    ))));
    $special = trim(implode(' | ', array_filter(array(
      $r->aa_comment, $r->a_purpose, $r->a_extra, $r->a_internal_note,
    ))));
    $flags = array();
    if ($r->aa_backrest || $r->aa_chowky) { $flags[] = 'backrest'; }
    if ($r->aa_chowky) { $flags[] = 'chowky'; }
    if ($r->aa_chair)  { $flags[] = 'chair'; }
    if ($r->ac_teacher) { $flags[] = 'teacher'; }
    if ($r->a_monk)  { $flags[] = 'non-householder'; }
    if ($r->a_alist) { $flags[] = 'a-list'; }
    if (!$r->a_old)  { $flags[] = 'new-student'; }

    $out[(string) $r->a_id] = array(
      'name'     => $r->name,
      'gender'   => $r->gender,
      'age'      => is_null($r->age) ? null : (int) $r->age,
      'language' => $language,
      'old_student' => (bool) $r->a_old,
      'type'     => $r->a_type,
      'health'   => array(
        'physical'            => (string) $r->ae_desc_physical,
        'mental'              => (string) $r->ae_desc_mental,
        'medication'          => (string) $r->ae_desc_medication,
        'special_requirement' => (string) $r->ae_desc_special_requirement,
        'pregnant'            => (bool) $r->ae_pregnant,
      ),
      'meditation_history' => $meditation,
      'special_requests'   => $special,
      'accommodation'      => $acco,
      'flags'              => $flags,
    );
  }
  return $out;
}

/**
 * Occupied seats + per-seat card fields for a course, honouring the course's
 * seat plan (main vs group). Seat labels are Dipi's cached values (aa_seat /
 * aa_group_seat) — not recomputed. Read-only, parameterized.
 */
function dcv_course_seats($course_id) {
  $course_id = (int) $course_id;
  // dh_course_plan() lives in dh_manageapp/inc/allocate.inc (dependency).
  $plan = function_exists('dh_course_plan') ? dh_course_plan($course_id, 'seat') : 'main';
  if ($plan === 'group') {
    $col_c = 'aa_group_seat_col'; $col_r = 'aa_group_seat_row'; $col_l = 'aa_group_seat';
  }
  else {
    $col_c = 'aa_seat_col'; $col_r = 'aa_seat_row'; $col_l = 'aa_seat';
  }
  // Column names are chosen from a fixed whitelist above — safe to interpolate.
  $sql = "select a_id, a_gender, a_old,
            concat(a_f_name,' ',a_l_name) as name,
            TIMESTAMPDIFF(YEAR, a_dob, CURDATE()) as age,
            ac_teen, ac_10d, ac_stp, ac_20d, ac_30d, ac_45d, ac_60d, ac_tsc, ac_spl,
            aa_section, aa_acco, aa_cell, aa_group,
            aa_chowky, aa_chair, aa_backrest,
            $col_r as seat_row, $col_c as seat_col, $col_l as seat_label
          from dh_applicant
            left join dh_applicant_course   on a_id = ac_applicant
            left join dh_applicant_attended on a_id = aa_applicant
          where a_course = :c and a_attended = 1 and a_type = 'Student'
            and $col_r is not null and $col_c is not null";
  $out = array();
  foreach (db_query($sql, array(':c' => $course_id)) as $r) {
    $acco = trim($r->aa_section . '-' . $r->aa_acco, '-');
    if (!empty($r->aa_cell)) { $acco .= ' / ' . $r->aa_cell; }
    $short = (int) $r->ac_teen + (int) $r->ac_10d + (int) $r->ac_stp;
    $long  = (int) $r->ac_20d + (int) $r->ac_30d + (int) $r->ac_45d
           + (int) $r->ac_60d + (int) $r->ac_tsc + (int) $r->ac_spl;
    $out[] = array(
      'section'    => ($r->a_gender === 'M') ? 'RIGHT' : 'LEFT',
      'group'      => (int) $r->aa_group,
      'row'        => (int) $r->seat_row,
      'col'        => (int) $r->seat_col,
      'label'      => (string) $r->seat_label,
      'chowky'     => (bool) $r->aa_chowky,
      'chair'      => (bool) $r->aa_chair,
      'student_id' => (int) $r->a_id,
      'name'       => $r->name,
      'acco'       => $acco,
      'age'        => is_null($r->age) ? null : (int) $r->age,
      'old_student'   => (bool) $r->a_old,
      'short_courses' => $short,
      'long_courses'  => $long,
      'backrest'      => (bool) ($r->aa_backrest || $r->aa_chowky),
    );
  }
  return $out;
}

/** Course header for the bundle, or null if the course does not exist. */
function dcv_course_header($course_id) {
  $r = db_query("select c.c_id, c.c_name, c.c_center, c.c_start, c.c_end,
                        c.c_seat_plan, td.td_val1 as course_type,
                        ce.c_name as centre_name
                 from dh_course c
                   left join dh_type_detail td on c.c_course_type = td.td_id
                   left join dh_center ce on c.c_center = ce.c_id
                 where c.c_id = :c",
    array(':c' => (int) $course_id))->fetchObject();
  if (!$r) { return null; }
  $plan = function_exists('dh_course_plan') ? dh_course_plan((int) $course_id, 'seat') : 'main';
  return array(
    'id'          => (int) $r->c_id,
    'name'        => $r->c_name,
    'centre'      => (int) $r->c_center,
    'centre_name' => (string) $r->centre_name,
    'start_date'  => $r->c_start,
    'end_date'    => $r->c_end,
    'course_type' => (string) $r->course_type,
    'seat_plan'   => $plan,
  );
}

/** Assemble the full course bundle (spec §5), or null if course missing. */
function dcv_build_bundle($course_id) {
  $header = dcv_course_header($course_id);
  if ($header === null) { return null; }
  $centre = $header['centre'];
  $seat_cfg = db_query("select cs_seat_config from dh_center_setting where cs_center = :c",
    array(':c' => $centre))->fetchField();
  $sections = $seat_cfg ? dcv_parse_seat_config($seat_cfg) : array();
  $expires = date('Y-m-d', strtotime($header['end_date'] . ' +1 day'));
  return array(
    'schema_version' => 1,
    'course'         => $header,
    'sections'       => $sections,
    'seats'          => dcv_course_seats($course_id),
    'students'       => dcv_course_students($course_id),
    'generated_at'   => date('c'),
    'expires_after'  => $expires,
  );
}
