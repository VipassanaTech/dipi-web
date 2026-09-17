<?php
/**
 * Allocation "Review & Edit" workbench — grouped editable grid, shared by Dining
 * AND Cells (dh_alloc_workbench($centre,$course,$resource); dh_dining_workbench /
 * dh_cell_workbench are thin wrappers). Resource-parameterized via the engine
 * descriptor (config field + main/group/fixed columns); labels switch Cell/Dining.
 *
 * A course uses ONE plan — either the default (Main) or the group-wise
 * plan — so the workbench shows a SINGLE dining column plus a switch to flip
 * between them (?plan=main|group, default main). "Save all" posts every row to
 * dining-workbench-save/%centre/%course with the active plan; the write touches
 * only that plan's column (aa_dining or aa_group_dining) plus the shared
 * aa_dining_fixed, leaving the other plan's values untouched. "Auto-fill"
 * re-runs the engine for the active plan (respecting Fixed rows) via
 * ?plan=<plan>&fill=1. "Clear non-fixed" and duplicate-seat highlighting are
 * client-side only. Each dining-seat cell is a select2 picker whose options are
 * that row's section (range + keep-aside) minus seats already taken by other
 * rows in the section (rebuilt on open); tags:true still allows a manual value.
 * See docs/superpowers/plans/2026-09-13-unified-allocation-engine-p2.md.
 */

/**
 * Build (and normally print) the dining review-and-edit workbench for one
 * course. Mirrors dh_dining_list()'s $return/print-and-exit pattern so the page
 * fn is unit-testable.
 */
function dh_alloc_workbench($centre, $course, $resource = 'dining', $return = FALSE)
{
	global $base_url;
	module_load_include('inc', 'dh_manageapp', 'inc/allocate');
	$course = (int) $course;
	$desc = dh_alloc_descriptor($resource);
	$rlabel  = ($resource === 'cell') ? 'Cell' : 'Dining';
	$maincol = $desc['main_col']; $groupcol = $desc['group_col'];
	// A course uses one plan at a time; the workbench shows one and switches.
	$plan = (isset($_REQUEST['plan']) && $_REQUEST['plan'] === 'group') ? 'group' : 'main';
	// Fixed flag is per plan (main vs group each have their own).
	$fixedcol = ($plan === 'group') ? $desc['group_fixed_col'] : $desc['fixed_col'];

	if (isset($_REQUEST['fill']))
	{
		dh_alloc_run($centre, $course, $desc, $plan);
		// Post/Redirect/Get: drop ?fill from the URL so a later refresh re-shows the
		// saved values instead of silently re-running the auto-fill, which would
		// clobber manual edits saved after the fill. Only for real page requests;
		// $return (programmatic/test) callers just get the freshly-filled page back.
		if (!$return)
			drupal_goto($resource.'-workbench/'.$centre.'/'.$course, array('query' => array('plan' => $plan)));
	}

	$cfg = db_query("select ".$desc['config']." from dh_center_setting where cs_center=:c", array(':c' => $centre))->fetchField();
	$ini = ($cfg !== '' && $cfg !== NULL) ? parse_ini_string($cfg, TRUE, INI_SCANNER_RAW) : array();
	if (!is_array($ini)) $ini = array();
	$warnings = dh_alloc_validate($ini);
	$has_server = dh_alloc_has_server_config($ini);

	$mode = ($plan === 'group') ? 'group' : 'main';
	// Option lists (range seats + keep-aside) per effective INI section, for the
	// dining-seat select2 pickers. Registered as they are used and emitted once as
	// WB_OPTS; the picker for a row offers its own section's seats.
	$wb_opts = array();
	$opts_for = function($key) use (&$wb_opts, $ini) {
		if ($key === NULL || $key === '') return array();
		if (isset($wb_opts[$key])) return $wb_opts[$key];
		$sec = (isset($ini[$key]) && is_array($ini[$key])) ? $ini[$key] : array();
		$all = array_merge(
			dh_alloc_expand(isset($sec['Cells']) ? $sec['Cells'] : ''),
			dh_alloc_expand(isset($sec['Old']) ? $sec['Old'] : ''),
			dh_alloc_expand(isset($sec['New']) ? $sec['New'] : ''),
			dh_alloc_expand(isset($sec['Reserved']) ? $sec['Reserved'] : '')   // keep-aside seats too
		);
		$seen = array(); $u = array();
		foreach ($all as $v) { if ($v !== '' && !isset($seen[$v])) { $seen[$v] = TRUE; $u[] = $v; } }
		$wb_opts[$key] = $u;
		return $u;
	};

	$course_name = db_query("select c_name from dh_course where c_id=$course")->fetchField();

	$type_filter = $has_server ? "and a_type in ('Student','Sevak')" : "and a_type='Student'";
	$batchcol = $desc['batch_col'];
	$q = "select aa_id, a_gender, a_old, aa_group, a_type, $maincol as vmain, $groupcol as vgroup, $fixedcol as vfixed,
			IFNULL($batchcol,0) as vbatch,
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
		// Groups only matter in the group-wise plan; in Main everyone in a
		// gender/old bucket shares one section, so collapse to a single block
		// (keeps seniority order) and show no group heading.
		$grp = ($plan === 'group') ? (int) $r['aa_group'] : 0;
		$room = trim($r['aa_section'].'-'.$r['aa_acco'], '-');
		$data[$class][$g][$old][$grp][] = array(
			'aa_id' => $r['aa_id'],
			'name' => $r['name'],
			'room' => $room,
			'val' => ($plan === 'group') ? $r['vgroup'] : $r['vmain'],
			'fixed' => $r['vfixed'],
			'batch' => (int) $r['vbatch'],
		);
	}

	$plan_name = ($plan === 'group') ? 'Group-wise' : 'Main';
	$unit = ($resource === 'cell') ? 'Cell No' : 'Dining Seat';
	$unit_plural = ($resource === 'cell') ? 'cells' : 'dining seats';
	$col_label = ($plan === 'group') ? ('Group '.$unit) : $unit;

	$title = dh_course_file_basename($centre, $course, $resource.'-workbench');
	$out = '<html><head><title>'.$title.'</title>';
	$out .= '<link rel="stylesheet" href="'.$base_url.'/sites/all/modules/dh_manageapp/css/select2.min.css">';
	$out .= '<style>@import url("'.$base_url.'/sites/all/modules/dh_manageapp/css/workbench.css");</style>';
	$out .= '<style>.wb-grid td .select2-container{min-width:120px}'.
		'.select2-container.wb-dup .select2-selection{border-color:#d00 !important;background:#ffe6e6 !important}'.
		// Fixed column layout so every section table lines up (same widths across tables).
		'.wb-grid{table-layout:fixed;width:100%}'.
		'.wb-grid .wbc-idx{width:44px}'.
		'.wb-grid .wbc-room{width:96px}'.
		'.wb-grid .wbc-cell{width:180px}'.
		'.wb-grid .wbc-batch{width:80px}'.
		'.wb-grid .wbc-fixed{width:70px}'.
		'.wb-bar-links{margin-bottom:10px}.wb-bar-buttons button{margin:2px 2px}'.
		'.wb-grid td.wb-name{white-space:normal;word-break:break-word}'.
		'.wb-grid th.wb-batch,.wb-grid td.wb-batch{text-align:center;color:#555}</style>';
	$out .= '</head><body>';

	$out .= '<div class="wb-header no-print"><div align="center" class="title"><b>'.check_plain($rlabel).' Review &amp; Edit</b></div>'.
			'<div align="center" class="title-head">'.check_plain($course_name).'</div></div>';

	// Plan switch: a course uses one plan; show that one, switch to the other.
	$sw = $resource.'-workbench/'.$centre.'/'.$course;
	$sw_main  = ($plan === 'main')  ? '<b>Main</b>'       : l('Main',       $sw, array('query' => array('plan' => 'main')));
	$sw_group = ($plan === 'group') ? '<b>Group-wise</b>' : l('Group-wise', $sw, array('query' => array('plan' => 'group')));

	// Clean printable list for the ACTIVE plan + resource (cell-list/dining-list,
	// group-* for the group plan). Opens in a new tab so the workbench stays put.
	$list_slug = ($plan === 'group' ? 'group-' : '').($resource === 'cell' ? 'cell-list' : 'dining-list');
	$list_path = $list_slug.'/'.$centre.'/'.$course;
	$list_link = l('Printable list', $list_path, array('attributes' => array('target' => '_blank')));

	$out .= '<div class="wb-bar no-print">'.
		// Line 1: navigation links.
		'<div class="wb-bar-links">'.
			'<a href="'.$base_url.'/course/'.$centre.'/'.$course.'">&#8592; Back to Course page</a>'.
			' &nbsp;&nbsp;&bull;&nbsp;&nbsp; <span class="wb-plan-switch">Plan: '.$sw_main.' | '.$sw_group.'</span>'.
			' &nbsp;&nbsp;&bull;&nbsp;&nbsp; '.$list_link.
		'</div>'.
		// Line 2: action buttons.
		'<div class="wb-bar-buttons">'.
			'<button type="button" id="wb-fill">Auto-fill '.$plan_name.' from ranges</button> '.
			'<button type="button" id="wb-clear">Clear non-fixed '.$unit_plural.'</button> '.
			'<button type="button" id="wb-clearall">Clear all '.$unit_plural.'</button> '.
			'<button type="button" id="wb-unfix">Clear all fixed checkboxes</button> '.
			'<button type="button" id="wb-save">Save all</button>'.
		'</div>'.
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
					$block_key = check_plain($class.'|'.$g.'|'.$old.'|'.$grp);
					// The seats offered by this bucket's pickers = its effective
					// section's range + keep-aside (main mode ignores the group).
					$seckey = dh_alloc_effective_key($ini, $g, $grp, ($class === 'Sevak' ? 'server' : 'student'), $mode);
					$opts_for($seckey);
					$sec_attr = check_plain((string) $seckey);
					// The Batch column is always shown (default 1) so every table keeps
					// identical columns and lines up.
					$show_batch = TRUE;
					$out .= '<div class="wb-group-block">'.(($plan === 'group') ? '<div class="wb-group-title">'.check_plain($grp > 0 ? 'Group '.$grp : 'No Group').'</div>' : '');
					$out .= '<table class="wb-grid">';
					$out .= '<colgroup><col class="wbc-idx"><col class="wbc-name"><col class="wbc-room"><col class="wbc-cell">'.($show_batch ? '<col class="wbc-batch">' : '').'<col class="wbc-fixed"></colgroup>';
					$out .= '<thead><tr>'.
						'<th class="wb-idx">#</th><th>Name</th><th>Room</th><th>'.check_plain($col_label).'</th>'.
						($show_batch ? '<th class="wb-batch">Batch</th>' : '').
						'<th>Fixed</th>'.
						'</tr></thead><tbody>';
					$idx = 0;
					foreach ($rows as $row)
					{
						$idx++;
						$aa_id = (int) $row['aa_id'];
						$val = (string) $row['val'];
						$batch = (int) $row['batch'];
						// A <select> (upgraded to select2 in JS). Starts with just the
						// blank + current value; the section's full seat list is added
						// lazily on first open (keeps a ~300-row page fast).
						$sel = '<select class="wb-val" data-aa="'.$aa_id.'" data-sec="'.$sec_attr.'" data-batch="'.$batch.'">'.
							'<option value=""></option>';
						if ($val !== '') $sel .= '<option value="'.check_plain($val).'" selected="selected">'.check_plain($val).'</option>';
						$sel .= '</select>';
						// Editable batch picker (value = 0-based batch, label = 1-based),
						// offering batches 1..99. Default is 1 (batch 0) when unset.
						$bsel = '';
						if ($show_batch)
						{
							$bsel = '<select class="wb-batch-sel" data-aa="'.$aa_id.'">';
							for ($bi = 0; $bi <= 98; $bi++)
								$bsel .= '<option value="'.$bi.'"'.($bi === $batch ? ' selected="selected"' : '').'>'.($bi + 1).'</option>';
							$bsel .= '</select>';
						}
						$out .= '<tr data-aa="'.$aa_id.'" data-block="'.$block_key.'" data-batch="'.$batch.'">'.
							'<td class="wb-idx">'.$idx.'</td>'.
							'<td class="wb-name">'.check_plain($row['name']).'</td>'.
							'<td class="wb-room">'.check_plain($row['room']).'</td>'.
							'<td>'.$sel.'</td>'.
							($show_batch ? '<td class="wb-batch">'.$bsel.'</td>' : '').
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
	$out .= '<script src="'.$base_url.'/sites/all/modules/dh_manageapp/js/select2.min.js"></script>';
	$out .= '<script>(function($){'.
		'var WB_CENTRE='.(int) $centre.', WB_COURSE='.(int) $course.', WB_PLAN="'.$plan.'", WB_RES="'.$resource.'", WB_UNIT="'.($resource==="cell"?"cells":"dining seats").'";'.
		'var WB_OPTS='.drupal_json_encode($wb_opts).';'.
		// Current batch of a row: the editable batch <select> when present, else the
		// row's rendered data-batch (blocks with a single batch have no picker).
		'function wbRowBatch($tr){ var $b=$tr.find(".wb-batch-sel"); return $b.length ? String($b.val()) : String($tr.data("batch")||0); }'.
		'function wbCheckDupes(){'.
			// Scope dedup to each row's data-block (class|gender|old-new|group): a
			// group with its own range legitimately reuses seat numbers other groups
			// use, so a page-wide check would false-flag those.
			'var blocks={};'.
			'$(".wb-val").each(function(){'.
				'var $r=$(this).closest("tr[data-block]"); var block=($r.data("block")||"")+"|b"+wbRowBatch($r);'.
				'if (!blocks[block]) blocks[block]={};'.
				'var v=$.trim($(this).val());'.
				'if (v==="") return;'.
				'blocks[block][v]=(blocks[block][v]||0)+1;'.
			'});'.
			'var dupes=0;'.
			'$(".wb-val").each(function(){'.
				'var $r=$(this).closest("tr[data-block]"); var block=($r.data("block")||"")+"|b"+wbRowBatch($r);'.
				'var v=$.trim($(this).val());'.
				'var isDup = v!=="" && (blocks[block]||{})[v]>1;'.
				// the <select> is hidden by select2; mirror the flag onto its container.
				'$(this).toggleClass("wb-dup", isDup).next(".select2-container").toggleClass("wb-dup", isDup);'.
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
					'rows.push({ aa: $tr.data("aa"), val: $tr.find(".wb-val").val(), df: $tr.find(".wb-fixed").is(":checked") ? 1 : 0, bt: wbRowBatch($tr) });'.
				'});'.
				'$.post("/"+WB_RES+"-workbench-save/"+WB_CENTRE+"/"+WB_COURSE, {rows: rows, plan: WB_PLAN}, function(r){'.
					'if (r && r.status) { alert("Saved "+r.saved+" row(s)."); }'.
					'else { alert("Save failed."); }'.
				'}, "json").fail(function(){ alert("Save failed."); });'.
			'});'.
			'$("#wb-fill").on("click", function(){'.
				'if (confirm("Re-fill all non-fixed "+WB_PLAN+" "+WB_UNIT+" from the centre ranges? Unsaved edits will be lost.")){'.
					'location = "/"+WB_RES+"-workbench/"+WB_CENTRE+"/"+WB_COURSE+"?plan="+WB_PLAN+"&fill=1";'.
				'}'.
			'});'.
			'$("#wb-clear").on("click", function(){'.
				'$("tr[data-aa]").each(function(){'.
					'var $tr=$(this);'.
					'if (!$tr.find(".wb-fixed").is(":checked")) $tr.find(".wb-val").val("").trigger("change");'.
				'});'.
				'wbUpdateWarn();'.
			'});'.
			'$("#wb-clearall").on("click", function(){'.
				'if (confirm("Clear ALL "+WB_UNIT+" (fixed and non-fixed)? Seat values are emptied; click Save all to persist.")){'.
					'$(".wb-val").val("").trigger("change");'.
					'wbUpdateWarn();'.
				'}'.
			'});'.
			'$("#wb-unfix").on("click", function(){'.
				'if (confirm("Untick every Fixed checkbox? Seat values are kept; click Save all to persist.")){'.
					'$(".wb-fixed:checked").prop("checked", false);'.
				'}'.
			'});'.
			// Upgrade each dining-seat <select> to select2. On every open, rebuild the
			// option list from WB_OPTS[section] (that section: range + keep-aside)
			// MINUS seats already taken by other rows in the same section, so the
			// picker only offers UNUSED seats (plus this row own current value).
			// tags:true still allows a manual/out-of-range value.
			// A seat used in one batch is still free for another batch, so scope the
			// "already used" set to this row's section AND its batch.
			'function wbUsedInSection($s, sec){'.
				'var used={}, b=wbRowBatch($s.closest("tr[data-block]"));'.
				'$(".wb-val").each(function(){ if(this!==$s[0] && $(this).attr("data-sec")===sec && wbRowBatch($(this).closest("tr[data-block]"))===b){ var v=$(this).val(); if(v){ used[v]=1; } } });'.
				'return used;'.
			'}'.
			'$(".wb-val").each(function(){'.
				'var $s=$(this);'.
				'$s.select2({tags:true, width:"150px", placeholder:"\\u2014", allowClear:true});'.
				'$s.on("select2:opening", function(){'.
					'var sec=$s.attr("data-sec"), all=WB_OPTS[sec]||[], cur=$s.val();'.
					'var used=wbUsedInSection($s, sec), added={};'.
					'var html="<option value=\\"\\"></option>";'.
					'if(cur){ html+="<option value=\\""+cur+"\\" selected=\\"selected\\">"+cur+"</option>"; added[cur]=1; }'.
					'for(var i=0;i<all.length;i++){ var v=String(all[i]); if(!used[v] && !added[v]){ html+="<option value=\\""+v+"\\">"+v+"</option>"; added[v]=1; } }'.
					'$s.html(html);'.
				'});'.
			'});'.
			'$(document).on("change", ".wb-val", wbUpdateWarn);'.
			'$(document).on("change", ".wb-batch-sel", wbUpdateWarn);'.
			'wbUpdateWarn();'.
		'});'.
	'})(jQuery);</script>';

	$out .= '</body></html>';

	if ($return) return $out;
	print $out;
	exit;
}

/* Route wrappers (keep the resource-specific callbacks + unit-test entry points). */
function dh_dining_workbench($centre, $course, $return = FALSE) { return dh_alloc_workbench($centre, $course, 'dining', $return); }
function dh_cell_workbench($centre, $course, $return = FALSE)   { return dh_alloc_workbench($centre, $course, 'cell', $return); }

/**
 * Write posted dining-workbench rows for one course into the ACTIVE plan's
 * column only (aa_dining for 'main', aa_group_dining for 'group') plus the
 * shared aa_dining_fixed; the other plan's column is left untouched. Rejects
 * any aa_id that doesn't belong to the course (security guard). Returns the
 * accepted count.
 */
function _dh_workbench_write_rows($centre, $course, $rows, $plan = 'main', $resource = 'dining')
{
	module_load_include('inc', 'dh_manageapp', 'inc/allocate');
	$course = (int) $course;
	$desc = dh_alloc_descriptor($resource);
	$col = ($plan === 'group') ? $desc['group_col'] : $desc['main_col'];
	$fixedcol = ($plan === 'group') ? $desc['group_fixed_col'] : $desc['fixed_col'];
	$batchcol = $desc['batch_col'];

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

		$fields = array(
			$col => ($val === '' ? NULL : $val),
			$fixedcol => $df ? 1 : 0,
		);
		// Batch is editable in the workbench too (only sent when the row had a
		// batch picker); persist it into the shared batch column when present.
		if (isset($row['bt']) && $row['bt'] !== '')
			$fields[$batchcol] = (int) $row['bt'];

		db_update('dh_applicant_attended')->fields($fields)->condition('aa_id', $aa)->execute();
		$n++;
	}
	return $n;
}

/**
 * Bulk-save page callback for the dining workbench's "Save all" button.
 * Locked like dh_app_update_attended() to avoid concurrent-save races.
 */
function dh_alloc_workbench_save($centre, $course, $resource = 'dining')
{
	$rows = isset($_POST['rows']) ? $_POST['rows'] : array();
	$plan = (isset($_POST['plan']) && $_POST['plan'] === 'group') ? 'group' : 'main';

	if ( !dh_manageapp_lock_acquire("dh_alloc_workbench_save", 10.0, 10) )
	{
		drupal_json_output(array('status' => 0, 'msg' => 'Unable to get lock'));
		exit;
	}

	$n = _dh_workbench_write_rows($centre, $course, $rows, $plan, $resource);
	lock_release("dh_alloc_workbench_save");

	drupal_json_output(array('status' => 1, 'saved' => $n));
	exit;
}
function dh_dining_workbench_save($centre, $course) { return dh_alloc_workbench_save($centre, $course, 'dining'); }
function dh_cell_workbench_save($centre, $course)   { return dh_alloc_workbench_save($centre, $course, 'cell'); }
