
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_yats.h"

// Declaration global variables
ZEND_DECLARE_MODULE_GLOBALS(yats)

/* True global resources - no need for thread safety here */
static int le_yats;

/* {{{ yats_functions[]
 *
 * Every user visible function must have an entry in yats_functions[].
 */
const zend_function_entry yats_functions[] = {
	PHP_FE(confirm_yats_compiled,	NULL)		/* For testing, remove later. */
	{NULL, NULL, NULL}	/* Must be the last line in yats_functions[] */
};
/* }}} */

/* {{{ yats_module_entry
 */
zend_module_entry yats_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
	STANDARD_MODULE_HEADER,
#endif
	"yats",
	yats_functions,
	PHP_MINIT(yats),
	PHP_MSHUTDOWN(yats),
	PHP_RINIT(yats),		/* Replace with NULL if there's nothing to do at request start */
	PHP_RSHUTDOWN(yats),	/* Replace with NULL if there's nothing to do at request end */
	PHP_MINFO(yats),
#if ZEND_MODULE_API_NO >= 20010901
	YATS_VERSION, /* Replace with version number for your extension */
#endif
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_YATS
ZEND_GET_MODULE(yats)
#endif

/* {{{ INI Settings */
PHP_INI_BEGIN()
	PHP_INI_ENTRY(YATS_INI_CACHE, "0", PHP_INI_ALL, NULL)
PHP_INI_END()
/* }}} */

/* {{{ php_yats_init_globals
 */
/* Uncomment this function if you have INI entries
static void php_yats_init_globals(zend_yats_globals *yats_globals)
{
	yats_globals->global_value = 0;
	yats_globals->global_string = NULL;
}
*/
/* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(yats)
{
	REGISTER_INI_ENTRIES();
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(yats)
{
	UNREGISTER_INI_ENTRIES();
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request start */
/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(yats)
{
	YATS_G(bCache) = INI_INT(YATS_INI_CACHE);
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request end */
/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(yats)
{
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(yats)
{
	php_info_print_table_start();
	php_info_print_table_header(1, "YATS -- Yet Another Template System");
	php_info_print_table_end();

	php_info_print_table_start();
	php_info_print_table_row(2, "version", YATS_VERSION);
	php_info_print_table_row(2, "author", "Dan Libby");
	php_info_print_table_row(2, "author", "Franck \"Progi1984\" LEFEVRE");
	php_info_print_table_row(2, "homepage", "https://github.com/Progi1984/YATS");
	php_info_print_table_end();

	php_info_print_table_start();
	php_info_print_table_header(2, "yats support", "enabled");
	php_info_print_table_end();

	DISPLAY_INI_ENTRIES();
}
/* }}} */


/* Remove the following function when you have succesfully modified config.m4
   so that your module can be compiled into PHP, it exists only for testing
   purposes. */

/* Every user-visible function in PHP should document itself in the source */
/* {{{ proto string confirm_yats_compiled(string arg)
   Return a string to confirm that the module is compiled in */
PHP_FUNCTION(confirm_yats_compiled)
{
	char *arg = NULL;
	int arg_len, len;
	char *strg;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &arg, &arg_len) == FAILURE) {
		return;
	}

	len = spprintf(&strg, 0, "Congratulations! You have successfully modified ext/%.78s/config.m4. Module %.78s is now compiled into PHP.", "yats", arg);
	RETURN_STRINGL(strg, len, 0);
}
/* }}} */
/* The previous line is meant for vim and emacs, so it can correctly fold and 
   unfold functions in source code. See the corresponding marks just before 
   function definition, where the functions purpose is also documented. Please 
   follow this convention for the convenience of others editing your code.
*/


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
