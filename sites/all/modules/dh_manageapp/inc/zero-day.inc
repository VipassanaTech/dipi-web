<?php

/**
 * A seat's printed label from its zero-based column and row.
 *
 * cs_seat_naming_conv:
 *   0  numerical - handled by the callers, not here
 *   1  alphabetical, columns lettered:  column A,B,C  row 1,2,3  -> "A1"
 *   2  alphabetical, columns numbered:  column 1,2,3  row A,B,C  -> "1A"
 *
 * One place, so the two conventions cannot drift between the four screens that
 * print seat numbers.
 */
function dh_seat_label($col, $row, $conv)
{
	if ($conv == 2)
		return dh_seat_letter($row) . ($col + 1);   /* row letter, column number: A1, A2 across a row */
	return dh_seat_letter($col) . ($row + 1);       /* column letter, row number: A1, B1 across a row */
}

/**
 * A zero-based index as spreadsheet-style letters: 0->A .. 25->Z, 26->AA, ...
 *
 * A plain chr($n+65) breaks past Z, which matters here because convention 2 puts
 * the letters on the row axis and halls run to hundreds of rows (the data has
 * rows past 400). Convention 1 keeps letters on the columns, which never exceed
 * about twenty, so its output is unchanged.
 */
function dh_seat_letter($n)
{
	$n = (int) $n;
	$s = '';
	do {
		$s = chr(65 + ($n % 26)) . $s;
		$n = intdiv($n, 26) - 1;
	} while ($n >= 0);
	return $s;
}

function dh_manage_day_zero()
{
	switch( arg(0) )
	{
		case 'zero-day':
			if ( arg(1) == '' )
				return drupal_get_form('dh_zero_select_centre');

			if ( arg(2) == '' )
				return drupal_get_form('dh_zero_select_course');

			return dh_zero_main();
		break;
		case 'teacher-list':
      $seating = 0;
      if (isset($_REQUEST['seating']) && ($_REQUEST['seating'] == 1))
        $seating = 1;
			zeroize_new_course_data(arg(1), arg(2));
			dh_generate_teacher_list(arg(1), arg(2), $seating);
		break;
		case 'manager-list':			
			zeroize_new_course_data(arg(1), arg(2));
			dh_generate_manager_list(arg(1), arg(2));
		break;
		case 'seating':
			zeroize_new_course_data(arg(1), arg(2));
			$other_course = db_query("select c_combined_seat_course from dh_course where c_id=".arg(2))->fetchField();
			if ($other_course > 0 )
				$course = array(arg(2), $other_course);
			else
				$course = arg(2);
			if (isset($_REQUEST['r']))
				dh_auto_allocate_seats(arg(1), $course);
			return dh_generate_seating_plan(arg(1), $course);
		break;
		case 'group-seating':
			zeroize_new_course_data(arg(1), arg(2));
			$other_course = db_query("select c_combined_seat_course from dh_course where c_id=".arg(2))->fetchField();
			if ($other_course > 0 )
				$course = array(arg(2), $other_course);
			else
				$course = arg(2);
			// Fill in positions for any unseated students (full re-pack when ?r=1),
			// then render. Uses group columns; the main hall plan is untouched.
			dh_auto_allocate_group_seats(arg(1), $course, isset($_REQUEST['r']));
			return dh_generate_group_seating_plan(arg(1), $course);
		break;
		case 'group-seating-update':
			$other_course = db_query("select c_combined_seat_course from dh_course where c_id=".arg(2))->fetchField();
			if ($other_course > 0 )
				$course = array(arg(2), $other_course);
			else
				$course = arg(2);
			dh_update_group_seating(arg(1), $course);
		break;
		case 'seating2':
			zeroize_new_course_data(arg(1), arg(2));
			return dh_generate_seating_plan2(arg(1), arg(2));
		break;
    case 'seating-update':
      $other_course = db_query("select c_combined_seat_course from dh_course where c_id=".arg(2))->fetchField();
      if ($other_course > 0 )
        $course = array(arg(2), $other_course);
      else
        $course = arg(2);
      dh_update_seating(arg(1), $course);
    break;
		case 'day0-list':
			return dh_zero_day_list(arg(1), arg(2));
		break;		
		case 'cell-list':
			zeroize_new_course_data(arg(1), arg(2));
			if (isset($_REQUEST['r']))
				generate_cell_list(arg(1), arg(2));
			$debug = 0 ;
			if (isset($_REQUEST['d']) && ($_REQUEST['d'] == 1))
				$debug = 1;
			return dh_cell_list(arg(1), arg(2), $debug);
		break;		
		case 'laundry-list':
			return dh_laundry_list(arg(1), arg(2));
		break;		
		case 'valuable-list':
			return dh_valuable_list(arg(1), arg(2));
		break;		
		case 'student-chit':
      $seating = 0;
      if (isset($_REQUEST['seating']) && ($_REQUEST['seating'] == 1))
        $seating = 1;
			return dh_student_chit(arg(1), arg(2), $seating);
		break;
    case 'checking-slip':
      $seating = 0;
      if (isset($_REQUEST['seating']) && ($_REQUEST['seating'] == 1))
        $seating = 1;
      return dh_checking_slip(arg(1), arg(2), $seating);
    break;
		case 'app':
			$row = array();
			$ajax = '';
			if (arg(2) == 'edit') 
			{
				$ajax = arg(3);
				$row = db_query("select * from dh_applicant left join dh_applicant_extra on a_id=ae_applicant 
					left join dh_applicant_course on a_id=ac_applicant left join dh_applicant_attended on a_id=aa_applicant
					left join dh_applicant_lc on a_id=al_applicant 
					where a_id='".arg(1)."'")->fetchAssoc();
			}
			elseif ( arg(1) == 'add' ) 
			{
				$ajax = arg(4);
				if ( isset($_REQUEST['ref']) && ($_REQUEST['ref'] <> '') )
				{
					$row = db_query("select * from dh_applicant left join dh_applicant_extra on a_id=ae_applicant 
						left join dh_applicant_course on a_id=ac_applicant left join dh_applicant_lc on a_id=al_applicant 
						where a_id='".$_REQUEST['ref']."'")->fetchAssoc();	
					$row['ref'] = 1;
          $row['ref_id'] = $_REQUEST['ref'];
				}
			}

			if ($ajax <> '')
			{
				ctools_include('ajax');
    			ctools_include('modal');
   			  	drupal_add_library('system', 'ui.autocomplete');  			  	
   			  	ctools_add_js('manageapp', 'dh_manageapp', '');

    			$form_state = array(
      				'ajax' => TRUE,
      				'title' => t('Applicant Details'),
    			);
    			$form_state['build_info']['args'] = array($row);
    			$output = ctools_modal_form_wrapper('dh_ma_applicant_form', $form_state );
    			//$output[] = ctools_command_invoke(NULL, 'check_attending');
    			if (!empty($form_state['ajax_commands'])) 
    			{
     				 $output = $form_state['ajax_commands'];
    			}
    			//$output[] = ctools_ajax_command_reload();
    			print ajax_render($output);
    			drupal_exit();					
			}
			else				
				return drupal_get_form('dh_ma_applicant_form', $row);
				
		break;

	}
}


function dh_zero_select_centre($form, &$form_state)
{
	global $user;
	drupal_add_css(drupal_get_path('module', 'dh_manageapp').'/css/select2.min.css');
	drupal_add_js(drupal_get_path('module', 'dh_manageapp').'/js/select2.min.js');

	if ( user_access("access all centres"))
	{
		$admin = '<h1>SiteAdmin</h1><ul><li><h2>'.l("Add Centre", "centre/add").'</h2></li>'.
		'<li><h2>'.l("Manage User Mapping", "user-mapping").'</h2></li>'.
		'<li><h2>'.l("Search", "search-app").'</h2></li></ul>';

		$form['a'] = array('#markup' => $admin );
		$q = "select c_id, c_name from dh_center order by c_name";
	}
	else if (user_access('access center dashboard')) 
	{
        drupal_goto('center-dashboard');
    } 
    else if (user_access('access all center dashboard')) {
        drupal_goto('vri-management');
    }
    else if (user_access('approve applications')) {
    	$search_lc = '<ul><li><h2>'.l("AT-Portal", "at-portal").'</h2></li>'.'<li><h2>'.l("R-AT Review", "search-lc/r-atreview").'</h2></li>'.'<li><h2>'.l("A-AT Review", "search-lc/a-atreview").'</h2></li></ul>';
    	$form['a'] = array('#markup' => $search_lc );
    	$q = "select c_id, c_name from dh_center left join dh_user_center on (c_id=uc_center and uc_deleted=0) where uc_user='".$user->uid."' order by c_name";
    }
    else if (user_access('a-at review')) {
    	$search_lc = '<ul><li><h2>'.l("R-AT Review", "search-lc/r-atreview").'</h2></li>'.'<li><h2>'.l("A-AT Review", "search-lc/a-atreview").'</h2></li></ul>';
    	$form['a'] = array('#markup' => $search_lc );
    	$q = "select c_id, c_name from dh_center left join dh_user_center on (c_id=uc_center and uc_deleted=0) where uc_user='".$user->uid."' order by c_name";
    }
    else if (user_access('r-at review')) {
    	$search_lc = '<ul><li><h2>'.l("R-AT Review", "search-lc/r-atreview").'</h2></li></ul>';
    	$form['a'] = array('#markup' => $search_lc );
    	$q = "select c_id, c_name from dh_center left join dh_user_center on (c_id=uc_center and uc_deleted=0) where uc_user='".$user->uid."' order by c_name";
    }
	else
	{
		$res = db_query("select uc_center from dh_user_center where uc_user='".$user->uid."'  and uc_deleted=0");
		$c = 0;
		while($r = $res->fetchAssoc())
		{
			$centre = $r['uc_center'];
			$c++;
		}
		if (( $c == 1 ) && (is_numeric($centre)) )
			drupal_goto(arg(0)."/$centre");
		else
		{
			$q = "select c_id, c_name from dh_center left join dh_user_center on (c_id=uc_center and uc_deleted=0) where uc_user='".$user->uid."' order by c_name";
		}
	}
	$centres = db_query($q)->fetchAllKeyed();
	if ( count( $centres ) == 1 )
	{
		list($cid, $cname) = each($centres);
		drupal_goto(arg(0)."/".$cid);
	}
	$centres[''] = 'Choose';
	$form['centre'] = array('#title' => 'Select Centre', '#type' => 'select', '#options' => $centres, '#weight' => 1);
	$js = '
		(function ($) {
			$(document).ready(function(){
				$("#edit-centre").select2();
				$("#edit-centre").change(function(){
					//alert($(this).val());
					location.href = "/centre/"+$(this).val();
				});
			});
		})(jQuery);
	';
	drupal_add_js($js, 'inline');

	return $form;

}

function dh_zero_select_course($form, &$form_state, $with_header = true)
{
	global $user;
	$centre_name = db_query("select c_name from dh_center where c_id=".arg(1))->fetchField();
	$max_old_courses =  date('Y-m-d',strtotime("-".variable_get('max_old_courses', '6 month')));
	//$course_id = db_query("select c_id from dh_course where c_center='".arg(1)."' and c_start ='".date("Y-m-d")."' limit 1")->fetchField();
	$course_id = '';

	if ( $course_id == '' )
	{
		drupal_add_css(drupal_get_path('module', 'dh_manageapp').'/css/select2.min.css');
		drupal_add_js(drupal_get_path('module', 'dh_manageapp').'/js/select2.min.js');
		$title = '';
		if ( $with_header )
		{
			$title = '<h1>'.$centre_name.'</h1>';
			$form['a'] = array('#markup' => '<h2>'.l("Back to Dashboard", "centre/".arg(1)).'</h2>');
		}
		$q = "select c_id, c_name from dh_course where c_center='".arg(1)."' and c_start >='$max_old_courses' and c_deleted=0  order by c_start";
		$courses = array();
		$courses[''] = 'Choose';
		$courses += db_query($q)->fetchAllKeyed();
		$form['course'] = array('#title' => 'Select Course', '#type' => 'select', '#options' => $courses, '#weight' => 1, 
			'#prefix' => $title);		
	}
	else
		drupal_goto("zero-day/".arg(1)."/$course_id");

	$js = '
		(function ($) {
			$(document).ready(function(){
				$("#edit-course").select2();
				$("#edit-course").change(function(){
					//alert($(this).val());
					location.href = "/course/'.arg(1).'/"+$(this).val();
				});
			});
		})(jQuery);
	';
	drupal_add_js($js, 'inline');
	return $form;
}


function dh_get_acco($centre_id, $course_id)
{
	$acco['M'] = $acco['F'] = array();
	$q = "select csa_gender, csa_section, csa_room from dh_center_setting_acco where csa_center=$centre_id and csa_deleted=0";
	$result = db_query($q);
	while($r = $result->fetchAssoc())
	{
		$temp = explode(",", $r['csa_room']);
		foreach($temp as $v )
			if (trim($v) <> '')
				$acco[strtoupper($r['csa_gender'])][$r['csa_section']][] = trim($v); 
	}
	return $acco;
}

function dh_get_alloted($centre_id, $course_id)
{
	$alloted['M'] = $alloted['F'] = array();
	$q = "select a_gender, aa_section, aa_acco from dh_applicant left join dh_applicant_attended on a_id=aa_applicant where a_course=$course_id and a_attended=1";
	$result = db_query($q);
	while($r = $result->fetchAssoc())
	{
		$alloted[strtoupper($r['a_gender'])][$r['aa_section']][] = $r['aa_acco']; 
	}
	return $alloted;
}


function dh_manageapp_list($centre_id, $course_id)
{
	$q = "select a_id, a_uri, a_conf_no as 'ConfNo', concat(a_f_name, ' ',a_m_name, ' ',a_l_name) as 'Full Name', a_gender, c_name, a_city_str, TIMESTAMPDIFF( YEAR, a_dob, CURDATE() ) as 'Age' from dh_applicant left join dh_applicant_attended on a_id=aa_applicant left join dh_city on a_city=c_id where a_center='$centre_id' and a_course='$course_id' and a_attended=0 order by a_f_name, a_m_name, a_l_name";
//and a_status='Confirmed' 
	$result = db_query($q);
	$rows = array();
	while( $r = $result->fetchAssoc() )
	{
		unset($rs);

		$rs[] = $r['a_id'];
		$rs[] = $r['ConfNo'];
		$pdf_link = '';
		if ( $r['a_uri'] <> '' )
		{
			//$pdf_link = '&nbsp;('.l("PDF", file_create_url($r['a_uri']), array('attributes' => array("target" => "_blank") ) ).')';
			$pdf_link = '&nbsp;('.l("PDF", "show-application/".$r['a_id'], array('attributes' => array("target" => "_blank") ) ).')';
		}
		$rs[] = l($r['Full Name'],'app/'.$r['a_id'].'/edit', array('attributes' => array('appid' => $r['a_id'] )) ).$pdf_link;
		$rs[] = (strtoupper($r['a_gender'])=='M')?'Male':'Female';
		$rs[] = $r['c_name']?$r['c_name']:$r['a_city_str']; 
		$rs[] = $r['Age'];
		$rows[] = array_values($rs);
	}
	$header = array('Action', 'ConfNo', 'Name', 'Gender', 'City', 'Age');
	$attributes = array('id' => 'table-applicants');
  $out = "<h2><br>Unattended Applicants</h2>";
	$out .= theme('table', array('header' => $header, 'rows' => $rows, 'attributes' => $attributes));
	return $out;
}

function dh_zero_main()
{
	ctools_include('modal');
	ctools_include('ajax');
  	ctools_modal_add_js();
	drupal_add_library('system', 'ui.core');
	drupal_add_library('system', 'ui.dialog');

  	$_SESSION['lasturi'] = $_SERVER['REQUEST_URI'];

	$finalized = db_query("select c_finalized from dh_course where c_id=".arg(2))->fetchField();
	if ( $finalized )
	{
		drupal_set_message("Course has been Finalized, cannot access zero day", "error");
		drupal_goto("course/".arg(1)."/".arg(2));
	}


  	$q = "select c_name from dh_course where c_id=".arg(2);
  	$course_name = db_query($q)->fetchField();
  	$out = '<h3>'.l("Back to Course", "course/".arg(1).'/'.arg(2)).'&nbsp;&nbsp;|&nbsp;&nbsp;'.l("Back to Dashboard", "centre/".arg(1)).'</h3>';
  	$out .= '<h2>'.$course_name.'</h2>';
  	$out .= '<h4>'._ma_make_link('Add New Applicant', 'app/add/'.arg(1).'/'.arg(2)).'</h4>';

	//drupal_add_js(drupal_get_path('module', 'dh_manageapp').'/datatables.min.js' );
	//drupal_add_css(drupal_get_path('module', 'dh_manageapp').'/datatables.min.css' );
	drupal_add_css( libraries_get_path("editor"). "/DataTables/datatables.min.css");
	drupal_add_css('
	  .validateTips {
	  	color: #ff0000;
	  	font-weight: bold;
	  }
		', 'inline');

	drupal_add_js( libraries_get_path("editor").  "/DataTables/datatables.min.js");

	$centre_id = arg(1);
	$course_id = arg(2);

	$acco_json = json_encode(dh_get_acco($centre_id, $course_id));
	$alloted_json = json_encode(dh_get_alloted($centre_id, $course_id));


	$out .= '<div id="applicant-list">'.dh_manageapp_list( $centre_id, $course_id ).'</div>';
	//$out .= '<div id="replace-me"></div>';
	$js = '
		(function ($) {
			var acco = '.$acco_json.';
			var alloted = '.$alloted_json.';
			var cur_gender = "";
			var cur_app_id = "";
			var app_table, att_table, def_room;

			function load_editor()
			{
				app_table = $("#table-applicants").DataTable({
				    "columnDefs": [{
				      "targets": 0,
				      "data": null,
				      "sortable": false,
				      "defaultContent": "<button>Update</button>"
				    }]

				});
				att_table = $("#table-attending").DataTable({
				    "columnDefs": [{
				      "targets": 0,
				      "data": null,
				      "sortable": false,
				      "defaultContent": "<button>Update</button>"
				    }, 
				    {
				    	"targets": 15,
				    	"visible": false,
				    	"searchable": false,
				    }]
				});


			}

			function attended_dialog(params)
			{
				var name = $("<div/>").html(params.applicant_name).text().toUpperCase();
				var group = "";
				for(i=1; i<10; i++)
					group += "<option value=\""+i+"\">"+i+"</option>";
				var s = "<option value=\"\">Choose</option>";
				$.each(params.sections, function(key, item) {
					s += "<option value=\""+key+"\">"+key+"</option>";
				});
				var out = "" +
				"<div id=\"dialog-form\" title=\"Student Attended\">"+
				"  <label class=\"control-label title-applicant-name\">"+name+
				"      	&nbsp;&nbsp;&nbsp;Attending&nbsp;<input type=\"checkbox\" name=\"attending\" id=\"attending\" value=\"1\" class=\"text ui-widget-content ui-corner-all\" checked=checked >"+
				" </label>"+
				"  <p class=\"validateTips\"></p>"+
				"	<form class=\"form-inline\">"+
				"	<fieldset>"+
				"	  <div class=\"form-item form-group\">"+
				"	      <label class=\"control-label\" for=\"section\">Room Section</label>"+
				"	      <select name=\"section\" id=\"section\" class=\"form-control attending-fields select ui-widget-content ui-corner-all\">"+s+"</select>"+
				"      </div>"+
				"      <div class=\"form-item form-group\">"+
				"      	<label class=\"control-label\" for=\"room-no\">Room No</label>"+
				"      	<select name=\"room-no\" id=\"room-no\" class=\"form-control attending-fields select ui-widget-content ui-corner-all\"><option value=\"\">Choose</option></select>"+
				"      </div>"+
				"	  <div class=\"form-item form-group\">"+
				"      	<label class=\"control-label\" for=\"laundry\">Laundry</label>"+
				"      	<input type=\"text\" name=\"laundry\" id=\"laundry\" size=\"5\" value=\"\" class=\"form-control attending-fields text ui-widget-content ui-corner-all\">"+
				"      </div>"+
				"	  <div class=\"form-item form-group\">"+
				"      	<label class=\"control-label\" for=\"valuable\">Valuable</label>"+
				"      	<input type=\"text\" name=\"valuable\" id=\"valuable\" size=\"5\" value=\"\" class=\"form-control attending-fields text ui-widget-content ui-corner-all\">"+
				"      </div>"+
				"	  <div class=\"form-item form-group\">"+
				"      	<label class=\"control-label\" for=\"cell\">CellNo</label>"+
				"      	<input type=\"text\" name=\"cell\" id=\"cell\" value=\"\" size=\"5\" class=\"form-control attending-fields text ui-widget-content ui-corner-all\">"+
				"      </div>"+
				"	  <div class=\"form-item form-group\">"+
				"	      <label class=\"control-label\" for=\"cell-fixed\">Cell Fixed</label>"+
				"	      <input type=\"checkbox\" name=\"cellfixed\" id=\"cell-fixed\" value=\"1\" class=\"attending-fields ui-widget-content ui-corner-all\">"+
				"      </div>"+
				"      <div class=\"form-item form-group\">"+
				"      	<label class=\"control-label\" for=\"group\">Group</label>"+
				"      	<select name=\"group\" id=\"group\" class=\"form-control attending-fields select ui-widget-content ui-corner-all\">"+group+"</select>"+
				"      </div>"+
				"      <div class=\"form-item form-group\">"+
				"      	<label class=\"control-label\">Special Seating</label>"+
				"		<div class=\"form-radios\" >"+
				"      		<label><input type=\"radio\" id=\"special-seating-none\" name=\"specialseat\" class=\"attending-fields select ui-widget-content ui-corner-all\" >"+
				"      	 	None</label>" +
				"      		<label><input type=\"radio\" id=\"special-seating-chowky\" name=\"specialseat\" class=\"attending-fields select ui-widget-content ui-corner-all\" >"+
				"      	 	Chowky</label>" +
				"      		<label><input type=\"radio\" id=\"special-seating-chair\" name=\"specialseat\" class=\"attending-fields select ui-widget-content ui-corner-all\" >"+
				"      	 	Chair</label>" +
				"      		<label><input type=\"radio\" id=\"special-seating-backrest\" name=\"specialseat\" class=\"attending-fields select ui-widget-content ui-corner-all\" >"+
				"      	 	BackRest</label>" +
				"	   </div>"+
				"	  <div class=\"form-item form-group\">"+
				"	      <label class=\"control-label\" for=\"comments\">Comments</label>"+
				"	      <textarea name=\"comments\" id=\"comments\" rows=\"4\" class=\"form-control attending-fields\">"+
				"    	  </textarea>"+
				"      </div>"+
				"	</fieldset>"+
				"	</form>"+
				"</div>";

				return out;
			}			
			function check_attending()
			{
				//alert("called - "+$("input[name=attending]:checked").val());
		 		if( $("input[name=attending]:checked").val() )
					$(".attending-fields").prop("disabled", false);
				else
					$(".attending-fields").prop("disabled", true);				
			}

			function get_sections( gender )
			{
				var sec = {};
				$.each(acco[gender], function(key, item) 
				{
					if ( item.length > 0 )
						sec[key] = key;
				});	
				return sec;
			}

			function get_rooms( section, gender, default_room = "" )
			{
				var rooms = acco[gender][section].slice();
				if (alloted[gender][section])
				{
					$.each(alloted[gender][section], function(key, item) 
					{
						if ( item != default_room )
						{
							var i = rooms.indexOf(item);
							if (i > -1)
								rooms.splice(i, 1);
						}
					});						
				}
				return rooms;
			}

			$(document).ready(function(){
			    //form = dialog.find( "form" ).on( "submit", function( event ) {
			    //  event.preventDefault();
			    //});

				load_editor();
	  		  	$(document).on( "click", "#table-applicants tbody button,#table-attending tbody button", function () {
				    var row_id = $(this).parents("tr");
				    var t = $(this).closest("table");
				    var data;
				    if (t.attr("id") == "table-applicants")
				    	data = app_table.row(row_id).data();
				    else
				    	data = att_table.row(row_id).data();

				    //$(".title-applicant-name").html(data[2]);
				    cur_app_id =  $(data[2]).attr("appid");
					if ( data[3] == "Male")
						cur_gender = "M";
					else
						cur_gender = "F";

				    var sections = get_sections( cur_gender );
				    //console.log(sections);
				    var d = $("#dialog-div").html( attended_dialog({applicant_name: data[2], sections: sections })).dialog({
				      	height: 460,
				      	width: 900,
				      	modal: true,
				      	buttons: {
				        	"Update": function() { 
					        	$(".ui-dialog-buttonset").loading({zIndex: 9999});
						        $.ajax({
						          url: "/app-update-attended/" + cur_app_id ,
						          method: "POST",
						          data: {s: $("#section").val(), r: $("#room-no").val(), g: $("#group").val(),l: $("#laundry").val(), v: $("#valuable").val(),
						          	c: $("#cell").val(), cf: $("#cell-fixed").is(":checked") , chow: $("#special-seating-chowky").is(":checked") , 
						          	chai: $("#special-seating-chair").is(":checked"), back:  $("#special-seating-backrest").is(":checked"),
						          	comment: $("#comments").val(), a: $("#attending").is(":checked")  },
						          dataType: "json",
						        })
								.done(function( data ){
						          	if ( ! data.status )
						          	{
						          		$(".validateTips").html(data.msg);
							        	$(".ui-dialog-buttonset").loading("stop");
						          	}
						          	else
						          	{
						          		$("#applicant-list").html(data.applicant);
						          		$("#attended-list").html(data.attended);
						          		acco = data.acco;
						          		alloted = data.alloted;
						          		load_editor();
							        	$(".ui-dialog-buttonset").loading("stop");
							        	d.dialog("close");
						          	}
						        })
								.fail(function(jqXHR, textStatus){
						        	$(".ui-dialog-buttonset").loading("stop");
									alert("Failed!!" + textStatus)

								});
				        	},
				        	Cancel: function() {
				          		$(this).dialog( "close" );
				        	}
				      	},
				      	close: function() {
				        	//form[ 0 ].reset();
				        	//allFields.removeClass( "ui-state-error" );
				      	}
			    	});
				    if (t.attr("id") == "table-attending")
				    {
				    	var temp = data[8].split("-");
				    	def_room = data[8].substring(temp[0].length+1);
				    	$("#section").val(temp);
				    	$("#section").trigger("change");
				    	$("#laundry").val(data[9]);
				    	$("#valuable").val(data[10]);
				    	$("#special-seating-none").prop("checked", true);
				    	if (data[11] == "Yes") $("#special-seating-chowky").prop("checked", true);
				    	if (data[12] == "Yes") $("#special-seating-chair").prop("checked", true);
				    	if (data[13] == "Yes") $("#special-seating-backrest").prop("checked", true);
				    	$("#group").val(data[14]);
				    	$("#room-no").val(def_room);
				    	temp = data[15].split("||");
				    	$("#cell").val(temp[0]);
				    	if (temp[1] == "1") $("#cell-fixed").prop("checked", true);
				    	$("#comments").val(temp[2]);
				    	//alert(temp[1]);
				    }
				});

				$(document).on("change", "input[type=checkbox][name=attending]", function(){
					check_attending();
				});

				$(document).on("change", "#section", function(){
					var r = get_rooms($(this).val(), cur_gender, def_room);
					$("#room-no").empty();
					var t = $("<option></option>").attr("value", "").text("Choose");
					$("#room-no").append(t);
					$.each(r, function(k,v) {					
						t = $("<option></option>").attr("value", v).text(v);
						$("#room-no").append(t);
					});

				});
			 
			});

		})(jQuery);

	';
	drupal_add_js($js, 'inline');
	drupal_add_css(drupal_get_path('module', 'dh_manageapp').'/css/manageapp.css');

	$out .= '<div id="dialog-div"></div><div id="attended-list">'.dh_manageapp_attended($centre_id, $course_id).'</div>';

	return $out;
}



function dh_manageapp_attended( $centre, $course_id )
{
	$q = "select count(a_id) as 'c', a_gender, a_old, a_type from dh_applicant where a_course='".$course_id."' and a_status in ('Confirmed', 'Expected') group by a_gender, a_old, a_type";
	$res = db_query($q);
	$conf = array();
	while($r = $res->fetchAssoc())
	{
		$conf[$r['a_old']?'old':'new'][$r['a_gender']][$r['a_type']] = $r['c'];
	}

	//$q = "select a_id, a_center, a_gender, a_old, a_conf_no as 'ConfNo', concat(a_f_name, ' ',a_m_name, ' ',a_l_name) as 'Full Name', TIMESTAMPDIFF( YEAR, a_dob, CURDATE() ) as 'Age', ac_10d, ac_stp, ac_spl, ac_20d, ac_30d, ac_45d, ac_60d, ac_tsc, ac_teacher, aa_group, aa_acco, aa_section, aa_laundry, aa_chowky, aa_valuable, aa_chowky, aa_chair, aa_backrest from dh_applicant left join dh_applicant_course on a_id=ac_applicant left join dh_applicant_attended on a_id=aa_applicant where a_course='$course_id' and aa_acco IS NOT NULL order by a_f_name, a_m_name, a_l_name";
	$q = "select a_id, a_type, a_center, a_gender, a_old, a_conf_no as 'ConfNo', concat(a_f_name, ' ',a_m_name, ' ',a_l_name) as 'Full Name', TIMESTAMPDIFF( YEAR, a_dob, CURDATE() ) as 'Age', a_uri, ac_teen, ac_10d, ac_stp, ac_spl, ac_20d, ac_30d, ac_45d, ac_60d, ac_tsc, ac_teacher, aa_group, aa_acco, aa_section, aa_laundry, aa_cell, aa_cell_fixed, aa_comment, aa_valuable, aa_chowky, aa_chair, aa_backrest from dh_applicant left join dh_applicant_course on a_id=ac_applicant left join dh_applicant_attended on a_id=aa_applicant where a_course='$course_id' and a_attended=1 order by a_f_name, a_m_name, a_l_name";
	$result = db_query($q);
	$rows = array();
	$totals['new']['F']['Student'] = $totals['new']['F']['Sevak'] = $totals['old']['F']['Student'] = $totals['old']['F']['Sevak'] = $totals['new']['M']['Student'] = $totals['new']['M']['Sevak'] = $totals['old']['M']['Student'] = $totals['old']['M']['Sevak'] = 0;
	$special['new']['F']['chowky'] = $special['old']['F']['chowky'] = $special['new']['M']['chowky'] = $special['old']['M']['chowky'] = 0;
	$special['new']['F']['chair'] = $special['old']['F']['chair'] = $special['new']['M']['chair'] = $special['old']['M']['chair'] = 0;
	$special['new']['F']['backrest'] = $special['old']['F']['backrest'] = $special['new']['M']['backrest'] = $special['old']['M']['backrest'] = 0;
	$group = array();
	//$group['new']['F'][1] = $group['old']['F'][1] = $group['new']['M'][1] = $group['old']['M'][1] = 0;
	//$centre = 0;
	while( $r = $result->fetchAssoc() )
	{
		unset($rs);
		//$centre = $r['a_center'];
		$rs[] = $r['a_id'];
		$rs[] = $r['ConfNo'];
		$pdf_link = '';
		if ( $r['a_uri'] <> '' )
		{
			//$pdf_link = '<br>('.l("PDF", file_create_url($r['a_uri']), array('attributes' => array("target" => "_blank") ) ).')';
			$pdf_link = '<br>('.l("PDF", "show-application/".$r['a_id'], array('attributes' => array("target" => "_blank") ) ).')';
		}
		$rs[] = l($r['Full Name'],'app/'.$r['a_id'].'/edit',array('attributes' => array('appid' => $r['a_id'] ))).$pdf_link;
		$rs[] = ($r['a_gender']=='M')?'Male':'Female';
		$rs[] = $r['a_type'];
		$rs[] = $r['Age'];
		$rs[] = $r['ac_teen']+$r['ac_10d']+$r['ac_stp'];
		$rs[] = $r['ac_spl']+$r['ac_20d']+$r['ac_30d']+$r['ac_45d']+$r['ac_60d']+$r['ac_tsc'];
		$rs[] = $r['aa_section'].'-'.$r['aa_acco'];
		$rs[] = $r['aa_laundry'];
		$rs[] = $r['aa_valuable'];
		$rs[] = $r['aa_chowky']?'Yes':'No';
		$rs[] = $r['aa_chair']?'Yes':'No';
		$rs[] = $r['aa_backrest']?'Yes':'No';
		$rs[] = $r['aa_group'];
		$rs[] = $r['aa_cell']."||".$r['aa_cell_fixed']."||".$r['aa_comment'];		
		$rows[] = array_values($rs);

		$totals[$r['a_old']?'old':'new'][strtoupper($r['a_gender'])][$r['a_type']] += 1;
		$special[$r['a_old']?'old':'new'][strtoupper($r['a_gender'])]['chowky'] += $r['aa_chowky'];
		$special[$r['a_old']?'old':'new'][strtoupper($r['a_gender'])]['chair'] += $r['aa_chair'];
		$special[$r['a_old']?'old':'new'][strtoupper($r['a_gender'])]['backrest'] += $r['aa_backrest'];
		if ( !isset($group[$r['aa_group']][$r['a_old']?'old':'new'][strtoupper($r['a_gender'])][$r['a_type']] ) )
			$group[$r['aa_group']][$r['a_old']?'old':'new'][strtoupper($r['a_gender'])][$r['a_type']] = 0;
		$group[$r['aa_group']][$r['a_old']?'old':'new'][strtoupper($r['a_gender'])][$r['a_type']] += 1;
	}
	$header = array('Update', 'ConfNo', 'Name', 'Gender', 'Type', 'Age', 'Teen/10D/STP', 'LC', 'RoomNo', 'Laundry', 'Valuable', 'Chowky', 'Chair', 'BackRest', 'Group', 'H' );
	$attributes = array('id' => 'table-attending');

	$out = '<br /><br /><h3>Attending Applicants  ( '.l("Assign Teacher", "assign-teacher/".$centre."/".$course_id).' | '.l("Teacher List", "teacher-list/".$centre."/".$course_id).' | '.l("Manager List", "manager-list/".$centre."/".$course_id).' | '.l("Seating Plan", "seating/".$centre."/".$course_id).' ('.l("Re-Gen", "seating/".$centre."/".$course_id, array("query" => array("r" => 1))).')  | '.l("Group-wise Seating", "group-seating/".$centre."/".$course_id).'  ) </h3>';
	if (!isset($totals['new']['M']['Student'])) $totals['new']['M']['Student'] = 0;
  if (!isset($totals['new']['M']['Sevak'])) $totals['new']['M']['Sevak'] = 0;
  if (!isset($totals['new']['F']['Student'])) $totals['new']['F']['Student'] = 0;
  if (!isset($totals['new']['F']['Sevak'])) $totals['new']['F']['Sevak'] = 0;
	$all_total = $totals['new']['M']['Student']+$totals['new']['M']['Sevak']+$totals['old']['M']['Student']+$totals['old']['M']['Sevak']+$totals['new']['F']['Student']+$totals['new']['F']['Sevak']+$totals['old']['F']['Student']+$totals['old']['F']['Sevak'];
	$rows_totals[] = array(
    'Attended Male',
    $totals['old']['M']['Student'],
    $totals['new']['M']['Student'],
    '<b>'.($totals['new']['M']['Student']+$totals['old']['M']['Student']).'<b>',
    ($totals['new']['M']['Sevak']+$totals['old']['M']['Sevak']),
    // '<b>'.($totals['new']['M']['Student']+$totals['old']['M']['Student']+$totals['new']['M']['Sevak']+$totals['old']['M']['Sevak']).'<b>',
  );
	$rows_totals[] = array(
    'Attended Female',
    $totals['old']['F']['Student'],
    $totals['new']['F']['Student'],
    '<b>'.($totals['new']['F']['Student']+$totals['old']['F']['Student']).'<b>',
    ($totals['new']['F']['Sevak']+$totals['old']['F']['Sevak']),
    // '<b>'.($totals['new']['F']['Student']+$totals['old']['F']['Student']+$totals['new']['F']['Sevak']+$totals['old']['F']['Sevak']).'<b>',
  );
	$rows_totals[] = array(
    '<b>Total</b>',
    ($totals['old']['M']['Student']+$totals['old']['F']['Student']),
    ($totals['new']['M']['Student']+$totals['new']['F']['Student']),
    '<b>'.($totals['new']['M']['Student']+$totals['old']['M']['Student']+$totals['new']['F']['Student']+$totals['old']['F']['Student']).'<b>',
    ($totals['new']['M']['Sevak']+$totals['old']['M']['Sevak']+$totals['new']['F']['Sevak']+$totals['old']['F']['Sevak']),
    // "<b>$all_total</b>",
  );

	$header_totals = array('', 'Old', 'New', 'Total', 'Server');
	if (!isset($conf['new']['M']['Student'])) $conf['new']['M']['Student'] = 0;
  if (!isset($conf['new']['M']['Sevak'])) $conf['new']['M']['Sevak'] = 0;
	if (!isset($conf['new']['F']['Student'])) $conf['new']['F']['Student'] = 0;
  if (!isset($conf['new']['F']['Sevak'])) $conf['new']['F']['Sevak'] = 0;
	$all_conf_total = $conf['new']['M']['Student']+$conf['new']['M']['Sevak']+$conf['old']['M']['Student']+$conf['old']['M']['Sevak']+$conf['new']['F']['Student']+$conf['new']['F']['Sevak']+$conf['old']['F']['Student']+$conf['old']['F']['Sevak'];
	$rows_conf[] = array(
    'Confirmed Male',
    $conf['old']['M']['Student'],
    $conf['new']['M']['Student'],
    '<b>'.($conf['new']['M']['Student']+$conf['old']['M']['Student']).'<b>',
    ($conf['new']['M']['Sevak']+$conf['old']['M']['Sevak']),
    // '<b>'.($conf['new']['M']['Student']+$conf['old']['M']['Student']+$conf['new']['M']['Sevak']+$conf['old']['M']['Sevak']).'<b>',
  );
	$rows_conf[] = array(
    'Confirmed Female',
    $conf['old']['F']['Student'],
    $conf['new']['F']['Student'],
    '<b>'.($conf['new']['F']['Student']+$conf['old']['F']['Student']).'<b>',
    ($conf['new']['F']['Sevak']+$conf['old']['F']['Sevak']),
    // '<b>'.($conf['new']['F']['Student']+$conf['old']['F']['Student']+$conf['new']['F']['Sevak']+$conf['old']['F']['Sevak']).'<b>',
  );
	$rows_conf[] = array(
    '<b>Total</b>',
    ($conf['old']['M']['Student']+$conf['old']['F']['Student']),
    ($conf['new']['M']['Student']+$conf['new']['F']['Student']),
    '<b>'.($conf['new']['M']['Student']+$conf['old']['M']['Student']+$conf['new']['F']['Student']+$conf['old']['F']['Student']).'<b>',
    ($conf['new']['M']['Sevak']+$conf['old']['M']['Sevak']+$conf['new']['F']['Sevak']+$conf['old']['F']['Sevak']),
    // "<b>$all_conf_total</b>",
  );


	$header_special = array('', 'Chowky', 'Chair', 'Backrest');
	$rows_special[] = array( 'Male', $special['old']['M']['chowky'].' (O) + '.$special['new']['M']['chowky'].' (N)',
								$special['old']['M']['chair'].' (O) + '.$special['new']['M']['chair'].' (N)', 
								$special['old']['M']['backrest'].' (O) + '.$special['new']['M']['backrest'].' (N)' );
	$rows_special[] = array( 'Female', $special['old']['F']['chowky'].' (O) + '.$special['new']['F']['chowky'].' (N)',
								$special['old']['F']['chair'].' (O) + '.$special['new']['F']['chair'].' (N)', 
								$special['old']['F']['backrest'].' (O) + '.$special['new']['F']['backrest'].' (N)' );
	$t_chowky = $special['old']['M']['chowky'] + $special['new']['M']['chowky'] + $special['old']['F']['chowky'] + $special['new']['F']['chowky'];
	$t_chair = $special['old']['M']['chair'] + $special['new']['M']['chair'] + $special['old']['F']['chair'] + $special['new']['F']['chair'];
	$t_backrest = $special['old']['M']['backrest'] + $special['new']['M']['backrest'] + $special['old']['F']['backrest'] + $special['new']['F']['backrest'];
	$rows_special[] = array( '<b>Total</b>', '<b>'.$t_chowky.'<b>', '<b>'.$t_chair.'</b>', "<b>$t_backrest</b>" );

    $group_teachers = db_query(
        "select ct_group,
        GROUP_CONCAT(concat(t_f_name, ' ', t_l_name) SEPARATOR ' | ')
        FROM `dh_course_teacher`
        left join dh_teacher on t_id=ct_teacher
        where ct_course=$course_id
        and ct_status='confirmed'
        group by ct_group"
    )->fetchAllKeyed();
	ksort($group);
	$header_group = array('Group No', 'Teachers', 'NM', 'OM', 'Total', 'SM', '&nbsp;&nbsp;', 'NF', 'OF', 'Total', 'SF');
	$rows_group = array(); $row = array();
	foreach( $group as $g_no => $v )
	{
		if ( ! isset($group[$g_no]['old']['M']['Student']) ) $group[$g_no]['old']['M']['Student'] = 0;
		if ( ! isset($group[$g_no]['new']['M']['Student']) ) $group[$g_no]['new']['M']['Student'] = 0;
		if ( ! isset($group[$g_no]['old']['F']['Student']) ) $group[$g_no]['old']['F']['Student'] = 0;
		if ( ! isset($group[$g_no]['new']['F']['Student']) ) $group[$g_no]['new']['F']['Student'] = 0;

        if ( ! isset($group[$g_no]['old']['M']['Sevak']) ) $group[$g_no]['old']['M']['Sevak'] = 0;
        if ( ! isset($group[$g_no]['new']['M']['Sevak']) ) $group[$g_no]['new']['M']['Sevak'] = 0;
        if ( ! isset($group[$g_no]['old']['F']['Sevak']) ) $group[$g_no]['old']['F']['Sevak'] = 0;
        if ( ! isset($group[$g_no]['new']['F']['Sevak']) ) $group[$g_no]['new']['F']['Sevak'] = 0;

		unset($row);
		$row[] = '<b>'.$g_no.'</b>';
        $row[] = '<b>'.$group_teachers[$g_no].'</b>';
		$row[] = $group[$g_no]['new']['M']['Student'];
		$row[] = $group[$g_no]['old']['M']['Student'];
		$row[] = '<b>'.($group[$g_no]['old']['M']['Student'] + $group[$g_no]['new']['M']['Student']).'</b>';
        $row[] = '<b>'.($group[$g_no]['old']['M']['Sevak'] + $group[$g_no]['new']['M']['Sevak']).'</b>';
		$row[] = '&nbsp;&nbsp;';
		$row[] = $group[$g_no]['new']['F']['Student'];
		$row[] = $group[$g_no]['old']['F']['Student'];
		$row[] = '<b>'.($group[$g_no]['old']['F']['Student'] + $group[$g_no]['new']['F']['Student']).'</b>';
        $row[] = '<b>'.($group[$g_no]['old']['F']['Sevak'] + $group[$g_no]['new']['F']['Sevak']).'</b>';
		$rows_group[] = $row;
	}


	$out .= '<div id="day-summary">';
	$out .= theme('table', array('header' => $header_totals, 'rows' => $rows_conf, 'attributes' => array('id' => 'table-conf') ));
	$out .= theme('table', array('header' => $header_totals, 'rows' => $rows_totals, 'attributes' => array('id' => 'table-totals') ));
	$out .= theme('table', array('header' => $header_special, 'rows' => $rows_special, 'attributes' => array('id' => 'table-special') ));
	if ( count($group) > 1 )
	{
		$out .= theme('table', array('header' => $header_group, 'rows' => $rows_group, 'attributes' => array('id' => 'table-group') ));
	}
	$out .= '</div><br />';
	if (count($rows) == 0 )
    $attributes = array('id' => 'table-attending-empty');
  $out .= "<br><h2>Attended Applicants</h2>";
	$out .= theme('table', array('header' => $header, 'rows' => $rows, 'attributes' => $attributes));
	return $out;
}

function dh_generate_teacher_list($centre, $course, $seating, $pdf = '')
{
	global $base_url;
	$is_pdf = ($pdf <> '');
	$out = '<style>@import url("'.$base_url.'/sites/all/modules/dh_manageapp/css/teacher-list-v2.css");</style>';

	$q = "select ct_type, ct_group, t_code, CONCAT(t_f_name, ' ',t_l_name) as 't_name', t_gender from dh_course_teacher ct left join dh_teacher t on ct_teacher=t_id where ct_course='$course' and ct_status='Confirmed'";
	$t_hand = db_query($q);
	$teacher = array();
	while($t_r = $t_hand->fetchAssoc())
		$teacher[strtoupper($t_r['t_gender'])][$t_r['ct_group']] = $t_r;
	if (!isset($teacher['M']))
		$teacher['M'][1] = isset($teacher['F']) ? $teacher['F'][1] : array('t_name' => '', 't_code' => '');
	if (!isset($teacher['F']))
		$teacher['F'][1] = $teacher['M'][1];

	$orderby = " order by a_gender, aa_group, a_type desc, IFNULL(ac_teacher,0) desc, IFNULL(t_bhante,0) desc, IFNULL(t_full_t,0) desc, IFNULL(t_sat,0) desc, IFNULL(t_year_appointed,".date("Y").") , IFNULL(ac_60d,0) desc, IFNULL(ac_45d,0) desc, IFNULL(ac_30d,0) desc, IFNULL(ac_20d,0) desc, IFNULL(ac_tsc,0) desc, IFNULL(ac_spl,0) desc, IFNULL(ac_stp,0) desc, IFNULL(ac_10d,0) desc, IFNULL(ac_teen,0) desc, TIMESTAMPDIFF( YEAR, a_dob, CURDATE() ) desc ";
	if ($seating)
		$orderby = " order by a_gender, aa_group, a_type desc, aa_chowky_chair desc, aa_seat_row asc, aa_seat_col asc ";

	$course_name = db_query("select c_name from dh_course where c_id=$course")->fetchField();
	$q = "select a_id, a_gender, a_old, a_f_name, a_m_name, a_l_name, concat(a_f_name, ' ',a_m_name, ' ',a_l_name) as 'Full Name', TIMESTAMPDIFF( YEAR, a_dob, CURDATE() ) as 'Age', ac_teen, ac_10d, ac_stp, ac_spl, ac_20d, ac_30d, ac_45d, ac_60d, ac_tsc, ac_teacher, aa_group, aa_acco, aa_section, aa_laundry, GREATEST(aa_chowky, aa_chair) as aa_chowky_chair, aa_valuable, aa_chowky, aa_chair, aa_backrest, a_occupation, a_education, a_lang_discourse, a_lang_1, a_lang_2, a_langs, aa_comment, aa_cell, aa_cell_group, aa_seat, IF(a_city is null, a_city_str, c_name) as 'City', ae_pregnant, ae_pregnant_detail, ae_desc_physical, ae_desc_mental, ae_desc_addiction_past, ae_desc_addiction_current, ae_desc_other_technique, ae_desc_other_technique_old, ae_desc_medication, ae_nationality, a_type, t_sat, t_full_t, t_bhante, t_year_appointed from dh_applicant left join dh_applicant_course on a_id=ac_applicant left join dh_applicant_attended on a_id=aa_applicant left join dh_applicant_extra ae on a_id=ae_applicant left join dh_city on a_city=c_id left join dh_teacher t on ( ac_teacher_code = CONCAT(t_code,'-',t_gender ))  where a_course='$course' and a_attended=1 $orderby";

	$result = db_query($q);
	$data = array();
	while($r = $result->fetchAssoc())
		$data[strtoupper($r['a_gender'])][$r['a_old']?'old':'new'][$r['aa_group']][] = $r;

	// Column spec: key => [label, class]. Languages / Comments are hide-able.
	$cols = array(
		'sn'       => array('S/N', 'tl-sn'),
		'student'  => array('Student', 'tl-student'),
		'room'     => array('Room', 'tl-room'),
		'age'      => array('Age', 'tl-age'),
		'city'     => array('City', 'tl-city'),
		'courses'  => array('Courses', 'tl-courses'),
		'cell'     => array('Cell', 'tl-cell'),
		'seat'     => array('Seat', 'tl-seat'),
		'occ'      => array('Occupation', 'tl-occ'),
		'edu'      => array('Education', 'tl-edu'),
		'langs'    => array('Languages', 'tl-langs'),
		'comments' => array('Comments', 'tl-comments'),
	);
	$toggle = array('langs' => 'Languages', 'comments' => 'Comments');
	$ncol = count($cols);

	$out .= '<div class="header-teacher"><div class="title">Teacher List</div>'.
		'<div class="title-head">'.check_plain($course_name).'</div></div>';

	// Screen-only toolbar: seniority/seating sort toggle + column hide/show.
	if (!$is_pdf)
	{
		$base = $base_url."/teacher-list/$centre/$course";
		$out .= '<div class="tl-toolbar no-print">';
		$out .= '<span class="grp"><b>Order:</b> '
			.'<a class="'.($seating?'':'active').'" href="'.$base.'">Seniority</a> &middot; '
			.'<a class="'.($seating?'active':'').'" href="'.$base.'?seating=1">Seating plan</a></span>';
		$out .= '<span class="grp"><b>Columns:</b>';
		foreach ($toggle as $key => $lbl)
			$out .= ' <button type="button" class="col-toggle" data-toggle-col="'.$key.'">'.check_plain($lbl).'</button>';
		$out .= '</span></div>';
	}

	$gender = array('M', 'F');
	$type = array('old', 'new');
	$first_block = true;

	foreach($gender as $g)
	{
		foreach ($type as $t)
		{
			if (!isset($data[$g][$t]))
				continue;

			foreach ($data[$g][$t] as $group => $group_data)
			{
				if (empty($group_data))
					continue;
				$total = count($group_data);

				// Each AT group starts on its own printed page.
				$brk = $first_block ? '' : ' tl-break';
				$first_block = false;

				$at = isset($teacher[$g][$group]) ? $teacher[$g][$group] : array('t_name' => '', 't_code' => '');
				$at_name = trim($at['t_name']) !== '' ? check_plain(dh_day0_case($at['t_name'])).' ['.check_plain($at['t_code']).']' : '(unassigned)';

				$out .= '<div class="tl-block'.$brk.'"><div class="tl-scroll"><table class="table-teacher-list"><colgroup>';
				foreach ($cols as $c)
					$out .= '<col class="'.$c[1].'">';
				$out .= '</colgroup><thead>';
				$out .= '<tr><th class="tl-groupinfo" colspan="'.$ncol.'">'
					.'<span class="at">AT: '.$at_name.'</span><span class="sep">|</span>'
					.(($g=='M')?'Male':'Female').'<span class="sep">|</span>'.ucfirst($t)
					.'<span class="sep">|</span>Group '.check_plain($group)
					.'<span class="sep">|</span><span class="cnt">'.$total.' total</span></th></tr>';
				$out .= '<tr>';
				foreach ($cols as $key => $c)
				{
					$lbl = check_plain($c[0]);
					if (isset($toggle[$key]))
						$lbl = '<span class="tlc">'.$lbl.'</span>';
					$out .= '<th class="'.$c[1].'">'.$lbl.'</th>';
				}
				$out .= '</tr></thead><tbody>';

				$i = 1;
				foreach ($group_data as $r)
				{
					// Student name (title-cased, NULL-safe) + teacher/sevak suffix.
					$name = check_plain(dh_day0_name($r));
					if ($r['ac_teacher'])
					{
						if ($r['t_bhante'])       $code = 'BT';
						elseif ($r['t_full_t'])   $code = 'T';
						elseif ($r['t_sat'])      $code = 'SAT';
						else                      $code = 'AT';
						$yr = trim($r['t_year_appointed']);
						$sfx = $code.($yr !== '' ? '-'.$yr : '');   // no dangling dash when year is blank
						$name .= ' <b>('.check_plain($sfx).')</b>';
					}
					if (strtolower($r['a_type']) == 'sevak')
						$name .= ' <b>(Sevak)</b>';

					$courses = '';
					foreach (array('ac_10d'=>'10D','ac_stp'=>'STP','ac_spl'=>'SPL','ac_tsc'=>'TSC','ac_20d'=>'20D','ac_30d'=>'30D','ac_45d'=>'45D','ac_60d'=>'60D') as $f => $cl)
						if (!empty($r[$f]))
							$courses .= '<b>'.$cl.':</b>'.check_plain($r[$f]).' ';

					if ($r['aa_cell_group'] !== null && $r['aa_cell_group'] >= 1)
						$cellno = check_plain($r['aa_cell'].' ('.($r['aa_cell_group']+1).')');
					else
						$cellno = check_plain($r['aa_cell']);

					// Comments: keep labels, sentence-case the shouty free-text pieces.
					$cm = '';
					if (trim($r['ae_nationality']) !== '')
						$cm .= 'Nationality: '.check_plain(dh_day0_case(trim($r['ae_nationality']))).'<br>';
					if ($g == 'F' && $r['ae_pregnant'])
						$cm .= 'Pregnant: '.check_plain($r['ae_pregnant_detail']).' ';
					$pieces = array();
					foreach (array('ae_desc_physical','ae_desc_mental','ae_desc_addiction_past','ae_desc_addiction_current','ae_desc_medication','ae_desc_other_technique') as $f)
						if (trim($r[$f]) !== '')
							$pieces[] = check_plain(dh_day0_sentence(trim($r[$f])));
					if (trim($r['aa_comment']) !== '')
						$pieces[] = check_plain(dh_day0_sentence(trim($r['aa_comment'])));
					$cm .= implode(' ', $pieces);

					$cell = array(
						'sn'       => $i++,
						'student'  => $name,
						'room'     => check_plain(trim($r['aa_section'].'-'.$r['aa_acco'], '-')),
						'age'      => check_plain($r['Age']),
						'city'     => check_plain(dh_day0_case($r['City'])),
						'courses'  => trim($courses),
						'cell'     => $cellno,
						'seat'     => check_plain($r['aa_seat']).(!empty($r['aa_backrest']) ? '<span class="tl-br" title="Backrest">BR</span>' : ''),
						'occ'      => check_plain(dh_day0_case($r['a_occupation'])),
						'edu'      => check_plain(trim($r['a_education'])),
						'langs'    => check_plain(dh_day0_langs($r['a_lang_discourse'].','.$r['a_langs'])),
						'comments' => $cm,
					);

					$out .= '<tr>';
					foreach ($cols as $key => $c)
					{
						$v = isset($cell[$key]) ? $cell[$key] : '';
						if (isset($toggle[$key]))
							$v = '<span class="tlc">'.$v.'</span>';
						$out .= '<td class="'.$c[1].'">'.$v.'</td>';
					}
					$out .= '</tr>';
				}
				$out .= '</tbody></table></div></div>';
			}
		}
	}

	if ($is_pdf)
	{
		create_pdf_from_html($out, $pdf);
		return;
	}

	// Self-contained column hide/show (no jQuery). Collapses the <col> width and
	// hides the cell-content wrapper so no width or row-height is reserved.
	$out .= '<script>
document.addEventListener("click", function(e){
	var b = e.target.closest ? e.target.closest("[data-toggle-col]") : null;
	if (!b) return;
	var hide = b.classList.toggle("off");
	var els = document.querySelectorAll(".tl-" + b.getAttribute("data-toggle-col"));
	for (var i = 0; i < els.length; i++) {
		var el = els[i];
		if (el.tagName === "COL") { el.style.width = hide ? "0" : ""; continue; }
		el.style.paddingLeft = el.style.paddingRight = hide ? "0" : "";
		var w = el.querySelector(".tlc");
		if (w) w.style.display = hide ? "none" : "";
	}
});
</script>';
	print $out;
	exit;
}


function dh_generate_manager_list($centre, $course, $pdf = '')
{
	global $base_url;
	$is_pdf = ($pdf <> '');
	$out = '<style>@import url("'.$base_url.'/sites/all/modules/dh_manageapp/css/manager-list-v2.css");</style>';

	$q = "select ct_type, ct_group, t_code, CONCAT(t_f_name, ' ',t_l_name) as 't_name', t_gender from dh_course_teacher ct left join dh_teacher t on ct_teacher=t_id where ct_course='$course' and ct_status='Confirmed'";
	$t_hand = db_query($q);
	$teacher_m = array();
	while($t_r = $t_hand->fetchAssoc())
		$teacher_m[strtoupper($t_r['t_gender'])][$t_r['ct_group']] = $t_r;
	if (!isset($teacher_m['F']) && isset($teacher_m['M']))
		$teacher_m['F'][1] = $teacher_m['M'][1];

	// Per-gender AT list (shown above each block).
	$t_info = array();
	foreach (array('M', 'F') as $g)
	{
		if (!isset($teacher_m[$g]))
			continue;
		$parts = array();
		foreach ($teacher_m[$g] as $value)
			$parts[] = '<span class="at">'.check_plain(dh_day0_case($value['t_name'])).' <span class="code">['.check_plain($value['t_code']).']</span></span>';
		$t_info[$g] = '<div class="ml-teachers"><span class="lbl">Teachers</span>'.implode('', $parts).'</div>';
	}

	$course_name = db_query("select c_name from dh_course where c_id=$course")->fetchField();
	$q = "select a_id, a_gender, a_old, a_type, a_f_name, a_m_name, a_l_name, concat(a_f_name, ' ',a_m_name, ' ',a_l_name) as 'Full Name', TIMESTAMPDIFF( YEAR, a_dob, CURDATE() ) as 'Age', aa_group, aa_acco, aa_section, aa_laundry, aa_chowky, aa_valuable, aa_chowky, aa_chair, aa_backrest, a_emergency_name, a_emergency_relation, a_emergency_num, a_langs, aa_comment, aa_cell, aa_cell_group, aa_seat, aa_seat_row, aa_seat_col, c_name, a_conf_no from dh_applicant left join dh_applicant_attended on a_id=aa_applicant left join dh_city on a_city=c_id where a_course='$course' and a_attended=1 order by a_type desc, a_f_name, a_m_name, a_l_name";
	$result = db_query($q);
	$data = array();
	while($r = $result->fetchAssoc())
		$data[strtoupper($r['a_gender'])][] = $r;

	$cols = array(
		'sn'      => array('S/N', 'ml-sn'),
		'student' => array('Student', 'ml-student'),
		'age'     => array('Age', 'ml-age'),
		'room'    => array('Room', 'ml-room'),
		'ename'   => array('Emergency Contact', 'ml-ename'),
		'enum'    => array('Emergency No.', 'ml-enum'),
		'cell'    => array('Cell', 'ml-cell'),
		'seat'    => array('Seat', 'ml-seat'),
		'conf'    => array('Conf No', 'ml-conf'),
		'set'     => array('Set', 'ml-set'),
	);
	$ncol = count($cols);

	$out .= '<div class="header-manager"><div class="title">Manager List</div>'.
		'<div class="title-head">'.check_plain($course_name).'</div></div>';

	$first_block = true;
	foreach (array('M', 'F') as $g)
	{
		if (empty($data[$g]))
			continue;
		$total = count($data[$g]);
		$brk = $first_block ? '' : ' ml-break';
		$first_block = false;

		$out .= '<div class="ml-block'.$brk.'"><div class="ml-scroll"><table class="table-manager-list"><colgroup>';
		foreach ($cols as $c)
			$out .= '<col class="'.$c[1].'">';
		$out .= '</colgroup><thead>';
		$out .= '<tr><th class="ml-groupinfo" colspan="'.$ncol.'">'
			.(($g=='M')?'Male':'Female').'<span class="sep">|</span><span class="cnt">'.$total.' total</span></th></tr>';
		if (!empty($t_info[$g]))
			$out .= '<tr><th class="ml-groupinfo" colspan="'.$ncol.'" style="padding-top:0;font-weight:normal">'.$t_info[$g].'</th></tr>';
		$out .= '<tr>';
		foreach ($cols as $c)
			$out .= '<th class="'.$c[1].'">'.check_plain($c[0]).'</th>';
		$out .= '</tr></thead><tbody>';

		$i = 1;
		foreach ($data[$g] as $r)
		{
			$name = check_plain(dh_day0_name($r));
			if ($r['a_type'] == 'Sevak')
				$name .= ' <b>(Sevak)</b>';

			$en = trim($r['a_emergency_name']);
			$er = trim($r['a_emergency_relation']);
			// Relation words (Son/Wife/Brother...) are always safe to title-case.
			$ename = ($en !== '') ? check_plain(dh_day0_case($en)).($er !== '' ? ' ('.check_plain(mb_convert_case($er, MB_CASE_TITLE, 'UTF-8')).')' : '') : '';

			if ($r['aa_cell_group'] !== null && $r['aa_cell_group'] >= 1)
				$cellno = check_plain($r['aa_cell'].' ('.($r['aa_cell_group']+1).')');
			else
				$cellno = check_plain($r['aa_cell']);

			$code = isset($teacher_m[$g][$r['aa_group']]['t_code']) ? trim($teacher_m[$g][$r['aa_group']]['t_code']) : '';

			$cell = array(
				'sn'      => $i++,
				'student' => $name,
				'age'     => check_plain($r['Age']),
				'room'    => check_plain(trim($r['aa_section'].'-'.$r['aa_acco'], '-')),
				'ename'   => $ename,
				'enum'    => check_plain($r['a_emergency_num']),
				'cell'    => $cellno,
				'seat'    => check_plain($r['aa_seat']).(!empty($r['aa_backrest']) ? '<span class="ml-br" title="Backrest">BR</span>' : ''),
				'conf'    => check_plain($r['a_conf_no']),
				'set'     => $code !== '' ? check_plain($code) : '',
			);

			$out .= '<tr>';
			foreach ($cols as $key => $c)
				$out .= '<td class="'.$c[1].'">'.(isset($cell[$key]) ? $cell[$key] : '').'</td>';
			$out .= '</tr>';
		}
		$out .= '</tbody></table></div></div>';
	}

	if ($is_pdf)
	{
		create_pdf_from_html($out, $pdf);
		return;
	}
	print $out;
	exit;
}

/**
 * Resolve where a gender's chowky block sits: 'left', 'right' or 'back'.
 * $cfgval is the configured ChowkyPosition (may be empty). When unset, keep the
 * historical behaviour: the side implied by the seat direction.
 */
function dh_chowky_position($cfgval, $direction)
{
	$p = strtolower(trim((string)$cfgval));
	if ($p == 'left' || $p == 'right' || $p == 'back')
		return $p;
	return ($direction == LEFT_TO_RIGHT) ? 'right' : 'left';
}


/**
 * Resolve a gender's seat direction from config: 'right' aligns seats to the
 * right (LEFT_TO_RIGHT), 'left' to the left (RIGHT_TO_LEFT). Anything else keeps
 * $default_dir (the historical direction implied by which side the gender is on).
 */
function dh_seat_direction($cfgval, $default_dir)
{
	$v = strtolower(trim((string)$cfgval));
	if ($v == 'right') return LEFT_TO_RIGHT;
	if ($v == 'left')  return RIGHT_TO_LEFT;
	return $default_dir;
}


/**
 * Assemble the seats + chowky blocks for one gender according to $position:
 * 'back' puts chowky as a full-width block above the seats; 'left'/'right' keep
 * it as a side column.
 */
function dh_layout_seats_chowky($out_seats, $out_chowky, $position)
{
	if ($position == 'back')
		return '<div class="chowky-back">'.$out_chowky.'</div>'.$out_seats;
	if ($position == 'left')
		return $out_chowky.$out_seats;
	return $out_seats.$out_chowky;
}


/**
 * Validate a Seating Configuration string. Returns an array of human-readable
 * problems (empty array = OK). Only clearly-wrong values are flagged; unknown
 * keys are ignored so existing configs keep working.
 */
function dh_validate_seat_config($settings)
{
	if (!is_string($settings) || trim($settings) === '')
		return array('The Seating Configuration is empty.');
	$ini = @parse_ini_string($settings, true, INI_SCANNER_RAW);
	if (!is_array($ini))
		return array('The Seating Configuration could not be read. Check the format: [RIGHT] / [LEFT] sections with "key = value" lines.');

	$sections = array();
	foreach ($ini as $s => $keys)
		$sections[strtoupper($s)] = $keys;

	$errors = array();
	if (!isset($sections['LEFT']) && !isset($sections['RIGHT']))
		$errors[] = 'Missing [RIGHT] (Male) and/or [LEFT] (Female) section.';

	foreach ($sections as $sname => $keys)
	{
		if (!is_array($keys)) continue;
		foreach ($keys as $k => $v)
		{
			// Classify by the key name, ignoring any GROUP<n>- prefix. Only the
			// placement keys are value-checked: numeric keys (SeatsPerRow etc.) are
			// left alone because the plans already tolerate 0 / blank (they clamp),
			// and many existing centres rely on that.
			$base = $k;
			if (preg_match('/^group\s*[0-9a-z]+-(.+)$/i', $k, $m)) $base = $m[1];
			$bl = strtolower(trim($base));
			$vt = trim((string)$v);
			if ($bl === 'chowkyposition')
			{
				if (!in_array(strtolower($vt), array('left', 'right', 'back')))
					$errors[] = "[$sname] $k must be left, right or back (got \"$vt\").";
			}
			elseif ($bl === 'seatdirection')
			{
				if (!in_array(strtolower($vt), array('left', 'right')))
					$errors[] = "[$sname] $k must be left or right (got \"$vt\").";
			}
		}
	}

	// Each side that is defined must carry an (un-prefixed) SeatsPerRow line.
	// This catches typos like "SeatsPerRw" that would otherwise be ignored.
	foreach (array('RIGHT', 'LEFT') as $sn)
	{
		if (!isset($sections[$sn]) || !is_array($sections[$sn])) continue;
		$has_spr = false;
		foreach ($sections[$sn] as $k => $v)
		{
			if (preg_match('/^group/i', $k)) continue;   // ignore GROUP<n>- keys
			if (strtolower(trim($k)) === 'seatsperrow') { $has_spr = true; break; }
		}
		if (!$has_spr)
			$errors[] = "[$sn] has no SeatsPerRow line (each side needs one - check for a typo in the key name).";
	}

	return $errors;
}


/**
 * Print (or PDF) a "settings are not correct" page listing the problems, then
 * exit. Used by the seating plans when dh_validate_seat_config() fails.
 */
function dh_seat_config_error_output($centre, $errors, $pdf = '')
{
	global $base_url;
	$out = '<html><head>';
	$out .= '<style>@import url("'.$base_url.'/sites/all/modules/dh_manageapp/css/seating.css?v='.filemtime(drupal_get_path("module","dh_manageapp")."/css/seating.css").'");</style>';
	$out .= '</head><body>';
	$out .= '<div class="helptext">';
	$out .= '<h2>Seating settings are not correct</h2>';
	$out .= '<p>Please correct the <b>Seating Configuration</b> in '.l('Center Settings', 'centre/'.$centre.'/edit').', then open the seating plan again.</p>';
	if (!empty($errors))
	{
		$out .= '<ul>';
		foreach ($errors as $e)
			$out .= '<li>'.check_plain($e).'</li>';
		$out .= '</ul>';
	}
	$out .= '</div></body></html>';

	if ($pdf <> '')
		create_pdf_from_html($out, $pdf);
	else
	{
		print $out;
		exit;
	}
}


/* The separator shown between the room and the dormitory cell (e.g. F-5 [home] 7):
   a small filled-grey house icon. Inline SVG so it is self-contained and prints
   crisply in black & white. */
function dh_room_cell_sep()
{
	return ' <span class="rc-sep"><svg width="11" height="11" viewBox="0 0 16 16" xmlns="http://www.w3.org/2000/svg">'
		.'<g fill="none" stroke="#555" stroke-width="1.1" stroke-linejoin="round" stroke-linecap="round">'
		.'<path d="M1.7 8.3 L8 2.8 L14.3 8.3"/><path d="M3.5 8 V13.5 H12.5 V8"/>'
		.'<rect x="6.8" y="9.9" width="2.4" height="3.6"/></g></svg></span> ';
}

/* Page header for a seating-plan page: the centered title, flanked by a "field
   key" that reuses the empty space on either side of the title. On the left, a
   tinted sample cell (same classes as a real cell, so it looks identical) shows
   what each position means; on the right, a short decoder for the codes. The
   legend is outside any .sortable/.bg-table grid, so the plan JS never treats it
   as a draggable/counted seat. $title_class is 'teacher-seat' (main plan) or
   'group-title' (group plan). */
function dh_seat_plan_header($title_html, $title_class = 'teacher-seat')
{
	$legend = '<div class="seat-legend">'
		.'<div class="ui-state-default legend-cell">'
		.'<div class="s-seat-no"><div class="seat-no"><span class="seatnumber">Seat</span></div></div>'
		.'<div class="s-acco">Room'.dh_room_cell_sep().'Cell</div>'
		.'<div class="s-cell"></div>'
		.'<div class="s-name">Name</div>'
		.'<div class="s-old-student">(O) s:_ L:_</div>'
		.'<div class="s-age">Age</div>'
		.'<div class="s-backrest s-backrest-yes"></div>'
		.'</div></div>';
	$sep = ' <span class="rc-sep">&bull;</span> ';
	$note = '<div class="legend-note">(O) old student'.$sep.'s: short courses'.$sep.'L: long courses'.$sep.'<span class="legend-bar"></span> backrest'.$sep.'<span class="legend-swatch"></span> shaded name = teacher</div>';
	return '<div class="plan-header">'
		.$legend
		.'<div class="'.$title_class.' plan-header-title">'.$title_html.'</div>'
		.$note
		.'</div><div style="clear:both"></div>';
}

function dh_generate_seating_plan($centre, $course, $pdf = '')
{
	global $base_url;
	if (!is_array($course))
		$course = array($course);

	$out = '<html><head><script src="https://code.jquery.com/jquery-1.12.4.js"></script>
  <script src="https://code.jquery.com/ui/1.12.1/jquery-ui.js"></script>
  <script>
  $( function() {

    //$( ".sortable-m" ).sortable({ items: "td" });
    //$( ".sortable-m" ).disableSelection();
    //$( ".sortable-f" ).sortable({ items: "td" });
    //$( ".sortable-f" ).disableSelection();
    //$(".chowky-div").sortable({ items: "td" });
    //$(".chowky-div").disableSelection();

	$(".sortable-m .seatnumber").empty();
	$(".bg-table-m .s-acco").empty();
	$(".bg-table-m .s-name").empty();
	$(".bg-table-m .s-old-student").empty();
	$(".bg-table-m .s-age").empty();
	$(".bg-table-m .s-backrest").empty();
  $(".bg-table-m .s-cell").empty();

	$(".sortable-f .seatnumber").empty();
	$(".bg-table-f .s-acco").empty();
	$(".bg-table-f .s-name").empty();
	$(".bg-table-f .s-old-student").empty();
	$(".bg-table-f .s-age").empty();
	$(".bg-table-f .s-backrest").empty();
  $(".bg-table-f .s-cell").empty();

	$(".bg-table-m .drag-img").empty();
	$(".bg-table-f .drag-img").empty();


	/* Capture each grid column count now, while it is still a table (the first
	   row cell count). After the table becomes a <ul> of floated <li> cells the
	   row structure is gone. The width is applied further down, once the cells
	   are <li>, so it matches their real width exactly. Pinning the width to
	   columns x cell-width (not the whole-grid measured width) keeps the layout
	   stable when the window is narrower than the plan. */
	function dhCols(sel){ var tr = $(sel).find("tr").first(); return tr.children("td").length; }
	var dhColsMM = dhCols(".main-div .sortable-m"),  dhColsMC = dhCols(".chowky-div .sortable-m"),
	    dhColsFM = dhCols(".main-div .sortable-f"),  dhColsFC = dhCols(".chowky-div .sortable-f");

    $(".sortable-m td, .sortable-f td").replaceWith(function(){
      return this.outerHTML.replace("<td", "<li").replace("</td", "</li")
    });
    $(".sortable-m tr, .sortable-f tr").replaceWith(function(){
      return this.outerHTML.replace("<tr>", "").replace("</tr>", "")
    });
    $(".sortable-m tbody, .sortable-f tbody").replaceWith(function(){
      return this.outerHTML.replace("<tbody>", "").replace("</tbody>", "")
    });     
    $(".sortable-m, .sortable-f").replaceWith(function(){
      return this.outerHTML.replace("<table", "<ul").replace("</table", "<ul")
    });

    $(".bg-table-m td, .bg-table-f td").replaceWith(function(){
      return this.outerHTML.replace("<td", "<li").replace("</td", "</li")
    });
    $(".bg-table-m tr, .bg-table-f tr").replaceWith(function(){
      return this.outerHTML.replace("<tr>", "").replace("</tr>", "")
    });
    $(".bg-table-m tbody, .bg-table-f tbody").replaceWith(function(){
      return this.outerHTML.replace("<tbody>", "").replace("</tbody>", "")
    });     
    $(".bg-table-m, .bg-table-f").replaceWith(function(){
      return this.outerHTML.replace("<table", "<ul").replace("</table", "<ul")
    });

	/* Now that every grid is a <ul> of floated <li> cells, pin each to columns x
	   actual cell width. Doing it here (not on the table) uses the real <li>
	   width, so a full row fills the width exactly with no slack - otherwise the
	   slack lands on the left of the right-floated male grid and its left margin
	   no longer matches the left-floated female grid. */
	function dhSetW(sel, cols){ var ul = $(sel), li = ul.find("li.ui-state-default")[0];
	  /* +1px per column: an exactly-fit width makes the last cell wrap when the
	     browser rounds cell widths up a hair (notably on print), dropping a whole
	     column - the small buffer keeps all columns on one row. */
	  if (li && cols) ul.width(cols * (li.getBoundingClientRect().width + 1)); }
	dhSetW(".main-div .sortable-m", dhColsMM);  dhSetW(".chowky-div .sortable-m", dhColsMC);
	dhSetW(".main-div .sortable-f", dhColsFM);  dhSetW(".chowky-div .sortable-f", dhColsFC);
	dhSetW(".main-div .bg-table-m", dhColsMM);  dhSetW(".chowky-div .bg-table-m", dhColsMC);
	dhSetW(".main-div .bg-table-f", dhColsFM);  dhSetW(".chowky-div .bg-table-f", dhColsFC);

    /* Shorten an over-long name at a word boundary: keep as many whole leading
       words as fit, then show the first word that does not fit as just its
       initial + "." (e.g. "Abhimanyu Singh Gaikwad Deshmukh" -> "Abhimanyu
       Singh G."). Chrome has no custom text-overflow string (it would show
       "..."), hence doing it in JS. Full name stays on hover. Run after the grid
       is built so the cell width is final. */
    $(".sortable-m .s-name, .sortable-f .s-name").each(function(){
      var el = this;
      if (el.scrollWidth <= el.clientWidth) return;
      var full = el.textContent.replace(/\s+/g, " ").replace(/^ | $/g, "");
      el.title = full;
      var words = full.split(" "), kept = [];
      for (var i = 0; i < words.length; i++) {
        el.textContent = kept.concat([words[i]]).join(" ");
        if (el.scrollWidth <= el.clientWidth) { kept.push(words[i]); continue; }
        el.textContent = kept.concat([words[i].charAt(0) + "."]).join(" ");
        while (el.scrollWidth > el.clientWidth && kept.length) {
          kept.pop();
          el.textContent = kept.concat([words[i].charAt(0) + "."]).join(" ");
        }
        break;
      }
    });

    $(document).on("click", ".add-row", function(){
		var $div = $(this).closest(".main-div, .chowky-div");
		var $fg = $div.find("ul[class*=sortable]").first();
		var $bg = $div.find("ul[class*=bg-table]").first();
		var cols = parseInt($div.find(".s-app-id[data-cols]").first().attr("data-cols"),10) || parseInt($(this).val(),10);
		for (var i=1 ; i <= cols ; i++)
		{
		    $fg.prepend(\'<li class="ui-state-default"><div class="drag-img no-print"><img src="/sites/all/modules/dh_manageapp/imgs/drag.png" alt="drag"></div></li>\');
	    	$bg.prepend(\'<li class="ui-state-default"></li>\');
		}
		$(".sortable-m .drag-img").hide();
		$(".sortable-f .drag-img").hide();
	});

    original_array = [];

  $(".store-seat-changes").click( function(){
    //console.log("inside click event");
    $(this).off("click");
    $(this).html("Processing... updated seating plan will be loaded soon.").css({
    });
    update_array = [];
    current_path = window.location.pathname;
    update_path = current_path.replace("seating", "seating-update");
    $(".ui-sortable li .s-app-id").each(function(){
      closest_ul = $(this).closest("ul");
      student_name = $(this).siblings(".s-name ").html();
      application_id = $(this).attr("data-s-app-id");
      col_per_row = $(this).attr("data-cols");
      if(application_id)
      {
        no_of_li = closest_ul.children("li").length;
        current_li = $(this).closest("li").index()+1;
        current_position = no_of_li - current_li + 1;
        student_row = Math.ceil(current_position / col_per_row) - 1;
        student_column = (current_position % col_per_row) - 1;
        if(student_column == -1)
          student_column = col_per_row - 1 ;

        // console.log("app_id: "+$(this).attr("data-s-app-id"));
        // console.log("student_name: " + student_name);
        // console.log("col per row: " + col_per_row);
        // console.log("no_of_li: " + no_of_li);
        // console.log("current_li: " + current_li);
        // console.log("current_position: " + current_position);
        //console.log("current_path: " + current_path);
        //console.log("update_path: " + update_path);
        //console.log("name: "+student_name+" id: "+application_id+" row: "+student_row+" column: "+student_column);

        update_array.push([application_id, student_row, student_column]);
      }
    });

    let originalMap = {};
    original_array.forEach(row => {
        originalMap[row[0]] = row;
    });

    let changed = [];

    update_array.forEach(row => {
        let id = row[0];

        if (!originalMap[id] || JSON.stringify(originalMap[id]) !== JSON.stringify(row)) {
            changed.push(row);
        }
    });

    // console.log(changed);

    $.getJSON( update_path, { u: JSON.stringify(changed) } )
        .done(function( json ) {
          if (json.status == "OK")
          {
            setTimeout(function() {
              window.location = current_path;
            }, 5000);
          }
          else
          {
            alert("Update failed!\n"+json.msg);
          }
          })
          .fail(function() {
            alert("Update failed fail!");
          });

      $(".store-seat-changes").on("click");
  });

	$(".sortable-m .drag-img").hide();
	$(".sortable-f .drag-img").hide();


	$(document).on("mouseenter", ".ui-state-default", function(){
			$(this).css({"cursor": "grab"});
	        $(this).find(".drag-img").show();
	});
	$(document).on("mouseleave", ".ui-state-default", function(){
	        $(this).find(".drag-img").hide();
	        $(this).css({"cursor": "default"});
	});

	$(document).on("mousedown", ".ui-state-default", function(){
			$(this).css({"cursor": "move"});
	});
	$(document).on("mouseup", ".ui-state-default", function(){
	        $(this).css({"cursor": "grab"});
	});

    $(".sortable-m").sortable();
    $(".sortable-m").disableSelection();

    $(".sortable-f").sortable();
    $(".sortable-f").disableSelection();


    $(".ui-sortable li .s-app-id").each(function(){
      closest_ul = $(this).closest("ul");
      student_name = $(this).siblings(".s-name ").html();
      application_id = $(this).attr("data-s-app-id");
      col_per_row = $(this).attr("data-cols");
      if(application_id)
      {
        no_of_li = closest_ul.children("li").length;
        current_li = $(this).closest("li").index()+1;
        current_position = no_of_li - current_li + 1;
        student_row = Math.ceil(current_position / col_per_row) - 1;
        student_column = (current_position % col_per_row) - 1;
        if(student_column == -1)
          student_column = col_per_row - 1 ;

        original_array.push([application_id, student_row, student_column]);
      }
    });
    // console.log(original_array);

  } );
  </script>
';

	$settings = db_query("select cs_seat_config from dh_center_setting where cs_center=$centre")->fetchField();
	$config_errors = dh_validate_seat_config($settings);
	if (!empty($config_errors))
		return dh_seat_config_error_output($centre, $config_errors, $pdf);
	$alpha = db_query("select cs_seat_naming_conv from dh_center_setting where cs_center=$centre")->fetchField();
	$ini = array();
	if ($settings <> '')
		$ini = parse_ini_string($settings, true, INI_SCANNER_RAW);
	if (!isset($ini['LEFT']['gender']))
		$ini['LEFT']['gender'] = 'F';
	if (!isset($ini['RIGHT']['gender']))
		$ini['RIGHT']['gender'] = 'M';

	$out .= '<style>@import url("'.$base_url.'/sites/all/modules/dh_manageapp/css/seating.css?v='.filemtime(drupal_get_path("module","dh_manageapp")."/css/seating.css").'");</style></head><body>';
	$out .= '<div class="helptext no-print">
		<h2>Editing Seating Plan:</h2>
		<p>Male and Female each on their own page, laid out from the hall dimensions in the centre\'s Seating Configuration (the [RIGHT] / [LEFT] section keys). "Regenerate" re-packs all seats from the configuration.</p>
		<p>1- Check basic Hall Settings in <a href="/wiki-centre-settings">Center Settings</a>.</p>
		<p>2- Rearrange the plan by dragging and dropping student seats in grid.</p>
		<p>3- Create empty rows for a grid by clicking "+ Blank Row" button if needed.</p>
		<p>4- Adjust the columns of a grid: "+ Col" adds a column and re-flows the students to fill it; "&minus; Col" removes a column and re-flows; "+ Blank Col" inserts an empty column to drag students into. (Take effect after "Store Seat Changes".)</p>
    <p>5- Save Sitting arrangement changes by clicking "Store Seat Changes" button.</p>
		<p>6- Print by clicking "Print Seating Plan" button or CTL-P.</p>
		<p>7- Adjust print settings(Layout, Scale, Paper Size) as needed and print the seating plan.</p>
		<p>Note: All help text and buttons will be ignored on print.</p>
		</div>
		<button class="no-print" onclick="window.print()">Print Seating Plan</button>
    <button type="button" class="no-print store-seat-changes">Store Seat Changes</button>';
	if ( is_array($course) )
	{
		$course_name = '';
		$h = db_query("select c_name from dh_course where c_id in (".implode(",", $course).")");
		while($r = $h->fetchAssoc())
			$course_name .= $r['c_name']." / ";
	}

	$q = "select a_id, aa_id, a_gender, a_old,  concat(a_f_name, ' ',a_l_name) as 'Full Name', TIMESTAMPDIFF( YEAR, a_dob, CURDATE() ) as 'Age', ac_teen, ac_10d, ac_stp, ac_spl, ac_20d, ac_30d, ac_45d, ac_60d, ac_tsc, ac_teacher, aa_cell, aa_group, aa_acco, aa_section, aa_laundry, aa_valuable, aa_chowky, aa_chair, aa_backrest, aa_seat_row, aa_seat_col  from dh_applicant left join dh_applicant_course on a_id=ac_applicant left join dh_applicant_attended on a_id=aa_applicant  left join dh_teacher t on ( ac_teacher_code = CONCAT(t_code,'-',t_gender )) where a_course in (".implode(",", $course).") and a_attended=1 and a_type='Student' order by  a_gender, IFNULL(ac_teacher,0) desc, IFNULL(t_seniority,0) desc, IFNULL(t_year_appointed,".date("Y").") , IFNULL(ac_60d,0) desc, IFNULL(ac_45d,0) desc, IFNULL(ac_30d,0) desc, IFNULL(ac_20d,0) desc, IFNULL(ac_tsc,0) desc, IFNULL(ac_spl,0) desc, IFNULL(ac_stp,0) desc, IFNULL(ac_10d,0) desc, IFNULL(ac_teen,0) desc, IFNULL(ac_service,0) desc, TIMESTAMPDIFF( YEAR, a_dob, CURDATE() ) desc";
	$result = db_query($q);
	$seats['chowky'] = $seats['seats'] = array();
	$max_col_chowky['M'] = 0; $max_col_chowky['F'] = 0; $max_col['M'] = 0; $max_col['F'] = 0;
	$counter = array();
	while($r = $result->fetchAssoc())
	{
		if ($r['aa_chowky'] || $r['aa_chair'])
		{
			if ( $r['aa_seat_col'] > $max_col_chowky[strtoupper($r['a_gender'])] )
				$max_col_chowky[strtoupper($r['a_gender'])] = $r['aa_seat_col'];
			if (!isset($counter[strtoupper($r['a_gender'])."-ch"]))
				$counter[strtoupper($r['a_gender'])."-ch"] = 1;
			$r['counter'] = $counter[strtoupper($r['a_gender'])."-ch"];
			$counter[strtoupper($r['a_gender'])."-ch"]++;
			$seats[strtoupper($r['a_gender'])]['chowky'][$r['aa_seat_row']][$r['aa_seat_col']] = $r;			
		}
		else
		{
			if ( $r['aa_seat_col'] > $max_col[strtoupper($r['a_gender'])] )
				$max_col[strtoupper($r['a_gender'])] = $r['aa_seat_col'];
			if (!isset($counter[strtoupper($r['a_gender'])."-seat"]))
				$counter[strtoupper($r['a_gender'])."-seat"] = 1;
			$r['counter'] = $counter[strtoupper($r['a_gender'])."-seat"];
			$counter[strtoupper($r['a_gender'])."-seat"]++;
			$seats[strtoupper($r['a_gender'])]['seats'][$r['aa_seat_row']][$r['aa_seat_col']] = $r;			
		}
	}

	$place = 'RIGHT';

	if ($ini['RIGHT']['gender'] == 'M')
		$male_dir = LEFT_TO_RIGHT;
	else
		$male_dir = RIGHT_TO_LEFT;
	$male_section = ($ini['RIGHT']['gender'] == 'M') ? 'RIGHT' : 'LEFT';
	$male_dir = dh_seat_direction(isset($ini[$male_section]['SeatDirection']) ? $ini[$male_section]['SeatDirection'] : '', $male_dir);
	$male_chowky_pos = dh_chowky_position(isset($ini[$male_section]['ChowkyPosition']) ? $ini[$male_section]['ChowkyPosition'] : '', $male_dir);

	$out .= '<div class="dh-page-sep no-print"></div>';   // screen-only gap before the male page
	$out .= dh_seat_plan_header('Seating Plan - MALE<br>'.$course_name);
	$out_seats = dh_print_seats($seats['M']['seats'], $max_col['M']+1, $male_dir, 'main-div', $alpha);
	$out_chowky = dh_print_seats($seats['M']['chowky'], $max_col_chowky['M']+1, $male_dir, 'chowky-div', $alpha);
	$out .= dh_layout_seats_chowky($out_seats, $out_chowky, $male_chowky_pos);

	$out .= '<div class="teacher-seat">TEACHER</div>';
	$out .= '<div class="footer"></div>';

	if ($ini['LEFT']['gender'] == 'F')
		$female_dir = RIGHT_TO_LEFT;
	else
		$female_dir = LEFT_TO_RIGHT;
	$female_section = ($ini['LEFT']['gender'] == 'F') ? 'LEFT' : 'RIGHT';
	$female_dir = dh_seat_direction(isset($ini[$female_section]['SeatDirection']) ? $ini[$female_section]['SeatDirection'] : '', $female_dir);
	$female_chowky_pos = dh_chowky_position(isset($ini[$female_section]['ChowkyPosition']) ? $ini[$female_section]['ChowkyPosition'] : '', $female_dir);

	$place = 'LEFT';

	$out .= '<div class="dh-page-sep no-print"></div>';   // screen-only gap before the female page
	$out .= dh_seat_plan_header('Seating Plan - FEMALE<br>'.$course_name);
	$out_chowky = dh_print_seats($seats['F']['chowky'], $max_col_chowky['F']+1, $female_dir, 'chowky-div', $alpha);
	$out_seats = dh_print_seats($seats['F']['seats'], $max_col['F']+1, $female_dir, 'main-div', $alpha);
	$out .= dh_layout_seats_chowky($out_seats, $out_chowky, $female_chowky_pos);

	$out .= '<div class="teacher-seat">TEACHER</div>';
	$out .= '<div class="footer"></div>';

	$out .= '<script>
(function($){
  var DH_CONV = '.(int)$alpha.';
  function dhLetter(n){ n=parseInt(n,10); var s=""; do{ s=String.fromCharCode(65+(n%26))+s; n=Math.floor(n/26)-1; }while(n>=0); return s; }
  function dhLabel(col,row){ return (DH_CONV==2) ? (dhLetter(row)+(col+1)) : (dhLetter(col)+(row+1)); }
  function dhCellW($ul){ var li=$ul.children("li")[0]; return li ? li.getBoundingClientRect().width : 142; }
  function dhUls($div){ return { fg: $div.find("ul[class*=sortable]").first(), bg: $div.find("ul[class*=bg-table]").first() }; }
  function dhCurCols($div){ var v=$div.find(".s-app-id[data-cols]").first().attr("data-cols"); if(v) return parseInt(v,10); return parseInt($div.find(".add-row").first().val(),10)||1; }
  function dhBgCell(label,pfx){ var p=pfx?("<span class=\"cwch\">"+pfx+"</span>"):""; return "<li class=\"ui-state-default\"><div class=\"s-seat-no\"><div class=\"seat-no\">"+p+"<span class=\"seatnumber\">"+label+"</span></div></div></li>"; }
  function dhRelabelBg($bg,total,cols,pfx){ var newR=Math.ceil(total/cols), html=""; for(var i=0;i<total;i++){ var rft=Math.floor(i/cols), cir=i%cols, sc=(cols-1)-cir, sr=(newR-1)-rft; html+=dhBgCell(dhLabel(sc,sr),pfx); } $bg[0].innerHTML=html; }
  function dhRefreshSortable($fg){ if($fg.hasClass("ui-sortable")){ try{ $fg.sortable("refresh"); }catch(e){} } }
  function dhReflow($div,newC){
    if(newC<1) return;
    var u=dhUls($div); if(!u.fg.length) return; var pfx=u.fg.find(".cwch").first().text();  /* chowky/chair prefix, so reflowed bg numbers keep their alignment */
    var cw=dhCellW(u.fg);
    var apps=u.fg.children("li").filter(function(){ var a=this.querySelector(".s-app-id"); return a && a.getAttribute("data-s-app-id"); }).get();
    var A=apps.length; if(!A) return;
    var newR=Math.ceil(A/newC), total=newR*newC, numEmpty=total-A;
    apps.forEach(function(el){ if(el.parentNode) el.parentNode.removeChild(el); });
    var frag=""; for(var e=0;e<numEmpty;e++) frag+="<li class=\"ui-state-default\"></li>"; u.fg[0].innerHTML=frag;
    apps.forEach(function(el){ u.fg[0].appendChild(el); });
    u.fg.find(".s-app-id").attr("data-cols",newC);
    u.fg.css("width",(newC*(cw+1))+"px");
    dhRelabelBg(u.bg,total,newC,pfx); u.bg.css("width",(newC*(cw+1))+"px");
    dhRefreshSortable(u.fg);
  }
  function dhBlankCol($div){
    var u=dhUls($div); if(!u.fg.length) return; var pfx=u.fg.find(".cwch").first().text();  /* chowky/chair prefix, so reflowed bg numbers keep their alignment */
    var C=dhCurCols($div), newC=C+1, cw=dhCellW(u.fg);
    var cells=u.fg.children("li").get(); u.fg[0].innerHTML="";
    for(var i=0;i<cells.length;i++){ if(i%C===0) u.fg.append("<li class=\"ui-state-default\"></li>"); u.fg[0].appendChild(cells[i]); }
    var total=u.fg.children("li").length;
    u.fg.find(".s-app-id").attr("data-cols",newC);
    u.fg.css("width",(newC*(cw+1))+"px");
    dhRelabelBg(u.bg,total,newC,pfx); u.bg.css("width",(newC*(cw+1))+"px");
    dhRefreshSortable(u.fg);
  }
  $(document).on("click",".dh-add-col",function(){ var $d=$(this).closest(".main-div,.chowky-div"); dhReflow($d,dhCurCols($d)+1); });
  $(document).on("click",".dh-del-col",function(){ var $d=$(this).closest(".main-div,.chowky-div"); var C=dhCurCols($d); if(C>1) dhReflow($d,C-1); });
  $(document).on("click",".dh-blank-col",function(){ dhBlankCol($(this).closest(".main-div,.chowky-div")); });
})(jQuery);
</script>';

	$out .= '</body></html>';

	if ($pdf <> '')
		create_pdf_from_html($out, $pdf);
	else
	{
		print $out;
		exit;
	}


}


function dh_auto_allocate_seats($centre, $course)
{
	if (!is_array($course))
		$course = array($course);
	$settings = db_query("select cs_seat_config from dh_center_setting where cs_center=$centre")->fetchAssoc();
	if (dh_validate_seat_config($settings['cs_seat_config']))
		return;   // invalid config - dh_generate_seating_plan will show the error

	$ini = array();
	if ($settings['cs_seat_config'] <> '')
		$ini = parse_ini_string($settings['cs_seat_config'], true, INI_SCANNER_RAW);
	
	$places = array('LEFT', 'RIGHT');
	$types = array('SeatsPerRow', 'SeatsPerRowChowky');
	$special_rows['seats']['LEFT'] = $special_rows['seats']['RIGHT'] = $special_rows['chowky']['LEFT'] = $special_rows['chowky']['RIGHT'] = array();
	$seat_alloc = array();
	foreach ($types as $t) 
	{
		$seat_alloc[$t]['LEFT'] = $ini['LEFT'][$t];
		$seat_alloc[$t]['RIGHT'] = $ini['RIGHT'][$t];;
	}
	$max_col['LEFT'] = $seat_alloc['SeatsPerRow']['LEFT'];
	$max_col['RIGHT'] = $seat_alloc['SeatsPerRow']['RIGHT'];
	$max_col_chowky['LEFT'] = $seat_alloc['SeatsPerRowChowky']['LEFT'];
	$max_col_chowky['RIGHT'] = $seat_alloc['SeatsPerRowChowky']['RIGHT'];
	foreach($places as $p)
	{
		foreach($ini[$p] as $k => $v)
		{
			if (substr(strtolower($k),0,4) == 'row-' )
			{
				$special_rows['seats'][$p][str_replace("row-", "", strtolower($k)) - 1] = $v;
				if ($v > $max_col[$p])
					$max_col[$p] = $v;
			}
			elseif (substr(strtolower($k),0,4) == 'chowkyrow-' )
			{
				$special_rows['chowky'][$p][str_replace("chowkyrow-", "", strtolower($k)) - 1] = $v;
				if ($v > $max_col[$p])
					$max_col[$p] = $v;
			}	

		}		
		if (!isset($ini[$p]['gender']))
			$ini[$p]['gender'] = ($p == 'RIGHT')?'M':'F';
	}


	$q = "select a_id, aa_id, a_gender, a_old,  concat(a_f_name, ' ',a_l_name) as 'Full Name', TIMESTAMPDIFF( YEAR, a_dob, CURDATE() ) as 'Age', ac_teen, ac_10d, ac_stp, ac_spl, ac_20d, ac_30d, ac_45d, ac_60d, ac_tsc, ac_teacher, aa_group, aa_acco, aa_section, aa_laundry, aa_chowky, aa_valuable, aa_chowky, aa_chair, aa_backrest, a_occupation, a_education, a_langs, aa_comment, aa_cell from dh_applicant left join dh_applicant_course on a_id=ac_applicant left join dh_applicant_attended on a_id=aa_applicant left join dh_teacher t on ( ac_teacher_code = CONCAT(t_code,'-',t_gender )) where a_course IN (".implode("," , $course).") and a_attended=1 and a_type='Student' order by  a_gender, IFNULL(ac_teacher,0) desc, IFNULL(t_seniority,0) desc, IFNULL(t_year_appointed,".date("Y").") , IFNULL(ac_60d,0) desc, IFNULL(ac_45d,0) desc, IFNULL(ac_30d,0) desc, IFNULL(ac_20d,0) desc, IFNULL(ac_tsc,0) desc, IFNULL(ac_spl,0) desc, IFNULL(ac_stp,0) desc, IFNULL(ac_10d,0) desc, IFNULL(ac_teen,0) desc, IFNULL(ac_service,0) desc, TIMESTAMPDIFF( YEAR, a_dob, CURDATE() ) desc";
	$result = db_query($q);
	$data = array();
	while($r = $result->fetchAssoc())
	{
		if ($r['aa_chowky'] || $r['aa_chair'])
			$chowky[strtoupper($r['a_gender'])][$r['a_old']?'old':'new'][] = $r;
		else
			$data[strtoupper($r['a_gender'])][$r['a_old']?'old':'new'][] = $r;
	}
	if (!is_array($chowky['M']))
	{
		$chowky['M']['old'] = array();
		$chowky['M']['new'] = array();				
	}

	if (!is_array($chowky['F']))
	{
		$chowky['F']['old'] = array();
		$chowky['F']['new'] = array();		
	}

	$place = 'RIGHT';
	$seats['seats'] = dh_allocate_seats($data[$ini[$place]['gender']], $seat_alloc['SeatsPerRow'][$place], $special_rows['seats'][$place] );
	$seats['chowky'] = dh_allocate_seats($chowky[$ini[$place]['gender']], $seat_alloc['SeatsPerRowChowky'][$place], $special_rows['chowky'][$place] );

	$place = 'LEFT';
	$seats['seats'] = dh_allocate_seats($data[$ini[$place]['gender']], $seat_alloc['SeatsPerRow'][$place], $special_rows['seats'][$place] );
	$seats['chowky'] = dh_allocate_seats($chowky[$ini[$place]['gender']], $seat_alloc['SeatsPerRowChowky'][$place], $special_rows['chowky'][$place] );

	return $seats;
} 


function dh_allocate_seats($data, $cushion_cols, $special_rows = array())
{
	if (!isset($data['new']))
		$data['new'] = array();
	if (!isset($data['old']))
		$data['old'] = array();
	$seats = array();
	$row = 0; $col = 0;
	$total_cushion_seats = count($data['new']) + count($data['old']);
	$i = 0; $r = 0; $total_rows = 0;
	$max_col = $cushion_cols;
	while( $i < $total_cushion_seats )
	{

		if (isset($special_rows[$r]))
		{
			$i += $special_rows[$r];
			if ($special_rows[$r] > $max_col)
				$max_col = $special_rows[$r];			
		}
		else
			$i += $cushion_cols;
		$r++;
		$total_rows++;
	}
	$top_row = $total_rows;
	$extra = $i - $total_cushion_seats;
	if (isset($special_rows[$total_rows]))
		$top_col = $special_rows[$total_rows] - $extra;	
	else
		$top_col = $cushion_cols - $extra;
	//$total_rows = floor($total_cushion_seats / $cushion_cols);
	$counter = 1 ;
	$alpha = db_query("select cs_seat_naming_conv from dh_center_setting where cs_center=".arg(1))->fetchField();

	foreach ($data['old'] as $r) 
	{	
		db_update("dh_applicant_attended")->fields(array('aa_seat_col' => $col, 'aa_seat_row' => $row))->condition('aa_id', $r['aa_id'])->execute();
		$seats[$row][$col++] = $r;//$counter++;
		//$q = "update dh_applicant_attended set where "
		$current_row_cols =  $cushion_cols;
		if (isset($special_rows[$row]))
			$current_row_cols =  $special_rows[$row];
		if ($col >= $current_row_cols) 
		{
			$row++;
			$col = 0;
		}
	}

	/*
	if (!$alpha)
	{
		foreach ($data['new'] as $r) 
		{	
			db_update("dh_applicant_attended")->fields(array('aa_seat_col' => $col, 'aa_seat_row' => $row))->condition('aa_id', $r['aa_id'])->execute();
			$seats[$row][$col++] = $r;//$counter++;
			//$q = "update dh_applicant_attended set where "
			$current_row_cols =  $cushion_cols;
			if (isset($special_rows[$row]))
				$current_row_cols =  $special_rows[$row];
			if ($col >= $current_row_cols) 
			{
				$row++;
				$col = 0;
			}
		}
	}
	else
	{
	*/
		//	print "Max cols - $max_col, Total rows = $total_rows";
		
		for($c=0; $c< $max_col; $c++)
		{
			for($r=0; $r < $total_rows; $r++ )
			{
				$current_row_cols =  $cushion_cols;
				if (isset($special_rows[$r]))
					$current_row_cols =  $special_rows[$r];
				if ( $c >= $current_row_cols )
					continue;
				if ( isset($seats[$r][$c]) && is_array($seats[$r][$c]) )
					continue;				
				if ($r == ($total_rows-1))
					if ($c >= $top_col)
						continue;
				list($k, $seat_data) = each($data['new']);
				db_update("dh_applicant_attended")->fields(array('aa_seat_col' => $c, 'aa_seat_row' => $r))->condition('aa_id', $seat_data['aa_id'])->execute();
				$seats[$r][$c] = $seat_data;
			}
		}		
	//}

	
	return $seats;
}

function dh_print_seats($seats, $cushion_cols, $direction, $div, $alpha)
{
	if (count((array)$seats) <= 0)
		return '';

  // var_dump($seats);
  // var_dump(max(array_keys($seats)));

  for($i=0; $i <= max(array_keys($seats)); $i++)
  {
    if(!isset($seats[$i]))
      $seats[$i] = '';
  }

  $counter = count($seats) * $cushion_cols;

	// foreach ($seats as $a)
	// 	foreach ($a as $b)
	// 		$counter++;

	if ($direction == LEFT_TO_RIGHT) $append = '-m'; else $append = '-f';

	$ret = '<div class="'.$div.'">';
	$out = '<table class="sortable'.$append.'">';

	for ($row = count($seats)-1; $row >= 0; $row--)
	{
		$out .= '<tr>';
		for ($col=0; $col< $cushion_cols; $col++)
		{
			$out .= '<td class="ui-state-default">';
			
			$out .= '<div class="drag-img no-print"><img src="/sites/all/modules/dh_manageapp/imgs/drag.png" alt="drag"></div>';

			//if ($direction == LEFT_TO_RIGHT)
				//$col2 = $col;
			//else
			$col2 = ($cushion_cols - $col -1);

      $s_no = $counter;
      $counter--;
      if ($alpha) {
        $s_no = dh_seat_label($col2, $row, $alpha);
      }
      $aa_seat = $s_no;
      $s_no = '<span class="seatnumber">'.$s_no.'</span>';

      $seat_no = '<div class="seat-no">'.$s_no.'</div>';

			if ( isset($seats[$row][$col2]) )
			{
				$r = $seats[$row][$col2];

        if ($r['aa_chowky'])
          $aa_seat = 'CW-'.$aa_seat;
        if ($r['aa_chair'])
          $aa_seat = 'CH-'.$aa_seat;

        $q = "update dh_applicant_attended aa left join dh_applicant a on a.a_id = aa.aa_applicant set aa_seat=:aa_seat where a_id=:a_id";
        db_query($q, array(":aa_seat" => $aa_seat, ":a_id" => $r['a_id']));
				if ($r['a_old']) $t = '(O)'; else $t ='';
				$teacher = "";	
				//$s_no = $r['counter'];

      if ($r['aa_chowky'])
        $s_no = '<span class="cwch">CW-</span>'.$s_no;
      if ($r['aa_chair'])
        $s_no = '<span class="cwch">CH-</span>'.$s_no;

      $seat_no = '<div class="seat-no">'.$s_no.'</div>';

				$br = '';
				if ($r['aa_backrest'] || $r['aa_chowky']) $br = 's-backrest-yes';
				$age_info = ' ('.$r['Age'].')'; 

				$short_courses = $long_courses = '';
				if ( $r['a_old'] )
				{
					$short_courses = 's:'.($r['ac_teen'] + $r['ac_10d'] + $r['ac_stp']);
					$long_courses = 'L:'.($r['ac_20d'] + $r['ac_30d'] + $r['ac_45d'] + $r['ac_60d'] + $r['ac_tsc'] + $r['ac_spl']);					
	  			    if( $r['ac_teacher'] )
				  	   $teacher = "s-teacher";
				}
				$info = '<div class="s-app-id" data-s-app-id="'.$r['a_id'].'" data-cols="'.$cushion_cols.'"></div>'.
            '<div class="s-seat-no">'.$seat_no.'</div>'.
						'<div class="s-acco">'.$r['aa_section'].'-'.$r['aa_acco'].($r['aa_cell'] ? dh_room_cell_sep().$r['aa_cell'] : '').'</div>'.
            '<div class="s-cell"></div>'.
						'<div class="s-name '.$teacher.'">'.$r['Full Name'].'</div>'.
						'<div class="s-old-student">'.$t.' '.$short_courses.'&nbsp;'.$long_courses.'</div>'.						
						'<div class="s-age">'.(($r['Age'] !== null && $r['Age'] !== '') ? $r['Age'].'y' : '').'</div>'.
						'<div class="s-backrest '.$br.'"></div>';

				$out .= $info;
			}
      else
      {
        $out .= '<div class="s-app-id" data-s-app-id=""></div>'.
          '<div class="s-seat-no">'.$seat_no.'</div>';
      }
			$out .= '</td>';
		}			
		$out .= '</tr>';	

	}
	$out .= '</table>';
	$out = str_replace('class="sortable', 'class="bg-table', $out).$out;
	$ret .= $out;
	$ret .= '<div class="no-print" style="clear:both;padding-top:6px">';
	$ret .= '<button type="button" class="no-print add-row" value="'.$cushion_cols.'">+ Blank Row</button>';
	$ret .= ' <button type="button" class="no-print dh-add-col" title="Add a column and re-flow applicants into it">+ Col</button>';
	$ret .= ' <button type="button" class="no-print dh-del-col" title="Remove a column and re-flow applicants">&minus; Col</button>';
	$ret .= ' <button type="button" class="no-print dh-blank-col" title="Add an empty column to drag applicants into">+ Blank Col</button>';
	$ret .= '</div>';
	$ret .= '</div>';


	return $ret;
}


/**
 * Extract the per-group overrides that share the centre's Seating Configuration
 * (cs_seat_config), parsed with the standard INI parser. Format:
 *
 *   [RIGHT]                        ; section = Male
 *   SeatsPerRow = 5                ; hall default (also the group fallback)
 *   GROUP1-SeatsPerRow = 4         ; per-group override, prefixed GROUP<n>-
 *   GROUP1-SeatsPerRowChowky = 2
 *   GROUP2-SeatsPerRow = 5
 *   [LEFT]                         ; section = Female
 *   ...
 *
 * Returns $cfg[SECTION][GROUP][key] = value; keys without a GROUP<n>- prefix
 * are stored under group '' (the section-wide default). Per-row overrides use
 * GROUP<n>-Row-<r> (seats) and GROUP<n>-ChowkyRow-<r> (chowky).
 */
function dh_parse_group_seat_config($text)
{
	$cfg = array();
	if (!is_string($text) || trim($text) === '')
		return $cfg;
	$ini = parse_ini_string($text, true, INI_SCANNER_RAW);
	if (!is_array($ini))
		return $cfg;
	foreach ($ini as $section => $keys)
	{
		$section = strtoupper($section);
		if (!is_array($keys))
			continue;
		foreach ($keys as $k => $v)
		{
			if (preg_match('/^group\s*([0-9a-zA-Z]+)-(.+)$/i', $k, $m))
				$cfg[$section][$m[1]][$m[2]] = $v;   // group-specific key
			else
				$cfg[$section][''][$k] = $v;          // section-wide default
		}
	}
	return $cfg;
}


/**
 * Resolve grid dimensions for one group + section (gender). SeatsPerRow /
 * SeatsPerRowChowky fall back: group block -> section-wide default ('') ->
 * main hall config ($ini[$section]) -> hard defaults. Per-row overrides are
 * taken ONLY from the group's own GROUP<n>- block (never inherited from the
 * hall's Row-N), so group grids stay predictable.
 */
function dh_group_dims($gcfg, $ini, $section, $group)
{
	$grp = isset($gcfg[$section][$group]) ? $gcfg[$section][$group] : array();
	$def = isset($gcfg[$section]['']) ? $gcfg[$section][''] : array();

	$spr  = isset($grp['SeatsPerRow'])       ? (int)$grp['SeatsPerRow']       : (isset($def['SeatsPerRow'])       ? (int)$def['SeatsPerRow']       : (isset($ini[$section]['SeatsPerRow'])       ? (int)$ini[$section]['SeatsPerRow']       : 5));
	$sprc = isset($grp['SeatsPerRowChowky']) ? (int)$grp['SeatsPerRowChowky'] : (isset($def['SeatsPerRowChowky']) ? (int)$def['SeatsPerRowChowky'] : (isset($ini[$section]['SeatsPerRowChowky']) ? (int)$ini[$section]['SeatsPerRowChowky'] : 1));
	if ($spr  < 1) $spr  = 1;
	if ($sprc < 1) $sprc = 1;

	// Optional per-row overrides (1-based row number -> 0-based index), from the
	// group's own block only.
	$row_overrides = array();
	$chowky_overrides = array();
	foreach ($grp as $k => $v)
	{
		if (preg_match('/^chowkyrow-([0-9]+)$/i', $k, $m))
			$chowky_overrides[(int)$m[1] - 1] = (int)$v;
		elseif (preg_match('/^row-([0-9]+)$/i', $k, $m))
			$row_overrides[(int)$m[1] - 1] = (int)$v;
	}

	$seats_cols = $spr;
	foreach ($row_overrides as $c) if ($c > $seats_cols) $seats_cols = $c;
	$chowky_cols = $sprc;
	foreach ($chowky_overrides as $c) if ($c > $chowky_cols) $chowky_cols = $c;

	// Chowky placement and seat direction: group block -> section default -> hall
	// config -> '' (caller falls back to the gender's default).
	$cpos = isset($grp['ChowkyPosition']) ? $grp['ChowkyPosition'] : (isset($def['ChowkyPosition']) ? $def['ChowkyPosition'] : (isset($ini[$section]['ChowkyPosition']) ? $ini[$section]['ChowkyPosition'] : ''));
	$sdir = isset($grp['SeatDirection']) ? $grp['SeatDirection'] : (isset($def['SeatDirection']) ? $def['SeatDirection'] : (isset($ini[$section]['SeatDirection']) ? $ini[$section]['SeatDirection'] : ''));

	return array(
		'SeatsPerRow' => $spr, 'SeatsPerRowChowky' => $sprc,
		'row_overrides' => $row_overrides, 'chowky_overrides' => $chowky_overrides,
		'seats_cols' => $seats_cols, 'chowky_cols' => $chowky_cols,
		'chowky_position' => strtolower(trim((string)$cpos)),
		'seat_direction' => strtolower(trim((string)$sdir)),
	);
}


/**
 * Lay an ordered (seniority-sorted) list of students into a grid of the given
 * width. Fills row by row; a per-row override caps that specific row's width.
 * Returns array('grid' => [row][col] => student, 'maxrow' => int, 'maxcol' => int).
 */
function dh_group_allocate($students, $seats_per_row, $row_overrides = array())
{
	$grid = array();
	$row = 0; $col = 0; $maxcol = 0;
	if ($seats_per_row < 1) $seats_per_row = 1;
	$cap = isset($row_overrides[0]) ? $row_overrides[0] : $seats_per_row;
	if ($cap < 1) $cap = $seats_per_row;
	foreach ($students as $st)
	{
		if ($col >= $cap)
		{
			$row++; $col = 0;
			$cap = isset($row_overrides[$row]) ? $row_overrides[$row] : $seats_per_row;
			if ($cap < 1) $cap = $seats_per_row;
		}
		$grid[$row][$col] = $st;
		if ($col > $maxcol) $maxcol = $col;
		$col++;
	}
	$maxrow = count($grid) ? max(array_keys($grid)) : 0;
	return array('grid' => $grid, 'maxrow' => $maxrow, 'maxcol' => $maxcol);
}


/**
 * Allocate group-wise seat positions (aa_group_seat_row / aa_group_seat_col),
 * kept separate from the main hall positions (aa_seat_row / aa_seat_col). Each
 * group/gender is packed into its own dimensions (dh_group_dims) in seniority
 * order. $force = full re-pack (clears then re-assigns everyone); otherwise only
 * students without a position yet are appended below the current bottom row, so
 * manually dragged arrangements are preserved.
 */
function dh_auto_allocate_group_seats($centre, $course, $force = false)
{
	if (!is_array($course))
		$course = array($course);

	$settings = db_query("select cs_seat_config from dh_center_setting where cs_center=$centre")->fetchField();
	if (dh_validate_seat_config($settings))
		return;   // invalid config - dh_generate_group_seating_plan will show the error
	$ini = array();
	if ($settings <> '')
		$ini = parse_ini_string($settings, true, INI_SCANNER_RAW);
	if (!isset($ini['LEFT']['gender']))  $ini['LEFT']['gender']  = 'F';
	if (!isset($ini['RIGHT']['gender'])) $ini['RIGHT']['gender'] = 'M';
	$gcfg = dh_parse_group_seat_config($settings);
	$male_section   = ($ini['RIGHT']['gender'] == 'M') ? 'RIGHT' : 'LEFT';
	$female_section = ($male_section == 'RIGHT') ? 'LEFT' : 'RIGHT';
	$section_for = array('M' => $male_section, 'F' => $female_section);

	if ($force)
		db_query("update dh_applicant_attended aa left join dh_applicant a on a.a_id=aa.aa_applicant set aa_group_seat_row=NULL, aa_group_seat_col=NULL where a_course in (".implode(",", $course).")");

	$q = "select a_id, aa_id, a_gender, aa_group, aa_chowky, aa_chair, aa_group_seat_row, aa_group_seat_col, ac_teacher from dh_applicant left join dh_applicant_course on a_id=ac_applicant left join dh_applicant_attended on a_id=aa_applicant left join dh_teacher t on ( ac_teacher_code = CONCAT(t_code,'-',t_gender )) where a_course in (".implode(",", $course).") and a_attended=1 and a_type='Student' order by  a_gender, IFNULL(ac_teacher,0) desc, IFNULL(t_seniority,0) desc, IFNULL(t_year_appointed,".date("Y").") , IFNULL(ac_60d,0) desc, IFNULL(ac_45d,0) desc, IFNULL(ac_30d,0) desc, IFNULL(ac_20d,0) desc, IFNULL(ac_tsc,0) desc, IFNULL(ac_spl,0) desc, IFNULL(ac_stp,0) desc, IFNULL(ac_10d,0) desc, IFNULL(ac_teen,0) desc, IFNULL(ac_service,0) desc, TIMESTAMPDIFF( YEAR, a_dob, CURDATE() ) desc";
	$result = db_query($q);
	$grids = array();
	while($r = $result->fetchAssoc())
	{
		$g = trim($r['aa_group']);
		if ($g === '') $g = 'Unassigned';
		$gender = strtoupper($r['a_gender']);
		if ($gender != 'M' && $gender != 'F') continue;
		$type = ($r['aa_chowky'] || $r['aa_chair']) ? 'chowky' : 'seats';
		$grids[$g][$gender][$type][] = $r;
	}

	foreach ($grids as $g => $byGender)
	{
		foreach (array('M', 'F') as $gender)
		{
			if (empty($byGender[$gender])) continue;
			$dims = dh_group_dims($gcfg, $ini, $section_for[$gender], $g);
			foreach (array('seats' => 'SeatsPerRow', 'chowky' => 'SeatsPerRowChowky') as $type => $sprk)
			{
				if (empty($byGender[$gender][$type])) continue;
				$ov  = ($type == 'seats') ? $dims['row_overrides'] : $dims['chowky_overrides'];

				// Students still needing a seat, and the current bottom row of this grid.
				$missing = array();
				$base_row = -1;
				foreach ($byGender[$gender][$type] as $st)
				{
					if ($st['aa_group_seat_row'] === null || $st['aa_group_seat_col'] === null)
						$missing[] = $st;
					elseif ((int)$st['aa_group_seat_row'] > $base_row)
						$base_row = (int)$st['aa_group_seat_row'];
				}
				if (empty($missing)) continue;

				$alloc = dh_group_allocate($missing, $dims[$sprk], $ov);
				$offset = $base_row + 1;
				foreach ($alloc['grid'] as $rr => $cols)
					foreach ($cols as $cc => $st)
						db_update('dh_applicant_attended')
							->fields(array('aa_group_seat_row' => $rr + $offset, 'aa_group_seat_col' => $cc))
							->condition('aa_id', $st['aa_id'])->execute();
			}
		}
	}
}


/**
 * Persist drag-and-drop changes from the group-wise seating plan into
 * aa_group_seat_row / aa_group_seat_col. Mirrors dh_update_seating but targets
 * the group columns. Payload $_REQUEST['u'] is a JSON array of [a_id, row, col].
 */
function dh_update_group_seating($centre, $course)
{
	$updates = json_decode($_REQUEST['u']);
	if (!is_array($course))
		$course = array($course);
	if (is_array($updates))
	{
		foreach ($updates as $update)
		{
			$q = "update dh_applicant_attended aa left join dh_applicant a on a.a_id= aa.aa_applicant set aa_group_seat_row=:aa_seat_row, aa_group_seat_col=:aa_seat_col where a.a_id=:a_id and a_center=:a_center and a_course in (:a_course)";
			db_query($q, array(
				":aa_seat_row" => $update[1],
				":aa_seat_col" => $update[2],
				":a_id"        => $update[0],
				":a_center"    => $centre,
				":a_course"    => $course,
			));
		}
	}
	drupal_json_output(array('status' => 'OK'));
	exit;
}


/**
 * Group-wise seating plan: one page per zero-day group number. Each group's
 * students are laid out into that group's OWN dimensions, taken from the
 * centre's Seating Configuration (cs_seat_config) via GROUP<n>- prefixed keys.
 * A group with no such key falls back to the un-prefixed (hall) dimensions.
 * Seats are draggable and saved via "Store Seat Changes" (group columns).
 *
 * Because each group can have different dimensions, seat numbers here are local
 * to the group grid and need not match the main (full-hall) Seating Plan.
 */
function dh_generate_group_seating_plan($centre, $course, $pdf = '')
{
	global $base_url;
	if (!is_array($course))
		$course = array($course);

	$settings = db_query("select cs_seat_config from dh_center_setting where cs_center=$centre")->fetchField();
	$config_errors = dh_validate_seat_config($settings);
	if (!empty($config_errors))
		return dh_seat_config_error_output($centre, $config_errors, $pdf);
	$alpha = db_query("select cs_seat_naming_conv from dh_center_setting where cs_center=$centre")->fetchField();
	$ini = array();
	if ($settings <> '')
		$ini = parse_ini_string($settings, true, INI_SCANNER_RAW);
	if (!isset($ini['LEFT']['gender']))
		$ini['LEFT']['gender'] = 'F';
	if (!isset($ini['RIGHT']['gender']))
		$ini['RIGHT']['gender'] = 'M';
	// Per-group overrides live in the SAME config, as GROUP<n>- prefixed keys.
	$gcfg = dh_parse_group_seat_config($settings);

	// Which INI section is Male / Female (usually RIGHT=M, LEFT=F).
	$male_section   = ($ini['RIGHT']['gender'] == 'M') ? 'RIGHT' : 'LEFT';
	$female_section = ($male_section == 'RIGHT') ? 'LEFT' : 'RIGHT';
	$section_for = array('M' => $male_section, 'F' => $female_section);
	$dir = array(
		'M' => ($ini['RIGHT']['gender'] == 'M') ? LEFT_TO_RIGHT : RIGHT_TO_LEFT,
		'F' => ($ini['LEFT']['gender'] == 'F')  ? RIGHT_TO_LEFT : LEFT_TO_RIGHT,
	);

	$course_name = '';
	$h = db_query("select c_name from dh_course where c_id in (".implode(",", $course).")");
	while($r = $h->fetchAssoc())
		$course_name .= $r['c_name']." / ";

	// Teachers per group (same source/format as the Day-0 teacher table).
	$group_teachers = array();
	$gt = db_query(
		"select ct_group, GROUP_CONCAT(concat(t_f_name, ' ', t_l_name) SEPARATOR ' | ') as tnames
		 from dh_course_teacher left join dh_teacher on t_id=ct_teacher
		 where ct_course in (".implode(",", $course).") and ct_status='Confirmed'
		 group by ct_group");
	while($tr = $gt->fetchAssoc())
		$group_teachers[trim($tr['ct_group'])] = $tr['tnames'];

	// Read the (already allocated) group seat positions.
	$q = "select a_id, aa_id, a_gender, a_old,  concat(a_f_name, ' ',a_l_name) as 'Full Name', TIMESTAMPDIFF( YEAR, a_dob, CURDATE() ) as 'Age', ac_teen, ac_10d, ac_stp, ac_spl, ac_20d, ac_30d, ac_45d, ac_60d, ac_tsc, ac_teacher, aa_cell, aa_group, aa_acco, aa_section, aa_laundry, aa_valuable, aa_chowky, aa_chair, aa_backrest, aa_group_seat_row, aa_group_seat_col  from dh_applicant left join dh_applicant_course on a_id=ac_applicant left join dh_applicant_attended on a_id=aa_applicant  left join dh_teacher t on ( ac_teacher_code = CONCAT(t_code,'-',t_gender )) where a_course in (".implode(",", $course).") and a_attended=1 and a_type='Student' order by  a_gender, IFNULL(ac_teacher,0) desc, IFNULL(t_seniority,0) desc, IFNULL(t_year_appointed,".date("Y").") , IFNULL(ac_60d,0) desc, IFNULL(ac_45d,0) desc, IFNULL(ac_30d,0) desc, IFNULL(ac_20d,0) desc, IFNULL(ac_tsc,0) desc, IFNULL(ac_spl,0) desc, IFNULL(ac_stp,0) desc, IFNULL(ac_10d,0) desc, IFNULL(ac_teen,0) desc, IFNULL(ac_service,0) desc, TIMESTAMPDIFF( YEAR, a_dob, CURDATE() ) desc";
	$result = db_query($q);

	// Build grids from stored group positions: $groups[label][GENDER][type][row][col] = student.
	$groups = array();
	$maxrow = array();
	$maxcol = array();
	$group_keys = array();
	while($r = $result->fetchAssoc())
	{
		$g = trim($r['aa_group']);
		if ($g === '')
			$g = 'Unassigned';
		$group_keys[$g] = $g;
		$gender = strtoupper($r['a_gender']);
		if ($gender != 'M' && $gender != 'F')
			continue;
		$type = ($r['aa_chowky'] || $r['aa_chair']) ? 'chowky' : 'seats';
		if ($r['aa_group_seat_row'] === null || $r['aa_group_seat_col'] === null)
			continue;
		$row = (int)$r['aa_group_seat_row'];
		$col = (int)$r['aa_group_seat_col'];
		$groups[$g][$gender][$type][$row][$col] = $r;
		if (!isset($maxrow[$g][$gender][$type]) || $row > $maxrow[$g][$gender][$type]) $maxrow[$g][$gender][$type] = $row;
		if (!isset($maxcol[$g][$gender][$type]) || $col > $maxcol[$g][$gender][$type]) $maxcol[$g][$gender][$type] = $col;
	}

	// Numeric groups ascending, 'Unassigned' last.
	$group_labels = array_values($group_keys);
	usort($group_labels, function($a, $b) {
		if ($a === 'Unassigned') return 1;
		if ($b === 'Unassigned') return -1;
		return intval($a) - intval($b);
	});

	$out = '<html><head>';
	$out .= '<script src="https://code.jquery.com/jquery-1.12.4.js"></script><script src="https://code.jquery.com/ui/1.12.1/jquery-ui.js"></script>';
	$out .= '<style>@import url("'.$base_url.'/sites/all/modules/dh_manageapp/css/seating.css?v='.filemtime(drupal_get_path("module","dh_manageapp")."/css/seating.css").'");</style>';
	$out .= '<style>.group-page{page-break-before: always;} .group-title{width:1000px;text-align:center;font-weight:bold;font-size:22px;margin-top:15px;} .group-empty{color:#888;padding:10px;}
		/* Keep CW-/CH- seat numbers on one line so chowky cells do not overflow.
		   (cwch is NOT shrunk here: a smaller prefix than the number would make the
		   foreground prefix and the background number misalign vertically.) */
		.s-seat-no{white-space:nowrap;}
		/* Clip any residual overflow so a cell can never bleed into the next one. */
		.ui-state-default{overflow:hidden;}</style>';
	$out .= '<script>
	$(function(){
		// Seat numbers live only in the fixed background layer; student cards live
		// only in the draggable layer. This way seat numbers stay put at their
		// location while students are dragged between them.
		$(".sortable-m .seatnumber, .sortable-f .seatnumber").empty();
		$(".bg-table-m, .bg-table-f").find(".s-acco, .s-cell, .s-name, .s-old-student, .s-age, .s-backrest").empty();

		$(".sortable-m, .sortable-f").sortable();
		$(".sortable-m, .sortable-f").disableSelection();
		$(document).on("mouseenter", ".sortable-m .ui-state-default, .sortable-f .ui-state-default", function(){ $(this).css({"cursor":"move"}); });

		/* Shorten an over-long name at a word boundary (same as the main plan):
		   keep whole leading words, then the first word that does not fit as just
		   its initial + "." (e.g. "Abhimanyu Singh Gaikwad Deshmukh" -> "Abhimanyu
		   Singh G."). Full name stays on hover. */
		$(".sortable-m .s-name, .sortable-f .s-name").each(function(){
		  var el = this;
		  if (el.scrollWidth <= el.clientWidth) return;
		  var full = el.textContent.replace(/\s+/g, " ").replace(/^ | $/g, "");
		  el.title = full;
		  var words = full.split(" "), kept = [];
		  for (var i = 0; i < words.length; i++) {
		    el.textContent = kept.concat([words[i]]).join(" ");
		    if (el.scrollWidth <= el.clientWidth) { kept.push(words[i]); continue; }
		    el.textContent = kept.concat([words[i].charAt(0) + "."]).join(" ");
		    while (el.scrollWidth > el.clientWidth && kept.length) {
		      kept.pop();
		      el.textContent = kept.concat([words[i].charAt(0) + "."]).join(" ");
		    }
		    break;
		  }
		});

		$(document).on("click", ".add-row", function(){
			var $div = $(this).closest(".main-div, .chowky-div");
			var $fg = $div.find("ul[class*=sortable]").first();
			var $bg = $div.find("ul[class*=bg-table]").first();
			var cols = parseInt($div.find(".s-app-id[data-cols]").first().attr("data-cols"), 10) || parseInt($(this).val(), 10) || 1;
			for (var i=0; i<cols; i++) {
				$fg.prepend(\'<li class="ui-state-default"><div class="s-app-id" data-s-app-id=""></div></li>\');
				$bg.prepend(\'<li class="ui-state-default"></li>\');
			}
		});

		$(".store-seat-changes").click(function(){
			var btn = $(this);
			btn.off("click").text("Processing... the seating plan will reload shortly.");
			var update_array = [];
			var current_path = window.location.pathname;
			var update_path = current_path.replace("group-seating", "group-seating-update");
			$(".ui-sortable li .s-app-id").each(function(){
				var closest_ul = $(this).closest("ul");
				var application_id = $(this).attr("data-s-app-id");
				var col_per_row = parseInt($(this).attr("data-cols"), 10);
				if (application_id) {
					var no_of_li = closest_ul.children("li").length;
					var current_li = $(this).closest("li").index() + 1;
					var current_position = no_of_li - current_li + 1;
					var student_row = Math.ceil(current_position / col_per_row) - 1;
					var student_column = (current_position % col_per_row) - 1;
					if (student_column == -1) student_column = col_per_row - 1;
					update_array.push([application_id, student_row, student_column]);
				}
			});
			$.getJSON(update_path, { u: JSON.stringify(update_array) })
				.done(function(json){
					if (json.status == "OK") { window.location = current_path; }
					else { alert("Update failed!\n" + (json.msg || "")); btn.text("Store Seat Changes").on("click"); }
				})
				.fail(function(){ alert("Update failed!"); btn.text("Store Seat Changes"); });
		});
	});
	</script>';
	$out .= '</head><body>';
	$out .= '<div class="helptext no-print">
		<h2>Editing Group-wise Seating Plan:</h2>
		<p>One page per group, laid out using each group\'s own dimensions from the centre\'s Seating Configuration (GROUP&lt;n&gt;- keys; falls back to the hall dimensions). "Regenerate" re-packs all groups from the configuration.</p>
		<p>1- Check basic Hall Settings in <a href="/wiki-centre-settings">Center Settings</a>.</p>
		<p>2- Rearrange the plan by dragging and dropping student seats in grid.</p>
		<p>3- Create empty rows for a grid by clicking "+ Blank Row" button if needed.</p>
		<p>4- Adjust the columns of a grid: "+ Col" adds a column and re-flows the students to fill it; "&minus; Col" removes a column and re-flows; "+ Blank Col" inserts an empty column to drag students into. (Take effect after "Store Seat Changes".)</p>
		<p>5- Save Sitting arrangement changes by clicking "Store Seat Changes" button.</p>
		<p>6- Print by clicking "Print Seating Plan" button or CTL-P.</p>
		<p>7- Adjust print settings(Layout, Scale, Paper Size) as needed and print the seating plan.</p>
		<p>Note: All help text and buttons will be ignored on print.</p>
		</div>
		<button class="no-print" onclick="window.print()">Print Seating Plan</button>
		<button type="button" class="no-print store-seat-changes">Store Seat Changes</button>';

	if (count($group_labels) == 0)
		$out .= '<div class="group-empty">No attending students found for this course.</div>';

	$first = true;
	foreach ($group_labels as $g)
	{
		$rendered_any = false;
		foreach (array('M' => 'MALE', 'F' => 'FEMALE') as $gender => $label)
		{
			if (!isset($groups[$g][$gender]['seats']) && !isset($groups[$g][$gender]['chowky']))
				continue;
			$rendered_any = true;

			/* Each gender of each group prints on its own page. */
			$page_class = $first ? '' : 'group-page';
			$out .= '<div class="dh-page-sep no-print"></div>';   // screen-only gap before every group page
			$first = false;
			$out .= '<div class="'.$page_class.'">';
			$gt_names = isset($group_teachers[trim($g)]) ? trim($group_teachers[trim($g)]) : '';
			$teacher_part = ($gt_names !== '') ? ' ('.check_plain($gt_names).')' : '';
			$out .= dh_seat_plan_header('GROUP '.check_plain($g).$teacher_part.' &ndash; '.$label.'<br>'.$course_name, 'group-title');

			$dims = dh_group_dims($gcfg, $ini, $section_for[$gender], $g);

			$s_grid = isset($groups[$g][$gender]['seats'])  ? $groups[$g][$gender]['seats']  : array();
			$c_grid = isset($groups[$g][$gender]['chowky']) ? $groups[$g][$gender]['chowky'] : array();
			// Grid width from config, but never narrower than the stored positions.
			$s_cols = $dims['seats_cols'];
			if (isset($maxcol[$g][$gender]['seats']) && $maxcol[$g][$gender]['seats'] + 1 > $s_cols) $s_cols = $maxcol[$g][$gender]['seats'] + 1;
			$c_cols = $dims['chowky_cols'];
			if (isset($maxcol[$g][$gender]['chowky']) && $maxcol[$g][$gender]['chowky'] + 1 > $c_cols) $c_cols = $maxcol[$g][$gender]['chowky'] + 1;
			$s_mr = isset($maxrow[$g][$gender]['seats'])  ? $maxrow[$g][$gender]['seats']  : 0;
			$c_mr = isset($maxrow[$g][$gender]['chowky']) ? $maxrow[$g][$gender]['chowky'] : 0;

			$gdir = dh_seat_direction($dims['seat_direction'], $dir[$gender]);
			$out_seats  = dh_print_group_seats($s_grid, $s_cols, $s_mr, $gdir, 'main-div',   $alpha);
			$out_chowky = dh_print_group_seats($c_grid, $c_cols, $c_mr, $gdir, 'chowky-div', $alpha);
			$chowky_pos = dh_chowky_position($dims['chowky_position'], $gdir);
			$out .= dh_layout_seats_chowky($out_seats, $out_chowky, $chowky_pos);
			$out .= '<div style="clear:both"></div><div class="teacher-seat">TEACHER</div>';
			$out .= '</div>';   /* close this gender's page */
		}

		if (!$rendered_any)
		{
			$page_class = $first ? '' : 'group-page';
			$out .= '<div class="dh-page-sep no-print"></div>';   // screen-only gap before every group page
			$first = false;
			$out .= '<div class="'.$page_class.'"><div class="group-empty">No students in group '.check_plain($g).'.</div></div>';
		}
	}

	$out .= '<script>
(function($){
  var DH_CONV = '.(int)$alpha.';
  function dhLetter(n){ n=parseInt(n,10); var s=""; do{ s=String.fromCharCode(65+(n%26))+s; n=Math.floor(n/26)-1; }while(n>=0); return s; }
  function dhLabel(col,row){ return (DH_CONV==2) ? (dhLetter(row)+(col+1)) : (dhLetter(col)+(row+1)); }
  function dhCellW($ul){ var li=$ul.children("li")[0]; return li ? li.getBoundingClientRect().width : 142; }
  function dhUls($div){ return { fg: $div.find("ul[class*=sortable]").first(), bg: $div.find("ul[class*=bg-table]").first() }; }
  function dhCurCols($div){ var v=$div.find(".s-app-id[data-cols]").first().attr("data-cols"); if(v) return parseInt(v,10); return parseInt($div.find(".add-row").first().val(),10)||1; }
  function dhBgCell(label,pfx){ var p=pfx?("<span class=\"cwch\">"+pfx+"</span>"):""; return "<li class=\"ui-state-default\"><div class=\"s-seat-no\"><div class=\"seat-no\">"+p+"<span class=\"seatnumber\">"+label+"</span></div></div></li>"; }
  function dhRelabelBg($bg,total,cols,pfx){ var newR=Math.ceil(total/cols), html=""; for(var i=0;i<total;i++){ var rft=Math.floor(i/cols), cir=i%cols, sc=(cols-1)-cir, sr=(newR-1)-rft; html+=dhBgCell(dhLabel(sc,sr),pfx); } $bg[0].innerHTML=html; }
  function dhRefreshSortable($fg){ if($fg.hasClass("ui-sortable")){ try{ $fg.sortable("refresh"); }catch(e){} } }
  function dhReflow($div,newC){
    if(newC<1) return;
    var u=dhUls($div); if(!u.fg.length) return; var pfx=u.fg.find(".cwch").first().text();  /* chowky/chair prefix, so reflowed bg numbers keep their alignment */
    var cw=dhCellW(u.fg);
    var apps=u.fg.children("li").filter(function(){ var a=this.querySelector(".s-app-id"); return a && a.getAttribute("data-s-app-id"); }).get();
    var A=apps.length; if(!A) return;
    var newR=Math.ceil(A/newC), total=newR*newC, numEmpty=total-A;
    apps.forEach(function(el){ if(el.parentNode) el.parentNode.removeChild(el); });
    var frag=""; for(var e=0;e<numEmpty;e++) frag+="<li class=\"ui-state-default\"></li>"; u.fg[0].innerHTML=frag;
    apps.forEach(function(el){ u.fg[0].appendChild(el); });
    u.fg.find(".s-app-id").attr("data-cols",newC);
    u.fg.css("width",(newC*(cw+1))+"px");
    dhRelabelBg(u.bg,total,newC,pfx); u.bg.css("width",(newC*(cw+1))+"px");
    dhRefreshSortable(u.fg);
  }
  function dhBlankCol($div){
    var u=dhUls($div); if(!u.fg.length) return; var pfx=u.fg.find(".cwch").first().text();  /* chowky/chair prefix, so reflowed bg numbers keep their alignment */
    var C=dhCurCols($div), newC=C+1, cw=dhCellW(u.fg);
    var cells=u.fg.children("li").get(); u.fg[0].innerHTML="";
    for(var i=0;i<cells.length;i++){ if(i%C===0) u.fg.append("<li class=\"ui-state-default\"></li>"); u.fg[0].appendChild(cells[i]); }
    var total=u.fg.children("li").length;
    u.fg.find(".s-app-id").attr("data-cols",newC);
    u.fg.css("width",(newC*(cw+1))+"px");
    dhRelabelBg(u.bg,total,newC,pfx); u.bg.css("width",(newC*(cw+1))+"px");
    dhRefreshSortable(u.fg);
  }
  $(document).on("click",".dh-add-col",function(){ var $d=$(this).closest(".main-div,.chowky-div"); dhReflow($d,dhCurCols($d)+1); });
  $(document).on("click",".dh-del-col",function(){ var $d=$(this).closest(".main-div,.chowky-div"); var C=dhCurCols($d); if(C>1) dhReflow($d,C-1); });
  $(document).on("click",".dh-blank-col",function(){ dhBlankCol($(this).closest(".main-div,.chowky-div")); });
})(jQuery);
</script>';

	$out .= '</body></html>';

	if ($pdf <> '')
		create_pdf_from_html($out, $pdf);
	else
	{
		print $out;
		exit;
	}
}


/**
 * Grid renderer for the group-wise seating plan. Renders a single <ul> of
 * fixed-width <li> cells (which jQuery UI sortable turns into a drag grid) and
 * a "+ Blank Row" button. Does NOT write to the DB - saving is handled by
 * the Store Seat Changes flow (dh_update_group_seating). $max_row is the last
 * row index of the grid; $cushion_cols its width. Seat numbers are local to
 * this grid.
 */
function dh_print_group_seats($seats, $cushion_cols, $max_row, $direction, $div, $alpha)
{
	if (count((array)$seats) <= 0)
		return '';

	$rows = $max_row + 1;
	$counter = $rows * $cushion_cols;
	$cls = ($direction == LEFT_TO_RIGHT) ? 'sortable-m' : 'sortable-f';
	$bg_cls = ($direction == LEFT_TO_RIGHT) ? 'bg-table-m' : 'bg-table-f';
	$width = ($cushion_cols * 142) + 6;

	$ret = '<div class="'.$div.'">';
	$out = '<ul class="'.$cls.'" style="width:'.$width.'px">';

	for ($row = $rows - 1; $row >= 0; $row--)
	{
		for ($col = 0; $col < $cushion_cols; $col++)
		{
			$col2 = $cushion_cols - $col - 1;
			$s_no = $counter;
			$counter--;
			if ($alpha)
				$s_no = dh_seat_label($col2, $row, $alpha);
			$s_no_html = '<span class="seatnumber">'.$s_no.'</span>';
			$seat_no = '<div class="seat-no">'.$s_no_html.'</div>';

			$out .= '<li class="ui-state-default">';
			if ( isset($seats[$row][$col2]) && is_array($seats[$row][$col2]) )
			{
				$r = $seats[$row][$col2];
				if ($r['aa_chowky'])
					$seat_no = '<div class="seat-no"><span class="cwch">CW-</span>'.$s_no_html.'</div>';
				if ($r['aa_chair'])
					$seat_no = '<div class="seat-no"><span class="cwch">CH-</span>'.$s_no_html.'</div>';
				if ($r['a_old']) $t = '(O)'; else $t = '';
				$teacher = '';
				$br = '';
				if ($r['aa_backrest'] || $r['aa_chowky']) $br = 's-backrest-yes';
				$age_info = ' ('.$r['Age'].')';
				$short_courses = $long_courses = '';
				if ( $r['a_old'] )
				{
					$short_courses = 's:'.($r['ac_teen'] + $r['ac_10d'] + $r['ac_stp']);
					$long_courses = 'L:'.($r['ac_20d'] + $r['ac_30d'] + $r['ac_45d'] + $r['ac_60d'] + $r['ac_tsc'] + $r['ac_spl']);
					if ( $r['ac_teacher'] )
						$teacher = 's-teacher';
				}
				$out .= '<div class="s-app-id" data-s-app-id="'.$r['a_id'].'" data-cols="'.$cushion_cols.'"></div>'.
					'<div class="s-seat-no">'.$seat_no.'</div>'.
					'<div class="s-acco">'.$r['aa_section'].'-'.$r['aa_acco'].($r['aa_cell'] ? dh_room_cell_sep().$r['aa_cell'] : '').'</div>'.
					'<div class="s-cell"></div>'.
					'<div class="s-name '.$teacher.'">'.$r['Full Name'].'</div>'.
					'<div class="s-old-student">'.$t.' '.$short_courses.'&nbsp;'.$long_courses.'</div>'.
					'<div class="s-age">'.(($r['Age'] !== null && $r['Age'] !== '') ? $r['Age'].'y' : '').'</div>'.
					'<div class="s-backrest '.$br.'"></div>';
			}
			else
			{
				$out .= '<div class="s-app-id" data-s-app-id=""></div>'.
					'<div class="s-seat-no">'.$seat_no.'</div>';
			}
			$out .= '</li>';
		}
	}
	$out .= '</ul>';
	// Duplicate the grid: a fixed background layer (bg-table-*, holds the seat
	// numbers) behind the draggable foreground layer (sortable-*, holds the
	// student cards). The layers are separated in JS; see the render script.
	$out = str_replace('class="'.$cls.'"', 'class="'.$bg_cls.'"', $out).$out;
	$ret .= $out;
	$ret .= '<div class="no-print" style="clear:both;padding-top:6px">';
	$ret .= '<button type="button" class="no-print add-row" value="'.$cushion_cols.'">+ Blank Row</button>';
	$ret .= ' <button type="button" class="no-print dh-add-col" title="Add a column and re-flow applicants into it">+ Col</button>';
	$ret .= ' <button type="button" class="no-print dh-del-col" title="Remove a column and re-flow applicants">&minus; Col</button>';
	$ret .= ' <button type="button" class="no-print dh-blank-col" title="Add an empty column to drag applicants into">+ Blank Col</button>';
	$ret .= '</div>';
	$ret .= '</div>';
	return $ret;
}



function dh_update_seating($centre, $course)
{
  $updates = $_REQUEST['u'];
  $updates = json_decode($updates);
  // watchdog("sitting-update", print_r($updates, true));

  if (!is_array($course))
    $course = array($course);

  foreach($updates as $update)
  {
    $q ="update dh_applicant_attended aa left join dh_applicant a on a.a_id= aa.aa_applicant set aa_seat_row=:aa_seat_row, aa_seat_col=:aa_seat_col where a.a_id=:a_id and a_center=:a_center and a_course in (:a_course)";
    db_query($q,
      array(
        ":aa_seat_row" => $update[1],
        ":aa_seat_col" => $update[2],
        ":a_id" => $update[0],
        ":a_center" => $centre,
        ":a_course" => $course,
      )
    );
  }

    //$out['status'] = 'Failed';
    $out['status'] = 'OK';
    //$out['msg'] ="Invalid Group";
    drupal_json_output($out);
    exit;
}



/* Title-case a value only when it is stored fully upper-case (fixes shouty
   entries like "RETIRED POWER ENGINEER") while leaving already mixed-case and
   acronym-heavy values (M.Sc, BCOM) untouched. Blank/junk stay blank. */
function dh_day0_case($s)
{
	$s = trim($s);
	if ($s === '')
		return '';
	if (mb_strtoupper($s, 'UTF-8') === $s)
		return mb_convert_case($s, MB_CASE_TITLE, 'UTF-8');
	return $s;
}

/* Full name from parts (avoids the NULL-concat gap when a middle name is null);
   collapses the double space left by a missing middle name. */
function dh_day0_name($r)
{
	$name = trim($r['a_f_name'].' '.$r['a_m_name'].' '.$r['a_l_name']);
	$name = preg_replace('/\s+/', ' ', $name);
	// Title-case every word so mixed/shouty parts normalise (e.g. "Aagam KUSHAIL
	// Jain" -> "Aagam Kushail Jain", "Akash surendra ..." -> "Akash Surendra ...").
	return mb_convert_case($name, MB_CASE_TITLE, 'UTF-8');
}

/* Sentence-case a free-text value that is stored fully upper-case (long health /
   special-requirement notes), leaving already mixed-case text untouched. */
function dh_day0_sentence($s)
{
	$s = trim($s);
	if ($s === '' || mb_strtoupper($s, 'UTF-8') !== $s)
		return $s;
	$s = mb_strtolower($s, 'UTF-8');
	$s = preg_replace_callback('/(^|[.!?]\s+)(\p{L})/u',
		function ($m) { return $m[1].mb_strtoupper($m[2], 'UTF-8'); }, $s);
	return preg_replace('/\bi\b/', 'I', $s);   // keep the pronoun "I" capitalised
}

/* Clean a comma-joined language list: trim, drop blanks, de-dup (case-insensitive),
   title-case shouty tokens. */
function dh_day0_langs($raw)
{
	$seen = array();
	$out = array();
	foreach (explode(',', $raw) as $p)
	{
		$p = trim($p);
		$k = strtolower($p);
		if ($p !== '' && !isset($seen[$k]))
		{
			$seen[$k] = 1;
			$out[] = dh_day0_case($p);
		}
	}
	return implode(', ', $out);
}

/* Occupation (designation), dropping placeholder designations like None/NA/00. */
function dh_day0_occupation($r)
{
	$occ = dh_day0_case($r['a_occupation']);
	$des = trim($r['a_designation']);
	$junk = array('', 'none', 'na', 'n/a', 'nil', '-', '--', '.', '()', '00', '0');
	$is_junk = in_array(strtolower(trim($des, ' ()')), $junk);
	// Skip a designation that just repeats the occupation.
	$dup = strtolower($des) === strtolower(trim($r['a_occupation']));
	if ($des !== '' && !$is_junk && !$dup)
	{
		$des = dh_day0_case($des);
		return $occ !== '' ? $occ.' ('.$des.')' : $des;
	}
	return $occ;
}

function dh_zero_day_list($centre, $course, $pdf = '')
{
	global $base_url;
	$is_pdf = ($pdf <> '');

	$out = '<style>@import url("'.$base_url.'/sites/all/modules/dh_manageapp/css/day0-list.css");</style>';

	$temp = db_query("select c_name,td_val3 from dh_course c left join  dh_type_detail td on c.c_course_type=td.td_id where c_id=$course")->fetchAssoc();
	$course_name = $temp['c_name'];
	$c_type = strtolower($temp['td_val3']);
	$is_lc = ($c_type == 'lc');

	$by_conf = (isset($_REQUEST['conf']) && ($_REQUEST['conf'] == 1));
	$orderby = " order by a_type desc, a_gender, a_old DESC, a_f_name, a_l_name, IFNULL(ac_teacher,0) desc, IFNULL(ac_60d,0) desc, IFNULL(ac_45d,0) desc, IFNULL(ac_30d,0) desc, IFNULL(ac_20d,0) desc, IFNULL(ac_tsc,0) desc, IFNULL(ac_spl,0) desc, IFNULL(ac_stp,0) desc, IFNULL(ac_10d,0) desc, IFNULL(ac_teen,0) desc, TIMESTAMPDIFF( YEAR, a_dob, CURDATE() ) desc";
	if ($by_conf)
		$orderby = " order by LEFT(a_conf_no, 2), CAST(SUBSTRING(a_conf_no, 3) AS UNSIGNED)";

	$q = "select a_id, a_gender, a_type, a_old, a_f_name, a_m_name, a_l_name, TIMESTAMPDIFF( YEAR, a_dob, CURDATE() ) as 'Age', a_city_str, a_conf_no , a_mob_country, a_phone_mobile, a_email,a_learn_about_vip, a_willing_to_serve, ac_teen, ac_10d, ac_stp, ac_spl, ac_20d, ac_30d, ac_45d, ac_60d, ac_tsc, ac_teacher, a_occupation, a_designation, a_education, a_lang_1, a_lang_2, a_langs, a_lang_discourse, ci.c_name as 'City', al_special_req_details, al_at_year, ae_desc_physical, ae_desc_mental, ae_pregnant, ae_pregnant_detail, co.c_name as 'Country', a_source from dh_applicant left join dh_applicant_course on a_id=ac_applicant left join dh_applicant_extra ae on a_id=ae_applicant left join dh_applicant_lc on a_id=al_applicant left join dh_city ci on a_city=ci.c_id left join dh_country co on (a_country=co.c_code) where a_course='$course' and a_status in ('Confirmed', 'Expected', 'ReConfirmation', 'Attended') $orderby";
	$result = db_query($q);
	$data['M']['old'] = $data['M']['new'] = $data['F']['old'] = $data['F']['new'] = array();
	while($r = $result->fetchAssoc())
		$data[strtoupper($r['a_gender'])][$r['a_old']?'old':'new'][] = $r;

	// Column spec: key => [label, colgroup-class]. Toggleable columns are named
	// in $toggle. One spec for a normal course, one for a long course.
	if (!$is_lc)
	{
		$cols = array(
			'sr'       => array('Sr', 'c-sr'),
			'conf'     => array('Conf No', 'c-conf'),
			'student'  => array('Student', 'c-student'),
			'courses'  => array('Crs', 'c-courses'),
			'edu'      => array('Education', 'c-edu'),
			'age'      => array('Age', 'c-age'),
			'city'     => array('City', 'c-city'),
			'occ'      => array('Occupation', 'c-occ'),
			'contact'  => array('Contact Details', 'c-contact'),
			'comments' => array('Recom. / Comments', 'c-comments'),
		);
		$toggle = array('occ' => 'Occupation', 'contact' => 'Contact', 'comments' => 'Comments');
	}
	else
	{
		$cols = array(
			'sr'      => array('Sr', 'l-sr'),
			'conf'    => array('Conf No', 'l-conf'),
			'student' => array('Student', 'l-student'),
			'courses' => array('Crs', 'l-courses'),
			'city'    => array('City / Country', 'l-city'),
			'age'     => array('Age', 'l-age'),
			'langs'   => array('Languages', 'l-langs'),
			'health'  => array('Health Problems', 'l-health'),
			'special' => array('Special Requirement', 'l-special'),
			'10d'     => array('10d', 'l-10d'),
			'stp'     => array('STP', 'l-stp'),
			'tsc'     => array('TSC', 'l-tsc'),
			'20d'     => array('20d', 'l-20d'),
			'30d'     => array('30d', 'l-30d'),
			'45d'     => array('45d', 'l-45d'),
			'60d'     => array('60d', 'l-60d'),
			'contact' => array('Contact', 'l-contact'),
		);
		$toggle = array('health' => 'Health', 'special' => 'Special Req', 'contact' => 'Contact');
	}
	$ncol = count($cols);

	$title = '<div class="header-day0"><div class="title">Day 0 List</div>'.
			'<div class="title-head">'.check_plain($course_name).'</div></div>';
	$out .= $title;

	// Screen-only toolbar: sort toggle + column hide/show (skipped for PDF).
	if (!$is_pdf)
	{
		$base = $base_url."/day0-list/$centre/$course";
		$out .= '<div class="day0-toolbar no-print">';
		$out .= '<span class="grp"><b>Sort:</b> '
			.'<a class="'.($by_conf?'':'active').'" href="'.$base.'">Name</a> &middot; '
			.'<a class="'.($by_conf?'active':'').'" href="'.$base.'?conf=1">Confirmation no.</a></span>';
		$out .= '<span class="grp"><b>Columns:</b>';
		foreach ($toggle as $key => $lbl)
			$out .= ' <button type="button" class="col-toggle" data-toggle-col="'.$key.'">'.check_plain($lbl).'</button>';
		$out .= '</span>';
		$out .= '</div>';
	}

	$gender = array('M', 'F');
	$type = array('old', 'new');
	$female_started = false;

	foreach($gender as $g)
	{
		foreach ($type as $t)
		{
			$total = count($data[$g][$t]);
			if ($total == 0)
				continue;

			// Start the female roster on a fresh printed page.
			$brk = '';
			if ($g == 'F' && !$female_started)
			{
				$brk = ' day0-break';
				$female_started = true;
			}

			$out .= '<div class="day0-block'.$brk.'"><div class="day0-scroll"><table class="table-day0-list"><colgroup>';
			foreach ($cols as $key => $c)
				$out .= '<col class="'.$c[1].' d0-'.$key.'">';
			$out .= '</colgroup><thead>';
			// Block-info row (repeats on print), then the column-header row.
			$out .= '<tr><th class="day0-blockinfo" colspan="'.$ncol.'">'
				.(($g=='M')?'Male':'Female').'<span class="sep">|</span>'.ucfirst($t).' students'
				.'<span class="sep">|</span><span class="cnt">'.$total.' total</span></th></tr>';
			$out .= '<tr>';
			foreach ($cols as $key => $c)
			{
				$lbl = check_plain($c[0]);
				if (isset($toggle[$key]))          // wrap so JS can hide the content (not the cell)
					$lbl = '<span class="d0c">'.$lbl.'</span>';
				$out .= '<th class="'.$c[1].' d0-'.$key.'">'.$lbl.'</th>';
			}
			$out .= '</tr></thead><tbody>';

			$i = 1;
			foreach ($data[$g][$t] as $r)
			{
				$cell = array();
				$cell['sr'] = $i++;
				$cell['conf'] = check_plain($r['a_conf_no']);

				$name = check_plain(dh_day0_name($r));
				if ($r['ac_teacher'])
					$name .= ' <span style="color:#888">(AT-'.check_plain($r['al_at_year']).')</span>';
				elseif ($r['a_type'] == 'Sevak')
					$name .= ' <span style="color:#888">(Sevak)</span>';
				$cell['student'] = $name;

				$cell['courses'] = $r['ac_10d']+$r['ac_stp']+$r['ac_spl']+$r['ac_20d']+$r['ac_30d']+$r['ac_45d']+$r['ac_60d']+$r['ac_tsc'];

				$city = ($r['City'] <> '') ? $r['City'] : $r['a_city_str'];

				if (!$is_lc)
				{
					$cell['edu'] = check_plain(trim($r['a_education']));
					$cell['age'] = check_plain($r['Age']);
					$cell['city'] = check_plain(dh_day0_case($city));
					$cell['occ'] = check_plain(dh_day0_occupation($r));

					$cc = trim($r['a_mob_country']);
					$ph = trim($r['a_phone_mobile']);
					$em = trim($r['a_email']);
					$contact = '';
					if ($ph !== '')
						$contact .= '<div class="ph">M: '.($cc!==''?'('.check_plain($cc).') ':'').check_plain($ph).'</div>';
					if ($em !== '')
						$contact .= '<div class="em">'.check_plain(strtolower($em)).'</div>';
					$cell['contact'] = $contact;

					$comments = check_plain(trim($r['a_learn_about_vip']));
					if ($r['a_willing_to_serve'])
						$comments .= ($comments!==''?' ':'').'<em>Can serve</em>';
					if ($r['a_lang_discourse'])
						$comments .= '<div>'.check_plain($r['a_lang_discourse']).'</div>';
					if ($r['ae_pregnant'])
						$comments .= '<div>Pregnant (month: '.check_plain($r['ae_pregnant_detail']).')</div>';
					$cell['comments'] = $comments;
				}
				else
				{
					if (strtoupper($r['Country']) == 'INDIA')
						$cell['city'] = check_plain(dh_day0_case($city));
					else
						$cell['city'] = check_plain(dh_day0_case($r['Country']));
					$cell['age'] = check_plain($r['Age']);

					if ($r['a_source'] == 'ExtApp')
						$langs = dh_day0_langs($r['a_lang_discourse']);
					else
						$langs = dh_day0_langs($r['a_lang_discourse'].','.$r['a_lang_1'].','.$r['a_lang_2'].','.$r['a_langs']);
					$cell['langs'] = check_plain($langs);
					$cell['health'] = check_plain(dh_day0_sentence(trim($r['ae_desc_physical'].' '.$r['ae_desc_mental'])));
					$cell['special'] = check_plain(dh_day0_sentence($r['al_special_req_details']));
					$cell['10d'] = check_plain($r['ac_10d']);
					$cell['stp'] = check_plain($r['ac_stp']);
					$cell['tsc'] = check_plain($r['ac_tsc']);
					$cell['20d'] = check_plain($r['ac_20d']);
					$cell['30d'] = check_plain($r['ac_30d']);
					$cell['45d'] = check_plain($r['ac_45d']);
					$cell['60d'] = check_plain($r['ac_60d']);
					$cc = trim($r['a_mob_country']);
					$ph = trim($r['a_phone_mobile']);
					$cell['contact'] = $ph!=='' ? '<div class="ph">M: '.($cc!==''?'('.check_plain($cc).') ':'').check_plain($ph).'</div>' : '';
				}

				$out .= '<tr>';
				foreach ($cols as $key => $c)
				{
					$v = isset($cell[$key]) ? $cell[$key] : '';
					if (isset($toggle[$key]))      // wrap so JS can hide the content (not the cell)
						$v = '<span class="d0c">'.$v.'</span>';
					$out .= '<td class="'.$c[1].' d0-'.$key.'">'.$v.'</td>';
				}
				$out .= '</tr>';
			}
			$out .= '</tbody></table></div></div>';
		}
	}

	if ($is_pdf)
	{
		create_pdf_from_html($out, $pdf);
		return;
	}

	// Self-contained column hide/show (no jQuery). Columns carry a d0-<key> class
	// on col/th/td. To hide a column we keep the <td> in place (so later cells do
	// NOT shift into the wrong <col> widths), collapse the <col> width to 0, zero
	// the cell padding, and display:none the content wrapper - the last part is
	// what stops the hidden text from still reserving row height (visibility:
	// collapse leaves that height behind, which bloated the print to 1-2 rows/page).
	$out .= '<script>
document.addEventListener("click", function(e){
	var b = e.target.closest ? e.target.closest("[data-toggle-col]") : null;
	if (!b) return;
	var hide = b.classList.toggle("off");
	var els = document.querySelectorAll(".d0-" + b.getAttribute("data-toggle-col"));
	for (var i = 0; i < els.length; i++) {
		var el = els[i];
		if (el.tagName === "COL") { el.style.width = hide ? "0" : ""; continue; }
		el.style.paddingLeft = el.style.paddingRight = hide ? "0" : "";
		var w = el.querySelector(".d0c");
		if (w) w.style.display = hide ? "none" : "";
	}
});
</script>';
	print $out;
	exit;
}


/* One #|Name|Room|Cell group for the cell list. $idx is the position in the
   sorted batch; $keys are its cell numbers. Blank cells for an empty slot. */
function dh_cell_list_pair($cd, $keys, $idx)
{
	if (!isset($keys[$idx]))
		return '<td class="cl-idx"></td><td class="cl-name"></td><td class="cl-room"></td><td class="cl-cell"></td>';
	$k = $keys[$idx];
	return '<td class="cl-idx">'.($idx + 1).'</td>'
		.'<td class="cl-name">'.check_plain($cd[$k]['name']).'</td>'
		.'<td class="cl-room">'.check_plain($cd[$k]['room']).'</td>'
		.'<td class="cl-cell">'.check_plain($cd[$k]['cell'])
			.(!empty($cd[$k]['fixed']) ? '<span class="cl-fixed no-print" title="Fixed cell - kept when the list is regenerated">&#9679;</span>' : '')
			.'</td>';
}

function dh_cell_list($centre, $course, $debug = 0, $pdf = '' )
{
	global $base_url;
	$ROWS_PER_PAGE = variable_get('cell_list_rows_per_page', '5');
	// Sort order within each batch: 'name' (default) or 'cell' (natural cell order).
	$sort = (isset($_REQUEST['sort']) && $_REQUEST['sort'] == 'cell') ? 'cell' : 'name';
	// Layout: 1 column (default, full names) or 2 columns (denser).
	$cols = (isset($_REQUEST['cols']) && $_REQUEST['cols'] == 2) ? 2 : 1;
	$gender = array('M', 'F');
	$data['M'][0] = array(); $data['F'][0] = array();
	$course_name = db_query("select c_name from dh_course where c_id=$course")->fetchField();
	$q = "select a_id, aa_id, a_gender, a_old,  concat(a_f_name, ' ',a_m_name, ' ',a_l_name) as 'Full Name', TIMESTAMPDIFF( YEAR, a_dob, CURDATE() ) as 'Age', a_conf_no , ac_teen, ac_10d, ac_stp, ac_spl, ac_20d, ac_30d, ac_45d, ac_60d, ac_tsc, ac_teacher, aa_group, aa_section, aa_acco, aa_cell, aa_cell_fixed, IFNULL(aa_cell_group,0) as 'aa_cell_group' from dh_applicant left join dh_applicant_course on a_id=ac_applicant left join dh_applicant_attended on a_id=aa_applicant left join dh_teacher on (ac_teacher_code = CONCAT(t_code, '-', t_gender)) where a_course='$course' and a_attended=1  and a_type='Student' order by a_gender, IFNULL(ac_teacher,0) desc, IFNULL(t_seniority,0) desc, IFNULL(t_year_appointed,".date("Y").") , IFNULL(ac_60d,0) desc, IFNULL(ac_45d,0) desc, IFNULL(ac_30d,0) desc, IFNULL(ac_20d,0) desc, IFNULL(ac_tsc,0) desc, IFNULL(ac_spl,0) desc, IFNULL(ac_stp,0) desc, IFNULL(ac_10d,0) desc, IFNULL(ac_teen,0) desc, TIMESTAMPDIFF( YEAR, a_dob, CURDATE() ) desc, a_id";
	$result = db_query($q);
	$has_fixed = false;
	while($r = $result->fetchAssoc())
	{
		if ($r['aa_cell_fixed']) $has_fixed = true;
		$name = $r['Full Name'];
		if ($debug) $name .= ' ('.$r['Age'].')';
		$room = $r['aa_section'].'-'.$r['aa_acco'];
		if ($room === '-') $room = '';
		// Key by the unique applicant id, NOT aa_cell: several students can share a
		// cell value (or have a blank cell), which would collapse them to one entry.
		$data[strtoupper($r['a_gender'])][$r['a_old']][$r['aa_cell_group']][$r['a_id']] = array('name' => $name, 'room' => $room, 'cell' => $r['aa_cell'], 'fixed' => $r['aa_cell_fixed']);
		if ( !isset( $age_range[strtoupper($r['a_gender'])][$r['a_old']][$r['aa_cell_group']]) )
		{
			$age_range[strtoupper($r['a_gender'])][$r['a_old']][$r['aa_cell_group']]['lower'] = $r['Age'];
			$age_range[strtoupper($r['a_gender'])][$r['a_old']][$r['aa_cell_group']]['upper'] = 0;
		}
		if ( $r['Age'] > $age_range[strtoupper($r['a_gender'])][$r['a_old']][$r['aa_cell_group']]['upper'] )
			$age_range[strtoupper($r['a_gender'])][$r['a_old']][$r['aa_cell_group']]['upper'] = $r['Age'];

		if ( $r['Age'] < $age_range[strtoupper($r['a_gender'])][$r['a_old']][$r['aa_cell_group']]['lower'] )
			$age_range[strtoupper($r['a_gender'])][$r['a_old']][$r['aa_cell_group']]['lower'] = $r['Age'];

	}
	$out = '<html><head>';
	$out .= '<style>@import url("'.$base_url.'/sites/all/modules/dh_manageapp/css/cell-list.css");</style>';
	$out .= '</head><body>';
	// Screen-only document title (in print, each table's repeating <thead> carries
	// the title onto every page - see the batch loop below).
	$out .= '<div class="header-day0 no-print"><div align="center" class="title"><b>Cell List</b></div>'.
			'<div align="center" class="title-head">'.$course_name.'</div></div>';
	$sort_base = 'cell-list/'.$centre.'/'.$course;
	$keep_cols = ($cols == 2) ? array('cols' => 2) : array();          // preserve column choice on sort links
	$keep_sort = ($sort == 'cell') ? array('sort' => 'cell') : array(); // preserve sort choice on column links
	$out .= '<div class="cl-sort no-print">Sort by: '.
		($sort=='name' ? '<b>Name</b>' : l('Name', $sort_base, array('query'=>array('sort'=>'name') + $keep_cols))).
		' &middot; '.
		($sort=='cell' ? '<b>Cell</b>' : l('Cell', $sort_base, array('query'=>array('sort'=>'cell') + $keep_cols))).
		' &nbsp;&nbsp;&bull;&nbsp;&nbsp; Columns: '.
		($cols==1 ? '<b>1</b>' : l('1', $sort_base, array('query'=>$keep_sort))).
		' &middot; '.
		($cols==2 ? '<b>2</b>' : l('2', $sort_base, array('query'=>array('cols'=>2) + $keep_sort))).
		'</div>';
	if ($has_fixed)
		$out .= '<div class="cl-fixed-note no-print"><span class="cl-fixed">&#9679;</span> A red dot means the cell is <b>fixed</b> &ndash; it is kept as-is when the list is regenerated. (Shown on screen only, not in print.)</div>';
	// Outer table: its <thead> is the document title, which browsers reprint at the
	// top of every page (the reliable running-header technique). All the batches go
	// in its single body cell.
	$out .= '<table class="cl-doc"><thead><tr><td class="cl-doc-cell">'.
		'<div class="cl-dt-main">Cell List</div><div class="cl-dt-sub">'.check_plain($course_name).'</div>'.
		'</td></tr></thead><tbody><tr><td class="cl-doc-body">';
	$out .= '<div class="main-div">';
	$first = true;
	foreach( $gender as $g )
	{
		for($i=1; $i>=0 ; $i--)
		{
			if (empty($data[$g][$i]))
				continue;
			$num_batches = count($data[$g][$i]);   // only label "Batch N" when there is more than one
			$cat_first = true;                     // first batch of this category starts a new page
			foreach( $data[$g][$i] as $cell_group => $cell_data )
			{
				//$out .= '<div class="title-info">'.$g.' Batch '.$cell_group.'</div>';
				if ( ! $debug )
				{
					if ($sort == 'cell')
						uasort($cell_data, function($a, $b){ return strnatcasecmp($a['cell'], $b['cell']); });  // natural cell order
					else
						uasort($cell_data, function($a, $b){ return strcasecmp($a['name'], $b['name']); });      // by name
				}
				$keys = array_keys($cell_data);
				$n = count($keys);
				$batch_label = ($i?'Old':'New').' '.($g=='M'?'Male':'Female').($num_batches > 1 ? ' Batch '.($cell_group+1) : '').
					' ('.$age_range[$g][$i][$cell_group]['lower'].'&ndash;'.$age_range[$g][$i][$cell_group]['upper'].' yrs)';
				if ($cols == 2) { $rows = (int) ceil($n / 2); $colspan = 9; }  // two entries per row (row-major)
				else            { $rows = $n;                 $colspan = 4; }  // one entry per row
				{
					// Each batch starts on its own page; the table's <thead> (doc title +
					// batch title + column header) repeats on every page it spans.
					// Page break before the first batch of each category; batches within a
					// category flow together (they just must not split across pages - see CSS).
					$out .= '<div class="cl-batch'.((!$first && $cat_first) ? ' cl-break' : '').'">';
					$first = false; $cat_first = false;
					$out .= '<table cellspacing="0" class="table-cell-list'.($cols==2 ? ' cl-2col' : '').'">';
					if ($cols == 2)
						$out .= '<colgroup><col class="c2-idx"><col class="c2-name"><col class="c2-room"><col class="c2-cell"><col class="c2-gap"><col class="c2-idx"><col class="c2-name"><col class="c2-room"><col class="c2-cell"></colgroup>';
					else
						$out .= '<colgroup><col class="c-idx"><col class="c-name"><col class="c-room"><col class="c-cell"></colgroup>';
					$out .= '<thead>';
					$out .= '<tr><th colspan="'.$colspan.'" class="cl-batch-title">'.$batch_label.'</th></tr>';
					if ($cols == 2)
						$out .= '<tr><th class="cl-idx">#</th><th>Name</th><th>Room</th><th>Cell</th><th class="cl-gap">&nbsp;</th><th class="cl-idx">#</th><th>Name</th><th>Room</th><th>Cell</th></tr>';
					else
						$out .= '<tr><th class="cl-idx">#</th><th>Name</th><th>Room</th><th>Cell</th></tr>';
					$out .= '</thead><tbody>';

					for($a=0; $a<$rows; $a++ )
					{
						if ($cols == 2)
							// Row-major: left = 2a, right = 2a+1, so the running number stays
							// sequential down the rows and does not break when a batch spans pages.
							$out .= '<tr>'.dh_cell_list_pair($cell_data, $keys, 2 * $a).'<td class="cl-gap">&nbsp;</td>'.dh_cell_list_pair($cell_data, $keys, 2 * $a + 1).'</tr>';
						else
							$out .= '<tr>'.dh_cell_list_pair($cell_data, $keys, $a).'</tr>';
					}
					$out .= '</tbody></table>';
					$out .= '</div>';
				}
			}

		}
	}
	$out .= '</div>';                              // .main-div
	$out .= '</td></tr></tbody></table>';          // .cl-doc outer running-header table
	$out .= '</html>';

	if ($pdf <> '')
		create_pdf_from_html($out, $pdf);
	else
	{
		print $out;
		exit;
	}
}

function generate_cell_list($centre, $course)
{
	$settings = db_query("select cs_cells_config from dh_center_setting where cs_center=$centre")->fetchAssoc();

	$ini = array();
	if ($settings['cs_cells_config'] <> '')
		$ini = parse_ini_string($settings['cs_cells_config'], true, INI_SCANNER_RAW);
	
	$gender = array('MALE', 'FEMALE');
	$types = array('Reserved', 'Cells', 'Old', 'New');
	foreach ($types as $v) {
		$cell_alloc[$v]['MALE'] = array();
		$cell_alloc[$v]['FEMALE'] = array();
	}

	foreach($gender as $g)
	{
		foreach( $types as $t )
		{
			if ( isset($ini[$g][$t]) && ($ini[$g][$t] <> '') )
			{
				$temp = explode(",", $ini[$g][$t]);
				foreach( $temp as $v )
				{
					$temp2 = explode("-",trim($v));
					if (count($temp2) > 1)
					{
						for($i=preg_replace("/[A-Za-z-]/", "", $temp2[0]); $i<=preg_replace("/[A-Za-z-]/", "", $temp2[1]); $i++)
							$cell_alloc[$t][$g][] = preg_replace("/[0-9]/", "", $temp2[0]).$i;
					}
					else
						$cell_alloc[$t][$g][] = $temp2[0];
				}
			}
		}
	}

	$q = "select a_id, aa_id, a_gender, a_old,  concat(a_f_name, ' ',a_m_name, ' ',a_l_name) as 'Full Name', TIMESTAMPDIFF( YEAR, a_dob, CURDATE() ) as 'Age', a_conf_no , ac_teen, ac_10d, ac_stp, ac_spl, ac_20d, ac_30d, ac_45d, ac_60d, ac_tsc, ac_teacher, aa_group, aa_cell, aa_cell_fixed from dh_applicant left join dh_applicant_course on a_id=ac_applicant left join dh_applicant_attended on a_id=aa_applicant left join dh_teacher on (ac_teacher_code = CONCAT(t_code, '-', t_gender)) where a_course='$course' and a_attended=1  and a_type='Student' order by a_gender, IFNULL(ac_teacher,0) desc, IFNULL(t_seniority,0) desc, IFNULL(t_year_appointed,".date("Y").") , IFNULL(ac_60d,0) desc, IFNULL(ac_45d,0) desc, IFNULL(ac_30d,0) desc, IFNULL(ac_20d,0) desc, IFNULL(ac_tsc,0) desc, IFNULL(ac_spl,0) desc, IFNULL(ac_stp,0) desc, IFNULL(ac_10d,0) desc, IFNULL(ac_teen,0) desc, TIMESTAMPDIFF( YEAR, a_dob, CURDATE() ) desc, a_id";
	$result = db_query($q);
	$data['MALE']['old'] = $data['MALE']['new'] = $data['FEMALE']['old'] = $data['FEMALE']['new'] = array();
	$reserved['Reserved-old']['MALE'] = $reserved['Reserved-new']['MALE'] = $reserved['Reserved-old']['FEMALE'] = $reserved['Reserved-new']['FEMALE'] = array();
	while($r = $result->fetchAssoc())
	{
		$data[strtoupper($r['a_gender']) == 'M'?'MALE':'FEMALE'][$r['a_old']?'old':'new'][] = $r;
		/* A cell reserved with no cell number is meaningless, and it used to hang
		   the allocation loops below: the blank lands in Reserved, and once $c
		   runs past the end of Cells the lookup yields NULL, which loosely
		   equals '' - so the while() never exits. Skip blanks. */
		if ($r['aa_cell_fixed'] && trim($r['aa_cell']) !== '')
		{
			$cell_alloc['Reserved'][$r['a_gender'] == 'M'?'MALE':'FEMALE'][] = $r['aa_cell'];
			$reserved['Reserved-'.($r['a_old']?'old':'new')][$r['a_gender'] == 'M'?'MALE':'FEMALE'][] = $r['aa_cell'];
		}
	}

	/* Remove reserved cells from the list */

	foreach( $gender as $g )
	{
		//$cell_alloc['Old'][$g] = $reserved['Reserved-old'][$g];
		//$cell_alloc['New'][$g] = $reserved['Reserved-new'][$g];
		if ( count($cell_alloc['Cells'][$g] ) > 0 )
		{
			/* Cells are given, so we fill up from old to new, no reservation for old/new separately */
			$c = 0;
			for($i=0; $i<count($data[$g]['old']) - count($reserved['Reserved-old'][$g]); $i++ )
			{
				if ( !in_array($cell_alloc['Cells'][$g][$c], array_values($cell_alloc['Reserved'][$g]) ) )
				{
					if ( $cell_alloc['Cells'][$g][$c] <> '' )
						$cell_alloc['Old'][$g][] = $cell_alloc['Cells'][$g][$c++];
				}
				else
				{
					/* $c < count() guard: past the last cell the lookup is NULL, and
					   NULL == '' loosely, so without it this loop never exits. */
					while( $c < count($cell_alloc['Cells'][$g])
					       && in_array($cell_alloc['Cells'][$g][$c], array_values($cell_alloc['Reserved'][$g]) ) )
						$c++;
					if ( $cell_alloc['Cells'][$g][$c] <> '' )
						$cell_alloc['Old'][$g][] = $cell_alloc['Cells'][$g][$c++];
				}
			}
			/* Now fill up new */
			for( ; $i<count($cell_alloc['Cells'][$g]); $i++ )
			{
				if ( !in_array($cell_alloc['Cells'][$g][$c], array_values($cell_alloc['Reserved'][$g]) ) )
				{
					if ( $cell_alloc['Cells'][$g][$c] <> '' )
						$cell_alloc['New'][$g][] = $cell_alloc['Cells'][$g][$c++];					
				}
				else
				{
					/* same guard as the 'old' loop above */
					while( $c < count($cell_alloc['Cells'][$g])
					       && in_array($cell_alloc['Cells'][$g][$c], array_values($cell_alloc['Reserved'][$g]) )  )
						$c++;
					if ( $cell_alloc['Cells'][$g][$c] <> '' )
						$cell_alloc['New'][$g][] = $cell_alloc['Cells'][$g][$c++];
				}
			}
		}
		else
		{
			for($i=0; $i<count($cell_alloc['Old'][$g]) ; $i++ )
			{
				if ( in_array($cell_alloc['Old'][$g][$i], $cell_alloc['Reserved'][$g]) )
					unset($cell_alloc['Old'][$g][$i]);
			}
			for($i=0; $i<count($cell_alloc['New'][$g]); $i++ )
			{
				if ( in_array($cell_alloc['New'][$g][$i], $cell_alloc['Reserved'][$g]) )
					unset($cell_alloc['New'][$g][$i]);
			}
		}	
	}

	if ( isset($_REQUEST['d']) && ($_REQUEST['d'] == 1))
	{
		print_r($cell_alloc);		
	}
	//exit;

	$cells[0]['MALE'] = $cells[0]['FEMALE'] = array();
	$batch_size['MALE'] = $batch_size['FEMALE'] = 0;

	/* Auto Allocate cells */

	foreach( $gender as $g )
	{
		$batch_size[$g] = count($cell_alloc['Old'][$g]); 
		$batch = 0; $count = 0;
		$keys = array_keys($cell_alloc['Old'][$g]);
		foreach($data[$g]['old'] as $r)
		{
			// Only roll into a new batch when cells exist; batch_size 0 (no cells for
			// this group) would otherwise start a fresh batch on every student.
			if ( $batch_size[$g] > 0 && $count >= $batch_size[$g] )
			{
				$batch++; $count= 0;
			}
			if ($r['aa_cell_fixed'])
			{
				$cells[$batch][$g][$r['aa_cell']] = $r['Full Name']." - ".$r['a_old'];
				continue;
			}
			$cells[$batch][$g][$cell_alloc['Old'][$g][$keys[$count]]] = $r['Full Name']." - ".$r['a_old'];
			db_update('dh_applicant_attended')->fields(array('aa_cell_group' => $batch, 'aa_cell' => $cell_alloc['Old'][$g][$keys[$count]]))->condition('aa_id', $r['aa_id'])->execute();
			$count++;
		}
		if ( isset($_REQUEST['d']) && ($_REQUEST['d'] == 1) )
		{
			print "Batch size - ".$batch_size[$g]."<br>"."Batch - $batch<br>";

		}

		/* Now we create batches for new students, so even if there are more students than no of cells, we split them into batches */
		$batch_size[$g] = count($cell_alloc['New'][$g]); //$total[$g] - count($reserved[$g]) - count($cells[0][$g]);
		//$last_cell[$g] = $cell_no[$g];
		$batch = 0; $count = 0;

		$keys = array_keys($cell_alloc['New'][$g]);
		foreach($data[$g]['new'] as $r)
		{
			// Only roll into a new batch when cells exist; batch_size 0 (no cells for
			// this group) would otherwise start a fresh batch on every student.
			if ( $batch_size[$g] > 0 && $count >= $batch_size[$g] )
			{
				$batch++; $count= 0;
			}
			if ($r['aa_cell_fixed'])
			{
				$cells[$batch][$g][$r['aa_cell']] = $r['Full Name']." - ".$r['a_old'];
				continue;
			}			
			$cells[$batch][$g][$cell_alloc['New'][$g][$keys[$count]]] = $r['Full Name']." - ".$r['a_old'];
			db_update('dh_applicant_attended')->fields(array('aa_cell_group' => $batch, 'aa_cell' => $cell_alloc['New'][$g][$keys[$count]] ))->condition('aa_id', $r['aa_id'])->execute();
			$count++;
		}

	}
	return $cells;
}

function dh_laundry_list($centre, $course)
{
	$BASE_COLUMN = 'A';
	$BASE_ROW = 16;
	$q = "select a_id, a_gender, a_old,  concat(a_f_name, ' ',a_m_name, ' ',a_l_name) as 'Full Name', TIMESTAMPDIFF( YEAR, a_dob, CURDATE() ) as 'Age', a_conf_no , ac_teen, ac_10d, ac_stp, ac_spl, ac_20d, ac_30d, ac_45d, ac_60d, ac_tsc, ac_teacher, aa_group, aa_cell, aa_cell_fixed, aa_laundry, aa_section, aa_acco from dh_applicant left join dh_applicant_course on a_id=ac_applicant left join dh_applicant_attended on a_id=aa_applicant where a_course='$course' and a_attended=1 order by a_gender, a_type, (aa_laundry*1) asc";
	$res = db_query($q);

	$template = drupal_get_path('module', 'dh_manageapp').'/templates/laundry-template.xls';
	$spreadsheet = \PhpOffice\PhpSpreadsheet\IOFactory::load($template);
	$sheet['M'] = $spreadsheet->getSheetByName('Male'); //$spreadsheet->getActiveSheet();
	$sheet['F'] = $spreadsheet->getSheetByName('Female'); //$spreadsheet->getActiveSheet();

	$m = 0; $f = 0; $i['M'] = $i['F'] = $BASE_ROW;
	while( $r = $res->fetchAssoc())
	{
		if ( $r['aa_laundry'] == '')
			continue;

		 $sheet[$r['a_gender']]->getCell('A'.$i[$r['a_gender']])->setValue( $r['aa_laundry'] );
		 $sheet[$r['a_gender']]->getCell('B'.$i[$r['a_gender']])->setValue( $r['Full Name'].' ('.$r['aa_section'].'-'.$r['aa_acco'].')' );
		 $i[$r['a_gender']]++;
	}

//	$worksheet->getCell('A1')->setValue('John');
//	$worksheet->getCell('A2')->setValue('Smith');

	$writer = \PhpOffice\PhpSpreadsheet\IOFactory::createWriter($spreadsheet, 'Xls');
	$tmpfile = tempnam(sys_get_temp_dir(), gethostname()).'.xls';
	$writer->save($tmpfile);

	$course_name = db_query("select c_name from dh_course where c_id=$course")->fetchField();
	$temp = explode("/", $course_name);


	$filename = "Laundry-".trim($temp[1])."-".trim($temp[3])."-".trim($temp[2]).".xls";
	header("Content-Type:   application/vnd.ms-excel; charset=utf-8");
	header("Content-Disposition: attachment; filename=$filename");  //File name extension was wrong
	header("Expires: 0");
	header("Cache-Control: must-revalidate, post-check=0, pre-check=0");
	header("Cache-Control: private",false);
    header('Pragma: public');
    header('Content-Length: ' . filesize($tmpfile));
    ob_clean();
    flush();
    readfile($tmpfile);
    unlink($tmpfile);
	exit;
}


function dh_valuable_list($centre, $course)
{
	$BASE_COLUMN = 'A';
	$BASE_ROW = 2;
	$ROWS_PER_PAGE = 10;
	$q = "select a_id, a_gender, a_old,  concat(a_f_name, ' ',a_m_name, ' ',a_l_name) as 'Full Name', TIMESTAMPDIFF( YEAR, a_dob, CURDATE() ) as 'Age', a_conf_no , ac_teen, ac_10d, ac_stp, ac_spl, ac_20d, ac_30d, ac_45d, ac_60d, ac_tsc, ac_teacher, aa_group, aa_cell, aa_cell_fixed, aa_laundry, aa_valuable, aa_section, aa_acco from dh_applicant left join dh_applicant_course on a_id=ac_applicant left join dh_applicant_attended on a_id=aa_applicant where a_course='$course' and a_attended=1
    order by a_gender, (aa_valuable * 1) asc";
	$res = db_query($q);

	$template = drupal_get_path('module', 'dh_manageapp').'/templates/valuable-template.xls';
	$spreadsheet = \PhpOffice\PhpSpreadsheet\IOFactory::load($template);
	$sheet['M'] = $spreadsheet->getSheetByName('Male'); //$spreadsheet->getActiveSheet();
	$sheet['F'] = $spreadsheet->getSheetByName('Female'); //$spreadsheet->getActiveSheet();

	$m = 0; $f = 0; $i['M'] = $i['F'] = $BASE_ROW;
	$token = 'A'; $name = 'B'; $c = 0; $old = 'F';
	while( $r = $res->fetchAssoc())
	{
		if ( $r['aa_valuable'] == '')
			continue;
		if ( $r['a_gender'] <> $old )
		{
			$c = 0;
			$old = 'M';		
			$token = 'A'; $name = 'B';	
		}
		if ( ($c++ % 2) == 0 )
		{
			$i[$r['a_gender']]++;
		}
			
		$sheet[$r['a_gender']]->getCell($token.$i[$r['a_gender']])->setValue( $r['aa_valuable'] );
		$sheet[$r['a_gender']]->getCell($name.$i[$r['a_gender']])->setValue( $r['Full Name'].' ('.$r['aa_section'].'-'.$r['aa_acco'].')' );
		if ($token == 'A') $token = 'E'; else $token = 'A';
		if ($name == 'B') $name = 'F'; else $name = 'B';

	}
//	drupal_set_message(print_r($i, true));
//	$worksheet->getCell('A1')->setValue('John');
//	$worksheet->getCell('A2')->setValue('Smith');

	$writer = \PhpOffice\PhpSpreadsheet\IOFactory::createWriter($spreadsheet, 'Xls');
	$tmpfile = tempnam(sys_get_temp_dir(), gethostname()).'.xls';	
	$writer->save($tmpfile);

	$course_name = db_query("select c_name from dh_course where c_id=$course")->fetchField();
	$temp = explode("/", $course_name);


	$filename = "Valuable-".trim($temp[1])."-".trim($temp[3])."-".trim($temp[2]).".xls";
	header("Content-Type:   application/vnd.ms-excel; charset=utf-8");
	header("Content-Disposition: attachment; filename=$filename");  //File name extension was wrong
	header("Expires: 0");
	header("Cache-Control: must-revalidate, post-check=0, pre-check=0");
	header("Cache-Control: private",false);
    header('Pragma: public');
    header('Content-Length: ' . filesize($tmpfile));
    ob_clean();
    flush();
    readfile($tmpfile);
    unlink($tmpfile);

	exit;
}


function dh_student_chit($centre, $course, $seating)
{
	global $base_url;
	$alpha = db_query("select cs_seat_naming_conv from dh_center_setting where cs_center=$centre")->fetchField();
	$BLOCKS_PER_PAGE = variable_get('student_chit_rows_per_page', '18');
	$gender = array('M', 'F');
	$data['M'][0] = array(); $data['F'][0] = array();
	$course_name = db_query("select c_name from dh_course where c_id=$course")->fetchField();

  $orderby = " order by a_gender, a_f_name, a_l_name ";
  if($seating)
    $orderby = " order by aa_chowky_chair desc, aa_seat_row asc, aa_seat_col asc ";

	$q = "select a_id, aa_id, a_gender, a_old,  a_f_name, a_l_name, a_conf_no , aa_group, aa_cell, aa_cell_fixed, aa_cell_group, CONCAT(aa_section,'-', aa_acco) as 'AccoNo', aa_seat_col, aa_seat_row, aa_seat, aa_chowky, aa_chair, GREATEST(aa_chowky, aa_chair) as aa_chowky_chair, aa_backrest from dh_applicant left join dh_applicant_course on a_id=ac_applicant left join dh_applicant_attended on a_id=aa_applicant where a_course='$course' and a_attended=1  and a_type='Student' $orderby";
	//ac_teacher desc, ac_60d desc, ac_45d desc, ac_30d desc, ac_20d desc, ac_tsc desc, ac_spl desc,ac_stp desc, ac_10d desc, TIMESTAMPDIFF( YEAR, a_dob, CURDATE() )  desc,
	$result = db_query($q);
	while($r = $result->fetchAssoc())
	{
		$data[strtoupper($r['a_gender'])][] = $r;
	}
	$max_cols = array('M' => array(), 'F' => array());
	$q = "SELECT a_gender, aa_seat_row, count(aa_id) as c FROM `dh_applicant_attended` aa left join dh_applicant a on (aa.aa_applicant=a.a_id) WHERE a.a_course = '$course' and a.a_attended=1 and aa.aa_chowky=0 and a.a_type='Student' group by a_gender, aa_seat_row";
	$result = db_query($q);
	while($r = $result->fetchAssoc())
	{
		$max_cols[strtoupper($r['a_gender'])][$r['aa_seat_row']]  = $r['c'];
	}	
	$out = '<html><head>';
	$out .= '<style>@import url("'.$base_url.'/sites/all/modules/dh_manageapp/css/student-chit.css");</style>';
	$out .= '</head><body>';
	$title = '<div class="header-day0"><div align="center" class="title"><b>Student Chit</b></div>'.
			'<div align="center" class="title-head">'.$course_name.'</div></div>';
	$out .= $title;
  $student_chit_as_seating = $base_url."/student-chit/$centre/$course?seating=1";
  $out .= '<button type="button" class="no-print remove-seat">Click to remove Seats</button>';
  $out .= ' <button type="button" class="no-print remove-cell">Click to remove Cells</button>';
  $out .= ' <a class="no-print" href="'.$student_chit_as_seating.'/">Click to Generate Student Chit in order of sitting plan.</a>';
	foreach( $gender as $g )
	{
		$count = 1;
		if ($g == 'F')
			$out .= '<div class="footer"></div><br>'.$title;
		$out .= '<div class="main-div">';
		foreach($data[$g] as $r)
		{
			if ($count > $BLOCKS_PER_PAGE)
			{
				$count = 1 ;
				$out .= '<div class="footer"></div><br>';
			}
			$seat_type = 'Seat';
			if ( !isset($r['a_f_name']) )
				continue;
			if ($alpha)
				$r['SeatNo'] = dh_seat_label($r['aa_seat_col'], $r['aa_seat_row'], $alpha);
			else
			{
				$offset = 0;
				if ($r['aa_seat_row'])
				{
					for ($j=0; $j<$r['aa_seat_row']; $j++)
						$offset += $max_cols[$g][$j];
				}
				$offset += $r['aa_seat_col'] + 1;
				$r['SeatNo'] = $offset;
			}
			if ($r['aa_chowky'])
				$r['SeatNo'] = 'CW-'.($r['aa_seat_row']+1);
			if ($r['aa_chair'])
				$r['SeatNo'] = 'CH-'.($r['aa_seat_row']+1);

			if ($r['aa_chowky'])
				$seat_type = 'Chowky';
			elseif ($r['aa_chair'])
				$seat_type = 'Chair';
			elseif ($r['aa_backrest'])
				$seat_type = 'Backrest';
	
			$out .= '<div class="table-student-chit"><div class="seat">'.$seat_type.': '.$r['aa_seat'].'</div><div class="name">'.$r['a_f_name'].'<br>'.$r['a_l_name'].'</div><div class="cell">Cell: '.$r['aa_cell'].'</div><div class="room">Room No.:'.$r['AccoNo'].'</div></div>';
			$count++;
		}
		$out .= '</div>';		
	}
	$out .= '</body></html>';
  $out .= '
    <script src="https://code.jquery.com/jquery-1.12.4.js"></script>
    <script>
    (function ($) {
    $(document).ready(function(){

      $(document).on("click", ".remove-seat", function(){
          $(".seat").remove();
      });

      $(document).on("click", ".remove-cell", function(){
          $(".cell").remove();
      });

    });
    })(jQuery);
    </script>
    ';
	print $out;
	exit;
}


function dh_checking_slip($centre, $course, $seating)
{
  global $base_url;
  $alpha = db_query("select cs_seat_naming_conv from dh_center_setting where cs_center=$centre")->fetchField();
  $BLOCKS_PER_PAGE = variable_get('checking_slip_per_page', '6');
  $gender = array('M', 'F');
  $data['M'][0] = array(); $data['F'][0] = array();
  $course_name = db_query("select c_name from dh_course where c_id=$course")->fetchField();

  $orderby = " order by a_gender, a_f_name, a_l_name ";
  if($seating)
    $orderby = " order by aa_chowky_chair desc, aa_seat_row asc, aa_seat_col asc ";

  $q = "select a_id, aa_id, a_gender, a_old,  a_f_name, a_l_name, a_conf_no , aa_group, aa_cell, aa_cell_fixed, aa_cell_group, CONCAT(aa_section,'-', aa_acco) as 'AccoNo', aa_seat_col, aa_seat_row, aa_seat, aa_chowky, aa_chair, GREATEST(aa_chowky, aa_chair) as aa_chowky_chair, aa_backrest from dh_applicant left join dh_applicant_course on a_id=ac_applicant left join dh_applicant_attended on a_id=aa_applicant where a_course='$course' and a_attended=1  and a_type='Student' $orderby";
  //ac_teacher desc, ac_60d desc, ac_45d desc, ac_30d desc, ac_20d desc, ac_tsc desc, ac_spl desc,ac_stp desc, ac_10d desc, TIMESTAMPDIFF( YEAR, a_dob, CURDATE() )  desc,
  $result = db_query($q);
  while($r = $result->fetchAssoc())
  {
    $data[strtoupper($r['a_gender'])][] = $r;
  }
  $max_cols = array('M' => array(), 'F' => array());
  $q = "SELECT a_gender, aa_seat_row, count(aa_id) as c FROM `dh_applicant_attended` aa left join dh_applicant a on (aa.aa_applicant=a.a_id) WHERE a.a_course = '$course' and a.a_attended=1 and aa.aa_chowky=0 and a.a_type='Student' group by a_gender, aa_seat_row";
  $result = db_query($q);
  while($r = $result->fetchAssoc())
  {
    $max_cols[strtoupper($r['a_gender'])][$r['aa_seat_row']]  = $r['c'];
  }
  $out = '<html><head>';
  $out .= '<style>@import url("'.$base_url.'/sites/all/modules/dh_manageapp/css/checking-slip.css");</style>';
  $out .= '</head><body>';
  $title = '<div class="header-day0"><div align="center" class="title"><b>Student Chit</b></div>'.
      '<div align="center" class="title-head">'.$course_name.'</div></div>';
  $out .= $title;
  $student_chit_as_seating = $base_url."/checking-slip/$centre/$course?seating=1";
  $out .= ' <a class="no-print" href="'.$student_chit_as_seating.'/">Click to Generate Checking Slip in order of sitting plan.</a>';
  $out .= '<div class="no-print">
            <label for="eng-place-txt">English-Text:</label>
            <input type="text" id="eng-place-txt" name="eng-place-txt">
            <label for="hin-place-txt">Hindi-Text:</label>
            <input type="text" id="hin-place-txt" name="hin-place-txt">
            <button type="button" class="replace-text">Replace empty location with provided text</button>
          </div>';
  foreach( $gender as $g )
  {
    $count = 1;
    if ($g == 'F')
      $out .= '<div class="footer"></div><br>'.$title;
    $out .= '<div class="main-div">';
    foreach($data[$g] as $r)
    {
      if ($count > $BLOCKS_PER_PAGE)
      {
        $count = 1 ;
        $out .= '<div class="footer"></div><br>';
      }
      $seat_type = 'Seat';
      if ( !isset($r['a_f_name']) )
        continue;
      if ($alpha)
        $r['SeatNo'] = dh_seat_label($r['aa_seat_col'], $r['aa_seat_row'], $alpha);
      else
      {
        $offset = 0;
        if ($r['aa_seat_row'])
        {
          for ($j=0; $j<$r['aa_seat_row']; $j++)
            $offset += $max_cols[$g][$j];
        }
        $offset += $r['aa_seat_col'] + 1;
        $r['SeatNo'] = $offset;
      }
      if ($r['aa_chowky'])
        $r['SeatNo'] = 'CW-'.($r['aa_seat_row']+1);
      if ($r['aa_chair'])
        $r['SeatNo'] = 'CH-'.($r['aa_seat_row']+1);

      if ($r['aa_chowky'])
        $seat_type = 'Chowky';
      elseif ($r['aa_chair'])
        $seat_type = 'Chair';
      elseif ($r['aa_backrest'])
        $seat_type = 'Backrest';

      $out .= '<div class="table-student-chit"><div class="name">'.mb_substr($r['a_f_name'].' '.$r['a_l_name'], 0, 25).' ('.$seat_type.': '.$r['aa_seat'].')'.'</div><div class="room">Room No.:'.$r['AccoNo'].'</div><div class="cell">Cell: '.$r['aa_cell'].'</div><div class="linebreak"><hr></div>
        <div class="message-english">Kindly meet the teacher TODAY at _______ AM/PM at Teacher\'sRoom / InterviewRoom / DhammaHall / MiniHall / <span class="eng-place">_____________ </span>, bring this note with you. Be Happy.</div>
        <div class="message-hindi">कृपया आचार्य / आचार्या से भेंट के लिये आज सुबह / दोपहर _______ बजे आचार्यनिवास / मुलाकातकक्ष / धम्माहॉल / मिनीहॉल / <span class="hin-place">_____________</span> में जायें, यह परची अपने साथ में लायें | मंगल हो | </div>
        </div>';
      $count++;
    }
    $out .= '</div>';
  }
  $out .= '</body></html>';
  $out .= '
    <script src="https://code.jquery.com/jquery-1.12.4.js"></script>
    <script>
    (function ($) {
    $(document).ready(function(){

      $(document).on("click", ".remove-seat", function(){
          $(".seat").remove();
      });

      $(document).on("click", ".remove-cell", function(){
          $(".cell").remove();
      });

      $(document).on("click", ".replace-text", function(){
          $(".eng-place").html($("#eng-place-txt").val());
          $(".hin-place").html($("#hin-place-txt").val());
      });

    });
    })(jQuery);
    </script>
    ';
  print $out;
  exit;
}


function course_archive_lists($centre, $course)
{
	$prefix = "$centre-$course";
	dh_zero_day_list($centre, $course, $course, $prefix."-zero.pdf" );
	dh_generate_teacher_list($centre, $course);
	dh_generate_manager_list($centre, $course);
	dh_cell_list($centre, $course);
	dh_generate_seating_plan($centre, $course);
}

/* Sometimes new students can have course data, when old becomes new etc, a bit crazy  */
function zeroize_new_course_data($centre, $course)
{
	//$q = "update dh_applicant_course left join dh_applicant on ac_applicant=a_id set ac_10d=0, ac_stp=0, ac_spl=0, ac_20d=0, ac_30d=0, ac_tsc=0, ac_45d=0, ac_teacher=0 where a_center=$centre and a_course=$course and a_old=0 and ac_10d IS NOT NULL";
	$q = "delete dh_applicant_course from dh_applicant_course left join dh_applicant on ac_applicant=a_id where a_center=$centre and a_course=$course and a_old=0 and ac_10d IS NOT NULL";
	db_query($q);
}


