<?php

/*******************
                              
A script to fetch gettext strings from a YATS template.
The strings must be in the special yats/gettext format:

{{text}}My literal string{{/text}}

 -- OR --
 
{{text parse="yes"}}My string with embedded YATS variables: {{myvar1}}, {{myvar2}}.{{/text}}

It uses stdin and stdout and takes no parameters.

Usage:

cat filename.tmpl | php yats-xgettext.php | xgettext -o file.po -


Note that this has been tested with the command line version of PHP executable only.
It is possible/likely that it would also work with the CGI version.
This is a simple script, so if you can't get either to work, then I'd recommend porting
it to python or perl.

Copyright Dan Libby, YATS 2004

********************/

$strings = parse_template( STDIN );

foreach( $strings as $str ) {
   $str = preg_replace ( '/\r?\n/', '\\n', $str );
   $str = preg_replace ( '/\"/', '\\"', $str );
   print 'gettext("' . $str . '")' . "\n";
}


function parse_template( $fh ) {

   $buf = '';
   while (!feof($fh)) {
     $buf .= fread($fh, 8192);
   }                      

   $pattern = '-{{text[^}}]*}}(.*){{/text}}-msU';
   preg_match_all( $pattern, $buf, $matches, PREG_PATTERN_ORDER );
   
   return $matches[1];
}
