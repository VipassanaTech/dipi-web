<?php
// Enqueue due administrator bulk emails. CLI only.
if (isset($_SERVER) && isset($_SERVER['REMOTE_ADDR'])) { echo "I wont run from the web\n"; exit(1); }
$lock_file = "cron-admin-bulk-mail.lock";
$f = fopen($lock_file, 'w') or die("Cannot create/open lock file $lock_file, exiting!\n");
if (!flock($f, LOCK_EX | LOCK_NB)) { die("not able to lock $lock_file, exiting!\n"); }

define('DRUPAL_ROOT', getcwd());
require_once DRUPAL_ROOT . '/includes/bootstrap.inc';
drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);

$rows = db_query("select abm_id, abm_query from dh_admin_bulk_mail where DATE(abm_schedule_date)=CURDATE() and abm_processed=1 and abm_deleted=0");
foreach ($rows as $row) {
  echo "Processing Admin Bulk Mail ID " . $row->abm_id . "\n";
  db_query("update dh_admin_bulk_mail set abm_processed=2, abm_updated=:u where abm_id=:id", array(':u' => date('Y-m-d H:i:s'), ':id' => $row->abm_id));
  $abm_id = $row->abm_id;
  dh_bulk_mail_enqueue(
    $row->abm_query,
    function ($app_id) use ($abm_id) {
      $a = db_query("select concat(ifnull(a_f_name,''),' ',ifnull(a_l_name,'')) name, a_email, a_center from dh_applicant where a_id=:id", array(':id' => $app_id))->fetchAssoc();
      return array('type' => 'admin', 'abm_id' => $abm_id, 'app_id' => $app_id, 'email' => $a['a_email'], 'name' => $a['name'], 'center' => $a['a_center']);
    },
    array('type' => 'admin', 'abm_id' => $abm_id, 'completed' => 1)
  );
  watchdog('AdminBulkMail', 'Enqueued blast @id', array('@id' => $abm_id), WATCHDOG_INFO);
}
