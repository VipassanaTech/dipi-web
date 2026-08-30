<?php

/**
 * @file
 * Configuration for find_people search.
 */

/**
 * Menu callback; set search by either username or email or both.
 */
function find_people_config_search() {
  $form['find_people_name'] = array(
    '#type' => 'checkbox',
    '#title' => t('Search by Username'),
    '#default_value' => variable_get('find_people_name', 1),
  );

  $form['find_people_email'] = array(
    '#type' => 'checkbox',
    '#title' => t('Search by E-mail'),
    '#default_value' => variable_get('find_people_email', 1),
  );
  $form['#validate'][] = 'find_people_settings_validate';
  return system_settings_form($form);
}

/**
 * Validator for the find_people_settings() form.
 */
function find_people_settings_validate($form, &$form_state) {
  if ($form_state['values']['find_people_name'] == 0 && $form_state['values']['find_people_email'] == 0) {
    form_set_error('find_people_name', t('Please select at least one search option'));
    variable_set('find_people_name', 1);
    variable_set('find_people_email', 0);
  }
}

/**
 * Clone-a-role form: pick a source role + a new name, and get a new role with
 * the same permissions, which can then be tweaked on the Permissions page.
 */
function find_people_role_clone_form($form, &$form_state) {
  $roles = user_roles(TRUE); // all roles except 'anonymous user'
  unset($roles[DRUPAL_AUTHENTICATED_RID]); // 'authenticated user' is not a real assignable role to copy

  $form['source_rid'] = array(
    '#type' => 'select',
    '#title' => t('Copy permissions from'),
    '#options' => $roles,
    '#required' => TRUE,
  );
  $form['new_name'] = array(
    '#type' => 'textfield',
    '#title' => t('New role name'),
    '#required' => TRUE,
    '#maxlength' => 64,
    '#description' => t('A new role is created with the same permissions as the selected role. Fine-tune it afterwards on the Permissions page.'),
  );
  $form['actions'] = array('#type' => 'actions');
  $form['actions']['submit'] = array('#type' => 'submit', '#value' => t('Create copy'));
  $form['actions']['cancel'] = array('#markup' => l(t('Cancel'), 'admin/people/permissions/roles'));
  return $form;
}

/**
 * Validate: require a non-empty, unique new role name.
 */
function find_people_role_clone_form_validate($form, &$form_state) {
  $name = trim($form_state['values']['new_name']);
  if ($name === '') {
    form_set_error('new_name', t('Enter a name for the new role.'));
  }
  elseif (user_role_load_by_name($name)) {
    form_set_error('new_name', t('A role named %name already exists.', array('%name' => $name)));
  }
  $form_state['values']['new_name'] = $name;
}

/**
 * Submit: create the new role and grant it the source role's permissions.
 */
function find_people_role_clone_form_submit($form, &$form_state) {
  $src_rid = $form_state['values']['source_rid'];
  $name = $form_state['values']['new_name'];
  $source = user_role_load($src_rid);

  $role = new stdClass();
  $role->name = $name;
  $role->weight = $source->weight;
  user_role_save($role); // populates $role->rid

  $perms = user_role_permissions(array($src_rid => $src_rid));
  $granted = array_keys($perms[$src_rid]);
  if (!empty($granted)) {
    user_role_grant_permissions($role->rid, $granted);
  }

  drupal_set_message(t('Created role %new as a copy of %src with @count permission(s). Adjust it on the Permissions page.', array(
    '%new' => $name,
    '%src' => $source->name,
    '@count' => count($granted),
  )));
  $form_state['redirect'] = 'admin/people/permissions/roles';
}
