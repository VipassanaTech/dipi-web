<?php
/**
 * Dining "Review & Edit" workbench — grouped editable grid.
 *
 * Render-only page (Task 2 of the unified-allocation-engine P2 work). The
 * toolbar buttons (Auto-fill Main / Auto-fill Group-wise / Clear non-fixed /
 * Save all) are inert until Tasks 3-4 wire up the client-side JS + save
 * endpoint. See docs/superpowers/sdd/2026-09-13-unified-allocation-engine-p2/.
 */

/**
 * Build (and normally print) the dining review-and-edit workbench for one
 * course. Mirrors dh_dining_list()'s $return/print-and-exit pattern in
 * inc/zero-day.inc so the page fn is unit-testable.
 */
function dh_dining_workbench($centre, $course, $return = FALSE)
{
	global $base_url;
	$course = (int) $course;

	$cfg = db_query("select cs_dining_config from dh_center_setting where cs_center=:c", array(':c' => $centre))->fetchField();
	$ini = ($cfg !== '' && $cfg !== NULL) ? parse_ini_string($cfg, TRUE, INI_SCANNER_RAW) : array();
	if (!is_array($ini)) $ini = array();
	$warnings = dh_alloc_validate($ini);
	$has_server = dh_alloc_has_server_config($ini);

	$course_name = db_query("select c_name from dh_course where c_id=$course")->fetchField();

	$type_filter = $has_server ? "and a_type in ('Student','Sevak')" : "and a_type='Student'";
	$q = "select aa_id, a_gender, a_old, aa_group, a_type, aa_dining, aa_group_dining, aa_dining_fixed,
			aa_section, aa_acco, concat(a_f_name, ' ', a_m_name, ' ', a_l_name) as name
		from dh_applicant
		left join dh_applicant_course on a_id=ac_applicant
		left join dh_applicant_attended on a_id=aa_applicant
		left join dh_teacher on (ac_teacher_code = CONCAT(t_code, '-', t_gender))
		where a_course='$course' and a_attended=1 $type_filter
		order by a_gender, IFNULL(ac_teacher,0) desc, IFNULL(t_seniority,0) desc, IFNULL(t_year_appointed,".date('Y')."),
			IFNULL(ac_60d,0) desc, IFNULL(ac_45d,0) desc, IFNULL(ac_30d,0) desc, IFNULL(ac_20d,0) desc,
			IFNULL(ac_tsc,0) desc, IFNULL(ac_spl,0) desc, IFNULL(ac_stp,0) desc, IFNULL(ac_10d,0) desc,
			IFNULL(ac_teen,0) desc, TIMESTAMPDIFF(YEAR, a_dob, CURDATE()) desc, a_id";
	$result = db_query($q);

	// data[class][gender][old-flag][group] = ordered list of attendee rows.
	$data = array();
	$classes = array('Student' => TRUE);
	if ($has_server) $classes['Sevak'] = TRUE;
	while ($r = $result->fetchAssoc())
	{
		$class = (strtolower($r['a_type']) === 'sevak') ? 'Sevak' : 'Student';
		if (!isset($classes[$class])) continue;
		$g = strtoupper($r['a_gender']);
		$old = $r['a_old'] ? 1 : 0;
		$grp = (int) $r['aa_group'];
		$room = trim($r['aa_section'].'-'.$r['aa_acco'], '-');
		$data[$class][$g][$old][$grp][] = array(
			'aa_id' => $r['aa_id'],
			'name' => $r['name'],
			'room' => $room,
			'main' => $r['aa_dining'],
			'group_val' => $r['aa_group_dining'],
			'fixed' => $r['aa_dining_fixed'],
		);
	}

	$title = dh_course_file_basename($centre, $course, 'dining-workbench');
	$out = '<html><head><title>'.$title.'</title>';
	$out .= '<style>@import url("'.$base_url.'/sites/all/modules/dh_manageapp/css/workbench.css");</style>';
	$out .= '</head><body>';

	$out .= '<div class="wb-header no-print"><div align="center" class="title"><b>Dining Review &amp; Edit</b></div>'.
			'<div align="center" class="title-head">'.check_plain($course_name).'</div></div>';

	$out .= '<div class="wb-bar no-print">'.
		'<a href="'.$base_url.'/course/'.$centre.'/'.$course.'">&#8592; Back to Course page</a>'.
		' &nbsp;&nbsp;&bull;&nbsp;&nbsp; '.
		'<button type="button" id="wb-fill-main">Auto-fill Main</button> '.
		'<button type="button" id="wb-fill-group">Auto-fill Group-wise</button> '.
		'<button type="button" id="wb-clear">Clear non-fixed</button> '.
		'<button type="button" id="wb-save">Save all</button>'.
		' &nbsp;&nbsp;&bull;&nbsp;&nbsp; <a href="#" onclick="window.print();return false;">Print</a>'.
		'</div>';

	$out .= '<div class="wb-warn no-print"'.(empty($warnings) ? ' style="display:none"' : '').'>';
	if (!empty($warnings))
	{
		$out .= '<div class="wb-warn-title">Configuration warnings:</div><ul>';
		foreach ($warnings as $w) $out .= '<li>'.check_plain($w).'</li>';
		$out .= '</ul>';
	}
	$out .= '</div>';

	$out .= '<div class="wb-main-div">';
	$gender_order = array('M', 'F');
	foreach (array_keys($classes) as $class)
	{
		if (empty($data[$class])) continue;
		$out .= '<div class="wb-class"><h2 class="wb-class-title">'.check_plain($class === 'Sevak' ? 'Servers' : 'Students').'</h2>';
		foreach ($gender_order as $g)
		{
			if (empty($data[$class][$g])) continue;
			for ($old = 1; $old >= 0; $old--)
			{
				if (empty($data[$class][$g][$old])) continue;
				$bucket_label = ($old ? 'Old' : 'New').' '.($g === 'M' ? 'Male' : 'Female');
				$out .= '<div class="wb-bucket"><div class="wb-bucket-title">'.$bucket_label.'</div>';
				foreach ($data[$class][$g][$old] as $grp => $rows)
				{
					$group_label = $grp > 0 ? 'Group '.$grp : 'No Group';
					$out .= '<div class="wb-group-block"><div class="wb-group-title">'.check_plain($group_label).'</div>';
					$out .= '<table class="wb-grid"><thead><tr>'.
						'<th class="wb-idx">#</th><th>Name</th><th>Room</th><th>Main</th><th>Group</th><th>Fixed</th>'.
						'</tr></thead><tbody>';
					$idx = 0;
					foreach ($rows as $row)
					{
						$idx++;
						$aa_id = (int) $row['aa_id'];
						$out .= '<tr data-aa="'.$aa_id.'">'.
							'<td class="wb-idx">'.$idx.'</td>'.
							'<td class="wb-name">'.check_plain($row['name']).'</td>'.
							'<td class="wb-room">'.check_plain($row['room']).'</td>'.
							'<td><input type="text" class="wb-main" data-aa="'.$aa_id.'" value="'.check_plain($row['main']).'"></td>'.
							'<td><input type="text" class="wb-group" data-aa="'.$aa_id.'" value="'.check_plain($row['group_val']).'"></td>'.
							'<td><input type="checkbox" class="wb-fixed" data-aa="'.$aa_id.'"'.($row['fixed'] ? ' checked' : '').'></td>'.
							'</tr>';
					}
					$out .= '</tbody></table></div>';
				}
				$out .= '</div>';
			}
		}
		$out .= '</div>';
	}
	$out .= '</div>';
	$out .= '</body></html>';

	if ($return) return $out;
	print $out;
	exit;
}
