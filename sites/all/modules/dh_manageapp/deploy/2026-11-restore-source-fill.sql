-- Restore of the 2020 migrated students: fill dh_restore_source from the old system's export.
-- Prod only (dipi_migration exists only there), as root, after deploy/2026-11-restore-source-table.php made the table:
--   sudo mysql dipi < sites/all/modules/dh_manageapp/deploy/2026-11-restore-source-fill.sql
-- Safe to run again: it empties the table first. Ends with two checks: rows (want 229164), old lines without a row (want 0).
TRUNCATE TABLE dh_restore_source;
DROP TEMPORARY TABLE IF EXISTS rs_cm;
CREATE TEMPORARY TABLE rs_cm ENGINE=MEMORY AS
  SELECT CourseID, 28499 + ROW_NUMBER() OVER (ORDER BY CourseID) AS c_id FROM dipi_migration.coursemaster;
ALTER TABLE rs_cm ADD PRIMARY KEY (CourseID);
INSERT INTO dh_restore_source (rs_src, rs_old_id, rs_course, rs_conf_no, rs_type, rs_f_name, rs_m_name, rs_l_name, rs_gender, rs_dob,
  rs_address, rs_zip, rs_country_name, rs_state_name, rs_city_name, rs_email, rs_phone_mobile, rs_phone_home, rs_phone_office, rs_phone_fax,
  rs_education, rs_occupation, rs_company, rs_post, rs_department, rs_mother_tongue, rs_other_langs,
  rs_health_physical, rs_health_mental, rs_medication, rs_intoxicants, rs_other_technique, rs_teach_others, rs_acco, rs_seat)
SELECT o.src, o.ID, cm.c_id, CONCAT(IFNULL(o.ConfirmationNumberPrefix, ''), ' ', IFNULL(o.ConfirmationNumber, '')), o.RegStudentType,
  o.FirstName, o.MiddleName, o.LastName, o.Gender, IF(YEAR(o.DOB) > 1900, DATE(o.DOB), NULL),
  o.Address, o.ZipOrPinCode, co.CountryName, st.StateName, ci.CityName, o.Email, o.PhoneMobile, o.PhoneHome, o.PhoneOffice, o.PhoneFax,
  o.Education, o.Occupation, o.Company, o.Post, o.Department, o.MotherTongue, o.OtherLanguagesKnown,
  o.HealthProblemPhysical, o.HealthProblemMental, o.Medication, o.intoxicants, o.OtherKnownTechniques, o.TeachOrPracticeOthTechniques,
  o.Accommodation, o.SeatNumber
FROM (
  SELECT 'b' AS src, ID, CourseID, ConfirmationNumberPrefix, ConfirmationNumber, RegStudentType, FirstName, MiddleName, LastName, Gender, DOB,
    Address, ZipOrPinCode, CountryID, StateID, CityID, Email, PhoneMobile, PhoneHome, PhoneOffice, PhoneFax, Education, Occupation, Company,
    Post, Department, MotherTongue, OtherLanguagesKnown, HealthProblemPhysical, HealthProblemMental, Medication, intoxicants,
    OtherKnownTechniques, TeachOrPracticeOthTechniques, Accommodation, SeatNumber
  FROM dipi_migration.part_b
  UNION ALL
  SELECT 'c', ID, CourseID, ConfirmationNumberPrefix, ConfirmationNumber, RegStudentType, FirstName, MiddleName, LastName, Gender, DOB,
    Address, ZipOrPinCode, CountryID, StateID, CityID, Email, PhoneMobile, PhoneHome, PhoneOffice, PhoneFax, Education, Occupation, Company,
    Post, Department, MotherTongue, OtherLanguagesKnown, HealthProblemPhysical, HealthProblemMental, Medication, intoxicants,
    OtherKnownTechniques, TeachOrPracticeOthTechniques, Accommodation, SeatNumber
  FROM dipi_migration.part_c
) o
JOIN rs_cm cm ON cm.CourseID = o.CourseID
LEFT JOIN dipi_migration.countrymaster co ON co.CountryID = o.CountryID
LEFT JOIN dipi_migration.statemaster st ON st.StateID = o.StateID
LEFT JOIN dipi_migration.citymaster ci ON ci.CityID = o.CityID;
SELECT COUNT(*) AS source_rows_want_229164 FROM dh_restore_source;
SELECT COUNT(*) AS old_lines_without_a_row_want_0 FROM dh_student_course sc
  WHERE sc.from_migration_script = 'Yes'
  AND NOT EXISTS (SELECT 1 FROM dh_restore_source r WHERE r.rs_course = sc.sc_course AND r.rs_conf_no = sc.sc_conf_no);
