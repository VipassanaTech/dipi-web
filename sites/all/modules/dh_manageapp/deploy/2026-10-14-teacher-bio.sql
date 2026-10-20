-- ============================================================================
-- Teacher bio data (the BIO DATA page of the AT acceptance letter), filled by the teacher in
-- the AT Portal (My Profile) or by the AT Book team. Run once, from the command line:
--     drush sql-cli < sites/all/modules/dh_manageapp/deploy/2026-10-14-teacher-bio.sql
-- dh_teacher itself is unchanged (its t_martial_status and t_photo columns are now used).
-- ============================================================================

-- One row per teacher. Everything is optional.
CREATE TABLE IF NOT EXISTS dh_teacher_bio (
  tb_teacher int unsigned NOT NULL,          -- dh_teacher.t_id
  tb_birth_place varchar(100) NOT NULL DEFAULT '',
  tb_nationality varchar(100) NOT NULL DEFAULT '',   -- a dh_country.c_nationality value
  tb_first_course_year smallint unsigned NULL,
  tb_first_course_month tinyint unsigned NULL,       -- 1-12, optional
  tb_first_course_by varchar(150) NOT NULL DEFAULT '',  -- who conducted the first course
  tb_education text NULL,
  tb_languages varchar(500) NOT NULL DEFAULT '',     -- dh_languages.l_name values, comma separated
  tb_occupation text NULL,                           -- business / job details
  tb_activities text NULL,                           -- other activities and memberships
  tb_serve_time text NULL,                           -- time to serve courses over the next 5 years
  tb_updated datetime NOT NULL,
  tb_updated_by int unsigned NOT NULL,
  PRIMARY KEY (tb_teacher)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

-- Family members, as asked on the bio-data page (name, age, relation, if meditator: courses).
CREATE TABLE IF NOT EXISTS dh_teacher_family (
  tf_id int unsigned NOT NULL AUTO_INCREMENT,
  tf_teacher int unsigned NOT NULL,
  tf_name varchar(100) NOT NULL,
  tf_relation varchar(30) NOT NULL DEFAULT '',
  tf_birth_year smallint unsigned NULL,      -- the age is worked out from it
  tf_meditator tinyint(1) NOT NULL DEFAULT 0,
  tf_courses smallint unsigned NULL,         -- number of courses, if a meditator
  tf_updated datetime NOT NULL,
  tf_updated_by int unsigned NOT NULL,
  PRIMARY KEY (tf_id),
  KEY tf_teacher (tf_teacher)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;
