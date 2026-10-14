-- ============================================================================
-- Teacher acceptance letters (AT Portal). Run once, from the command line:
--     drush sql-cli < sites/all/modules/dh_manageapp/deploy/2026-10-08-teacher-acceptance.sql
--
-- ATAL (Assistant Teacher acceptance letter): every Active teacher.
-- CTAL (Centre Teacher acceptance letter): Active teachers with a CT centre (dh_teacher_role).
-- An AT Portal login whose teacher has not accepted the current version of a letter
-- is sent to at-portal/acceptance until they do (dh_atportal/acceptance.inc).
--
-- The letter texts are NOT in git (this repo is public). They are loaded into
-- dh_acceptance_doc by 2026-10-08-acceptance-letters-load.php from a private folder.
-- ============================================================================

-- One row per letter, version and language. Only one version of a letter is active;
-- loading a new version makes every teacher accept again.
CREATE TABLE IF NOT EXISTS dh_acceptance_doc (
  ad_id int unsigned NOT NULL AUTO_INCREMENT,
  ad_doc varchar(10) NOT NULL,              -- ATAL | CTAL
  ad_version varchar(20) NOT NULL,          -- e.g. 2012
  ad_lang char(2) NOT NULL,                 -- en | hi
  ad_title varchar(500) NOT NULL,
  ad_body mediumtext NOT NULL,              -- HTML
  ad_ack varchar(500) NOT NULL,             -- text next to the tick box
  ad_active tinyint(1) NOT NULL DEFAULT 1,
  ad_updated datetime NOT NULL,
  PRIMARY KEY (ad_id),
  UNIQUE KEY ad_doc_version_lang (ad_doc, ad_version, ad_lang)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

-- One row per teacher, letter and version accepted.
CREATE TABLE IF NOT EXISTS dh_teacher_acceptance (
  ta_id int unsigned NOT NULL AUTO_INCREMENT,
  ta_teacher int unsigned NOT NULL,         -- dh_teacher.t_id
  ta_doc varchar(10) NOT NULL,
  ta_version varchar(20) NOT NULL,
  ta_lang char(2) NOT NULL,                 -- language the letter was shown in when accepted
  ta_accepted datetime NOT NULL,
  ta_uid int unsigned NOT NULL,             -- account that accepted
  ta_ip varchar(45) NOT NULL DEFAULT '',
  PRIMARY KEY (ta_id),
  UNIQUE KEY ta_teacher_doc_version (ta_teacher, ta_doc, ta_version)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;
