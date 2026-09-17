-- ============================================================================
-- Cell + Dining unified engine  —  PROD schema migration
-- Branch: dining-unified-engine
-- ============================================================================
-- RUN ONCE, and BEFORE the new code serves traffic. The OLD (current) prod code
-- keeps working after this runs — it simply ignores the new columns — so it is
-- safe to apply first and then `git pull`.
--
-- Prod was verified on 2026-09-17 to be MISSING every column added below.
-- Column definitions match LOCAL exactly (utf8mb4 / utf8mb4_general_ci table;
-- new varchar/mediumtext columns inherit the table charset).
--
--   mysql <dbname> < 2026-09-17-cell-dining-schema.sql
-- ============================================================================

ALTER TABLE dh_applicant_attended
  ADD COLUMN aa_cell_batch         tinyint(4)  NULL     DEFAULT NULL,
  ADD COLUMN aa_group_cell         varchar(10) NULL     DEFAULT NULL,
  ADD COLUMN aa_group_cell_fixed   tinyint(1)  NOT NULL DEFAULT 0,
  ADD COLUMN aa_dining             varchar(10) NULL     DEFAULT NULL,
  ADD COLUMN aa_dining_fixed       tinyint(1)  NULL     DEFAULT 0,
  ADD COLUMN aa_dining_batch       tinyint(4)  NULL     DEFAULT NULL,
  ADD COLUMN aa_group_dining       varchar(10) NULL     DEFAULT NULL,
  ADD COLUMN aa_group_dining_fixed tinyint(1)  NOT NULL DEFAULT 0;

-- Carry existing per-applicant cell batch numbers over from the old column name
-- (the branch renamed aa_cell_group -> aa_cell_batch).
UPDATE dh_applicant_attended SET aa_cell_batch = aa_cell_group WHERE aa_cell_group IS NOT NULL;

ALTER TABLE dh_center_setting
  ADD COLUMN cs_has_dining    tinyint(1) NULL DEFAULT 0,
  ADD COLUMN cs_dining_config mediumtext NULL DEFAULT NULL;

-- ----------------------------------------------------------------------------
-- OPTIONAL cleanup — run only AFTER the new code has been verified in prod
-- (a day or so later), NOT in the same deploy window:
--   ALTER TABLE dh_applicant_attended DROP COLUMN aa_cell_group;
-- ----------------------------------------------------------------------------
