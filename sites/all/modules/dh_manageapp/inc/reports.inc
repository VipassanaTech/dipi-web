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
	drupal_add_css( drupal_get_path('module', 'dh_manageapp'). "/css/jquery-confirm-responsive.css");
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
	// Day / Month / Year dropdowns (inc/dmy.inc), DIPI's usual date input.
	$years = array(2000, (int) date('Y'));
	$form['report_from_date'] = dh_dmy_element(array('title' => t('From'), 'default' => date('Y-m-d', strtotime('-1 year')),
		'required' => TRUE, 'years' => $years, 'weight' => 5));
	$form['report_to_date'] = dh_dmy_element(array('title' => t('To'), 'default' => date('Y-m-d'),
		'required' => TRUE, 'years' => $years, 'weight' => 10));

	$form['sub'] = array(
		'#type' => 'submit',
		'#value' => 'Download Course Report',
		'#weight' => 15,
		'#prefix' => '<br>',
	);
	return $form;
}


/** To must not be before From (both are already real dates after the day/month/year fields' own check). */
function dh_center_course_report_form_validate($form, &$form_state)
{
	$from = $form_state['values']['report_from_date'];
	$to = $form_state['values']['report_to_date'];
	if ($from !== '' && $to !== '' && $to < $from)
		form_set_error('report_to_date', t('The To date cannot be before the From date.'));
}

/** Course report rows of a centre for courses starting between two dates ('Y-m-d'); the values reach the database only as data. */
function dh_course_report_rows($centre_id, $from_date, $to_date)
{
	$q = "select CourseID, Course, NewMale, NewFemale, (NewMale + NewFemale) as NewTotal, OldMale, OldFemale, (OldMale + OldFemale) as OldTotal , (NewMale + NewFemale + OldMale + OldFemale) as StudentTotal, SevakMale, SevakFemale, (SevakMale + SevakFemale) as SevakTotal from 
		( select co.c_id as 'CourseID', co.c_name as Course, count(case when a.a_type='Student' and a.a_gender = 'M' and a.a_old = 0 then a.a_id end) as NewMale, count(case when a.a_type='Student' and a.a_gender = 'F' and a.a_old = 0 then a.a_id end) AS NewFemale, count(case when a.a_type='Student' and a.a_gender = 'M' and a.a_old = 1 then a.a_id end) AS OldMale, count(case when a.a_type='Student' and a.a_gender = 'F' and a.a_old = 1 then a.a_id end) AS OldFemale, count(case when a.a_type='Sevak' and a.a_gender = 'M' then a.a_id end) AS SevakMale, count(case when a.a_type='Sevak' and a.a_gender = 'F' then a.a_id end) AS SevakFemale from dh_applicant a left join dh_course co on a.a_course=co.c_id where a.a_attended=1 and DATE(co.c_start)>=:from and DATE(co.c_start)<=:to and c_cancelled=0 and c_deleted=0 and a.a_center = :centre group by co.c_name order by co.c_start) q";
	return db_query($q, array(':from' => (string) $from_date, ':to' => (string) $to_date, ':centre' => (int) $centre_id));
}

function dh_center_course_report_form_submit($form, &$form_state)
{
	$centre_id = arg(1);
	$from_date = $form_state['values']['report_from_date'];
	$to_date = $form_state['values']['report_to_date'];

	$result = dh_course_report_rows($centre_id, $from_date, $to_date);


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




/** Seconds a built finalize report is given out again (it takes 9-14 s to build on prod). */
define('DH_FINALIZE_CSV_KEEP', 900);

/** Cache id (and lock name) of one of the two finalize reports. */
function dh_finalize_csv_cid($not_marked)
{
  return 'dh_finalize_csv:' . ($not_marked ? 'not_marked' : 'marked');
}

/** The finalize report as CSV text: runs the (slow) query. $not_marked: courses with no one marked attended. */
function dh_finalize_csv_build($not_marked)
{
  $append = $not_marked ? " and MarkedA = 0" : " and MarkedA <> 0";
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
  $fp = fopen('php://temp', 'w+');
  $header_done = false;
  while ($result && ($row = $result->fetchAssoc()))
  {
    if (!$header_done)
    {
      fputcsv($fp, array_keys($row));
      $header_done = true;
    }
    fputcsv($fp, $row);
  }
  rewind($fp);
  $csv = stream_get_contents($fp);
  fclose($fp);
  return $csv;
}

/**
 * The finalize report, built at most once every 15 minutes: everyone in between gets the same copy, so repeated
 * downloads (the pages need no login) cannot load the database. While another download is building it, the
 * previous copy is given; with no copy at all this waits up to $wait seconds for that build, else builds it.
 */
function dh_finalize_csv_get($not_marked, $wait = 30)
{
  $cid = dh_finalize_csv_cid($not_marked);
  $kept = cache_get($cid);
  if ($kept && $kept->expire > REQUEST_TIME)
    return $kept->data;
  if (lock_acquire($cid, 120))
  {
    $csv = dh_finalize_csv_build($not_marked);
    cache_set($cid, $csv, 'cache', REQUEST_TIME + DH_FINALIZE_CSV_KEEP);
    lock_release($cid);
    return $csv;
  }
  if ($kept)
    return $kept->data;
  lock_wait($cid, $wait);
  $kept = cache_get($cid);
  return $kept ? $kept->data : dh_finalize_csv_build($not_marked);
}

function finalize_csv()
{
  $not_marked = (arg(0) == "finalize-not-marked-attended");
  $filename = $not_marked ? "NotMarkedAttended" : "MarkedAttended";
  $csv = dh_finalize_csv_get($not_marked);

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
  print $csv;
  exit();
}
