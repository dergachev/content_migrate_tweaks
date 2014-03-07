<?php

/**
 * @file content_migrate_tweaks.drush.inc
 * Drush commands for content_migrate_tweaks,
 */

include_once(dirname(__FILE__) . '/ContentMigrateTweaksUtils.php');

define("CONTENT_MIGRATE_TWEAKS_SUPPORTED_TYPES", "list_text number_float number_integer text text_long");

/**
 * Implementation of hook_drush_help().
 */
function content_migrate_tweaks_drush_help($section) {
  switch ($section) {
    case 'content migrate tweaks:content-migrate-field-data-optimized':
      return dt("Migrate a single field data. Optimized version of content-migrate-data.");
    case 'content migrate tweaks:content-migrate-field-optimized':
      return dt('Runs content-migrate-rollback, content-migrate-field-structure, content-migrate-field-data-optimized for a single field.');
  }
}

/**
 * Implementation of hook_drush_command().
 */
function content_migrate_tweaks_drush_command() {
  $items['content-migrate-field-optimized'] = array(
    'description' => 'Runs content-migrate-rollback, content-migrate-field-structure, content-migrate-field-data-optimized for a single field.',
    'arguments' => array(
      'name' => 'A field system name.',
    ),
  );
  $items['content-migrate-field-data-optimized'] = array(
    'description' => 'Migrate a single field data. Optimized version of content-migrate-field-data.',
    'arguments' => array(
      'name' => 'A field system name.',
    ),
  );
  return $items;
}

/**
 * Batch operation callback to migrate data.Copy old table data to new field table.
 * Modified version of _content_migrate_batch_process_migrate_data.
 *
 * Command callback.
 */
function drush_content_migrate_tweaks_content_migrate_field_data_optimized($field_name) {
  drush_log(dt('Migrating data for !field', array('!field' => $field_name)), 'status');

  $field_value = content_migrate_get_field_values($field_name);
  if (isset($field_value['messages'])) {
    unset($field_value['messages']);
  }
  $instance_values = content_migrate_get_instance_values(NULL, $field_name);
  if (isset($instance_values['messages'])) {
    unset($instance_values['messages']);
  }
  $types = array();
  foreach ($instance_values as $bundle => $instance_value) {
    $types[] = $bundle;
  }
  $field = field_info_field($field_name);

  if (!in_array($field['type'], explode(" ", CONTENT_MIGRATE_TWEAKS_SUPPORTED_TYPES))) {
    throw new Exception("content_migrate_tweaks doesn't support this field type: ${field['type']}.");
  }

  $old_table          = content_migrate_old_table($field_value, $instance_value);
  $old_cols           = content_migrate_old_columns($field_value, $instance_value);
  $new_table          = content_migrate_new_table($field);
  $new_revision_table = content_migrate_new_revision($field);
  $new_columns        = content_migrate_new_columns($field);
  // Shared, non-multiple fields do not have a delta but are still in per-field tables.
  $add_delta          = $field_value['cardinality'] != 1 && content_migrate_storage_type($field_value, $instance_value) == CONTENT_DB_STORAGE_PER_FIELD;

  $data_select_query = ContentMigrateTweaks\Utils::getDataQuery($old_table, $add_delta, $old_cols, $new_columns, $_active_only = TRUE);
  $data_insert_query = ContentMigrateTweaks\Utils::getInsertQuery($new_table, $data_select_query, $field);
  $data_insert_query->execute();

  $revision_select_query = ContentMigrateTweaks\Utils::getDataQuery($old_table, $add_delta, $old_cols, $new_columns, $_active_only = FALSE);
  $revision_insert_query = ContentMigrateTweaks\Utils::getInsertQuery($new_revision_table, $revision_select_query, $field);
  $revision_insert_query->execute();

}

/**
 * Command callback.
 */
function drush_content_migrate_tweaks_content_migrate_field_optimized($field_name) {
  drush_invoke_process('@self', 'content-migrate-rollback', array($field_name));
  drush_invoke_process('@self', 'content-migrate-field-structure', array($field_name));
  drush_invoke_process('@self', 'content-migrate-field-data-optimized', array($field_name));
  _drush_log_drupal_messages();
  drush_log(dt('Migration complete'), 'status');
}


function content_migrate_tweaks_is_empty($record, $field, $new_columns) {
  if (empty($record)) {
    return TRUE;
  }
  $function = $field['module'] . '_field_is_empty';
  if (function_exists($function)) {
    // The $record array has the database columns as keys, which drupal_write_record() will need,
    // but the _field_is_empty() function will be looking for the short, normalized column name.
    $item = array();
    foreach ($new_columns as $column_name => $db_column_name) {
      if (array_key_exists($db_column_name, $record)) {
        $item[$column_name] = $record[$db_column_name];
      }
    }
    return $function($item, $field);
  }
  else {
    return FALSE;
  }
}
