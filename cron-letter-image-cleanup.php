<?php

/**
 * Janitor cron: delete orphaned letter-body images.
 *
 * Images uploaded through the letter WYSIWYG live under
 * public://letters/images and are referenced only by <img src> URLs inside
 * dh_letter.l_body. This script removes files that no letter references any
 * more (skipping anything modified within the grace period). Schedule it the
 * same way as the other cron-*.php scripts (e.g. via scripts/cron-curl.sh).
 */

if (isset($_SERVER) && ( isset($_SERVER['REMOTE_ADDR']) ))
{
   echo "I wont run from the web\n";
   exit(1);
}


$lock_file = "cron-letter-image-cleanup.lock";
$f = fopen($lock_file, 'w') or die ("Cannot create/open lock file $lock_file, exiting!\n");

if (!flock($f, LOCK_EX | LOCK_NB))
{
  die ("not able to lock $lock_file, exiting!\n");
}



define('DRUPAL_ROOT', getcwd());

require_once DRUPAL_ROOT . '/includes/bootstrap.inc';
drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);


// Hours before an unreferenced image becomes eligible for deletion.
$grace_hours = variable_get('letter_image_cleanup_grace_hours', 24);

$stats = dh_letter_cleanup_orphan_images($grace_hours, TRUE);

echo date('Y-m-d H:i:s') . " letter-image cleanup: scanned {$stats['scanned']}, deleted {$stats['deleted']}, kept {$stats['kept']}\n";
