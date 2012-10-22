dnl $Id$
dnl config.m4 for extension yats

dnl Comments in this file start with the string 'dnl'.
dnl Remove where necessary. This file will not work
dnl without editing.

dnl If your extension references something external, use with:

dnl PHP_ARG_WITH(yats, for yats support,
dnl Make sure that the comment is aligned:
dnl [  --with-yats             Include yats support])

dnl Otherwise use enable:

PHP_ARG_ENABLE(yats, whether to enable yats support,
Make sure that the comment is aligned:
[  --enable-yats           Enable yats support])

if test "$PHP_YATS" != "no"; then
  dnl Write more examples of tests here...

  dnl # --with-yats -> check with-path
  dnl SEARCH_PATH="/usr/local /usr"     # you might want to change this
  dnl SEARCH_FOR="/include/yats.h"  # you most likely want to change this
  dnl if test -r $PHP_YATS/$SEARCH_FOR; then # path given as parameter
  dnl   YATS_DIR=$PHP_YATS
  dnl else # search default path list
  dnl   AC_MSG_CHECKING([for yats files in default path])
  dnl   for i in $SEARCH_PATH ; do
  dnl     if test -r $i/$SEARCH_FOR; then
  dnl       YATS_DIR=$i
  dnl       AC_MSG_RESULT(found in $i)
  dnl     fi
  dnl   done
  dnl fi
  dnl
  dnl if test -z "$YATS_DIR"; then
  dnl   AC_MSG_RESULT([not found])
  dnl   AC_MSG_ERROR([Please reinstall the yats distribution])
  dnl fi

  dnl # --with-yats -> add include path
  dnl PHP_ADD_INCLUDE($YATS_DIR/include)

  dnl # --with-yats -> check for lib and symbol presence
  dnl LIBNAME=yats # you may want to change this
  dnl LIBSYMBOL=yats # you most likely want to change this 

  dnl PHP_CHECK_LIBRARY($LIBNAME,$LIBSYMBOL,
  dnl [
  dnl   PHP_ADD_LIBRARY_WITH_PATH($LIBNAME, $YATS_DIR/lib, YATS_SHARED_LIBADD)
  dnl   AC_DEFINE(HAVE_YATSLIB,1,[ ])
  dnl ],[
  dnl   AC_MSG_ERROR([wrong yats lib version or lib not found])
  dnl ],[
  dnl   -L$YATS_DIR/lib -lm
  dnl ])
  dnl
  dnl PHP_SUBST(YATS_SHARED_LIBADD)

  PHP_NEW_EXTENSION(yats, yats.c, $ext_shared)
fi
