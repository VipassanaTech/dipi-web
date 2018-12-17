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
