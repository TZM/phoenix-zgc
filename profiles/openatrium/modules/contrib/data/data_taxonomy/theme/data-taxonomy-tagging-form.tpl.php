<?php
// $Id: data-taxonomy-tagging-form.tpl.php,v 1.1.2.2 2010/01/21 21:38:13 yhahn Exp $
?>
<div class='data-taxonomy-tagging-form'>
  <label><?php print $label ?></label>
  <div class='data-taxonomy-tags clear-block'>
    <?php print $edit ?>
    <div id='data-taxonomy-tags-<?php print $form['vid']['#value'] ?>-<?php print $form['id']['#value'] ?>'><?php print $tags ?></div>
  </div>
  <div class='data-taxonomy-form clear-block'>
    <?php print drupal_render($form); ?>
  </div>
</div>
