<?php
namespace ContentMigrateTweaks;

if (!defined('DRUPAL_ROOT')) {
  // works if phpunit is run from drupal root dir OR via "drush phpunit"
  define('DRUPAL_ROOT', getcwd());
}
if ( !isset( $_SERVER['REMOTE_ADDR'] ) ) {
  $_SERVER['REMOTE_ADDR'] = '127.0.0.1';
}
require_once DRUPAL_ROOT . '/includes/bootstrap.inc';
drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);
require(dirname(__FILE__) . "/../ContentMigrateTweaksUtils.php");

/*
 * @file
 *   PHPUnit Tests for MenuBlockUtils. 
 *   This uses Drush's test framework, which is based on PHPUnit.
 */
class UtilsTest extends \Drush_CommandTestCase {

  private function normalizeSQL($sql) {
    $sql = preg_replace('/\s+/', ' ', $sql);
    $sql = preg_replace('/,/', ",\n", $sql);
    return $sql;
  }

  private function getDataFixtures($name = NULL) {

    $columns = array('note_value' => 'field_course_note_value', 'note_format' => 'field_course_note_format');
    $fixtures = array();
    $fixtures['all_revisions_singlevalued'] = array(
      'query' => Utils::getDataQuery('content_field_course_note', FALSE, $columns, $columns),
      'sql'   => "SELECT n.type AS bundle,
                         n.vid AS active_revision_id,
                         old_table.nid AS entity_id,
                         old_table.vid AS revision_id,
                         old_table.field_course_note_value AS field_course_note_value,
                         old_table.field_course_note_format AS field_course_note_format,
                         0 AS delta,
                         'node' AS entity_type,
                         'und' AS language
                       FROM
                         {content_field_course_note} old_table
                       INNER JOIN {node} n ON old_table.nid=n.nid
                       ORDER BY old_table.vid ASC"
    );
    $fixtures['all_revisions_multivalued'] = array(
      'query' => Utils::getDataQuery('content_field_course_note', TRUE, $columns, $columns),
      'sql'   => "SELECT n.type AS bundle,
                         n.vid AS active_revision_id,
                         old_table.nid AS entity_id,
                         old_table.vid AS revision_id,
                         old_table.field_course_note_value AS field_course_note_value,
                         old_table.field_course_note_format AS field_course_note_format,
                         old_table.delta AS delta,
                         'node' AS entity_type,
                         'und' AS language
                       FROM
                         {content_field_course_note} old_table
                       INNER JOIN {node} n ON old_table.nid=n.nid
                       ORDER BY old_table.vid ASC"
    );
    $fixtures['active_revisions_multivalued'] = array(
      'query' => Utils::getDataQuery('content_field_course_note', TRUE, $columns, $columns, TRUE),
      'sql'   => "SELECT n.type AS bundle,
                         n.vid AS active_revision_id,
                         old_table.nid AS entity_id,
                         old_table.vid AS revision_id,
                         old_table.field_course_note_value AS field_course_note_value,
                         old_table.field_course_note_format AS field_course_note_format,
                         old_table.delta AS delta,
                         'node' AS entity_type,
                         'und' AS language
                       FROM
                         {content_field_course_note} old_table
                       INNER JOIN {node} n ON old_table.vid=n.vid
                       ORDER BY old_table.vid ASC"
    );

    if ($name) {
      return $fixtures[$name];
    } else {
      return $fixtures;
    }
  }

  private function getInsertFixtures($name = NULL) {

    $fixtures = array();

    // all_revisions_singlevalued 
    $columns = array('note_value' => 'field_course_note_value', 'note_format' => 'field_course_note_format');
    $select_query = Utils::getDataQuery('content_field_course_note', FALSE, $columns, $columns);
    $query = Utils::getInsertQuery('field_revisions_field_course_note', $select_query);
    $sql = "INSERT INTO {field_revisions_field_course_note} 
      (bundle, entity_id, revision_id, field_course_note_value, field_course_note_format, delta, entity_type, language) 
      $select_query";
    $fixtures['all_revisions_singlevalued'] = array(
      'select_query' => $select_query, 
      'query' => $query,
      'sql' => $sql,
    );

    // all_revisions_multivalued
    $columns = array('note_value' => 'field_course_note_value', 'note_format' => 'field_course_note_format');
    $select_query = Utils::getDataQuery('content_field_course_note', FALSE, $columns, $columns);
    $query = Utils::getInsertQuery('field_revisions_field_course_note', $select_query);
    $sql = "INSERT INTO {field_revisions_field_course_note}
      (bundle, entity_id, revision_id, field_course_note_value, field_course_note_format, delta, entity_type, language) 
      $select_query";
    $fixtures['all_revisions_multivalued'] = array(
      'select_query' => $select_query, 
      'query' => $query,
      'sql' => $sql,
    );

    // active_revisions_multivalued
    $columns = array('note_value' => 'field_course_note_value', 'note_format' => 'field_course_note_format');
    $select_query = Utils::getDataQuery('content_field_course_note', TRUE, $columns, $columns);
    $query = Utils::getInsertQuery('field_data_field_course_note', $select_query);
    $sql = "INSERT INTO {field_data_field_course_note}
      (bundle, entity_id, revision_id, field_course_note_value, field_course_note_format, delta, entity_type, language) 
      $select_query";
    $fixtures['active_revisions_multivalued'] = array(
      'select_query' => $select_query, 
      'query' => $query,
      'sql' => $sql,
    );

    if ($name) {
      return $fixtures[$name];
    } else {
      return $fixtures;
    }
  }

  /**
   * Generates query to pull in all field data.
   */
  public function testGetQuery() {
    foreach($this->getDataFixtures() as $test_name => $fixture) {
      $actual = $this->normalizeSQL((string) $fixture['query']);
      $expected = $this->normalizeSQL($fixture['sql']);
      $this->assertEquals($expected, $actual, "Ensure $test_name generates the right SQL");
    }
  }

  public function testGetInsertQuery() {
    foreach($this->getInsertFixtures() as $test_name => $fixture) {
      $actual = $this->normalizeSQL((string) $fixture['query']);
      $expected = $this->normalizeSQL($fixture['sql']);
      $this->assertEquals($expected, $actual, "Ensure $test_name generates the right SQL");
    }
  }

}
