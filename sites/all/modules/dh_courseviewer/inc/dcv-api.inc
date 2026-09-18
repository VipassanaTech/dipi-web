<?php
/**
 * @file
 * DCV API helpers + login/courses endpoints.
 */

/** Build the standard response envelope (pure — unit tested). */
function dcv_envelope($status, $data = array(), $msg = '') {
  return array('status' => $status, 'msg' => $msg, 'data' => $data);
}

/** Emit the envelope as JSON and stop. */
function dcv_json($status, $data = array(), $msg = '') {
  drupal_json_output(dcv_envelope($status, $data, $msg));
  exit;
}

/** GET dcv/ping — liveness check. */
function dcv_ping() {
  dcv_json('OK', array('pong' => true), 'dcv');
}
