-- ============================================================================
-- Per-course Main/Group plan selector  —  PROD schema migration
-- Branch: course-plan-selector
-- ============================================================================
-- RUN ONCE, and BEFORE the new code serves traffic. The OLD (current) prod code
-- keeps working after this runs — it simply ignores the new columns — so it is
-- safe to apply first and then `git pull`.
--
-- Column definitions match LOCAL exactly.
--   mysql <dbname> < 2026-09-17-course-plan-schema.sql
-- ============================================================================

-- c_*_plan: 0 = Main (default), 1 = Group-wise. One flag per resource.
ALTER TABLE dh_course
  ADD COLUMN c_cell_plan   tinyint(1) NOT NULL DEFAULT 0,
  ADD COLUMN c_dining_plan tinyint(1) NOT NULL DEFAULT 0,
  ADD COLUMN c_seat_plan   tinyint(1) NOT NULL DEFAULT 0;

-- Cache of the computed group-seating label, so seat-in-reports render the group
-- plan without recomputing client-side (populated by dh_render_seat_grid).
ALTER TABLE dh_applicant_attended
  ADD COLUMN aa_group_seat varchar(10) NULL DEFAULT NULL;
