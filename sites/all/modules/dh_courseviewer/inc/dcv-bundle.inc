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
