<?php 
/* Id */
/**
 * Extends the update parser to work with releases
 * 
 * The update parser uses version tag to index releases. We will use 'language' and 'tag'
 * 
 * @todo Parse languages too
 * 
 * @todo Update the server side and get rid of this
 */
// We do the include this way to be installer safe.
require_once './modules/update/update.fetch.inc';

class l10n_update_xml_parser extends update_xml_parser {
  var $current_language;

/*  
  function start($parser, $name, $attr) {
    $this->current_tag = $name;
    switch ($name) {
      case 'LANGUAGE':
        unset($this->current_object);
        $this->current_language = array();
        $this->current_object = &$this->current_language;
        break;
      default:
        parent::start($parser, $name, $attr);
    }
  }
*/
  function end($parser, $name) {
    switch ($name) {
      case 'RELEASE':
        unset($this->current_object);        
        $langcode = isset($this->current_release['langcode']) ? $this->current_release['langcode'] : $this->current_release['language'];
        $this->current_project['releases'][$langcode][$this->current_release['tag']] = $this->current_release;
        break;
      /*
      case 'LANGUAGE':
        $this->current_project['languages'][$this->current_language['langcode']] = $this->current_language;
        break;
      case 'LANGUAGES':
        $this->current_object = &$this->current_project;
        break;
      */
      default:
        parent::end($parser, $name);
    }
  }
/*
  function data($parser, $data) {
    if ($this->current_tag && !in_array($this->current_tag, array('LANGUAGE', 'LANGUAGES'))) {
      parent::data($parser, $data);
    }
  }
  */
}