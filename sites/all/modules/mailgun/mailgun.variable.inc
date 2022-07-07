<?php

/**
 * @file
 * Variable module integration.
 */

/**
 * Implements hook_variable_info().
 */
function mailgun_variable_info($options) {
  $variables[MAILGUN_API_KEY] = array(
    'title' => t('Mailgun API key', array(), $options),
    'description' => t('Get your Secret API key from the !link.', array(
      '!link' => l(t('Mailgun dashboard'), MAILGUN_DASHBOARD_LINK),
    ), $options),
    'default' => '',
    'group' => 'mailgun',
    'required' => TRUE,
  );

  $variables[MAILGUN_DOMAIN] = array(
    'title' => t('Domain', array(), $options),
    'description' => t('Mails will be sent using this domain.', array(), $options),
    'default' => '',
    'group' => 'mailgun',
  );

  $variables[MAILGUN_API_ENDPOINT] = array(
    'title' => t('Mailgun region', array(), $options),
    'description' => t('Select which Mailgun region to use.', array(), $options),
    'default' => 'https://api.mailgun.net',
    'group' => 'mailgun',
  );

  $variables[MAILGUN_TEST_MODE] = array(
    'title' => t('Test mode', array(), $options),
    'description' => t('Mailgun will accept the message but will not send it. This is useful for testing purposes.', array(), $options),
    'type' => 'boolean',
    'default' => 0,
    'group' => 'mailgun',
  );

  $variables[MAILGUN_LOG_EMAILS] = array(
    'title' => t('Log mails', array(), $options),
    'description' => t('Log all mails sent through Mailgun. Messages fail to send will be logged regardless of this setting.', array(), $options),
    'type' => 'boolean',
    'default' => 0,
    'group' => 'mailgun',
  );

  $variables[MAILGUN_TRACKING] = array(
    'title' => t('Enable tracking', array(), $options),
    'description' => t('Whether to enable event tracking by default or not. See !link for details.', array(
      '!link' => l(t('Tracking Messages'), 'https://documentation.mailgun.com/user_manual.html#tracking-messages'),
    ), $options),
    'default' => '',
    'group' => 'mailgun',
  );

  $variables[MAILGUN_TRACKING_CLICKS] = array(
    'title' => t('Enable click tracking', array(), $options),
    'description' => t('Whether to enable click tracking by default or not.', array(), $options),
    'default' => '',
    'group' => 'mailgun',
  );

  $variables[MAILGUN_TRACKING_OPENS] = array(
    'title' => t('Enable open tracking', array(), $options),
    'description' => t('Whether to enable open tracking by default or not.', array(), $options),
    'default' => '',
    'group' => 'mailgun',
  );

  $variables[MAILGUN_FORMAT] = array(
    'title' => t('Text format', array(), $options),
    'description' => t('Specify an additional text format to filter the message through before sending the email.', array(), $options),
    'default' => '',
    'group' => 'mailgun',
  );

  $variables[MAILGUN_QUEUE] = array(
    'title' => t('Queue mails', array(), $options),
    'description' => t('Mails will be queued and sent during cron runs. Useful for sending a large number of emails.', array(), $options),
    'default' => '',
    'group' => 'mailgun',
  );

  $variables[MAILGUN_QUEUE_THRESHOLD] = array(
    'title' => t('Queue threshold', array(), $options),
    'description' => t('Queue messages only if the number of sent messages exceeds this threshold. NOTE: Queueing with threshold works only for views_bulk_operations module.', array(), $options),
    'default' => 0,
    'group' => 'mailgun',
  );

  $variables[MAILGUN_TAGGING_MAILKEY] = array(
    'title' => t('Enable tags by mail key', array(), $options),
    'description' => t('See !url for details.', array(
      '!url' => l(t('Tagging'), 'https://documentation.mailgun.com/user_manual.html#tagging', array(
        'attributes' => array(
          'onclick' => "target='_blank'",
        ),
      )),
    ), $options),
    'default' => '',
    'group' => 'mailgun',
  );

  return $variables;
}

/**
 * Implements hook_variable_group_info().
 */
function mailgun_variable_group_info() {
  $groups['mailgun'] = array(
    'title' => t('Mailgun'),
    'description' => t('Mailgun configuration settings'),
    'access' => 'administer mailgun',
    'path' => array(MAILGUN_ADMIN_PAGE),
  );
  return $groups;
}
