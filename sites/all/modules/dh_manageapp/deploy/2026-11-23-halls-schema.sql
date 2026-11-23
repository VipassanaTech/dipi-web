-- ============================================================================
-- Dhamma Halls (23 Nov 2026): a centre's halls, the usual hall of each student
-- group (1-9) and a course's own changes. Only centres with 2+ halls use them.
-- Run once BEFORE the new code (local: the php one-liner in the plan; prod: in
-- the prod plan). Re-runnable (IF NOT EXISTS).
-- ============================================================================
CREATE TABLE IF NOT EXISTS dh_center_hall (
  ch_id bigint unsigned NOT NULL AUTO_INCREMENT,
  ch_center bigint NOT NULL,
  ch_code varchar(6) NOT NULL,
  ch_name varchar(40) NOT NULL,
  ch_weight int NOT NULL DEFAULT 0,
  ch_deleted tinyint(1) NOT NULL DEFAULT 0,
  ch_created datetime NOT NULL,
  ch_created_by int unsigned NOT NULL,
  ch_updated datetime NOT NULL,
  ch_updated_by int unsigned NOT NULL,
  PRIMARY KEY (ch_id),
  KEY ch_center (ch_center, ch_deleted)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

CREATE TABLE IF NOT EXISTS dh_center_hall_group (
  chg_center bigint NOT NULL,
  chg_group tinyint unsigned NOT NULL,
  chg_hall bigint unsigned NOT NULL,
  PRIMARY KEY (chg_center, chg_group)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

CREATE TABLE IF NOT EXISTS dh_course_hall_group (
  cg_course bigint NOT NULL,
  cg_group tinyint unsigned NOT NULL,
  cg_hall bigint unsigned NOT NULL,
  PRIMARY KEY (cg_course, cg_group)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;
