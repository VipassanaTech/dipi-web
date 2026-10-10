-- ============================================================================
-- Backup for the one-time merge-field auto-fix (deploy/2026-10-10-letter-autofix.php).
-- One row per changed column of a letter (dh_letter), with the text before and after, so
-- any fix can be put back. Run once, before the script:
--     drush sql-cli < sites/all/modules/dh_manageapp/deploy/2026-10-10-letter-fix-backup.sql
-- ============================================================================
CREATE TABLE IF NOT EXISTS dh_letter_fix_backup (
  b_id int unsigned NOT NULL AUTO_INCREMENT,
  b_table varchar(20) NOT NULL,          -- dh_letter
  b_row bigint NOT NULL,                 -- l_id
  b_center bigint NOT NULL,
  b_column varchar(20) NOT NULL,         -- l_subject, l_body or l_sms
  b_before longtext NOT NULL,
  b_after longtext NOT NULL,
  b_at datetime NOT NULL,
  PRIMARY KEY (b_id),
  KEY b_row (b_table, b_row)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;
