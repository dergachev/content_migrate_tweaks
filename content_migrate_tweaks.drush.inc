<?php

# include_once(dirname(__FILE__) . '/../modules/content_migrate.file.inc');
# include_once(dirname(__FILE__) . '/../modules/content_migrate.number.inc');
# include_once(dirname(__FILE__) . '/../modules/content_migrate.options.inc');
# include_once(dirname(__FILE__) . '/../modules/content_migrate.text.inc');

/**
 * @file content_migrate_tweaks.drush.inc
 * Drush commands for content_migrate_tweaks,
 */

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
 * Command callback.
 */
function drush_content_migrate_tweaks_content_migrate_field_data_optimized($field_name) {

  drush_log(dt('Migrating data for !field', array('!field' => $field_name)), 'status');
  $batch = array(
    'title' => t('Migrating data'),
    //'file'  => drupal_get_path('module', 'content_migrate_tweaks') . '/includes/content_migrate.admin.inc',
    'operations' => array(array('_content_migrate_tweaks_batch_process_migrate_data', array($field_name))),
    'finished' => "Field migration is finished",
    'init_message' => t("Fields migration is starting."),
    'progress_message' => t('Processed @current out of @total.'),
    'error_message' => t('Field migration has encountered an error.'),
  );
  batch_set($batch);
  $batch =& batch_get();
  $batch['progressive'] = FALSE;
  drush_backend_batch_process();
}

/**
 * Batch operation callback to migrate data.
 * Copy old table data to new field table.
 *
 * Modified version of _content_migrate_batch_process_migrate_data.
 */
function _content_migrate_tweaks_batch_process_migrate_data($field_name, &$context) {

module_load_include('inc', 'content_migrate', 'modules/content_migrate.file');
module_load_include('inc', 'content_migrate', 'modules/content_migrate.number');
module_load_include('inc', 'content_migrate', 'modules/content_migrate.options');
module_load_include('inc', 'content_migrate', 'modules/content_migrate.text');
module_load_include('php', 'content_migrate_tweaks', 'ContentMigrateTweaksUtils');

  // The first time through, find all the nodes that have this field.
  if (!isset($context['sandbox']['progress'])) {

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
    $old_table          = content_migrate_old_table($field_value, $instance_value);
    $old_cols           = content_migrate_old_columns($field_value, $instance_value);
    $new_table          = content_migrate_new_table($field);
    $new_revision_table = content_migrate_new_revision($field);
    $new_columns        = content_migrate_new_columns($field);
    // Shared, non-multiple fields do not have a delta but are still in per-field tables.
    $add_delta          = $field_value['cardinality'] != 1 && content_migrate_storage_type($field_value, $instance_value) == CONTENT_DB_STORAGE_PER_FIELD;

    $results = ContentMigrateTweaks\Utils::getDataQuery($old_table, $add_delta, $old_cols, $new_columns)
                 ->execute();

    foreach ($results as $row) {
      $rows[] = $row;
    }

    $sand = array();
    $sand['progress'] = 0;
    $sand['max'] = count($rows);
    $sand['rows'] = $rows;
    $sand['old_table'] = $old_table;
    $sand['new_table'] = $new_table;
    $sand['new_revision_table'] = $new_revision_table;
    $sand['old_cols'] = $old_cols;
    $sand['new_cols'] = $new_columns;
    $sand['types'] = $types;
    $sand['field'] = $field;
    $sand['add_delta'] = $add_delta;
  }

  // Process 100 nodes in each batch.
  $field = field_info_field($field_name);

  // can kill this, already included on top
  module_load_include('inc', 'content_migrate', 'modules/content_migrate.' . $field['module']);

  for ($i = 0; $i < count($sand['rows']); $i++) {
    // We found that `$record = array_shift($sand['rows']);` was very slow since it's O(n^2).
    // see http://kb.ucla.edu/articles/performance-of-array_shift-and-array_pop-in-php
    $record = $sand['rows'][$i];
    $instance = field_info_instance('node', $field_name, $record['bundle']);

      try {

        // Let modules alter this before the insert.
        drupal_alter('content_migrate_data_record', $record, $field, $instance);

        // Don't save empty values.
        if (!empty($record)) {
          $function = $field['module'] . '_field_is_empty';
          if (function_exists($function)) {
            // The $record array has the database columns as keys, which drupal_write_record() will need,
            // but the _field_is_empty() function will be looking for the short, normalized column name.
            $item = array();
            foreach ($sand['new_cols'] as $column_name => $db_column_name) {
              if (array_key_exists($db_column_name, $record)) {
                $item[$column_name] = $record[$db_column_name];
              }
            }
            if ($function($item, $field)) {
              $record = NULL;
            }
          }
        }

        if (!empty($record)) {
          $active_revision_id = $record['active_revision_id'];
          unset($record['active_revision_id']);
          if ($record['revision_id'] == $active_revision_id) {
            drupal_write_record($sand['new_table'], $record);
          }
          drupal_write_record($sand['new_revision_table'], $record);
        }
      }
      catch (Exception $e) {
      die("CAUGHT EXCEPTION $e");
        // An error has occurred trying to migrate a record of this field.
        // Set this field to be rolledback and display a detailed error message.
        $sand['rollback'] = TRUE;
        $exception = t('<p>Requesting rollback of field "@field" due to failure to convert record:</p>' .
            '<p>@record</p>  <p>Cause:</p> <p>@cause</p>',
            array('@field' => $field_name, '@record' => var_export($record, TRUE), '@cause' => $e));
        drupal_set_message($exception, 'error');
      }

      // Update our progress information.
      $sand['progress']++;
      if($sand['progress'] % 1000 == 0) {
        drush_log(t('Progress: %done / %total', array('%done' => $sand['progress'], '%total' => $sand['max'])));
      }
    }
    $context['finished'] = true;

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

