<?php

namespace ContentMigrateTweaks;

class Utils {

  public static function getDataQuery($old_table, $has_delta_column, $old_columns, $new_columns, $active_only = false) {
    $query = \db_select($old_table, 'old_table', array('fetch' => \PDO::FETCH_ASSOC));

    if ($active_only) {
      $query->join('node', 'n', 'old_table.vid=n.vid');
    }
    else {
      $query->join('node', 'n', 'old_table.nid=n.nid');
    }

    $query->addField('n', 'type', 'bundle');
    $query->addField('old_table', 'nid', 'entity_id');
    $query->addField('old_table', 'vid', 'revision_id');
    foreach ($old_columns as $column_name => $db_column_name) {
      $query->addField('old_table', $db_column_name, $new_columns[$column_name]);
    }
    // NOTE: insertion order matters, or tests will fail.
    // specifically delta needs to sit in the middle to be either the last field, or the first expression.
    if ($has_delta_column) {
      $query->addField('old_table', 'delta', 'delta');
    }
    else {
      $query->addExpression("0", 'delta');
    }
    $query->addExpression("'node'", 'entity_type');
    $query->addExpression("'und'", 'language');
    $query->orderBy('old_table.vid', 'ASC');

    return $query;
  }

  /**
   * Ensure that the select query omits null or empty values.
   *
   * @param $query - the query to modify
   * @param $field - as returned by field_info_field()
   * @return @void
   *
   * @see _field_is_empty check in _content_migrate_batch_process_migrate_data().
   */
  public static function addEmptyCheck(&$query, $field) {
    //TODO: figure out if its OK to leave the newly introduced $format_column (only in D7 for text fields) as NULL
    // However this seems to be consistent content-migrate-field-data, so OK for now.
    switch($field['type']) {
    case "text":
    case "text_long":
    case "number_float":
    case "number_integer":
    case "list_text":
      $field_name = $field['field_name'];
      $value_column = "old_table.${field_name}_value";
      $query->isNotNull($value_column);
      // NB: The condition must be "NOT LIKE" and not "NOT EQUALS" because in mysql,
      // "SELECT '' EQUALS '   '" and "SELECT 0 EQUALS '   '" are both true, while
      // "SELECT '' LIKE 0" behaves as expected (eg. strict comparison).
      $query->condition($value_column, '', 'NOT LIKE');
      break;
    case "node_reference":
      $field_name = $field['field_name'];
      $value_column = "old_table.${field_name}_nid";
      $query->isNotNull($value_column);
      $query->condition($value_column, '', 'NOT LIKE');
      break;
    }
  }

  public static function getInsertQuery($new_table, $select_query, $field) {
    self::addEmptyCheck($select_query, $field);
    $fields = $select_query->getFields();
    $expressions = $select_query->getExpressions();
    $new_columns = array_merge(array_keys($fields), array_keys($expressions));
    return db_insert($new_table)
             ->fields(array_values($new_columns))
             ->from($select_query);
  }

}
