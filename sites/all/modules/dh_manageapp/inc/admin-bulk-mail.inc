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

function dh_admin_bulk_mail_image_upload() { drupal_json_output(array()); }
function dh_admin_bulk_mail_attach($abm_id) { return 'TODO attach '.$abm_id; }
function dh_admin_bulk_mail_attach_delete($abm_id, $abma_id) { return 'TODO detach'; }
function dh_admin_bulk_mail_delete($abm_id) { return 'TODO delete '.$abm_id; }
function dh_admin_bulk_mail_show_log($abm_id) { return 'TODO log '.$abm_id; }
function dh_admin_bulk_mail_get_log($abm_id) { drupal_json_output(array()); }
function dh_admin_bulk_mail_unsubscribe($token) { return 'TODO unsub'; }
