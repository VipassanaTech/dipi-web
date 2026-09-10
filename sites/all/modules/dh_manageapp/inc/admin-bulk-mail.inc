<?php
// sites/all/modules/dh_manageapp/inc/admin-bulk-mail.inc
// Administrator (centre-independent) bulk email — self-contained subsystem.

function dh_admin_bulk_mail_create_draft($where, $criteria, $centres_csv) {
  global $user;
  $rq = dh_bulk_mail_build_recipient_query($where);
  $now = date('Y-m-d H:i:s');
  $fields = array(
    'abm_query' => $rq['query'],
    'abm_count' => $rq['count'],
    'abm_criteria' => dh_bulk_mail_clean_criteria($criteria),
    'abm_centres' => $centres_csv,
    'abm_updated' => $now,
    'abm_updated_by' => $user->uid,
  );
  // Reuse this session's still-unscheduled draft so repeated searches update one
  // draft instead of accumulating orphans; otherwise insert a fresh draft.
  $cur = isset($_SESSION['abm_working_draft']) ? $_SESSION['abm_working_draft'] : 0;
  if ($cur && db_query("select abm_id from dh_admin_bulk_mail where abm_id=:id and abm_processed=0 and abm_deleted=0", array(':id' => $cur))->fetchField()) {
    db_update('dh_admin_bulk_mail')->fields($fields)->condition('abm_id', $cur)->execute();
    return $cur;
  }
  $fields['abm_processed'] = 0;
  $fields['abm_created'] = $now;
  $fields['abm_created_by'] = $user->uid;
  $id = db_insert('dh_admin_bulk_mail')->fields($fields)->execute();
  $_SESSION['abm_working_draft'] = $id;
  return $id;
}

function dh_admin_bulk_mail_list() {
  $labels = array(0 => 'Draft', 1 => 'Pending', 2 => 'In-progress', 3 => 'Done', 4 => 'Failed', 5 => 'Cancelled');
  $rows = array();
  $res = db_query("select * from dh_admin_bulk_mail where abm_deleted=0 order by abm_id desc");
  foreach ($res as $r) {
    $centres = $r->abm_centres !== '' ? (count(array_filter(explode(',', $r->abm_centres))) . ' centre(s)') : 'All';
    $actions = array();
    if ($r->abm_processed == 0 || $r->abm_processed == 1) {
      $actions[] = l('Compose', 'admin-bulk-mail/' . $r->abm_id . '/compose');
    }
    $actions[] = l('Log', 'admin-bulk-mail/' . $r->abm_id . '/show-log');
    $actions[] = l('Delete', 'admin-bulk-mail/' . $r->abm_id . '/delete');
    $rows[] = array(
      check_plain($r->abm_name) === '' ? '(unnamed)' : check_plain($r->abm_name),
      $centres,
      (int) $r->abm_count,
      (int) $r->abm_sent,
      (int) $r->abm_failed,
      ((int) $r->abm_skipped_unsub + (int) $r->abm_skipped_invalid),
      check_plain($r->abm_schedule_date),
      $labels[$r->abm_processed],
      implode(' | ', $actions),
    );
  }
  $out = '<h2>' . l('+ New admin bulk email', 'admin-bulk-mail/new') . '</h2>';
  $out .= theme('table', array(
    'header' => array('Name', 'Centres', 'Recipients', 'Sent', 'Failed', 'Skipped', 'Send date', 'Status', 'Actions'),
    'rows' => $rows,
    'empty' => 'No admin bulk emails yet.',
  ));
  return $out;
}

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
  $cnt = db_query("select abm_count from dh_admin_bulk_mail where abm_id=:id", array(':id' => $form_state['abm_id']))->fetchField();
  if ((int) $cnt <= 0) {
    form_set_error('', 'This blast has 0 recipients — adjust the search criteria before scheduling.');
  }
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
  unset($_SESSION['abm_working_draft']); // scheduled -> next search starts a fresh draft
  drupal_set_message('Bulk email scheduled.');
  $form_state['redirect'] = 'admin-bulk-mail';
}

/**
 * Send ONE admin bulk-mail message via Mailgun. Used by test-send and by the
 * queue consumer (Task 8). $abm is a dh_admin_bulk_mail row (assoc array) with
 * abm_id/abm_subject/abm_body/abm_from_name/abm_from_email/abm_reply_to.
 * Embeds CID inline images and attaches the blast's documents. Honours test mode.
 * Returns array('result'=>TRUE,'res_id'=>...); throws on Mailgun error.
 */
function dh_send_admin_bulk_mail(array $abm, $to_email, $to_name = '', $footer = '', $app_id = 0) {
  $from = $abm['abm_from_name'] . ' <' . $abm['abm_from_email'] . '>';
  $mg = \Mailgun\Mailgun::create(variable_get('mailgun_key', ''));
  $test_mode = variable_get('mode_test', '0');
  $to = $test_mode ? variable_get('mode_test_emails', '') : $to_email;

  // Embed letter-style inline images (same public://letters/images dir) as CID.
  $embedded = _dh_letter_embed_inline_images($abm['abm_body']);
  $html = $embedded['html'] . $footer;

  $options = array(
    'from' => $from,
    'to' => $to,
    'subject' => $abm['abm_subject'],
    'html' => $html,
    'v:abm-id' => $abm['abm_id'],
    'v:app-id' => $app_id,
    'v:test-mode' => $test_mode,
  );
  if (!empty($abm['abm_reply_to'])) {
    $options['h:Reply-To'] = $abm['abm_reply_to'];
  }

  // Document attachments for this blast.
  $atts = db_query("select abma_uri, abma_name from dh_admin_bulk_mail_attachment where abma_abm=:id", array(':id' => $abm['abm_id']));
  foreach ($atts as $a) {
    $path = drupal_realpath($a->abma_uri);
    if ($path && file_exists($path)) {
      $options['attachment'][] = array('filePath' => $path, 'filename' => $a->abma_name);
    }
  }

  // CID inline images.
  if (!empty($embedded['inline'])) {
    foreach ($embedded['inline'] as $img) {
      $options['inline'][] = $img;
    }
  }

  $domain = get_mailgun_domain($from);
  $res = $mg->messages()->send($domain, $options);
  return array('result' => TRUE, 'res_id' => $res->getId());
}

function dh_admin_bulk_mail_test_submit($form, &$form_state) {
  global $user;
  // #limit_validation_errors => array() on this button prunes $form_state['values']
  // down to (essentially) empty in Drupal 7, so read the raw, fully-populated
  // input instead -- otherwise the UPDATE below writes NULL into NOT-NULL columns.
  $in = $form_state['input'];
  $g = function ($k) use ($in) { return isset($in[$k]) ? $in[$k] : ''; };
  $abm_id = $form_state['abm_id'];
  // Persist current content to the row so the test reflects exactly what will send.
  db_update('dh_admin_bulk_mail')->fields(array(
    'abm_name' => $g('abm_name'), 'abm_subject' => $g('abm_subject'), 'abm_body' => $g('abm_body'),
    'abm_from_name' => $g('abm_from_name'), 'abm_from_email' => $g('abm_from_email'), 'abm_reply_to' => $g('abm_reply_to'),
    'abm_test_email' => $g('test_email'), 'abm_test_sent_at' => date('Y-m-d H:i:s'),
    'abm_updated' => date('Y-m-d H:i:s'), 'abm_updated_by' => $user->uid,
  ))->condition('abm_id', $abm_id)->execute();

  if (!filter_var($g('test_email'), FILTER_VALIDATE_EMAIL)) {
    drupal_set_message('Enter a valid test email address.', 'error');
    $form_state['rebuild'] = TRUE;
    return;
  }
  $abm = db_query("select * from dh_admin_bulk_mail where abm_id=:id", array(':id' => $abm_id))->fetchAssoc();
  try {
    // Include the REAL unsubscribe footer so the test is a faithful preview of what
    // recipients receive (the marker just flags it as a test).
    $footer = "<br><br><small>[test email]</small>" . dh_admin_bulk_mail_unsub_footer($g('test_email'));
    $ret = dh_send_admin_bulk_mail($abm, $g('test_email'), 'Test', $footer, 0);
    $note = variable_get('mode_test', '0') ? ' (test mode is ON — actually delivered to ' . check_plain(variable_get('mode_test_emails', '')) . ')' : '';
    drupal_set_message('Test email sent to ' . check_plain($g('test_email')) . $note . '. Mailgun id: ' . check_plain($ret['res_id']));
  }
  catch (\Throwable $e) {
    drupal_set_message('Test send failed: ' . check_plain($e->getMessage()), 'error');
    watchdog('AdminBulkMail', 'test send failed abm=@a: @m', array('@a' => $abm_id, '@m' => $e->getMessage()), WATCHDOG_ERROR);
  }
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
function dh_admin_bulk_mail_delete($abm_id) {
  db_query("update dh_admin_bulk_mail set abm_deleted=1, abm_updated=:u where abm_id=:id", array(':u' => date('Y-m-d H:i:s'), ':id' => $abm_id));
  drupal_set_message('Admin bulk email deleted.');
  drupal_goto('admin-bulk-mail');
}
function dh_admin_bulk_mail_show_log($abm_id) {
  $abm = db_query("select * from dh_admin_bulk_mail where abm_id=:id", array(':id' => $abm_id))->fetchAssoc();
  if (!$abm) { drupal_not_found(); drupal_exit(); }
  $out = '<h2>' . l('Back to Admin Bulk Email', 'admin-bulk-mail') . '</h2>';
  $out .= '<h3>' . check_plain($abm['abm_name'] !== '' ? $abm['abm_name'] : '(unnamed)') . '</h3>';
  $out .= '<p>Matched <b>' . (int) $abm['abm_count'] . '</b> &middot; Sent <b>' . (int) $abm['abm_sent'] . '</b> &middot; Failed <b>' . (int) $abm['abm_failed'] . '</b> &middot; Skipped (unsubscribed) <b>' . (int) $abm['abm_skipped_unsub'] . '</b> &middot; Skipped (invalid email) <b>' . (int) $abm['abm_skipped_invalid'] . '</b></p>';
  $out .= '<p><em>Only problem recipients are listed below; successful sends are counted but not listed individually.</em></p>';
  $rows = array();
  $res = db_query("select * from dh_admin_bulk_mail_log where abml_abm=:id order by abml_id desc", array(':id' => $abm_id));
  foreach ($res as $r) {
    $rows[] = array(
      check_plain($r->abml_name),
      check_plain($r->abml_email),
      check_plain($r->abml_center),
      check_plain($r->abml_status),
      check_plain($r->abml_error),
      check_plain($r->abml_created),
    );
  }
  $out .= theme('table', array(
    'header' => array('Name', 'Email', 'Centre', 'Status', 'Error', 'Time'),
    'rows' => $rows,
    'empty' => 'No problem recipients (all sends succeeded, or the blast has not run yet).',
  ));
  return $out;
}
function dh_admin_bulk_mail_get_log($abm_id) { drupal_json_output(array()); }
/** Insert an EXCEPTIONS-ONLY log row (failed / skipped-unsub / skipped-invalid). Successes are not logged per-row. */
function dh_admin_bulk_mail_log_row($abm_id, $app_id, $name, $email, $center, $status, $error) {
  db_insert('dh_admin_bulk_mail_log')->fields(array(
    'abml_abm' => $abm_id, 'abml_applicant' => $app_id, 'abml_name' => $name, 'abml_email' => $email,
    'abml_center' => $center, 'abml_status' => $status, 'abml_error' => substr((string) $error, 0, 1000),
    'abml_created' => date('Y-m-d H:i:s'),
  ))->execute();
}

/**
 * Global unsubscribe footer. The link points to the PUBLIC applicant app
 * (applicant.vridhamma.org/abm-unsubscribe/<token>), exactly like centre bulk mail
 * uses applicant_url/bm-unsubscribe — NOT the DIPI admin app. The applicant handler
 * shows a confirmation page (opt-out happens only on the button click there).
 */
function dh_admin_bulk_mail_unsub_footer($email) {
  $pass = variable_get('bulk_mail_auth_pass', '');
  $authcode = bin2hex(openssl_encrypt($email, "AES-128-ECB", $pass));
  $url = variable_get('applicant_url', '') . '/abm-unsubscribe/' . $authcode;
  return "<br><br><small>If you don't want to receive these emails, <a href='$url'>Unsubscribe</a></small>";
}

/** Handle one queued admin message (recipient or completion sentinel). Updates summary tallies; logs only exceptions. */
function dh_admin_bulk_mail_consume($data) {
  if (!empty($data->completed)) {
    db_query("update dh_admin_bulk_mail set abm_processed=3, abm_updated=:u where abm_id=:id", array(':u' => date('Y-m-d H:i:s'), ':id' => $data->abm_id));
    return;
  }
  $abm = db_query("select * from dh_admin_bulk_mail where abm_id=:id", array(':id' => $data->abm_id))->fetchAssoc();
  if (!$abm) { return; }
  $email = isset($data->email) ? $data->email : '';
  $name = isset($data->name) ? $data->name : '';
  $app_id = isset($data->app_id) ? $data->app_id : 0;
  $center = isset($data->center) ? $data->center : NULL;

  if (!filter_var($email, FILTER_VALIDATE_EMAIL)) {
    db_query("update dh_admin_bulk_mail set abm_skipped_invalid=abm_skipped_invalid+1 where abm_id=:id", array(':id' => $abm['abm_id']));
    dh_admin_bulk_mail_log_row($abm['abm_id'], $app_id, $name, $email, $center, 'skipped-invalid', '');
    return;
  }
  if (db_query("select abu_id from dh_admin_bulk_mail_unsubscribe where abu_email=:e", array(':e' => $email))->fetchField()) {
    db_query("update dh_admin_bulk_mail set abm_skipped_unsub=abm_skipped_unsub+1 where abm_id=:id", array(':id' => $abm['abm_id']));
    dh_admin_bulk_mail_log_row($abm['abm_id'], $app_id, $name, $email, $center, 'skipped-unsub', '');
    return;
  }
  try {
    dh_send_admin_bulk_mail($abm, $email, $name, dh_admin_bulk_mail_unsub_footer($email), $app_id);
    db_query("update dh_admin_bulk_mail set abm_sent=abm_sent+1 where abm_id=:id", array(':id' => $abm['abm_id']));
  }
  catch (\Throwable $e) {
    db_query("update dh_admin_bulk_mail set abm_failed=abm_failed+1 where abm_id=:id", array(':id' => $abm['abm_id']));
    dh_admin_bulk_mail_log_row($abm['abm_id'], $app_id, $name, $email, $center, 'failed', $e->getMessage());
    watchdog('AdminBulkMail', 'send failed abm=@a email=@e: @m', array('@a' => $abm['abm_id'], '@e' => $email, '@m' => $e->getMessage()), WATCHDOG_ERROR);
  }
}

// NOTE: the unsubscribe landing is intentionally NOT handled here. It lives in the
// public applicant app (applicant.vridhamma.org/abm-unsubscribe.php) so recipients
// never touch the DIPI admin app, and so it can show a confirmation page before
// opting out — mirroring the centre bm-unsubscribe flow. The applicant handler writes
// dh_admin_bulk_mail_unsubscribe (same DB), which the consumer above checks.
