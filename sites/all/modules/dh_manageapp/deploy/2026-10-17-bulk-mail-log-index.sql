-- ============================================================================
-- Bulk mail: the mail consumer now checks dh_bulk_mail_log before sending, so a message handed
-- back by RabbitMQ is not sent twice (bulk mail 676, 17 Oct 2026). This index keeps that check
-- fast (521,878 log rows on prod, only the primary key until now). Built online, no table lock.
-- Run once, before restarting the mail consumers:
--     drush sql-cli < sites/all/modules/dh_manageapp/deploy/2026-10-17-bulk-mail-log-index.sql
-- ============================================================================

ALTER TABLE dh_bulk_mail_log
  ADD INDEX bml_bulk_mail_applicant (bml_bulk_mail, bml_applicant),
  ALGORITHM=INPLACE, LOCK=NONE;
