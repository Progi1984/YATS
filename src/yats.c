
#ifdef HAVE_CONFIG_H
	#include "config.h"
#endif

#include <stdio.h>
#include "php.h"
#include "php_ini.h"
//#include "ext/standard/info.h"
#include "php_yats.h"

#include <libintl.h>
#include <locale.h>
#include <sys/stat.h>

// Declaration global variables
ZEND_DECLARE_MODULE_GLOBALS(yats)

/* True global resources - no need for thread safety here */
static int le_yats;

/* {{{ yats_functions[]
 *
 * Every user visible function must have an entry in yats_functions[].
 */
const zend_function_entry yats_functions[] = {
	PHP_FE(yats_define, NULL)
	PHP_FE(yats_assign, NULL)
	PHP_FE(yats_getbuf, NULL)
	PHP_FE(yats_getvars, NULL)
	PHP_FE(yats_hide, NULL)
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
   HashTable* ht = YATS_G(htFileCache);
   if (ht) {
	  my_hash_destroy(ht, YATS_G(iCache));
	  YATS_G(htFileCache) = 0;
   }
	UNREGISTER_INI_ENTRIES();
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(yats)
{
	// Determine whether to use yats cacheing or not
	YATS_G(sCache) = INI_INT(YATS_INI_CACHE);
	YATS_G(iCache) = (int)YATS_G(sCache);
	YATS_G(htFileCache) = 0;
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(yats)
{
	HashTable* ht = YATS_G(htFileCache);
	if (ht) {
		void** val = 0;
		zend_hash_internal_pointer_reset(ht);

		do {
			if (zend_hash_get_current_data(ht, (void**)&val) == SUCCESS) {
				release_request_data(val);
			}
			else {
				break;
			}
		} while(zend_hash_move_forward(ht) == SUCCESS );


		if(!YATS_G(iCache)) {
			my_hash_destroy(ht, YATS_G(iCache));
			YATS_G(htFileCache) = 0;
		}
	}
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


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
