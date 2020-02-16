<?php

/**
 * @file
 * Views inc file for User Created By module.
 */

/**
 * Implements hook_views_data().
 */
function user_created_by_views_data() {
  // The 'group' index will be used as a prefix in the UI for any of this
  // table's fields, sort criteria, etc. so it's easy to tell where they came
  // from.
  $data['user_created_by_data']['table']['group'] = t('User Created By');

  // This table references the {users} table. The declaration below creates an
  // 'implicit' relationship to the users table, so that when 'users' is the
  // base table, the fields are automatically available.
  $data['user_created_by_data']['table']['join'] = array(
    // Index this array by the table name to which this table refers.
    // 'left_field' is the primary key in the referenced table.
    // 'field' is the foreign key in this table.
    'users' => array(
      'left_field' => 'uid',
      'field' => 'uid',
    ),
  );

  // creator_uid table field.
  $data['user_created_by_data']['creator_uid'] = array(
    'title' => t('Creator Uid'),
    'help' => t('User Creator Uid field.'),
    'relationship' => array(
      'base' => 'users',
      'base field' => 'uid',
      'handler' => 'views_handler_relationship',
      'label' => t('Creator Uid'),
      'title' => t('Creator Uid'),
      'help' => t('Creator Uid'),
    ),
  );

  return $data;
}
