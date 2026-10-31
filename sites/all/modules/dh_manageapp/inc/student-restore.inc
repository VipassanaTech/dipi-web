<?php
/* The 2020 migrated students: bring back the students the April 2020 import made (numbers 82,481 to 281,166) that were
   later deleted by hand, or merge them into the student DIPI already has, so the old course lines point to real people.
   Design and plan: private Claude Docs "Restoring the 2020 migrated students". The old data comes from dh_restore_source,
   a temporary table filled from the prod-only database dipi_migration (deploy/2026-11-restore-source-fill.sql). */

define('DH_RESTORE_FIRST', 82481);
define('DH_RESTORE_LAST', 281166);

/* The temporary table of old rows: one row per old course line of part_b and part_c (a line in both has two rows).
   Safe to call twice. A CREATE TABLE commits any open transaction: never call it inside one. */
function dh_restore_source_create()
{
	db_query("CREATE TABLE IF NOT EXISTS dh_restore_source (
		rs_id bigint(20) NOT NULL AUTO_INCREMENT,
		rs_src char(1) NOT NULL,
		rs_old_id bigint(20) NOT NULL,
		rs_course bigint(20) NOT NULL,
		rs_conf_no varchar(20) NOT NULL,
		rs_type varchar(30) DEFAULT NULL,
		rs_f_name text, rs_m_name text, rs_l_name text, rs_gender varchar(10) DEFAULT NULL, rs_dob date DEFAULT NULL,
		rs_address text, rs_zip text, rs_country_name text, rs_state_name text, rs_city_name text,
		rs_email text, rs_phone_mobile text, rs_phone_home text, rs_phone_office text, rs_phone_fax text,
		rs_education text, rs_occupation text, rs_company text, rs_post text, rs_department text,
		rs_mother_tongue text, rs_other_langs text,
		rs_health_physical text, rs_health_mental text, rs_medication text, rs_intoxicants text, rs_other_technique text, rs_teach_others text,
		rs_acco text, rs_seat text,
		PRIMARY KEY (rs_id),
		KEY rs_line (rs_course, rs_conf_no)
	) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci");
}
