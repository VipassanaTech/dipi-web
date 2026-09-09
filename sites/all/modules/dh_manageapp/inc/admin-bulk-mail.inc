<?php
// sites/all/modules/dh_manageapp/inc/admin-bulk-mail.inc
// Administrator (centre-independent) bulk email — self-contained subsystem.

function dh_admin_bulk_mail_create_draft($where, $criteria, $centres_csv) {
  global $user;
  $rq = dh_bulk_mail_build_recipient_query($where);
  $now = date('Y-m-d H:i:s');
  return db_insert('dh_admin_bulk_mail')->fields(array(
    'abm_query' => $rq['query'],
    'abm_count' => $rq['count'],
    'abm_criteria' => dh_bulk_mail_clean_criteria($criteria),
    'abm_centres' => $centres_csv,
    'abm_processed' => 0,   // draft
    'abm_created' => $now,
    'abm_updated' => $now,
    'abm_created_by' => $user->uid,
    'abm_updated_by' => $user->uid,
  ))->execute();
}

function dh_admin_bulk_mail_list() { return 'TODO list'; }

function dh_admin_bulk_mail_compose($abm_id) {
  return drupal_get_form('dh_admin_bulk_mail_compose_form', $abm_id);
}

function dh_admin_bulk_mail_compose_form($form, &$form_state, $abm_id) {
  $abm = db_query("select * from dh_admin_bulk_mail where abm_id=:id", array(':id' => $abm_id))->fetchAssoc();
  if (!$abm) { drupal_not_found(); drupal_exit(); }
  $form_state['abm_id'] = $abm_id;

  $centres_desc = $abm['abm_centres'] !== '' ? ('centres ' . check_plain($abm['abm_centres'])) : 'ALL centres';
  $form['info'] = array('#markup' => '<p>This blast targets <b>' . (int) $abm['abm_count'] . '</b> recipients (' . $centres_desc . ').</p>');

  $form['abm_name'] = array('#title' => 'Batch name', '#type' => 'textfield', '#required' => TRUE, '#default_value' => $abm['abm_name'], '#weight' => 1);
  $form['abm_from_name'] = array('#title' => 'From name', '#type' => 'textfield', '#required' => TRUE, '#default_value' => $abm['abm_from_name'], '#weight' => 2);
  $form['abm_from_email'] = array('#title' => 'From email', '#type' => 'textfield', '#required' => TRUE, '#default_value' => $abm['abm_from_email'], '#weight' => 3);
  $form['abm_reply_to'] = array('#title' => 'Reply-to (optional)', '#type' => 'textfield', '#default_value' => $abm['abm_reply_to'], '#weight' => 4);
  $form['abm_subject'] = array('#title' => 'Subject', '#type' => 'textfield', '#required' => TRUE, '#default_value' => $abm['abm_subject'], '#weight' => 5);
  $form['abm_body'] = array('#title' => 'Body (HTML)', '#type' => 'textarea', '#rows' => 18, '#required' => TRUE, '#default_value' => $abm['abm_body'], '#weight' => 6);

  drupal_add_js(libraries_get_path('ckeditor') . '/ckeditor.js');
  drupal_add_js('
    (function ($) {
      $(document).ready(function(){
        if (window.CKEDITOR) {
          if (CKEDITOR.instances["edit-abm-body"]) { CKEDITOR.instances["edit-abm-body"].destroy(true); }
          CKEDITOR.replace("edit-abm-body", {
            allowedContent: true,
            height: 380,
            resize_enabled: true,
            pasteFromWordRemoveFontStyles: false,
            pasteFromWordRemoveStyles: false,
            filebrowserImageUploadUrl: "/admin-bulk-mail/image-upload",
            uploadUrl: "/admin-bulk-mail/image-upload",
            filebrowserUploadMethod: "xhr",
            toolbar: [
              { name: "document", items: ["Source","-","Preview","Print"] },
              { name: "clipboard", items: ["Cut","Copy","Paste","PasteText","PasteFromWord","-","Undo","Redo"] },
              { name: "basicstyles", items: ["Bold","Italic","Underline","Strike","-","RemoveFormat"] },
              { name: "paragraph", items: ["NumberedList","BulletedList","-","Outdent","Indent","-","Blockquote","-","JustifyLeft","JustifyCenter","JustifyRight"] },
              { name: "links", items: ["Link","Unlink"] },
              { name: "insert", items: ["Image","Table","HorizontalRule","SpecialChar"] },
              "/",
              { name: "styles", items: ["Format","Font","FontSize"] },
              { name: "colors", items: ["TextColor","BGColor"] },
              { name: "tools", items: ["Maximize"] }
            ]
          });
        }
      });
    })(jQuery);
  ', 'inline');

  $atts = db_query("select abma_id, abma_name, abma_size from dh_admin_bulk_mail_attachment where abma_abm=:id order by abma_id", array(':id' => $abm_id));
  $items = array();
  foreach ($atts as $a) {
    $items[] = check_plain($a->abma_name) . ' (' . round($a->abma_size / 1024) . ' KB) — ' . l('remove', 'admin-bulk-mail/' . $abm_id . '/attach-delete/' . $a->abma_id);
  }
  $form['attachments_list'] = array(
    '#markup' => '<div class="abm-attachments"><label>Attachments</label>' . ($items ? theme('item_list', array('items' => $items)) : '<em>none</em>') . '</div>',
    '#weight' => 6.4,
  );
  $form['abm_attachment'] = array('#title' => 'Add attachment', '#type' => 'file', '#weight' => 6.5);
  $form['upload'] = array(
    '#type' => 'submit', '#value' => 'Upload attachment', '#weight' => 6.6,
    '#submit' => array('dh_admin_bulk_mail_attach_submit'), '#limit_validation_errors' => array(),
  );

  $form['abm_schedule_date'] = array(
    '#title' => 'Send date', '#type' => 'date_popup', '#date_format' => 'Y-m-d',
    '#default_value' => $abm['abm_schedule_date'] ? $abm['abm_schedule_date'] : date('Y-m-d'),
    '#date_year_range' => '-0:+1', '#weight' => 7,
  );

  $form['test_email'] = array('#title' => 'Send a test to', '#type' => 'textfield', '#default_value' => $abm['abm_test_email'], '#weight' => 8);
  $form['test'] = array('#type' => 'submit', '#value' => 'Send test email', '#submit' => array('dh_admin_bulk_mail_test_submit'), '#limit_validation_errors' => array(), '#weight' => 9);

  $centres_confirm = $abm['abm_centres'] !== '' ? (count(array_filter(explode(',', $abm['abm_centres']))) . ' centre(s)') : 'ALL centres';
  $form['schedule'] = array(
    '#type' => 'submit', '#value' => 'Confirm & Schedule', '#weight' => 10,
    '#attributes' => array('onclick' => 'return confirm("Schedule this email to ' . (int) $abm['abm_count'] . ' recipients (' . $centres_confirm . ')?");'),
  );
  return $form;
}

function dh_admin_bulk_mail_compose_form_validate($form, &$form_state) {
  // The test button uses #limit_validation_errors=>array(), so it won't reach here.
  if (isset($form_state['triggering_element']['#value']) && $form_state['triggering_element']['#value'] == 'Send test email') { return; }
  $v = $form_state['values'];
  if (!filter_var($v['abm_from_email'], FILTER_VALIDATE_EMAIL)) {
    form_set_error('abm_from_email', 'A valid From email is required.');
  }
  if (trim(strip_tags($v['abm_body'])) == '') {
    form_set_error('abm_body', 'Body is required.');
  }
}

function dh_admin_bulk_mail_compose_form_submit($form, &$form_state) {
  global $user;
  $v = $form_state['values'];
  db_update('dh_admin_bulk_mail')->fields(array(
    'abm_name' => $v['abm_name'],
    'abm_subject' => $v['abm_subject'],
    'abm_body' => $v['abm_body'],
    'abm_from_name' => $v['abm_from_name'],
    'abm_from_email' => $v['abm_from_email'],
    'abm_reply_to' => $v['abm_reply_to'],
    'abm_schedule_date' => $v['abm_schedule_date'],
    'abm_processed' => 1, // pending
    'abm_updated' => date('Y-m-d H:i:s'),
    'abm_updated_by' => $user->uid,
  ))->condition('abm_id', $form_state['abm_id'])->execute();
  drupal_set_message('Bulk email scheduled.');
  $form_state['redirect'] = 'admin-bulk-mail';
}

// Test-send is implemented in Task 7; stub for now so the button exists.
function dh_admin_bulk_mail_test_submit($form, &$form_state) {
  drupal_set_message('Test send will be wired in a later step.');
  $form_state['rebuild'] = TRUE;
}

function dh_admin_bulk_mail_image_upload() {
  // Reuse the letters image-upload verbatim: same image validation, same
  // public://letters/images dir (so _dh_letter_embed_inline_images finds them at
  // send time), same CKEditor JSON. It prints JSON and drupal_exit()s.
  return dh_letter_image_upload();
}

function dh_admin_bulk_mail_attach_submit($form, &$form_state) {
  $abm_id = $form_state['abm_id'];
  $dir = 'private://admin-bulk-mail/' . $abm_id;
  file_prepare_directory($dir, FILE_CREATE_DIRECTORY);
  $validators = array('file_validate_extensions' => array('pdf doc docx xls xlsx ppt pptx txt csv png jpg jpeg gif'));
  $file = file_save_upload('abm_attachment', $validators, $dir, FILE_EXISTS_RENAME);
  if ($file) {
    $cur = (int) db_query("select coalesce(sum(abma_size),0) from dh_admin_bulk_mail_attachment where abma_abm=:id", array(':id' => $abm_id))->fetchField();
    if ($cur + $file->filesize > 20 * 1024 * 1024) {
      file_delete($file);
      drupal_set_message('Attachment exceeds the total size limit (~20 MB).', 'error');
    }
    else {
      $file->status = FILE_STATUS_PERMANENT;
      file_save($file);
      file_usage_add($file, 'dh_manageapp', 'abm', $abm_id);
      db_insert('dh_admin_bulk_mail_attachment')->fields(array(
        'abma_abm' => $abm_id, 'abma_uri' => $file->uri, 'abma_name' => $file->filename,
        'abma_size' => $file->filesize, 'abma_created' => date('Y-m-d H:i:s'),
      ))->execute();
      drupal_set_message('Attachment added: ' . check_plain($file->filename));
    }
  }
  $form_state['rebuild'] = TRUE;
}

function dh_admin_bulk_mail_attach($abm_id) { drupal_goto('admin-bulk-mail/' . $abm_id . '/compose'); }

function dh_admin_bulk_mail_attach_delete($abm_id, $abma_id) {
  $uri = db_query("select abma_uri from dh_admin_bulk_mail_attachment where abma_id=:a and abma_abm=:b", array(':a' => $abma_id, ':b' => $abm_id))->fetchField();
  if ($uri) {
    db_delete('dh_admin_bulk_mail_attachment')->condition('abma_id', $abma_id)->execute();
    $fid = db_query("select fid from file_managed where uri=:u", array(':u' => $uri))->fetchField();
    if ($fid && ($fo = file_load($fid))) {
      file_usage_delete($fo, 'dh_manageapp', 'abm', $abm_id);
      file_delete($fo);
    }
    else {
      @drupal_unlink($uri);
    }
    drupal_set_message('Attachment removed.');
  }
  drupal_goto('admin-bulk-mail/' . $abm_id . '/compose');
}
function dh_admin_bulk_mail_delete($abm_id) { return 'TODO delete '.$abm_id; }
function dh_admin_bulk_mail_show_log($abm_id) { return 'TODO log '.$abm_id; }
function dh_admin_bulk_mail_get_log($abm_id) { drupal_json_output(array()); }
function dh_admin_bulk_mail_unsubscribe($token) { return 'TODO unsub'; }
