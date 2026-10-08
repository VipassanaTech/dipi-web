<?php


function dh_manage_at_schedule()
{
	global $user;
	$centre_id = arg(1);
  $center_name = db_query("select c_name from dh_center where c_id=$centre_id")->fetchField();
	drupal_add_css(drupal_get_path( 'module', 'dh_manageapp').'/css/manageapp.css');
	drupal_add_css( libraries_get_path("editor"). "/DataTables/datatables.min.css");
	drupal_add_js( libraries_get_path("editor").  "/DataTables/datatables.min.js");
	drupal_add_css(drupal_get_path('module', 'dh_manageapp').'/css/select2.min.css');
	drupal_add_js(drupal_get_path('module', 'dh_manageapp').'/js/select2.min.js');
	drupal_add_css( drupal_get_path('module', 'dh_manageapp'). "/css/jquery-confirm.min.css");
	drupal_add_css( drupal_get_path('module', 'dh_manageapp'). "/css/jquery-confirm-responsive.css");
	drupal_add_js( drupal_get_path('module', 'dh_manageapp'). "/js/jquery-confirm.min.js");
	drupal_add_css(drupal_get_path('module', 'dh_manageapp').'/css/tables.css' );

	$iPod    = stripos($_SERVER['HTTP_USER_AGENT'],"iPod");
	$iPhone  = stripos($_SERVER['HTTP_USER_AGENT'],"iPhone");
	$iPad    = stripos($_SERVER['HTTP_USER_AGENT'],"iPad");

	if ( !($iPad || $iPhone || $iPod) )
	{
		drupal_add_js( libraries_get_path("editor").  "/DataTables/Buttons-1.5.1/js/buttons.print.min.js");
    drupal_add_js( libraries_get_path("editor").  "/DataTables/Buttons-1.5.1/js/buttons.html5.js");
    drupal_add_js( libraries_get_path("editor").  "/DataTables/pdfmake-0.1.32/pdfmake.min.js");
    drupal_add_js( libraries_get_path("editor").  "/DataTables/pdfmake-0.1.32/vfs_fonts.js");
	}
	

	
	$out = '<h3>'.l("Back to Dashboard", "centre/".arg(1)).'</h3>';



	$q = "Select
		t_id,
		ct_id,
		c_id,
		c_name,
		CONCAT(t_f_name, ' ', t_l_name) as 'teacher',
		t_gender,
		ct_updated,
		ct_status
		from
		dh_course
		left join dh_course_teacher on ct_course=c_id
		left join dh_teacher on ct_teacher=t_id
		where
		ct_status='Received' and
		c_deleted=0 and
		c_cancelled=0 and
		c_start >= curdate() and
		c_center=$centre_id
		order by
		ct_updated
		desc";

	$result = db_query($q);
	$recent_data = array();
	while($r = $result->fetchAssoc())
	{	
		$change_status = '
			<select id=\"new-at-status-'.$r['ct_id'].'" name="newstatus'.$r['ct_id'].'" class="at-status-select">
			<option value="">Choose</option>
		';

		if ( $r['ct_status'] == "Received" )
		{
			$change_status .= '
				<option value="Confirmed">Confirmed</option>
				<option value="Cancelled">Cancelled</option>
			';
		}
		elseif ( $r['ct_status'] == "Confirmed" )
		{
			$change_status .= '
				<option value="Cancelled">Cancelled</option>
			';
		}
		elseif ( $r['ct_status'] == "Cancelled" )
		{
			$change_status .= '
				<option value="Confirmed">Confirmed</option>
			';
		}

		$change_status .= '
			</select>
		';

		$recent_data[] = array(
			'ct_id' => $r['ct_id'],
			'course_name_link' => l($r['c_name'], "/assign-teacher/{$centre_id}/{$r['c_id']}"),
			'teacher_formatted' => '<a href="#" id="'.$r['t_id'].'" class="get-at-info" >'.$r['teacher'].'</a>', 
			't_gender' => $r['t_gender'], 
			'ct_updated' =>$r['ct_updated'],
			'change_status' =>$change_status,
			'teacher' => $r['teacher'],
			'course_name' => $r['c_name'],
			'current_status' => $r['ct_status'],
		);

	}

	$out .= "<br><br><h2>Recent Received AT Applications</h2>";
	$out .= theme('table', array('rows' => array(), 'attributes' => array('id' => 'recent-at-apps')));
	$recent_data = json_encode($recent_data);





	$q = "Select
		c_id,
		c_start,
		c_end,
		c_name,
		c_at_m_count,
		c_at_f_count,
		t_id,
		CONCAT(t_f_name, ' ', t_l_name) as 'teacher',
		t_gender,
		ct_id,
		ct_type as 'Type',
		ct_status as 'status'
		from
		dh_course
		left join dh_course_teacher on ct_course=c_id
		left join dh_teacher on ct_teacher=t_id
		where
		c_deleted=0 and
		c_cancelled=0 and
		c_center=$centre_id
		order by
		c_start asc,
    ct_status desc,
		t_gender desc,
		ct_type
		asc";

	$result = db_query($q);
	$data = array();

	while($r = $result->fetchAssoc())
	{	
		$data[$r['c_id'].'||||'.$r['c_name'].'||||'.$r['c_start'].'||||'.$r['c_at_m_count'].'||||'.$r['c_at_f_count']][] = array('type' => $r['Type'], 'teacher_id' => $r['t_id'], 'teacher' => $r['teacher'], 'status' => $r['status'], 'gender' => $r['t_gender'], 'ct_id' => $r['ct_id']);

	}
	$old_rows = $upcoming_rows = array();

	foreach($data as $course => $course_data)
	{
		unset($row);	
		$t = explode("||||", $course);

		$row['course_name'] = l($t[1], "assign-teacher/$centre_id/".$t[0]);
		$row['at_required'] = "M: {$t[3]}&nbsp;&nbsp;|&nbsp;&nbsp;F: {$t[4]}";
    $course_name_uformtd_array = explode("/", $t[1]);
		$row['course_name_uformtd'] = $course_name_uformtd_array[1]."/".$course_name_uformtd_array[3]."/".$course_name_uformtd_array[2];
		$row['at_required_uformtd'] = "M: {$t[3]}  |  F: {$t[4]}";
		
		$temp = array();
		$row['teachers_uformtd'] = "";


		foreach($course_data as $data )
		{
			if($data['ct_id'])
			{
				$gender = $data['gender']=='M' ? "Male" : "Female";
				if ( $t[2]  < date('Y-m-d') )
				{
					if ( $data['status'] == "Confirmed")
					{
						$temp[] = array('<a href="#" id="'.$data['teacher_id'].'" class="get-at-info" >'.$data['teacher'].'</a>', $data['type'].' '.$gender);
						$row['teachers_uformtd'] .= "{$data['teacher']} - {$data['type']} {$gender}\n";
					}
				}
				else
				{
          $at_name = $data['teacher'];
          if ($data['status'] == "Confirmed")
            $at_app_status = '<span style="color:green;">'.$data['status'].'</span>';
          elseif ($data['status'] == "Cancelled")
          {
            $at_app_status = '<span style="color:red;">'.$data['status'].'</span>';
            $at_name = "<del>{$data['teacher']}</del>";
          }
          else
            $at_app_status = $data['status'];

					$temp[] = array('<a href="#" id="'.$data['teacher_id'].'" class="get-at-info" >'.$at_name.'</a>', $data['type'].' '.$gender, $at_app_status);

          if(!$at_status)
            $at_status = $data['status'];
          elseif($at_status <> $data['status'])
          {
            $row['teachers_uformtd'] .= "\n";
            $at_status = $data['status'];
          }

					$row['teachers_uformtd'] .= "{$data['teacher']} - {$data['type']} {$gender} ({$data['status']})\n";
				}
				
			}
		}

		$row['teachers'] = theme("table", array('rows' => $temp, 'attributes' => array('class' => array('teacher-internal-table', ), ), ), );
		$row['teachers_uformtd'] = trim($row['teachers_uformtd']);

		$date = DateTime::createFromFormat("Y-m-d", $t[2]);

		if ($date)
		{
			$year = $date->format("Y");

			if ( $t[2]  < date('Y-m-d') ) 
				$old_rows[$year][] = $row;
			else
				$upcoming_rows[] = $row;
		}
		else
		{
			drupal_set_message("Start date not set for course: {$t[1]}");
		}

			
	}





	$out .= "<br><br><h2>Upcoming Courses</h2>";

	$out .= theme('table', array('rows' => array(), 'attributes' => array('id' => 'upcoming-courses-table')));

	$upcoming_rows = json_encode($upcoming_rows);

	$out .= "<br><br><h2>Completed Courses:</h2>";

	$out .= '
			<h3><select class="completed-year-select">
			<option value="">Select Year</option>
		';

	foreach($old_rows as $key => $value)	
		$out .= '<option value="'.$key.'">'.$key.' Courses</option>';

	$out .= '</select></h3>';

	$out .= theme('table', array('rows' => array(), 'attributes' => array('id' => 'completed-courses-table')));

	$old_rows = json_encode($old_rows);

	$button_access = '';
	if (user_access('export data'))
		if ( !($iPad || $iPhone || $iPod) )
			$button_access = '                        
        { extend: "excelHtml5", exportOptions: buttonCommon, title: "AT Schedule : '.$center_name.'" },
        { extend: "pdfHtml5", exportOptions: buttonCommon, pageSize: "A4", title: "AT Schedule : '.$center_name.'",
          customize: function (doc) {
           doc.content[1].table.widths =
             Array(doc.content[1].table.body[0].length + 1).join("*").split("");
            doc.content[1].table.dontBreakRows = true;
          },
        },

			';


	$js = '
		function format ( d ) {
		    return "<div class=\"slider\">"+
				"<div class=\"activity-log-"+d.ct_id+"\" style=\"margin-left : 50px; margin-bottom : 10px;\">Loading Activity Log...</div>"+
		    	"</div>";
		}

		(function ($) {
			function do_datatable( tid, data )
			{
				var table = $(tid).DataTable({
					dom: "lfr<\"clear\">tip", 
					data: data,
					"ordering": false,
					destroy: true,
					"oLanguage": {"sEmptyTable": "No Pending AT Application"},
			        columns: [
			        	{
			                "class":          "details-control",
			                "orderable":      false,
			                "data":           null,
			                "title":		  "Detail",
			                "defaultContent": ""
			            },
			            { data: "course_name_link", "title" : "Course-Name" },
			            { data: "teacher_formatted", "title" : "AT-Name" },
			            { data: "t_gender", "title" : "Gender" },
			            { data: "current_status", "title" : "Current-Status" },
			            { data: "change_status", "title" : "Change-Status" },
			        ],
			    });	


			    $("#recent-at-apps tbody").on("click", "td.details-control", function () {	

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

				    	$(".activity-log-"+row.data().ct_id).load("/at-app-activity/"+row.data().ct_id);	
            	
				    }
				});

				$("#recent-at-apps tbody").on("change", ".at-status-select", function() {

					var tr = $(this).closest("tr");

					status_to_be_updated = $(this).find(":selected").val();


					if (status_to_be_updated == "")
						return;

				    var row = table.row( tr );

				    id = row.data().ct_id;



				   $.confirm({
					    title: row.data().course_name + "<br><br>AT: " + row.data().teacher,
					    content:  "Change Application status to <b>" + status_to_be_updated + "</b>?"+
					    	"<form action=\"\" class=\"formName\">" +
						    "<div class=\"form-group\">" +
						    "<label>Optional Comments</label>"+
						    "<textarea name=\"comment\" id=\"txt-comment\"></textarea>"+
						    "</div>" +
						    "</form>",
					    buttons: {
					        Ok: {
					            text: "Ok",
					            btnClass: "btn-blue",
					            action: function () {

					            	const statuses = ["Confirmed", "Cancelled"];
					            	if (statuses.includes(status_to_be_updated))
					            	{
					            		comment = this.$content.find("#txt-comment").val();
					            		$.getJSON( "/at-schedule/change-status/"+id, { s: status_to_be_updated, comment: comment } )
									  .done(function( json ) {
									  	if (json.status == "OK")
									  	{
						  					row.remove().draw();
						  					alert(json.msg)
									  	}
									  	else
									  		alert("Update failed!\n"+json.msg)
									  })
									  .fail(function( jqxhr, textStatus, error ) {
									    var err = textStatus + ", " + error;
									    alert("Error updating "+err);
									});


					     
					            	}
					
					            }
					        },
					        Cancel: function () {
					            //close
					        },
					    },
					    onContentReady: function () {
					        // bind to events
					        var jc = this;
					        this.$content.find("form").on("submit", function (e) {
					            // if the user submits the form by pressing enter in the field.
					            e.preventDefault();
					            jc.$$formSubmit.trigger("click"); // reference the button and click it
					        });
					    }
					});




				});


			}


			function do_datatable_upcoming( tid, data )
			{
				var buttonCommon = {
                  columns: [0,1],
			            format: {
			            	header: function( data, column, node ) {
			            		var h = ["Course-Name", "Teachers", ];
			            		return h[column];
			            	},
			                body: function ( data, row, column, node ) {
			                	var tmp = table.rows().data();
			                	var a;
			                	if ( column == 0 )
			                	{
			                		return tmp[ row ].course_name_uformtd;
			                	}
			                	else if ( column == 1 )
			                	{
			                		return "AT Needed: "+tmp[ row ].at_required_uformtd+"\n"+tmp[ row ].teachers_uformtd;
			                	}
		
			                	return data;
			                }
			            }
			    };

				var table = $(tid).DataTable({
					dom: "lBfr<\"clear\">tip", 
					data: data,
					"ordering": false,
					destroy: true,
					"oLanguage": {"sEmptyTable": "No course for given criteria."},
			        columns: [
			            { data: "course_name", "title" : "Course-Name" },
			            { data: "at_required", "title" : "AT Needed" },
			            { data: "teachers", "title" : "Teachers" },
			        ],
			        buttons:   [  
						'.$button_access.'                        
			         ],
			    });	

			}


			function do_datatable_completed( tid, data )
			{
				var buttonCommon = {
                  columns: [0,1],
			            format: {
			            	header: function( data, column, node ) {
			            		var h = ["Course-Name", "Teachers", ]; 
			            		return h[column];
			            	},
			                body: function ( data, row, column, node ) {
			                	var tmp = table.rows().data();
			                	var a;
			                	if ( column == 0 )
			                	{
			                		return tmp[ row ].course_name_uformtd;
			                	}
			                	else if ( column == 1 )
			                	{
			                		return tmp[ row ].teachers_uformtd;
			                	}
		
			                	return data;
			                }
			            }
			    };

				var table = $(tid).DataTable({
					dom: "lBfr<\"clear\">tip", 
					data: data,
					"ordering": false,
					destroy: true,
					"oLanguage": {"sEmptyTable": "No course for given criteria."},
			        columns: [
			            { data: "course_name", "title" : "Course-Name" },
			            { data: "teachers", "title" : "Teachers" },
			        ],
			        buttons:   [  
						'.$button_access.'                        
			         ],
			    });	

			}



			$(document).ready(function(){
				do_datatable("#recent-at-apps", '.$recent_data.');
				do_datatable_upcoming("#upcoming-courses-table", '.$upcoming_rows.');

				var completed_courses = '.$old_rows.';

				$(".completed-year-select").change(function() {
					year = $(this).find(":selected").val();
					
					if (year == "")
						$("#completed-courses-table").empty();

					do_datatable_completed("#completed-courses-table", completed_courses[year]);	
				});

				$("body").on("click", ".get-at-info", function() {	
					teacher_id = $(this).attr("id");
					$.getJSON( "/at-schedule/get-at-info/"+teacher_id )
						.done(function( json ) {
							if (json.status == "OK")
							{
								$.alert(
									"<b><u>"+json.at_name+" ( "+json.at_year+" )</b></u><br><br>"+
									"<b>Status:</b> "+json.at_status+"<br>"+
									"<b>Mobile:</b> "+json.at_phone+"<br>"+
									"<b>Email:</b> "+json.at_email+"<br>"+
									"<b>Address:</b> "+json.at_address+"<br>"
								);
							}
							else
								$.alert("Not able to get AT Info")
						})
						.fail(function( jqxhr, textStatus, error ) {
						var err = textStatus + ", " + error;
						$.alert("Not able to get AT Info "+err);
						});

					return false;

				});

			});
		})(jQuery);
	';

	drupal_add_js($js, 'inline');


		

	$css = '
		#upcoming-courses-table td:nth-child(1) { width: 43% }
		#upcoming-courses-table td:nth-child(2) { width: 12% }
		#upcoming-courses-table td:nth-child(3) { width: 45% }

		#upcoming-courses-table .teacher-internal-table td:nth-child(1) { width: 40% }
		#upcoming-courses-table .teacher-internal-table td:nth-child(2) { width: 40% }
		#upcoming-courses-table .teacher-internal-table td:nth-child(3) { width: 20% }

		#completed-courses-table td:nth-child(1) { width: 50% }
		#completed-courses-table td:nth-child(2) { width: 50% }

		#completed-courses-table .teacher-internal-table td:nth-child(1) { width: 50% }
		#completed-courses-table .teacher-internal-table td:nth-child(2) { width: 50% }
		
	';

	drupal_add_css($css, 'inline');


	return $out;
}


function dh_assign_teacher()
{
	$centre = arg(1);
	$course = arg(2);
	$row = db_query("select c_name, c_finalized, c_at_m_count, c_at_f_count from dh_course where c_id=".$course)->fetchAssoc();
	if ( $row['c_finalized'] )
	{
		drupal_set_message("Course has been Finalized, cannot access Assign Teacher", "error");
		drupal_goto("course/".$centre."/".$course);
	}

	drupal_add_css(drupal_get_path( 'module', 'dh_manageapp').'/css/manageapp.css');
	drupal_add_css( libraries_get_path("editor"). "/DataTables/datatables.min.css");
	drupal_add_js( libraries_get_path("editor").  "/DataTables/datatables.min.js");
	drupal_add_css(drupal_get_path('module', 'dh_manageapp').'/css/select2.min.css');
	drupal_add_js(drupal_get_path('module', 'dh_manageapp').'/js/select2.min.js');
	drupal_add_css( drupal_get_path('module', 'dh_manageapp'). "/css/jquery-confirm.min.css");
	drupal_add_css( drupal_get_path('module', 'dh_manageapp'). "/css/jquery-confirm-responsive.css");
	drupal_add_js( drupal_get_path('module', 'dh_manageapp'). "/js/jquery-confirm.min.js");
	drupal_add_css(drupal_get_path('module', 'dh_manageapp').'/css/tables.css' );


	$out = '<h2>'.l("Back to Course", "course/$centre/$course")."&nbsp;&nbsp;";
	if ( strpos($_SERVER['HTTP_REFERER'], "at-schedule") )
		$out .= " | &nbsp;&nbsp; ".l("Back to AT Schedule", "at-schedule/$centre");
	else
		$out .=  " | &nbsp;&nbsp; ".l("Back to Zero Day", "zero-day/$centre/$course");
	$out .= '</h2>';

	$out .= '<br><h2>'.$row['c_name'].'</h2>';
	$out .= '<h3><i>Male Teacher Required: '.$row['c_at_m_count'].' | Female Teacher Required: '.$row['c_at_f_count'].'</i></h3>';
	
	$out .= "<br><br><h2>Teacher Applications</h2>";
	$temp = drupal_get_form('dh_assign_teacher_form');
	$out .= drupal_render( $temp )."<br>";


	// Teacher applications: Confirmed, then Received, then Cancelled; within each,
	// male teachers before female; then oldest application first.
	$q = "select
		t_id,
		CONCAT(t_f_name, ' ', t_l_name) as 'teacher',
		t_gender,
		ct_id,
		ct_updated ,
		ct_status,
		ct_type,
		ct_group,
		ct_sees
		from
		dh_course_teacher
		left join dh_course on ct_course=c_id
		left join dh_teacher on ct_teacher=t_id
		where
		ct_type<>'Training' and
		c_id=$course
		order by
		case ct_status when 'Confirmed' then 1 when 'Received' then 2 when 'Cancelled' then 3 else 4 end,
		case t_gender when 'M' then 1 when 'F' then 2 else 3 end,
		ct_created
		asc";

	$result = db_query($q);
	$data = array();
	while($r = $result->fetchAssoc())
	{	

		$change_status = '
			<select class="at-status-select">
			<option value="">Choose</option>
			<option value="Confirmed">Confirmed</option>
			<option value="Cancelled">Cancelled</option>
			</select>
		';

		$change_type = '
			<select class="at-type-select">
			<option value="">Choose</option>
			<option value="Conducting">Conducting</option>
			<option value="Assisting">Assisting</option>
			</select>
		';

		$change_group = '
			<select class="at-group-select">
			<option value="">Choose</option>
			<option value="1">1</option>
			<option value="2">2</option>
			<option value="3">3</option>
			<option value="4">4</option>
			<option value="5">5</option>
			<option value="6">6</option>
			<option value="7">7</option>
			<option value="8">8</option>
			<option value="9">9</option>
			</select>
		';


		// Which side of the course this AT sees in the AT course view (default: own gender).
		$change_sees = '
			<select class="at-sees-select">
			<option value="">Choose</option>
			<option value="M">Male</option>
			<option value="F">Female</option>
			<option value="B">Both</option>
			</select>
		';

		if ($r['ct_status'] == "Confirmed")
			$current_status = '<span style="color:green;">'.$r['ct_status'].'</span>';
		elseif ($r['ct_status'] == "Cancelled")
			$current_status = '<span style="color:red;">'.$r['ct_status'].'</span>';
		else
			$current_status = $r['ct_status'];

		$data[] = array(
			'ct_id' => $r['ct_id'], 
			'teacher_formatted' => '<a href="#" id="'.$r['t_id'].'" class="get-at-info" >'.$r['teacher'].'</a>',
			't_gender' => $r['t_gender'], 
			'ct_updated' =>$r['ct_updated'],
			'change_status' =>$change_status,
			'teacher' => $r['teacher'],
			'current_status' => $current_status,
			'group' => $r['ct_group'],
			'type' => $r['ct_type'],
			'change_type' => $change_type,
			'change_group' => $change_group,
			'sees' => dh_teacher_sees_label($r['ct_sees'], $r['t_gender'], TRUE),
			'change_sees' => $change_sees,
		);

	}

	$data = json_encode($data);

	$out .= theme('table', array('rows' => array(), 'attributes' => array('id' => 'course-at-apps')));

	$out .= "<br><br><h2>Trainee Teachers</h2>";

	$temp1 = drupal_get_form('dh_assign_trainee_teacher_form');
	$out .= drupal_render( $temp1 );

	$q = "select
		t_id,
		CONCAT(t_f_name, ' ', t_l_name) as 'teacher',
		t_gender,
		ct_group,
		ct_id
		from
		dh_course_teacher
		left join dh_course on ct_course=c_id
		left join dh_teacher on ct_teacher=t_id
		where
		ct_type='Training' and
		c_id=$course
		order by
		ct_created
		asc";

	$res = db_query($q);

	$rows = array();

	while($r = $res->fetchAssoc())
	{
		$r['teacher'] = '<a href="#" id="'.$r['t_id'].'" class="get-at-info" >'.$r['teacher'].'</a>'; 
		$r['ct_id'] = l("Delete", "at-schedule/del-trainee-teacher/$centre/$course/{$r['ct_id']}", array('attributes' => array('class' => array('delete-confirm')))) ;  
		unset($r['t_id']);
		$rows[] = $r;
	}

	$header = array("Trainee Teacher", "Gender", "Group", "Delete");
	$out .= "<br>".theme("table", array('header' => $header, 'rows' => $rows ));


	drupal_add_js('
		function format ( d ) {
		    return "<div class=\"slider\">"+
				"<div class=\"activity-log-"+d.ct_id+"\" style=\"margin-left : 50px; margin-bottom : 10px;\">Loading Activity Log...</div>"+
		    	"</div>";
		}

		(function ($) {
			function do_datatable( tid, data )
			{
				var table = $(tid).DataTable({
					dom: "lfr<\"clear\">tip", 
					data: data,
					"ordering": false,
					destroy: true,
					"oLanguage": {"sEmptyTable": "No AT Applications for the Course."},
					searching: false,
					"columnDefs": [
					            {
					                "targets": [ 0 ],
					                "searchable": false
					            }
			        ],					
			        columns: [
			        	{
			                "class":          "details-control",
			                "orderable":      false,
			                "data":           null,
			                "title":		  "Detail",
			                "defaultContent": ""
			            },
			            { data: "teacher_formatted", "title" : "AT-Name" },
			            { data: "t_gender", "title" : "Gender" },
			            { data: "current_status", "title" : "Status" },
			            { data: "type", "title" : "Type" },
			            { data: "group", "title" : "Group" },
			            { data: "sees", "title" : "Sees" },
			            { data: "change_status", "title" : "Change-Status" },
			            { data: "change_type", "title" : "Change-Type" },
			            { data: "change_group", "title" : "Change-Group" },
			            { data: "change_sees", "title" : "Change-Sees" },
			        ],
			    });	


			    $("#course-at-apps tbody").on("click", "td.details-control", function () {	

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

				    	$(".activity-log-"+row.data().ct_id).load("/at-app-activity/"+row.data().ct_id);	
            	
				    }
				});

				$("#course-at-apps tbody").on("change", ".at-status-select", function() {

					var tr = $(this).closest("tr");

					status_to_be_updated = $(this).find(":selected").val();


					if (status_to_be_updated == "")
						return;

				    var row = table.row( tr );

				    id = row.data().ct_id;



				   $.confirm({
					    title: "AT: " + row.data().teacher,
					    content: "Change Application status to <b>" + status_to_be_updated + "</b>?"+
					    	"<form action=\"\" class=\"formName\">" +
						    "<div class=\"form-group\">" +
						    "<label>Optional Comments</label>"+
						    "<textarea name=\"comment\" id=\"txt-comment\"></textarea>"+
						    "</div>" +
						    "</form>",
					    buttons: {
					        Ok: {
					            text: "Ok",
					            btnClass: "btn-blue",
					            action: function () {

					            	const statuses = ["Confirmed", "Cancelled"];
					            	if (statuses.includes(status_to_be_updated))
					            	{
					            		comment = this.$content.find("#txt-comment").val();
					            		$.getJSON( "/at-schedule/change-status/"+id, { s: status_to_be_updated, comment: comment } )
									  .done(function( json ) {
									  	if (json.status == "OK")
									  	{
									  		row_data = row.data();

									  		current_status = status_to_be_updated;
									  		if (status_to_be_updated == "Confirmed")
												current_status = "<span style=\"color:green;\">"+status_to_be_updated+"</span>";
											if (status_to_be_updated == "Cancelled")
												current_status = "<span style=\"color:red;\">"+status_to_be_updated+"</span>";
											
												
									  		row_data.current_status = current_status;
									  		row.data(row_data);
						  					alert(json.msg)
									  	}
									  	else
									  		alert("Update failed!\n"+json.msg)
									  })
									  .fail(function( jqxhr, textStatus, error ) {
									    var err = textStatus + ", " + error;
									    alert("Error updating "+err);
									});


					     
					            	}
					
					            }
					        },
					        Cancel: function () {
					            //close
					        },
					    },
					    onContentReady: function () {
					        // bind to events
					        var jc = this;
					        this.$content.find("form").on("submit", function (e) {
					            // if the user submits the form by pressing enter in the field.
					            e.preventDefault();
					            jc.$$formSubmit.trigger("click"); // reference the button and click it
					        });
					    }
					});

				});



				$("#course-at-apps tbody").on("change", ".at-type-select", function() {

					var tr = $(this).closest("tr");

					type_to_be_updated = $(this).find(":selected").val();


					if (type_to_be_updated == "")
						return;

				    var row = table.row( tr );

				    id = row.data().ct_id;



				   $.confirm({
					    title: "AT: " + row.data().teacher,
					    content: "Change AT Type to <b>" + type_to_be_updated + "</b>?",
					    buttons: {
					        Ok: {
					            text: "Ok",
					            btnClass: "btn-blue",
					            action: function () {

					            	const types = ["Conducting", "Assisting", "Training"];
					            	if (types.includes(type_to_be_updated))
					            	{
					            		$.getJSON( "/at-schedule/change-type/"+id, { s: type_to_be_updated } )
									  .done(function( json ) {
									  	if (json.status == "OK")
									  	{
									  		row_data = row.data();
									  		row_data.type = type_to_be_updated;
									  		row.data(row_data);
						  					alert(json.msg)
									  	}
									  	else
									  		alert("Update failed!\n"+json.msg)
									  })
									  .fail(function( jqxhr, textStatus, error ) {
									    var err = textStatus + ", " + error;
									    alert("Error updating "+err);
									});


					     
					            	}
					
					            }
					        },
					        Cancel: function () {
					            //close
					        },
					    },
					    onContentReady: function () {
					        // bind to events
					        var jc = this;
					        this.$content.find("form").on("submit", function (e) {
					            // if the user submits the form by pressing enter in the field.
					            e.preventDefault();
					            jc.$$formSubmit.trigger("click"); // reference the button and click it
					        });
					    }
					});

				});





				$("#course-at-apps tbody").on("change", ".at-group-select", function() {

					var tr = $(this).closest("tr");

					group_to_be_updated = $(this).find(":selected").val();


					if (group_to_be_updated == "")
						return;

				    var row = table.row( tr );

				    id = row.data().ct_id;



				   $.confirm({
					    title: "AT: " + row.data().teacher,
					    content: "Change AT Group to <b>" + group_to_be_updated + "</b>?",
					    buttons: {
					        Ok: {
					            text: "Ok",
					            btnClass: "btn-blue",
					            action: function () {

					            	const groups = ["1", "2", "3", "4", "5", "6", "7", "8", "9"];
					            	if (groups.includes(group_to_be_updated))
					            	{
					            		$.getJSON( "/at-schedule/change-group/"+id, { s: group_to_be_updated } )
									  .done(function( json ) {
									  	if (json.status == "OK")
									  	{
									  		row_data = row.data();
									  		row_data.group = group_to_be_updated;
									  		row.data(row_data);
						  					alert(json.msg)
									  	}
									  	else
									  		alert("Update failed!\n"+json.msg)
									  })
									  .fail(function( jqxhr, textStatus, error ) {
									    var err = textStatus + ", " + error;
									    alert("Error updating "+err);
									});


					     
					            	}
					
					            }
					        },
					        Cancel: function () {
					            //close
					        },
					    },
					    onContentReady: function () {
					        // bind to events
					        var jc = this;
					        this.$content.find("form").on("submit", function (e) {
					            // if the user submits the form by pressing enter in the field.
					            e.preventDefault();
					            jc.$$formSubmit.trigger("click"); // reference the button and click it
					        });
					    }
					});

				});

				$("#course-at-apps tbody").on("change", ".at-sees-select", function() {

					var tr = $(this).closest("tr");

					sees_to_be_updated = $(this).find(":selected").val();


					if (sees_to_be_updated == "")
						return;

				    var row = table.row( tr );

				    id = row.data().ct_id;



				   $.confirm({
					    title: "AT: " + row.data().teacher,
					    content: "This AT will see <b>" + {M: "Male", F: "Female", B: "Both"}[sees_to_be_updated] + "</b> applicants in the AT course view. Save?",
					    buttons: {
					        Ok: {
					            text: "Ok",
					            btnClass: "btn-blue",
					            action: function () {

					            	const choices = ["M", "F", "B"];
					            	if (choices.includes(sees_to_be_updated))
					            	{
					            		$.getJSON( "/at-schedule/change-sees/"+id, { s: sees_to_be_updated } )
									  .done(function( json ) {
									  	if (json.status == "OK")
									  	{
									  		row_data = row.data();
									  		row_data.sees = json.label;
									  		row.data(row_data);
						  					alert(json.msg)
									  	}
									  	else
									  		alert("Update failed!\n"+json.msg)
									  })
									  .fail(function( jqxhr, textStatus, error ) {
									    var err = textStatus + ", " + error;
									    alert("Error updating "+err);
									});


					     
					            	}
					
					            }
					        },
					        Cancel: function () {
					            //close
					        },
					    },
					    onContentReady: function () {
					        // bind to events
					        var jc = this;
					        this.$content.find("form").on("submit", function (e) {
					            // if the user submits the form by pressing enter in the field.
					            e.preventDefault();
					            jc.$$formSubmit.trigger("click"); // reference the button and click it
					        });
					    }
					});

				});



			}


			$(document).ready(function(){
				do_datatable("#course-at-apps", '.$data.');
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

		  		$("#edit-ct-trainee-teacher").autocomplete({
					source: function( request, response ) {
						$("#edit-ct-trainee-teacher").addClass( "throbbing" );
				        $.ajax({
				          url: "/autocomplete/get-trainee-teacher/" + request.term,
				          dataType: "json",
				          success: function( data ) {
				          	$("#edit-ct-trainee-teacher").removeClass( "throbbing" );
				            response( data );
				          }
				        });
					},
		  			minLength: 1,
		  			select: function( event, ui ) {
		        		$( "#edit-ct-trainee-teacher" ).val( ui.item.code );
		        		return false;
		      		}
		  		});



		  		$("body").on("click", ".get-at-info", function() {	
					teacher_id = $(this).attr("id");
					$.getJSON( "/at-schedule/get-at-info/"+teacher_id )
						.done(function( json ) {
							if (json.status == "OK")
							{
								$.alert(
									"<b><u>"+json.at_name+" ( "+json.at_year+" )</b></u><br><br>"+
									"<b>Status:</b> "+json.at_status+"<br>"+
									"<b>Mobile:</b> "+json.at_phone+"<br>"+
									"<b>Email:</b> "+json.at_email+"<br>"+
									"<b>Address:</b> "+json.at_address+"<br>"
								);
							}
							else
								$.alert("Not able to get AT Info")
						})
						.fail(function( jqxhr, textStatus, error ) {
						var err = textStatus + ", " + error;
						$.alert("Not able to get AT Info "+err);
						});

					return false;

				});
			
			})
		})(jQuery);


		', 'inline');


	$css = '
		.form-item-ct-trainee-teacher { float: left; margin-right: 12px; }
		.form-item-ct-trainee-group { float: left; margin-right: 12px; }
		#edit-ct-trainee-sub {
			color: #fff;
		    background-color: #337ab7;
		    border-color: #2e6da4;
		    margin-bottom: 12px;
		}
	';

	drupal_add_css($css, 'inline');



	return $out;
}


function dh_assign_teacher_form($form, &$form_state)
{
	$form['ct_teacher'] = array( '#title' => 'Teacher',  '#type' => 'textfield', 
		'#weight' => 1, '#size' => 50, '#required' => 1, 
    '#attributes' => array(
        'placeholder' => t('Start typing AT Name for suggestions, then select the AT.'),
      ),
 		'#attached' => array(
      		'library' => array(
        		array('system', 'ui.autocomplete'),
      	)),
	 ) ;
	$types = array('Conducting' => 'Conducting', 'Assisting' => 'Assisting' );
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

	if (count($temp) != 2 )
	{
		drupal_set_message("Selected teacher is not valid", 'error');
		drupal_goto("assign-teacher/".arg(1)."/".arg(2));
		exit;
	}

	$tid = db_query("select t_id from dh_teacher where t_code='".$temp[0]."' and t_gender='".$temp[1]."'")->fetchField();

	if (!$tid)
	{
		drupal_set_message("Selected teacher is not valid", 'error');
		drupal_goto("assign-teacher/".arg(1)."/".arg(2));
		exit;
	}

	$test = db_query("select count(*) from dh_course_teacher where ct_course=".arg(2)." and ct_teacher=".$tid)->fetchField();

	if ($test > 0)
	{
		drupal_set_message("Teacher is already present", 'error');
		drupal_goto("assign-teacher/".arg(1)."/".arg(2));
		exit;
	}


	$required = check_at_requirement(arg(2), $temp[1]);


	if ($required)
	{
		$teacher['ct_teacher'] = $tid;
		$teacher['ct_type'] = $input['ct_type'];
		$teacher['ct_group'] = $input['ct_group'];
		$teacher['ct_created_by'] = $user->uid;
		$teacher['ct_updated_by'] = $user->uid;
		$teacher['ct_updated'] = date("Y-m-d H:i:s");
		$teacher['ct_course'] = arg(2);
		$teacher['ct_status'] = 'Confirmed';

		$course_teacher_id = db_insert('dh_course_teacher')->fields($teacher)->execute();

		if($course_teacher_id)
		{
			log_course_teacher($course_teacher_id, "Center added AT Application as Confirmed");
			update_course_current_at_count( arg(2) );
			drupal_set_message("Teacher added successfully");
			send_at_schedule_email($course_teacher_id);
		}
		else
		{
			drupal_set_message("Not able to add Teacher");
		}
		
	}
	else
		drupal_set_message("Can not add Teacher<br>".($temp[1] == 'M' ? 'Male' : 'Female')." AT requirement is fulfilled, to change the same please check course settings in Manage Courses", 'error');

	
}


function dh_assign_trainee_teacher_form($form, &$form_state)
{
	$form['ct_trainee_teacher'] = array( '#title' => 'Trainee Teacher',  '#type' => 'textfield', 
		'#weight' => 1, '#size' => 50, '#required' => 1, 
    '#attributes' => array(
        'placeholder' => t('Start typing Trainee Teacher Name for suggestions, then select the name.'),
      ),
 		'#attached' => array(
      		'library' => array(
        		array('system', 'ui.autocomplete'),
      	)),
	 ) ;
	$groups = array();
	for($i=1; $i<10; $i++)
		$groups[$i] = $i;
	$form['ct_trainee_group'] = array( '#title' => 'Group', '#type' => 'select', '#options' => $groups, '#weight' => 3);
	$form['ct_trainee_sub'] = array('#type' => 'submit', '#value' => 'Assign Trainee Teacher', '#weight' => 4);
	return $form;
}

function dh_assign_trainee_teacher_form_submit($form, &$form_state)
{
	global $user;
	$input = $form_state['input'];
	$temp = explode("-", $input['ct_trainee_teacher']);

	if (count($temp) != 2 )
	{
		drupal_set_message("Selected Trainee Teacher is not valid", 'error');
		drupal_goto("assign-teacher/".arg(1)."/".arg(2));
		exit;
	}

	$tid = db_query("select t_id from dh_teacher where t_status='Training' and t_code='".$temp[0]."' and t_gender='".$temp[1]."'")->fetchField();

	if (!$tid)
	{
		drupal_set_message("Selected Trainee Teacher is not valid", 'error');
		drupal_goto("assign-teacher/".arg(1)."/".arg(2));
		exit;
	}

	$test = db_query("select count(*) from dh_course_teacher where ct_course=".arg(2)." and ct_teacher=".$tid)->fetchField();

	if ($test > 0)
	{
		drupal_set_message("Trainee Teacher is already present", 'error');
		drupal_goto("assign-teacher/".arg(1)."/".arg(2));
		exit;
	}


	$teacher['ct_teacher'] = $tid;
	$teacher['ct_type'] = 'Training';
	$teacher['ct_group'] = $input['ct_trainee_group'];
	$teacher['ct_created_by'] = $user->uid;
	$teacher['ct_updated_by'] = $user->uid;
	$teacher['ct_updated'] = date("Y-m-d H:i:s");
	$teacher['ct_course'] = arg(2);
	$teacher['ct_status'] = 'Confirmed';

	$course_teacher_id = db_insert('dh_course_teacher')->fields($teacher)->execute();

	if($course_teacher_id)
	{
		log_course_teacher($course_teacher_id, "Center added Trainee as Confirmed");
		drupal_set_message("Trainee Teacher added successfully");
	}
	else
	{
		drupal_set_message("Not able to add Trainee Teacher");
	}


	
}




/**
 * "Male", "Female" or "Both": the side of a course a mapped AT sees in the AT course view
 * (dh_course_teacher.ct_sees; empty = the teacher's own gender, the default). With $html,
 * the default is marked "(default)" for the Assign Teachers table.
 */
function dh_teacher_sees_label($sees, $gender, $html = FALSE)
{
	$s = $sees ? $sees : $gender;
	$label = ($s == 'B') ? 'Both' : (($s == 'M') ? 'Male' : (($s == 'F') ? 'Female' : ''));
	if ($html && !$sees)
		$label .= ' <span style="color:#888;">(default)</span>';
	return $label;
}

function at_app_change()
{
	$course_teacher_id = arg(2);

	$out = array();
	$out['status'] = 'OK';
	$out['msg'] ="";

	$q = "Select t_gender, c_at_m_count, c_at_f_count, c_at_m_conf, c_at_f_conf, ct.ct_course as 'course_id', ce.c_id as 'center_id' from dh_course_teacher ct left join dh_course co on ct.ct_course=co.c_id left join dh_center ce on co.c_center=ce.c_id left join dh_teacher on t_id=ct_teacher where ct.ct_id=:ct_id";
	$res = db_query($q, array( ':ct_id' => $course_teacher_id ))->fetchAssoc();

	if ( !$res )
	{
		$out['status'] = 'Failed';
		$out['msg'] ="Teacher Application Not Found";
		drupal_json_output($out);
		exit;
	}

	if (arg(1) == "change-status")
	{
		$status = $_REQUEST['s'];
		$comment = $_REQUEST['comment'];

		if ( !in_array($status, array("Confirmed", "Cancelled")) )
		{
			$out['status'] = 'Failed';
			$out['msg'] ="Invalid Status";
			drupal_json_output($out);
			exit;
		}

		if ($res['t_gender'] == 'M')
		{
			$at_need = $res['c_at_m_count'];
			$at_conf = $res['c_at_m_conf'];
			$at_gender = 'Male';
		}
		else
		{
			$at_need = $res['c_at_f_count'];
			$at_conf = $res['c_at_f_conf'];
			$at_gender = 'Female';
		}

		$required = check_at_requirement($res['course_id'], $res['t_gender']);

		if ($status == "Confirmed" && !$required )
		{
			$out['status'] = 'Failed';
			$out['msg'] =($res['t_gender'] == 'M' ? 'Male' : 'Female')." AT requirement is fulfilled";
			drupal_json_output($out);
			exit;
		}

		$updated = db_update('dh_course_teacher')
			->fields( array('ct_status' => $status) )
			->condition('ct_id', $course_teacher_id, '=')
			->execute();

		if ($updated)
		{
			log_course_teacher($course_teacher_id, 'Status updated to '.$status.'<br>Comments: '.$comment);	
			update_course_current_at_count( $res['course_id'] );
			$out['msg'] ="AT Application status updated to $status.";
			send_at_schedule_email($course_teacher_id);
		}
		else
		{
			$out['status'] = 'Failed';
			$out['msg'] ="Some issue in updating AT Status";
			drupal_json_output($out);
			exit;
		}
	}

	if (arg(1) == "change-type")
	{
		$type = $_REQUEST['s'];

		if ( !in_array($type, array("Conducting", "Assisting")) )
		{
			$out['status'] = 'Failed';
			$out['msg'] ="Invalid type";
			drupal_json_output($out);
			exit;
		}

		

		$old_type = db_query("select ct_type from dh_course_teacher where ct_id=:id", array(':id' => $course_teacher_id))->fetchField();
		db_update('dh_course_teacher')
			->fields( array('ct_type' => $type) )
			->condition('ct_id', $course_teacher_id, '=')
			->execute();
		if ($old_type !== $type)
			log_course_teacher($course_teacher_id, "Type changed from $old_type to $type");


		$out['msg'] ="AT Application type updated to $type.";
	}
	
	if (arg(1) == "change-group")
	{
		$group = $_REQUEST['s'];

		if ( !in_array($group, array("1", "2", "3", "4", "5", "6", "7", "8", "9", )) )
		{
			$out['status'] = 'Failed';
			$out['msg'] ="Invalid Group";
			drupal_json_output($out);
			exit;
		}

		

		$old_group = db_query("select ct_group from dh_course_teacher where ct_id=:id", array(':id' => $course_teacher_id))->fetchField();
		db_update('dh_course_teacher')
			->fields( array('ct_group' => $group) )
			->condition('ct_id', $course_teacher_id, '=')
			->execute();
		if ((string) $old_group !== (string) $group)
			log_course_teacher($course_teacher_id, "Group changed from $old_group to $group");


		$out['msg'] ="AT Application Group updated to $group.";
	}

	if (arg(1) == "change-sees")
	{
		$sees = $_REQUEST['s'];

		if ( !in_array($sees, array("M", "F", "B")) )
		{
			$out['status'] = 'Failed';
			$out['msg'] ="Invalid choice";
			drupal_json_output($out);
			exit;
		}

		$old = db_query("select ct_sees, t_gender from dh_course_teacher left join dh_teacher on ct_teacher=t_id where ct_id=:id", array(':id' => $course_teacher_id))->fetchAssoc();
		// The teacher's own gender is the default, kept as NULL.
		$new = ($sees == $old['t_gender']) ? NULL : $sees;
		db_update('dh_course_teacher')
			->fields( array('ct_sees' => $new) )
			->condition('ct_id', $course_teacher_id, '=')
			->execute();
		$from = dh_teacher_sees_label($old['ct_sees'], $old['t_gender']);
		$to = dh_teacher_sees_label($new, $old['t_gender']);
		if ($from !== $to)
			log_course_teacher($course_teacher_id, "Sees changed from $from to $to");

		$out['msg'] = "This AT will now see $to applicants.";
		$out['label'] = dh_teacher_sees_label($new, $old['t_gender'], TRUE);
	}

	drupal_json_output($out);
	exit;
}


function get_at_info ()
{

	$at_id = arg(2);

	$out = array();
	$out['status'] = 'OK';
	$out['msg'] = "";
	$out['at_name'] ="";
	$out['at_year'] ="";
	$out['at_phone'] ="";
	$out['at_email'] ="";
	$out['at_address'] ="";

	if (!is_numeric($at_id))
	{
		$out['status'] = 'Failed';
		$out['msg'] ="AT Not Found";
		drupal_json_output($out);
		exit;
	}

	$r = db_query("select CONCAT(t_f_name, ' ', t_l_name) as 'teacher', t_address, t_sat, t_full_t, t_year_appointed, t_mob_phone, t_email, t_status from dh_teacher where t_id=$at_id")->fetchAssoc();

	if (!$r)
	{
		$out['status'] = 'Failed';
		$out['msg'] ="AT Not Found";
		drupal_json_output($out);
		exit;
	}

	$out['at_name'] = $r['teacher'];

	if ($r['t_full_t'])
		$out['at_year'] = "T-".$r['t_year_appointed'];
	elseif ($r['t_sat'])
		$out['at_year'] = "SAT-".$r['t_year_appointed'];
	else
		$out['at_year'] = "AT-".$r['t_year_appointed'];

	$out['at_phone'] = $r['t_mob_phone'];
	$out['at_email'] = $r['t_email'];
	$out['at_address'] = $r['t_address'];
	$out['at_status'] = $r['t_status'];
	
	drupal_json_output($out);
	exit;
} 



function del_trainee_teacher()
{
	global $user;
	$center = arg(2);
	$course = arg(3);
	$at_app_id = arg(4);

	$check = db_query("select ct_id from dh_course_teacher where ct_id=$at_app_id and ct_type='Training'")->fetchField();

	if ($check)
	{
		// Log first: the row is deleted, so its course and teacher cannot be read afterwards.
		// Logged under the course id (module Course-Teacher-Del) so the course Change Log
		// can find it; the assignment and teacher ids in the text link it to the earlier lines.
		$tr = db_query("select ct.ct_course, ct.ct_teacher, co.c_center, concat(t.t_f_name, ' ', t.t_l_name) as name from dh_course_teacher ct left join dh_course co on co.c_id = ct.ct_course left join dh_teacher t on t.t_id = ct.ct_teacher where ct.ct_id = :id", array(':id' => $at_app_id))->fetchAssoc();
		db_insert('dh_log')->fields(array('l_center' => (int) $tr['c_center'], 'l_module' => 'Course-Teacher-Del', 'l_identifier' => (int) $tr['ct_course'],
			'l_event' => 'Delete', 'l_msg' => "Trainee removed: ".check_plain($tr['name'])." (assignment ".(int) $at_app_id.", teacher ".(int) $tr['ct_teacher'].")",
			'l_tstamp' => date('Y-m-d H:i:s'), 'l_user' => $user->uid))->execute();
		$r = db_query("delete from dh_course_teacher where ct_id=$at_app_id and ct_type='Training'");

		if ($r)
			drupal_set_message("Trainee Teacher deleted successfully.");
		else
			drupal_set_message("Not able to delete Trainee Teacher");

	}
	else
		drupal_set_message("Not able to delete Trainee Teacher");
	

	drupal_goto('assign-teacher/'.arg(2).'/'.arg(3));
}
