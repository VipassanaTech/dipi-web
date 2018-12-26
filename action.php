<?php

if ( $argc < 2)
{
   echo "Usage: php ".$argv[0]." <id> <action>\n";
   exit(1);
}

$id = $argv[1];
$action = $argv[2];

if ( !is_numeric($id) )
{
   echo "ID should be numeric\n";
   exit(1);
}

if ( !in_array($action, array("Photo", "Finalize", "PCAutoCancel", "CronExpected", "ReminderFinalize")) )
{
   echo "Invalid Action!\n";
   exit(1);
}

define('DRUPAL_ROOT', getcwd());

require_once DRUPAL_ROOT . '/includes/bootstrap.inc';
drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);


switch($action)
{
   case 'Photo':
	create_application_pdf( $id );
	break;
   case 'Finalize':
	finalize_course( $id );
   dh_patrika_add_course_students($id);
	break;
   case 'PCAutoCancel':
	cron_pc_auto_cancel();
	break;
   case 'CronExpected':
	cron_expected();
	break;
   case 'ReminderFinalize':
   reminder_finalize();
   break;
}
//update_status_external($id, $event);



?>
