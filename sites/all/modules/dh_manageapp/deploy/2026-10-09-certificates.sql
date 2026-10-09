-- ============================================================================
-- Course certificates (inc/certificate.inc). Run once, from the command line:
--     drush sql-cli < sites/all/modules/dh_manageapp/deploy/2026-10-09-certificates.sql
-- ============================================================================

-- The register: one certificate per application, numbered per centre per year
-- (e.g. CAKKA/2026/0001). Reprints keep the number and the first issue date, so a
-- centre can look up the student from the number on a certificate.
CREATE TABLE IF NOT EXISTS dh_certificate (
  cert_id int unsigned NOT NULL AUTO_INCREMENT,
  cert_applicant bigint NOT NULL,            -- dh_applicant.a_id
  cert_course bigint unsigned NOT NULL,
  cert_center bigint unsigned NOT NULL,
  cert_year smallint unsigned NOT NULL,      -- year of first issue
  cert_serial int unsigned NOT NULL,         -- 1, 2, 3 ... per centre per year
  cert_number varchar(40) NOT NULL,          -- PREFIX/YEAR/0001
  cert_issued date NOT NULL,                 -- first issue date (printed on every copy)
  cert_issued_by int unsigned NOT NULL,
  cert_prints int unsigned NOT NULL DEFAULT 1,
  cert_last_print datetime NOT NULL,
  PRIMARY KEY (cert_id),
  UNIQUE KEY cert_applicant (cert_applicant),
  UNIQUE KEY cert_number (cert_number),
  UNIQUE KEY cert_center_year_serial (cert_center, cert_year, cert_serial)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

-- Per-centre certificate text, edited on centre/{c}/certificate. A centre without a row
-- uses defaults built from its dh_center record.
CREATE TABLE IF NOT EXISTS dh_certificate_setting (
  certs_center bigint unsigned NOT NULL,
  certs_prefix varchar(20) NOT NULL,         -- number prefix, e.g. CAKKA
  certs_title varchar(200) NOT NULL,         -- big heading, e.g. DHAMMA CAKKA
  certs_subtitle varchar(200) NOT NULL,      -- e.g. SARNATH · VIPASSANA MEDITATION CENTRE
  certs_centre_name varchar(200) NOT NULL,   -- used in "... at Dhamma Cakka."
  certs_description text NOT NULL,           -- the centre paragraph (one line per row)
  certs_footer text NOT NULL,                -- contact block, the first line is printed bold
  certs_bottom varchar(300) NOT NULL,        -- one short address line at the bottom
  certs_updated datetime NOT NULL,
  certs_updated_by int unsigned NOT NULL,
  PRIMARY KEY (certs_center)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;
