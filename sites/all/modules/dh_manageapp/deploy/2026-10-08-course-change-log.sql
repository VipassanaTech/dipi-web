-- ============================================================================
-- Course change log  -  PROD schema migration (2026-10-08)
-- ============================================================================
-- Who changed what on a course, old -> new, ONE ROW PER CHANGED FIELD (same shape
-- as dh_center_log). Written by triggers, so every writer is covered (course editor,
-- crons, the dhamma.org sync and intake, direct SQL).
--   crl_action    'create' (new course: one row per filled field), 'update',
--                 'delete' / 'restore' (the save that flips c_deleted)
--   crl_field     the dh_course column, e.g. c_start
--   crl_user      c_updated_by of the save (c_created_by for a new course); automatic
--                 jobs stamp the System user (dh_type_detail COURSE-SYSTEM-UID)
--   crl_dh_log_id set only on rows converted from the old text log (dh_log.l_id)
-- Not logged: sync bookkeeping (c_processed, c_processed_error, c_finalized_tstamp,
-- c_uri, c_updated*), c_name (built from type + description + dates, all logged),
-- c_id, c_mute.
-- Replaces the old text trigger (one "CHANGE X changed from A To B" line in dh_log);
-- its rows already in dh_log stay. Rollback: 2026-10-08-course-change-log-rollback.sql
-- Run as root:  mysql <db> < this file   (same sql_mode as the trigger it replaces;
-- CREATE OR REPLACE swaps it in one statement; the short lock wait gives up instead
-- of queueing course queries)
-- ============================================================================
CREATE TABLE IF NOT EXISTS dh_course_log (
  crl_id        bigint(20) unsigned NOT NULL AUTO_INCREMENT,
  crl_course    bigint(20) unsigned NOT NULL,
  crl_center    bigint(20) NOT NULL,
  crl_action    varchar(10) NOT NULL,
  crl_field     varchar(64) NOT NULL,
  crl_old       mediumtext,
  crl_new       mediumtext,
  crl_user      bigint(20) NOT NULL,
  crl_tstamp    datetime NOT NULL,
  crl_dh_log_id bigint(20) DEFAULT NULL,
  PRIMARY KEY (crl_id),
  KEY crl_course_tstamp (crl_course, crl_tstamp),
  KEY crl_center_tstamp (crl_center, crl_tstamp),
  KEY crl_dh_log_id (crl_dh_log_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

SET SESSION sql_mode = 'ERROR_FOR_DIVISION_BY_ZERO,NO_AUTO_CREATE_USER';
SET SESSION lock_wait_timeout = 5;
DELIMITER $$
CREATE OR REPLACE TRIGGER dh_course_au AFTER UPDATE ON dh_course FOR EACH ROW
BEGIN
  DECLARE act VARCHAR(10);
  DECLARE who BIGINT;
  DECLARE ctr BIGINT;
  DECLARE ts DATETIME;
  SET act = 'update';
  SET who = IFNULL(NEW.c_updated_by, 0);
  SET ctr = IFNULL(NEW.c_center, 0);
  SET ts = NOW();
  IF NOT (OLD.c_deleted <=> NEW.c_deleted) THEN
    SET act = IF(NEW.c_deleted = 1, 'delete', 'restore');
  END IF;
  IF NOT (OLD.c_course_type <=> NEW.c_course_type) THEN
    INSERT INTO dh_course_log (crl_course, crl_center, crl_action, crl_field, crl_old, crl_new, crl_user, crl_tstamp) VALUES (NEW.c_id, ctr, act, 'c_course_type', OLD.c_course_type, NEW.c_course_type, who, ts);
  END IF;
  IF NOT (OLD.c_center <=> NEW.c_center) THEN
    INSERT INTO dh_course_log (crl_course, crl_center, crl_action, crl_field, crl_old, crl_new, crl_user, crl_tstamp) VALUES (NEW.c_id, ctr, act, 'c_center', OLD.c_center, NEW.c_center, who, ts);
  END IF;
  IF NOT (OLD.c_cancelled <=> NEW.c_cancelled) THEN
    INSERT INTO dh_course_log (crl_course, crl_center, crl_action, crl_field, crl_old, crl_new, crl_user, crl_tstamp) VALUES (NEW.c_id, ctr, act, 'c_cancelled', OLD.c_cancelled, NEW.c_cancelled, who, ts);
  END IF;
  IF NOT (OLD.c_enrol_date <=> NEW.c_enrol_date) THEN
    INSERT INTO dh_course_log (crl_course, crl_center, crl_action, crl_field, crl_old, crl_new, crl_user, crl_tstamp) VALUES (NEW.c_id, ctr, act, 'c_enrol_date', OLD.c_enrol_date, NEW.c_enrol_date, who, ts);
  END IF;
  IF NOT (OLD.c_date_change <=> NEW.c_date_change) THEN
    INSERT INTO dh_course_log (crl_course, crl_center, crl_action, crl_field, crl_old, crl_new, crl_user, crl_tstamp) VALUES (NEW.c_id, ctr, act, 'c_date_change', OLD.c_date_change, NEW.c_date_change, who, ts);
  END IF;
  IF NOT (OLD.c_end <=> NEW.c_end) THEN
    INSERT INTO dh_course_log (crl_course, crl_center, crl_action, crl_field, crl_old, crl_new, crl_user, crl_tstamp) VALUES (NEW.c_id, ctr, act, 'c_end', OLD.c_end, NEW.c_end, who, ts);
  END IF;
  IF NOT (OLD.c_start <=> NEW.c_start) THEN
    INSERT INTO dh_course_log (crl_course, crl_center, crl_action, crl_field, crl_old, crl_new, crl_user, crl_tstamp) VALUES (NEW.c_id, ctr, act, 'c_start', OLD.c_start, NEW.c_start, who, ts);
  END IF;
  IF BINARY IFNULL(OLD.c_status_om, '') <> BINARY IFNULL(NEW.c_status_om, '') THEN
    INSERT INTO dh_course_log (crl_course, crl_center, crl_action, crl_field, crl_old, crl_new, crl_user, crl_tstamp) VALUES (NEW.c_id, ctr, act, 'c_status_om', OLD.c_status_om, NEW.c_status_om, who, ts);
  END IF;
  IF BINARY IFNULL(OLD.c_status_nm, '') <> BINARY IFNULL(NEW.c_status_nm, '') THEN
    INSERT INTO dh_course_log (crl_course, crl_center, crl_action, crl_field, crl_old, crl_new, crl_user, crl_tstamp) VALUES (NEW.c_id, ctr, act, 'c_status_nm', OLD.c_status_nm, NEW.c_status_nm, who, ts);
  END IF;
  IF BINARY IFNULL(OLD.c_status_of, '') <> BINARY IFNULL(NEW.c_status_of, '') THEN
    INSERT INTO dh_course_log (crl_course, crl_center, crl_action, crl_field, crl_old, crl_new, crl_user, crl_tstamp) VALUES (NEW.c_id, ctr, act, 'c_status_of', OLD.c_status_of, NEW.c_status_of, who, ts);
  END IF;
  IF BINARY IFNULL(OLD.c_status_nf, '') <> BINARY IFNULL(NEW.c_status_nf, '') THEN
    INSERT INTO dh_course_log (crl_course, crl_center, crl_action, crl_field, crl_old, crl_new, crl_user, crl_tstamp) VALUES (NEW.c_id, ctr, act, 'c_status_nf', OLD.c_status_nf, NEW.c_status_nf, who, ts);
  END IF;
  IF NOT (OLD.c_list_only <=> NEW.c_list_only) THEN
    INSERT INTO dh_course_log (crl_course, crl_center, crl_action, crl_field, crl_old, crl_new, crl_user, crl_tstamp) VALUES (NEW.c_id, ctr, act, 'c_list_only', OLD.c_list_only, NEW.c_list_only, who, ts);
  END IF;
  IF BINARY IFNULL(OLD.c_status_svr_m, '') <> BINARY IFNULL(NEW.c_status_svr_m, '') THEN
    INSERT INTO dh_course_log (crl_course, crl_center, crl_action, crl_field, crl_old, crl_new, crl_user, crl_tstamp) VALUES (NEW.c_id, ctr, act, 'c_status_svr_m', OLD.c_status_svr_m, NEW.c_status_svr_m, who, ts);
  END IF;
  IF BINARY IFNULL(OLD.c_status_svr_f, '') <> BINARY IFNULL(NEW.c_status_svr_f, '') THEN
    INSERT INTO dh_course_log (crl_course, crl_center, crl_action, crl_field, crl_old, crl_new, crl_user, crl_tstamp) VALUES (NEW.c_id, ctr, act, 'c_status_svr_f', OLD.c_status_svr_f, NEW.c_status_svr_f, who, ts);
  END IF;
  IF BINARY IFNULL(OLD.c_form_langs, '') <> BINARY IFNULL(NEW.c_form_langs, '') THEN
    INSERT INTO dh_course_log (crl_course, crl_center, crl_action, crl_field, crl_old, crl_new, crl_user, crl_tstamp) VALUES (NEW.c_id, ctr, act, 'c_form_langs', OLD.c_form_langs, NEW.c_form_langs, who, ts);
  END IF;
  IF BINARY IFNULL(OLD.c_comments, '') <> BINARY IFNULL(NEW.c_comments, '') THEN
    INSERT INTO dh_course_log (crl_course, crl_center, crl_action, crl_field, crl_old, crl_new, crl_user, crl_tstamp) VALUES (NEW.c_id, ctr, act, 'c_comments', OLD.c_comments, NEW.c_comments, who, ts);
  END IF;
  IF BINARY IFNULL(OLD.c_description, '') <> BINARY IFNULL(NEW.c_description, '') THEN
    INSERT INTO dh_course_log (crl_course, crl_center, crl_action, crl_field, crl_old, crl_new, crl_user, crl_tstamp) VALUES (NEW.c_id, ctr, act, 'c_description', OLD.c_description, NEW.c_description, who, ts);
  END IF;
  IF NOT (OLD.c_deleted <=> NEW.c_deleted) THEN
    INSERT INTO dh_course_log (crl_course, crl_center, crl_action, crl_field, crl_old, crl_new, crl_user, crl_tstamp) VALUES (NEW.c_id, ctr, act, 'c_deleted', OLD.c_deleted, NEW.c_deleted, who, ts);
  END IF;
  IF NOT (OLD.c_combined_seat_course <=> NEW.c_combined_seat_course) THEN
    INSERT INTO dh_course_log (crl_course, crl_center, crl_action, crl_field, crl_old, crl_new, crl_user, crl_tstamp) VALUES (NEW.c_id, ctr, act, 'c_combined_seat_course', OLD.c_combined_seat_course, NEW.c_combined_seat_course, who, ts);
  END IF;
  IF NOT (OLD.c_at_m_count <=> NEW.c_at_m_count) THEN
    INSERT INTO dh_course_log (crl_course, crl_center, crl_action, crl_field, crl_old, crl_new, crl_user, crl_tstamp) VALUES (NEW.c_id, ctr, act, 'c_at_m_count', OLD.c_at_m_count, NEW.c_at_m_count, who, ts);
  END IF;
  IF NOT (OLD.c_at_f_count <=> NEW.c_at_f_count) THEN
    INSERT INTO dh_course_log (crl_course, crl_center, crl_action, crl_field, crl_old, crl_new, crl_user, crl_tstamp) VALUES (NEW.c_id, ctr, act, 'c_at_f_count', OLD.c_at_f_count, NEW.c_at_f_count, who, ts);
  END IF;
  IF NOT (OLD.c_at_m_conf <=> NEW.c_at_m_conf) THEN
    INSERT INTO dh_course_log (crl_course, crl_center, crl_action, crl_field, crl_old, crl_new, crl_user, crl_tstamp) VALUES (NEW.c_id, ctr, act, 'c_at_m_conf', OLD.c_at_m_conf, NEW.c_at_m_conf, who, ts);
  END IF;
  IF NOT (OLD.c_at_f_conf <=> NEW.c_at_f_conf) THEN
    INSERT INTO dh_course_log (crl_course, crl_center, crl_action, crl_field, crl_old, crl_new, crl_user, crl_tstamp) VALUES (NEW.c_id, ctr, act, 'c_at_f_conf', OLD.c_at_f_conf, NEW.c_at_f_conf, who, ts);
  END IF;
  IF NOT (OLD.c_finalized <=> NEW.c_finalized) THEN
    INSERT INTO dh_course_log (crl_course, crl_center, crl_action, crl_field, crl_old, crl_new, crl_user, crl_tstamp) VALUES (NEW.c_id, ctr, act, 'c_finalized', OLD.c_finalized, NEW.c_finalized, who, ts);
  END IF;
  IF BINARY IFNULL(OLD.c_status, '') <> BINARY IFNULL(NEW.c_status, '') THEN
    INSERT INTO dh_course_log (crl_course, crl_center, crl_action, crl_field, crl_old, crl_new, crl_user, crl_tstamp) VALUES (NEW.c_id, ctr, act, 'c_status', OLD.c_status, NEW.c_status, who, ts);
  END IF;
  IF NOT (OLD.c_auto_confirm <=> NEW.c_auto_confirm) THEN
    INSERT INTO dh_course_log (crl_course, crl_center, crl_action, crl_field, crl_old, crl_new, crl_user, crl_tstamp) VALUES (NEW.c_id, ctr, act, 'c_auto_confirm', OLD.c_auto_confirm, NEW.c_auto_confirm, who, ts);
  END IF;
  IF NOT (OLD.c_cell_plan <=> NEW.c_cell_plan) THEN
    INSERT INTO dh_course_log (crl_course, crl_center, crl_action, crl_field, crl_old, crl_new, crl_user, crl_tstamp) VALUES (NEW.c_id, ctr, act, 'c_cell_plan', OLD.c_cell_plan, NEW.c_cell_plan, who, ts);
  END IF;
  IF NOT (OLD.c_dining_plan <=> NEW.c_dining_plan) THEN
    INSERT INTO dh_course_log (crl_course, crl_center, crl_action, crl_field, crl_old, crl_new, crl_user, crl_tstamp) VALUES (NEW.c_id, ctr, act, 'c_dining_plan', OLD.c_dining_plan, NEW.c_dining_plan, who, ts);
  END IF;
  IF NOT (OLD.c_seat_plan <=> NEW.c_seat_plan) THEN
    INSERT INTO dh_course_log (crl_course, crl_center, crl_action, crl_field, crl_old, crl_new, crl_user, crl_tstamp) VALUES (NEW.c_id, ctr, act, 'c_seat_plan', OLD.c_seat_plan, NEW.c_seat_plan, who, ts);
  END IF;
  IF NOT (OLD.c_dashboard_pin <=> NEW.c_dashboard_pin) THEN
    INSERT INTO dh_course_log (crl_course, crl_center, crl_action, crl_field, crl_old, crl_new, crl_user, crl_tstamp) VALUES (NEW.c_id, ctr, act, 'c_dashboard_pin', OLD.c_dashboard_pin, NEW.c_dashboard_pin, who, ts);
  END IF;
END
$$
CREATE OR REPLACE TRIGGER dh_course_ai AFTER INSERT ON dh_course FOR EACH ROW
BEGIN
  DECLARE act VARCHAR(10);
  DECLARE who BIGINT;
  DECLARE ctr BIGINT;
  DECLARE ts DATETIME;
  SET act = 'create';
  SET who = IFNULL(NEW.c_created_by, IFNULL(NEW.c_updated_by, 0));
  SET ctr = IFNULL(NEW.c_center, 0);
  SET ts = NOW();
  IF IFNULL(NEW.c_course_type, 0) <> 0 THEN
    INSERT INTO dh_course_log (crl_course, crl_center, crl_action, crl_field, crl_old, crl_new, crl_user, crl_tstamp) VALUES (NEW.c_id, ctr, act, 'c_course_type', NULL, NEW.c_course_type, who, ts);
  END IF;
  IF IFNULL(NEW.c_center, 0) <> 0 THEN
    INSERT INTO dh_course_log (crl_course, crl_center, crl_action, crl_field, crl_old, crl_new, crl_user, crl_tstamp) VALUES (NEW.c_id, ctr, act, 'c_center', NULL, NEW.c_center, who, ts);
  END IF;
  IF IFNULL(NEW.c_cancelled, 0) <> 0 THEN
    INSERT INTO dh_course_log (crl_course, crl_center, crl_action, crl_field, crl_old, crl_new, crl_user, crl_tstamp) VALUES (NEW.c_id, ctr, act, 'c_cancelled', NULL, NEW.c_cancelled, who, ts);
  END IF;
  IF NEW.c_enrol_date IS NOT NULL THEN
    INSERT INTO dh_course_log (crl_course, crl_center, crl_action, crl_field, crl_old, crl_new, crl_user, crl_tstamp) VALUES (NEW.c_id, ctr, act, 'c_enrol_date', NULL, NEW.c_enrol_date, who, ts);
  END IF;
  IF IFNULL(NEW.c_date_change, 0) <> 0 THEN
    INSERT INTO dh_course_log (crl_course, crl_center, crl_action, crl_field, crl_old, crl_new, crl_user, crl_tstamp) VALUES (NEW.c_id, ctr, act, 'c_date_change', NULL, NEW.c_date_change, who, ts);
  END IF;
  IF NEW.c_end IS NOT NULL THEN
    INSERT INTO dh_course_log (crl_course, crl_center, crl_action, crl_field, crl_old, crl_new, crl_user, crl_tstamp) VALUES (NEW.c_id, ctr, act, 'c_end', NULL, NEW.c_end, who, ts);
  END IF;
  IF NEW.c_start IS NOT NULL THEN
    INSERT INTO dh_course_log (crl_course, crl_center, crl_action, crl_field, crl_old, crl_new, crl_user, crl_tstamp) VALUES (NEW.c_id, ctr, act, 'c_start', NULL, NEW.c_start, who, ts);
  END IF;
  IF IFNULL(NEW.c_status_om, '') <> '' THEN
    INSERT INTO dh_course_log (crl_course, crl_center, crl_action, crl_field, crl_old, crl_new, crl_user, crl_tstamp) VALUES (NEW.c_id, ctr, act, 'c_status_om', NULL, NEW.c_status_om, who, ts);
  END IF;
  IF IFNULL(NEW.c_status_nm, '') <> '' THEN
    INSERT INTO dh_course_log (crl_course, crl_center, crl_action, crl_field, crl_old, crl_new, crl_user, crl_tstamp) VALUES (NEW.c_id, ctr, act, 'c_status_nm', NULL, NEW.c_status_nm, who, ts);
  END IF;
  IF IFNULL(NEW.c_status_of, '') <> '' THEN
    INSERT INTO dh_course_log (crl_course, crl_center, crl_action, crl_field, crl_old, crl_new, crl_user, crl_tstamp) VALUES (NEW.c_id, ctr, act, 'c_status_of', NULL, NEW.c_status_of, who, ts);
  END IF;
  IF IFNULL(NEW.c_status_nf, '') <> '' THEN
    INSERT INTO dh_course_log (crl_course, crl_center, crl_action, crl_field, crl_old, crl_new, crl_user, crl_tstamp) VALUES (NEW.c_id, ctr, act, 'c_status_nf', NULL, NEW.c_status_nf, who, ts);
  END IF;
  IF IFNULL(NEW.c_list_only, 0) <> 0 THEN
    INSERT INTO dh_course_log (crl_course, crl_center, crl_action, crl_field, crl_old, crl_new, crl_user, crl_tstamp) VALUES (NEW.c_id, ctr, act, 'c_list_only', NULL, NEW.c_list_only, who, ts);
  END IF;
  IF IFNULL(NEW.c_status_svr_m, '') <> '' THEN
    INSERT INTO dh_course_log (crl_course, crl_center, crl_action, crl_field, crl_old, crl_new, crl_user, crl_tstamp) VALUES (NEW.c_id, ctr, act, 'c_status_svr_m', NULL, NEW.c_status_svr_m, who, ts);
  END IF;
  IF IFNULL(NEW.c_status_svr_f, '') <> '' THEN
    INSERT INTO dh_course_log (crl_course, crl_center, crl_action, crl_field, crl_old, crl_new, crl_user, crl_tstamp) VALUES (NEW.c_id, ctr, act, 'c_status_svr_f', NULL, NEW.c_status_svr_f, who, ts);
  END IF;
  IF IFNULL(NEW.c_form_langs, '') <> '' THEN
    INSERT INTO dh_course_log (crl_course, crl_center, crl_action, crl_field, crl_old, crl_new, crl_user, crl_tstamp) VALUES (NEW.c_id, ctr, act, 'c_form_langs', NULL, NEW.c_form_langs, who, ts);
  END IF;
  IF IFNULL(NEW.c_comments, '') <> '' THEN
    INSERT INTO dh_course_log (crl_course, crl_center, crl_action, crl_field, crl_old, crl_new, crl_user, crl_tstamp) VALUES (NEW.c_id, ctr, act, 'c_comments', NULL, NEW.c_comments, who, ts);
  END IF;
  IF IFNULL(NEW.c_description, '') <> '' THEN
    INSERT INTO dh_course_log (crl_course, crl_center, crl_action, crl_field, crl_old, crl_new, crl_user, crl_tstamp) VALUES (NEW.c_id, ctr, act, 'c_description', NULL, NEW.c_description, who, ts);
  END IF;
  IF IFNULL(NEW.c_deleted, 0) <> 0 THEN
    INSERT INTO dh_course_log (crl_course, crl_center, crl_action, crl_field, crl_old, crl_new, crl_user, crl_tstamp) VALUES (NEW.c_id, ctr, act, 'c_deleted', NULL, NEW.c_deleted, who, ts);
  END IF;
  IF IFNULL(NEW.c_combined_seat_course, 0) <> 0 THEN
    INSERT INTO dh_course_log (crl_course, crl_center, crl_action, crl_field, crl_old, crl_new, crl_user, crl_tstamp) VALUES (NEW.c_id, ctr, act, 'c_combined_seat_course', NULL, NEW.c_combined_seat_course, who, ts);
  END IF;
  IF IFNULL(NEW.c_at_m_count, 0) <> 0 THEN
    INSERT INTO dh_course_log (crl_course, crl_center, crl_action, crl_field, crl_old, crl_new, crl_user, crl_tstamp) VALUES (NEW.c_id, ctr, act, 'c_at_m_count', NULL, NEW.c_at_m_count, who, ts);
  END IF;
  IF IFNULL(NEW.c_at_f_count, 0) <> 0 THEN
    INSERT INTO dh_course_log (crl_course, crl_center, crl_action, crl_field, crl_old, crl_new, crl_user, crl_tstamp) VALUES (NEW.c_id, ctr, act, 'c_at_f_count', NULL, NEW.c_at_f_count, who, ts);
  END IF;
  IF IFNULL(NEW.c_at_m_conf, 0) <> 0 THEN
    INSERT INTO dh_course_log (crl_course, crl_center, crl_action, crl_field, crl_old, crl_new, crl_user, crl_tstamp) VALUES (NEW.c_id, ctr, act, 'c_at_m_conf', NULL, NEW.c_at_m_conf, who, ts);
  END IF;
  IF IFNULL(NEW.c_at_f_conf, 0) <> 0 THEN
    INSERT INTO dh_course_log (crl_course, crl_center, crl_action, crl_field, crl_old, crl_new, crl_user, crl_tstamp) VALUES (NEW.c_id, ctr, act, 'c_at_f_conf', NULL, NEW.c_at_f_conf, who, ts);
  END IF;
  IF IFNULL(NEW.c_finalized, 0) <> 0 THEN
    INSERT INTO dh_course_log (crl_course, crl_center, crl_action, crl_field, crl_old, crl_new, crl_user, crl_tstamp) VALUES (NEW.c_id, ctr, act, 'c_finalized', NULL, NEW.c_finalized, who, ts);
  END IF;
  IF IFNULL(NEW.c_status, '') <> '' THEN
    INSERT INTO dh_course_log (crl_course, crl_center, crl_action, crl_field, crl_old, crl_new, crl_user, crl_tstamp) VALUES (NEW.c_id, ctr, act, 'c_status', NULL, NEW.c_status, who, ts);
  END IF;
  IF IFNULL(NEW.c_auto_confirm, 0) <> 0 THEN
    INSERT INTO dh_course_log (crl_course, crl_center, crl_action, crl_field, crl_old, crl_new, crl_user, crl_tstamp) VALUES (NEW.c_id, ctr, act, 'c_auto_confirm', NULL, NEW.c_auto_confirm, who, ts);
  END IF;
  IF IFNULL(NEW.c_cell_plan, 0) <> 0 THEN
    INSERT INTO dh_course_log (crl_course, crl_center, crl_action, crl_field, crl_old, crl_new, crl_user, crl_tstamp) VALUES (NEW.c_id, ctr, act, 'c_cell_plan', NULL, NEW.c_cell_plan, who, ts);
  END IF;
  IF IFNULL(NEW.c_dining_plan, 0) <> 0 THEN
    INSERT INTO dh_course_log (crl_course, crl_center, crl_action, crl_field, crl_old, crl_new, crl_user, crl_tstamp) VALUES (NEW.c_id, ctr, act, 'c_dining_plan', NULL, NEW.c_dining_plan, who, ts);
  END IF;
  IF IFNULL(NEW.c_seat_plan, 0) <> 0 THEN
    INSERT INTO dh_course_log (crl_course, crl_center, crl_action, crl_field, crl_old, crl_new, crl_user, crl_tstamp) VALUES (NEW.c_id, ctr, act, 'c_seat_plan', NULL, NEW.c_seat_plan, who, ts);
  END IF;
  IF IFNULL(NEW.c_dashboard_pin, 0) <> 0 THEN
    INSERT INTO dh_course_log (crl_course, crl_center, crl_action, crl_field, crl_old, crl_new, crl_user, crl_tstamp) VALUES (NEW.c_id, ctr, act, 'c_dashboard_pin', NULL, NEW.c_dashboard_pin, who, ts);
  END IF;
END
$$
DELIMITER ;
