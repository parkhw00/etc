<?php

setlocale(LC_CTYPE, 'en_US.UTF-8');
//date_default_timezone_set ('Etc/GMT-9');

$debug_message = '';
$error_message = '';
$output = '';

//$pwd = $_ENV["PWD"];
$pwd = dirname (__FILE__);
//$data_dir = dirname (__FILE__) . '/d';

function error ($message)
{
	global $error_message;

	$error_message .= $message.'<br />';
}

function debug ($message)
{
	global $debug_message;

	$debug_message .= $message.'<br />';
}

function error_handler($errno, $errstr, $errfile, $errline)
{
	$errortype = array (
		E_ERROR              => 'Error',
		E_WARNING            => 'Warning',
		E_PARSE              => 'Parsing Error',
		E_NOTICE             => 'Notice',
		E_CORE_ERROR         => 'Core Error',
		E_CORE_WARNING       => 'Core Warning',
		E_COMPILE_ERROR      => 'Compile Error',
		E_COMPILE_WARNING    => 'Compile Warning',
		E_USER_ERROR         => 'User Error',
		E_USER_WARNING       => 'User Warning',
		E_USER_NOTICE        => 'User Notice',
		E_STRICT             => 'Runtime Notice',
		E_RECOVERABLE_ERROR  => 'Catchable Fatal Error'
	);

	error ($errfile.':'.$errline.': '.'['.$errortype[$errno].'('.$errno.')] '.$errstr);

	return false;
}

// set to the user defined error handler
$old_error_handler = set_error_handler("error_handler");

function l($disp, $addr, $args=array())
{
	$ret = '<a href="'.$addr;

	if (count ($args))
	{
		$ret .= '?';
		foreach ($args as $name => $value)
			$ret .= $name.'='.$value.'&';
	}

	$ret .= '">'.$disp.'</a>';

	return $ret;
}


if (file_exists ('config.php'))
{
	include ('config.php');
}
else
	$config_mirror_dir = '.';

$dir = opendir ($config_mirror_dir);
if ($dir)
{
	$output .= '<table>';
	$output .= '<tr>';
	$output .= '<th>url';
	$output .= '<th>status';
	while (true)
	{
		$file = readdir ($dir);
		if ($file === false)
			break;
		debug ("filename: $file");
		if (substr($file, -7) == '.status')
		{
			$target = substr ($file, 0, -7);
			$origin = file_get_contents ("$config_mirror_dir/$target.origin");
			$status = file_get_contents ("$config_mirror_dir/$target.status");
			debug ("target: $target");

			$output .= '<tr>';
			$output .= "<td><a href=\"$origin\">$origin</a>";
			$output .= "<td>$status";
			#$output .= "<td><a href=\"".l('";
		}
	}
	$output .= '</table>';
}

function get($name, $default=False)
{
	if (isset ($_REQUEST[$name]))
		return $_REQUEST[$name];
	else
		return $default;
}

$do = get('do', 'nothing');
$url_default = 'git://example.com/repo.git';
$url = get ('url', $url_default);

debug ("do : $do");
if ($do == "clone")
{
	if ($url == $url_default)
		error ('default url');
	else
	{
		$log = "/dev/null";
		$cmd = "cd $config_mirror_dir && $pwd/fast_cmd clone $url > $log 2>&1";
		debug ("do clone.. cmd: $cmd");
		exec ($cmd, $out);
		foreach ($out as $row)
			debug ("out: $row");
	}
}

$output .= '<form action="." method="post">';
$output .= '<label for="url">add mirror:</label>';
$output .= "<input type=\"text\" id=\"url\" name=\"url\" value=\"$url_default\">";
$output .= '<input type="hidden" id="do" name="do" value="clone">'; 
$output .= '<input type="submit">'; 
$output .= '</form>';

if (isset ($output) || isset ($debug_message) || isset ($error_message))
{
	echo '<html><head>';
	echo '<meta name="viewport" ';
	echo 'content="user-scalable=no, initial-scale=1.0, maximum-scale=1.0, minimum-scale=1.0, width=device-width" />';
	echo '<style type="text/css">';
	echo 'td.list_size { text-align: right; }';
	echo 'table { border-collapse: collapse; }';
	echo 'table tr td { border: 1px solid black; }';
	echo '</style>';
	echo '</head><body>';

	if ($error_message != '')
	{
		echo '<div id="error">', $error_message, '</div>';
	}

	if ($debug_message != '')
	{
		echo '<div id="debug">', $debug_message, '</div>';
	}

	if (isset ($output))
		echo $output;

	echo '</body></html>';
}


?>
