-- ============================================================================
-- ROLLBACK ONLY: re-create the old dh_center_au trigger exactly as it was on prod
-- before 2026-10-02 (same body, same sql_mode). Not part of a normal deploy.
--   mysql <dbname> < 2026-10-02-dh_center_au-rollback.sql
-- ============================================================================
SET SESSION sql_mode = 'NO_AUTO_VALUE_ON_ZERO';
DELIMITER $$
CREATE TRIGGER dh_center_au AFTER UPDATE ON dh_center FOR EACH ROW
BEGIN
DECLARE STATUS VARCHAR(255);


	if (old.c_subdomain != new.c_subdomain) THEN
	SET STATUS 	= 'Domain register';
	else 
	SET STATUS	= 'Center Update';		
	END IF	; 

    INSERT INTO dh_log
    SET 
	l_center	= old.c_id,
	l_module 	= 'dh_Center',	
	l_identifier 	= new.c_id,
	l_event		= STATUS,
	l_msg		= 'Center Update',
	l_tstamp	= NOW(),
	l_user 		= old.c_updated_by;



END$$
DELIMITER ;
