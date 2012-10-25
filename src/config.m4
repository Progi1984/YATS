dnl config.m4 for extension yats

PHP_ARG_ENABLE(yats, whether to enable yats support,
Make sure that the comment is aligned:
[  --enable-yats           Enable yats support])

if test "$PHP_YATS" != "no"; then
  PHP_NEW_EXTENSION(yats, yats.c \
  						  yats_hash.c \
  						  yats_utils.c \
  						  yats_list.c \
  						  yats_token.c \
  						  yats_string.c \
  						  yats_parser.c \
  						  yats_cache.c \
  						  yats_main.c \
 , $ext_shared)
fi
