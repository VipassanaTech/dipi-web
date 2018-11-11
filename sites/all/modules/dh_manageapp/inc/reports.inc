<?php


function dh_manage_at_schedule()
{
	global $user;
	$centre_id = arg(1);
	drupal_add_css(drupal_get_path( 'module', 'dh_manageapp').'/css/manageapp.css');
	
	$course = db_query("select c_id from dh_course where c_center=$centre_id")->fetchAssoc();
	

	$q = "Select ct_id, c_id, c_start, c_end, c_name, CONCAT(t_f_name, ' ', t_l_name) as 'teacher', ct_type as 'Type', t_gender from dh_course_teacher left join dh_teacher on ct_teacher=t_id left join dh_course as c on ct_course=c_id where c_start >= ((CURDATE() - INTERVAL 1 YEAR)) and c.c_center=$centre_id order by c_start asc, ct_type desc";

	$result = db_query($q);
	$data = array();
	$i = 1;

	while($r = $result->fetchAssoc())
	{	
		$data[$r['c_id'].'||||'.$r['c_name'].'||||'.$r['c_end']][$r['t_gender']][] = array('type' => $r['Type'], 'teacher' => $r['teacher']) ;
		
	}
	$old_rows = $upcoming_rows = array();
	$gender = array('M', 'F');
	foreach($data as $course => $course_data)
	{
		unset($row);	
		$t = explode("||||", $course);
		$row[] = $t[1];

		foreach($gender as $g)
		{			
			$temp = "";
			if (isset($course_data[$g]))
			{
				foreach($course_data[$g] as $gender_data )
				{
					$temp .= $gender_data['teacher'].' - '.$gender_data['type'].'<br>' ;
				}
			}
			$row[] = $temp;
		}

		$row[] = l("View", "assign-teacher/$centre_id/".$t[0]);
		if ( $t[2]  < date('Y-m-d') ) 


			$old_rows[] = $row;
		else
			$upcoming_rows[] = $row;


			
	}
	
	$header = array('Course Name', 'Male', 'Female', 'Action');
	$out = '<h3>'.l("Back to Dashboard", "centre/".arg(1)).'</h3>';


	$completed = theme("table", array('header' => $header, 'rows' => $old_rows ));
	$fieldset_vars = array(
		'element' => array(
		'#title' => t('Completed courses'),
		'#collapsible' => true,
		'#collapsed' => true,
		'#value' => '',
		'#children' => $completed
		),
	);

	$out .= theme('fieldset', $fieldset_vars);


	$out .= '<h3>Upcoming courses</h3>';
	$out .= theme("table", array('header' => $header, 'rows' => $upcoming_rows ));


	return $out;
}