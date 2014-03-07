content_migrate_tweaks
======================

This Drupal 7 module extends [content_migrate
submodule](https://drupal.org/node/1144136) to limit the amount of write
records `drush content-migrate-fields` does in a given run.  Potentially useful for debugging.

Usage:

```bash
# Ensure drupal_write_record is only called for the first 10 nodes
MAX=10 drush content-migrate-fields field_myfield -y

# Ensure drupal_write_record is never called.
SKIP_ALL=true drush content-migrate-fields field_myfield -y
```

If you're playing with this, FYI:

```
drush content-migrate-rollback field_myfield -y
```

## DEVNOTES

```bash
make destroy_coursecal
make run_d7
make scp_dotfiles
make ssh

##
## Some nice things to install while developing on docker
##

apt-get install -y locate mlocate ack-grep time bash-completion colormake
updatedb
cp /usr/share/php/drush/drush.complete.sh /etc/bash_completion.d/

# Fix perl locale warnings; writes to /etc/default/locale, requires re-login.
#   - http://www.turnkeylinux.org/forum/support/20090202/locales-error-perl-warning-setting-locale-failed
update-locale LANG=en_US.UTF-8 LC_ALL=C

cd /var/shared/sites/coursecal/site/
git clone git@github.com:dergachev/content_migrate_tweaks.git sites/all/modules/custom/content_migrate_tweaks
drush en -y content_migrate_tweaks
cd sites/all/modules/custom/content_migrate_tweaks

# in a second tab
make ssh
cd /var/shared/sites/coursecal/site/sites/all/modules/custom/content_migrate_tweaks/tests/integration
make orig

time colormake $(make list-fields bundle=program type=text)
#    about 1 min
time colormake $(make list-fields bundle=catalog type=text)
#    about 2 min
time colormake $(make list-fields bundle=publication_page type=text)
#    real	0m2.759s

time colormake $(make list-fields bundle=content type=text)
#    real	0m48.182s

time colormake $(make list-fields type=text)
#    real	0m40.489s

#=========================================================================================
# comment out the drupal_alter
#=========================================================================================
vim content_migrate_tweaks.drush.inc # remove drupal_alter
make rerun_all
time colormake $(make list-fields type=text)
#    real	2m11.488s
make diff
# no diff

#=========================================================================================
# remove content_migrate_tweaks_is_empty() check
#=========================================================================================
vim content_migrate_tweaks.drush.inc # remove content_migrate_tweaks_is_empty() check
make rerun_all
time colormake $(make list-fields type=text)
#    real	2m11.488s
make diff
#    about 21 fields
make diff | grep '^\+drupal.field_data'
#    +drupal.field_data_field_bio_name	2106633209
#    +drupal.field_data_field_building_departments	2098141990
#    +drupal.field_data_field_building_services	2098141990
#    +drupal.field_data_field_cell_phone	2106633209
#    +drupal.field_data_field_committee_revised_date	3723193455
#    +drupal.field_data_field_course_symbols	3753586894
#    +drupal.field_data_field_date_published	176902687
#    +drupal.field_data_field_extended_address	2106633209
#    +drupal.field_data_field_long_title	3266126306
#    +drupal.field_data_field_modified_date	2245265155
#    +drupal.field_data_field_organization_name	2106633209
#    +drupal.field_data_field_person_name	2106633209
#    +drupal.field_data_field_postcode	2108564747
#    +drupal.field_data_field_price	2106633209
#    +drupal.field_data_field_program_revised_date	3663353687
#    +drupal.field_data_field_revision	2567088992
#    +drupal.field_data_field_set_revised_date	3678773597
#    +drupal.field_data_field_set_title	1538336610
#    +drupal.field_data_field_short_title	1891126288
#    +drupal.field_data_field_sort_title	2423736993
#    +drupal.field_data_field_work_phone	2106633209

make diff-dump | grep '^\-'
#    nothing removed
make diff-dump | grep '^\+'
#    shows all additions relate to fields with value column of "" or NULL (this is expected)

cp dump-current.sql dump-no-empty-check.sql
cp checksum-current.txt checksum-no-empty-check.sql

#=========================================================================================
# now started to use my ::getInsertQuery()
#=========================================================================================
vim content_migrate_tweaks.drush.inc # remove content_migrate_tweaks_is_empty() check
rm migrate-field-optimized-field_course_title.txt
time colormake field_course_title
#    1 second

make diff
#     no diff

time colormake $(make list-fields bundle=program type=text)
#    about 6.754s
make diff
#    same diff as with empty case
make diff-dump
#    same diff as with empty case

time colormake $(make list-fields type=text)
#    real	0m29.123s

make diff | grep '^\+drupal.field_data'
#    same diff as no-empty-check

make diff-dump
#    same diff as no-empty-check

git diff --no-index dump-no-empty-check.sql dump-current.sql
#    no diff


#=========================================================================================
# now started to using ::addEmptyCheck() for text
#=========================================================================================

make rerun_all
time colormake $(make list-fields type=text)
#    real	0m36.288s
make diff
#    no diff


#=========================================================================================
# now do it for type=text_long
#=========================================================================================

time colormake $(make list-fields type=text_long)
#    real	0m9.851s

make diff
#    looks like empty related problems


make diff | grep '^\+drupal.field_data'
#    +drupal.field_data_field_address	3284144410
#    +drupal.field_data_field_annotation	4107257415
#    +drupal.field_data_field_begin_text	3174661062
#    +drupal.field_data_field_bio	2106633209
#    +drupal.field_data_field_calendar_description	4116978964
#    +drupal.field_data_field_course_text_narrative	4071848714
#    +drupal.field_data_field_directions	3905793673
#    +drupal.field_data_field_end_text	3582762385
#    +drupal.field_data_field_faculty_address	140803992

make diff-dump | grep -v '^@@' | grep -v 'NULL,NULL.;$'
#    nothing left; proves that its all empty related problems

# apply the same empty check logic for type=text_long 

make rerun_all
time colormake $(make list-fields type=text_long)
#    real	0m9.773s

make diff
#    -drupal.field_data_field_address        661473935
#    +drupal.field_data_field_address        2745529213
#    -drupal.field_revision_field_address    521978488
#    +drupal.field_revision_field_address    55525659

make diff-dump 
#    All differences look like this:
#      -INSERT INTO `field_revision_field_address` VALUES ('node','building',0,537,768,'und',0,'    ',NULL);
#    This is due to SQL treating blank (all spaces) strings the same as empty.
#    On the other hand, drupal's text_field_is_empty thinks they're legit values and keeps them there.
#    I was able to work-around this "bug" by changing my empty check to use "NOT LIKE" instead of "NOT EQUAL"
#    See http://stackoverflow.com/questions/7455147/comparing-strings-with-one-having-empty-spaces-before-while-the-other-does-not

make rerun_all
time colormake $(make list-fields type=text_long) $(make list-fields type=text)
#    real	0m46.071s

make diff
#    no diff
make diff-dump
#    no diff

#=========================================================================================
# now do it for type=number_float, type=number_integer
#=========================================================================================

# first ensure that empty detection is supports those types, then

time colormake rerun_all $(make list-fields type=number_float)
#    real	0m1.061s
make diff
#    no diff

time colormake rerun_all $(make list-fields type=number_integer)
#    real	0m5.422s
make diff
#    no diff

#=========================================================================================
# now do it for type=list_text
#=========================================================================================

time colormake rerun_all $(make list-fields type=list_text)
#    real	0m12.175s

make diff | grep '^\+drupal.field_data'
#    no diff

#=========================================================================================
# now do it for type=list_boolean
#=========================================================================================

time colormake rerun_all $(make list-fields type=list_boolean)
#    Received the following error:
#        WD php: PDOException: SQLSTATE[HY000]: 
#        General error: 1366 
#        Incorrect [error] integer value: 'no' for column 'field_all_day_value' at row 1: 
#          INSERT INTO {field_data_field_all_day} 
#            (bundle, entity_id, revision_id, field_all_day_value, delta, entity_type, language) 
#          SELECT 
#            n.type AS bundle, old_table.nid AS entity_id, old_table.vid AS revision_id, 
#            old_table.field_all_day_value AS field_all_day_value, 0 AS delta, 'node' AS entity_type, 
#            'und' AS language FROM {content_type_channel_event} old_table 
#          INNER JOIN {node} n ON old_table.vid=n.vid 
#          ORDER BY old_table.vid ASC;
#    Note that this should be handled by  text_content_migrate_data_record_alter but that code is clearly buggy ($results)??

make show-fields type=list_boolean
#    field_all_day	list_boolean	channel_event
#    field_distributed	list_boolean	channel_event
#    field_distributed	list_boolean	channel_news
#    field_location_banner	list_boolean	channel_event
#    field_official	list_boolean	channel_event
#    field_official	list_boolean	channel_news

# None of these fields are significant, so I have decided to omit optimizing this type.
# I am adding an check for this in the PHP code.

#=========================================================================================
# Conclusion:
#   The following supported types have clean diffs:
#     * list_boolean list_text number_float number_integer text text_long
#   The following types will probably never work:
#     * entityreference file image link_field
#   The following types might work but haven't been tested:
#     * text_with_summary list_boolean
#=========================================================================================
```
