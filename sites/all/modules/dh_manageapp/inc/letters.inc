<?php

require_once 'vendor/autoload.php';
use Mailgun\Mailgun;

define('DELIVERY_TYPE_EMAIL', 1);
define('DELIVERY_TYPE_SMS', 2);


//define('MAILGUN_KEY', 'key-1dec8567ba03bd362ea0d719f554c1cb');
define('SENDGRID_KEY', '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');
//define('PRIMARY_DOMAIN', 'nl.vipassana.co');


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
		case 'delete':
			db_update("dh_letter")->fields(array('l_deleted' => 1))->condition('l_id', arg(3))->execute();
			drupal_set_message("Letter deleted successfully");
			drupal_goto("letters/".arg(1));
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
		$rs[] = l("Delete", "letters/$centre_id/delete/".$r['l_id'], array("attributes" => array("class" => array("delete-letter") ,"txt" => $r['LetterName'])));
		$rows[] = array_values($rs);
	}
	$header = array('Letter Name', 'Status', 'Course Type', 'Subject', 'Action', 'Action');
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

	$merge_fields = _get_letter_fields(arg(1));
	$field_count = 1;
	$merge_block = '<table class="table-merge-fields"><tr><td colspan='.$field_count.'>';
	$i = 0;
	foreach($merge_fields as $v)
	{
		if ( ($i % $field_count) == 0 )
			$merge_block .= '</tr><tr>';
		$merge_block .= '<td><a class="merge-field-link" href="#">'.$v.'</a></td>';
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
	$statuses[$system_statuses['STATUS-RECEIVED']] = $system_statuses['STATUS-RECEIVED'];
	$statuses[$system_statuses['STATUS-LEFT']] = $system_statuses['STATUS-LEFT'];
	$statuses[$system_statuses['STATUS-CLARIFICATIONRESPONSE']] = $system_statuses['STATUS-CLARIFICATIONRESPONSE'];	
	$statuses[$system_statuses['STATUS-EXPECTED']] = $system_statuses['STATUS-EXPECTED'];
	$statuses[$system_statuses['STATUS-CUSTOM']] = $system_statuses['STATUS-CUSTOM'];
	$statuses[$system_statuses['STATUS-ATTENDED']] = $system_statuses['STATUS-ATTENDED'];

	$q = "select td_id, td_val1 from dh_type_detail where td_type='COURSE-TYPE'";
	$course_types = db_query($q)->fetchAllKeyed();
	$course_types['0'] = 'All';

	$form['#attributes'] = array('class' => array('container-inline')); 
	$form['a'] = array('#markup' => $out);
	$form['b'] = array('#type' => 'fieldset', '#title' => 'Merge Fields', '#collapsed' => false, '#collapsible' => true);
	$form['b']['a'] = array('#markup' => $merge_block);
 	//$form[$group] = array('#type' => 'fieldset', '#title' => 'Personal Information',  '#collapsible' => true, '#collapsed' => false);
	$form['l_name'] = array('#title' => 'Letter Name', '#type' => 'textfield', '#size' => '50', '#weight' => 1, 
		'#default_value' => $add?'':$edit_data['l_name'], '#required' => 1,  );
	$form['l_event'] = array('#title' => 'Status', '#type' => 'select', '#weight' => 2, 
		'#default_value' => $add?'':$edit_data['l_event'], '#options' => $statuses, '#required' => 1,  );
	$form['l_course_type'] = array('#title' => 'Course Type', '#type' => 'select',  '#weight' => 2, 
		'#default_value' => $add?'0':$edit_data['l_course_type'], '#options' => $course_types, '#required' => 0,  );
	$form['l_subject'] = array('#title' => 'Subject', '#type' => 'textfield', '#size' => 120,  '#weight' => 3, 
		'#default_value' => $add?'':$edit_data['l_subject'], '#required' => 1,  );
	$form['l_body'] = array('#title' => 'Letter Body', '#type' => 'textarea', '#mutliple' => 1 , '#rows' => '20', '#cols' => 125, '#weight' => 4, 
		'#default_value' => $add?'':$edit_data['l_body'], '#required' => 1,  );
	$form['l_sms'] = array('#title' => 'SMS Text', '#type' => 'textarea', '#mutliple' => 1 , '#rows' => '3', '#cols' => 125, '#weight' => 5, 
		'#default_value' => $add?'':$edit_data['l_sms'], '#required' => 0,  );

	$attach_link = '';
	if (!$add)
		if ( isset($edit_data['l_attachment']))
		{
			$attach_link = l( "View Attachment" , file_create_url($edit_data['l_attachment']));
		}
	$form['l_attachment'] = array('#title' => 'Attachment', '#type' => 'file', '#weight' => 5, 
		'#default_value' => '' , '#required' => 0, '#description' => $add?'':$attach_link  );

	if ($add)
		$action = 'Add Letter';
	else
		$action = 'Update Letter';
	$form['sub'] = array('#type' => 'submit', '#value' => $action, '#weight' => 100);

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
	$fields['l_body'] = $input['l_body'];
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
		$fields['l_attachment'] = $fname;

	}


	if ($action == 'add')
	{
		$fields['l_created'] = date('Y-m-d H:i:s'); 
		$fields['l_created_by'] = $user->uid;		
		$fields['l_center'] = $centre_id;
		db_insert('dh_letter')->fields($fields)->execute();
	}
	else
		db_update('dh_letter')->fields($fields)->condition('l_id', $id )->execute();
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
	if ( ! in_array($type, array('applicant', 'student')) )
	{
		return array('result' => false, 'msg' => 'Invalid Type');
	}
	$delivery_type = DELIVERY_TYPE_EMAIL;
	$q = "";
	switch( $type )
	{
		case 'applicant':
			$q = "select a.a_id, a.a_type, a.a_center, a.a_f_name, a.a_m_name, a.a_l_name, a.a_address, a.a_zip, a.a_phone_mobile, a.a_email, a.a_gender, a.a_dob, a_conf_no, a.a_auth_code, a_login, a.a_center as 'CentreID', a_uri, ci.c_name as 'a_city', co.c_name as 'a_country', s.s_name as 'a_state', ce.c_name as 'a_center', ce.c_phone, ce.c_fax, ce.c_email, ce.c_url, ce.c_address, ce.c_pincode,  cco.c_name 'c_country', cci.c_name as 'c_city', cs.s_name as 'c_state', cr.c_name as 'CourseName', cr.c_start, cr.c_end, td.td_val1 as 'c_course_type'
				from dh_applicant a left join dh_country co on a.a_country=co.c_code left join dh_state s on (a.a_country=s.s_country and a.a_state=s.s_code) left join dh_city ci on a.a_city=ci.c_id  left join dh_center ce on a.a_center=ce.c_id left join dh_country cco on ce.c_country=cco.c_code left join dh_state cs on (ce.c_country=cs.s_country and ce.c_state=cs.s_code) left join dh_city cci on ce.c_city=cci.c_id left join dh_course cr on a.a_course=cr.c_id left join dh_type_detail td on cr.c_course_type=td.td_id where a.a_id='$id' ";
			break;
		case 'student':
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
 		$row['a_auth_code'] = generate_random_str();
		db_update('dh_applicant')->fields(array('a_auth_code' => $row['a_auth_code'], 'a_login' => $row['a_login']))->condition('a_id', $id)->execute();
	}
		

	$view_link = variable_get('applicant_url', ''); //'http://applicant.vipassana.co';

	$search['[FirstName]'] 		= $row['a_f_name'];
	$search['[MiddleName]'] 	= $row['a_m_name'];
	$search['[LastName]'] 		= $row['a_l_name'];
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
	$search['[CourseType]'] 	= $row['c_course_type']; //trim(str_replace('Course', '', 
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
	$search['[Applicant_id]'] 	= $row['a_id'];
	$search['[ConfirmationNo]'] = $row['a_conf_no'];
	$search['[ApplicantType]']  = $row['a_type'];
	$search['[UserInput]'] 		= $comment;


	$q = "select lf_name, lf_value from dh_letter_fields where lf_center=".$row['CentreID']." and lf_deleted=0";
	$merge_fields = db_query($q)->fetchAllKeyed();
	foreach ($merge_fields as $key => $value) 
		$search['['.$key.']'] = str_replace (array_keys($search), array_values($search), $value);

	if ( $letter_id == 0)
		$where = " and l_event='$event' and l_course_type in ('".$row['c_course_type']."', '0') and l_deleted=0";  
	else
		$where = " and l_id=$letter_id";
	$q = "select l_id, l_name, l_subject, l_body, l_attachment, l_event, l_sms from dh_letter l where l_center='".$row['CentreID']."' $where";
	watchdog('LETTER', $q);
	$result = db_query($q);
	$letter = $result->fetchAssoc();

	if ($letter['l_subject'] == '')
	{
		logit($centre, 'Letter', $id, 'No letter Found!' );		
		return array('result' => false, 'msg' => 'No Letter Found');
	}
	$search['[LetterDetailURL]']= variable_get('applicant_url', '')."/l.php?a=".simple_crypt($row['a_id']."-".$letter['l_id']); //http://applicant.vipassana.co


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

	$out = array('result' => true, 'row' => $row, 'attachment' => $attachment, 
		'subject' => $subject, 'body' => $body, 'sms_body' => $sms_body, 
		'delivery_type' => $delivery_type, 'letter_name' => $letter_name);
	return $out;
}


function dh_send_letter($type, $id, $event, $letter_id = 0, $comment = '')
{
	global $user;

	$ret = dh_get_letter($type, $id, $event, $letter_id, $comment);

	if (!$ret['result'])
		return;

	$row = $ret['row'];
	$centre = $row['CentreID'];
	$subject = $ret['subject'];
	$body = $ret['body'];
	$sms_body = $ret['sms_body'];
	$attachment = $ret['attachment'];
	$delivery_type = $ret['delivery_type'];
	$letter_name = $ret['letter_name'];

	$STATUS_CONFIRMED = db_query("select td_val1 from dh_type_detail where td_type='COURSE-SYSTEM-STATUS' and td_key='STATUS-CONFIRMED'")->fetchField();
	if ( $event == $STATUS_CONFIRMED )
	{
		if ( $row['a_uri'] == '' )
			$row['a_uri'] = create_application_pdf($row['a_id']);

		$attachment = str_replace("private://", drupal_realpath("private://"), $row['a_uri']);
	}


	if ( $delivery_type == DELIVERY_TYPE_EMAIL)
	{
		$from_email = db_query("select cs_email_from_name, cs_email_reply_to from dh_center_setting where cs_center=".$row['CentreID'])->fetchAssoc();
		//$from = $from_email['cs_email_from_name'];
		$from = $from_email['cs_email_from_name'].' <'.$from_email['cs_email_reply_to'].'>';
		$mg = Mailgun::create(variable_get('mailgun_key', ''));

		/*$reply_to = new SendGrid\ReplyTo($from_email['cs_email_reply_to'], $from_email['cs_email_from_name']);
		$from = new SendGrid\Email($from_email['cs_email_from_name'], "registrar@me.vipassana.co" );
		//$subject = "Sending with SendGrid is Fun";
		$to = new SendGrid\Email("Vinay", "ramyan04@gmail.com");
		$content = new SendGrid\Content("text/plain", $body);
		$mail = new SendGrid\Mail($from, $subject, $to, $content);
    	$mail->setReplyTo($reply_to);

    	$cc = new SendGrid\Email("Info Centre", "info@dhamma.net.in");
    	//$mail->personalization[0]->addCC($cc);

		$apiKey = SENDGRID_KEY;
		$sg = new \SendGrid($apiKey);
		
		if ($attachment <> '')
		{
			$filename = basename($attachment);
			$file_encoded = base64_encode(file_get_contents($attachment));
			$attach = new SendGrid\Attachment();
			$attach->setContent($file_encoded);
			$attach->setType("application/pdf");
			$attach->setDisposition("attachment");
			$attach->setFilename($filename);
			$mail->addAttachment($attach);
		}


		$response = $sg->client->mail()->send()->post($mail);
			

		watchdog('SENDGRID', 'Attachment: '.$attachment.' Response: '.$response->statusCode()." Body: ".$response->body() );
		*/
		//$response->statusCode();
		//print_r($response->headers());
		//echo $response->body();

		$test_mode = variable_get('mode_test', '0');
		$to = $row['a_email'];
		if ( $test_mode )
			$to = variable_get('mode_test_emails', '');
		$options = array(
		  'from'    => $from,
		  'to'      => $to,  //info@dhamma.net.in
		  'subject' => $subject,
		  'text'    => $body,
		  'v:app-id' => $id,
		  'v:test-mode' => $test_mode
		);

//		  'cc'      => 'vinay@vinay.in',

		if ($attachment <> '')
		{
			 $options['attachment'] = array(array('filePath' => $attachment));
		}
		$res = $mg->messages()->send(variable_get('mailgun_primary_domain', ''), $options );

		//watchdog('MAILGUN', print_r($res, true));
		$log = "Letter sent (Email) - ".$letter_name." ( Response: ".$res->getId()." )";
		if ($comment <> '')
			$log .= " ($comment)";
		logit($centre,  'Letter', $id, $log );
	}
	else
	{
		$sms_url = variable_get('sms_url', '');
		if ($sms_body <> '' )
		{
			$sms_url = str_replace(array('%phone', '%data'), array($row['a_phone_mobile'], urlencode($sms_body)), $sms_url );
			//watchdog('SMS', $sms_body);
			//http://bhashsms.com/api/sendmsg.php?user=success&pass=123456&sender=DHAMMA&phone=9003099142&text=TestSMS&priority=ndnd&stype=normal
			$response = file_get_contents($sms_url);
			$log = "Letter sent (SMS) - ".$letter_name." ( Response: $response )";
			logit($centre,  'Letter', $id, $log );
		}
		else
			logit($centre, 'Letter', $id, "No SMS Body found in $letter_name!");
	}
}


function _get_letter_fields( $centre )
{
	$FIELDS = array('[FirstName]', '[MiddleName]', '[LastName]', '[Applicantadd]', '[ApplicantCity]', '[ApplicantCountry]', 
		'[ApplicantState]', '[ApplicantPinCode]', '[ApplicantGender]', '[Age]', '[ApplicantEmail]', '[AuthLogin]', '[AuthCode]', '[ViewLink]', '[CourseName]',
		'[StartDate]', '[EndDate]', '[CourseType]', '[CentreName]', '[CentreAdd]', '[CentreCity]', '[CentreState]', '[CentreCountry]', 
		'[CentrePinCode]', '[CentrePhone]', '[CentreFax]', '[CentreEmail]', '[CentreUrl]', '[Applicant_id]', '[ConfirmationNo]', '[UserInput]', 
		'[LetterDetailURL]', '[ApplicantType]');

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
	if ($_SERVER['REQUEST_METHOD'] == 'POST') 
	{
		if(isset($_POST['timestamp']) && isset($_POST['token']) && isset($_POST['signature']) && hash_hmac('sha256', $_POST['timestamp'] . $_POST['token'], $key) === $_POST['signature'])
		{
			switch($_POST['event'])
			{
				case 'delivered':
					watchdog('WEBHOOK', "Test Mode: ".$_POST['test-mode'].", App id is ".$_POST['app-id']);
					break;
				case 'complained':
				case 'bounced':
				case 'dropped':
				case 'failed':
				case 'rejected':
					if ($_POST['test-mode'] == 1)
						break;
					$desc = strtoupper($_POST['event'])." - ".$_POST['recipient'];
					if (isset($_POST['error'])) $desc .= " ".$_POST['error'];
					if (isset($_POST['reason'])) $desc .= " ".$_POST['reason'];
					if (isset($_POST['notification'])) $desc .= " ".$_POST['notification'];
					if (isset($_POST['description'])) $desc .= " ".$_POST['description'];
					$temp = db_query("select a_center, a_email from dh_applicant where a_id=".$_POST['app-id'])->fetchAssoc();
					$centre = $temp['a_center'];
					$new_email = $temp['a_email'];
					if ( $new_email == $_POST['recipient'] )
					{
						db_query("update dh_applicant set a_status='Errors', a_updated = '".date('Y-m-d H:i:s')."', a_updated_by='".$UID_SYSTEM."' where a_id=".$_POST['app-id']);
						logit($centre,  'Letter', $_POST['app-id'], $desc);
					}
					break;

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

