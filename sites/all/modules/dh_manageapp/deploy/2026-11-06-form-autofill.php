<?php
// Form autofill API (inc/autofill.inc): tables, indexes and settings. Command line only, from the Drupal root:
//   php sites/all/modules/dh_manageapp/deploy/2026-11-06-form-autofill.php                  tables + settings (switch stays off)
//   php sites/all/modules/dh_manageapp/deploy/2026-11-06-form-autofill.php --indexes        the a_email index on dh_applicant (online; gives up after 10 s of lock wait)
//   php sites/all/modules/dh_manageapp/deploy/2026-11-06-form-autofill.php --cleanup        delete codes/tokens older than 24 h and log rows older than 180 days
//   php sites/all/modules/dh_manageapp/deploy/2026-11-06-form-autofill.php --new-key FILE   a new API key: its hash into DIPI, the key into FILE (0600)
//   php sites/all/modules/dh_manageapp/deploy/2026-11-06-form-autofill.php --rollback       drop the tables, the settings and the index named a_email
// Running it again changes nothing that is already there.
if (php_sapi_name() !== 'cli') { echo "Command line only.\n"; exit(1); }
define('DRUPAL_ROOT', getcwd());
$_SERVER['REMOTE_ADDR'] = '127.0.0.1';
require_once DRUPAL_ROOT . '/includes/bootstrap.inc';
drupal_bootstrap(DRUPAL_BOOTSTRAP_VARIABLES);

$mode = isset($argv[1]) ? $argv[1] : '';

$tables = array(
	'dh_autofill_code' => "CREATE TABLE IF NOT EXISTS dh_autofill_code (
		afc_otp_id char(32) NOT NULL,
		afc_email varchar(255) NOT NULL DEFAULT '',
		afc_email_hash char(64) NOT NULL,
		afc_code_hash char(64) NOT NULL,
		afc_form varchar(64) NOT NULL,
		afc_client_ip varchar(45) NOT NULL DEFAULT '',
		afc_tries tinyint NOT NULL DEFAULT 0,
		afc_created int NOT NULL,
		afc_expires int NOT NULL,
		afc_used int NULL DEFAULT NULL,
		PRIMARY KEY (afc_otp_id),
		KEY afc_email_hash (afc_email_hash, afc_created),
		KEY afc_client_ip (afc_client_ip, afc_created),
		KEY afc_created (afc_created)
	) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci",
	'dh_autofill_token' => "CREATE TABLE IF NOT EXISTS dh_autofill_token (
		aft_hash char(64) NOT NULL,
		aft_email_hash char(64) NOT NULL,
		aft_form varchar(64) NOT NULL,
		aft_result varchar(16) NOT NULL,
		aft_source_app bigint NULL DEFAULT NULL,
		aft_client_ip varchar(45) NOT NULL DEFAULT '',
		aft_created int NOT NULL,
		aft_expires int NOT NULL,
		aft_used int NULL DEFAULT NULL,
		PRIMARY KEY (aft_hash),
		KEY aft_created (aft_created)
	) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci",
	'dh_autofill_log' => "CREATE TABLE IF NOT EXISTS dh_autofill_log (
		afl_id bigint NOT NULL AUTO_INCREMENT,
		afl_tstamp int NOT NULL,
		afl_endpoint varchar(16) NOT NULL,
		afl_email_hash char(64) NOT NULL DEFAULT '',
		afl_form varchar(64) NOT NULL DEFAULT '',
		afl_result varchar(24) NOT NULL,
		afl_client_ip varchar(45) NOT NULL DEFAULT '',
		afl_request_id varchar(64) NOT NULL DEFAULT '',
		PRIMARY KEY (afl_id),
		KEY afl_tstamp (afl_tstamp),
		KEY afl_result (afl_result, afl_tstamp)
	) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci",
);
$settings = array('autofill_enabled', 'autofill_api_key_hash', 'autofill_allowed_ips', 'autofill_test_email', 'autofill_hash_secret', 'autofill_mail_off', 'autofill_global_hourly_cap');

// An index whose FIRST column is a_email, under any name, counts as already there.
function af_email_index_exists()
{
	return (bool) db_query("select count(*) from information_schema.statistics where table_schema = database() and table_name = 'dh_applicant' and column_name = 'a_email' and seq_in_index = 1")->fetchField();
}
// An index with exactly this name on dh_applicant.
function af_index_named($name)
{
	return (bool) db_query("select count(*) from information_schema.statistics where table_schema = database() and table_name = 'dh_applicant' and index_name = :n", array(':n' => $name))->fetchField();
}

try
{
	if ($mode === '')
	{
		foreach ($tables as $name => $sql)
		{
			$had = db_table_exists($name);
			db_query($sql);
			echo $name . ($had ? ': already there' : ': created') . "\n";
		}
		if (strlen(variable_get('autofill_hash_secret', '')) != 64)
		{
			variable_set('autofill_hash_secret', bin2hex(random_bytes(32)));
			echo "autofill_hash_secret: set\n";
		}
		else
			echo "autofill_hash_secret: already set\n";
		if (variable_get('autofill_enabled', NULL) === NULL)
		{
			variable_set('autofill_enabled', 0);
			echo "autofill_enabled: 0 (off)\n";
		}
		else
			echo 'autofill_enabled: already ' . variable_get('autofill_enabled', 0) . "\n";
	}
	elseif ($mode === '--indexes')
	{
		if (af_email_index_exists())
			echo "index a_email: already there\n";
		else
		{
			// Never wait long for the table: a long query or a backup holding it must not stall DIPI behind this ALTER.
			try
			{
				db_query("SET SESSION lock_wait_timeout = 10");
				$t = microtime(TRUE);
				db_query("ALTER TABLE dh_applicant ADD INDEX a_email (a_email), ALGORITHM=INPLACE, LOCK=NONE");
				printf("index a_email: added in %.1f s\n", microtime(TRUE) - $t);
			}
			catch (Throwable $e)
			{
				echo 'FAILED: ' . $e->getMessage() . "\n";
				echo "DIPI was not changed. Retry when no long query or backup is running.\n";
				exit(1);
			}
		}
	}
	elseif ($mode === '--cleanup')
	{
		// The same rules as dh_autofill_cleanup() in inc/autofill.inc.
		$n1 = db_delete('dh_autofill_code')->condition('afc_created', time() - 86400, '<')->execute();
		$n2 = db_delete('dh_autofill_token')->condition('aft_created', time() - 86400, '<')->execute();
		$n3 = db_delete('dh_autofill_log')->condition('afl_tstamp', time() - 180 * 86400, '<')->execute();
		echo "dh_autofill_code: $n1 rows deleted\ndh_autofill_token: $n2 rows deleted\ndh_autofill_log: $n3 rows deleted\n";
	}
	elseif ($mode === '--new-key')
	{
		$file = isset($argv[2]) ? $argv[2] : '';
		if ($file === '' || file_exists($file))
		{
			echo "Give a new file name for the key (it must not exist yet).\n";
			exit(1);
		}
		$key = rtrim(strtr(base64_encode(random_bytes(32)), '+/', '-_'), '=');
		$old = umask(0077);
		$ok = file_put_contents($file, $key . "\n");
		umask($old);
		if ($ok === FALSE)
		{
			echo "Could not write $file; nothing changed.\n";
			exit(1);
		}
		chmod($file, 0600);
		variable_set('autofill_api_key_hash', hash('sha256', $key));
		echo "API key written to $file (mode 0600); its hash is stored in DIPI. The key is not shown here.\n";
	}
	elseif ($mode === '--rollback')
	{
		foreach (array_keys($tables) as $name)
		{
			db_query("DROP TABLE IF EXISTS $name");
			echo "$name: dropped\n";
		}
		foreach ($settings as $v)
			variable_del($v);
		echo "autofill settings: removed\n";
		if (af_index_named('a_email'))
		{
			try
			{
				db_query("SET SESSION lock_wait_timeout = 10");
				db_query("ALTER TABLE dh_applicant DROP INDEX a_email, ALGORITHM=INPLACE, LOCK=NONE");
				echo "index a_email: dropped\n";
			}
			catch (Throwable $e)
			{
				echo 'FAILED: ' . $e->getMessage() . "\n";
				echo "The index was not dropped; DIPI was not changed. Retry when no long query or backup is running.\n";
				exit(1);
			}
		}
	}
	else
	{
		echo "Unknown option $mode\n";
		exit(1);
	}
}
catch (Throwable $e)
{
	echo 'FAILED: ' . $e->getMessage() . "\n";
	exit(1);
}
exit(0);
