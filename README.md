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
