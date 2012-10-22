
#ifndef PHP_YATS_H
#define PHP_YATS_H

#include "php_yats_common.h"

extern zend_module_entry yats_module_entry;
#define phpext_yats_ptr &yats_module_entry

#ifdef PHP_WIN32
#	define PHP_YATS_API __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
#	define PHP_YATS_API __attribute__ ((visibility("default")))
#else
#	define PHP_YATS_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

PHP_MINIT_FUNCTION(yats);
PHP_MSHUTDOWN_FUNCTION(yats);
PHP_RINIT_FUNCTION(yats);
PHP_RSHUTDOWN_FUNCTION(yats);
PHP_MINFO_FUNCTION(yats);

PHP_FUNCTION(confirm_yats_compiled);	/* For testing, remove later. */

/* 
  	Declare any global variables you may need between the BEGIN
	and END macros here:     

ZEND_BEGIN_MODULE_GLOBALS(yats)
	long  global_value;
	char *global_string;
ZEND_END_MODULE_GLOBALS(yats)
*/

/* In every utility function you add that needs to use variables 
   in php_yats_globals, call TSRMLS_FETCH(); after declaring other 
   variables used by that function, or better yet, pass in TSRMLS_CC
   after the last function argument and declare your utility function
   with TSRMLS_DC after the last declared argument.  Always refer to
   the globals in your function as YATS_G(variable).  You are 
   encouraged to rename these macros something shorter, see
   examples in any other php module directory.
*/

#ifdef ZTS
#define YATS_G(v) TSRMG(yats_globals_id, zend_yats_globals *, v)
#else
#define YATS_G(v) (yats_globals.v)
#endif

#endif	/* PHP_YATS_H */

