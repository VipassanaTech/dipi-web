-- ============================================================================
-- AT course view: which side of a course a mapped teacher sees. Run once, from the command line:
--     drush sql-cli < sites/all/modules/dh_manageapp/deploy/2026-10-08-teacher-sees.sql
--
-- ct_sees: 'M' = male applicants, 'F' = female applicants, 'B' = both.
-- NULL = the teacher's own gender (the default, so existing mappings need no change).
-- Set by the centre on the Assign Teachers page ("Sees"); read by dh_atportal/view-courses.inc.
-- ============================================================================
SET SESSION lock_wait_timeout = 5;
ALTER TABLE dh_course_teacher ADD COLUMN ct_sees CHAR(1) NULL DEFAULT NULL AFTER ct_group;

-- One-time, at deploy: in courses that have already started (start date today or earlier) whose Confirmed
-- teachers are all of ONE gender (a single teacher, or several of the same gender), those teachers see Both
-- sides. Conducting and Assisting count the same; trainees, cancelled mappings and mappings whose teacher record
-- no longer exists are not counted. Courses that have not started are left to the centre (Assign Teachers,
-- "Sees"). Re-runnable: it only fills empty settings.
UPDATE dh_course_teacher ct
  JOIN dh_teacher t ON t.t_id = ct.ct_teacher
  JOIN dh_course co ON co.c_id = ct.ct_course
  JOIN (SELECT x.ct_course FROM dh_course_teacher x JOIN dh_teacher xt ON xt.t_id = x.ct_teacher
        WHERE x.ct_status = 'Confirmed' AND x.ct_type IN ('Conducting', 'Assisting')
        GROUP BY x.ct_course HAVING COUNT(DISTINCT xt.t_gender) = 1) one ON one.ct_course = ct.ct_course
  SET ct.ct_sees = 'B'
  WHERE ct.ct_status = 'Confirmed' AND ct.ct_type IN ('Conducting', 'Assisting') AND ct.ct_sees IS NULL
    AND co.c_start <= CURDATE() AND co.c_deleted = 0 AND co.c_cancelled = 0;
