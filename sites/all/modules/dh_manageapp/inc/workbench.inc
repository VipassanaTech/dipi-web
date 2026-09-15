<?php
/**
 * Dining "Review & Edit" workbench — grouped editable grid.
 *
 * A course uses ONE dining plan — either the default (Main) or the group-wise
 * plan — so the workbench shows a SINGLE dining column plus a switch to flip
 * between them (?plan=main|group, default main). "Save all" posts every row to
 * dining-workbench-save/%centre/%course with the active plan; the write touches
 * only that plan's column (aa_dining or aa_group_dining) plus the shared
 * aa_dining_fixed, leaving the other plan's values untouched. "Auto-fill"
 * re-runs the engine for the active plan (respecting Fixed rows) via
 * ?plan=<plan>&fill=1. "Clear non-fixed" and duplicate-seat highlighting are
 * client-side only. See docs/superpowers/plans/2026-09-13-unified-allocation-engine-p2.md.
 */

/**
 * Build (and normally print) the dining review-and-edit workbench for one
 * course. Mirrors dh_dining_list()'s $return/print-and-exit pattern so the page
 * fn is unit-testable.
 */
function dh_dining_workbench($centre, $course, $return = FALSE)
{
	global $base_url;
	$course = (int) $course;
	// A course uses one plan at a time; the workbench shows one and switches.
	$plan = (isset($_REQUEST['plan']) && $_REQUEST['plan'] === 'group') ? 'group' : 'main';

	if (isset($_REQUEST['fill']))
	{
		if ($plan === 'group')
			generate_group_dining_list($centre, $course);
		else
			generate_dining_list($centre, $course);
	}

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

	// data[class][gender][old-flag][group] = ordered list of attendee rows. Each
	// row carries only the ACTIVE plan's value ('val').
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
			'val' => ($plan === 'group') ? $r['aa_group_dining'] : $r['aa_dining'],
			'fixed' => $r['aa_dining_fixed'],
		);
	}

	$plan_name = ($plan === 'group') ? 'Group-wise' : 'Main';
	$col_label = ($plan === 'group') ? 'Group Dining Seat' : 'Dining Seat';

	$title = dh_course_file_basename($centre, $course, 'dining-workbench');
	$out = '<html><head><title>'.$title.'</title>';
	$out .= '<style>@import url("'.$base_url.'/sites/all/modules/dh_manageapp/css/workbench.css");</style>';
	$out .= '</head><body>';

	$out .= '<div class="wb-header no-print"><div align="center" class="title"><b>Dining Review &amp; Edit</b></div>'.
			'<div align="center" class="title-head">'.check_plain($course_name).'</div></div>';

	// Plan switch: a course uses one plan; show that one, switch to the other.
	$sw = 'dining-workbench/'.$centre.'/'.$course;
	$sw_main  = ($plan === 'main')  ? '<b>Main</b>'       : l('Main',       $sw, array('query' => array('plan' => 'main')));
	$sw_group = ($plan === 'group') ? '<b>Group-wise</b>' : l('Group-wise', $sw, array('query' => array('plan' => 'group')));

	$out .= '<div class="wb-bar no-print">'.
		'<a href="'.$base_url.'/course/'.$centre.'/'.$course.'">&#8592; Back to Course page</a>'.
		' &nbsp;&nbsp;&bull;&nbsp;&nbsp; <span class="wb-plan-switch">Plan: '.$sw_main.' | '.$sw_group.'</span>'.
		' &nbsp;&nbsp;&bull;&nbsp;&nbsp; '.
		'<button type="button" id="wb-fill">Auto-fill '.$plan_name.' from ranges</button> '.
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
	$out .= '<div id="wb-dup-count"></div>';
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
					$block_key = check_plain($class.'|'.$g.'|'.$old.'|'.$grp);
					$out .= '<div class="wb-group-block"><div class="wb-group-title">'.check_plain($group_label).'</div>';
					$out .= '<table class="wb-grid"><thead><tr>'.
						'<th class="wb-idx">#</th><th>Name</th><th>Room</th><th>'.check_plain($col_label).'</th><th>Fixed</th>'.
						'</tr></thead><tbody>';
					$idx = 0;
					foreach ($rows as $row)
					{
						$idx++;
						$aa_id = (int) $row['aa_id'];
						$out .= '<tr data-aa="'.$aa_id.'" data-block="'.$block_key.'">'.
							'<td class="wb-idx">'.$idx.'</td>'.
							'<td class="wb-name">'.check_plain($row['name']).'</td>'.
							'<td class="wb-room">'.check_plain($row['room']).'</td>'.
							'<td><input type="text" class="wb-val" data-aa="'.$aa_id.'" value="'.check_plain($row['val']).'"></td>'.
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

	$out .= '<script src="https://code.jquery.com/jquery-1.12.4.js"></script>';
	$out .= '<script>(function($){'.
		'var WB_CENTRE='.(int) $centre.', WB_COURSE='.(int) $course.', WB_PLAN="'.$plan.'";'.
		'function wbCheckDupes(){'.
			// Scope dedup to each row's data-block (class|gender|old-new|group): a
			// group with its own range legitimately reuses seat numbers other groups
			// use, so a page-wide check would false-flag those.
			'var blocks={};'.
			'$(".wb-val").each(function(){'.
				'var block=$(this).closest("tr[data-block]").data("block")||"";'.
				'if (!blocks[block]) blocks[block]={};'.
				'var v=$.trim($(this).val());'.
				'if (v==="") return;'.
				'blocks[block][v]=(blocks[block][v]||0)+1;'.
			'});'.
			'var dupes=0;'.
			'$(".wb-val").each(function(){'.
				'var block=$(this).closest("tr[data-block]").data("block")||"";'.
				'var v=$.trim($(this).val());'.
				'var isDup = v!=="" && (blocks[block]||{})[v]>1;'.
				'$(this).toggleClass("wb-dup", isDup);'.
				'if (isDup) dupes++;'.
			'});'.
			'return dupes;'.
		'}'.
		'function wbUpdateWarn(){'.
			'var total=wbCheckDupes();'.
			'var $box=$(".wb-warn"), $dup=$("#wb-dup-count");'.
			'if (total>0){ $dup.text("Duplicate seat values: "+total); $box.show(); }'.
			'else { $dup.text(""); if ($box.find(".wb-warn-title").length===0) $box.hide(); }'.
		'}'.
		'$(function(){'.
			'$("#wb-save").on("click", function(){'.
				'var rows=[];'.
				'$("tr[data-aa]").each(function(){'.
					'var $tr=$(this);'.
					'rows.push({ aa: $tr.data("aa"), val: $tr.find(".wb-val").val(), df: $tr.find(".wb-fixed").is(":checked") ? 1 : 0 });'.
				'});'.
				'$.post("/dining-workbench-save/"+WB_CENTRE+"/"+WB_COURSE, {rows: rows, plan: WB_PLAN}, function(r){'.
					'if (r && r.status) { alert("Saved "+r.saved+" row(s)."); }'.
					'else { alert("Save failed."); }'.
				'}, "json").fail(function(){ alert("Save failed."); });'.
			'});'.
			'$("#wb-fill").on("click", function(){'.
				'if (confirm("Re-fill all non-fixed "+WB_PLAN+" dining seats from the centre ranges? Unsaved edits will be lost.")){'.
					'location = "/dining-workbench/"+WB_CENTRE+"/"+WB_COURSE+"?plan="+WB_PLAN+"&fill=1";'.
				'}'.
			'});'.
			'$("#wb-clear").on("click", function(){'.
				'$("tr[data-aa]").each(function(){'.
					'var $tr=$(this);'.
					'if (!$tr.find(".wb-fixed").is(":checked")) $tr.find(".wb-val").val("");'.
				'});'.
				'wbUpdateWarn();'.
			'});'.
			'$(document).on("input", ".wb-val", wbUpdateWarn);'.
			'wbUpdateWarn();'.
		'});'.
	'})(jQuery);</script>';

	$out .= '</body></html>';

	if ($return) return $out;
	print $out;
	exit;
}

/**
 * Write posted dining-workbench rows for one course into the ACTIVE plan's
 * column only (aa_dining for 'main', aa_group_dining for 'group') plus the
 * shared aa_dining_fixed; the other plan's column is left untouched. Rejects
 * any aa_id that doesn't belong to the course (security guard). Returns the
 * accepted count.
 */
function _dh_workbench_write_rows($centre, $course, $rows, $plan = 'main')
{
	$course = (int) $course;
	$col = ($plan === 'group') ? 'aa_group_dining' : 'aa_dining';

	$valid = array();
	$result = db_query("select aa_id from dh_applicant_attended aa join dh_applicant a on a.a_id=aa.aa_applicant where a.a_course=:c", array(':c' => $course));
	foreach ($result as $r) $valid[(int) $r->aa_id] = TRUE;

	$n = 0;
	if (!is_array($rows)) return 0;
	foreach ($rows as $row)
	{
		$aa = isset($row['aa']) ? (int) $row['aa'] : 0;
		if (!$aa || empty($valid[$aa])) continue;

		$val = isset($row['val']) ? (string) $row['val'] : '';
		$df = !empty($row['df']);

		db_update('dh_applicant_attended')->fields(array(
			$col => ($val === '' ? NULL : $val),
			'aa_dining_fixed' => $df ? 1 : 0,
		))->condition('aa_id', $aa)->execute();
		$n++;
	}
	return $n;
}

/**
 * Bulk-save page callback for the dining workbench's "Save all" button.
 * Locked like dh_app_update_attended() to avoid concurrent-save races.
 */
function dh_dining_workbench_save($centre, $course)
{
	$rows = isset($_POST['rows']) ? $_POST['rows'] : array();
	$plan = (isset($_POST['plan']) && $_POST['plan'] === 'group') ? 'group' : 'main';

	if ( !dh_manageapp_lock_acquire("dh_dining_workbench_save", 10.0, 10) )
	{
		drupal_json_output(array('status' => 0, 'msg' => 'Unable to get lock'));
		exit;
	}

	$n = _dh_workbench_write_rows($centre, $course, $rows, $plan);
	lock_release("dh_dining_workbench_save");

	drupal_json_output(array('status' => 1, 'saved' => $n));
	exit;
}
