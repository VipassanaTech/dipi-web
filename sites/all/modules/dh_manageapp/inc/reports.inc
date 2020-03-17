<?php


function dh_manage_at_schedule()
{
	global $user;
	$centre_id = arg(1);
	drupal_add_css(drupal_get_path( 'module', 'dh_manageapp').'/css/manageapp.css');
	
	$course = db_query("select c_id from dh_course where c_center=$centre_id")->fetchAssoc();
	

	$q = "Select ct_id, c_id, c_start, c_end, c_name, CONCAT(t_f_name, ' ', t_l_name) as 'teacher', ct_type as 'Type', t_gender from dh_course left join dh_course_teacher on ct_course=c_id left join dh_teacher on ct_teacher=t_id where c_start >= ((CURDATE() - INTERVAL 1 YEAR)) and (c_deleted = 0) and c_center=$centre_id order by c_start asc, ct_type desc";

	$result = db_query($q);
	$data = array();

	while($r = $result->fetchAssoc())
	{	
		$data[$r['c_id'].'||||'.$r['c_name'].'||||'.$r['c_start']][$r['t_gender']][] = array('type' => $r['Type'], 'teacher' => $r['teacher']) ;
		
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
		if ( $t[2]  <= date('Y-m-d') ) 

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