<?php


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
			zeroize_new_course_data(arg(1), arg(2));
			dh_generate_teacher_list(arg(1), arg(2));
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
		case 'assign-teacher':
			return dh_assign_teacher(arg(1), arg(2));
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
			return dh_student_chit(arg(1), arg(2));
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
	$header = array('Action', 'ConfNo', 'Applicant Name', 'Gender', 'City', 'Age');
	$attributes = array('id' => 'table-applicants');
	$out = theme('table', array('header' => $header, 'rows' => $rows, 'attributes' => $attributes));
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
	$q = "select count(a_id) as 'c', a_gender, a_old from dh_applicant where a_course='$course_id' and a_status in ('Confirmed', 'Expected') group by a_gender, a_old";
	$res = db_query($q);
	$conf = array();
	while($r = $res->fetchAssoc())
	{
		$conf[$r['a_old']?'old':'new'][$r['a_gender']] = $r['c'];
	}

	//$q = "select a_id, a_center, a_gender, a_old, a_conf_no as 'ConfNo', concat(a_f_name, ' ',a_m_name, ' ',a_l_name) as 'Full Name', TIMESTAMPDIFF( YEAR, a_dob, CURDATE() ) as 'Age', ac_10d, ac_stp, ac_spl, ac_20d, ac_30d, ac_45d, ac_60d, ac_tsc, ac_teacher, aa_group, aa_acco, aa_section, aa_laundry, aa_chowky, aa_valuable, aa_chowky, aa_chair, aa_backrest from dh_applicant left join dh_applicant_course on a_id=ac_applicant left join dh_applicant_attended on a_id=aa_applicant where a_course='$course_id' and aa_acco IS NOT NULL order by a_f_name, a_m_name, a_l_name";
	$q = "select a_id, a_type, a_center, a_gender, a_old, a_conf_no as 'ConfNo', concat(a_f_name, ' ',a_m_name, ' ',a_l_name) as 'Full Name', TIMESTAMPDIFF( YEAR, a_dob, CURDATE() ) as 'Age', a_uri, ac_teen, ac_10d, ac_stp, ac_spl, ac_20d, ac_30d, ac_45d, ac_60d, ac_tsc, ac_teacher, aa_group, aa_acco, aa_section, aa_laundry, aa_cell, aa_cell_fixed, aa_comment, aa_valuable, aa_chowky, aa_chair, aa_backrest from dh_applicant left join dh_applicant_course on a_id=ac_applicant left join dh_applicant_attended on a_id=aa_applicant where a_course='$course_id' and a_attended=1 order by a_f_name, a_m_name, a_l_name";
	$result = db_query($q);
	$rows = array();
	$totals['new']['F'] = $totals['old']['F'] = $totals['new']['M'] = $totals['old']['M'] = 0; 
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

		$totals[$r['a_old']?'old':'new'][strtoupper($r['a_gender'])] += 1;
		$special[$r['a_old']?'old':'new'][strtoupper($r['a_gender'])]['chowky'] += $r['aa_chowky'];
		$special[$r['a_old']?'old':'new'][strtoupper($r['a_gender'])]['chair'] += $r['aa_chair'];
		$special[$r['a_old']?'old':'new'][strtoupper($r['a_gender'])]['backrest'] += $r['aa_backrest'];
		if ( !isset($group[$r['aa_group']][$r['a_old']?'old':'new'][strtoupper($r['a_gender'])] ) )
			$group[$r['aa_group']][$r['a_old']?'old':'new'][strtoupper($r['a_gender'])] = 0;
		$group[$r['aa_group']][$r['a_old']?'old':'new'][strtoupper($r['a_gender'])] += 1;
	}
	$header = array('Update', 'ConfNo', 'Applicant Name', 'Gender', 'Type', 'Age', 'Teen/10D/STP', 'LC', 'RoomNo', 'Laundry', 'Valuable', 'Chowky', 'Chair', 'BackRest', 'Group', 'H' );
	$attributes = array('id' => 'table-attending');

	$out = '<br /><br /><h3>Attending Applicants  ( '.l("Assign Teacher", "assign-teacher/".$centre."/".$course_id).' | '.l("Teacher List", "teacher-list/".$centre."/".$course_id).' | '.l("Manager List", "manager-list/".$centre."/".$course_id).' | '.l("Seating Plan", "seating/".$centre."/".$course_id).' ('.l("Re-Gen", "seating/".$centre."/".$course_id, array("query" => array("r" => 1))).')  ) </h3>';
	if (!isset($totals['new']['M'])) $totals['new']['M'] = 0;
	if (!isset($totals['new']['F'])) $totals['new']['F'] = 0;
	$all_total = $totals['new']['M']+$totals['old']['M']+$totals['new']['F']+$totals['old']['F'];
	$header_totals = array('', 'Old', 'New', 'Total');
	$rows_totals[] = array( 'Attended Male', $totals['old']['M'], $totals['new']['M'],'<b>'.($totals['new']['M']+$totals['old']['M']).'<b>');
	$rows_totals[] = array( 'Attended Female',$totals['old']['F'] , $totals['new']['F'],'<b>'.($totals['new']['F']+$totals['old']['F']).'</b>');
	$rows_totals[] = array( '<b>Total</b>','' , '', "<b>$all_total</b>" );

	$header_totals = array('', 'Old', 'New', 'Total');
	if (!isset($conf['new']['M'])) $conf['new']['M'] = 0;
	if (!isset($conf['new']['F'])) $conf['new']['F'] = 0;
	$all_conf_total = $conf['new']['M']+$conf['old']['M']+$conf['new']['F']+$conf['old']['F'];
	$rows_conf[] = array( 'Confirmed Male', $conf['old']['M'], $conf['new']['M'],'<b>'.($conf['new']['M']+$conf['old']['M']).'<b>');
	$rows_conf[] = array( 'Confirmed Female',$conf['old']['F'] , $conf['new']['F'],'<b>'.($conf['new']['F']+$conf['old']['F']).'</b>');
	$rows_conf[] = array( '<b>Total</b>','' , '', "<b>$all_conf_total</b>" );


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

	ksort($group);
	$header_group = array('Group No', 'NM', 'OM', 'Total', '&nbsp;&nbsp;', 'NF', 'OF', 'Total');
	$rows_group = array(); $row = array();
	foreach( $group as $g_no => $v )
	{
		if ( ! isset($group[$g_no]['old']['M']) ) $group[$g_no]['old']['M'] = 0;
		if ( ! isset($group[$g_no]['new']['M']) ) $group[$g_no]['new']['M'] = 0;
		if ( ! isset($group[$g_no]['old']['F']) ) $group[$g_no]['old']['F'] = 0;
		if ( ! isset($group[$g_no]['new']['F']) ) $group[$g_no]['new']['F'] = 0;
		unset($row);
		$row[] = '<b>'.$g_no.'</b>';
		$row[] = $group[$g_no]['new']['M'];
		$row[] = $group[$g_no]['old']['M'];
		$row[] = '<b>'.($group[$g_no]['old']['M'] + $group[$g_no]['new']['M']).'</b>';
		$row[] = '&nbsp;&nbsp;';
		$row[] = $group[$g_no]['new']['F'];
		$row[] = $group[$g_no]['old']['F'];
		$row[] = '<b>'.($group[$g_no]['old']['F'] + $group[$g_no]['new']['F']).'</b>';
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
	if (count($rows) > 0 )
		$out .= theme('table', array('header' => $header, 'rows' => $rows, 'attributes' => $attributes));
	return $out;
}

function dh_assign_teacher_form($form, &$form_state)
{
	$form['ct_teacher'] = array( '#title' => 'Teacher',  '#type' => 'textfield', 
		'#weight' => 1, '#size' => 50, '#required' => 1, 
 		'#attached' => array(
      		'library' => array(
        		array('system', 'ui.autocomplete'),
      	)),
	 ) ;
	$types = array('Conducting' => 'Conducting', 'Assisting' => 'Assisting', 'Training' => 'Training' );
	$form['ct_type'] = array( '#title' => 'Type', '#type' => 'select', '#options' => $types, 
		'#weight' => 2);
	$groups = array();
	for($i=1; $i<10; $i++)
		$groups[$i] = $i;
	$form['ct_group'] = array( '#title' => 'Group', '#type' => 'select', '#options' => $groups, '#weight' => 3);
	$form['sub'] = array('#type' => 'submit', '#value' => 'Assign Teacher', '#weight' => 4);
	return $form;
}

function dh_assign_teacher_form_submit($form, &$form_state)
{
	global $user;
	$input = $form_state['input'];
	$temp = explode("-", $input['ct_teacher']);
	$tid = db_query("select t_id from dh_teacher where t_code='".$temp[0]."' and t_gender='".$temp[1]."'")->fetchField();
	$teacher['ct_teacher'] = $tid;
	$teacher['ct_type'] = $input['ct_type'];
	$teacher['ct_group'] = $input['ct_group'];
	$teacher['ct_created_by'] = $user->uid;
	$teacher['ct_updated_by'] = $user->uid;
	$teacher['ct_updated'] = date("Y-m-d H:i:s");
	$teacher['ct_course'] = arg(2);
	$teacher['ct_status'] = 'Confirmed';

	db_insert('dh_course_teacher')->fields($teacher)->execute();
}

function dh_assign_teacher($centre, $course)
{
	$finalized = db_query("select c_finalized from dh_course where c_id=".$course)->fetchField();
	if ( $finalized )
	{
		drupal_set_message("Course has been Finalized, cannot access Assign Teacher", "error");
		drupal_goto("course/".$centre."/".$course);
	}


	drupal_add_css(drupal_get_path( 'module', 'dh_manageapp').'/css/manageapp.css');
	//drupal_add_js(drupal_get_path('module', 'dh_manageapp').'/js/manageapp.js');
	drupal_add_js('

		(function ($) {
			$(document).ready(function(){
				$("#edit-ct-teacher").autocomplete({
					source: function( request, response ) {
						$("#edit-ct-teacher").addClass( "throbbing" );
				        $.ajax({
				          url: "/autocomplete/get-teacher/" + request.term,
				          dataType: "json",
				          success: function( data ) {
				          	$("#edit-ct-teacher").removeClass( "throbbing" );
				            response( data );
				          }
				        });
					},
		  			minLength: 1,
		  			select: function( event, ui ) {
		        		//console.log( "Selected: " + ui.item.value + " aka " + ui.item.code );
		        		$( "#edit-ct-teacher" ).val( ui.item.code );
		        		return false;
		      		}
		  		});

				$(".delete-confirm").click(function(){
					  var ret;
					  ret = confirm("Are you sure you want to delete ?");
					  return ret;
				});				
			})
		})(jQuery);


		', 'inline');

	$course_name = db_query("select c_name from dh_course where c_id=$course")->fetchField();
	$temp = drupal_get_form('dh_assign_teacher_form');
	$out = '<h2>'.$course_name.'</h2>';
	$out .= '<h2>'.l("Back to Course", "course/$centre/$course")."&nbsp;&nbsp;";
	if ( strpos($_SERVER['HTTP_REFERER'], "at-schedule") )
		$out .= " | &nbsp;&nbsp; ".l("Back to AT Schedule", "at-schedule/$centre");
	else
		$out .=  " | &nbsp;&nbsp; ".l("Back to Zero Day", "zero-day/$centre/$course");
	$out .= '</h2>';
	$out .= drupal_render( $temp );
	$q = "select CONCAT(t_f_name, ' ', t_l_name ) as 'teacher', ct_type, ct_group, ct_id from dh_course_teacher left join dh_teacher on ct_teacher=t_id where ct_course=$course";
	$res = db_query($q);
	$rows = array();
	while($r = $res->fetchAssoc())
	{
		$r['ct_id'] = l("Delete", "assign-teacher/delete/$centre/$course/".$r['ct_id'], array('attributes' => array('class' => array('delete-confirm')))) ;  
		$rows[] = $r;
	}
	$header = array("Teacher", "Type", "Group", "Delete");
	$out .= theme("table", array('header' => $header, 'rows' => $rows ));
	return $out;
}


function dh_assign_teacher_delete()
{
	$id = arg(4);
	db_query("delete from dh_course_teacher where ct_id=$id");
	drupal_goto('assign-teacher/'.arg(2).'/'.arg(3));
}

function dh_generate_teacher_list($centre, $course, $pdf = '')
{
	//drupal_add_css();
	global $base_url;
	$out = '<style>@import url("'.$base_url.'/sites/all/modules/dh_manageapp/css/teacher-list.css");</style>';

	$q = "select ct_type, ct_group, t_code, CONCAT(t_f_name, ' ',t_l_name) as 't_name', t_gender from dh_course_teacher ct left join dh_teacher t on ct_teacher=t_id where ct_course='$course' and ct_status='Confirmed'";
	$t_hand = db_query($q);
	while($t_r = $t_hand->fetchAssoc())
	{
		$teacher[strtoupper($t_r['t_gender'])][$t_r['ct_group']] = $t_r;
		//$teacher[strtoupper($t_r['ct_type'])][$t_r['ct_group']] = $t_r;
	}
	if (! isset($teacher['M']) )
	{
		if ( isset( $teacher['F']) )
			$teacher['M'][1] = $teacher['F'][1];
		else
			$teacher['M'][1] = '';
	}

	if( !isset($teacher['F']))
		$teacher['F'][1] = $teacher['M'][1];

	$course_name = db_query("select c_name from dh_course where c_id=$course")->fetchField();
	//$q = "select a_id, a_gender, a_old,  concat(a_f_name, ' ',a_m_name, ' ',a_l_name) as 'Full Name', TIMESTAMPDIFF( YEAR, a_dob, CURDATE() ) as 'Age', ac_10d, ac_stp, ac_spl, ac_20d, ac_30d, ac_45d, ac_60d, ac_tsc, ac_teacher, aa_group, aa_acco, aa_section, aa_laundry, aa_chowky, aa_valuable, aa_chowky, aa_chair, aa_backrest, a_occupation, a_education, a_langs, aa_comment, aa_cell, c_name from dh_applicant left join dh_applicant_course on a_id=ac_applicant left join dh_applicant_attended on a_id=aa_applicant left join dh_city on a_city=c_id where a_course='$course' and aa_acco IS NOT NULL order by a_gender, ac_teacher desc, ac_60d desc, ac_45d desc, ac_30d desc, ac_20d desc, ac_tsc desc, ac_spl desc,ac_stp desc, ac_10d desc, TIMESTAMPDIFF( YEAR, a_dob, CURDATE() ) desc";
	$q = "select a_id, a_gender, a_old,  concat(a_f_name, ' ',a_m_name, ' ',a_l_name) as 'Full Name', TIMESTAMPDIFF( YEAR, a_dob, CURDATE() ) as 'Age', ac_teen, ac_10d, ac_stp, ac_spl, ac_20d, ac_30d, ac_45d, ac_60d, ac_tsc, ac_teacher, aa_group, aa_acco, aa_section, aa_laundry, aa_chowky, aa_valuable, aa_chowky, aa_chair, aa_backrest, a_occupation, a_education, a_langs, aa_comment, aa_cell, c_name, ae_pregnant, ae_pregnant_detail, ae_desc_physical, ae_desc_mental, ae_desc_addiction_past, ae_desc_addiction_current, ae_desc_other_technique, ae_desc_other_technique_old, ae_desc_medication, ae_nationality from dh_applicant left join dh_applicant_course on a_id=ac_applicant left join dh_applicant_attended on a_id=aa_applicant left join dh_applicant_extra ae on a_id=ae_applicant left join dh_city on a_city=c_id left join dh_teacher t on ( ac_teacher_code = CONCAT(t_code,'-',t_gender ))  where a_course='$course' and a_attended=1 and a_type='Student' order by a_gender, aa_group, IFNULL(ac_teacher,0) desc, IFNULL(t_seniority,0) desc, IFNULL(t_year_appointed,".date("Y").") , IFNULL(ac_60d,0) desc, IFNULL(ac_45d,0) desc, IFNULL(ac_30d,0) desc, IFNULL(ac_20d,0) desc, IFNULL(ac_tsc,0) desc, IFNULL(ac_spl,0) desc, IFNULL(ac_stp,0) desc, IFNULL(ac_10d,0) desc, IFNULL(ac_teen,0) desc, TIMESTAMPDIFF( YEAR, a_dob, CURDATE() ) desc";
	$result = db_query($q);
	$data = array();
	while($r = $result->fetchAssoc())
	{
		$data[strtoupper($r['a_gender'])][$r['a_old']?'old':'new'][$r['aa_group']][] = $r;
	}

	$header = array('SrNo', 'Student', 'RoomNo', 'Age', 'City', 'LongCourses', '10Day', 'CellNo', 'Occupation', 'Education', 'Languages', 'Comments');
	$attributes = array('class' => array('table-teacher-list') );
	$title = '<div class="header-teacher"><div align="center" class="title"><b>Teacher List</b></div>'.
			'<div align="center" class="title-head">'.$course_name.'</div></div>';

	$gender = array('M', 'F');
	$type = array('old', 'new');
	$i = 1;
	foreach($gender as $g)
	{
		foreach ($type as $t) 
		{
			if (!isset($data[$g][$g]))
				$data[$g][$t][] = array();

			foreach ($data[$g][$t] as $group => $group_data) 
			{
				unset($rows); $total =0 ; $i=1;
				foreach( $group_data as $r)
				{
					unset($row);
					$row[] = $i++;
					$row[] = $r['Full Name'];
					$row[] = $r['aa_section'].'-'.$r['aa_acco'];
					$row[] = $r['Age'];
					$row[] = $r['c_name'];
					$row[] = $r['ac_spl']+$r['ac_20d']+$r['ac_30d']+$r['ac_45d']+$r['ac_60d']+$r['ac_tsc'];
					$row[] = $r['ac_teen']+$r['ac_10d']+$r['ac_stp'];
					$row[] = $r['aa_cell'];
					$row[] = $r['a_occupation'];
					$row[] = $r['a_education'];
					$row[] = $r['a_langs'];
					$comment = '';
					if ($r['ae_nationality'] <> '')
						$comment .= 'Nationality: '.$r['ae_nationality'].'<br />';
					if (($g == 'F') && ($r['ae_pregnant']))
						$comment .= 'Pregnant: '.$r['ae_pregnant_detail']." ";
					if ($r['ae_desc_physical'] <> '')
						$comment .= ' '.$r['ae_desc_physical'];
					if ($r['ae_desc_mental'] <> '')
						$comment .= ' '.$r['ae_desc_mental'];
					if ($r['ae_desc_addiction_past'] <> '')
						$comment .= $r['ae_desc_addiction_past'];
					if ($r['ae_desc_addiction_current'] <> '')
						$comment .= ' '.$r['ae_desc_addiction_current'];
					if ($r['ae_desc_medication'] <> '')
						$comment .= ' '.$r['ae_desc_medication'];
					if ($r['ae_desc_other_technique'] <> '')
						$comment .= ' '.$r['ae_desc_other_technique'];

					$comment .= $r['aa_comment'];
					$row[] = $comment;
					$rows[] = $row;
					$total++;					
				}
				if ($total > 0 )
				{
					$out .= $title.'<br /><div class="title-info">AT: '.$teacher[$g][$group]['t_name'].'['.$teacher[$g][$group]['t_code'].'] | Gender: '.(($g=='M')?'MALE':'FEMALE').' | Type: '.strtoupper($t).' | Group: '.$group.' | Total: '.$total.'</div><br>';
					$out .= theme('table',array('header' => $header, 'rows' => $rows, 'attributes' => $attributes) );
					$out .= '<div id="footer">&nbsp;</div>';				
				}

			}
		}
	}
	if ($pdf <> '')
		create_pdf_from_html($out, $pdf);
	else
	{
		print $out;
		exit;		
	}
}


function dh_generate_manager_list($centre, $course, $pdf = '')
{
	global $base_url;
	$out = '<style>@import url("'.$base_url.'/sites/all/modules/dh_manageapp/css/teacher-list.css");</style>';
	
	$q = "select ct_type, ct_group, t_code, CONCAT(t_f_name, ' ',t_l_name) as 't_name', t_gender from dh_course_teacher ct left join dh_teacher t on ct_teacher=t_id where ct_course='$course' and ct_status='Confirmed'";
	$t_hand = db_query($q);
	while($t_r = $t_hand->fetchAssoc())
	{
		//$teacher[strtoupper($t_r['t_gender'])][$t_r['ct_group']] = $t_r;
		$teacher[strtoupper($t_r['ct_type'])][$t_r['ct_group']] = $t_r;
		$teacher_m[strtoupper($t_r['t_gender'])][$t_r['ct_group']] = $t_r;
	}
	if( !isset($teacher_m['F']))
		$teacher_m['F'][1] = $teacher_m['M'][1];

	$gender = array('M', 'F');
	foreach($gender as $g)
	{
		if ( !isset($teacher_m[$g]))
			continue;
		//if ( !isset($teacher[($g=='M')?'CONDUCTING':'ASSISTING']) )
		//	continue;
		$t_info[$g] = '<div class="teachers-title"> ';
		//foreach ($teacher[($g=='M')?'CONDUCTING':'ASSISTING'] as $key => $value)
		foreach ($teacher_m[$g] as $key => $value) 
		{
			$t_info[$g] .= $value['t_name'].' - ['.$value['t_code'].']<br />';
		}
		$t_info[$g] .= '</div>';
	}

	$course_name = db_query("select c_name from dh_course where c_id=$course")->fetchField();
	//$q = "select a_id, a_gender, a_old,  concat(a_f_name, ' ',a_m_name, ' ',a_l_name) as 'Full Name', TIMESTAMPDIFF( YEAR, a_dob, CURDATE() ) as 'Age', aa_group, aa_acco, aa_section, aa_laundry, aa_chowky, aa_valuable, aa_chowky, aa_chair, aa_backrest, a_occupation, a_education, a_langs, aa_comment, aa_cell, c_name from dh_applicant left join dh_applicant_attended on a_id=aa_applicant left join dh_city on a_city=c_id where a_course='$course' and aa_acco IS NOT NULL order by aa_group, a_f_name, a_m_name, a_l_name";
	$q = "select a_id, a_gender, a_old, a_type, concat(a_f_name, ' ',a_m_name, ' ',a_l_name) as 'Full Name', TIMESTAMPDIFF( YEAR, a_dob, CURDATE() ) as 'Age', aa_group, aa_acco, aa_section, aa_laundry, aa_chowky, aa_valuable, aa_chowky, aa_chair, aa_backrest, a_emergency_name, a_emergency_relation, a_emergency_num, a_langs, aa_comment, aa_cell, aa_seat_row, aa_seat_col, c_name from dh_applicant left join dh_applicant_attended on a_id=aa_applicant left join dh_city on a_city=c_id where a_course='$course' and a_attended=1 order by a_type desc, aa_group, a_f_name, a_m_name, a_l_name";
	$result = db_query($q);
	$data = array();
	while($r = $result->fetchAssoc())
	{
		$data[strtoupper($r['a_gender'])][] = $r;
	}

	$header = array('SrNo', 'Student', 'Age', 'Room', 'EmergencyName', 'EmergencyNum',  'Cell',  'Group', 'Set', 'Seat' );
	$attributes = array('class' => array('table-manager-list') );
	$title = '<div class="header-manager"><div align="center" class="title"><b>Manager List</b></div>'.
			'<div align="center" class="title-head">'.$course_name.'</div></div>';

	$i = 1;

	foreach($gender as $g)
	{
		unset($rows); $total =0 ; $i=1; $rows = array();
		if (isset($data[$g]))
			foreach ($data[$g] as $r) 
			{
				if (!isset($teacher_m[$g][$r['aa_group']]['t_code']))
					$teacher_m[$g][$r['aa_group']]['t_code'] = '';
				unset($row);
				$group = 'N';
				if ($r['a_old'])
					$group = 'O';
				$group .= $g;
				$row[] = $i++;
				$sevak = '';
				if ($r['a_type'] == 'Sevak')
			      $sevak .= ' (Sevak)';
				$row[] = $r['Full Name'].$sevak;
				//$row[] = $r['c_name'];
				$row[] = $r['Age'];
				$row[] = $r['aa_section'].'-'.$r['aa_acco'];
				$row[] = $r['a_emergency_name'].' ('.$r['a_emergency_relation'].')';
				$row[] = $r['a_emergency_num'];
				$row[] = $r['aa_cell'];
				$row[] = $group;
				$row[] = '['.$teacher_m[$g][$r['aa_group']]['t_code'].']';
				$extra = '';
				if ($r['aa_chowky']) $extra .= '(CW)';
				if ($r['aa_chair']) $extra .= '(CH)';
				if ($r['aa_backrest']) $extra .= '(BR)';
				$row[] = 'R'.($r['aa_seat_row']+1).",C".($r['aa_seat_col']+1).$extra;
				$rows[] = $row;
				$total++;
			}
		$out .= $title.'<br /><div class="title-info">Gender: '.(($g=='M')?'MALE':'FEMALE').'  | Total: '.$total.'</div><br>';
		$out .= $t_info[$g].theme('table',array('header' => $header, 'rows' => $rows, 'attributes' => $attributes) );
		$out .= '<div id="footer">&nbsp;</div>';
	}

	if ($pdf <> '')
		create_pdf_from_html($out, $pdf);
	else
	{
		print $out;
		exit;
	}
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
    $( ".sortable-m" ).sortable({ items: "td" });
    $( ".sortable-m" ).disableSelection();
    $( ".sortable-f" ).sortable({ items: "td" });
    $( ".sortable-f" ).disableSelection();
    $(".chowky-div").sortable({ items: "td" });
    $(".chowky-div").disableSelection();
  } );
  </script>
';

	$settings = db_query("select cs_seat_config from dh_center_setting where cs_center=$centre")->fetchField();
	$alpha = db_query("select cs_seat_naming_conv from dh_center_setting where cs_center=$centre")->fetchField();
	$ini = array();
	if ($settings <> '')
		$ini = parse_ini_string($settings, true, INI_SCANNER_RAW);
	if (!isset($ini['LEFT']['gender']))
		$ini['LEFT']['gender'] = 'F';
	if (!isset($ini['RIGHT']['gender']))
		$ini['RIGHT']['gender'] = 'M';

	$out .= '<style>@import url("'.$base_url.'/sites/all/modules/dh_manageapp/css/seating.css");</style></head><body>';
	if ( is_array($course) )
	{
		$course_name = '';
		$h = db_query("select c_name from dh_course where c_id in (".implode(",", $course).")");
		while($r = $h->fetchAssoc())
			$course_name .= $r['c_name']." / ";
	}

	$q = "select a_id, aa_id, a_gender, a_old,  concat(a_f_name, ' ',a_l_name) as 'Full Name', TIMESTAMPDIFF( YEAR, a_dob, CURDATE() ) as 'Age', ac_teen, ac_10d, ac_stp, ac_spl, ac_20d, ac_30d, ac_45d, ac_60d, ac_tsc, ac_teacher, aa_group, aa_acco, aa_section, aa_laundry, aa_valuable, aa_chowky, aa_chair, aa_backrest, aa_seat_row, aa_seat_col  from dh_applicant left join dh_applicant_course on a_id=ac_applicant left join dh_applicant_attended on a_id=aa_applicant  left join dh_teacher t on ( ac_teacher_code = CONCAT(t_code,'-',t_gender )) where a_course in (".implode(",", $course).") and a_attended=1 and a_type='Student' order by  a_gender, IFNULL(ac_teacher,0) desc, IFNULL(t_seniority,0) desc, IFNULL(t_year_appointed,".date("Y").") , IFNULL(ac_60d,0) desc, IFNULL(ac_45d,0) desc, IFNULL(ac_30d,0) desc, IFNULL(ac_20d,0) desc, IFNULL(ac_tsc,0) desc, IFNULL(ac_spl,0) desc, IFNULL(ac_stp,0) desc, IFNULL(ac_10d,0) desc, IFNULL(ac_teen,0) desc, TIMESTAMPDIFF( YEAR, a_dob, CURDATE() ) desc";
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
	//$seats['seats'] = dh_allocate_seats($data['M'], $seat_alloc['SeatsPerRow'][$place], $special_rows['seats'][$place] );
	//$seats['chowky'] = dh_allocate_seats($chowky['M'], $seat_alloc['SeatsPerRowChowky'][$place], $special_rows['chowky'][$place] );
	if ($ini['RIGHT']['gender'] == 'M')
		$male_dir = LEFT_TO_RIGHT;
	else
		$male_dir = RIGHT_TO_LEFT;

	$out .= '<div class="teacher-seat">Seating Plan - MALE<br>'.$course_name.'</div>';	
	$out_seats = dh_print_seats($seats['M']['seats'], $max_col['M']+1, $male_dir, 'main-div', $alpha);
	$out_chowky = dh_print_seats($seats['M']['chowky'], $max_col_chowky['M']+1, $male_dir, 'chowky-div', $alpha);		
	if ($male_dir == LEFT_TO_RIGHT)
		$out .= $out_seats.$out_chowky;
	else
		$out .= $out_chowky.$out_seats;

	$out .= '<div class="teacher-seat">TEACHER</div>';
	$out .= '<div class="footer"></div>';

	if ($ini['LEFT']['gender'] == 'F')
		$female_dir = RIGHT_TO_LEFT;
	else
		$female_dir = LEFT_TO_RIGHT;

	$place = 'LEFT';
	//$seats['seats'] = dh_get_seats($data['F'], $seat_alloc['SeatsPerRow'][$place], $special_rows['seats'][$place] );
	//$seats['chowky'] = dh_get_seats($chowky['F'], $seat_alloc['SeatsPerRowChowky'][$place], $special_rows['chowky'][$place] );
	$out .= '<div class="teacher-seat">Seating Plan - FEMALE<br>'.$course_name.'</div>';	
	$out_chowky = dh_print_seats($seats['F']['chowky'], $max_col_chowky['F']+1, $female_dir, 'chowky-div', $alpha);
	$out_seats = dh_print_seats($seats['F']['seats'], $max_col['F']+1, $female_dir, 'main-div', $alpha);
	if ($female_dir == LEFT_TO_RIGHT)
		$out .= $out_seats.$out_chowky;
	else
		$out .= $out_chowky.$out_seats;

	$out .= '<div class="teacher-seat">TEACHER</div>';
	$out .= '<div class="footer"></div>';

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


	$q = "select a_id, aa_id, a_gender, a_old,  concat(a_f_name, ' ',a_l_name) as 'Full Name', TIMESTAMPDIFF( YEAR, a_dob, CURDATE() ) as 'Age', ac_teen, ac_10d, ac_stp, ac_spl, ac_20d, ac_30d, ac_45d, ac_60d, ac_tsc, ac_teacher, aa_group, aa_acco, aa_section, aa_laundry, aa_chowky, aa_valuable, aa_chowky, aa_chair, aa_backrest, a_occupation, a_education, a_langs, aa_comment, aa_cell from dh_applicant left join dh_applicant_course on a_id=ac_applicant left join dh_applicant_attended on a_id=aa_applicant left join dh_teacher t on ( ac_teacher_code = CONCAT(t_code,'-',t_gender )) where a_course IN (".implode("," , $course).") and a_attended=1 and a_type='Student' order by  a_gender, IFNULL(ac_teacher,0) desc, IFNULL(t_seniority,0) desc, IFNULL(t_year_appointed,".date("Y").") , IFNULL(ac_60d,0) desc, IFNULL(ac_45d,0) desc, IFNULL(ac_30d,0) desc, IFNULL(ac_20d,0) desc, IFNULL(ac_tsc,0) desc, IFNULL(ac_spl,0) desc, IFNULL(ac_stp,0) desc, IFNULL(ac_10d,0) desc, IFNULL(ac_teen,0) desc, TIMESTAMPDIFF( YEAR, a_dob, CURDATE() ) desc";
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
	}

	
	return $seats;
}

function dh_print_seats($seats, $cushion_cols, $direction, $div, $alpha)
{
//	$seats = $seat_data['seats'];
	if ($direction == LEFT_TO_RIGHT) $append = '-m'; else $append = '-f';

	$out = '<div class="'.$div.'"><table class="sortable'.$append.'">'; // id=
//	$out = '<div class="main-div"><ul id="sortable">';
	$inc = count($seats)*$cushion_cols;
	for ($row = count($seats)-1; $row >= 0; $row--)
	{
		$out .= '<tr>';
		for ($col=0; $col< $cushion_cols; $col++)
		{
			$out .= '<td class="ui-state-default">';
			//$out .= '<li  class="ui-state-default">';

			if ($direction == LEFT_TO_RIGHT)
				$col2 = $col;
			else
				$col2 = ($cushion_cols - $col -1);
			if ( isset($seats[$row][$col2]) )
			{
				$r = $seats[$row][$col2];
				if ($r['a_old']) $t = '(O)'; else $t ='';
				$teacher = "";	
				$s_no = $r['counter'];
				if ($alpha) {
					$s_no = sprintf("%c%d", (65+$col2), ($row+1));	
				}
				if ($r['aa_chowky'])
					$s_no = 'CW-'.$s_no;
				if ($r['aa_chair'])
					$s_no = 'CH-'.$s_no;

				$seat_no = '<div class="seat-no">'.$s_no.'</div>';
				$br = '';
				if ($r['aa_backrest'] || $r['aa_chowky']) $br = 's-backrest-yes';
				$age_info = ' ('.$r['Age'].')'; 
				//$info = $r['Full Name'].'<br>'.$t.' '.$r['aa_section'].'-'.$r['aa_acco'].$age_info.$br.'<br><br>'.$seat_no;
				$short_courses = $long_courses = '';
				if ( $r['a_old'] )
				{
					$short_courses = 's:'.($r['ac_teen'] + $r['ac_10d'] + $r['ac_stp']);
					$long_courses = 'L:'.($r['ac_20d'] + $r['ac_30d'] + $r['ac_45d'] + $r['ac_60d'] + $r['ac_tsc'] + $r['ac_spl']);					
	  			    if( $r['ac_teacher'] )
				  	   $teacher = "s-teacher";
				}
				$info = '<div class="s-seat-no">'.$seat_no.'</div>'.
						'<div class="s-acco">'.$r['aa_section'].'-'.$r['aa_acco'].'</div>'.
						'<div class="s-name '.$teacher.'">'.$r['Full Name'].'</div>'.
						'<div class="s-old-student">'.$t.' '.$short_courses.'&nbsp;'.$long_courses.'</div>'.						
						'<div class="s-age">Age: '.$age_info.'</div>'.
						'<div class="s-backrest '.$br.'"></div>';
				//$out .= '<div class="grid-item">'.$info.'</div>';
				$out .= $info;
			}
			$out .= '</td>';
			//$out .= '</li>';
		}			
		$out .= '</tr>';		
	}
//	$out .= '</ul>';
	$out .= '</table></div>';

	$out_chowky = '';
	/*
	$out_chowky = '<div class="chowky-div"><table>';
	for ($i=count($seat_data['chowky'])-1; $i>=0; $i--) 
	{
		$r = $seat_data['chowky'][$i];
		$out_chowky .= '<tr><td>';
		if ($r['a_old']) $t = '(O)'; else $t ='';
		$type = 'CHOWKY'; $br = 's-backrest-yes';
		if ($r['aa_chair']) 
		{
			$type = 'CHAIR';
			$br = '';
		}	
//		$info = $r['Full Name'].'<br>'.$t.' '.$r['aa_section'].'-'.$r['aa_acco'].' ('.$r['Age'].')<br><br>'.$type;
		$short_courses = $long_courses = '';
		$teacher = '';
		if ( $r['a_old'] )
		{
			$short_courses = 's:'.($r['ac_10d'] + $r['ac_stp']);
			$long_courses = 'L:'.($r['ac_20d'] + $r['ac_30d'] + $r['ac_45d'] + $r['ac_60d'] + $r['ac_tsc'] + $r['ac_spl']);		
		  
		    if( $r['ac_teacher'] )
	  	   		$teacher = "s-teacher";
		}

		$info = '<div class="s-seat-no">'.$seat_no.'</div>'.
				'<div class="s-acco">'.$r['aa_section'].'-'.$r['aa_acco'].'</div>'.
				'<div class="s-name '.$teacher.'">'.$r['Full Name'].'</div>'.
				'<div class="s-old-student">'.$t.' '.$short_courses.'&nbsp;'.$long_courses.'</div>'.						
				'<div class="s-age">Age: '.$age_info.'</div>'.
				'<div class="s-backrest '.$br.'"></div>';

		$out_chowky .= $info.'</td></tr>';
	}

	$out_chowky .= '</table></div>';
	*/

	if ($direction == LEFT_TO_RIGHT)
		return $out.$out_chowky;
	else
		return $out_chowky.$out;
}

function dh_zero_day_list($centre, $course, $pdf = '')
{
	global $base_url;
	$out = '<style>@import url("'.$base_url.'/sites/all/modules/dh_manageapp/css/day0-list.css");</style>';

	$temp = db_query("select c_name,td_val3 from dh_course c left join  dh_type_detail td on c.c_course_type=td.td_id where c_id=$course")->fetchAssoc();
	$course_name = $temp['c_name'];
	$c_type = strtolower($temp['td_val3']);
	$q = "select a_id, a_gender, a_type, a_old,  concat(a_f_name, ' ',a_m_name, ' ',a_l_name) as 'Full Name', TIMESTAMPDIFF( YEAR, a_dob, CURDATE() ) as 'Age', a_city_str, a_conf_no , a_mob_country, a_phone_mobile, a_email,a_learn_about_vip, a_willing_to_serve, ac_teen, ac_10d, ac_stp, ac_spl, ac_20d, ac_30d, ac_45d, ac_60d, ac_tsc, ac_teacher, a_occupation, a_education, a_lang_1, a_lang_2, a_langs, a_lang_discourse, ci.c_name as 'City', al_special_req_details, al_at_year, ae_desc_physical, ae_desc_mental, co.c_name as 'Country' from dh_applicant left join dh_applicant_course on a_id=ac_applicant left join dh_applicant_extra ae on a_id=ae_applicant left join dh_applicant_lc on a_id=al_applicant left join dh_city ci on a_city=ci.c_id left join dh_country co on (a_country=co.c_code) where a_course='$course' and a_status in ('Confirmed', 'Expected', 'ReConfirmation') order by a_type desc, a_gender, a_old DESC, a_f_name, a_l_name, IFNULL(ac_teacher,0) desc, IFNULL(ac_60d,0) desc, IFNULL(ac_45d,0) desc, IFNULL(ac_30d,0) desc, IFNULL(ac_20d,0) desc, IFNULL(ac_tsc,0) desc, IFNULL(ac_spl,0) desc, IFNULL(ac_stp,0) desc, IFNULL(ac_10d,0) desc, IFNULL(ac_teen,0) desc, TIMESTAMPDIFF( YEAR, a_dob, CURDATE() ) desc";
	$result = db_query($q); 
	$data['M']['old'] = $data['M']['new'] = $data['F']['old'] = $data['F']['new'] = array();
	while($r = $result->fetchAssoc())
	{
		$data[strtoupper($r['a_gender'])][$r['a_old']?'old':'new'][] = $r;
	}

	$header = array('Sr.', 'ConfNo', 'Student', 'Courses', 'Education', 'Age', 'City', 'Occupation', 'ContactDetails' , 'Recom. By / Comments');
	if ($c_type == 'lc')
		$header = array('Sr', 'ConfNo', 'Student', 'Courses', 'City/Country', 'Age' , 'Contact', 'Health Problems', 'Special Requirement', '10d', 'STP', 'TSC', '20d', '30d', '45d', '60d', 'Languages' );
	$attributes = array('class' => array('table-day0-list') );
	$title = '<div class="header-day0"><div align="center" class="title"><b>Day 0 List</b></div>'.
			'<div align="center" class="title-head">'.$course_name.'</div></div>';

	$gender = array('M', 'F');
	$type = array('old', 'new');
	$i = 1;

	foreach($gender as $g)
	{
		foreach ($type as $t) 
		{
			unset($rows); $total =0 ; $i=1;
			$i = 1;
			foreach ($data[$g][$t] as $r) 
			{
				unset($row);
				$row[] = $i++;
				$row[] = $r['a_conf_no'];
				if ($r['ac_teacher'])
					$row[] = $r['Full Name']." (AT-".$r['al_at_year'].")";
				elseif ($r['a_type'] == 'Sevak')
			    	$row[] = $r['Full Name']." (Sevak)";
				else 
					$row[] = $r['Full Name'];
					$row[] = $r['ac_10d']+$r['ac_stp']+$r['ac_spl']+$r['ac_20d']+$r['ac_30d']+$r['ac_45d']+$r['ac_60d']+$r['ac_tsc'];
				if ($c_type <> 'lc')
				{
					$row[] = $r['a_education'];
					$row[] = $r['Age'];
					if ( $r['City'] <> '')
						$row[] = $r['City'];
					else
						$row[] = $r['a_city_str'];					

					$row[] = $r['a_occupation'];
					$row[] = 'M: '.'('.$r['a_mob_country'].') '.$r['a_phone_mobile'].' Email:'.$r['a_email'];
					$row[] = $r['a_learn_about_vip'].($r['a_willing_to_serve']?'Can Serve':'').($r['a_lang_discourse']?'<br>'.$r['a_lang_discourse']:'');
				}
				else
				{
					if ( strtoupper($r['Country']) == 'INDIA') 
					{
						if ($r['City'] <> '')
							$row[] = $r['City'];
						else
							$row[] = $r['a_city_str'];
					}
					else
						$row[] = $r['Country'];					
					$row[] = $r['Age'];
					$row[] = 'M: '.'('.$r['a_mob_country'].') '.$r['a_phone_mobile'];
					$row[] = $r['ae_desc_physical'].' '.$r['ae_desc_mental'];
					$row[] = $r['al_special_req_details'];
					$row[] = $r['ac_10d'];
					$row[] = $r['ac_stp'];
					$row[] = $r['ac_tsc'];
					$row[] = $r['ac_20d'];
					$row[] = $r['ac_30d'];
					$row[] = $r['ac_45d'];
					$row[] = $r['ac_60d'];
					$row[] = $r['a_lang_1'].', '.$r['a_lang_2'].', '.$r['a_langs'];
				}
				$rows[] = $row;
				$total++;
			}
			if ($total > 0 )
			{
				$out .= $title.'<br /><div class="title-info">Gender: '.(($g=='M')?'MALE':'FEMALE').' | Type: '.strtoupper($t).' | Total: '.$total.'</div><br>';
				$out .= theme('table',array('header' => $header, 'rows' => $rows, 'attributes' => $attributes) );
				$out .= '<div id="footer">&nbsp;</div>';				
			}

		}
	}

	if ($pdf <> '')
		create_pdf_from_html($out, $pdf);
	else
	{
		print $out;
		exit;
	}	
}


function dh_cell_list($centre, $course, $debug = 0, $pdf = '' )
{
	global $base_url;
	$ROWS_PER_PAGE = variable_get('cell_list_rows_per_page', '5');
	$gender = array('M', 'F');
	$data['M'][0] = array(); $data['F'][0] = array();
	$course_name = db_query("select c_name from dh_course where c_id=$course")->fetchField();
	$q = "select a_id, aa_id, a_gender, a_old,  concat(a_f_name, ' ',a_m_name, ' ',a_l_name) as 'Full Name', TIMESTAMPDIFF( YEAR, a_dob, CURDATE() ) as 'Age', a_conf_no , ac_teen, ac_10d, ac_stp, ac_spl, ac_20d, ac_30d, ac_45d, ac_60d, ac_tsc, ac_teacher, aa_group, aa_cell, aa_cell_fixed, IFNULL(aa_cell_group,0) as 'aa_cell_group' from dh_applicant left join dh_applicant_course on a_id=ac_applicant left join dh_applicant_attended on a_id=aa_applicant left join dh_teacher on (ac_teacher_code = CONCAT(t_code, '-', t_gender)) where a_course='$course' and a_attended=1  and a_type='Student' order by a_gender, IFNULL(ac_teacher,0) desc, IFNULL(t_seniority,0) desc, IFNULL(t_year_appointed,".date("Y").") , IFNULL(ac_60d,0) desc, IFNULL(ac_45d,0) desc, IFNULL(ac_30d,0) desc, IFNULL(ac_20d,0) desc, IFNULL(ac_tsc,0) desc, IFNULL(ac_spl,0) desc, IFNULL(ac_stp,0) desc, IFNULL(ac_10d,0) desc, IFNULL(ac_teen,0) desc, TIMESTAMPDIFF( YEAR, a_dob, CURDATE() ) desc";
	$result = db_query($q);
	while($r = $result->fetchAssoc())
	{
		$name = $r['Full Name'];
		if ($debug) $name .= ' ('.$r['Age'].')';
		$data[strtoupper($r['a_gender'])][$r['a_old']][$r['aa_cell_group']][$r['aa_cell']] = $name;
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
	$title = '<div class="header-day0"><div align="center" class="title"><b>Cell List</b></div>'.
			'<div align="center" class="title-head">'.$course_name.'</div></div>';
	$out .= $title;
	$out .= '<div class="main-div">';
	$c = 0;
	foreach( $gender as $g )
	{
		for($i=1; $i>=0 ; $i--)
		{
			foreach( $data[$g][$i] as $cell_group => $cell_data )
			{
				//$out .= '<div class="title-info">'.$g.' Batch '.$cell_group.'</div>';
				if ( ! $debug )
					asort($cell_data);
				$keys = array_keys($cell_data);
				$loop = floor(count($cell_data)/($ROWS_PER_PAGE*2));
				if ( (count($cell_data)%($ROWS_PER_PAGE*2)) > 0 )
					$loop++;
				for($t=0; $t< $loop ; $t++ )
				{
					$c = 0;
					$out .= '<div class="title-info">'.($i?'Old':'new').' '.($g=='M'?'Male':'Female').' Batch '.($cell_group+1).
					' ('.$age_range[$g][$i][$cell_group]['lower'].' - '.$age_range[$g][$i][$cell_group]['upper'].')</div>';
					$out .= '<table cellspacing="0" class="table-cell-list">';
					$out .= '<tr><th>Name</th><th>Cell</th><th>&nbsp;</th><th>Name</th><th>Cell</th></tr>';

					for($a=0; $a<$ROWS_PER_PAGE; $a++ )
					{
						$c1 = $a + ($t * $ROWS_PER_PAGE * 2);
						$c2 = $a + ($t * $ROWS_PER_PAGE * 2) + $ROWS_PER_PAGE;
						$out .= '<tr>';
						$out .= '<td>'.(isset($keys[$c1])?$cell_data[$keys[$c1]]:'').'</td><td align="center"><b>'.(isset($keys[$c1])?$keys[$c1]:'').'</b></td><td>&nbsp;&nbsp;</td>';
						$out .= '<td>'.(isset($keys[$c2])?$cell_data[$keys[$c2]]:'').'</td><td align="center"><b>'.(isset($keys[$c2])?$keys[$c2]:'').'</b></td>';
						$out .= '</tr>';
						$c++;				
					}
					if ($t <= ($loop - 1))
						$out .= '</table><div class="footer"></div><br>'.$title;
				}
			}

		}
	}
	$out .= '</table>';
	$out .= '</div></html>';

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

	$q = "select a_id, aa_id, a_gender, a_old,  concat(a_f_name, ' ',a_m_name, ' ',a_l_name) as 'Full Name', TIMESTAMPDIFF( YEAR, a_dob, CURDATE() ) as 'Age', a_conf_no , ac_teen, ac_10d, ac_stp, ac_spl, ac_20d, ac_30d, ac_45d, ac_60d, ac_tsc, ac_teacher, aa_group, aa_cell, aa_cell_fixed from dh_applicant left join dh_applicant_course on a_id=ac_applicant left join dh_applicant_attended on a_id=aa_applicant left join dh_teacher on (ac_teacher_code = CONCAT(t_code, '-', t_gender)) where a_course='$course' and a_attended=1  and a_type='Student' order by a_gender, IFNULL(ac_teacher,0) desc, IFNULL(t_seniority,0) desc, IFNULL(t_year_appointed,".date("Y").") , IFNULL(ac_60d,0) desc, IFNULL(ac_45d,0) desc, IFNULL(ac_30d,0) desc, IFNULL(ac_20d,0) desc, IFNULL(ac_tsc,0) desc, IFNULL(ac_spl,0) desc, IFNULL(ac_stp,0) desc, IFNULL(ac_10d,0) desc, IFNULL(ac_teen,0) desc, TIMESTAMPDIFF( YEAR, a_dob, CURDATE() ) desc";
	$result = db_query($q);
	$data['MALE']['old'] = $data['MALE']['new'] = $data['FEMALE']['old'] = $data['FEMALE']['new'] = array();
	$reserved['Reserved-old']['MALE'] = $reserved['Reserved-new']['MALE'] = $reserved['Reserved-old']['FEMALE'] = $reserved['Reserved-new']['FEMALE'] = array();
	while($r = $result->fetchAssoc())
	{
		$data[strtoupper($r['a_gender']) == 'M'?'MALE':'FEMALE'][$r['a_old']?'old':'new'][] = $r;
		if ($r['aa_cell_fixed'])
		{
			$cell_alloc['Reserved'][$r['a_gender'] == 'M'?'MALE':'FEMALE'][] = $r['aa_cell'];
			$reserved['Reserved-'.($r['a_old']?'old':'new')][$r['a_gender'] == 'M'?'MALE':'FEMALE'][] = $r['aa_cell'];						
		}
	}

	/* Remove reserved cells from the list */

	foreach( $gender as $g )
	{
		$cell_alloc['Old'][$g] = $reserved['Reserved-old'][$g];
		$cell_alloc['New'][$g] = $reserved['Reserved-new'][$g];
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
					while( in_array($cell_alloc['Cells'][$g][$c], array_values($cell_alloc['Reserved'][$g]) ) )
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
					while( in_array($cell_alloc['Cells'][$g][$c], array_values($cell_alloc['Reserved'][$g]) )  )
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
		$c = 0;
		$keys = array_keys($cell_alloc['Old'][$g]);
		foreach($data[$g]['old'] as $r)
		{
			if ( $count >= $batch_size[$g] )
			{
				$batch++; $count= 0;
			}
			if ($r['aa_cell_fixed'])
			{
				$cells[$batch][$g][$r['aa_cell']] = $r['Full Name']." - ".$r['a_old'];
				$c++; $count++;
				continue;
			}
			$cells[$batch][$g][$cell_alloc['Old'][$g][$keys[$count]]] = $r['Full Name']." - ".$r['a_old'];
			db_update('dh_applicant_attended')->fields(array('aa_cell_group' => $batch, 'aa_cell' => $cell_alloc['Old'][$g][$keys[$count]]))->condition('aa_id', $r['aa_id'])->execute();
			$c++; $count++;
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
			if ( $count >= $batch_size[$g] )
			{
				$batch++; $count= 0;
			}
			if ($r['aa_cell_fixed'])
			{
				$cells[$batch][$g][$r['aa_cell']] = $r['Full Name']." - ".$r['a_old'];
				continue;
			}			
			$cells[$batch][$g][$cell_alloc['New'][$g][$keys[$count]]] = $r['Full Name']." - ".$r['a_old'];
			if ( $cell_alloc['New'][$g][$keys[$count]] == '' )
			{
				print "Count - $count - ".$r['Full Name']."<br>";
			}
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
	$q = "select a_id, a_gender, a_old,  concat(a_f_name, ' ',a_m_name, ' ',a_l_name) as 'Full Name', TIMESTAMPDIFF( YEAR, a_dob, CURDATE() ) as 'Age', a_conf_no , ac_teen, ac_10d, ac_stp, ac_spl, ac_20d, ac_30d, ac_45d, ac_60d, ac_tsc, ac_teacher, aa_group, aa_cell, aa_cell_fixed, aa_laundry, aa_section, aa_acco from dh_applicant left join dh_applicant_course on a_id=ac_applicant left join dh_applicant_attended on a_id=aa_applicant where a_course='$course' and a_attended=1  and a_type='Student' order by a_gender, (aa_laundry*1) asc";
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
	$q = "select a_id, a_gender, a_old,  concat(a_f_name, ' ',a_m_name, ' ',a_l_name) as 'Full Name', TIMESTAMPDIFF( YEAR, a_dob, CURDATE() ) as 'Age', a_conf_no , ac_teen, ac_10d, ac_stp, ac_spl, ac_20d, ac_30d, ac_45d, ac_60d, ac_tsc, ac_teacher, aa_group, aa_cell, aa_cell_fixed, aa_laundry, aa_valuable, aa_section, aa_acco from dh_applicant left join dh_applicant_course on a_id=ac_applicant left join dh_applicant_attended on a_id=aa_applicant where a_course='$course' and a_attended=1  and a_type='Student' order by a_gender, (aa_valuable * 1) asc";
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


function dh_student_chit($centre, $course)
{
	global $base_url;
	$alpha = db_query("select cs_seat_naming_conv from dh_center_setting where cs_center=$centre")->fetchField();
	$BLOCKS_PER_PAGE = variable_get('student_chit_rows_per_page', '18');
	$gender = array('M', 'F');
	$data['M'][0] = array(); $data['F'][0] = array();
	$course_name = db_query("select c_name from dh_course where c_id=$course")->fetchField();
	$q = "select a_id, aa_id, a_gender, a_old,  a_f_name, a_l_name, a_conf_no , aa_group, aa_cell, aa_cell_fixed, aa_cell_group, CONCAT(aa_section,'-', aa_acco) as 'AccoNo', aa_seat_col, aa_seat_row, aa_chowky, aa_chair, aa_backrest from dh_applicant left join dh_applicant_course on a_id=ac_applicant left join dh_applicant_attended on a_id=aa_applicant where a_course='$course' and a_attended=1  and a_type='Student' order by a_gender, a_f_name, a_l_name";
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
				$r['SeatNo'] = chr($r['aa_seat_col']+65).($r['aa_seat_row']+1);
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
	
			$out .= '<div class="table-student-chit">'.$seat_type.': '.$r['SeatNo'].'<br>'.$r['a_l_name'].'<br>'.$r['a_f_name'].'<br>Cell: '.$r['aa_cell'].'<br>Room No.:'.$r['AccoNo'].'</div>';
			$count++;
		}
		$out .= '</div>';		
	}
	$out .= '</body></html>';
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


