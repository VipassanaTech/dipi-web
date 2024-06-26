<?php

if ( $argc < 2)
{
   echo "Usage: php ".$argv[0]." <application-id> <Event>\n";
   exit(1);
}

$id = $argv[1];
$event = $argv[2];

if ( !is_numeric($id) )
{
   echo "Application ID should be numeric\n";
   exit(1);
}

if ( !in_array($event, array('Clarification-Response', 'Expected', 'Confirmed', 'Cancelled', 'Received', 'WaitList', 'R-ATReview', 'A-ATReview', 'Rejected-R-AT', 'Rejected-A-AT', 'R-ATTransfer')) )
{
   echo "Invalid Event!\n";
   exit(1);
}

define('DRUPAL_ROOT', getcwd());

require_once DRUPAL_ROOT . '/includes/bootstrap.inc';
drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);

watchdog('LALA', "App id - $id, Event = $event");
update_status_external($id, $event);


?>
