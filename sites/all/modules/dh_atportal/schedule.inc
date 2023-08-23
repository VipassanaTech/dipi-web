<?php

require_once 'vendor/autoload.php';
use Mailgun\Mailgun;

function at_course_list_form($form, &$form_state)
{

	global $user;
	$at_code = $user->name;

	$q = "select t_id, t_gender from dh_teacher where t_status='Active' and CONCAT(t_code,'.',t_gender) = '$at_code' limit 1";
	$row = db_query($q)->fetchAssoc();

	$at_id = $row['t_id'];
	$at_gender = $row['t_gender'];

	if (!$at_id)
	{
		drupal_set_message("Only AT can apply for courses.", "error");
		drupal_goto("at-portal");
	}

	drupal_add_css( libraries_get_path("editor"). "/DataTables/datatables.min.css");
	drupal_add_js( libraries_get_path("editor").  "/DataTables/datatables.min.js");

	drupal_add_css(drupal_get_path('module', 'dh_manageapp').'/css/select2.min.css');
	drupal_add_js(drupal_get_path('module', 'dh_manageapp').'/js/select2.min.js');
	drupal_add_css( drupal_get_path('module', 'dh_manageapp'). "/css/jquery-confirm.min.css");
	drupal_add_js( drupal_get_path('module', 'dh_manageapp'). "/js/jquery-confirm.min.js");


	$form['at-portal'] = array(
  		'#markup' => '<h2>'.l("Back to AT-Portal", "/at-portal").'</h2>',
	);
	

	$center = db_query("select concat('ce-', c_id), c_name from dh_center where c_mute=0")->fetchAllKeyed();
	$country = db_query("select concat('cy-', cy.c_id), cy.c_name from dh_center c left join dh_country cy on (c.c_country=cy.c_code)")->fetchAllKeyed();
	$state = db_query("select concat('st-', st.s_id), st.s_name from dh_center c left join dh_state st on (c.c_state=st.s_code AND c.c_country=st.s_country) ")->fetchAllKeyed();
	$city = db_query("select concat('ci-', ci.c_id), ci.c_name from dh_center c left join dh_city ci on (c.c_city=ci.c_id)")->fetchAllKeyed();

	$loc_options = array (
		"Center" => $center,
		"Country" => $country,
		"State" => $state,
		"City" => $city,
	);


	$course_type = db_query("select td_key, td_val1 from `dh_type_detail` where td_type='course-type'")->fetchAllKeyed();


	$form['from_date'] = array(
		'#type' => 'date_popup',
		'#default_value' => date('Y-m-d'),
		'#title' => t('From Date'),
		'#size' => 12,
		'#weight' => 5,
		'#date_format' => 'Y-m-d',
		'#date_year_range' => '+0:+4',
		'#date_label_position' => 'above',
		'#required' => 1,
		'#theme_wrappers' => array('date_popup'),
		'#datepicker_options' => array(
			'minDate' => 0,
			'#required' => 1,
			'dateFormat' => date_popup_format_to_popup('Y-m-d'),
		),
		'#attributes' => array(
		    'class' => array('from-date-class',),
		),
	);

	$form['to_date'] = array(
		'#type' => 'date_popup',
		'#default_value' => date('Y-m-d', strtotime('+1 year')),
		'#title' => t('To Date'),
		'#size' => 12,
		'#weight' => 10,
		'#date_format' => 'Y-m-d',
		'#date_year_range' => '+0:+4',
		'#date_label_position' => 'above',
		'#required' => 1,
		'#theme_wrappers' => array('date_popup'),
		'#datepicker_options' => array(
			'minDate' => 0,
			'#required' => 1,
			'dateFormat' => date_popup_format_to_popup('Y-m-d'),
		),
		'#attributes' => array(
		    'class' => array('to-date-class',),
		),
	);

	$form['location'] = array(
		'#title' => 'Centre / Country / Location / Place', 
		'#type' => 'select', 
		'#options' => $loc_options, 
		'#weight' => 15, 
		'#multiple' => "multiple",
		'#attributes' => array(
		    'id' => 'edit-location',
		),
	);

	$form['course-type'] = array('#title' => 'Course Type', '#type' => 'select', '#options' => $course_type, '#weight' => 20, '#multiple' => "multiple");

	$form['sub'] = array(
		'#type' => 'submit',
		'#value' => 'Search',
		'#weight' => 25,
		'#prefix' => '<br>',
		'#suffix' => '<br><br><br>',
	);

	$q = db_select('dh_course', 'co');
	$q->leftjoin('dh_center', 'ce', 'co.c_center=ce.c_id');
	$q->leftjoin('dh_country', 'cy', 'ce.c_country=cy.c_code');
	$q->leftjoin('dh_state', 'st', '(ce.c_state=st.s_code AND ce.c_country=st.s_country)');
	$q->leftjoin('dh_city', 'ci', 'ce.c_city=ci.c_id');
	$q->leftjoin('dh_type_detail', 'td', 'co.c_course_type=td.td_id');
	$q->addField('ce', 'c_name', 'center_name');
	$q->addField('ce', 'c_subdomain', 'center_sub');
	$q->addField('td', 'td_val1', 'course_type');
	$q->addField('co', 'c_id', 'course_id');
	$q->addField('co', 'c_start', 'start_date');
	$q->addField('co', 'c_end', 'end_date');
	$q->addField('co', 'c_at_m_count', 'male_at_count');
	$q->addField('co', 'c_at_f_count', 'female_at_count');
	$q->addField('co', 'c_at_m_conf', 'male_at_conf');
	$q->addField('co', 'c_at_f_conf', 'female_at_conf');
	$q->addField('ci', 'c_name', 'city');
	$q->addField('st', 's_name', 'state');
	$q->condition('ce.c_mute', '0', '=');


	

	if (isset($form_state['user_input']) && $form_state['user_input'])
	{
		watchdog("testing2","<pre>".print_r($form_state['user_input'],true)."</pre>");

		$values = $form_state['user_input'];
		$location = array('center' => array(),
			'country' => array(),
			'state' => array(),
			'city' => array()
		);

		if($values['from_date']['date'])
		{
			$q->condition('co.c_start', $values['from_date']['date'], '>=');
		}

		if($values['to_date']['date'])
		{
			$q->condition('co.c_start', $values['to_date']['date'], '<=');
		}


		$location['center']=array();
		$location['country']=array();
		$location['state']=array();
		$location['city']=array();

		if (is_array($values['location']))
		{
			// $form['location']['#default_value'] = $values['location'];
			foreach ($values['location'] as $loc)
			{
				if ( substr($loc,0,3) == 'ce-')
					$location['center'][] = str_replace('ce-', '', $loc);
				if ( substr($loc,0,3) == 'cy-')
					$location['country'][] = str_replace('cy-', '', $loc);
				if ( substr($loc,0,3) == 'st-')
					$location['state'][] = str_replace('st-', '', $loc);
				if ( substr($loc,0,3) == 'ci-')
					$location['city'][] = str_replace('ci-', '', $loc);
			}
		}
		

		if (count($location['center'])>0 || count($location['country'])>0 || count($location['state'])>0 || count($location['city'])>0)
		{
			$or = db_or();

			if (count($location['center'])>0)
				$or->condition('ce.c_id', $location['center'], 'in');

			if (count($location['country'])>0)
				$or->condition('cy.c_id', $location['country'], 'in');

			if (count($location['state'])>0)
				$or->condition('st.s_id', $location['state'], 'in');

			if (count($location['city'])>0)
				$or->condition('ci.c_id', $location['city'], 'in');

			$q->condition($or);
		}
		

		if (is_array($values['course-type']))
		{
			$form['course-type']['#default_value'] = $values['course-type'];
			$q->condition('td.td_key', $values['course-type'], 'in');
		}
		
	}

	$q->condition('co.c_start', date('Y-m-d'), '>=');
	$q->condition('co.c_deleted', 0, '=');
	$q->condition('co.c_cancelled', 0, '=');

	$q->orderBy("co.c_start", "asc");

	// drupal_set_message($q->__toString());	
	// drupal_set_message(print_r($q->arguments(), true));
	$res = $q->execute();

	$rows = array();
	foreach ($res as $row) {
		
		$row = (array) $row;

		$q = "select ct_id from dh_course_teacher where ct_teacher=:ct_teacher and ct_course=:ct_course limit 1";
		$ct_id = db_query($q, array( ':ct_teacher' => $at_id, ':ct_course' => $row['course_id'],))->fetchField();

		$required = check_at_requirement($row['course_id'], $at_gender);

		if ($ct_id)
			$row = array_merge(array("apply-link"=>"<button class=\"show-course-application\" onclick=\"location.href='/at-portal/show-course-application/$ct_id'\" type=\"button\">View</button>"), $row);
		elseif ($required)
     		$row = array_merge(array("apply-link"=>"<button class=\"apply-for-course\" onclick=\"location.href='/at-portal/course-apply/{$row['course_id']}'\" type=\"button\">Apply</button>"), $row);
     	else
     		$row = array_merge(array("apply-link"=>""), $row);

		unset($row['course_id']);

		$row['start_date'] = date('d M y', strtotime($row['start_date']));
		$row['end_date'] = date('d M y', strtotime($row['end_date']));

		$row['course_at_status'] = "M : ".
			($row['male_at_count'] > $row['male_at_conf'] ? '<span style="color:green;">Open</span>' : '<span style="color:red;">Close</span>').
			"&nbsp&nbsp|&nbsp&nbspF: ".
			($row['female_at_count'] > $row['female_at_conf'] ? '<span style="color:green;">Open</span>' : '<span style="color:red;">Close</span>');

		// $uri = "public:///center-img/{$row['center_sub']}.jpg";
		// $placeholder_uri = "public:///center-img/placeholder.png.jpg";

		// $row['center_name'] = "<b>{$row['center_name']}</b>";

		// if (file_exists($uri)) {
		// 	$row['center_name'] .= '<br><div class="center-image"><img src="'.file_create_url($uri).'" alt="'.$row['center_sub'].'"></div>'; 
		// }
		// else
		// 	$row['center_name'] .= '<br><div class="center-image"><img src="'.file_create_url($placeholder_uri).'" alt="'.$row['center_sub'].'"></div>';

		$rows[] = $row;
	}


		$out = theme('table', array('rows' => array(), 'attributes' => array('id' => 'at-course-list')));

		$form['a'] = array('#markup' => $out, '#weight' => 101,);
		$data = json_encode($rows);



	$js = '
		(function ($) {
			function do_datatable( tid, data )
			{
				var table = $(tid).DataTable({
					dom: "lfr<\"clear\">tip", 
					data: data,
					responsive: {
			            details: {
			                display: $.fn.dataTable.Responsive.display.childRowImmediate,
			                type: "none",
			                target: ""
			            }
			        },
					destroy: true,
          "ordering": false,
					"oLanguage": {"sEmptyTable": "No Courses found for Search Criteria."},
					searching: false,
					"columnDefs": [
					            {
					                "targets": [ 0 ],
					                "searchable": false
					            }
			        ],					
			        columns: [
			            { data: "apply-link", "title" : "Action" },
			            { data: "center_name", "title" : "Center" },
			            { data: "city", "title" : "City" },
			            { data: "state", "title" : "State" },
			            { data: "course_type", "title" : "Course Type" },
			            { data: "start_date", "title" : "Start Date" },
				        { data: "end_date", "title" : "End Date"},
				        { data: "course_at_status", "title" : "Course AT Status"},
			        ],
			    });			
			}
			$(document).ready(function(){
				$("#edit-location").select2();
				$("#edit-course-type").select2();

				var dataset = '.$data.';
				do_datatable("#at-course-list", dataset);

				$(".from-date-class").parent().parent().parent().parent().attr("id", "from-date-id-div");
				$(".to-date-class").parent().parent().parent().parent().attr("id", "to-date-id-div");
				$("#edit-location").parent().attr("id", "edit-location-div");
			});
		})(jQuery);
	';

	drupal_add_js($js, 'inline');

	$form['#attached']['css'][] = array(
		'data' => '#edit-from-date, #edit-to-date { float: none;}
					.container-inline-date {margin-bottom: 10px;}
					.show-course-application, .apply-for-course {
						background-color: #F5ECCE;
						border: 1px solid black;
						border-radius: 2px;
						color: #000;
						padding: 5px 25px;
						text-align: center;
						text-decoration: none;
						display: inline-block;
						font-family: "Helvetica Neue","Helvetica","Arial",sans-serif;
	    				font-size: 15px;
	    				-webkit-font-smoothing: antialiased;
	    				font-weight: 600;
					}
					#at-course-list td:nth-child(1) { width: 9%; text-align: left }
					#at-course-list td:nth-child(2) { width: 16%; text-align: left }
					#at-course-list td:nth-child(3) { width: 9%; text-align: left }
					#at-course-list td:nth-child(4) { width: 14%; text-align: left }
					#at-course-list td:nth-child(5) { width: 14%; text-align: left }
					#at-course-list td:nth-child(6) { width: 9%; text-align: left }
					#at-course-list td:nth-child(7) { width: 9%; text-align: left }
					#at-course-list td:nth-child(8) { width: 20%; text-align: left }
					#from-date-id-div { float: left; margin-right: 40px}
					#to-date-id-div { float: left; clear: none;  }
					#edit-location-div { clear: both; }
					',
		'type' => 'inline',
	);

	return $form;
}




function at_course_list_form_submit($form, &$form_state)
{
	$form_state['user_input'] = $form_state['input'];
  	$form_state['rebuild'] = TRUE;
}


function apply_at_application_form($form, &$form_state, $course_id)
{
	drupal_add_css( drupal_get_path('module', 'dh_manageapp'). "/css/jquery-confirm.min.css");
	drupal_add_js( drupal_get_path('module', 'dh_manageapp'). "/js/jquery-confirm.min.js");

	global $user;
	$at_code = $user->name;

	$q = "select t_id, t_gender from dh_teacher where t_status='Active' and CONCAT(t_code,'.',t_gender) = '$at_code' limit 1";
	$row = db_query($q)->fetchAssoc();

	$at_id = $row['t_id'];
	$at_gender = $row['t_gender'];

	if (!$at_id)
	{
		drupal_set_message("Only AT can apply for courses.", "error");
		drupal_goto("at-portal");
	}


	$q = "select ct_id from dh_course_teacher where ct_teacher=:ct_teacher and ct_course=:ct_course limit 1";
	$ct_id = db_query($q, array( ':ct_teacher' => $at_id, ':ct_course' => $course_id,))->fetchField();

	if ($ct_id)
		drupal_goto("at-portal/show-course-application/".$ct_id);

	$q = "select
		co.c_id as 'course_id',
		co.c_start as 'c_start_unformatted',
		co.c_name as 'course_name',
		ce.c_id as 'center_id',
		ce.c_name as 'center_name',
		td.td_val1 as 'course_type',
		DATE_FORMAT(co.c_start, '%D %M %Y') as 'start_date',
		DATE_FORMAT(co.c_end, '%D %M %Y') as 'end_date',
		co.c_description as 'course_desctiption',
		co.c_comments as 'course_comments',
		co.c_at_m_count as 'male_at_needed',
		co.c_at_f_count as 'female_at_needed',
		co.c_at_m_conf as 'male_at_confirmed',
		co.c_at_f_conf as 'female_at_confirmed',
		co.c_deleted as 'course_deleted',
		co.c_cancelled as 'course_cancelled'
		from dh_course co
		left join dh_center ce on co.c_center=ce.c_id
		left join dh_country cy on ce.c_country=cy.c_code
		left join dh_state st on (ce.c_state=st.s_code AND ce.c_country=st.s_country)
		left join dh_type_detail td on co.c_course_type=td.td_id
		where 
		co.c_id=:c_id";

	$course_details = db_query($q, array(':c_id'=>$course_id))->fetchAssoc();

	if (!$course_details)
	{
		drupal_set_message("The course does not exist.", "error");
		drupal_goto("at-portal/course-list");
	}


	$course_name = $course_details['course_name'];
	$center_id = $course_details['center_id'];
	$course_id = $course_details['course_id'];
	$start_date = $course_details['c_start_unformatted'];
	$course_deleted = $course_details['course_deleted'];
	$course_cancelled = $course_details['course_cancelled'];
	

	$form['at-portal'] = array(
  		'#markup' => '<h2>'.l("Back to AT-Portal", "/at-portal").'</h2>',
	);

	$form['course-list'] = array(
  		'#markup' => '<h2>'.l("Back to Course List", "/at-portal/course-list").'</h2><br><br>',
	);

	$course_at_status = "M : ".
		($course_details['male_at_needed'] > $course_details['male_at_confirmed'] ? '<span style="color:green;">Open</span>' : '<span style="color:red;">Close</span>').
		"&nbsp&nbsp|&nbsp&nbspF: ".
		($course_details['female_at_needed'] > $course_details['female_at_confirmed'] ? '<span style="color:green;">Open</span>' : '<span style="color:red;">Close</span>');

	$data = array();

	$data[] = array("Center Name", $course_details['center_name']);
	$data[] = array("Course Type", $course_details['course_type']);
	$data[] = array("Start Date", $course_details['start_date']);
	$data[] = array("End Date", $course_details['end_date']);
	$data[] = array("Course AT Status", $course_at_status);
	$data[] = array("Course Desctiption", $course_details['course_desctiption'] == "" ? "None" : $course_details['course_desctiption']);
	$data[] = array("Course Comments", $course_details['course_comments'] == "" ? "None" : $course_details['course_comments']);

	$form['details'] = array(
	    '#theme' => 'table',
	    '#rows' => $data,
	);

	$form['course_id'] = array(
		'#type' => 'value',
		'#value' => $course_id,
	);

	$form['at_id'] = array(
		'#type' => 'value',
		'#value' => $at_id,
	);

	$form['course_name'] = array(
		'#type' => 'value',
		'#value' => $course_name,
	);

	$form['center_id'] = array(
		'#type' => 'value',
		'#value' => $center_id,
	);

	$form['comment'] = array(
		'#title' => 'Optional Comments',
		'#type' => 'textarea',  
		'#rows' => 4,
		'#weight' => 20,  
		'#maxlength' => 200,
	);

	if (strtotime(date('Y-m-d')) > strtotime($start_date))
	{
		$form['course_started'] = array(
	  		'#markup' => '<h2 style="color:red">The Course is already Started</h2>',
		);
	}


	if ($course_deleted == 1)
	{
		$form['course_deleted'] = array(
	  		'#markup' => '<h2 style="color:red">The Course is Deleted</h2>',
		);
	}

	if ($course_cancelled == 1)
	{
		$form['course_cancelled'] = array(
	  		'#markup' => '<h2 style="color:red">The Course is Cancelled</h2>',
		);
	}

	$required = check_at_requirement($course_id, $at_gender);

	if ($required && !(strtotime(date('Y-m-d')) > strtotime($start_date)) && $course_deleted == 0 && $course_cancelled == 0)
	{
		$form['sub'] = array(
			'#type' => 'submit',
			'#value' => 'Apply for Course',
			'#weight' => 25,
			'#prefix' => '<br>',
			'#attributes' => array(
				'class' => array('apply-course-application'),
				'onclick' => 'if(!confirm("Applying for '.$course_name.'")){return false;}',
			),
		);
	}


	$form['#attached']['css'][] = array(
		'data' => '.course-detail {  }
					.course-detail-key {  }
					.course-detail-value {  }',
		'type' => 'inline',
	);


	return $form;

}


function apply_at_application_form_validate($form, &$form_state)
{

}

function apply_at_application_form_submit($form, &$form_state)
{
	global $user;
	$values = $form_state['values'];
	
	$fields = array();
	
	$fields['ct_course'] = $values['course_id'];
	$fields['ct_teacher'] = $values['at_id'];
	$fields['ct_status'] = 'Received';
	$fields['ct_type'] = 'Conducting';
	$fields['ct_group'] = 1;
	$fields['ct_created'] = date('Y-m-d H:i:s');
	$fields['ct_created_by'] = $user->uid;
	$fields['ct_updated'] = date('Y-m-d H:i:s');
	$fields['ct_updated_by'] = $user->uid;

	$ct_id = db_insert('dh_course_teacher')->fields( $fields )->execute();

	if($ct_id)
	{
		drupal_set_message("Applied in Course: {$values['course_name']}");
		log_course_teacher($ct_id, 'Application Received<br>Comments: '.$values['comment']);
		send_at_schedule_email($ct_id);
		send_at_schedule_email($ct_id, 1);
	}
	else
		drupal_set_message("Some issue in applying for course.");

  	drupal_goto("at-portal");
}





function show_at_application_form($form, &$form_state, $course_teacher_id)
{
	drupal_add_css( drupal_get_path('module', 'dh_manageapp'). "/css/jquery-confirm.min.css");
	drupal_add_js( drupal_get_path('module', 'dh_manageapp'). "/js/jquery-confirm.min.js");

	global $user;
	$at_code = $user->name;

	$q = "select t_id, t_gender from dh_teacher where t_status='Active' and CONCAT(t_code,'.',t_gender) = '$at_code' limit 1";
	$row = db_query($q)->fetchAssoc();
	$at_id = $row['t_id'];
	$at_gender = $row['t_gender'];

	if (!$at_id)
	{
		drupal_set_message("Only AT can apply for courses.", "error");
		drupal_goto("at-portal");
	}

	$q = "select
		co.c_id as 'course_id',
		co.c_start as 'c_start_unformatted',
		ct.ct_status as 'application_status',
		co.c_name as 'course_name',
		ce.c_id as 'center_id',
		ce.c_name as 'center_name',
		td.td_val1 as 'course_type',
		DATE_FORMAT(co.c_start, '%D %M %Y') as 'start_date',
		DATE_FORMAT(co.c_end, '%D %M %Y') as 'end_date',
		co.c_description as 'course_desctiption',
		co.c_comments as 'course_comments',
		co.c_at_m_count as 'male_at_needed',
		co.c_at_f_count as 'female_at_needed',
		co.c_at_m_conf as 'male_at_confirmed',
		co.c_at_f_conf as 'female_at_confirmed',
		co.c_deleted as 'course_deleted',
		co.c_cancelled as 'course_cancelled'
		from
		dh_course co
		left join dh_center ce on co.c_center=ce.c_id
		left join dh_type_detail td on co.c_course_type=td.td_id
		left join dh_course_teacher ct on co.c_id=ct.ct_course
		where ct.ct_id=:ct_id and
		ct.ct_teacher=:ct_teacher";

	$course_details = db_query($q, array(':ct_id'=>$course_teacher_id, ':ct_teacher'=>$at_id))->fetchAssoc();

	if (!$course_details)
	{
		drupal_set_message("Application not found for the Course.", "error");
		drupal_goto("at-portal");
	}

	$course_id = $course_details['course_id'];
	$course_name = $course_details['course_name'];
	$center_id = $course_details['center_id'];
	$start_date = $course_details['c_start_unformatted'];
	$app_status = $course_details['application_status'];
	$course_deleted = $course_details['course_deleted'];
	$course_cancelled = $course_details['course_cancelled'];
	

	$form['at-portal'] = array(
  		'#markup' => '<h2>'.l("Back to AT-Portal", "/at-portal").'</h2>',
	);

	$form['course-list'] = array(
  		'#markup' => '<h2>'.l("Back to Course List", "/at-portal/course-list").'</h2><br><br>',
	);

	$course_at_status = "M : ".
		($course_details['male_at_needed'] > $course_details['male_at_confirmed'] ? '<span style="color:green;">Open</span>' : '<span style="color:red;">Close</span>').
		"&nbsp&nbsp|&nbsp&nbspF: ".
		($course_details['female_at_needed'] > $course_details['female_at_confirmed'] ? '<span style="color:green;">Open</span>' : '<span style="color:red;">Close</span>');

	$data = array();

	$data[] = array("Center Name", $course_details['center_name']);
	$data[] = array("Course Type", $course_details['course_type']);
	$data[] = array("Start Date", $course_details['start_date']);
	$data[] = array("End Date", $course_details['end_date']);
	$data[] = array("Course AT Status", $course_at_status);
	$data[] = array("Course Desctiption", $course_details['course_desctiption'] == "" ? "None" : $course_details['course_desctiption']);
	$data[] = array("Course Comments", $course_details['course_comments'] == "" ? "None" : $course_details['course_comments']);

	$form['app_status'] = array(
  		'#markup' => "<h3>Application Status: $app_status</h3>",
	);

	$form['details'] = array(
		'#prefix' => '<h3>Course Details</h3>',
	    '#theme' => 'table',
	    '#rows' => $data,
	);

	$form['course_teacher_id'] = array(
		'#type' => 'value',
		'#value' => $course_teacher_id,
	);

	$form['at_id'] = array(
		'#type' => 'value',
		'#value' => $at_id,
	);

	$form['course_id'] = array(
		'#type' => 'value',
		'#value' => $course_id,
	);

	$form['course_name'] = array(
		'#type' => 'value',
		'#value' => $course_name,
	);

	$form['center_id'] = array(
		'#type' => 'value',
		'#value' => $center_id,
	);

	if (strtotime(date('Y-m-d')) > strtotime($start_date))
	{
		$form['course_started'] = array(
	  		'#markup' => '<h2 style="color:red">The Course is already Started</h2>',
		);
	}


	if ($course_deleted == 1)
	{
		$form['course_deleted'] = array(
	  		'#markup' => '<h2 style="color:red">The Course is Deleted</h2>',
		);
	}

	if ($course_cancelled == 1)
	{
		$form['course_cancelled'] = array(
	  		'#markup' => '<h2 style="color:red">The Course is Cancelled</h2>',
		);
	}

	
	if (!(strtotime(date('Y-m-d')) > strtotime($start_date)) && $course_deleted == 0 && $course_cancelled == 0)
	{
		$required = check_at_requirement($course_id, $at_gender);

		if ($app_status <> 'Cancelled')
		{

			$form['comment'] = array(
				'#title' => 'Optional Comments',
				'#type' => 'textarea',  
				'#rows' => 4,  
				'#maxlength' => 200,
			);


			$form['sub'] = array(
				'#type' => 'submit',
				'#value' => 'Cancel Course Application',
				'#prefix' => '<br>',
				'#attributes' => array(
					'onclick' => 'if(!confirm("Cancel Application for '.$course_name.'")){return false;}',
					'class' => array("cancel-app"),
				),
			);
		}
		elseif ($app_status == 'Cancelled' && $required)
		{
			$form['comment'] = array(
				'#title' => 'Optional Comments',
				'#type' => 'textarea',  
				'#rows' => 4,  
				'#maxlength' => 200,
			);

			$form['sub'] = array(
				'#type' => 'submit',
				'#value' => 'ReApply',
				'#prefix' => '<br>',
				'#attributes' => array(
					'onclick' => 'if(!confirm("ReApply for '.$course_name.'")){return false;}',
				),
			);
		}
		else
		{
			$form['at_not_needed'] = array(
		  		'#markup' => '<h2 style="color:red">'.$at_gender.' AT requirement is fulfilled</h2>',
			);
		}
		
	}


	$q = "select DATE_FORMAT(l_tstamp, '%r, %D %M %Y'), l_msg, name  from dh_log left join users on dh_log.l_user=users.uid where l_module='Course-Teacher' and l_identifier=:l_identifier order by l_id desc";
	$res = db_query($q, array(':l_identifier' => $course_teacher_id));

	if($res)
	{
		$data = array();

		while($row = $res->fetchAssoc())
		{
			$data[] = $row;
		}

		$headers = array('Timestamp', 'Activity', 'User');
		$form['activity-log'] = array(
			'#prefix' => '<br><h3>Activity Log</h3>',
		    '#theme' => 'table',
		    '#header' => $headers,
		    '#rows' => $data,
		);
	}

	

	$form['#attached']['css'][] = array(
		'data' => '.cancel-app { background-color: red !important; color: white !important;  }
					.course-detail-key {  }
					.course-detail-value {  }',
		'type' => 'inline',
	);


	return $form;

}


function show_at_application_form_validate($form, &$form_state)
{

}

function show_at_application_form_submit($form, &$form_state)
{
	global $user;
	$values = $form_state['values'];
	//drupal_set_message("<pre>".print_r($values, true)."</pre>");

	$fields = array();
	$fields['ct_updated'] = date('Y-m-d H:i:s');
	$fields['ct_updated_by'] = $user->uid;

	$temp = '';

	if ($values['sub'] == 'Cancel Course Application' )
	{
		$fields['ct_status'] = 'Cancelled';
		$temp = 'Cancelled';

	}
	elseif ($values['sub'] == 'ReApply' )
	{
		$fields['ct_status'] = 'Received';
		$temp = 'ReApplied';
	}
	
	$updated = db_update('dh_course_teacher')
		->fields( $fields )
		->condition('ct_id', $values['course_teacher_id'], '=')
		->execute();


	if($updated)
	{
		drupal_set_message("$temp Application for Course: {$values['course_name']}");
		log_course_teacher($values['course_teacher_id'], 'Status updated to '.$fields['ct_status'].'<br>Comments: '.$values['comment']);
		
		if ($fields['ct_status'] == 'Cancelled')
		{
			update_course_current_at_count( $values['course_id'] );
		}

		send_at_schedule_email($values['course_teacher_id']);
		send_at_schedule_email($values['course_teacher_id'], 1);
	}
	else
		drupal_set_message("Some issue in $temp Course Application.");	


  	drupal_goto("at-portal");
}



function at_applied_courses_list_form($form, &$form_state)
{

	global $user;
	$at_code = $user->name;

	$q = "select t_id from dh_teacher where t_status='Active' and CONCAT(t_code,'.',t_gender) = '$at_code' limit 1";
	$at_id = db_query($q)->fetchField();

	if (!$at_id)
	{
		drupal_set_message("Only AT can apply for courses.", "error");
		drupal_goto("at-portal");
	}

	drupal_add_css( libraries_get_path("editor"). "/DataTables/datatables.min.css");
	drupal_add_js( libraries_get_path("editor").  "/DataTables/datatables.min.js");
	drupal_add_css( drupal_get_path('module', 'dh_manageapp'). "/css/jquery-confirm.min.css");
	drupal_add_js( drupal_get_path('module', 'dh_manageapp'). "/js/jquery-confirm.min.js");
	drupal_add_css(drupal_get_path('module', 'dh_manageapp').'/css/select2.min.css');
	drupal_add_js(drupal_get_path('module', 'dh_manageapp').'/js/select2.min.js');


	$form['at-portal'] = array(
  		'#markup' => '<h2>'.l("Back to AT-Portal", "/at-portal").'</h2>',
	);


	$form['from_date'] = array(
		'#type' => 'date_popup',
		'#default_value' => date('Y-m-d'),
		'#title' => t('From Date'),
		'#size' => 12,
		'#weight' => 5,
		'#date_format' => 'Y-m-d',
		'#date_year_range' => '1969:+4',
		'#date_label_position' => 'above',
		'#required' => 1,
		'#theme_wrappers' => array('date_popup'),
		'#datepicker_options' => array(
			'#required' => 1,
			'dateFormat' => date_popup_format_to_popup('Y-m-d'),
		),
		'#attributes' => array(
		    'class' => array('from-date-class',),
		),
	);

	$form['to_date'] = array(
		'#type' => 'date_popup',
		'#default_value' => date('Y-m-d', strtotime('+1 year')),
		'#title' => t('To Date'),
		'#size' => 12,
		'#weight' => 10,
		'#date_format' => 'Y-m-d',
		'#date_year_range' => '1969:+4',
		'#date_label_position' => 'above',
		'#required' => 1,
		'#theme_wrappers' => array('date_popup'),
		'#datepicker_options' => array(
			'minDate' => 0,
			'#required' => 1,
			'dateFormat' => date_popup_format_to_popup('Y-m-d'),
		),
		'#attributes' => array(
		    'class' => array('to-date-class',),
		),
	);

	$statuses = array(
		"All" => "All Applications",
		"Received" => "Received",
		"Cancelled" => "Cancelled",
		"Confirmed" => "Confirmed",
	);

	$form['application-status'] = array(
		'#title' => 'Course Application Status',
		'#type' => 'select',
		'#options' => $statuses,
		'#weight' => 15, 
		'#attributes' => array(
		    'id' => 'application-status-id',
		),
	);


	$form['sub'] = array(
		'#type' => 'submit',
		'#value' => 'Search',
		'#weight' => 25,
		'#prefix' => '<br>',
		'#suffix' => '<br><br><br>',
	);

	$q = db_select('dh_course_teacher', 'ct');
	$q->leftjoin('dh_course', 'co', 'ct.ct_course=co.c_id');
	$q->leftjoin('dh_center', 'ce', 'co.c_center=ce.c_id');
	$q->leftjoin('dh_country', 'cy', 'ce.c_country=cy.c_code');
	$q->leftjoin('dh_state', 'st', '(ce.c_state=st.s_code AND ce.c_country=st.s_country)');
	$q->leftjoin('dh_city', 'ci', 'ce.c_city=ci.c_id');
	$q->leftjoin('dh_type_detail', 'td', 'co.c_course_type=td.td_id');
	$q->addField('ct', 'ct_id', 'course_teacher_id');
	$q->addField('ct', 'ct_status', 'course_teacher_status');
	$q->addField('ce', 'c_name', 'center_name');
	$q->addField('ce', 'c_subdomain', 'center_sub');
	$q->addField('td', 'td_val1', 'course_type');
	$q->addField('co', 'c_start', 'start_date');
	$q->addField('co', 'c_end', 'end_date');
	$q->addField('co', 'c_at_m_count', 'male_at_count');
	$q->addField('co', 'c_at_f_count', 'female_at_count');
	$q->addField('co', 'c_at_m_conf', 'male_at_conf');
	$q->addField('co', 'c_at_f_conf', 'female_at_conf');
	$q->addField('ci', 'c_name', 'city');
	$q->addField('st', 's_name', 'state');
	$q->condition('ct.ct_teacher', $at_id, '=');


	if (isset($form_state['user_input']) && $form_state['user_input'])
	{

		$values = $form_state['user_input'];

		//drupal_set_message("<pre>".print_r($values, true)."</pre>");

		if($values['from_date']['date'])
			$q->condition('co.c_start', $values['from_date']['date'], '>=');

		if($values['to_date']['date'])
			$q->condition('co.c_start', $values['to_date']['date'], '<=');

		if($values['application-status'] && $values['application-status'] !== "All")
		{
			$q->condition('ct.ct_status', $values['application-status'], '=');
		}
		
	}
	else
	{
		$q->condition('co.c_start', $form['from_date']['#default_value'], '>=');
		$q->condition('co.c_start', $form['to_date']['#default_value'], '<=');
	}
	

	// drupal_set_message($q->__toString());
	// drupal_set_message(print_r($q->arguments(), true));

	$q->condition('co.c_deleted', 0, '=');
	$q->condition('co.c_cancelled', 0, '=');

	$q->orderBy("co.c_start", "asc");
	$res = $q->execute();


	$rows = array();
	foreach ($res as $row) {
		
		$row = (array) $row;
		
     	$row = array_merge(array("apply-link"=>"<button class=\"show-course-application\" onclick=\"location.href='/at-portal/show-course-application/{$row['course_teacher_id']}'\" type=\"button\">View</button>"), $row);

		unset($row['course_teacher_id']);

		if ( $row['course_teacher_status'] == "Confirmed")
			$row['course_teacher_status'] = '<span style="color:#008000">'.$row['course_teacher_status'].'</span>';

		if ( $row['course_teacher_status'] == "Cancelled")
			$row['course_teacher_status'] = '<span style="color:#FF0000">'.$row['course_teacher_status'].'</span>';

		$row['course_teacher_status'] = "<b>{$row['course_teacher_status']}</b>";
		

		$row['start_date'] = date('d M y', strtotime($row['start_date']));
		$row['end_date'] = date('d M y', strtotime($row['end_date']));

		$row['course_at_status'] = "M : ".
			($row['male_at_count'] > $row['male_at_conf'] ? '<span style="color:green;">Open</span>' : '<span style="color:red;">Close</span>').
			"&nbsp&nbsp|&nbsp&nbspF: ".
			($row['female_at_count'] > $row['female_at_conf'] ? '<span style="color:green;">Open</span>' : '<span style="color:red;">Close</span>');

		// $uri = "public:///center-img/{$row['center_sub']}.jpg";
		// $placeholder_uri = "public:///center-img/placeholder.png.jpg";

		// $row['center_name'] = "<b>{$row['center_name']}</b>";

		// if (file_exists($uri)) {
		// 	$row['center_name'] .= '<br><div class="center-image"><img src="'.file_create_url($uri).'" alt="'.$row['center_sub'].'"></div>'; 
		// }
		// else
		// 	$row['center_name'] .= '<br><div class="center-image"><img src="'.file_create_url($placeholder_uri).'" alt="'.$row['center_sub'].'"></div>';


		$rows[] = $row;
	}

		$out = theme('table', array('rows' => array(), 'attributes' => array('id' => 'at-course-list')));

		$form['a'] = array('#markup' => $out, '#weight' => 101,);
		$data = json_encode($rows);



	$js = '
		(function ($) {
			function do_datatable( tid, data )
			{
				var table = $(tid).DataTable({
					dom: "lfr<\"clear\">tip", 
					data: data,
					responsive: {
			            details: {
			                display: $.fn.dataTable.Responsive.display.childRowImmediate,
			                type: "none",
			                target: ""
			            }
			        },
					destroy: true,
					"oLanguage": {"sEmptyTable": "No Courses found for Search Criteria."},
					searching: false,
					"columnDefs": [
					            {
					                "targets": [ 0 ],
					                "searchable": false
					            }
			        ],					
			        columns: [
			            { data: "apply-link", "title" : "Action" },
			            { data: "course_teacher_status", "title" : "App Status" },
			            { data: "center_name", "title" : "Center" },
			            { data: "city", "title" : "City" },
			            { data: "state", "title" : "State" },
			            { data: "course_type", "title" : "Course Type" },
			            { data: "start_date", "title" : "Start Date" },
				        { data: "end_date", "title" : "End Date"},
				        { data: "course_at_status", "title" : "Course AT Status"},
			        ],
			    });			
			}
			$(document).ready(function(){
				$("#edit-application-status").select2();
				
				var dataset = '.$data.';
				do_datatable("#at-course-list", dataset);

				$(".from-date-class").parent().parent().parent().parent().attr("id", "from-date-id-div");
				$(".to-date-class").parent().parent().parent().parent().attr("id", "to-date-id-div");
				$("#application-status-id").parent().attr("id", "application-status-id-div");
			});
		})(jQuery);
	';

	drupal_add_js($js, 'inline');

	$form['#attached']['css'][] = array(
		'data' => '#edit-from-date, #edit-to-date { float: none;}
					.container-inline-date {margin-bottom: 10px;}
					.show-course-application {
						background-color: #F5ECCE;
						border: 1px solid black;
						border-radius: 2px;
						color: #000;
						padding: 5px 25px;
						text-align: center;
						text-decoration: none;
						display: inline-block;
						font-family: "Helvetica Neue","Helvetica","Arial",sans-serif;
	    				font-size: 15px;
	    				-webkit-font-smoothing: antialiased;
	    				font-weight: 600;
					}
					#at-course-list td:nth-child(1) { width: 9%; text-align: left }
					#at-course-list td:nth-child(2) { width: 9%; text-align: left }
					#at-course-list td:nth-child(3) { width: 14%; text-align: left }
					#at-course-list td:nth-child(4) { width: 9%; text-align: left }
					#at-course-list td:nth-child(5) { width: 12%; text-align: left }
					#at-course-list td:nth-child(6) { width: 12%; text-align: left }
					#at-course-list td:nth-child(7) { width: 9%; text-align: left }
					#at-course-list td:nth-child(8) { width: 9%; text-align: left }
					#at-course-list td:nth-child(9) { width: 17%; text-align: left }
					#from-date-id-div { float: left; margin-right: 40px }
					#to-date-id-div { float: left; clear: none; }
					#application-status-id-div { clear: both; }

					',
		'type' => 'inline',
	);

	return $form;

}

function at_applied_courses_list_form_validate($form, &$form_state)
{

}

function at_applied_courses_list_form_submit($form, &$form_state)
{
	$form_state['user_input'] = $form_state['input'];
  	$form_state['rebuild'] = TRUE;
}


function log_course_teacher($at_app_id, $msg)
{
	global $user;

	$q = "select c_center from dh_course_teacher left join dh_course on ct_course=c_id where ct_id=".$at_app_id;
	$center = db_query($q)->fetchField();

	$fields = array();
	$fields['l_center'] = $center;
	$fields['l_module'] = 'Course-Teacher';
	$fields['l_identifier'] = $at_app_id;
	$fields['l_event'] = 'Change';
	$fields['l_msg'] = $msg;
	$fields['l_tstamp'] = date('Y-m-d H:i:s');
	$fields['l_user'] = $user->uid;

	db_insert('dh_log')->fields( $fields )->execute();
}


function update_course_current_at_count( $course_id )
{
	$q = "select COALESCE(sum(case when t_gender = 'M' then 1 else 0 end), 0) AS 'male_at_count', COALESCE(sum(case when t_gender = 'F' then 1 else 0 end), 0) AS 'female_at_count' from dh_course_teacher ct left join dh_teacher t on ct.ct_teacher=t.t_id where ct_course=:ct_course and ct_status='Confirmed' and ct_type <> 'Training';";

	$res = db_query($q, array(':ct_course' => $course_id))->fetchAssoc();

	$fields = array();
	$fields['c_at_m_conf'] = $res['male_at_count'];
	$fields['c_at_f_conf'] = $res['female_at_count'];
	
	$updated = db_update('dh_course')
		->fields( $fields )
		->condition('c_id', $course_id, '=')
		->execute();

}


function check_at_requirement($course_id, $at_gender)
{
	$q = "select c_at_m_count, c_at_m_conf, c_at_f_count, c_at_f_conf from dh_course where c_id='$course_id'";
	$row = db_query($q)->fetchAssoc();

	if ($at_gender == 'M')
	{
		$at_need = $row['c_at_m_count'];
		$at_conf = $row['c_at_m_conf'];
	}
	else
	{
		$at_need = $row['c_at_f_count'];
		$at_conf = $row['c_at_f_conf'];
	}

	if ($at_need > $at_conf)
		return true;
	else
		return false;
}


function _get_at_app_activity()
{
	$course_teacher_id = arg(1);

	$q = "select DATE_FORMAT(l_tstamp, '%r, %D %M %Y'), l_msg, name from dh_log left join users on dh_log.l_user=users.uid where l_module='Course-Teacher' and l_identifier=:l_identifier order by l_id desc";
	$res = db_query($q, array(':l_identifier' => $course_teacher_id));


	$data = array();

	while($row = $res->fetchAssoc())
	{
		$data[] = $row;
	}

	$headers = array('Timestamp', 'Activity', 'User');
	$out = '<h4>Activity Log</h4>';
	if (!empty($data))
		$out .= theme('table', array('header' => $headers, 'rows' => $data)) ;
	print $out;
	exit;

}


function send_at_schedule_email($at_app_id, $send_to_center=0)
{
	$row = db_query("select t_f_name, t_l_name, t_email, ct_status, co.c_name as 'course_name', ce.c_name as 'center_name', ce.c_phone as 'contact', cs_email_from_name, cs_email_reply_to from dh_course_teacher ct left join dh_course co on ct.ct_course=co.c_id left join dh_center ce on co.c_center=ce.c_id left join dh_center_setting cs on ce.c_id=cs_center left join dh_teacher t on ct.ct_teacher=t.t_id where ct.ct_id=$at_app_id")->fetchAssoc();

	$mg = Mailgun::create(variable_get('mailgun_key', ''));


	$to = trim($row['t_email']);
	$from = $row['cs_email_from_name'].' <'.trim($row['cs_email_reply_to']).'>';
	$contact = $row['contact'];

	$subject = "AT Application for Course - {$row['course_name']}";
	$body = "
		Dear {$row['t_f_name']} {$row['t_l_name']},<br><br><br>
		Current Status of your AT Application for course \"{$row['course_name']}\" is <b>{$row['ct_status']}</b>.<br><br><br>
		With Regards,<br><br>
		{$row['cs_email_from_name']}<br>
		Contact Number: $contact<br>
	";

	if ($send_to_center)
	{
		$to = trim($row['cs_email_reply_to']);
		$from = $row['cs_email_from_name'].' <'.trim($row['cs_email_reply_to']).'>';

		$subject = "{$row['ct_status']}, AT Application for Course - {$row['course_name']}";
		$body = "
			Dear {$row['cs_email_from_name']},<br><br><br>
			
			<b>{$row['ct_status']}</b>, AT Application for Course - {$row['course_name']}<br>
			AT Name: {$row['t_f_name']} {$row['t_l_name']}.<br><br><br>


			---System Generated Email---
		";
	}


	$test_mode = variable_get('mode_test', '0');
	//$test_mode = 0;

	if ( $test_mode )
		$to = variable_get('mode_test_emails', '');

	$options = array(
	  'from'    => $from,
	  'to'      => $to,
	  'subject' => $subject,
	  'html'    => $body,
	  'v:at-app-id' => $at_app_id,
	  'v:test-mode' => $test_mode
	);


	$res = $mg->messages()->send(variable_get('mailgun_primary_domain', ''), $options );
	$log = "<b>{$row['ct_status']}</b>, Email Notification sent to - ".$to." ( Response: ".$res->getId()." )";
	log_course_teacher($at_app_id, $log);
}

