<div id="comment-<?php print $id; ?>" class="comment<?php print ($comment->new) ? ' comment-new' : '' ?><?php print ($comment->uid != 1) ? ' comment-node-author' : '' ?> clear-block">
<?php if ($comment->new) { ?>
  <a id="new"></a>
  <span class="new"><?php print $new ?></span>
<?php } ?>
  <?php print $picture ?>
<h3>
    <a class="id" href="#comment-<?php print $comment->cid; ?>">#<?php print $id ?></a>
    <span class="author"><?php print $author; ?></span><?php if (variable_get('comment_subject_field_' . $node->type, false)): ?>
  : <span class="title"><?php print $title; ?></span>
  <?php endif; ?></h3>
  <div class="content"><?php print $content ?></div>
  <?php if ($signature): ?>
    <div class="user-signature clear-block">
      <?php print $signature ?>
    </div>
  <?php endif; ?>
  <?php if ($picture) { ?>
    <br class="clear" />
  <?php } ?>
  <div class="submitted"><?php print format_date($comment->timestamp, 'small') ?></div>
  <?php print $links ?>
</div>
