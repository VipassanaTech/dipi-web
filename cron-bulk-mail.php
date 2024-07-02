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

$q = "select bm_center ,bm_id, bm_query, bm_letter from dh_bulk_mail where DATE_FORMAT(bm_schedule_date, '%Y-%m-%d') = CURDATE() and bm_processed=0 and bm_deleted=0";
$result = db_query($q);
$bm_limit = variable_get('bulk_email_limit_per_month', 0);
while ($row = $result->fetchAssoc()) 
{
   echo "Processing Bulk Mail ID ".$row['bm_id']."\n";

   $bm_updated_by = db_query("select td_val1 from dh_type_detail where td_type = 'COURSE-APPLICANT' and td_key='COURSE-SYSTEM-UID'")->fetchField();
   $bm_updated = date('Y-m-d H:i:s');
   $center_count = 0;
   $center_count = db_query("select COALESCE(SUM(bm_count),0) FROM dh_bulk_mail where MONTH(bm_schedule_date) = MONTH(CURRENT_DATE()) AND YEAR(bm_schedule_date) = YEAR(CURRENT_DATE()) and bm_center='{$row['bm_center']}' and bm_processed in (1, 2);")->fetchField();

   if($center_count > $bm_limit)
   {
      echo "Failed! ".$row['bm_id'].", CenterLimit $center_count crossed BMLimit $bm_limit\n";
      $q = "update dh_bulk_mail set bm_processed=4, bm_updated='$bm_updated', bm_updated_by='$bm_updated_by' where bm_id=".$row['bm_id'];
   }
   else
   {
     $q = "update dh_bulk_mail set bm_processed=2, bm_updated='$bm_updated', bm_updated_by='$bm_updated_by' where bm_id=".$row['bm_id'];
     db_query($q);
     $result2 = db_query($row['bm_query']);
     while ($row2 = $result2->fetchAssoc())
     {
        echo "Pushing ".$row2['id']."\n";
        $data = array('bulk_mail_id' => $row['bm_id'], 'applicant_id' => $row2['id'], 'letter_id' => $row['bm_letter'], 'completed' => 0);
        push_to_queue( 'Dipi', 'mail', json_encode( $data ), 86400000);
     }
     $data = array('bulk_mail_id' => $row['bm_id'], 'completed' => 1);
      push_to_queue( 'Dipi', 'mail', json_encode( $data ), 86400000);
   }

}
