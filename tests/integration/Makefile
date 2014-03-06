drush_sqlq := `drush sql-connect` -B --skip-column-names -e
field_type = text

list-fields:
	@$(drush_sqlq) "SELECT field_name, type FROM field_config"

list-field-types:
	@$(drush_sqlq) "SELECT distinct(type) FROM field_config"

list-fields-by-type-%:
	@$(drush_sqlq) "SELECT field_name FROM field_config WHERE type = '$*'"

list-fields-by-bundle-%:
ifeq ($(field_type),)
	@$(drush_sqlq) "SELECT distinct(field_name) FROM field_config_instance WHERE bundle = '$*';"
else
	@$(drush_sqlq) "SELECT distinct(field_name) FROM field_config_instance WHERE bundle = '$*';"
endif

list-tables.txt:
	$(drush_sqlq) "SELECT TABLE_NAME name FROM INFORMATION_SCHEMA.TABLES AS t WHERE TABLE_NAME LIKE 'field_%' AND TABLE_SCHEMA='drupal';" > $@

# check ensures that even "make -B" doesn't obliterate the checksum
checksum-orig.txt: list-tables.txt
	test -e $@ || (cat list-tables.txt | xargs -I {} $(drush_sqlq) "checksum table {}" > $@)

# NOTE: The dependencies here are ALL existing migrate-field-optimized-*.txt files
#       This reruns the optimized stuff on anything that's been touched, ever.
checksum-current.txt: list-tables.txt migrate-field-optimized-*.txt
	cat list-tables.txt | xargs -I {} $(drush_sqlq) "checksum table {}" > $@

diff: checksum-orig.txt checksum-current.txt
	git diff --no-index --color checksum-orig.txt checksum-current.txt

# necessary only because 'drush content-migrate-field-data' might get exception and trigger rollback
# TODO: ($(drush_sqlq) "describe field_data_$*" > /dev/null) || $(MAKE) structure-$*
ensure-structure-exists-%:
	($(drush_sqlq) "describe field_data_$*" > /dev/null) || exit 1

migrate-field-orig-%.txt: ensure-structure-exists-%
	# Migrating $@
	$(drush_sqlq) "TRUNCATE TABLE field_data_$*;  TRUNCATE TABLE field_revision_$*;"
	time -p drush content-migrate-field-data $* -y 2>&1 | tee $@
	rm migrate-field-optimized-$*.txt

migrate-field-optimized-%.txt:
	$(MAKE) ensure-structure-exists-$*
	$(drush_sqlq) "TRUNCATE TABLE field_data_$*;  TRUNCATE TABLE field_revision_$*;"
	time -p drush content-migrate-field-data-optimized $* -y 2>&1 | tee $@

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
