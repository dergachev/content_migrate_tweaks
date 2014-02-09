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
    $query->addField('n', 'vid', 'active_revision_id');
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

  public static function getInsertQuery($new_table, $select_query) {
    $fields = $select_query->getFields();
    unset($fields['active_revision_id']);
    $expressions = $select_query->getExpressions();
    $new_columns = array_merge(array_keys($fields), array_keys($expressions));
    return db_insert($new_table)
             ->fields(array_values($new_columns))
             ->from($select_query);
  }

}
