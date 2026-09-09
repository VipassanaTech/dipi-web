-- docs/superpowers/plans/sql/admin-bulk-mail.sql
CREATE TABLE IF NOT EXISTS dh_admin_bulk_mail (
  abm_id INT AUTO_INCREMENT PRIMARY KEY,
  abm_name VARCHAR(255) NOT NULL DEFAULT '',
  abm_subject VARCHAR(1024) NOT NULL DEFAULT '',
  abm_body LONGTEXT,
  abm_from_name VARCHAR(255) NOT NULL DEFAULT '',
  abm_from_email VARCHAR(255) NOT NULL DEFAULT '',
  abm_reply_to VARCHAR(255) NOT NULL DEFAULT '',
  abm_query LONGTEXT,
  abm_criteria LONGTEXT,
  abm_centres VARCHAR(1024) NOT NULL DEFAULT '',
  abm_count INT NOT NULL DEFAULT 0,
  abm_sent INT NOT NULL DEFAULT 0,
  abm_failed INT NOT NULL DEFAULT 0,
  abm_skipped_unsub INT NOT NULL DEFAULT 0,
  abm_skipped_invalid INT NOT NULL DEFAULT 0,
  abm_schedule_date DATE DEFAULT NULL,
  abm_processed TINYINT NOT NULL DEFAULT 0,
  abm_test_email VARCHAR(255) NOT NULL DEFAULT '',
  abm_test_sent_at DATETIME DEFAULT NULL,
  abm_deleted TINYINT NOT NULL DEFAULT 0,
  abm_created DATETIME DEFAULT NULL,
  abm_updated DATETIME DEFAULT NULL,
  abm_created_by INT DEFAULT NULL,
  abm_updated_by INT DEFAULT NULL,
  KEY idx_processed_date (abm_processed, abm_schedule_date)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

CREATE TABLE IF NOT EXISTS dh_admin_bulk_mail_log (
  abml_id INT AUTO_INCREMENT PRIMARY KEY,
  abml_abm INT NOT NULL,
  abml_applicant INT DEFAULT NULL,
  abml_name VARCHAR(255) NOT NULL DEFAULT '',
  abml_email VARCHAR(255) NOT NULL DEFAULT '',
  abml_center INT DEFAULT NULL,
  abml_status VARCHAR(32) NOT NULL DEFAULT '',
  abml_error VARCHAR(1024) NOT NULL DEFAULT '',
  abml_created DATETIME DEFAULT NULL,
  KEY idx_abm (abml_abm),
  KEY idx_created (abml_created)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

CREATE TABLE IF NOT EXISTS dh_admin_bulk_mail_attachment (
  abma_id INT AUTO_INCREMENT PRIMARY KEY,
  abma_abm INT NOT NULL,
  abma_uri VARCHAR(1024) NOT NULL DEFAULT '',
  abma_name VARCHAR(512) NOT NULL DEFAULT '',
  abma_size INT NOT NULL DEFAULT 0,
  abma_created DATETIME DEFAULT NULL,
  KEY idx_abm (abma_abm)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

CREATE TABLE IF NOT EXISTS dh_admin_bulk_mail_unsubscribe (
  abu_id INT AUTO_INCREMENT PRIMARY KEY,
  abu_email VARCHAR(255) NOT NULL,
  abu_created DATETIME DEFAULT NULL,
  UNIQUE KEY uniq_email (abu_email)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;
