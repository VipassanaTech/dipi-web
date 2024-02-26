<?php


if (isset($_SERVER) && ( isset($_SERVER['REMOTE_ADDR']) ))
{
   echo "I wont run from the web\n";
   exit(1);
}


$lock_file = "cron-bulk-mail.lock";
$f = fopen($lock_file, 'w') or die ("Cannot create/open lock file $lock_file, exiting!\n");

if (!flock($f, LOCK_EX | LOCK_NB))
{
  die ("not able to lock $lock_file, exiting!\n");
}



define('DRUPAL_ROOT', getcwd());

require_once DRUPAL_ROOT . '/includes/bootstrap.inc';
drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);

$q = "select bm_id, bm_query, bm_letter from dh_bulk_mail where DATE_FORMAT(bm_schedule_date, '%Y-%m-%d') = CURDATE() and bm_processed=0";
$result = db_query($q);
while ($row = $result->fetchAssoc()) 
{
   echo "Processing Bulk Mail ID ".$row['bm_id']."\n";
   $result2 = db_query($row['bm_query']);
   while ($row2 = $result2->fetchAssoc()) 
   {
      echo "Pushing ".$row2['a_id']."\n";
      $data = array('applicant_id' => $row2['a_id'], 'letter_id' => $row['bm_letter']);
      push_to_queue( 'Dipi', 'mail', json_encode( $data ), 86400000);
   }
   $q = "update dh_bulk_mail set bm_processed=1 where bm_id=".$row['bm_id'];
   db_query($q);

}
