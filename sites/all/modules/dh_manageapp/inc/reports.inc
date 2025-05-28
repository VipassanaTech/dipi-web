<?php



function dh_center_sms_report()
{
	global $user;
  	$_SESSION['lasturi'] = $_SERVER['REQUEST_URI'];

  	$centre_id = arg(1);
	$out  = '<h2>'.l("Back to Dashboard", "/centre/$centre_id").'</h2>';


	$iPod    = stripos($_SERVER['HTTP_USER_AGENT'],"iPod");
	$iPhone  = stripos($_SERVER['HTTP_USER_AGENT'],"iPhone");
	$iPad    = stripos($_SERVER['HTTP_USER_AGENT'],"iPad");
	drupal_add_css( libraries_get_path("editor"). "/DataTables/datatables.min.css");
	drupal_add_js( libraries_get_path("editor").  "/DataTables/datatables.min.js");
	if ( !($iPad || $iPhone || $iPod) )
	{
		drupal_add_js( libraries_get_path("editor").  "/DataTables/Buttons-1.5.1/js/buttons.html5.min.js");
		drupal_add_js( libraries_get_path("editor").  "/DataTables/Buttons-1.5.1/js/buttons.print.min.js");
	}

	drupal_add_css( drupal_get_path('module', 'dh_manageapp'). "/css/jquery-confirm.min.css");
	drupal_add_js( drupal_get_path('module', 'dh_manageapp'). "/js/jquery-confirm.min.js");
	
	drupal_add_css(drupal_get_path('module', 'dh_manageapp').'/css/tables.css' );


	$q = "select cs_course, c_name as 'Course', SUM(cs_credits_used) as 'Count', l_name as 'Letter' from dh_center_sms left join dh_course on cs_course = c_id left join dh_letter on cs_letter = l_id where cs_center=$centre_id group by cs_course order by c_start";

	$result = db_query($q);
	$rows = array();
	while( $r = $result->fetchAssoc() )
	{
		unset($rs);
		$rs['cid'] = $r['cs_course'];
		$rs['course'] = $r['Course'];
		$rs['count'] = $r['Count'];
		$rows[] = $rs;
	}

	$header = array();
	$attributes = array('id' => 'table-applicants');
	$out .= theme('table', array('header' => $header, 'rows' => array(), 'attributes' => $attributes));
	$data = json_encode($rows);

	$js = '
		function format ( d ) {
		    return "<div class=\"slider\">"+
		        "<table cellpadding=\"5\" cellspacing=\"0\" border=\"0\" style=\"margin-left:50px;width:100%\">"+        
		            "<tr>"+
		                "<td colspan=3><div class=\"activity-log-"+d.cid+"\">Loading log...</div></td>"+
		             "</tr>"+		       
 		            "<tr>"+
		        "</table>"+
		    "</div>";
		}
		(function ($) {
			$(document).ready(function(){

				var dataset = '.$data.';
				var comment = "";
			    var iOS = !!navigator.platform && /iPad|iPhone|iPod/.test(navigator.platform);

				var table = $("#table-applicants").DataTable({
					data: dataset,					
			        columns: [
			            {
			                "class":          "details-control",
			                "orderable":      false,
			                "data":           null,
			                "title":		  "Detail",
			                "defaultContent": ""
			            },				        
			            { data: "course", "title" : "Course" },
			            { data: "count", "title" : "Total Count" },
		            ],   	
				});

			    // Add event listener for opening and closing details
			    $("#table-applicants tbody").on("click", "td.details-control", function () {
			        var tr = $(this).closest("tr");
			        var row = table.row( tr );
			 
			        if ( row.child.isShown() ) {
			            // This row is already open - close it
			            $("div.slider", row.child()).slideUp( function () {
			                row.child.hide();
			                tr.removeClass("shown");
			            });
			        }
			        else {
			            // Open this row
			            row.child( format(row.data()), "no-padding" ).show();			         
			            tr.addClass("shown");			 
			            $("div.slider", row.child()).slideDown();
			            $(".activity-log-"+row.data().cid).load("/sms-count/"+row.data().cid);          	
			        }
			    });
			});
		})(jQuery);
	';
	drupal_add_js($js, 'inline');	
	return $out;

}


function dh_center_course_report_form($form, &$form_state)
{
	$centre_id = arg(1);

	$form = array();
	$form['dashboard'] = array(
  		'#markup' => '<h2>'.l("Back to Dashboard", "/centre/$centre_id").'</h2>',
	);
	$form['heading'] = array(
  		'#markup' => '<h2>Get Report for Courses Having Start Date:</h2>',
	);
	$form['report_from_date'] = array(
		'#type' => 'date_popup',
		'#default_value' => date('Y-m-d', strtotime('-1 year')),
		'#title' => t('From'),
		'#size' => 12,
		'#weight' => 5,
		'#date_format' => 'Y-m-d',
		'#date_year_range' => '-100:+0',
		'#date_label_position' => 'above',
		'#theme_wrappers' => array('date_popup'),
		'#datepicker_options' => array(
			'maxDate' => 0,
			'#required' => 1,
			'dateFormat' => date_popup_format_to_popup('Y-m-d'),
		),
	);

	$form['report_to_date'] = array(
		'#type' => 'date_popup',
		'#default_value' => date('Y-m-d'),
		'#title' => t('To'),
		'#size' => 12,
		'#weight' => 10,
		'#date_format' => 'Y-m-d',
		'#date_year_range' => '-100:+0',
		'#date_label_position' => 'above',
		'#theme_wrappers' => array('date_popup'),
		'#datepicker_options' => array(
			'maxDate' => 0,
			'#required' => 1,
			'dateFormat' => date_popup_format_to_popup('Y-m-d'),
		),
	);

	$form['#attached']['css'][] = array(
		'data' => '#edit-report-from-date, #edit-report-to-date { float: none; }',
		'type' => 'inline',
	);


	$form['sub'] = array(
		'#type' => 'submit',
		'#value' => 'Download Course Report',
		'#weight' => 15,
		'#prefix' => '<br>',
	);
	return $form;
}


function dh_center_course_report_form_submit($form, &$form_state)
{
	$centre_id = arg(1);
	$from_date = $form_state['input']['report_from_date']['date'];
	$to_date = $form_state['input']['report_to_date']['date'];

	$q = "select CourseID, Course, NewMale, NewFemale, (NewMale + NewFemale) as NewTotal, OldMale, OldFemale, (OldMale + OldFemale) as OldTotal , (NewMale + NewFemale + OldMale + OldFemale) as StudentTotal, SevakMale, SevakFemale, (SevakMale + SevakFemale) as SevakTotal from 
		( select co.c_id as 'CourseID', co.c_name as Course, count(case when a.a_type='Student' and a.a_gender = 'M' and a.a_old = 0 then a.a_id end) as NewMale, count(case when a.a_type='Student' and a.a_gender = 'F' and a.a_old = 0 then a.a_id end) AS NewFemale, count(case when a.a_type='Student' and a.a_gender = 'M' and a.a_old = 1 then a.a_id end) AS OldMale, count(case when a.a_type='Student' and a.a_gender = 'F' and a.a_old = 1 then a.a_id end) AS OldFemale, count(case when a.a_type='Sevak' and a.a_gender = 'M' then a.a_id end) AS SevakMale, count(case when a.a_type='Sevak' and a.a_gender = 'F' then a.a_id end) AS SevakFemale from dh_applicant a left join dh_course co on a.a_course=co.c_id where a.a_attended=1 and DATE(co.c_start)>='$from_date' and DATE(co.c_start)<='$to_date' and a.a_center=$centre_id group by co.c_name order by co.c_start) q";

	$result = db_query($q);


	$header = array('Course', 'NewMale', 'NewFemale', 'NewTotal', 'OldMale', 'OldFemale', 'OldTotal', 'StudentTotal', 'SevakMale', 'SevakFemale', 'SevakTotal', 'ConductingTeachers', 'AssistingTeachers', 'TrainingTeachers');

	$fname = tempnam(sys_get_temp_dir(), "course_report");
	$fp = fopen($fname, "w");
	fputcsv($fp, $header);

	$new_male_total=0;
	$new_female_total=0;
	$new_total=0;
	$old_male_total=0;
	$old_female_total=0;
	$old_total=0;
	$student_total=0;
	$sevak_male_total=0;
	$sevak_female_total=0;
	$sevak_total=0;

	while($row = $result->fetchAssoc())
	{
		$course_id = $row['CourseID'];
		$q = "SELECT ct_type, CONCAT(t_f_name, ' ', t_l_name, ' (', t_gender, ')') as 'name' FROM dh_course_teacher ct left join dh_teacher t on ct.ct_teacher=t.t_id where ct_course=$course_id and ct_status='Confirmed' order by t_gender";
		unset($row['CourseID']);
		$out = db_query($q);

		$row['Conducting']='';
		$row['Assisting']='';
		$row['Training']='';
		while($r = $out->fetchAssoc())
		{
			$row[$r['ct_type']] = $row[$r['ct_type']].$r['name']."\n";
		}
		$row['Conducting'] = trim($row['Conducting']);
		$row['Assisting'] = trim($row['Assisting']);
		$row['Training'] = trim($row['Training']);
		fputcsv($fp, $row);

		$new_male_total = $new_male_total + $row['NewMale'];
		$new_female_total = $new_female_total + $row['NewFemale'];
		$new_total = $new_total + $row['NewTotal'];
		$old_male_total = $old_male_total + $row['OldMale'];
		$old_female_total = $old_female_total + $row['OldFemale'];
		$old_total = $old_total + $row['OldTotal'];
		$student_total = $student_total + $row['StudentTotal'];
		$sevak_male_total = $sevak_male_total + $row['SevakMale'];
		$sevak_female_total = $sevak_female_total + $row['SevakFemale'];
		$sevak_total = $sevak_total + $row['SevakTotal'];

	}

	fputcsv($fp, array('', 'NewMale', 'NewFemale', 'NewTotal', 'OldMale', 'OldFemale', 'OldTotal', 'StudentTotal', 'SevakMale', 'SevakFemale', 'SevakTotal',));

	fputcsv($fp, array('Total',$new_male_total, $new_female_total, $new_total, $old_male_total, $old_female_total, $old_total, $student_total, $sevak_male_total, $sevak_female_total, $sevak_total));

	fclose($fp);

	header("Content-Type: text/csv");
	header('Content-disposition:attachment; filename="course report '.$from_date.' to '.$to_date.'.csv"');
	header('Cache-Control: max-age=0');
	// If you're serving to IE 9, then the following may be needed
	//header('Cache-Control: max-age=1');

	// If you're serving to IE over SSL, then the following may be needed
	header ('Expires: Mon, 26 Jul 1997 05:00:00 GMT'); // Date in the past
	header ('Last-Modified: '.gmdate('D, d M Y H:i:s').' GMT'); // always modified
	header ('Cache-Control: cache, must-revalidate'); // HTTP/1.1
	header ('Pragma: public'); // HTTP/1.0
	readfile($fname);
	unlink($fname);
	exit();
}




function finalize_csv()
{
  $filename = "MarkedAttended";
  $append = " and MarkedA <> 0";
  if(arg(0) == "finalize-not-marked-attended")
  {
    $append = " and MarkedA = 0";
    $filename = "NotMarkedAttended";
  }


  $q = "select
      Course,
      MarkedA,
      TotalApplications
      from
      (
        select
        co.c_name as Course,
        count(case when a.a_attended=1 then a.a_id end) AS MarkedA,
        count(a.a_id) AS TotalApplications,
        co.c_finalized as Finalized,
        c.c_phone as Contact,
        c.c_email as Email
        from
        dh_applicant a
        left join dh_course co on a.a_course=co.c_id
        left join dh_center c on co.c_center=c.c_id
        where
        co.c_cancelled=0 and
        co.c_deleted=0 and
        co.c_end<curdate() and
        a.a_center<>286 and
        co.c_course_type=6 and
        co.c_name is not null
        group by
        a.a_course
        order by
        co.c_center ASC, co.c_start ASC
      ) q
      where
      Finalized = 0 ";

  $q .= $append;


  $result = db_query($q);

  if (!$result)
  {
    drupal_set_message("no record found.");
    exit();
  }

  $fname = tempnam(sys_get_temp_dir(), $filename);
  $fp = fopen($fname, "w");
  $header_done = false;

  while($row = $result->fetchAssoc())
  {
    if(!$header_done)
    {
      $header = array_keys($row);
      fputcsv($fp, $header);
      $header_done = true;
    }
    fputcsv($fp, $row);
  }
  fclose($fp);

  header("Content-Type: text/csv");
  header('Content-disposition:attachment; filename="'.$filename.'.csv"');
  header('Cache-Control: max-age=0');
  // If you're serving to IE 9, then the following may be needed
  //header('Cache-Control: max-age=1');

  // If you're serving to IE over SSL, then the following may be needed
  header ('Expires: Mon, 26 Jul 1997 05:00:00 GMT'); // Date in the past
  header ('Last-Modified: '.gmdate('D, d M Y H:i:s').' GMT'); // always modified
  header ('Cache-Control: cache, must-revalidate'); // HTTP/1.1
  header ('Pragma: public'); // HTTP/1.0
  readfile($fname);
  unlink($fname);
  exit();
}
