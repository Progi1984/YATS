<?php
echo "<!-- API-DOC.html is generated from API-DOC.php.  Do not edit the html file directly -->";
echo "<html><head><title>xmlrpc-epi-php API reference</title></head><body>";

$api = array(methods => 
      array(
         array(id => "encoding",
               title => "yats API",
               desc => "these functions are used to load template files, assign variables into them ". 
                       "and interpolate the results, thus creating usable output",
               notes => "",
               methods => array(
                  array(method => "yats_define",
                        desc => "load (and parse) a template",
                        sig => "handle yats_define(string filename)",
                        ret => "template handle, or false if error",
                        args => array(
                           'filename' => "path/filename of the template to be loaded"
                           )
                  ),
                  array(method => yats_assign,
                        desc => "assign php variable(s) into template namespace for potential use",
                        sig => "bool yats_assign(handle template, string key, mixed value) -- or --<br>" .
                               "bool yats_assign(handle template, array vars)",
                        ret => "true on success, else false",
                        args => array(
                           'template' => "template handle, as returned from yats_define()",
                           'key'   => "variable name",
                           'value' => "variable value (scalar)",
                           'vars'  => "list of variable name/value pairs.  error if value without matching key is found."
                           )
                  ),
                  array(method => "yats_getbuf",
                        desc => "inserts assigned variables into template as appropriate and returns buffer. If a locale is given and gettext is properly configured and template contains {{text}}..{{/text}} tags, then this function will return localized strings.",
                        sig => "string yats_getbuf(handle template [, string locale [, string gettext_domain [, string gettext_dir]]])",
                        ret => "buffer containing fully processed template output",
                        args => array(
                           'template' => "template handle, as returned from yats_define()",
                           'locale' => 'code for a valid locale installed on your O/S, eg es_ES. See GNU gettext documentation.',
                           'gettext_domain' => 'filename of gettext messages file, without the path or extension.  ie: messages, NOT messages.po.  See GNU gettext documentation.',
                           'gettext_dir' => 'root directory in which locale specific catalog directories are located. If ommitted default paths are searched by gettext. See GNU gettext documentation.'
                           )
                  ),
                  array(method => "yats_hide",
                        desc => "sets hidden state of a named template section",
                        sig => "bool yats_hide(handle template, string section, bool hide [, int row])",
                        ret => "true on success, else false",
                        args => array(
                           'template' => "template handle, as returned from yats_define()",
                           'section' => 'name of the section to hide/show',
                           'hide' => "if true, hides the section, else displays it",
                           'row' => 'Optional. While looping, hide/show this section for specific row only.  1 based.'
                           )
                  ),
                  array(method => "yats_getvars",
                        desc => "returns list of variable names/values assigned to template thus far",
                        sig => "array yats_getvars(handle template)",
                        ret => "array on success, else false",
                        args => array(
                           'template' => "template handle, as returned from yats_define()"
                           )
                  )
              )
          )
      )
   );

// echo "<center><h1>contents</h1></center>";

foreach($api[methods] as $section) {
   $id = $section[id];
   $title = $section[title] ? $section[title] : $id;
   echo "<h1>$title</h1><ul>";
   foreach($section[methods] as $meth) {
      $method = $meth[method];
      echo "<li><a href='#$method'>$method</a>\n";
   }
   echo "</ul>\n";
}

if($api[data]) {
   echo "<h1><a href='#data'>data structures</a></h1><ul>";
   foreach($api[data] as $data) {
     $id = $data[id];
     echo "<li><a href='#$id'>$id</a>\n";
   }
   echo "</ul>";
}


// echo "<center><h1>API Reference</h1></center>";

foreach($api[methods] as $section) {
   $id = $section[id];
   $title = $section[title] ? $section[title] : $id;
   $desc = $section[desc] ? "<p>" . $section[desc] . "</p>" : "";
//   echo "<h1><a name='$id'>$title</a></h1>$desc<blockquote>";
   foreach($section[methods] as $meth) {
      $method = $meth[method];
      $desc = $meth[desc];
      $sig = $meth[sig];
      $ret = $meth[ret];
      echo <<< END
 <h3><a name='$method'>$sig</a></h3>
 <blockquote>
 $desc
 <p>
 <b>returns:</b> $ret
 <p>
END;
      if($meth[args]) {
         echo "<b>args:</b></br><blockquote>";
         foreach($meth[args] as $arg => $val) {
            echo "<b>$arg:</b> $val<br>";
         }
         echo "</blockquote>";
      }
      echo "<br><br>";

      echo "</blockquote>\n";
   }
   echo "</blockquote>\n";
}

if($api[data]) {
   echo "<h1><a name='data'>data structures</a></h1>";
   foreach($api[data] as $data) {
      $id = $data[id];
      $type = $data[type];
      $desc = $data[desc];
      $xmp = $data[example];
   
      echo "<h3><a name='$id'>$id</a></h3><blockquote>";
      echo "<b>type: $type</b><br>";
      if($desc) {
         echo "$desc<br>";
      }
      echo "<br><b>values:</b><blockquote>";
   
   
      foreach($data[vals] as $val => $desc) {
         echo "<b>$val:</b> $desc<br><br>";
      }
      echo "</blockquote>";
   
      if($xmp) {
         echo "<h3>example usage</h3>";
         echo "<xmp>\n$xmp\n</xmp>";
      }
   
      echo "</blockquote>";
   }
}

?>
</html>
