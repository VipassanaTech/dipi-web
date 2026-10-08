-- ============================================================================
-- ROLLBACK for 2026-10-08-course-change-log.sql
-- ============================================================================
-- Removes the course-creation trigger and recreates dh_course_au exactly as it was
-- on prod before 2026-10-08 (31 fields, one text line per save in dh_log), with its
-- original sql_mode. No DEFINER (this file is in a public repo).
-- dh_course_log is KEPT (it holds the history written meanwhile); drop it by hand
-- only if it is really unwanted:  DROP TABLE dh_course_log;
-- Run as root:  mysql <db> < this file
-- ============================================================================
DROP TRIGGER IF EXISTS dh_course_ai;
SET SESSION sql_mode = 'ERROR_FOR_DIVISION_BY_ZERO,NO_AUTO_CREATE_USER';
SET SESSION lock_wait_timeout = 5;
DELIMITER $$
CREATE OR REPLACE TRIGGER dh_course_au AFTER UPDATE ON dh_course FOR EACH ROW
BEGIN
DECLARE MSG TEXT;
SET MSG = 'CHANGE';

    if (old.c_id != new.c_id) THEN
        SET MSG         = CONCAT(MSG, ' CID changed from ',OLD.c_id, ' To ',NEW.c_id);
    end if;
    if (old.c_course_type != new.c_course_type) THEN
        SET MSG         = CONCAT(MSG, ' CourseType changed from ',OLD.c_course_type, ' To ',NEW.c_course_type);
    end if;
    if (old.c_center != new.c_center) THEN
        SET MSG         = CONCAT(MSG, ' Centre changed from ',OLD.c_center, ' To ',NEW.c_center);
    end if;
    if (old.c_cancelled != new.c_cancelled) THEN
        SET MSG         = CONCAT(MSG, ' Cancelled changed from ',OLD.c_cancelled, ' To ',NEW.c_cancelled);
    end if;
    if (old.c_enrol_date != new.c_enrol_date) THEN
        SET MSG         = CONCAT(MSG, ' EnrolDate changed from ',OLD.c_enrol_date, ' To ',NEW.c_enrol_date);
    end if;

    if (old.c_date_change != new.c_date_change) THEN
        SET MSG         = CONCAT(MSG, ' DateChange changed from ',OLD.c_date_change, ' To ',NEW.c_date_change);
    end if;

    if (old.c_end != new.c_end) THEN
        SET MSG         = CONCAT(MSG, ' End changed from ',OLD.c_end, ' To ',NEW.c_end);
    end if;
    if (old.c_start != new.c_start) THEN
        SET MSG         = CONCAT(MSG, ' Start changed from ',OLD.c_start, ' To ',NEW.c_start);
    end if;
    if (old.c_processed != new.c_processed) THEN
        SET MSG         = CONCAT(MSG, ' Processed changed from ',OLD.c_processed, ' To ',NEW.c_processed);
    end if;
    if (old.c_name != new.c_name) THEN
        SET MSG         = CONCAT(MSG, ' Name changed from ',OLD.c_name, ' To ',NEW.c_name);
    end if;
    if (old.c_status_om != new.c_status_om) THEN
        SET MSG         = CONCAT(MSG, ' OM changed from ',OLD.c_status_om, ' To ',NEW.c_status_om);
    end if;
    if (old.c_status_nm != new.c_status_nm) THEN
        SET MSG         = CONCAT(MSG, ' NM changed from ',OLD.c_status_nm, ' To ',NEW.c_status_nm);
    end if;
    if (old.c_status_of != new.c_status_of) THEN
        SET MSG         = CONCAT(MSG, ' OF changed from ',OLD.c_status_of, ' To ',NEW.c_status_of);
    end if;
    if (old.c_status_nf != new.c_status_nf) THEN
        SET MSG         = CONCAT(MSG, ' NF changed from ',OLD.c_status_nf, ' To ',NEW.c_status_nf);
    end if;
    if (old.c_list_only != new.c_list_only) THEN
        SET MSG         = CONCAT(MSG, ' ListOnly changed from ',OLD.c_list_only, ' To ',NEW.c_list_only);
    end if;
    if (old.c_status_svr_m != new.c_status_svr_m) THEN
        SET MSG         = CONCAT(MSG, ' SVR M changed from ',OLD.c_status_svr_m, ' To ',NEW.c_status_svr_m);
    end if;
    
    
    
    
    if (old.c_status_svr_f != new.c_status_svr_f) THEN
        SET MSG         = CONCAT(MSG, ' SVR F changed from ',OLD.c_status_svr_f, ' To ',NEW.c_status_svr_f);
    end if;

    if (old.c_form_langs != new.c_form_langs) THEN
        SET MSG         = CONCAT(MSG, ' Form-Langs changed from ',OLD.c_form_langs, ' To ',NEW.c_form_langs);
    end if;

    if (old.c_comments != new.c_comments) THEN
        SET MSG         = CONCAT(MSG, ' Comments changed from ',OLD.c_comments, ' To ',NEW.c_comments);
    end if;

    if (old.c_description != new.c_description) THEN
        SET MSG         = CONCAT(MSG, ' Description changed from ',OLD.c_description, ' To ',NEW.c_description);
    end if;

    if (old.c_uri != new.c_uri) THEN
        SET MSG         = CONCAT(MSG, ' URI changed from ',OLD.c_uri, ' To ',NEW.c_uri);
    end if;

    if (old.c_deleted != new.c_deleted) THEN
        SET MSG         = CONCAT(MSG, ' Deleted changed from ',OLD.c_deleted, ' To ',NEW.c_deleted);
    end if;

    if (old.c_combined_seat_course != new.c_combined_seat_course) THEN
        SET MSG         = CONCAT(MSG, ' Combined-Seat-Course changed from ',OLD.c_combined_seat_course, ' To ',NEW.c_combined_seat_course);
    end if;

    if (old.c_at_m_count != new.c_at_m_count) THEN
        SET MSG         = CONCAT(MSG, ' AT-M-COUNT changed from ',OLD.c_at_m_count, ' To ',NEW.c_at_m_count);
    end if;

    if (old.c_at_f_count != new.c_at_f_count) THEN
        SET MSG         = CONCAT(MSG, ' AT-F-COUNT changed from ',OLD.c_at_f_count, ' To ',NEW.c_at_f_count);
    end if;

    if (old.c_at_m_conf != new.c_at_m_conf) THEN
        SET MSG         = CONCAT(MSG, ' AT-M-CONF changed from ',OLD.c_at_m_conf, ' To ',NEW.c_at_m_conf);
    end if;

    if (old.c_at_f_conf != new.c_at_f_conf) THEN
        SET MSG         = CONCAT(MSG, ' AT-F-CONF changed from ',OLD.c_at_f_conf, ' To ',NEW.c_at_f_conf);
    end if;

    if (old.c_finalized != new.c_finalized) THEN
        SET MSG         = CONCAT(MSG, ' Finalized changed from ',OLD.c_finalized, ' To ',NEW.c_finalized);
    end if;

    if (old.c_finalized_tstamp != new.c_finalized_tstamp) THEN
        SET MSG         = CONCAT(MSG, ' Finalized-Tstamp changed from ',OLD.c_finalized_tstamp, ' To ',NEW.c_finalized_tstamp);
    end if;

    if (old.c_status != new.c_status) THEN
        SET MSG         = CONCAT(MSG, ' Status changed from ',OLD.c_status, ' To ',NEW.c_status);
    end if;

    if (old.c_auto_confirm != new.c_auto_confirm) THEN
        SET MSG         = CONCAT(MSG, ' Auto-Confirm changed from ',OLD.c_auto_confirm, ' To ',NEW.c_auto_confirm);
    end if;    
    
    
    
    
    if (MSG != 'CHANGE') THEN

      INSERT INTO dh_log
      SET
          l_center        = new.c_center,
          l_module        = 'dh_course',
          l_identifier    = new.c_id,
          l_event         = new.c_cancelled,
          l_msg           = MSG,
          l_tstamp        = NOW(),
          l_user          = new.c_updated_by
          ;

    end if;    
    
    
    



END
$$
DELIMITER ;
