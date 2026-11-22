-- ============================================================================
-- Rooms: a centre's physical rooms (dh_center_room) and the places in each
-- (dh_center_room_place). A student's room stays dh_applicant_attended.aa_section
-- + aa_acco (= crp_name). Run once, from the Drupal root, BEFORE the new code:
--     drush sql-cli < sites/all/modules/dh_manageapp/deploy/2026-11-21-rooms-schema.sql
-- ============================================================================
CREATE TABLE IF NOT EXISTS dh_center_room (
  cr_id bigint unsigned NOT NULL AUTO_INCREMENT,
  cr_center bigint NOT NULL,
  cr_gender char(1) NOT NULL,
  cr_section varchar(10) NOT NULL DEFAULT '',
  cr_name varchar(40) NOT NULL,
  cr_toilet varchar(10) NOT NULL DEFAULT 'unset',
  cr_weight int NOT NULL DEFAULT 0,
  cr_review tinyint(1) NOT NULL DEFAULT 0,
  cr_deleted tinyint(1) NOT NULL DEFAULT 0,
  cr_created datetime NOT NULL,
  cr_created_by int unsigned NOT NULL,
  cr_updated datetime NOT NULL,
  cr_updated_by int unsigned NOT NULL,
  PRIMARY KEY (cr_id),
  KEY cr_center (cr_center, cr_gender, cr_section)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

CREATE TABLE IF NOT EXISTS dh_center_room_place (
  crp_id bigint unsigned NOT NULL AUTO_INCREMENT,
  crp_room bigint unsigned NOT NULL,
  crp_name varchar(15) NOT NULL,
  crp_weight int NOT NULL DEFAULT 0,
  crp_deleted tinyint(1) NOT NULL DEFAULT 0,
  PRIMARY KEY (crp_id),
  KEY crp_room (crp_room)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;
