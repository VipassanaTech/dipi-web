<?php

/**
 * @file
 * Administration page callbacks for Mailgun.
 */

/**
 * Menu callback: displays the Mailgun module settings page.
 *
 * @param array $form
 *   Form render array.
 * @param array $form_state
 *   Array containing form state values.
 *
 * @return array
 *   An array containing form items to place on the module settings page.
 */
function mailgun_admin_settings(array $form, array &$form_state) {
  // Check if the Mailgun PHP library is installed.
  if (!mailgun_check_library()) {
    drupal_set_message(t('The Mailgun PHP library is not installed. Please see Installation section in the !link.', array(
      '!link' => l(t('documentation'), MAILGUN_DOCUMENTATION_LINK),
    )), 'error');
  }

  global $language;

  // Get title and description from mailgun_variable_info().
  module_load_include('inc', 'mailgun', 'mailgun.variable');
  $variables = module_invoke('mailgun', 'variable_info', array('language' => $language));

  $key = variable_get(MAILGUN_API_KEY, '');

  $form[MAILGUN_API_KEY] = array(
    '#title' => $variables[MAILGUN_API_KEY]['title'],
    '#type' => 'textfield',
    '#description' => $variables[MAILGUN_API_KEY]['description'],
    '#default_value' => $key,
    '#required' => TRUE,
  );

  $client = FALSE;
  if (!empty($key)) {
    try {
      $endpoint = !empty($form_state['values'][MAILGUN_API_ENDPOINT]) ? $form_state['values'][MAILGUN_API_ENDPOINT] : '';
      $client = mailgun_get_client($key, $endpoint);
    }
    catch (Exception $e) {
      watchdog('mailgun', 'An exception occurred. @code: @message', array(
        '@code' => $e->getCode(),
        '@message' => $e->getMessage(),
      ), WATCHDOG_WARNING, MAILGUN_ADMIN_PAGE);
      drupal_set_message(t('Mailgun: %message', array('%message' => $e->getMessage())), 'error');
    }
  }

  // Display settings only when a valid API key is present and client is active.
  if ($client) {
    $domain_options = array(
      '_sender' => t('Get domain from sender address'),
    );
    $result = $client->domains()->index();
    if (!empty($result)) {
      if ($result->getTotalCount() > 100) {
        $result = $client->domains()->index($result->getTotalCount());
      }
      $options = [];
      foreach ($result->getDomains() as $domain) {
        $options[$domain->getName()] = $domain->getName();
      }
      ksort($options);
      $domain_options = array_merge($domain_options, $options);
    }

    $form[MAILGUN_DOMAIN] = array(
      '#title' => $variables[MAILGUN_DOMAIN]['title'],
      '#type' => 'select',
      '#options' => $domain_options,
      '#description' => $variables[MAILGUN_DOMAIN]['description'],
      '#default_value' => variable_get(MAILGUN_DOMAIN, '_sender'),
      '#prefix' => '<div id="mailgun-domains-wrapper">',
      '#suffix' => '</div>',
    );

    $form[MAILGUN_API_ENDPOINT] = array(
      '#title' => $variables[MAILGUN_API_ENDPOINT]['title'],
      '#type' => 'select',
      '#options' => [
        'https://api.mailgun.net' => t('Default (US)'),
        'https://api.eu.mailgun.net' => t('Europe'),
      ],
      '#description' => $variables[MAILGUN_API_ENDPOINT]['description'],
      '#default_value' => variable_get(MAILGUN_API_ENDPOINT, $variables[MAILGUN_API_ENDPOINT]['default']),
      '#ajax' => [
        'wrapper' => 'mailgun-domains-wrapper',
        'callback' => 'mailgun_admin_domains_callback',
      ],
    );

    $form[MAILGUN_TEST_MODE] = array(
      '#title' => $variables[MAILGUN_TEST_MODE]['title'],
      '#type' => 'checkbox',
      '#default_value' => variable_get(MAILGUN_TEST_MODE, FALSE),
      '#description' => $variables[MAILGUN_TEST_MODE]['description'],
    );

    $form[MAILGUN_LOG_EMAILS] = array(
      '#title' => $variables[MAILGUN_LOG_EMAILS]['title'],
      '#type' => 'checkbox',
      '#description' => $variables[MAILGUN_LOG_EMAILS]['description'],
      '#default_value' => variable_get(MAILGUN_LOG_EMAILS, FALSE),
    );

    $form['extra'] = array(
      '#type' => 'fieldset',
      '#title' => t('Additional settings'),
      '#description' => t('These default settings apply to messages sent using Mailgun and may be overriden on a per-message basis.'),
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
    );

    // We have the same options for all settings.
    $options = array(
      'default' => t('Use default setting'),
      'enabled' => t('Enabled'),
      'disabled' => t('Disabled'),
    );

    $form['extra']['tracking'] = array(
      '#type' => 'fieldset',
      '#title' => t('Tracking'),
    );

    $form['extra']['tracking'][MAILGUN_TRACKING] = array(
      '#title' => $variables[MAILGUN_TRACKING]['title'],
      '#type' => 'select',
      '#options' => $options,
      '#description' => $variables[MAILGUN_TRACKING]['description'],
      '#default_value' => variable_get(MAILGUN_TRACKING, 'default'),
    );

    $form['extra']['tracking'][MAILGUN_TRACKING_CLICKS] = array(
      '#title' => $variables[MAILGUN_TRACKING_CLICKS]['title'],
      '#type' => 'select',
      '#options' => $options,
      '#description' => $variables[MAILGUN_TRACKING_CLICKS]['description'],
      '#default_value' => variable_get(MAILGUN_TRACKING_CLICKS, 'default'),
    );

    $form['extra']['tracking'][MAILGUN_TRACKING_OPENS] = array(
      '#title' => $variables[MAILGUN_TRACKING_OPENS]['title'],
      '#type' => 'select',
      '#options' => $options,
      '#description' => $variables[MAILGUN_TRACKING_OPENS]['description'],
      '#default_value' => variable_get(MAILGUN_TRACKING_OPENS, 'default'),
    );

    $formats = array('_none' => t('- None -'));
    foreach (filter_formats() as $format) {
      if ($format->format === 'php_code') {
        continue;
      }
      $formats[$format->format] = $format->name;
    }
    $form['extra'][MAILGUN_FORMAT] = array(
      '#title' => $variables[MAILGUN_FORMAT]['title'],
      '#type' => 'select',
      '#description' => $variables[MAILGUN_FORMAT]['description'],
      '#options' => $formats,
      '#default_value' => variable_get(MAILGUN_FORMAT, '_none'),
    );

    $form['extra'][MAILGUN_QUEUE] = array(
      '#title' => $variables[MAILGUN_QUEUE]['title'],
      '#type' => 'checkbox',
      '#description' => $variables[MAILGUN_QUEUE]['description'],
      '#default_value' => variable_get(MAILGUN_QUEUE, FALSE),
    );

    $form['extra'][MAILGUN_TAGGING_MAILKEY] = array(
      '#type' => 'checkbox',
      '#title' => $variables[MAILGUN_TAGGING_MAILKEY]['title'],
      '#description' => $variables[MAILGUN_TAGGING_MAILKEY]['description'],
      '#default_value' => variable_get(MAILGUN_TAGGING_MAILKEY, TRUE),
    );
  }

  $form = system_settings_form($form);
  $form['#validate'][] = 'mailgun_admin_settings_validate';

  return $form;
}

/**
 * AJAX callback for domains list.
 */
function mailgun_admin_domains_callback($form, $form_state) {
  return $form[MAILGUN_DOMAIN];
}

/**
 * Form validation handler for mailgun_admin_settings().
 *
 * Perform additional validation to ensure the API key entered is valid.
 */
function mailgun_admin_settings_validate($form, &$form_state) {
  if ($form[MAILGUN_API_KEY]['#default_value'] != $form_state['values'][MAILGUN_API_KEY]) {
    // The API key has changed. Perform validation.
    $form_state['values'][MAILGUN_API_KEY] = trim($form_state['values'][MAILGUN_API_KEY]);
    $client = mailgun_get_client($form_state['values'][MAILGUN_API_KEY]);

    if ($client === FALSE) {
      drupal_set_message(t('Could not connect to Mailgun API. Please check your settings'), 'warning');
      return;
    }

    try {
      $client->domains()->index();
      drupal_set_message(t('Your API key has been successfully validated.'));
    }
    catch (Exception $e) {
      form_set_error(MAILGUN_API_KEY, t('An exception occurred. @code: @message', array(
        '@code' => $e->getCode(),
        '@message' => $e->getMessage(),
      )));
    }
  }
}

/**
 * Form builder. Display a form for sending a test e-mail.
 */
function mailgun_test_form($form, &$form_state) {
  drupal_set_title(t('Send test mail'));

  $form['to'] = array(
    '#type' => 'textfield',
    '#title' => t('To'),
    '#default_value' => variable_get('site_mail', ''),
    '#description' => t('Type in an address to have the test email sent there.'),
    '#required' => TRUE,
  );

  $message = "Howdy!\n\nIf this e-mail is displayed correctly and delivered sound and safe, congrats! You have successfully configured Mailgun. ";
  $message .= t('Visit the !project to contribute or read !documentation to learn more.', array(
    '!project' => l(t('project page'), 'https://www.drupal.org/project/mailgun'),
    '!documentation' => l(t('documentation'), MAILGUN_DOCUMENTATION_LINK),
  ));
  $form['message'] = array(
    '#type' => 'textarea',
    '#title' => t('Message'),
    '#default_value' => $message,
    '#required' => TRUE,
  );

  $form['attachment'] = array(
    '#title' => t('Include attachment'),
    '#type' => 'checkbox',
    '#description' => t('If checked, the Drupal icon will be included as an attachment with the test e-mail.'),
    '#default_value' => TRUE,
  );

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Send'),
  );
  $form['cancel'] = array(
    '#type' => 'link',
    '#href' => MAILGUN_ADMIN_PAGE,
    '#title' => t('Cancel'),
  );

  return $form;
}

/**
 * Form submission handler for mailgun_test_form().
 *
 * Send the test e-mail.
 */
function mailgun_test_form_submit($form, &$form_state) {
  $to = $form_state['values']['to'];
  $body = explode('\n', $form_state['values']['message']);

  $params = array(
    'message' => $body,
    'attachment' => $form_state['values']['attachment'],
  );
  $site_name = variable_get('site_name', '');
  $default_from = variable_get('site_mail', ini_get('sendmail_from'));
  $from = (!empty($site_name)) ? $site_name . ' <' . $default_from . '>' : $default_from;
  $result = drupal_mail('mailgun', 'test', $to, $GLOBALS['language'], $params, $from);

  drupal_set_message(t('Test email sent from %from to %to. If you have the "Log mails" setting enabled, check the <a href="@url">database log</a> for details.',
    array(
      '%from' => $result['from'],
      '%to' => $result['to'],
      '@url' => url('admin/reports/dblog'),
    )), 'status');
}
