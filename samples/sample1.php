<?php

$tmpl = yats_define("sample1.tmpl", '', getcwd());

$thing = $GLOBALS[HTTP_GET_VARS][thing];
if(!$thing) {
   $thing = "there";
}

if($tmpl) {
   yats_assign($tmpl, "thing", $thing);
   echo yats_getbuf($tmpl);
}
?>
