<?php 
// $Id$
/**
 * @file
 *   Reusable API for l10n remote updates
 */

include_once './includes/locale.inc';
include_once 'l10n_update.locale.inc';

// Translation status: String imported from po
define('L10N_UPDATE_STRING_DEFAULT', 0);
// Translation status: Custom string, overridden original import
define('L10N_UPDATE_STRING_CUSTOM', 1);

/**
 * Download and import remote file
 */
function l10n_update_download_import($download_url, $locale, $mode = LOCALE_IMPORT_OVERWRITE) {
  if ($file = l10n_update_download_file($download_url)) {
    $result = l10n_update_import_file($file, $locale, $mode);
    file_delete($file);
    return $result;
  }
}

/**
 * Import local file into the database
 * @param $file
 * @param $locale
 * @param $mode
 * @return unknown_type
 */
function l10n_update_import_file($file, $locale, $mode = LOCALE_IMPORT_OVERWRITE) {   
  // If the file is a filepath, create a $file object
  if (is_string($file)) {
    $filepath = $file;
    $file = new Stdclass();
    $file->filepath = $filepath;
    $file->filename = $filepath;
  }
  return _l10n_update_locale_import_po($file, $locale, $mode , 'default');
}

/**
 * Get remote file and download it to a temporary path
 */
function l10n_update_download_file($download_url, $destination = NULL) {
  $t = get_t();
  $variables['%download_link'] = $download_url;
  
  $file = $destination ? $destination : tempnam(file_directory_temp(), 'translation-');

  if ($file) {
    $variables['%tmpfile'] = $file;
    if (($contents = drupal_http_request($download_url)) && file_put_contents($file, $contents->data)) {
      watchdog('l10n_update', 'Successfully downloaded %download_link to %tmpfile', $variables);
      return $file; 
    }
    else {
      watchdog('l10n_update', 'Unable to download and save %download_link file to %tmpfile.', $variables, WATCHDOG_ERROR);
    }
  }
  else {
    $variables['%tmpdir'] = file_directory_temp();
    watchdog('l10n_update', 'Error creating temporary file for download in %tmpdir. Remote file is %download_link.', $variables, WATCHDOG_ERROR);
  }  
}

/**
 * Get names for the language list from locale system
 */
function l10n_update_get_language_names($string_list) {
  $t = get_t();
  $language_codes = array_map('trim', explode(',', $string_list));
  $languages = _locale_get_predefined_list();
  $result = array();
  foreach ($language_codes as $lang) {      
    if (array_key_exists($lang, $languages)) {
      // Try to use verbose locale name
      $name = $lang;
      $name = $languages[$name][0] . (isset($languages[$name][1]) ? ' '. $t('(@language)', array('@language' => $languages[$name][1])) : '');
      $result[$lang] = $name;
    }
  }
  return $result;
}

/**
 * Get update information about single release
 */
function l10n_update_project_get_release($name, $langcode, $version, $server = NULL) {
  $projects[] = _l10n_update_build_project($name, $version, $server);
  if ($updates = _l10n_update_fetch_releases($projects, $langcode)) {
    return _l10n_update_project_release($updates[$name], $langcode, $version);
  }
}

/**
 * Compare and get list of downloadable updates
 */
 function _l10n_update_compare($projects, $history, $available) {
   $updates = array();
   $langcodes = array_keys(l10n_update_language_list());
   foreach ($projects as $project) {
     $name = $project['name'];
     $version = $project['info']['version'];
     foreach ($langcodes as $lang) {
       // If there is an available release for that project / version / language
       if (!empty($available[$name]) && ($compare = _l10n_update_project_release($available[$name], $lang, $version))) {
         $latest = !empty($history[$name]->updates[$lang]) ? $history[$name]->updates[$lang] : NULL;
         if (!$latest || $latest->download_version != $compare['tag'] || $latest->download_date < $compare['date']) {
           $updates[$name][$lang] = $compare;
         }
       }
     }
   }
   return $updates;
 }

/**
 * Build project data as an array
 */
function _l10n_update_build_project($name, $version = NULL, $server = NULL) {
  $project = array(
    'name' => $name,
    'info' => array(
      'version' => $version,
      'translate status url' => $server ? $server : variable_get('l10n_update_server', NULL),
    ),
  );
  return $project;
}

/**
 * Get a given version from project releases
 * 
 * This one tries to find a perfect match first and if not it returns
 * the first one which should be the most recent matching this version.
 * 
 * @param $project
 *   Project data from update status
 * @param $version
 * @return unknown_type
 */
function _l10n_update_project_release($project, $langcode, $version) {
  if ($version && !empty($project['releases'][$langcode][$version])) {
    return $project['releases'][$langcode][$version];
  }
  elseif (!empty($project['releases'][$langcode])) {
    return current($project['releases'][$langcode]);
  }
}

/**
 * Get remote information about releases
 * 
 * Reuse the parser from update.fetch.inc
 * 
 * @param $projects
 *   Array of l10n_update projects
 * @param $languages
 *   Single language code or array of language codes
 */
function _l10n_update_fetch_releases($projects, $languages) {
  require_once 'l10n_update.parser.inc';
  $site_key = l10n_update_get_site_key();
  $data = array();
  foreach ($projects as $project) {
    if ($url = _l10n_update_build_fetch_url($project, $languages, $site_key)) {
      $xml = l10n_update_http_request($url);
      if (isset($xml->data)) {
        $data[] = $xml->data;
      }
    }
  }
  if ($data) {
    $parser = new l10n_update_xml_parser;
    $available = $parser->parse($data);
    return $available;
  }  
}

/**
 * Generates the URL to fetch information about project updates.
 *
 * This figures out the right URL to use, based on the project's .info file
 * and the global defaults. Appends optional query arguments when the site is
 * configured to report usage stats.
 *
 * @param $project
 *   The array of project information from update_get_projects().
 * @param $languages
 *   Single language code or array of language codes
 * @param $site_key
 *   The anonymous site key hash (optional).
 *
 * @see update_refresh()
 * @see update_get_projects()
 */
function _l10n_update_build_fetch_url($project, $languages, $site_key = '') {
  // If multiple languages, langcode = es,en,..
  $langcode = is_array($languages) ? implode(',', $languages) : $languages;
  if (!isset($project['info']['translate status url'])) {
    $project['info']['translate status url'] = variable_get('l10n_update_server', NULL);
  }
  // If not url yet, return none
  if (empty($project['info']['translate status url'])) {
    return;
  }
  $name = $project['name'];
  $url = $project['info']['translate status url'];
  $url .= '/l10n/status/project/'. $name;
  if (!empty($site_key)) {
    $url .= (strpos($url, '?') === TRUE) ? '&' : '?';
    $url .= 'site_key=';
    $url .= drupal_urlencode($site_key);
    if (!empty($project['info']['version'])) {
      $url .= '&version=';
      $url .= drupal_urlencode($project['info']['version']);
    }
    $url .= '&language=';
    $url .= drupal_urlencode($langcode);
  }
  return $url;
}

/**
 * Create a batch to download and import
 */
function l10n_update_download_create_batch($url, $locale, $mode = LOCALE_IMPORT_OVERWRITE) {  
  $operations[] = array('_l10n_update_batch_download_import', array($url, $locale, $mode));
  //$operations[] = array('_l10n_update_batch_import', array($locale));
  return _l10n_update_create_batch($operations);
}

/**
 * Create batch stub for this module
 * 
 * @return $batch
 */
function _l10n_update_create_batch($operations = array()) {
  $t = get_t();
  $batch = array(
    'operations'    => $operations,
    'title'         => $t('Updating translation.'),
    'init_message'  => $t('Downloading and importing files.'),
    'error_message' => $t('Error importing interface translations'),
    'file'          => drupal_get_path('module', 'l10n_update') . '/l10n_update.inc',
    'finished'      => '_l10n_update_batch_finished',
  );
  return $batch;
}

/**
 * Create a big batch for multiple projects and languages
 * 
 * @param $projects
 *   Array of projects
 * @param $languages
 *   Array of language codes
 * @param $updates
 *   Optional array of available updates. will be calculated if not passed
 */
function l10n_update_batch_multiple($projects, $languages, $updates = NULL, $mode = LOCALE_IMPORT_OVERWRITE) {
  if (is_null($updates)) {
    $available = _l10n_update_fetch_releases($projects, $languages);
    $updates = _l10n_update_compare($projects, array(), $available);
  }
  foreach ($projects as $project) {
    foreach ($languages as $lang) {
      if (!empty($updates[$project['name']][$lang])) {
        $release = $updates[$project['name']][$lang];
        $operations[] = array('_l10n_update_batch_download_import', array($release['download_link'], $lang, $mode));
        $operations[] = array('_l10n_update_batch_history', array($project, $release));
      }
    }
  }
  if (!empty($operations)) {
    return _l10n_update_create_batch($operations);
  }
}

/**
 * Some complex batch callback to handle the full download and import
 * 
 * Done this way so we pass variables from one step to the next
 * and we can better handle errors
 * 
 * I love batch processes ;-)
 */
function _l10n_update_batch_download_import($url, $locale, $mode, &$context) {
  $t = get_t();
  if (!isset($context['sandbox']['step'])) {
    $context['sandbox']['step'] = 0;
  }

  switch ($context['sandbox']['step']) {
    case 0: // Download message (for multiple downloads batch)
      // This message may never show up if next step takes less than a second?
      $context['message'] = $t('Downloading remote file from %url', array('%url' => $url));
      $context['sandbox']['step'] = 1;
      $context['finished'] = 0.1;
      break;
    case 1: // Download      
      if ($file = l10n_update_download_file($url)) {
        $context['sandbox']['file'] = $file;
        $context['message'] = $t('Importing downloaded translation: %url.', array('%url' => $url));
        $context['sandbox']['step'] = 2;
        $context['results'][] = $url;
      }
      else {
        $context['sandbox']['step'] = 10;
      }
      $context['finished'] = 0.5;      
      break;
    case 2: // Import
      if (!empty($context['sandbox']['file'])) {
        $file = $context['sandbox']['file'];
        $context['results'][] = $file;
        if (l10n_update_import_file($file, $locale, $mode)) {
          $context['message'] = $t('Imported translation file.');
          $context['finished'] = 1;
          drupal_set_message($t('Successfully downloaded and imported translation from %url', array('%url' => $url)));
        }
        else {
          $context['sandbox']['step'] = 10;
        }
        file_delete($file);
      }
      else {
        // This should not happen, just in case
        $context['sandbox']['step'] = 10;
      }
      
      break;    
    case 10: // Error
    default: // In case something goes really wrong
      $context['finished'] = 1;
      drupal_set_message($t('The download and import operation failed: %url', array('%url' => $url)), 'error');
      break;
  }      
}

function _l10n_update_batch_download($url, &$context) {
  $t = get_t();
  if ($file = l10n_update_download_file($url)) {
    $context['results'][] = $file;
  }
  else {
    drupal_set_message($t('Failed download of %url', array('%url' => $url)), 'error');
  }
}

/**
 * Batch process: Update the download history table
 */
function _l10n_update_batch_history($project, $release, &$context) {
  l10n_update_download_history($project, $release);
}

/**
 * Update the download history table
 * 
 * @param $project
 *   Project object
 * @param $release
 *   Release object
 * @param $context
 *   Batch context
 */
function l10n_update_download_history($project, $release) {
  $save = array(
    'project' => $project['name'],
    'language' => $release['language'],
    'import_date' => time(),
    'download_date' => $release['date'],
    'download_version' => $release['tag'],
    'download_link' => $release['download_link']
  );
  // Update or write new record
  if (db_result(db_query("SELECT project FROM {l10n_update_download} WHERE project = '%s' AND language = '%s'", $save['project'], $save['language']))) {
    $update = array('project', 'language');
  }
  else {
    $update = array();
  }
  return drupal_write_record('l10n_update_download', $save, $update);
}


/**
 * Batch import translation file
 * 
 * This takes a file parameter or continues from previous batch which should have downloaded a file
 * 
 * @param $file
 *   File to be imported. If empty, the file will be taken from $context['results']
 * @param $locale
 * @param $mode
 * @param $context
 */
function _l10n_update_batch_import($file, $locale, $mode, &$context) {
  $t = get_t();
  
  if (!$file && !empty($context['results'])) {
    $file = array_shift($context['results']);
  }
  if ($file) {
    if (l10n_update_import_file($file, $locale, $mode)) {
      drupal_set_message($t('Imported translation file %name.', array('%name' => $file->filepath)));
      file_delete($file);
    }
    else {
      drupal_set_message($t('Failed import of translation file %name.', array('%name' => $file->filepath)), 'error');
    }
  }
}

/**
 * Batch finished callback, set result message
 * 
 * @param $success
 * @param $results
 * @return unknown_type
 */
function _l10n_update_batch_finished($success, $results) {
  $t = get_t();
  if ($success) {
    drupal_set_message($t('Successfully imported translations.'));
  }
  else {
    drupal_set_message($t('Error importing translations.'), 'error');
  }
}

/**
 * Fetch project info via XML from a central server.
 */
function _l10n_update_refresh() {
  static $fail = array();
  global $base_url;

  $available = array();
  $data = array();
  $projects = l10n_update_get_projects();
  $languages = l10n_update_language_list();
  $locales = implode(',', array_keys($languages));
  // Refresh history too
  l10n_update_get_history(TRUE);
  // Now that we have the list of projects, we should also clear our cache of
  // available release data, since even if we fail to fetch new data, we need
  // to clear out the stale data at this point.
  cache_set('l10n_update_available_releases', NULL);
  $max_fetch_attempts = variable_get('update_max_fetch_attempts', UPDATE_MAX_FETCH_ATTEMPTS);
  
  $available = _l10n_update_fetch_releases($projects, $locales);
  
  if (!empty($available) && is_array($available)) {
    $frequency = variable_get('l10n_update_check_frequency', 1);
    cache_set('l10n_update_available_releases', $available, 'cache', time() + (60 * 60 * 24 * $frequency));
    watchdog('l10n_update', 'Attempted to fetch information about all available new releases and updates.', array(), WATCHDOG_NOTICE, l(t('view'), 'admin/reports/updates'));
  }
  else {
    watchdog('l10n_update', 'Unable to fetch any information about available new releases and updates.', array(), WATCHDOG_ERROR, l(t('view'), 'admin/reports/updates'));
  }
  // Whether this worked or not, we did just (try to) check for updates.
  variable_set('l10n_update_last_check', time());
  return $available;
}

/**
 * Perform an HTTP request. Installer safe simplified version.
 * 
 * We cannot use drupal_http_request() at install, see http://drupal.org/node/527484
 * 
 * This is a flexible and powerful HTTP client implementation. Correctly handles
 * GET, POST, PUT or any other HTTP requests. Handles redirects.
 *
 * @param $url
 *   A string containing a fully qualified URI.
 * @param $headers
 *   An array containing an HTTP header => value pair.
 * @param $method
 *   A string defining the HTTP request to use.
 * @param $data
 *   A string containing data to include in the request.
 * @param $retry
 *   An integer representing how many times to retry the request in case of a
 *   redirect.
 * @return
 *   An object containing the HTTP request headers, response code, headers,
 *   data and redirect status.
 */
function l10n_update_http_request($url, $headers = array(), $method = 'GET', $data = NULL, $retry = 3) {
  $result = new stdClass();

  // Parse the URL and make sure we can handle the schema.
  $uri = parse_url($url);

  if ($uri == FALSE) {
    $result->error = 'unable to parse URL';
    return $result;
  }

  if (!isset($uri['scheme'])) {
    $result->error = 'missing schema';
    return $result;
  }

  switch ($uri['scheme']) {
    case 'http':
      $port = isset($uri['port']) ? $uri['port'] : 80;
      $host = $uri['host'] . ($port != 80 ? ':'. $port : '');
      $fp = @fsockopen($uri['host'], $port, $errno, $errstr, 15);
      break;
    case 'https':
      // Note: Only works for PHP 4.3 compiled with OpenSSL.
      $port = isset($uri['port']) ? $uri['port'] : 443;
      $host = $uri['host'] . ($port != 443 ? ':'. $port : '');
      $fp = @fsockopen('ssl://'. $uri['host'], $port, $errno, $errstr, 20);
      break;
    default:
      $result->error = 'invalid schema '. $uri['scheme'];
      return $result;
  }

  // Make sure the socket opened properly.
  if (!$fp) {
    // When a network error occurs, we use a negative number so it does not
    // clash with the HTTP status codes.
    $result->code = -$errno;
    $result->error = trim($errstr);

    // Mark that this request failed. This will trigger a check of the web
    // server's ability to make outgoing HTTP requests the next time that
    // requirements checking is performed.
    // @see system_requirements()
    // variable_set('drupal_http_request_fails', TRUE);

    return $result;
  }

  // Construct the path to act on.
  $path = isset($uri['path']) ? $uri['path'] : '/';
  if (isset($uri['query'])) {
    $path .= '?'. $uri['query'];
  }

  // Create HTTP request.
  $defaults = array(
    // RFC 2616: "non-standard ports MUST, default ports MAY be included".
    // We don't add the port to prevent from breaking rewrite rules checking the
    // host that do not take into account the port number.
    'Host' => "Host: $host",
    'User-Agent' => 'User-Agent: Drupal (+http://drupal.org/)',
    'Content-Length' => 'Content-Length: '. strlen($data)
  );

  // If the server url has a user then attempt to use basic authentication
  if (isset($uri['user'])) {
    $defaults['Authorization'] = 'Authorization: Basic '. base64_encode($uri['user'] . (!empty($uri['pass']) ? ":". $uri['pass'] : ''));
  }

  // If the database prefix is being used by SimpleTest to run the tests in a copied
  // database then set the user-agent header to the database prefix so that any
  // calls to other Drupal pages will run the SimpleTest prefixed database. The
  // user-agent is used to ensure that multiple testing sessions running at the
  // same time won't interfere with each other as they would if the database
  // prefix were stored statically in a file or database variable.
  if (preg_match("/simpletest\d+/", $GLOBALS['db_prefix'], $matches)) {
    $defaults['User-Agent'] = 'User-Agent: ' . $matches[0];
  }

  foreach ($headers as $header => $value) {
    $defaults[$header] = $header .': '. $value;
  }

  $request = $method .' '. $path ." HTTP/1.0\r\n";
  $request .= implode("\r\n", $defaults);
  $request .= "\r\n\r\n";
  $request .= $data;

  $result->request = $request;

  fwrite($fp, $request);

  // Fetch response. If in test mode, don't fetch data;
  $response = '';
  while (!feof($fp) && $chunk = fread($fp, 1024)) {
    $response .= $chunk;
  }
  fclose($fp);

  // Parse response.
  list($split, $result->data) = explode("\r\n\r\n", $response, 2);
  $split = preg_split("/\r\n|\n|\r/", $split);

  list($protocol, $code, $text) = explode(' ', trim(array_shift($split)), 3);
  $result->headers = array();

  // Parse headers.
  while ($line = trim(array_shift($split))) {
    list($header, $value) = explode(':', $line, 2);
    if (isset($result->headers[$header]) && $header == 'Set-Cookie') {
      // RFC 2109: the Set-Cookie response header comprises the token Set-
      // Cookie:, followed by a comma-separated list of one or more cookies.
      $result->headers[$header] .= ','. trim($value);
    }
    else {
      $result->headers[$header] = trim($value);
    }
  }

  $responses = array(
    100 => 'Continue', 101 => 'Switching Protocols',
    200 => 'OK', 201 => 'Created', 202 => 'Accepted', 203 => 'Non-Authoritative Information', 204 => 'No Content', 205 => 'Reset Content', 206 => 'Partial Content',
    300 => 'Multiple Choices', 301 => 'Moved Permanently', 302 => 'Found', 303 => 'See Other', 304 => 'Not Modified', 305 => 'Use Proxy', 307 => 'Temporary Redirect',
    400 => 'Bad Request', 401 => 'Unauthorized', 402 => 'Payment Required', 403 => 'Forbidden', 404 => 'Not Found', 405 => 'Method Not Allowed', 406 => 'Not Acceptable', 407 => 'Proxy Authentication Required', 408 => 'Request Time-out', 409 => 'Conflict', 410 => 'Gone', 411 => 'Length Required', 412 => 'Precondition Failed', 413 => 'Request Entity Too Large', 414 => 'Request-URI Too Large', 415 => 'Unsupported Media Type', 416 => 'Requested range not satisfiable', 417 => 'Expectation Failed',
    500 => 'Internal Server Error', 501 => 'Not Implemented', 502 => 'Bad Gateway', 503 => 'Service Unavailable', 504 => 'Gateway Time-out', 505 => 'HTTP Version not supported'
  );
  // RFC 2616 states that all unknown HTTP codes must be treated the same as the
  // base code in their class.
  if (!isset($responses[$code])) {
    $code = floor($code / 100) * 100;
  }

  switch ($code) {
    case 200: // OK
    case 304: // Not modified
      break;
    case 301: // Moved permanently
    case 302: // Moved temporarily
    case 307: // Moved temporarily
      $location = $result->headers['Location'];

      if ($retry) {
        $result = l10n_update_http_request($result->headers['Location'], $headers, $method, $data, --$retry);
        $result->redirect_code = $result->code;
      }
      $result->redirect_url = $location;

      break;
    default:
      $result->error = $text;
  }

  $result->code = $code;
  return $result;
}

/**
 * Get a unique key for this site, not identifiable but usable to track the requests
 * from the same server.
 * 
 * We cannot use the private key before the database is created.
 * 
 * @return
 *   The site key.
 */
function l10n_update_get_site_key() {
  global $base_url;
  
  return md5('l10n_update' . $base_url . $_SERVER['SERVER_NAME'] . $_SERVER['SERVER_ADDR']);
}
