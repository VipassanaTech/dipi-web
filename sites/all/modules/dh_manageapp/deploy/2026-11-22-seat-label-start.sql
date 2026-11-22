-- Seat label start (22 Nov 2026): for one course, each grid (main / chowky) of each section of the
-- seating plans can start its labels later. One row per course + plan + section + grid; no row = normal start.
-- sls_start is an offset: style 1 letter index (I = 8), style 2 first column - 1, numbered first seat - 1.
-- Run BEFORE the code that uses it. Rollback: DROP TABLE dh_seat_label_start;
CREATE TABLE dh_seat_label_start (
  sls_id int unsigned NOT NULL AUTO_INCREMENT,
  sls_course int NOT NULL,
  sls_plan varchar(8) NOT NULL,
  sls_section varchar(32) NOT NULL,
  sls_grid varchar(8) NOT NULL,
  sls_conv tinyint NOT NULL,
  sls_start int NOT NULL DEFAULT 0,
  sls_updated_by int NOT NULL DEFAULT 0,
  sls_updated datetime NOT NULL,
  PRIMARY KEY (sls_id),
  UNIQUE KEY sls_key (sls_course, sls_plan, sls_section, sls_grid)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;
