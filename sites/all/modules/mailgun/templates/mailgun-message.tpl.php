<?php

/**
 * @file
 * Default theme implementation of Mailgun mail.
 *
 * Available variables:
 * - $subject: The message subject.
 * - $body: The message body.
 *
 * @see template_preprocess_mailgun_message()
 */
?>
<html>
  <head>
    <style type="text/css">
      body {
        font-family: Verdana, Arial, sans-serif;
        font-size: 12px;
      }
    </style>
  </head>
  <body>
    <div>
      <?php print $body; ?>
    </div>
  </body>
</html>
