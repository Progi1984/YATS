dnl $Id: config.m4,v 1.1 2001/06/28 19:14:32 danda Exp $
dnl config.m4 for extension tmpl
dnl don't forget to call PHP_EXTENSION(tmpl)

dnl Comments in this file start with the string 'dnl'.
dnl Remove where necessary. This file will not work
dnl without editing.

dnl If your extension references something external, use with:

dnl PHP_ARG_WITH(tmpl, for tmpl support,
dnl Make sure that the comment is aligned:
dnl [  --with-tmpl             Include tmpl support])

dnl Otherwise use enable:

PHP_ARG_ENABLE(tmpl, --enable-tmpl)

if test "$PHP_TMPL" != "no"; then
  dnl If you will not be testing anything external, like existence of
  dnl headers, libraries or functions in them, just uncomment the 
  dnl following line and you are ready to go.
  AC_DEFINE(HAVE_TMPL, 1, [ ])
  dnl Write more examples of tests here...
  PHP_EXTENSION(tmpl, $ext_shared)
fi