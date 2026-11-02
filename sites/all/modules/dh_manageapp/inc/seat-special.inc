<?php
/**
 * Seating plan tools: the shared re-seat step for a grid or group change (used
 * by the Zero-Day popup and the seating plans), and the backrest / grid save
 * from the plans.
 */

/**
 * Every course that shares one seating plan with $course_id: the course, the
 * course it combines its seating with, and any course combined with it.
 * Returns sorted int ids.
 */
function dh_seat_plan_courses($course_id)
{
	$course_id = (int) $course_id;
	$ids = db_query("select c_id from dh_course where c_id = :c1 or c_combined_seat_course = :c2
		or c_id = (select c_combined_seat_course from dh_course where c_id = :c3)",
		array(':c1' => $course_id, ':c2' => $course_id, ':c3' => $course_id))->fetchCol();
	$ids = array_values(array_unique(array_map('intval', $ids)));
	sort($ids);
	return $ids;
}

/**
 * Seat fields for a student who moves to the other grid (main <-> chowky/chair)
 * or to another group: the first seat of a new row at the top (back) of the new
 * grid, the place the Zero-Day popup has always used. Every course that shares
 * the plan is counted, so the seat is never one already used by a student of a
 * combined course; an empty grid starts at the front row (row 0).
 *
 * $app_id      the student (their own seat is never counted)
 * $course_id   the student's course (a_course)
 * $gender      'M' or 'F'
 * $now_chowky  TRUE when the student is now in the chowky/chair grid
 * $main        TRUE: a new seat in the seating plan (aa_seat_row / aa_seat_col)
 * $group       NULL: leave the group-wise plan alone; else the student's (new)
 *              group: a new seat in that group's grid (aa_group_seat_row / col)
 * Returns the fields to save (empty when nothing is asked).
 */
function dh_seat_reseat_fields($app_id, $course_id, $gender, $now_chowky, $main, $group)
{
	$fields = array();
	$args = array(':c' => dh_seat_plan_courses($course_id), ':g' => $gender, ':self' => (int) $app_id);
	$grid = $now_chowky ? " and (aa.aa_chowky = 1 or aa.aa_chair = 1)" : " and aa.aa_chowky = 0 and aa.aa_chair = 0";
	$from = " from dh_applicant_attended aa join dh_applicant a on a.a_id = aa.aa_applicant
		where a.a_course in (:c) and a.a_gender = :g and a.a_type = 'Student' and a.a_attended = 1
		and a.a_id <> :self".$grid;
	if ($main)
	{
		$max = db_query("select max(aa.aa_seat_row)".$from, $args)->fetchField();
		$fields['aa_seat_row'] = ($max === NULL || $max === FALSE) ? 0 : (int) $max + 1;
		$fields['aa_seat_col'] = 0;
	}
	if ($group !== NULL)
	{
		$max = db_query("select max(aa.aa_group_seat_row)".$from." and aa.aa_group = :grp", $args + array(':grp' => $group))->fetchField();
		$fields['aa_group_seat_row'] = ($max === NULL || $max === FALSE) ? 0 : (int) $max + 1;
		$fields['aa_group_seat_col'] = 0;
	}
	return $fields;
}
