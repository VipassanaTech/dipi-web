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
			if ( ! dh_letters_access('manage') ) { drupal_access_denied(); return; }
			return drupal_get_form('dh_letters_form');
			break;
		case 'view':
			$row = db_query("select * from dh_letter where l_id='".arg(3)."'")->fetchAssoc();
			return dh_letters_view($row);
			break;
		case 'edit':
			if ( ! dh_letters_access('manage') ) { drupal_access_denied(); return; }
			$row = db_query("select * from dh_letter where l_id='".arg(3)."'")->fetchAssoc();
			return drupal_get_form('dh_letters_form', $row);
			break;
    case 'copy':
      if ( ! dh_letters_access('manage') ) { drupal_access_denied(); return; }
      $row = db_query("select * from dh_letter where l_id='".arg(3)."'")->fetchAssoc();
      return drupal_get_form('dh_letters_form', $row);
      break;
		case 'delete':
			if ( ! dh_letters_access('manage') ) { drupal_access_denied(); return; }
			db_update("dh_letter")->fields(array('l_deleted' => 1))->condition('l_id', arg(3))->execute();
			drupal_set_message("Letter marked as inactive. It can be made active again from the Inactive Letters list below.");
			drupal_goto("letters/".arg(1));
        case 'restore':
            if ( ! dh_letters_access('manage') ) { drupal_access_denied(); return; }
            db_update("dh_letter")->fields(array('l_deleted' => 0))->condition('l_id', arg(3))->execute();
            drupal_set_message("Letter marked as active.");
            drupal_goto("letters/".arg(1));
		case 'delattach':
			if ( ! dh_letters_access('manage') ) { drupal_access_denied(); return; }
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
	$can_manage = dh_letters_access('manage');
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
		$rs[] = l("View", "letters/$centre_id/view/".$r['l_id'] );
		if ($can_manage)
		{
			$rs[] = l("Edit", "letters/$centre_id/edit/".$r['l_id'] );
			$rs[] = l("Copy", "letters/$centre_id/copy/".$r['l_id'] );
			$rs[] = l("Mark Inactive", "letters/$centre_id/delete/".$r['l_id'], array("attributes" => array("class" => array("delete-letter") ,"txt" => $r['LetterName'])));
		}
		$rows[] = array_values($rs);
	}
	$header = $can_manage ? array('Letter Name', 'Status', 'Course Type', 'Subject', 'Action', 'Action', 'Action', 'Action')
	                     : array('Letter Name', 'Status', 'Course Type', 'Subject', 'Action');
	$attributes = array('id' => 'table-letters');
	if ($can_manage)
		$out = '<h2>'.l("Add Letter", "letters/$centre_id/add").' &nbsp;&nbsp;|&nbsp;&nbsp;'.l("Manage Fields", "letter-fields/$centre_id").' &nbsp;&nbsp;|&nbsp;&nbsp;'.l("Back to Dashboard", "centre/".$centre_id).'</h2>';
	else
		$out = '<h2>'.l("Back to Dashboard", "centre/".$centre_id).'</h2>';
	$out .= "<h2>Active Letters</h2>";
	$out .= theme('table', array('header' => $header, 'rows' => $rows, 'attributes' => $attributes));


    $q_d = "select l_id, l_name as 'LetterName', l_event as 'Status', tdc.td_val1 as 'Course Type', l_subject as 'Subject' from dh_letter left join dh_type_detail tdc on l_course_type=tdc.td_id where l_center='$centre_id' and l_deleted=1 order by l_event , tdc.td_val1";
//and a_status='Confirmed'
    $result_d = db_query($q_d);
    $rows_d = array();
    while( $r_d = $result_d->fetchAssoc() )
    {
        unset($rs_d);

        $rs_d[] = $r_d['LetterName'];
        $rs_d[] = $r_d['Status'];
        /* was $r['Course Type'] - the active loop's variable, so every row here
           showed the last active letter's course type instead of its own */
        $rs_d[] = $r_d['Course Type']?$r_d['Course Type']:"All";
        $rs_d[] = $r_d['Subject'];
        $rs_d[] = l("View", "letters/$centre_id/view/".$r_d['l_id'] );
        if ($can_manage)
            $rs_d[] = l("Mark Active", "letters/$centre_id/restore/".$r_d['l_id'], array("attributes" => array("class" => array("restore-letter") ,"txt" => $r_d['LetterName'])));
        $rows_d[] = array_values($rs_d);
    }
    $header_d = $can_manage ? array('Letter Name', 'Status', 'Course Type', 'Subject', 'Action', 'Action')
                          : array('Letter Name', 'Status', 'Course Type', 'Subject', 'Action');
    $attributes_d = array('id' => 'table-letters-d');
    $out .= "<br><br><h2>Inactive Letters</h2>";
    $out .= $can_manage
        ? "<p>These letters are not in use and will not be sent. They are not deleted - use Mark Active to bring one back.</p>"
        : "<p>These letters are not in use and will not be sent. They are not deleted.</p>";
    $out .= theme('table', array('header' => $header_d, 'rows' => $rows_d, 'attributes' => $attributes_d));


	//$out .= '<div id="replace-me"></div>';
	$js = '
		(function ($) {
			$(document).ready(function(){
				$("#table-letters").DataTable();
                $("#table-letters-d").DataTable();
				$("#table-letters tbody").on("click", ".delete-letter", function () {
					var lname = $(this).attr("txt");
					var r = confirm("Mark the letter "+lname+" as inactive? It will stop being sent, but you can make it active again.");
					if ( r == false )
						return false;
					else
						return true;
				});
                $("#table-letters-d tbody").on("click", ".restore-letter", function () {
                    var lname = $(this).attr("txt");
                    var r = confirm("Mark the letter "+lname+" as active again?");
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

/**
 * Read-only rendering of one letter.
 *
 * Shows the letter as it is stored: the body is displayed as HTML because that
 * is how it is composed and sent, and the [MergeField] tokens are left exactly
 * as written - there is no applicant in this context to substitute them for.
 */
function dh_letters_view($row)
{
	if (!$row)
		return '<p>'.t('Letter not found.').'</p>';

	$centre_id = arg(1);
	drupal_add_css(drupal_get_path('module', 'dh_manageapp').'/css/manageapp.css');

	$course_type = $row['l_course_type']
		? db_query("select td_val1 from dh_type_detail where td_id=:t", array(':t' => $row['l_course_type']))->fetchField()
		: t('All');

	$out  = '<h2>'.l("Back to Manage Letters", "letters/$centre_id").'</h2>';
	if ( dh_letters_access('manage') )
		$out .= '<p><em>Read-only view. '.l("Edit this letter", "letters/$centre_id/edit/".$row['l_id']).' to make changes.</em></p>';
	else
		$out .= '<p><em>'.t('You are viewing this letter. You do not have permission to change it.').'</em></p>';

	/* metadata */
	$out .= '<table class="letter-view"><tbody>';
	$out .= '<tr><th>'.t('Letter Name').'</th><td>'.check_plain($row['l_name']).'</td></tr>';
	$out .= '<tr><th>'.t('Status').'</th><td>'.check_plain($row['l_event']).'</td></tr>';
	$out .= '<tr><th>'.t('Course Type').'</th><td>'.check_plain($course_type ? $course_type : t('All')).'</td></tr>';
	$out .= '<tr><th>'.t('Subject').'</th><td>'.check_plain($row['l_subject']).'</td></tr>';
	$out .= '</tbody></table>';

	/* body - stored and sent as HTML, so shown as HTML; tokens stay literal */
	$out .= '<h3>'.t('Letter Body').'</h3>';
	$out .= '<div class="letter-view-body" style="border:1px solid #ccc;padding:12px;background:#fff;">'
	      . ($row['l_body'] !== '' ? $row['l_body'] : '<em>'.t('(empty)').'</em>')
	      . '</div>';

	/* SMS - plain text */
	if (trim($row['l_sms']) !== '')
	{
		$out .= '<h3>'.t('SMS Text').'</h3>';
		$out .= '<div class="letter-view-sms" style="border:1px solid #ccc;padding:12px;white-space:pre-wrap;">'
		      . check_plain($row['l_sms']).'</div>';
	}

	/* attachment */
	if (trim($row['l_attachment']) !== '')
	{
		$out .= '<h3>'.t('Attachment').'</h3><p>'
		      . l(t('View Attachment'), file_create_url($row['l_attachment'])).'</p>';
	}

	return $out;
}

function dh_letters_form($form, &$form_state, $edit_data = array())
{
	$out = '<h2>'.l("Back to Manage Letters", "letters/".arg(1)).'</h2>';
	drupal_add_css(drupal_get_path('module', 'dh_manageapp').'/css/manageapp.css');
	// Load CKEditor directly (like the Search "review comment" editor) instead of
	// binding the body to a filtered text format. This gives us a real WYSIWYG with
	// ACF off (allowedContent:true), so pasted formatting is retained as-is.
	drupal_add_js( libraries_get_path("ckeditor")."/ckeditor.js" );
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
	// Plain textarea; CKEditor is attached to it manually in the inline JS below.
	$form['l_body'] = array('#title' => 'Letter Body', '#type' => 'textarea', '#rows' => '20', '#cols' => 125, '#weight' => 4,
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

				// Manually attach CKEditor to the letter body textarea. allowedContent:true
				// disables the Advanced Content Filter, so pasted formatting is kept as-is
				// (nothing stripped). CKEditor syncs back to the textarea on form submit.
				if (window.CKEDITOR) {
					if (CKEDITOR.instances["edit-l-body"]) { CKEDITOR.instances["edit-l-body"].destroy(true); }
					CKEDITOR.replace("edit-l-body", {
						allowedContent: true,
						height: 380,
						resize_enabled: true,
						pasteFromWordRemoveFontStyles: false,
						pasteFromWordRemoveStyles: false,
						// Enable "Upload" tab in the Image dialog + drag-drop/paste image upload.
						filebrowserImageUploadUrl: "/letters/'.arg(1).'/image-upload",
						uploadUrl: "/letters/'.arg(1).'/image-upload",
						filebrowserUploadMethod: "xhr",
						// Full toolbar: every document-editing button this CKEditor build ships.
						toolbar: [
							{ name: "document", items: ["Source","-","Preview","Print","-","Templates"] },
							{ name: "clipboard", items: ["Cut","Copy","Paste","PasteText","PasteFromWord","-","Undo","Redo"] },
							{ name: "editing", items: ["Find","Replace","-","SelectAll"] },
							{ name: "basicstyles", items: ["Bold","Italic","Underline","Strike","Subscript","Superscript","-","CopyFormatting","RemoveFormat"] },
							{ name: "paragraph", items: ["NumberedList","BulletedList","-","Outdent","Indent","-","Blockquote","CreateDiv","-","JustifyLeft","JustifyCenter","JustifyRight","JustifyBlock","-","BidiLtr","BidiRtl","Language"] },
							{ name: "links", items: ["Link","Unlink","Anchor"] },
							{ name: "insert", items: ["Image","Table","HorizontalRule","PageBreak","SpecialChar","Smiley"] },
							"/",
							{ name: "styles", items: ["Styles","Format","Font","FontSize"] },
							{ name: "colors", items: ["TextColor","BGColor"] },
							{ name: "tools", items: ["Maximize","ShowBlocks","-","About"] }
						]
					});
				}

				// Insert a merge field at the cursor inside the WYSIWYG (falls back to the
				// raw textarea if CKEditor is not active).
				$(".merge-field-link").click(function(){
					var txtToAdd = $(this).text();
					if (window.CKEDITOR && CKEDITOR.instances["edit-l-body"]) {
						CKEDITOR.instances["edit-l-body"].insertText(txtToAdd);
					} else {
						var $txt = $("#edit-l-body");
						var caretPos = $txt[0].selectionStart;
						var textAreaTxt = $txt.val();
						$txt.val(textAreaTxt.substring(0, caretPos) + txtToAdd + textAreaTxt.substring(caretPos) );
					}
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

/**
 * Sanitise an uploaded file's base name (without extension) for safe on-disk
 * storage: strip any path parts, replace anything outside [A-Za-z0-9_-] with
 * "_", cap the length, and trim stray separators. Returns $fallback when nothing
 * usable remains (e.g. a name made entirely of spaces or unicode). Shared by the
 * letter-body image upload and the letter attachment.
 */
function _dh_letter_safe_basename($name, $fallback = 'file', $maxlen = 60)
{
	$base = preg_replace('/[^A-Za-z0-9_-]/', '_', pathinfo($name, PATHINFO_FILENAME));
	$base = trim(substr($base, 0, $maxlen), '_-');
	return $base === '' ? $fallback : $base;
}

function dh_letters_form_validate($form, &$form_state)
{
	$centre_id = arg(1);
	// Accept the common document + image types people actually attach. Modern
	// formats (jpeg, png, xlsx, doc) were missing before, which caused most of
	// the "failed" uploads.
	$validators = array('file_validate_extensions' => array('pdf doc docx xls xlsx jpg jpeg png gif'));

	$file = file_save_upload('l_attachment', $validators);

	// file_save_upload() returns: a file object on success; FALSE when a file was
	// sent but failed validation (it has already set a specific error, e.g. wrong
	// extension / too large); NULL when no file was chosen. Only proceed on a real
	// object -- the old isset() check let FALSE through and masked the real error
	// with a generic "Failed to write" message.
	if ($file)
	{
		$dir = 'private:///letters/'.$centre_id;
		if (!file_prepare_directory($dir, FILE_CREATE_DIRECTORY))
		{
			form_set_error('l_attachment', t('Could not prepare the attachment folder.'));
			return;
		}

		// Build a safe destination: sanitised status prefix + sanitised base name +
		// the original (already validated) extension. Avoids spaces/unicode/special
		// chars breaking the stored path or the download URL.
		$info  = pathinfo($file->filename);
		$ext   = isset($info['extension']) ? strtolower($info['extension']) : '';
		$event = _dh_letter_safe_basename($form_state['input']['l_event'], 'letter');
		$base  = _dh_letter_safe_basename($file->filename, 'attachment');
		$dest  = $dir.'/'.$event.'-'.$base.($ext !== '' ? '.'.$ext : '');

		if ($file = file_move($file, $dest, FILE_EXISTS_RENAME))
		{
			$form_state['storage']['file'] = $file;
		}
		else
		{
			form_set_error('l_attachment', t('Failed to write the uploaded file.'));
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

/**
 * CKEditor image-upload endpoint for the letter body.
 *
 * Receives an XHR upload from CKEditor's uploadimage/filebrowser plugins,
 * validates it is a real image, stores it under the public files dir, and
 * returns the JSON CKEditor expects: {uploaded:1, fileName, url} on success,
 * {uploaded:0, error:{message}} on failure.
 */
function dh_letter_image_upload()
{
	drupal_add_http_header('Content-Type', 'application/json; charset=utf-8');

	$fail = function ($msg) {
		print drupal_json_encode(array('uploaded' => 0, 'error' => array('message' => $msg)));
		drupal_exit();
	};

	if ( empty($_FILES['upload']) || !isset($_FILES['upload']['tmp_name']) || $_FILES['upload']['error'] != UPLOAD_ERR_OK )
		$fail(t('No file was uploaded.'));

	// Make sure tmp_name is a genuine PHP upload, not a crafted local path.
	if ( !drupal_is_cli() && !is_uploaded_file($_FILES['upload']['tmp_name']) )
		$fail(t('Invalid upload.'));

	$upload = $_FILES['upload'];

	// Validate by CONTENT, not the client-supplied name: the real image type
	// decides both acceptance and the stored extension. This keeps odd names
	// (spaces, no extension, unicode, path parts) safe and the stored file type
	// honest (a script disguised with an image name can never be saved runnable).
	$info = @getimagesize($upload['tmp_name']);
	if ( $info === FALSE )
		$fail(t('The uploaded file is not a valid image.'));

	$type_ext = array(IMAGETYPE_JPEG => 'jpg', IMAGETYPE_PNG => 'png', IMAGETYPE_GIF => 'gif');
	if ( !isset($type_ext[$info[2]]) )
		$fail(t('Only JPG, PNG and GIF images are allowed.'));
	$ext = $type_ext[$info[2]];

	if ( $upload['size'] > 5 * 1024 * 1024 )
		$fail(t('Image is too large (max 5 MB).'));

	$dir = 'public://letters/images';
	if ( !file_prepare_directory($dir, FILE_CREATE_DIRECTORY) )
		$fail(t('Could not prepare the upload directory.'));

	// Readable display name (see _dh_letter_safe_basename); falls back to "image".
	$base = _dh_letter_safe_basename($upload['name'], 'image');
	$dest = $dir . '/' . $base . '-' . uniqid() . '.' . $ext;
	$saved = file_unmanaged_save_data(file_get_contents($upload['tmp_name']), $dest, FILE_EXISTS_RENAME);
	if ( !$saved )
		$fail(t('Failed to save the uploaded image.'));

	print drupal_json_encode(array(
		'uploaded' => 1,
		'fileName' => drupal_basename($saved),
		'url' => file_create_url($saved),
	));
	drupal_exit();
}

/**
 * Delete letter-body images that no letter references any more (janitor).
 *
 * Images uploaded via the letter WYSIWYG are stored, unmanaged, under
 * public://letters/images and are referenced only by <img src> URLs inside
 * dh_letter.l_body. When an image is removed from a letter (or the letter row
 * is hard-deleted) the file is left behind. This scans that directory and
 * removes any file whose name appears in no letter body. Files modified within
 * the last $grace_hours are skipped, so a freshly-uploaded image whose letter
 * has not been saved yet is never swept away. Soft-deleted letters (l_deleted=1)
 * still keep their row, so their images are preserved (safe for restore).
 *
 * @return array ('scanned','deleted','kept','files')
 */
function dh_letter_cleanup_orphan_images($grace_hours = 24, $verbose = FALSE)
{
	$stats = array('scanned' => 0, 'deleted' => 0, 'kept' => 0, 'files' => array());
	$dir = 'public://letters/images';
	$realdir = drupal_realpath($dir);
	if ( !$realdir || !is_dir($realdir) )
		return $stats;

	$handle = opendir($realdir);
	if ( !$handle )
		return $stats;

	$cutoff = REQUEST_TIME - ($grace_hours * 3600);

	while ( ($entry = readdir($handle)) !== FALSE )
	{
		if ( $entry === '.' || $entry === '..' || $entry === '.htaccess' )
			continue;
		$path = $realdir . '/' . $entry;
		if ( !is_file($path) )
			continue;

		$stats['scanned']++;

		// Grace period: leave a just-uploaded file whose letter may not be saved yet.
		if ( filemtime($path) > $cutoff )
		{
			$stats['kept']++;
			continue;
		}

		// Referenced by any letter body? db_like() escapes _ and % in the name.
		$referenced = (bool) db_query_range(
			"SELECT 1 FROM dh_letter WHERE l_body LIKE :pat",
			0, 1,
			array(':pat' => '%' . db_like($entry) . '%')
		)->fetchField();

		if ( $referenced )
		{
			$stats['kept']++;
			continue;
		}

		if ( file_unmanaged_delete($dir . '/' . $entry) )
		{
			$stats['deleted']++;
			$stats['files'][] = $entry;
			if ( $verbose )
				echo "Deleted orphan image: $entry\n";
		}
	}
	closedir($handle);

	if ( $stats['deleted'] > 0 )
		watchdog('LetterImageCleanup', 'Removed @d orphan letter image(s); kept @k, scanned @s.',
			array('@d' => $stats['deleted'], '@k' => $stats['kept'], '@s' => $stats['scanned']), WATCHDOG_NOTICE);

	return $stats;
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
			$q = "select t_email, t_mob_phone, t_country  from dh_teacher where CONCAT(t_f_name,' ', t_l_name) = '".addslashes($teacher)."' and t_status='Active' limit 1";
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

/**
 * Turn <img> tags that point to letter-uploaded images (public://letters/images)
 * into CID inline attachments, so the image is embedded in the email itself
 * instead of linked by URL. This keeps the internal DIPI URL out of applicant
 * emails and makes the image render without "show images".
 *
 * Externally-hosted images (http/https on other hosts), data: URIs, and letters
 * with no such images are returned unchanged. If a referenced file is missing on
 * disk, that tag is left exactly as-is -- it never breaks the body or the send.
 *
 * @param string $html Letter body HTML.
 * @return array ('html' => string, 'inline' => array of array('filePath','filename'))
 */
function _dh_letter_embed_inline_images($html)
{
	$inline = array();
	if ($html === '' || stripos($html, '<img') === FALSE)
		return array('html' => $html, 'inline' => $inline);

	$marker = '/' . trim(variable_get('file_public_path', 'sites/default/files'), '/') . '/letters/images/';
	$seen = array();

	$new = preg_replace_callback(
		'/(<img\b[^>]*?\bsrc\s*=\s*)(["\'])(.*?)\2/i',
		function ($m) use (&$inline, &$seen, $marker) {
			$path = parse_url(html_entity_decode($m[3]), PHP_URL_PATH);
			if (!$path || strpos($path, $marker) === FALSE)
				return $m[0]; // external / unrelated -> leave untouched

			$name = rawurldecode(basename($path));
			if ($name === '' || strpos($name, '..') !== FALSE)
				return $m[0];

			$real = drupal_realpath('public://letters/images/' . $name);
			if (!$real || !is_file($real))
				return $m[0]; // file gone -> don't break the tag

			if (!isset($seen[$name])) {
				$inline[] = array('filePath' => $real, 'filename' => $name);
				$seen[$name] = TRUE;
			}
			return $m[1] . $m[2] . 'cid:' . $name . $m[2];
		},
		$html
	);

	// preg_replace_callback() returns NULL on failure (e.g. backtrack limit on a
	// very large body); fall back to the original body with no inline images so a
	// send is never broken by this step.
	if ($new === NULL)
		return array('html' => $html, 'inline' => array());

	return array('html' => $new, 'inline' => $inline);
}

function dh_send_letter($type, $id, $event, $letter_id = 0, $comment = '', $force_delivery_type = 0, $footer = '')
{
  $ret_val = array();
	global $user;

	$orig_letter_id = $letter_id;
	$ret = dh_get_letter($type, $id, $event, $letter_id, $comment);

	if (!$ret['result'])
	{
		$ret_val['error'] = $ret['msg'];
		return $ret_val;
	}

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
            $ret_val['error'] = "No email found for AT";
            if ( in_array($event, array('R-ATReview')) )
            {
                $user_ids = db_query("select td_key, td_val1 from dh_type_detail where td_type='COURSE-APPLICANT'")->fetchAllKeyed();
                _update_status($id, "R-ATTransfer", $user_ids['COURSE-SYSTEM-UID']);
                $ret_val['error'] .= ", application moved to R-ATTransfer";
            }

			return $ret_val;
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

		// Embed letter-uploaded images as CID inline attachments so they travel
		// inside the email (no DIPI URL exposed; renders without "show images").
		// External images / letters with no uploaded images are unaffected.
		$embedded = _dh_letter_embed_inline_images($body);
		$body = $embedded['html'];

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

		// Inline (CID) images referenced as cid: in the body above.
		if ( !empty($embedded['inline']) )
			foreach ( $embedded['inline'] as $img )
				$options['inline'][] = $img;

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
		if ( (!in_array(strtolower($event), array('custom', 'expected', 'attended', 'errors', 'left', 'duplicate'))) && (in_array($type, array('applicant', 'student'))) && $center_setting['cs_whatsapp_msg'] )
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
                    if($response)
                    {
                        $log = "WhatsApp sent - ".$event." ( Response: $response )";
                        dh_sms_billing( $centre, $course, $letter_id, $id, $row['a_phone_mobile']);
                        logit($centre,  'Letter', $id, $log );
                    }
                    else
                    {
                        logit($centre,  'Letter', $id, "Could not send whatsapp" );
                    }

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
