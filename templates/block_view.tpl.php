<?php if (!empty($tags)): ?>
<div class="tagcloud-tags-wrapper">
  <?php foreach ($tags as $key => $tag): ?>
    <span class="tag tag-<?php print $tag['font_size']; ?>" style="font-size: <?php print $tag['font_size']; ?>px;">
      <?php print l($tag['name'], 'taxonomy/term/'.$tag['tid']); ?>
    </span>
  <?php endforeach; ?>
</div>
<?php endif; ?>
