<?php
/**
 * @file
 * Dipi App plans: the seating plans of a course that the app can load one or more
 * of — RIGHT / LEFT (main seating plan) or RIGHT-<group> / LEFT-<group> (group-wise).
 * A student or Dhamma server (a_type 'Sevak') belongs to a plan by gender (M → RIGHT, anything else → LEFT, as
 * dcv_course_seats() places seats) and, in a group-wise course, by aa_group.
 * A plan not chosen is not sent at all: the app's tablet never holds its students.
 */

/**
 * The ?plans= value: NULL when absent (the whole course, as 1.1.x apps ask), FALSE
 * when present but empty or malformed, else the de-duplicated ids. Pure.
 */
function dcv_parse_plans($raw) {
  if ($raw === NULL) { return NULL; }
  $ids = array();
  foreach (explode(',', (string) $raw) as $part) {
    $id = trim($part);
    if ($id === '') { continue; }
    if (!preg_match('/^(RIGHT|LEFT)(-[0-9]{1,3})?$/', $id)) { return FALSE; }
    $ids[$id] = TRUE;
  }
  return $ids ? array_keys($ids) : FALSE;
}

/** True if every id fits the course's seating-plan kind ('main' | 'group'). Pure. */
function dcv_plans_fit_kind(array $ids, $kind) {
  foreach ($ids as $id) {
    if ((strpos($id, '-') !== FALSE) !== ($kind === 'group')) { return FALSE; }
  }
  return TRUE;
}

/** A student's plan id from gender and group. Pure. */
function dcv_plan_id($gender, $group, $kind) {
  $key = (strtoupper((string) $gender) === 'M') ? 'RIGHT' : 'LEFT';
  return ($kind === 'group') ? $key . '-' . (int) $group : $key;
}

/**
 * SQL limiting rows to a teacher's sides (their "Sees": array('M'), array('F') or both) by
 * exact gender, appended to a WHERE; NULL = no limit (centre users). Applicants whose gender
 * is neither M nor F match no side, as on Dipi's AT course view. Pure.
 */
function dcv_sides_sql($sides) {
  if ($sides === NULL) { return array('', array()); }
  if (!$sides) { return array(' and (1=0)', array()); }
  return array(' and a_gender in (:dcv_sides)', array(':dcv_sides' => array_values($sides)));
}

/** True if plan $id is on one of $sides (RIGHT = M, LEFT = F). Pure. */
function dcv_plan_on_sides($id, array $sides) {
  $key = strtok($id, '-');
  return in_array($key === 'RIGHT' ? 'M' : 'F', $sides, TRUE);
}

/**
 * The SQL condition (and its arguments) selecting the students of the plans $ids,
 * over a_gender and aa_group — the student and seat queries both join
 * dh_applicant_attended. Group numbers are bound as parameters. Pure.
 */
function dcv_plans_sql(array $ids, $kind) {
  if (!$ids) {
    // No caller passes an empty $ids today (dcv_parse_plans() refuses an empty
    // ?plans=), but an empty OR-list would otherwise build the invalid SQL "()" —
    // guard it to a condition that matches nothing instead.
    return array('(1=0)', array());
  }
  $parts = array();
  $args = array();
  foreach (array_values($ids) as $i => $id) {
    $key = strtok($id, '-');
    $gender = ($key === 'RIGHT') ? "(a_gender = 'M')" : "(a_gender is null or a_gender <> 'M')";
    if ($kind === 'group') {
      $parts[] = "($gender and coalesce(aa_group, 0) = :pg$i)";
      $args[":pg$i"] = (int) substr($id, strlen($key) + 1);
    }
    else {
      $parts[] = $gender;
    }
  }
  return array('(' . implode(' or ', $parts) . ')', $args);
}

/**
 * The course's plans with their student and server counts, in the order the app's plan
 * switcher shows them (main: RIGHT then LEFT; group-wise: by group, then LEFT before RIGHT);
 * only plans with at least one attended student or server. $sides (a teacher's) limits rows
 * by exact gender, so only that side's plans and people are counted.
 */
function dcv_course_plans($course_id, $sides = NULL) {
  $course_id = (int) $course_id;
  $kind = function_exists('dh_course_plan') ? dh_course_plan($course_id, 'seat') : 'main';
  $labels = array('RIGHT' => 'Male', 'LEFT' => 'Female');
  list($fence, $fargs) = dcv_sides_sql($sides);
  $counts = array();
  $sql = "select (case when a_gender = 'M' then 'RIGHT' else 'LEFT' end) as sec,
                 coalesce(aa_group, 0) as grp, a_type as typ, count(*) as n
          from dh_applicant left join dh_applicant_attended on a_id = aa_applicant
          where a_course = :c and a_attended = 1 and a_type in ('Student', 'Sevak')$fence
          group by sec, grp, typ";
  foreach (db_query($sql, array(':c' => $course_id) + $fargs) as $r) {
    $id = ($kind === 'group') ? $r->sec . '-' . (int) $r->grp : $r->sec;
    if (!isset($counts[$id])) { $counts[$id] = array('students' => 0, 'servers' => 0); }
    $counts[$id][strtolower((string) $r->typ) === 'sevak' ? 'servers' : 'students'] += (int) $r->n;
  }
  $ids = array_keys($counts);
  usort($ids, function ($a, $b) use ($kind) {
    $ka = strtok($a, '-'); $kb = strtok($b, '-');
    if ($kind === 'group') {
      $ga = (int) substr($a, strlen($ka) + 1); $gb = (int) substr($b, strlen($kb) + 1);
      return ($ga - $gb) ?: strcmp($ka, $kb);
    }
    return ($ka === 'RIGHT' ? 0 : 1) - ($kb === 'RIGHT' ? 0 : 1);
  });
  $out = array();
  foreach ($ids as $id) {
    $key = strtok($id, '-');
    $label = ($kind === 'group') ? 'Group ' . substr($id, strlen($key) + 1) . ' · ' . $labels[$key] : $labels[$key];
    $out[] = array('id' => $id, 'label' => $label,
      'students' => $counts[$id]['students'], 'servers' => $counts[$id]['servers']);
  }
  return $out;
}

/**
 * The bundle's sections cut to the chosen plans: only their genders, and in a
 * group-wise course each section's groups only the chosen ones. Pure.
 */
function dcv_filter_sections(array $sections, array $ids, $kind) {
  $keep = array();
  foreach ($ids as $id) {
    $key = strtok($id, '-');
    if ($kind === 'group') { $keep[$key][] = (int) substr($id, strlen($key) + 1); }
    else { $keep[$key] = TRUE; }
  }
  $out = array();
  foreach ($sections as $s) {
    if (!isset($keep[$s['key']])) { continue; }
    if ($kind === 'group') {
      $want = $keep[$s['key']];
      $s['groups'] = array_values(array_filter($s['groups'], function ($g) use ($want) {
        return in_array((int) $g['n'], $want, TRUE);
      }));
    }
    $out[] = $s;
  }
  return $out;
}
