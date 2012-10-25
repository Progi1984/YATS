#include "php.h"
#include "php_yats.h"
#include <locale.h>

extern ZEND_DECLARE_MODULE_GLOBALS(yats)

/*
 * PHP API: assign one or more variables into a template
 */
PHP_FUNCTION(yats_assign)
{
   pval *arg1, *arg2, *arg3;
   parsed_file* f;

   if (ARG_COUNT(ht) == 2) {
      char* key = NULL;
      ulong num_index;

      if (getParameters(ht, 2, &arg1, &arg2) == FAILURE) {
         WRONG_PARAM_COUNT; /* prints/logs a warning and returns */
      }
      if (arg2->type != IS_ARRAY) {
         zend_error(E_WARNING,"Arg 2 is not an array.  Invalid param for %s() when called with two args",get_active_function_name());
         RETURN_FALSE;
      }

      f = (parsed_file*)arg1->value.lval;
      if (f && f->isValid == 1) {

         zend_hash_internal_pointer_reset(arg2->value.ht);

         while (1) {
            int bSuccess = FAILURE;

            int res = my_zend_hash_get_current_key(arg2->value.ht, &key, &num_index);
            if (res == HASH_KEY_IS_LONG) {
               zend_error(E_WARNING,"Index array not allowed here. %s()",get_active_function_name());
            } else if (res == HASH_KEY_NON_EXISTANT) {
               break;
            } else if (res == HASH_KEY_IS_STRING) {
               pval** val;
               if (zend_hash_get_current_data(arg2->value.ht, (void**)&val) == SUCCESS) {
                  bSuccess = add_val(f, key, *val);
               }
            }
            if (bSuccess != SUCCESS) {
               zend_error(E_WARNING,"Invalid object in array.  Ignored. %s()",get_active_function_name());
            }
            zend_hash_move_forward(arg2->value.ht);
         }
      }
   }

   else if (ARG_COUNT(ht) == 3) {
      if (getParameters(ht, 3, &arg1, &arg2, &arg3) == FAILURE) {
         WRONG_PARAM_COUNT; /* prints/logs a warning and returns */
      }

      /* validate yats arg */
      f = (parsed_file*)arg1->value.lval;
      if (f && f->isValid == 1) {
         char* var_id;
         convert_to_string(arg2);
         var_id = arg2->value.str.val;

         /* add value to yats */
         if (add_val(f, var_id, arg3) == SUCCESS) {
            RETURN_TRUE;
         } else {
            zend_error(E_ERROR,"Invalid object passed as param 2 for %s()",get_active_function_name());
         }
      } else {
         zend_error(E_ERROR,"Invalid template object passed as param 1 for %s()",get_active_function_name());
      }
   } else {
      WRONG_PARAM_COUNT;
   }

   RETURN_FALSE;
}

/**
 * Steps:
 * 1) Check if file in hash (already opened)
 * 2) Open file otherwise, and parse
 * 3) Store result in newly allocated structure.
 * 4) Return pointer to structure for later use.
 */
PHP_FUNCTION(yats_define)
{
   char* sFilePath;
   char* sDocRoot = 0;
   char* sSearchPath = 0;
   pval *arg1 = NULL, *arg2 = NULL, *arg3 = NULL;
   parsed_file* f;

   if(ARG_COUNT(ht) == 1){
      if(getParameters(ht, 1, &arg1) == FAILURE) {
          WRONG_PARAM_COUNT; /* prints/logs a warning and returns */
      }
   }
   else if(ARG_COUNT(ht) == 2){
      if(getParameters(ht, 2, &arg1, &arg2) == FAILURE) {
          WRONG_PARAM_COUNT; /* prints/logs a warning and returns */
      }
   }
   else if(ARG_COUNT(ht) == 3) {
      if(getParameters(ht, 3, &arg1, &arg2, &arg3) == FAILURE) {
          WRONG_PARAM_COUNT; /* prints/logs a warning and returns */
      }
   }

   /* validate yats arg */
   convert_to_string(arg1);
   sFilePath = arg1->value.str.val;

   if(arg2) {
       convert_to_string(arg2);
       if(strlen(arg2->value.str.val) > 0){
    	   sDocRoot = arg2->value.str.val;
       }
   }

   if(arg3) {
	   convert_to_string(arg3);
	   if(strlen(arg3->value.str.val) > 0){
		   sSearchPath = arg3->value.str.val;
	   }
   }

   f = get_file_possibly_cached(sFilePath, sDocRoot, sSearchPath);
   if (!f) {
      /* error getting data from server */
      RETURN_FALSE;
   }

   RETVAL_RESOURCE((long)f);
}

/* PHP API: interpolate and return buffer */

// yats_getbuf( parsed_file, [locale, [gettext_domain, [gettext_dir]]] )
PHP_FUNCTION(yats_getbuf) {
   pval *arg1, *arg2 = NULL, *arg3 = NULL, *arg4 = NULL;
   parsed_file* f;

   RETVAL_FALSE;

   if (ARG_COUNT(ht) == 1 ) {
      if( getParameters(ht, 1, &arg1) == FAILURE ) {
          WRONG_PARAM_COUNT; /* prints/logs a warning and returns */
      }
   }
   else if (ARG_COUNT(ht) == 2 ) {
      if( getParameters(ht, 2, &arg1, &arg2) == FAILURE ) {
          WRONG_PARAM_COUNT; /* prints/logs a warning and returns */
      }
   }
   else if (ARG_COUNT(ht) == 3 ) {
      if( getParameters(ht, 3, &arg1, &arg2, &arg3) == FAILURE ) {
          WRONG_PARAM_COUNT; /* prints/logs a warning and returns */
      }
   }
   else if (ARG_COUNT(ht) != 4 || getParameters(ht, 4, &arg1, &arg2, &arg3, &arg4) == FAILURE) {
      WRONG_PARAM_COUNT; /* prints/logs a warning and returns */
   }


   /* validate yats arg */
   f = (parsed_file*)arg1->value.lval;
   if (f && f->isValid == 1) {
      yatsstring buf;
      yatsstring_init(&buf);

      char* origlocale = NULL;

      /* arg2, arg3, and arg4 are gettext related */
      if( arg2 ) {
          convert_to_string(arg2);
          if( arg2->value.str.len ) {
              char* plocale = arg2->value.str.val;
              char* pdomain = f->filename;
              char* pdir = f->dir;

              if( arg3 ) {
                  convert_to_string( arg3 );
                  if( arg3->value.str.len ) {
                      pdomain = arg3->value.str.val;
                  }
              }
              if( arg4 ) {
                  convert_to_string( arg4 );
                  if( arg4->value.str.len ) {
                      pdir = arg4->value.str.val;
                  }
              }

              // Store the original locale, so we can restore it after. Do not store if same as requested locale.
              origlocale = setlocale( LC_MESSAGES, NULL );
              origlocale = origlocale == plocale ? NULL : origlocale;

              // this loop allows caller to pass in multiple locales in order of preference,
              // and we will use the first one that setlocale will accept.
              char* start = plocale;
              char* end = strchr( plocale, ' ');
              if( end ) {
                  while(start && end) {
                      *end = 0;
                      if( setlocale( LC_MESSAGES, start ) != NULL ) {
                          break;
                      }
                      start = end + 1;
                      end = strchr( start, ' ' );
                  }
              }
              else {
                  setlocale( LC_MESSAGES, plocale );
              }

              // setlocale( LC_MESSAGES, plocale );
              // setenv( "LANG", "foobar", 1 );
              // setenv( "LANGUAGE", plocale, 1 );
              // setlocale( LC_MESSAGES, "" );

              bindtextdomain( pdomain, pdir );
              textdomain( pdomain );
          }
      }

      if (fill_buf(f, &buf, f->tokens, NULL, 1, 0, 1) == SUCCESS) {
         RETVAL_STRING(buf.str, 0 ); /* do not duplicate */
      }
      if( origlocale ) {
          setlocale( LC_MESSAGES, origlocale );
      }
   }
   else {
      zend_error(E_ERROR,"Invalid template object passed as param 1 for %s()",get_active_function_name());
   }

   return;
}

/* PHP API: return list of vars */
PHP_FUNCTION(yats_getvars)
{
   pval* arg1;
   parsed_file* f;

   if (ARG_COUNT(ht) != 1 || getParameters(ht, 1, &arg1) == FAILURE) {
      WRONG_PARAM_COUNT; /* prints/logs a warning and returns */
   }

   /* validate yats arg */
   f = (parsed_file*)arg1->value.lval;
   if (f && f->isValid == 1) {
      *return_value = *f->assigned_vars;
      zval_copy_ctor(return_value);
      return;
   }

   RETURN_FALSE;
}

/* PHP API: hide/un-hide a section */
PHP_FUNCTION(yats_hide)
{
   pval *arg1, *arg2, *arg3, *arg4 = NULL;
   parsed_file* f;
   int row = - 1;

   if (ARG_COUNT(ht) == 3) {
      if (getParameters(ht, 3, &arg1, &arg2, &arg3) == FAILURE) {
         WRONG_PARAM_COUNT; /* prints/logs a warning and returns */
      }
   }
   else if (ARG_COUNT(ht) != 4 || getParameters(ht, 4, &arg1, &arg2, &arg3, &arg4) == FAILURE) {
      WRONG_PARAM_COUNT; /* prints/logs a warning and returns */
   }

   /* validate yats arg */
   f = (parsed_file*)arg1->value.lval;
   if (f && f->isValid == 1) {
      per_request_section_options* sec_ops = 0;

      convert_to_string(arg2);
      convert_to_long(arg3);

      sec_ops = getSectionOptions(f->section_options, arg2->value.str.val, arg2->value.str.len, 1);
      if(sec_ops) {
         int bHidden = arg3->value.lval ? 1 : 0;
         if(arg4) {
            HashTable* htHiddenRows = get_section_hidden_rows(sec_ops);

            if(htHiddenRows) {
               int bHiddenRows = bHidden ? 1 : 2;
               convert_to_long(arg4);
               if(zend_hash_index_update(htHiddenRows, arg4->value.lval, (void*)&bHiddenRows, sizeof(void*), NULL) == SUCCESS) {
                  RETURN_TRUE;
               }
            }
         }
         else {
            sec_ops->bHiddenAll = bHidden;

            // master visibility has changed.  unset child rows.
            my_hash_destroy(sec_ops->hiddenRows, 0);
            sec_ops->hiddenRows = 0;
         }
      }
      RETURN_TRUE;

   } else {
      zend_error(E_ERROR,"Invalid template object passed as param 1 for %s()",get_active_function_name());
   }

   RETURN_FALSE;
}
