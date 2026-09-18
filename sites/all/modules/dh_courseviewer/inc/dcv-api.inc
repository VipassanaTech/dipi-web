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

/**
 * POST dcv/login — validate Dipi credentials, establish a Drupal session.
 * Accepts form-encoded 'username' and 'password'. On success the response
 * sets the session cookie the app carries for the two download calls.
 */
function dcv_login() {
  $name = isset($_POST['username']) ? trim($_POST['username']) : '';
  $pass = isset($_POST['password']) ? $_POST['password'] : '';
  if ($name === '' || $pass === '') {
    dcv_json('FAIL', array(), 'Username and password are required.');
  }
  $uid = user_authenticate($name, $pass);
  if (!$uid) {
    dcv_json('FAIL', array(), 'Invalid username or password.');
  }
  global $user;
  $user = user_load($uid);
  $login = array('name' => $user->name, 'pass' => $pass);
  user_login_finalize($login); // sets up the session + Set-Cookie
  dcv_json('OK', array('uid' => (int) $uid, 'name' => $user->name), 'Logged in.');
}
