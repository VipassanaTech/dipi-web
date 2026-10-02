-- ============================================================================
-- Centre change log  —  PROD schema migration
-- ============================================================================
-- RUN ONCE when deploying the centre change log (inc/centre-log.inc). The table
-- must exist BEFORE the new code serves traffic: centre saves write to it.
--   mysql <dbname> < 2026-10-02-centre-change-log.sql
-- ============================================================================

-- Who changed what on a centre, old -> new, one row per changed field:
--   cl_area   'centre' (dh_center), 'settings' (dh_center_setting),
--             'acco' (dh_center_setting_acco), 'access' (dh_user_center)
--   cl_record csa_id / uc_id for the row-level areas (acco, access), else NULL
--   cl_action 'create', 'update', 'delete', 'restore'
CREATE TABLE IF NOT EXISTS dh_center_log (
  cl_id      bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  cl_center  bigint(20) NOT NULL,
  cl_area    varchar(20) NOT NULL,
  cl_record  bigint(20) DEFAULT NULL,
  cl_action  varchar(10) NOT NULL,
  cl_field   varchar(64) NOT NULL,
  cl_old     mediumtext,
  cl_new     mediumtext,
  cl_user    bigint(20) NOT NULL,
  cl_tstamp  datetime NOT NULL,
  PRIMARY KEY (cl_id),
  KEY cl_center_tstamp (cl_center, cl_tstamp)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

-- The old trigger wrote only a fixed 'Center Update' line to dh_log, naming the
-- PREVIOUS editor (OLD.c_updated_by); the change log above replaces it. Its rows
-- already in dh_log stay. To re-create it: 2026-10-02-dh_center_au-rollback.sql
DROP TRIGGER IF EXISTS dh_center_au;
