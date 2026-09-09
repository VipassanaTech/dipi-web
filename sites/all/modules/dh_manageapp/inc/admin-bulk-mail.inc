<?php
// sites/all/modules/dh_manageapp/inc/admin-bulk-mail.inc
// Administrator (centre-independent) bulk email — self-contained subsystem.

function dh_admin_bulk_mail_list() { return 'TODO list'; }
function dh_admin_bulk_mail_compose($abm_id) { return 'TODO compose '.$abm_id; }
function dh_admin_bulk_mail_image_upload() { drupal_json_output(array()); }
function dh_admin_bulk_mail_attach($abm_id) { return 'TODO attach '.$abm_id; }
function dh_admin_bulk_mail_attach_delete($abm_id, $abma_id) { return 'TODO detach'; }
function dh_admin_bulk_mail_delete($abm_id) { return 'TODO delete '.$abm_id; }
function dh_admin_bulk_mail_show_log($abm_id) { return 'TODO log '.$abm_id; }
function dh_admin_bulk_mail_get_log($abm_id) { drupal_json_output(array()); }
function dh_admin_bulk_mail_unsubscribe($token) { return 'TODO unsub'; }
