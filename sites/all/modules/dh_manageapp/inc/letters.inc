<?php

require_once 'vendor/autoload.php';
use Mailgun\Mailgun;

define('DELIVERY_TYPE_EMAIL', 1);
define('DELIVERY_TYPE_SMS', 2);
define('DELIVERY_TYPE_BOTH', 3);



function dh_manage_letters()
{
	switch( arg(2) )
	{
		case 'add':
			return drupal_get_form('dh_letters_form');
			break;
		case 'edit':
			$row = db_query("select * from dh_letter where l_id='".arg(3)."'")->fetchAssoc();
			return drupal_get_form('dh_letters_form', $row);
			break;
    case 'copy':
      $row = db_query("select * from dh_letter where l_id='".arg(3)."'")->fetchAssoc();
      return drupal_get_form('dh_letters_form', $row);
      break;
		case 'delete':
			db_update("dh_letter")->fields(array('l_deleted' => 1))->condition('l_id', arg(3))->execute();
			drupal_set_message("Letter deleted successfully");
			drupal_goto("letters/".arg(1));
		case 'delattach':
			db_update("dh_letter")->fields(array('l_attachment' => null))->condition('l_id', arg(3))->execute();
			drupal_set_message("Attachment deleted successfully");
			drupal_goto("letters/".arg(1)."/edit/".arg(3));
		default:
			return dh_letters_listing();
			break;
	}

}

function dh_letters_listing()
{
	$centre_id = arg(1);
	drupal_add_css( libraries_get_path("editor"). "/DataTables/datatables.min.css");
	drupal_add_js( libraries_get_path("editor").  "/DataTables/datatables.min.js");

	$q = "select l_id, l_name as 'LetterName', l_event as 'Status', tdc.td_val1 as 'Course Type', l_subject as 'Subject' from dh_letter left join dh_type_detail tdc on l_course_type=tdc.td_id where l_center='$centre_id' and l_deleted=0 order by l_event , tdc.td_val1";
//and a_status='Confirmed' 
	$result = db_query($q);
	$rows = array();
	while( $r = $result->fetchAssoc() )
	{
		unset($rs);

		$rs[] = $r['LetterName'];
		$rs[] = $r['Status'];
		$rs[] = $r['Course Type']?$r['Course Type']:"All";
		$rs[] = $r['Subject'];
		$rs[] = l("Edit", "letters/$centre_id/edit/".$r['l_id'] );
		$rs[] = l("Copy", "letters/$centre_id/copy/".$r['l_id'] );
		$rs[] = l("Delete", "letters/$centre_id/delete/".$r['l_id'], array("attributes" => array("class" => array("delete-letter") ,"txt" => $r['LetterName'])));
		$rows[] = array_values($rs);
	}
	$header = array('Letter Name', 'Status', 'Course Type', 'Subject', 'Action', 'Action', 'Action');
	$attributes = array('id' => 'table-letters');
	$out = '<h2>'.l("Add Letter", "letters/$centre_id/add").' &nbsp;&nbsp;|&nbsp;&nbsp;'.l("Manage Fields", "letter-fields/$centre_id").' &nbsp;&nbsp;|&nbsp;&nbsp;'.l("Back to Dashboard", "centre/".$centre_id).'</h2>';
	$out .= theme('table', array('header' => $header, 'rows' => $rows, 'attributes' => $attributes));
	//$out .= '<div id="replace-me"></div>';
	$js = '
		(function ($) {
			$(document).ready(function(){
				$("#table-letters").DataTable();
				$("#table-letters tbody").on("click", ".delete-letter", function () {
					var lname = $(this).attr("txt");
					var r = confirm("Are you sure you want to delete the letter "+lname+"?");
					if ( r == false )
						return false;
					else
						return true;
				});
			});

		})(jQuery);

	';
	drupal_add_js($js, 'inline');
	drupal_add_css(drupal_get_path('module', 'dh_manageapp').'/css/manageapp.css');

	return $out;

}

function dh_letters_form($form, &$form_state, $edit_data = array())
{
	$out = '<h2>'.l("Back to Manage Letters", "letters/".arg(1)).'</h2>';
	drupal_add_css(drupal_get_path('module', 'dh_manageapp').'/css/manageapp.css');
	$add = 0;
	if (count($edit_data) == 0)
		$add = 1;

	$action = arg(2);

	$merge_fields = _get_letter_fields(arg(1));
	$field_count = 1;
	$merge_block = '<table class="table-merge-fields"><tr><td colspan='.$field_count.'>';
	$i = 0;
	foreach($merge_fields as $v)
	{
		if ( ($i % $field_count) == 0 )
			$merge_block .= '</tr><tr>';
		//$merge_block .= '<td><a class="merge-field-link" href="#">'.$v.'</a></td>';
    $merge_block .= '<td>'.$v.'</td>';
		$i++;
	}
	$merge_block .= '</tr></table>';
	$q = "select td_key, td_val1 from dh_type_detail where td_type='COURSE-SYSTEM-STATUS'";
	$system_statuses = db_query($q)->fetchAllKeyed();


	$q = "select td_key, td_key from dh_type_detail where td_type='COURSE-STATUS'";
	$statuses = db_query($q)->fetchAllKeyed();
	$statuses[''] = 'Choose';
	$statuses[$system_statuses['STATUS-CONFIRMED']] = $system_statuses['STATUS-CONFIRMED'];
	$statuses[$system_statuses['STATUS-CANCELLED']] = $system_statuses['STATUS-CANCELLED'];
	$statuses[$system_statuses['STATUS-RECONFIRMATION']] = $system_statuses['STATUS-RECONFIRMATION'];
	$statuses[$system_statuses['STATUS-RECEIVED']] = $system_statuses['STATUS-RECEIVED'];
	$statuses[$system_statuses['STATUS-LEFT']] = $system_statuses['STATUS-LEFT'];
	$statuses[$system_statuses['STATUS-CLARIFICATIONRESPONSE']] = $system_statuses['STATUS-CLARIFICATIONRESPONSE'];	
	$statuses[$system_statuses['STATUS-EXPECTED']] = $system_statuses['STATUS-EXPECTED'];
	$statuses[$system_statuses['STATUS-CUSTOM']] = $system_statuses['STATUS-CUSTOM'];
	$statuses[$system_statuses['STATUS-ATTENDED']] = $system_statuses['STATUS-ATTENDED'];

	$q = "select td_key, td_val1 from dh_type_detail where td_type='COURSE-LC-STATUS'";
	$status_lc = db_query($q)->fetchAllKeyed();
	foreach ($status_lc as $key => $value) {
		$statuses[$key] = $value;
	}

	$q = "select td_id, td_val1 from dh_type_detail where td_type='COURSE-TYPE'";
	$course_types = db_query($q)->fetchAllKeyed();
	$course_types['0'] = 'All';

	$form['#attributes'] = array('class' => array('container-inline')); 
	$form['a'] = array('#markup' => $out);
	$form['b'] = array('#type' => 'fieldset', '#title' => 'Merge Fields', '#collapsed' => true, '#collapsible' => true);
	$form['b']['a'] = array('#markup' => $merge_block);
 	//$form[$group] = array('#type' => 'fieldset', '#title' => 'Personal Information',  '#collapsible' => true, '#collapsed' => false);
	$form['l_name'] = array('#title' => 'Letter Name', '#type' => 'textfield', '#size' => '50', '#weight' => 1, 
		'#default_value' => ($add || $action =="copy")?'':$edit_data['l_name'], '#required' => 1,  );
	$form['l_event'] = array('#title' => 'Status', '#type' => 'select', '#weight' => 2, 
		'#default_value' => $add?'':$edit_data['l_event'], '#options' => $statuses, '#required' => 1,  );
	$form['l_course_type'] = array('#title' => 'Course Type', '#type' => 'select',  '#weight' => 2, 
		'#default_value' => $add?'0':$edit_data['l_course_type'], '#options' => $course_types, '#required' => 0,  );
	$form['l_subject'] = array('#title' => 'Subject', '#type' => 'textfield', '#size' => 120,  '#weight' => 3, 
		'#default_value' => $add?'':$edit_data['l_subject'], '#required' => 1,  );
	$form['l_body'] = array('#title' => 'Letter Body', '#type' => 'text_format', '#format' => 'filtered_html',  '#mutliple' => 1 , '#rows' => '20', '#cols' => 125, '#weight' => 4, 
		'#default_value' => $add?'':$edit_data['l_body'], '#required' => 1,  );
	$form['l_sms'] = array('#title' => 'SMS Text', '#type' => 'textarea', '#mutliple' => 1 , '#rows' => '3', '#cols' => 125, '#weight' => 5, 
		'#default_value' => $add?'':$edit_data['l_sms'], '#required' => 0,  );

	$attach_link = '';
	if (!$add)
		if ( isset($edit_data['l_attachment']))
		{
			$attach_link = l( "View Attachment" , file_create_url($edit_data['l_attachment']))."  |  ".l( "Delete Attachment" , "letters/".arg(1)."/delattach/".arg(3), array("attributes" => array('id' => array("delete-letter-attachment"))));
		}
	$form['l_attachment'] = array('#title' => 'Attachment', '#type' => 'file', '#weight' => 5, 
		'#default_value' => '' , '#required' => 0, '#description' => $add?'':$attach_link  );

	if ($action == 'edit')
		$action_sub = 'Update Letter';
	else
		$action_sub = 'Add Letter';

	$form['sub'] = array('#type' => 'submit', '#value' => $action_sub, '#weight' => 100);

	drupal_add_js('
		(function ($) {
			$(document).ready(function(){

				$(".merge-field-link").click(function(){
			        var $txt = $("#edit-l-body");
			        var caretPos = $txt[0].selectionStart;
			        var textAreaTxt = $txt.val();
			        var txtToAdd = $(this).text();
			        $txt.val(textAreaTxt.substring(0, caretPos) + txtToAdd + textAreaTxt.substring(caretPos) );

					return false;
				});
				$("#delete-letter-attachment").click(function () {
					var r = confirm("Are you sure you want to delete the attachment ?");
					if ( r == false )
						return false;
					else
						return true;
				});
			});

		})(jQuery);


		', 'inline');
	return $form;
}

function dh_letters_form_validate($form, &$form_state) 
{
	$centre_id = arg(1);
	$validators = array('file_validate_extensions' => array('pdf docx xls jpg'));

  	$file = file_save_upload('l_attachment', $validators);
	if (isset($file)) 
	{
		$dir = 'private:///letters/'.$centre_id;
		file_prepare_directory( $dir , FILE_CREATE_DIRECTORY);
        if ($file = file_move($file, $dir.'/'.$form_state['input']['l_event'].'-'.$file->filename )) 
		{
	        $form_state['storage']['file'] = $file;
		}
        else 
        {
            form_set_error('l_attachment', t('Failed to write the uploaded file'));
        }
	}
}

function dh_letters_form_submit($form, &$form_state)
{
	global $user;
	$centre_id = arg(1);
	$input = $form_state['input'];
	$action = arg(2);
	$id = '';
	if ( $action == 'edit' )
		$id = arg(3);

	$fields['l_name'] = $input['l_name'];
	$fields['l_event'] = $input['l_event'];
	//if ( $input['l_course_type'])
	$fields['l_course_type'] = $input['l_course_type'];
	$fields['l_subject'] = $input['l_subject'];
	$fields['l_body'] = $input['l_body']['value'];
	$fields['l_sms'] = $input['l_sms'];

	$fields['l_updated'] = date('Y-m-d H:i:s'); 
	$fields['l_updated_by'] = $user->uid;

	$fname = '';
	if ( isset($form_state['storage']) && isset($form_state['storage']['file']) )
	{
	    $file = $form_state['storage']['file'];
		$temp = pathinfo($file->filename);
		$ext = $temp['extension'];
	    unset($form_state['storage']['file']);
	    $file->status = FILE_STATUS_PERMANENT;
	    // Save file status.
	    file_save($file);
	    $fname = $file->uri;
	    /*
		$ret = s3_put_file(variable_get('s3_bucket_name', ''), str_replace("private:///", '', $file->uri), str_replace("private:///", '', $file->uri));	    
		if (!$ret['success'])
			watchdog('S3Letters', $ret['message']);
		*/
		$fields['l_attachment'] = $fname;

	}


	if ($action == 'add' || $action == 'copy')
	{
		$fields['l_created'] = date('Y-m-d H:i:s'); 
		$fields['l_created_by'] = $user->uid;		
		$fields['l_center'] = $centre_id;
		db_insert('dh_letter')->fields($fields)->execute();
	}
	else
		db_update('dh_letter')->fields($fields)->condition('l_id', $id )->execute();

	if($action == "copy") $action = "copi";
	drupal_set_message("Letter ${action}ed successfully ");
	drupal_goto("letters/$centre_id");
}


function dh_manage_letter_fields()
{
	drupal_add_library('system', 'ui.core');
	drupal_add_library('system', 'ui.dialog');
	drupal_add_library('system', 'ui.datepicker');
	drupal_add_css( libraries_get_path("editor"). "/DataTables/datatables.min.css");
	drupal_add_js( libraries_get_path("editor").  "/DataTables/datatables.min.js");

	$js = '
		var editor;
		(function ($) {
			function load_editor()
			{
				editor = new $.fn.dataTable.Editor({ 
				    ajax: {url: "/letter-fields/handler/'.arg(1).'", type: "POST"},
					table: "#letter-fields-table",
				    fields: [
				        { name: "dh_letter_fields.lf_name", label: "Field Name", type: "text" },
				        { name: "dh_letter_fields.lf_value", label:  "Field Value", type: "textarea" }, 
				        { name: "dh_letter_fields.lf_deleted", type: "hidden" },

				    ]
				});


				var table = $("#letter-fields-table").DataTable( {
				    ajax: "/letter-fields/handler/'.arg(1).'",
				    dom: "Bfrtip",
					columns: [
						{ data: "dh_letter_fields.lf_name"  },
						{ data: "dh_letter_fields.lf_value" },
					],
				    select: true,
				    buttons: [
				        { extend: "create", editor: editor },
				        { extend: "edit",   editor: editor },
			            {
			                extend: "selected",
			                text: "Delete",
			                action: function ( e, dt, node, config ) {
			                    var rows = table.rows( {selected: true} ).indexes();
			 
			                    editor
			                        .hide( editor.fields() )
			                        .one( "close", function () {
			                            setTimeout( function () { // Wait for animation
			                                editor.show( editor.fields() );
			                            }, 500 );
			                        } )
			                        .edit( rows, {
			                            title: "Delete",
			                            message: rows.length === 1 ?
			                                "Are you sure you wish to delete this row?" :
			                                "Are you sure you wish to delete these "+rows.length+" rows",
			                            buttons: "Delete"
			                        } )
			                        .val( "dh_letter_fields.lf_deleted", 1 );
			                }
			            }
				    ]
				});

			}



			$(document).ready(function(){
				load_editor();
			});
		})(jQuery);
	';
	drupal_add_js($js, 'inline');

	$out = '<h2>'.l("Back to Manage Letters", "letters/".arg(1)).'</h2>';
	$out .= '<table id="letter-fields-table">
				<thead>
					<tr>
						<th>Field Name</th>
						<th>Field Value</th>
					</tr>
				</thead>

	</table>';
	return $out;

}

function dh_manage_letter_fields_handler()
{
	global $user;
	$centre_id = arg(2);
	include_once "sites/all/libraries/editor/php/DataTables.php";

	$editor = \DataTables\Editor::inst( $db, 'dh_letter_fields', 'lf_id' ); 
	 $editor->fields( \DataTables\Editor\Field::inst( 'dh_letter_fields.lf_name' )
   	        ->validator( \DataTables\Editor\Validate::notEmpty( \DataTables\Editor\ValidateOptions::inst()
                ->message( 'Field Name is required' )
            ) ),
	    \DataTables\Editor\Field::inst( 'dh_letter_fields.lf_value' )
   	        ->validator( \DataTables\Editor\Validate::notEmpty( \DataTables\Editor\ValidateOptions::inst()
                ->message( 'Field Value is required' )
            ) ),
	    \DataTables\Editor\Field::inst( 'dh_letter_fields.lf_deleted' ),	 
	    \DataTables\Editor\Field::inst( 'dh_letter_fields.lf_center' )
	    	->setValue( $centre_id )
	    )
    ->pkey('lf_id')
    ->on( 'preRemove', function() { return false; })
    ->where( 'lf_deleted', 0)
    ->where( 'lf_center', $centre_id)
    ->process( $_POST )
    ->json();
    exit;
}


function dh_get_letter($type, $id, $event, $letter_id = 0, $comment = '')
{
	if ( ! in_array($type, array('applicant', 'student', 'r-teacher', 'a-teacher', 'lc-admin')) )
	{
		return array('result' => false, 'msg' => 'Invalid Type');
	}
	$delivery_type = DELIVERY_TYPE_EMAIL;
	$q = "";
	switch( $type )
	{
		case 'applicant':
			$q = "select a.a_id, a.a_type, a.a_center, a.a_course, a.a_f_name, a.a_m_name, a.a_l_name, a.a_address, a.a_zip, a.a_phone_mobile, a.a_mob_country, a.a_email, a.a_gender, a.a_dob, a_conf_no, a.a_auth_code, a_login, a.a_center as 'CentreID', a_uri, a_lang_discourse, ci.c_name as 'a_city', co.c_name as 'a_country', s.s_name as 'a_state', ce.c_name as 'a_center', ce.c_phone, ce.c_fax, ce.c_email, ce.c_url, ce.c_address, ce.c_pincode,  cco.c_name 'c_country', cci.c_name as 'c_city', cs.s_name as 'c_state', cr.c_name as 'CourseName', cr.c_start, cr.c_end, cr.c_description, td.td_val1 as 'c_course_type_str', cr.c_course_type, ct_teacher, ce.c_sms, al.al_recommending, al.al_area_at, co.c_code as 'a_country_code'
				from dh_applicant a left join dh_country co on a.a_country=co.c_code left join dh_state s on (a.a_country=s.s_country and a.a_state=s.s_code) left join dh_city ci on a.a_city=ci.c_id  left join dh_center ce on a.a_center=ce.c_id left join dh_country cco on ce.c_country=cco.c_code left join dh_state cs on (ce.c_country=cs.s_country and ce.c_state=cs.s_code) left join dh_city cci on ce.c_city=cci.c_id left join dh_course cr on a.a_course=cr.c_id left join dh_type_detail td on cr.c_course_type=td.td_id left join dh_course_teacher on ct_course = cr.c_id left join dh_applicant_lc al on a.a_id=al.al_applicant where a.a_id='$id' ";
			break;
		case 'student':
			break;
		case 'r-teacher':
		case 'a-teacher':
		case 'lc-admin':
			$q = "select a.a_id, a.a_type, a.a_center, a.a_course, a.a_f_name, a.a_m_name, a.a_l_name, a.a_address, a.a_zip, a.a_phone_mobile, a.a_mob_country, a.a_email, a.a_gender, a.a_dob, a_conf_no, a.a_auth_code, a_login, a.a_center as 'CentreID', a_uri, a_lang_discourse, ci.c_name as 'a_city', co.c_name as 'a_country', s.s_name as 'a_state', ce.c_name as 'a_center', ce.c_phone, ce.c_fax, ce.c_email, ce.c_url, ce.c_address, ce.c_pincode,  cco.c_name 'c_country', cci.c_name as 'c_city', cs.s_name as 'c_state', cr.c_name as 'CourseName', cr.c_start, cr.c_end, cr.c_description, td.td_val1 as 'c_course_type_str', cr.c_course_type, ct_teacher, ce.c_sms, al.al_recommending, al.al_area_at, co.c_code as 'a_country_code'
				from dh_applicant a left join dh_country co on a.a_country=co.c_code left join dh_state s on (a.a_country=s.s_country and a.a_state=s.s_code) left join dh_city ci on a.a_city=ci.c_id  left join dh_center ce on a.a_center=ce.c_id left join dh_country cco on ce.c_country=cco.c_code left join dh_state cs on (ce.c_country=cs.s_country and ce.c_state=cs.s_code) left join dh_city cci on ce.c_city=cci.c_id left join dh_course cr on a.a_course=cr.c_id left join dh_type_detail td on cr.c_course_type=td.td_id left join dh_course_teacher on ct_course = cr.c_id left join dh_applicant_lc al on a.a_id=al.al_applicant where a.a_id='$id' ";			
			break;
	}
	$result = db_query($q);
	$row = $result->fetchAssoc();
	$centre = $row['CentreID'];


	if (($row['a_email'] == '') && ($row['a_phone_mobile'] == ''))
	{
		logit($centre, 'Letter', $id, 'No letter sent, No email or Mobile!' );		
		return array('result' => false, 'msg' => 'No Email or Mobile');
	}
	if (trim($row['a_email']) == '')
		$delivery_type = DELIVERY_TYPE_SMS;

	$age = 0;
	if ( $row['a_dob'] )
	{
		$age = date_diff(date_create($row['a_dob']), date_create('today'))->y;
	}
	if ( $row['a_login'] == '' )
	{
		$row['a_login'] = $row['a_email'];
		if ( $delivery_type == DELIVERY_TYPE_SMS )
			$row['a_login'] = $row['a_phone_mobile'];
 		$row['a_auth_code'] = $id.generate_random_str(15);
		db_update('dh_applicant')->fields(array('a_auth_code' => $row['a_auth_code'], 'a_login' => $row['a_login']))->condition('a_id', $id)->execute();
	}
	$delivery_type = DELIVERY_TYPE_BOTH;	
	if (in_array($type, array('r-teacher', 'a-teacher', 'lc-admin')))
	{
		$teacher = '';
		if ($type == 'r-teacher') $teacher = $row['al_recommending'];
		if ($type == 'a-teacher') $teacher = $row['al_area_at'];
		if ($type == 'lc-admin') $teacher = $row['al_recommending'];
		if (in_array($type, array('r-teacher', 'a-teacher')))
			$q = "select t_email, t_mob_phone, t_country  from dh_teacher where CONCAT(t_f_name,' ', t_l_name) = '".addslashes($teacher)."' limit 1";
		else
			$q = "select c_lc_admin_email as 't_email' from dh_center where c_id=".$centre;
		$t_data = db_query($q)->fetchAssoc();
		$row['a_email'] = $t_data['t_email'];

		if (isset($t_data['t_country']))
		{
			if(in_array(strtoupper($t_data['t_country']), variable_get('at_countries_disabled', '')))
			{
				$row['eu_union'] = true;
			}
		}

		if (isset($row['a_country_code']))
		{
			if(in_array(strtoupper($row['a_country_code']), variable_get('at_countries_disabled', '')))
			{
				$row['eu_union'] = true;
			}
		}

		$q = "select al_id, al_area_auth, al_recommending_auth from dh_applicant_lc where al_applicant=".$id;
		$al_row = db_query($q)->fetchAssoc();
		$auth_area = $al_row['al_area_auth'];
		$auth_reco = $al_row['al_recommending_auth'];
		if (!$auth_reco)
		{
	 		$auth_area = $id.generate_random_str(15);
	 		$auth_reco = $id.generate_random_str(15);
			db_update('dh_applicant_lc')->fields(array('al_area_auth' => $auth_area, 'al_recommending_auth' => $auth_reco))->condition('al_applicant', $id)->execute();
		}
		$search['[RAuth]'] = $auth_reco;
		$search['[AAuth]'] = $auth_area;
	}
	$view_link = variable_get('applicant_url', ''); //'http://applicant.vipassana.co';

	$search['[FirstName]'] 		= $row['a_f_name'];
	$search['[MiddleName]'] 	= $row['a_m_name'];
	$search['[LastName]'] 		= $row['a_l_name'];
	$search['[RegisteredMobile]'] = $row['a_phone_mobile'];
	$search['[Applicantadd]'] 	= $row['a_address'];
	$search['[ApplicantCity]'] 	= $row['a_city'];
	$search['[ApplicantCountry]']= $row['a_country'];
	$search['[ApplicantState]'] = $row['a_state'];
	$search['[ApplicantPinCode]']= $row['a_zip'];
	$search['[ApplicantGender]']= $row['a_gender'];
	$search['[Age]'] 			= $age;
	$search['[ApplicantEmail]'] = $row['a_email'];
	$search['[AuthLogin]'] 		= $row['a_login'];
	$search['[AuthCode]'] 		= $row['a_auth_code'];
	$search['[ViewLink]'] 		= $view_link;
	$search['[CourseName]'] 	= $row['CourseName'];
	$search['[StartDate]'] 		= date("jS F Y", strtotime($row['c_start']));
	$search['[EndDate]'] 		= date("jS F Y", strtotime($row['c_end']));
  $search['[StartDateNum]']    = date("d-m-Y", strtotime($row['c_start']));
  $search['[EndDateNum]']    = date("d-m-Y", strtotime($row['c_end']));
	$search['[CourseType]'] 	= $row['c_course_type_str']; //trim(str_replace('Course', '', 
	$search['[CourseDescription]'] 	= trim($row['c_description']);
	$search['[CentreName]'] 	= $row['a_center'];
	$search['[CentreAdd]'] 		= $row['c_address'];
	$search['[CentreCity]'] 	= $row['c_city'];
	$search['[CentreState]'] 	= $row['c_state'];
	$search['[CentreCountry]'] 	= $row['c_country'];
	$search['[CentrePinCode]'] 	= $row['c_pincode'];
	$search['[CentrePhone]'] 	= $row['c_phone'];
	$search['[CentreFax]']	 	= $row['c_fax'];
	$search['[CentreEmail]'] 	= $row['c_email'];
	$search['[CentreUrl]'] 		= $row['c_url'];
	$search['[ApplicantID]'] 	= $row['a_id'];
  $search['[ApplicantLang]']  = $row['a_lang_discourse'];
	$search['[ConfirmationNo]'] = $row['a_conf_no'];
	$search['[ApplicantType]']  = $row['a_type'];
	$search['[RecommendingAT]'] = $row['al_recommending'];
	$search['[AreaTeacher]'] = $row['al_area_at'];
	$search['[UserInput]'] 		= $comment;
	if ((strtotime(date('Y-m-d')) >= $row['c_end']) && $row['ct_teacher'] <> '')
	{
		$q = "select CONCAT(t_f_name,' ',t_l_name,' | ','Contact: ',t_mob_phone,' | ','Email: ',t_email) as 'teacher' from dh_teacher left join dh_course_teacher on ct_teacher = t_id left join dh_applicant on a_course = ct_course where a_id = '$id' and a_gender = t_gender  and ct_status='Confirmed' order by t_f_name asc ";
		$rows = db_query($q);
		foreach( $rows as $r )
			$search['[ATDetails]'] .= $r->teacher."\n";
	}
	else
	{
		$search['[ATDetails]']  = '';
	}

	$q = "select lf_name, lf_value from dh_letter_fields where lf_center=".$row['CentreID']." and lf_deleted=0";
	$merge_fields = db_query($q)->fetchAllKeyed();
	foreach ($merge_fields as $key => $value) 
		$search['['.$key.']'] = str_replace (array_keys($search), array_values($search), $value);

	if ( $letter_id == 0)
		$where = " and l_event='$event' and l_course_type in ('".$row['c_course_type']."', '0') and l_deleted=0 order by l_course_type desc, l_name asc limit 1";  
	else
		$where = " and l_id=$letter_id";
	$q = "select l_id, l_name, l_subject, l_body, l_attachment, l_event, l_sms from dh_letter l where l_center='".$row['CentreID']."' $where";
	//watchdog('LETTER', $q);
	$result = db_query($q);
	$letter = $result->fetchAssoc();

	if ($letter['l_subject'] == '')
	{
		watchdog('LETTER', $q);
		logit($centre, 'Letter', $id, 'No letter Found!' );		
		return array('result' => false, 'msg' => 'No Letter Found');
	}
	$search['[LetterDetailURL]']= variable_get('applicant_url', '')."/l.php?a=".simple_crypt($row['a_id']."-".$letter['l_id']); //http://applicant.vipassana.co

	$letter_id = $letter['l_id'];
	$letter_name = $letter['l_name'];
	$event_name = $letter['l_event'];
	$subject = str_replace(array_keys($search), array_values($search), $letter['l_subject']);
	$body = str_replace(array_keys($search), array_values($search), $letter['l_body']);
	$body = str_replace(array_keys($search), array_values($search), $body);

	$sms_body = str_replace(array_keys($search), array_values($search), $letter['l_sms']);
	$sms_body = str_replace(array_keys($search), array_values($search), $sms_body);

	$attachment = $letter['l_attachment'];
	if ( $attachment <> '')
		$attachment = str_replace('private://',drupal_realpath('private://'), $attachment );

	if ($row['c_sms'])
		$delivery_type = DELIVERY_TYPE_BOTH;
	$out = array('result' => true, 'row' => $row, 'attachment' => $attachment, 
		'subject' => $subject, 'body' => $body, 'sms_body' => $sms_body, 
		'delivery_type' => $delivery_type, 'letter_name' => $letter_name,
		'letter_id' => $letter_id, 'conf_no' => $row['a_conf_no'] );
	return $out;
}


function dh_send_letter($type, $id, $event, $letter_id = 0, $comment = '', $force_delivery_type = 0, $footer = '')
{
  $ret_val = array();
	global $user;

	$orig_letter_id = $letter_id;
	$ret = dh_get_letter($type, $id, $event, $letter_id, $comment);

	if (!$ret['result'])
		return;

	$row = $ret['row'];
	$centre = $row['CentreID'];
	$course = $row['a_course'];
	$letter_id = $ret['letter_id'];
	$subject = $ret['subject'];
	$body = $ret['body']."<br>".$footer;
	$sms_body = $ret['sms_body'];
	$conf_no = $ret['conf_no'];
	$attachment = array();
	if ( $ret['attachment'] <> '')
		$attachment[] = $ret['attachment'];
	if ($force_delivery_type)
		$delivery_type = $force_delivery_type;
	else
		$delivery_type = $ret['delivery_type'];
	$letter_name = $ret['letter_name'];
	$delivery_type = DELIVERY_TYPE_BOTH;

	$STATUS_CONFIRMED = db_query("select td_val1 from dh_type_detail where td_type='COURSE-SYSTEM-STATUS' and td_key in ('STATUS-CONFIRMED', 'STATUS-EXPECTED')")->fetchCol();
	if (  in_array($event, $STATUS_CONFIRMED) )
	{
		if ( $row['a_uri'] == '' )
			$row['a_uri'] = create_application_pdf($row['a_id']);

		$ret = s3_get_file(variable_get('s3_bucket_name', ''), str_replace("private:///", '', $row['a_uri']), 'file', 'pdf');
		$attachment[] = $ret['data'];
	}
	if ( in_array($event, array('R-ATReview', 'A-ATReview')) )
	{
		if ( $row['a_uri'] == '' )
			$row['a_uri'] = create_application_pdf($row['a_id']);

		if (!isset($row['eu_union']))
		{
			$ret = s3_get_file(variable_get('s3_bucket_name', ''), str_replace("private:///", '', $row['a_uri']), 'file', 'pdf');
			//$attachment[] = str_replace("private://", drupal_realpath("private://"), $row['a_uri']);
			$attachment[] = $ret['data'];
		}
	
		if ($row['a_email'] == '')
		{
			$log = "No email found for AT ";
			logit($centre,  'Letter', $id, $log );
			$user_ids = db_query("select td_key, td_val1 from dh_type_detail where td_type='COURSE-APPLICANT'")->fetchAllKeyed();
			_update_status($id, "R-ATTransfer", $user_ids['COURSE-SYSTEM-UID']);
		}
	}

  $center_setting = db_query("select cs_email_from_name, cs_email_reply_to, cs_whatsapp_msg from dh_center_setting where cs_center=".$row['CentreID'])->fetchAssoc();

	if ( ($delivery_type == DELIVERY_TYPE_EMAIL) || ($delivery_type == DELIVERY_TYPE_BOTH) )
	{

		//$from = $center_setting['cs_email_from_name'];
		$from = $center_setting['cs_email_from_name'].' <'.$center_setting['cs_email_reply_to'].'>';
		$mg = Mailgun::create(variable_get('mailgun_key', ''));
		$test_mode = variable_get('mode_test', '0');
		$to = $row['a_email'];

		if ( $test_mode )
			$to = variable_get('mode_test_emails', '');
		$options = array(
		  'from'    => $from,
		  'to'      => $to,  //info@dhamma.net.in
		  'subject' => $subject,
		  'html'    => $body,
		  'v:app-id' => $id,
		  'v:test-mode' => $test_mode
		);

//		  'cc'      => 'vinay@vinay.in',

		if ( count($attachment) > 0)
		{
			foreach( $attachment as $a )
				$options['attachment'][] = array('filePath' => $a);
		}

    $mailgun_domain = get_mailgun_domain($from);

		$res = $mg->messages()->send($mailgun_domain, $options );

    $ret_val['email_res_id'] = $res->getId();

		//watchdog('MAILGUN', print_r($res, true));
		$log = "Letter sent (Email: $to) - ".$letter_name." ( Response: ".$res->getId()." )";
		if ($comment <> '')
			$log .= " ($comment)";
		logit($centre,  'Letter', $id, $log );

		if ($mg)
		{
			unset($mg);
			$mg = null;
		}

		if ($res)
		{
			unset($res);
			$res = null;
		}
	}
	
	if ( ($delivery_type == DELIVERY_TYPE_SMS) || ($delivery_type == DELIVERY_TYPE_BOTH) )
	{
		$sms_url = variable_get('sms_url', '');
		//$sms_url = str_replace(array('%phone', '%data'), array($row['a_phone_mobile'], urlencode($sms_body)), $sms_url );
		//$response = file_get_contents($sms_url);
		if ( (!in_array(strtolower($event), array('custom', 'expected', 'attended', 'errors', 'left'))) && (in_array($type, array('applicant', 'student'))) && $center_setting['cs_whatsapp_msg'] )
		{
      $a_type = "Sit";
      if ($row['a_type'] == "Sevak")
        $a_type = "Serve";

			if ($conf_no <> '')
      {
        // $event .= " (".$conf_no.", ".$a_type.")";
        $event .= " (".$conf_no.")";
      }
      // else
      // {
      //   $event .= " (".$a_type.")";
      // }

			$params = array($row['a_f_name'].' '.$row['a_l_name'], $row['CourseName'], $event);
			
			if ($row['a_phone_mobile'])
			{
				$wa_id = whatsapp_360_get_waid("+".$row['a_mob_country'].$row['a_phone_mobile']);
				if ($wa_id)
				{
					$response = send_whatsapp_360($wa_id, "status_update_2", $params );
					$log = "WhatsApp sent - ".$event." ( Response: $response )";
					dh_sms_billing( $centre, $course, $letter_id, $id, $row['a_phone_mobile']);
					logit($centre,  'Letter', $id, $log );					
				}
				else
					logit($centre,  'Letter', $id, "Could not get contact whatsapp id to send" );
			}
			else
				logit($centre,  'Letter', $id, "Mobile number not present, not sending whatsapp" );
			
		}
	}
  return $ret_val;
}

function dh_sms_billing( $centre, $course, $letter, $applicant, $mobile )
{
	$sms = array();
	$sms['cs_center'] = $centre;
	$sms['cs_course'] = $course;
	$sms['cs_letter'] = $letter;
	$sms['cs_applicant'] = $applicant;
	$sms['cs_mobile'] = $mobile;
	/*$credits = floor($size/160);
	if (($size % 160) > 0)
		$credits += 1;
	*/
	$sms['cs_credits_used'] = 1;
	db_insert('dh_center_sms')->fields($sms)->execute();
}


function _get_letter_fields( $centre )
{
	$FIELDS = array('[FirstName]', '[MiddleName]', '[LastName]', '[RegisteredMobile]', '[Applicantadd]', '[ApplicantCity]', '[ApplicantCountry]', 
		'[ApplicantState]', '[ApplicantPinCode]', '[ApplicantGender]', '[Age]', '[ApplicantEmail]', '[AuthLogin]', '[AuthCode]', '[ViewLink]', '[CourseName]',
		'[StartDate]', '[EndDate]', '[StartDateNum]', '[EndDateNum]', '[CourseType]', '[CourseDescription]', '[CentreName]', '[CentreAdd]', '[CentreCity]', '[CentreState]', '[CentreCountry]',
		'[CentrePinCode]', '[CentrePhone]', '[CentreFax]', '[CentreEmail]', '[CentreUrl]', '[ApplicantID]', '[ApplicantLang]', '[ConfirmationNo]', '[UserInput]',
		'[RecommendingAT]', '[AreaTeacher]', '[LetterDetailURL]', '[ApplicantType]', '[ATDetails]');

	$q = "select lf_name from dh_letter_fields where lf_center = $centre  and lf_deleted =0  order by lf_name"; //(0, $centre)
	$custom_fields = db_query($q)->fetchCol();
	foreach ($custom_fields as $key => $value) 
		$FIELDS[] = '['.$value.']';
//	drupal_set_message(print_r($custom_fields, true));
//	if (! empty($custom_fields))
//		$FIELDS += $custom_fields;
	return $FIELDS;
}


function mailgun_webhook()
{
	$UID_SYSTEM = db_query("select td_val1 from dh_type_detail where td_type='COURSE-APPLICANT' and td_key='COURSE-SYSTEM-UID'")->fetchField();
	$key = variable_get('mailgun_key', '');
	//watchdog('WEBHOOK', print_r($_POST,true) );
	//$post_data = json_decode( file_get_contents( 'php://input' ), true );
	$post_data = $_POST;
	#watchdog('WEBHOOK', print_r($post_data, true) );
	//watchdog('WEBHOOK', print_r($_POST,true) );
	if ($_SERVER['REQUEST_METHOD'] == 'POST') 
	{
		if(isset($post_data['timestamp']) && isset($post_data['token']) && isset($post_data['signature']) && hash_hmac('sha256', $post_data['timestamp'] . $post_data['token'], $key) === $post_data['signature'])
		{
			if (isset($post_data['app-id']))
			{
				switch($post_data['event'])
				{
					case 'delivered':
						watchdog('WEBHOOK', "Test Mode: ".$post_data['test-mode'].", App id is ".$post_data['app-id']);
						break;
					case 'complained':
					case 'bounced':
					case 'dropped':
					case 'failed':
					case 'rejected':
						if ($post_data['test-mode'] == 1)
							break;
						$desc = strtoupper($post_data['event'])." - ".$post_data['recipient'];
						if (isset($post_data['error'])) $desc .= " ".$post_data['error'];
						if (isset($post_data['reason'])) $desc .= " ".$post_data['reason'];
						if (isset($post_data['notification'])) $desc .= " (".$post_data['notification'].") ";
						if (isset($post_data['description'])) $desc .= " ".$post_data['description'];
						$temp = db_query("select a_center, a_email from dh_applicant where a_id=".$post_data['app-id'])->fetchAssoc();
						$centre = $temp['a_center'];
						$new_email = $temp['a_email'];
						if ( $new_email == $post_data['recipient'] )
						{
							db_query("update dh_applicant set a_status='Errors', a_updated = '".date('Y-m-d H:i:s')."', a_updated_by='".$UID_SYSTEM."' where a_id=".$post_data['app-id']);
							logit($centre,  'Letter', $post_data['app-id'], $desc);
						}
						break;

				}
			}
			elseif (isset($post_data['at-app-id']))
			{
				switch($post_data['event'])
				{
					case 'delivered':
						watchdog('WEBHOOK', "Test Mode: ".$post_data['test-mode'].", AT App id is ".$post_data['at-app-id']);
						break;
					case 'complained':
					case 'bounced':
					case 'dropped':
					case 'failed':
					case 'rejected':
						if ($post_data['test-mode'] == 1)
							break;
						$desc = strtoupper($post_data['event'])." - ".$post_data['recipient'];
						if (isset($post_data['error'])) $desc .= " ".$post_data['error'];
						if (isset($post_data['reason'])) $desc .= " ".$post_data['reason'];
						if (isset($post_data['notification'])) $desc .= " (".$post_data['notification'].") ";
						if (isset($post_data['description'])) $desc .= " ".$post_data['description'];
						log_course_teacher($post_data['at-app-id'], $desc);
						break;

				}
			}


			/*}elseif($_POST['event'] == 'bounced'){
				$subject = "[Mailgun] Bounced Email";
				$body = "Recipient: " . $_POST['recipient'] . "\nDomain: " . $_POST['domain'] . "\nCode: " . $_POST['code'] . "\nError: " . $_POST['error'] . "\nNotification: " . $_POST['notification'] . "\n\nMessage-headers: " . $_POST['message-headers'] . "\n";
				mail($to, $subject, $body, "From: " . $from,"-f". $from);
			}elseif($_POST['event'] == 'dropped'){
				$subject = "[Mailgun] Failed Email";
				$body = "Recipient: " . $_POST['recipient'] . "\nDomain: " . $_POST['domain'] . "\nCode: " . $_POST['code'] . "\nReason: " . $_POST['reason'] . "\nDescription: " . $_POST['description'] . "\n\nMessage-headers: " . $_POST['message-headers'] . "\n";
				mail($to, $subject, $body, "From: " . $from,"-f". $from);
			}
			*/
		}
	}
	header('X-PHP-Response-Code: 200', true, 200);	
	print "ok 200";
}

function mailgun_webhook3()
{
	$UID_SYSTEM = db_query("select td_val1 from dh_type_detail where td_type='COURSE-APPLICANT' and td_key='COURSE-SYSTEM-UID'")->fetchField();
	$key = variable_get('mailgun_key', '');
	//watchdog('WEBHOOK', print_r($_POST,true) );
	$post_data = json_decode( file_get_contents( 'php://input' ), true );
	//$post_data = $_POST;
	//watchdog('WEBHOOK', print_r($post_data, true) );
	//watchdog('WEBHOOK', print_r($_POST,true) );
	if(isset($post_data['signature']['timestamp']) && isset($post_data['signature']['token']) && isset($post_data['signature']['signature']) && hash_hmac('sha256', $post_data['signature']['timestamp'] . $post_data['signature']['token'], $key) === $post_data['signature']['signature'])
	{
		switch($post_data['event-data']['event'])
		{
			case 'delivered':
				watchdog('WEBHOOK', "Test Mode: ".$post_data['user-variables']['test-mode'].", App id is ".$post_data['user-variables']['app-id']);
				break;
			case 'complained':
			case 'bounced':
			case 'dropped':
			case 'failed':
			case 'rejected':
			case 'failed':
				if (  (!isset($post_data['event-data']['user-variables']['app-id']))  && (!isset($post_data['event-data']['user-variables']['at-app-id'])) )
					break;
				if ($post_data['user-variables']['test-mode'] == 1)
					break;
				$desc = '(3) '.strtoupper($post_data['event-data']['event'])." - ".$post_data['event-data']['recipient'];
				if (isset($post_data['event-data']['delivery-status']['code'])) $desc .= " ".$post_data['event-data']['delivery-status']['code'];
				if (isset($post_data['event-data']['reason'])) $desc .= " ".$post_data['event-data']['reason'];
				if (isset($post_data['event-data']['delivery-status']['message'])) $desc .= " (".$post_data['event-data']['delivery-status']['message'].") ";
				if (isset($post_data['event-data']['delivery-status']['description'])) $desc .= " ".$post_data['event-data']['delivery-status']['description'];
				break;

		}
		if (isset($post_data['event-data']['user-variables']['app-id']))
		{
			$temp = db_query("select a_center, a_email from dh_applicant where a_id=".$post_data['event-data']['user-variables']['app-id'])->fetchAssoc();
			$centre = $temp['a_center'];
			$new_email = $temp['a_email'];
			if ( $new_email == $post_data['event-data']['recipient'] )
			{
				db_query("update dh_applicant set a_status='Errors', a_updated = '".date('Y-m-d H:i:s')."', a_updated_by='".$UID_SYSTEM."' where a_id=".$post_data['event-data']['user-variables']['app-id']);
				logit($centre,  'Letter', $post_data['event-data']['user-variables']['app-id'], $desc);
			}
		}
		elseif (isset($post_data['event-data']['user-variables']['at-app-id']))
		{
			log_course_teacher($post_data['event-data']['user-variables']['at-app-id'], $desc);
		}
		else
		{
			watchdog('WEBHOOK3-NO-ID', print_r($post_data, true) );
		}
	}
	else
	{
		watchdog('WEBHOOK3', print_r($post_data, true) );
	}
	header('X-PHP-Response-Code: 200', true, 200);	
	print "ok 200";
}


function get_mailgun_domain($from)
{
    $mailgun_dhamma_domain = "";
    $mailgun_vridhamma_domain = "";
    $mailgun_domains = array_map('trim', explode(',', variable_get('mailgun_primary_domain', '')));

    $mailgun_domain = $mailgun_domains[0];
    $from_top_domain = implode('.', array_slice(explode('.', imap_rfc822_parse_adrlist($from, "")[0]->host), -2, 2));

    foreach($mailgun_domains as $d)
    {
        $mailgun_top_domain = implode('.', array_slice(explode('.', $d), -2, 2));
        if(strtolower($from_top_domain) == strtolower($mailgun_top_domain))
          $mailgun_domain = $d;
    }

    return $mailgun_domain;
}
