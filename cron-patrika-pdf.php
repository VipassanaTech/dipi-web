<?php


if (isset($_REQUEST) && ( isset($_REQUEST['REMOTE_IP']) ))
{
   echo "I wont run from the web\n";
   exit(1);
}

define('DRUPAL_ROOT', getcwd());

require_once DRUPAL_ROOT . '/includes/bootstrap.inc';
drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);

$current_year = date("Y");
$start_year = 1965;
$patrika_links = array();

for ($year = $start_year ; $year <= $current_year ; $year++)
{
	$year_url = "https://www.vridhamma.org/newsletters?field_month_value%5Bvalue%5D%5Byear%5D=$year";

	$data = file_get_contents($year_url);

	$regex = '|<div class="field-item even">([\s\S]*?)</div>[\s\S]*?<td>([\s\S]*?)</td>[\s\S]*?<td>([\s\S]*?)</td>[\s\S]*?<td>([\s\S]*?)</td>[\s\S]*?<td>([\s\S]*?)</td>[\s\S]*?<td>([\s\S]*?)</td>[\s\S]*?<td>([\s\S]*?)</td>[\s\S]*?<td>([\s\S]*?)</td>[\s\S]*?<td>([\s\S]*?)</td>[\s\S]*?<td>([\s\S]*?)</td>[\s\S]*?<td>([\s\S]*?)</td>[\s\S]*?<td>([\s\S]*?)</td>[\s\S]*?<td>([\s\S]*?)</td>|';

	preg_match_all($regex,$data, $matches);
	
	$patrika_links[$year] = array();
	foreach($matches[1] as $key => $lang)
	{
		$patrika_links[$year][trim($lang)] = array();
		for ($i = 2 ; $i <= 13 ; $i++)
		{

			$temp = explode('"', $matches[$i][$key]);
			$patrika_links[$year][$lang][$i - 1] = trim($temp[1]);
		}

	}

}


$patrika_dir = "public:///patrika-pdf";
$patrika_recent = array();
$recent_dir = "$patrika_dir/recent";

if (file_prepare_directory($patrika_dir, FILE_CREATE_DIRECTORY))
{
  foreach($patrika_links as $year => $languages)
  {
      if(count($languages) > 0)
      {
      	$year_dir = "$patrika_dir/$year";
      	if (file_prepare_directory($year_dir, FILE_CREATE_DIRECTORY))
      	{
      		foreach($languages as $language => $months)
      		{
      			$lang_code = strtolower(db_query("select l_code from dh_languages where l_name=:l_name", array('l_name' => $language))->fetchField());
      			if ($lang_code)
      			{
	      			foreach($months as $month => $link)
	      			{
	      				if ($link <> '')
	      				{
	      					$patrika_recent[$lang_code] = $link;

	      					$file_name = "$year_dir/patrika-$lang_code-$month-$year.pdf";
	      					echo $file_name."\n";
	      					if (file_exists($file_name))
	      					{
	      						echo "file already present\n";
	      						$local_file_size = filesize($file_name);
	      						$head = array_change_key_case(get_headers($link, 1));
	      						$remote_file_size = $head['content-length'];

	      						if ($local_file_size <> $remote_file_size)
	      						{
	      							echo "file changed, downloading\n";
	      							system_retrieve_file($link, $file_name, false, $replace = FILE_EXISTS_REPLACE);
	      						}
	      						else
	      							echo "file not changed, not downloading\n";
	      					}
	      					else
	      					{
	      						echo "file not present, downloading\n";
	      						system_retrieve_file($link, $file_name, false, $replace = FILE_EXISTS_ERROR);
	      					}
	      					echo "\n";
	      				}
		      		}
      			}
      		}
      	}
      }
  }
}


if (file_prepare_directory($recent_dir, FILE_CREATE_DIRECTORY))
{
	foreach($patrika_recent as $lang_code => $link)
	{
		$file_name = "$recent_dir/patrika-$lang_code.pdf";
		echo $file_name."\n";
		if (file_exists($file_name))
		{
			echo "file already present\n";
			$local_file_size = filesize($file_name);
			$head = array_change_key_case(get_headers($link, 1));
			$remote_file_size = $head['content-length'];

			if ($local_file_size <> $remote_file_size)
			{
				echo "file changed, downloading\n";
				system_retrieve_file($link, $file_name, false, $replace = FILE_EXISTS_REPLACE);
			}
			else
				echo "file not changed, not downloading\n";
		}
		else
		{
			echo "file not present, downloading\n";
			system_retrieve_file($link, $file_name, false, $replace = FILE_EXISTS_ERROR);
		}
		echo "\n";
	}
}