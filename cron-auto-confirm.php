<?php

if (isset($_REQUEST) && ( isset($_REQUEST['REMOTE_IP']) ))
{
   echo "I wont run from the web\n";
   exit(1);
}

define('DRUPAL_ROOT', getcwd());

require_once DRUPAL_ROOT . '/includes/bootstrap.inc';
drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);


$q = "select a_id, a_course, a_f_name, a_l_name, a_email, a_status from dh_applicant a left join dh_course c on (a.a_course=c.c_id) where c.c_start >= CURDATE() and c.c_auto_confirm=1 and a.a_status in ('Received') limit 200";
$result = db_query($q);
while ($row = $result->fetchAssoc()) 
{
   echo $row['a_id']." - ".$row['a_f_name']." ".$row['a_l_name']."\n";
   $check = "select count(a_id) from dh_applicant where a_course=".$row['a_course']." and a_f_name= :fname and a_l_name = :lname and a_email = :email and a_status in ('Confirmed', 'Expected', 'ReConfirmation')";
   $ret = db_query($check, array(':fname' => $row['a_f_name'], ':lname' => $row['a_l_name'], ':email' => $row['a_email']))->fetchField();
   try 
   {
      if ($ret <= 0)
         update_status_external($row['a_id'], 'Confirmed');
      else
      {
         update_status_external($row['a_id'], 'Rejected');
         logit($row['a_center'], 'Auto-Reject', $row['a_id'], 'Auto rejected due to duplicate applications');
      }
      
   } catch (Exception $e) 
   {
      //   
   }
}



?>
