-- Student "choose another teacher" link (Recommending AT Returned letter, 21 Nov 2026).
-- One-time code per return, made by dh_lc_teacher_letter() (inc/letter-protect.inc) and used once by the page on the
-- applicant site. Run BEFORE the code that uses it. Rollback: ALTER TABLE dh_applicant_lc DROP INDEX al_choose_auth, DROP COLUMN al_choose_auth;
ALTER TABLE dh_applicant_lc
  ADD COLUMN al_choose_auth varchar(40) NULL DEFAULT NULL,
  ADD INDEX al_choose_auth (al_choose_auth);
