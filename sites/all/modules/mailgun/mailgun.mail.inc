<?php

/**
 * @file
 * Implements Mailgun as a Drupal MailSystemInterface.
 */

/**
 * Modify the Drupal mail system to use Mailgun when sending e-mails.
 */
class MailgunMailSystem implements MailSystemInterface {

  /**
   * Concatenate and wrap the e-mail body for either plain-text or HTML e-mails.
   *
   * @param array $message
   *   A message array, as described in hook_mail_alter().
   *
   * @return array
   *   The formatted $message.
   */
  public function format(array $message) {
    // Join the body array into one string.
    if (is_array($message['body'])) {
      $message['body'] = implode("\n\n", $message['body']);
    }

    // Run the message through text format if specified in Mailgun settings.
    $format = variable_get('mailgun_format', '_none');
    if ($format !== '_none') {
      $message['body'] = check_markup($message['body'], $format);
    }

    // Wrap body with theme function.
    $message['body'] = theme('mailgun_message', array(
      'subject' => $message['subject'],
      'body' => $message['body'],
      'message' => $message,
    ));

    return $message;
  }

  /**
   * Send the e-mail message.
   *
   * @param array $message
   *   A message array, as described in hook_mail_alter(). $message['params']
   *   may contain additional parameters. See mailgun_send().
   *
   * @return bool
   *   TRUE if the mail was successfully accepted or queued, FALSE otherwise.
   *
   * @see drupal_mail()
   * @see https://documentation.mailgun.com/api-sending.html#sending
   */
  public function mail(array $message) {
    // Mailsystem 3.x removes the automatic loading of html_to_text.inc that
    // was included in mailsystem_init() in version 2.x. Therefore, test to
    // see if the function is available, and load it if not, before trying to
    // use it.
    if (!function_exists('mailsystem_html_to_text')) {
      module_load_include('inc', 'mailsystem', 'html_to_text');
    }
    // Build the Mailgun message array.
    $mailgun_message = array(
      'from' => $message['from'],
      'to' => $message['to'],
      'subject' => $message['subject'],
      'text' => mailsystem_html_to_text($message['body']),
      'html' => $message['body'],
    );

    // Add CC, BCC and Reply-To fields if not empty.
    $headers = array_change_key_case($message['headers']);

    if (!empty($headers['cc'])) {
      $mailgun_message['cc'] = $headers['cc'];
    }
    if (!empty($headers['bcc'])) {
      $mailgun_message['bcc'] = $headers['bcc'];
    }
    if (!empty($headers['reply-to'])) {
      $mailgun_message['h:Reply-To'] = $headers['reply-to'];
    }

    $params = array();

    // Populate default tracking settings.
    $tracking_settings = array(
      'mailgun_tracking' => 'o:tracking',
      'mailgun_tracking_clicks' => 'o:tracking-clicks',
      'mailgun_tracking_opens' => 'o:tracking-opens',
    );
    foreach ($tracking_settings as $mailgun_variable => $mailgun_parameter) {
      if (($variable_value = variable_get($mailgun_variable, 'default')) !== 'default') {
        $params[$mailgun_parameter] = $variable_value === 'enabled' ? 'yes' : 'no';
      }
    }

    // For a full list of allowed parameters,
    // see: https://documentation.mailgun.com/api-sending.html#sending.
    $allowed_params = array(
      'o:tag',
      'o:campaign',
      'o:deliverytime',
      'o:dkim',
      'o:testmode',
      'o:tracking',
      'o:tracking-clicks',
      'o:tracking-opens',
    );

    if (isset($message['params']) && is_array($message['params'])) {
      foreach ($message['params'] as $key => $value) {
        // Check if it's one of the known parameters.
        $allowed = in_array($key, $allowed_params) ? TRUE : FALSE;

        if ($allowed) {
          $params[$key] = $value;
        }
        // Check for custom MIME headers or custom JSON data.
        if (substr($key, 0, 2) == 'h:' || substr($key, 0, 2) == 'v:') {
          $params[$key] = $value;
        }

        // Add additional HIME headers, like Reply-to if it exists.
        if ($key === 'headers' && is_array($value)) {
          foreach ($value as $headers_key => $headers_value) {
            $params['h:' . $headers_key] = $headers_value;
          }
        }
      }
    }

    // Add default tags by mail key if enabled.
    if (variable_get('mailgun_tagging_mailkey', TRUE)) {
      $params['o:tag'][] = $message['id'];
    }

    // Make sure the files provided in the attachments array exist.
    if (!empty($message['params']['attachments'])) {
      $params['attachment'] = array();
      foreach ($message['params']['attachments'] as $attachment) {
        if (is_array($attachment)) {
          // `filecontent` attachment key can be used by MimeMail as data
          // of the related file.
          if (array_key_exists('filecontent', $attachment)) {
            $temp_file = tmpfile();
            fwrite($temp_file, $attachment['filecontent']);
            $temp_f_meta_data = stream_get_meta_data($temp_file);
            $_attachment = array(
              'filePath' => $temp_f_meta_data['uri'],
            );
            if (array_key_exists('filename', $attachment)) {
              $_attachment['filename'] = $_attachment['remoteName'] = $attachment['filename'];
            }
            $params['attachment'][] = $_attachment;
          }
          elseif (array_key_exists('filepath', $attachment) && file_exists($attachment['filepath'])) {
            $_attachment = array(
              'filePath' => $attachment['filepath'],
            );
            if (array_key_exists('filename', $attachment)) {
              $_attachment['filename'] = $_attachment['remoteName'] = $attachment['filename'];
            }
            $params['attachment'][] = $_attachment;
          }
        }
        elseif (file_exists($attachment)) {
          $params['attachment'][] = $attachment;
        }
      }
    }

    $mailgun_message['params'] = $params;

    // Queue the message if the setting is enabled and number of emails sent at
    // once exceeds the threshold. Threshold is tested only for
    // views_bulk_operations module. If the threshold is larger than zero only
    // views_bulk_operations emails are queued.
    if (variable_get('mailgun_queue', FALSE)) {
      $queue_threshold = variable_get(MAILGUN_QUEUE_THRESHOLD, 0);
      $queue_threshold_exceeded = FALSE;
      if ($queue_threshold == 0) {
        $queue_threshold_exceeded = TRUE;
      }
      else if (isset($message['params']['context']['progress']['total'])) {
        $queue_threshold_exceeded = $message['params']['context']['progress']['total'] > $queue_threshold;
      }

      if ($queue_threshold_exceeded) {
        $queue = DrupalQueue::get('mailgun_queue', TRUE);
        $queue->createItem($mailgun_message);
        return TRUE;
      }
    }

    return mailgun_send($mailgun_message);
  }

}
