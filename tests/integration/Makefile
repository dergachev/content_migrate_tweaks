drush_sqlq := `drush sql-connect` -B --skip-column-names -e
type =
bundle =
ifneq ($(type),)
type_where = AND fc.type = '$(type)'
endif
ifneq ($(bundle),)
bundle_where = AND fci.bundle = '$(bundle)'
endif

list-field-types:
	@$(drush_sqlq) "SELECT distinct(type) FROM field_config"

list-bundles:
	@$(drush_sqlq) "SELECT distinct(bundle) FROM field_config_instance"

list-fields:
	@$(drush_sqlq) " \
		SELECT fc.field_name \
		FROM field_config_instance fci \
		INNER JOIN field_config fc \
		  ON fc.field_name = fci.field_name \
		WHERE \
		  fc.field_name NOT IN ('comment_body', 'body') \
		AND \
		  fc.type IN ('list_text' 'number_float', 'number_integer', 'text', 'text_long') \
		  $(type_where) $(bundle_where) \
	  GROUP BY fc.field_name;"

show-fields:
	@$(drush_sqlq) " \
		SELECT fc.field_name, fc.type, fci.bundle \
		FROM field_config_instance fci \
		INNER JOIN field_config fc \
		  ON fc.field_name = fci.field_name \
		WHERE \
		  fc.field_name NOT IN ('comment_body', 'body') \
		AND \
		  fc.type IN ('list_text' 'number_float', 'number_integer', 'text', 'text_long') \
		  $(type_where) $(bundle_where) \
	  ;"

list-tables.txt:
	$(drush_sqlq) "SELECT TABLE_NAME name FROM INFORMATION_SCHEMA.TABLES AS t WHERE TABLE_NAME LIKE 'field_%' AND TABLE_SCHEMA='drupal';" > $@

# check ensures that even "make -B" doesn't obliterate the checksum
checksum-orig.txt: list-tables.txt
	test -e $@ || (cat list-tables.txt | xargs -I {} $(drush_sqlq) "checksum table {}" > $@)

dump-orig.sql: list-tables.txt
	test -e $@ || (drush sql-dump --ordered-dump --tables-list=$$(cat list-tables.txt | paste -s -d, -) > $@)

dump-current.sql: list-tables.txt migrate-field-optimized-*.txt
	drush sql-dump --ordered-dump --tables-list=$$(cat list-tables.txt | paste -s -d, -) > $@

# NOTE: The dependencies here are ALL existing migrate-field-optimized-*.txt files
#       This reruns the optimized stuff on anything that's been touched, ever.
checksum-current.txt: list-tables.txt migrate-field-optimized-*.txt
	cat list-tables.txt | xargs -I {} $(drush_sqlq) "checksum table {}" > $@

diff: checksum-orig.txt checksum-current.txt
	-git diff -U0 --no-index checksum-orig.txt checksum-current.txt

diff-dump: dump-orig.sql dump-current.sql
	-git diff -U0 --no-index dump-orig.sql dump-current.sql

# necessary only because 'drush content-migrate-field-data' might get exception and trigger rollback
# TODO: ($(drush_sqlq) "describe field_data_$*" > /dev/null) || $(MAKE) structure-$*
ensure-structure-exists-%:
	($(drush_sqlq) "describe field_data_$*" > /dev/null) || exit 1

migrate-field-orig-%.txt: ensure-structure-exists-%
	# Migrating $@
	$(drush_sqlq) "TRUNCATE TABLE field_data_$*;  TRUNCATE TABLE field_revision_$*;"
	time -p drush content-migrate-field-data $* -y 2>&1
	touch $@
	-rm migrate-field-optimized-$*.txt

migrate-field-optimized-%.txt: checksum-orig.txt
	# Migrating $*
	$(drush_sqlq) "TRUNCATE TABLE field_data_$*;  TRUNCATE TABLE field_revision_$*;"
	time -p drush content-migrate-field-data-optimized $* -y 2>&1
	touch $@

structure-%:
	-rm migrate-field-optimized-$*.txt
	-drush content-migrate-rollback $* -y
	drush content-migrate-field-structure $* -y

restore =
field_%:
	# Running 'make $@'
ifeq ($(restore),true)
	$(MAKE) migrate-field-orig-$@.txt
else
	$(MAKE) migrate-field-optimized-$@.txt
endif

reload_orig:
	time cat list-tables.txt | xargs -I{} drush sqlq "truncate table {};"
	time cat dump-orig.sql | drush sqlc

rerun_all:
	-rm migrate-field-optimized-field_*

orig: checksum-orig.txt dump-orig.sql
