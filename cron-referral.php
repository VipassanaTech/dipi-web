<?php


if (isset($_SERVER) && ( isset($_SERVER['REMOTE_ADDR']) ))
{
   echo "I wont run from the web\n";
   exit(1);
}


$lock_file = "cron-referral.lock";
$f = fopen($lock_file, 'w') or die ("Cannot create/open lock file $lock_file, exiting!\n");

if (!flock($f, LOCK_EX | LOCK_NB))
{
  die ("not able to lock $lock_file, exiting!\n");
}



define('DRUPAL_ROOT', getcwd());

require_once DRUPAL_ROOT . '/includes/bootstrap.inc';
drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);


// eg for $add_options
//   array(
//     'cc' => $cc,
//     'bcc' => $bcc,
//     'v:at-app-id' => $at_app_id,
//     'attachment' => array(
//       array('filePath' => $a, 'filename' => basename($a)),
//       array('filePath' => $b, 'filename' => basename($b)),
//     ),
//   )

// send_email_generic( $to, $from_email, $from_name, $subject, $body, $add_options = array() )

$referral_from_email = variable_get('referral_from_email', '');
$referral_cc_email = variable_get('referral_cc_email', '');

if(!$referral_from_email)
  exit("Varialble referral_from_email is not set, Exiting.\n");

delete_expired_referral($referral_from_email, $referral_cc_email);
send_referral_notification($referral_from_email, $referral_cc_email);


function delete_expired_referral($referral_from_email, $referral_cc_email)
{
  $system_uids = db_query("select td_key, td_val1 from dh_type_detail where td_type = 'COURSE-APPLICANT'")->fetchAllKeyed();
  $uid = $system_uids['COURSE-SYSTEM-UID'];

  $q = "select
      r_id, s_f_name, s_l_name, t_email, t_f_name, t_l_name
      from dh_referral r
      left join dh_student s on r.r_student = s.s_id
      left join dh_teacher t on r.r_teacher = t.t_id
      where
      r_deleted=0 and
      r_end < CURRENT_DATE() and
      datediff(CURRENT_DATE(),r_email_notification_date) > 100 and
      r_email_notification_date is not null and
      r_teacher > 0 and
      r_readonly = 0";

  $res = db_query($q);

  $fields['r_updated_by'] = $uid;
  $fields['r_deleted'] = 1;

  while($r = $res->fetchAssoc())
  {
    $fields['r_updated'] = date('Y-m-d H:i:s');

    db_update('dh_referral')->fields($fields)->condition('r_id', $r['r_id'] )->execute();
    db_query("update dh_applicant set a_referral=0 where a_referral={$r['r_id']}");
    add_referral_activity($r['r_id'], "Deleted", "Referral Deleted");

    $body = "
      Dear {$r['t_f_name']} {$r['t_l_name']},<br><br>
      Referral entry for {$r['s_f_name']} {$r['s_l_name']} is deleted automatically by system, as referral period has ended for the same.<br><br><br>
      Thanks,<br>
      - Dipi Automation.
      ";

    $add_options = array();
    if($referral_cc_email)
      $add_options['cc'] = $referral_cc_email;

    $sent = send_email_generic( $r['t_email'], $referral_from_email, 'Dipi Automation', 'Referral Deletion Notification', $body, $add_options);

    if($sent['success'])
      add_referral_activity($r['r_id'], "Deleted-Email", "Deletion email notification sent to ".$r['r_to']." (".$sent['msg'].")");
    else
      add_referral_activity($r['r_id'], "Deleted-Email", "Not able to send deletion email notification to ".$r['r_to']);

    echo "Deleted Referral of {$r['s_f_name']} {$r['s_l_name']}\n";
  }
}


function send_referral_notification($referral_from_email, $referral_cc_email)
{
  $q = "update dh_referral
    set
    r_email_notification_date = NULL
    where
    r_deleted=0 and
    r_end > CURRENT_DATE() and
    datediff(CURRENT_DATE(), r_email_notification_date) > 100 and
    r_email_notification_date is not null and
    r_teacher > 0 and
    r_readonly = 0";

  db_query($q);

  $q = "select
    r_id, s_f_name, s_l_name, t_email, t_f_name, t_l_name
    from dh_referral r
    left join dh_student s on r.r_student=s.s_id
    left join dh_teacher t on r.r_teacher = t.t_id
    where
    r_deleted=0 and
    datediff(r_end, CURRENT_DATE()) <= 90 and
    r_email_notification_date is null and
    r_teacher > 0 and
    r_readonly = 0";

  $res = db_query($q);

  while($r = $res->fetchAssoc())
  {
    db_query("update dh_referral set r_email_notification_date=CURRENT_DATE() where r_id={$r['r_id']}");
    $deletion_date = date( "jS F Y", strtotime("+100 day"));
    $body = "
      Dear {$r['t_f_name']} {$r['t_l_name']},<br><br>
      Referral entry for {$r['s_f_name']} {$r['s_l_name']} will get deleted automatically by system on $deletion_date (ie. 100 days after this notification), as referral period is about to end for the same.<br><br>
      If you decide to extend the referral period, it is recommended that you take a compassionate view towards this student by not blocking the basic courses like 10 Day and Satipatthān (STP) to allow this student to mediate and overcome his/her defilements.<br><br>
      We also recommend that you personally monitor the progress of this student and decide upon his/her referral status at an appropriate time.<br><br>
      Nevertheless, if you still feel strongly otherwise, you may login to Dipi AT-Portal to modify your referral instructions or extend the referral period.<br><br><br>
      Thanks,<br>
      - DIPI Automation.
      ";

    $add_options = array();
    if($referral_cc_email)
      $add_options['cc'] = $referral_cc_email;

    $sent = send_email_generic( $r['t_email'], $referral_from_email, 'Dipi Automation', 'Referral about to expire, action needed', $body, $add_options);

    if($sent['success'])
      add_referral_activity($r['r_id'], "Notification-Email", "Notification email sent to ".$r['r_to']." (".$sent['msg'].")");
    else
      add_referral_activity($r['r_id'], "Notification-Email", "Not able to send notification email to ".$r['r_to']);

    echo "Sent Referral Notification for {$r['s_f_name']} {$r['s_l_name']}\n";
  }
}
