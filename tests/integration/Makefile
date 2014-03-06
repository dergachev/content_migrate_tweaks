drush_sqlq := `drush sql-connect` -B --skip-column-names -e
field_name = field_course_title

tables.txt:
	$(drush_sqlq) "SELECT TABLE_NAME name FROM INFORMATION_SCHEMA.TABLES AS t WHERE TABLE_NAME LIKE 'field_%' AND TABLE_SCHEMA='drupal';" > tables.txt

checksum-orig.txt: tables.txt migrate-field-orig.txt
	cat tables.txt | xargs -I {} $(drush_sqlq) "checksum table {}" > checksum-orig.txt

checksum-optimized.txt: tables.txt migrate-field-optimized.txt
	cat tables.txt | xargs -I {} $(drush_sqlq) "checksum table {}" > checksum-optimized.txt

diff: checksum-orig.txt checksum-optimized.txt
	git diff --no-index --color checksum-orig.txt checksum-optimized.txt

# necessary only because 'drush content-migrate-field-data' might get exception and trigger rollback
ensure-structure-exists:
	# $(drush_sqlq) "describe field_data_field_course_title" || $(MAKE) migrate-field-structure.txt
	$(drush_sqlq) "describe field_data_field_course_title" || exit 1

migrate-field-orig.txt:
	$(MAKE) ensure-structure-exists
	$(drush_sqlq) "TRUNCATE TABLE field_data_$(field_name);  TRUNCATE TABLE field_revision_$(field_name);"
	time drush content-migrate-field-data $(field_name) -y 2>&1 | tee migrate-field-orig.txt

migrate-field-optimized.txt:
	$(MAKE) ensure-structure-exists
	$(drush_sqlq) "TRUNCATE TABLE field_data_$(field_name);  TRUNCATE TABLE field_revision_$(field_name);"
	time drush content-migrate-field-data-optimized $(field_name) -y 2>&1 | tee migrate-field-optimized.txt

structure:
	-rm migrate-field-orig.txt migrate-field-optimized.txt
	-drush content-migrate-rollback $(field_name) -y
	drush content-migrate-field-structure $(field_name) -y

rm_optimized:
	-rm migrate-field-optimized.txt

rm_orig:
	-rm migrate-field-orig.txt

rm_all:
	-rm *.txt

reset: rm_all structure
