dnl $Id: config.m4,v 1.2 2001/07/02 04:23:17 danda Exp $
dnl config.m4 for extension yats
dnl don't forget to call PHP_EXTENSION(yats)

dnl Comments in this file start with the string 'dnl'.
dnl Remove where necessary. This file will not work
dnl without editing.

dnl If your extension references something external, use with:

dnl PHP_ARG_WITH(yats, for yats support,
dnl Make sure that the comment is aligned:
dnl [  --with-yats             Include yats support])

dnl Otherwise use enable:

PHP_ARG_ENABLE(yats, --enable-yats)

if test "$PHP_YATS" != "no"; then
  dnl If you will not be testing anything external, like existence of
  dnl headers, libraries or functions in them, just uncomment the 
  dnl following line and you are ready to go.
  AC_DEFINE(HAVE_YATS, 1, [ ])
  dnl Write more examples of tests here...
  PHP_EXTENSION(yats, $ext_shared)
fi
