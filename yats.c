/*  
  This file is part of YATS - (Y)et (A)nother (T)emplating (S)ystem for PHP     
  
  Author: Dan Libby (dan@libby.com)  
  Epinions.com may be contacted at feedback@epinions-inc.com  
*/

/*  
  Copyright 2000 Epinions, Inc. 

  Subject to the following 3 conditions, Epinions, Inc.  permits you, free 
  of charge, to (a) use, copy, distribute, modify, perform and display this 
  software and associated documentation files (the "Software"), and (b) 
  permit others to whom the Software is furnished to do so as well.  

  1) The above copyright notice and this permission notice shall be included 
  without modification in all copies or substantial portions of the 
  Software.  

  2) THE SOFTWARE IS PROVIDED "AS IS", WITHOUT ANY WARRANTY OR CONDITION OF 
  ANY KIND, EXPRESS, IMPLIED OR STATUTORY, INCLUDING WITHOUT LIMITATION ANY 
  IMPLIED WARRANTIES OF ACCURACY, MERCHANTABILITY, FITNESS FOR A PARTICULAR 
  PURPOSE OR NONINFRINGEMENT.  

  3) IN NO EVENT SHALL EPINIONS, INC. BE LIABLE FOR ANY DIRECT, INDIRECT, 
  SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES OR LOST PROFITS ARISING OUT 
  OF OR IN CONNECTION WITH THE SOFTWARE (HOWEVER ARISING, INCLUDING 
  NEGLIGENCE), EVEN IF EPINIONS, INC.  IS AWARE OF THE POSSIBILITY OF SUCH 
  DAMAGES.    

*/

#include <stdio.h>
#include "php.h"
#include "php3_yats.h"
#include "php_ini.h"

#include <libintl.h>
#include <locale.h>
#include <sys/stat.h>


/* poor man's package version system. auto* is friggin _hard_  */
#define YATS_VERSION "0.93"

#ifdef COMPILE_DL_YATS
ZEND_GET_MODULE(yats)
#endif

#if ZEND_MODULE_API_NO >= 20001222
#define my_zend_hash_get_current_key(ht, my_key, num_index) zend_hash_get_current_key(ht, my_key, num_index, 0)
#else
#define my_zend_hash_get_current_key(ht, my_key, num_index) zend_hash_get_current_key(ht, my_key, num_index)
#endif 

#define my_free(x) if(x) free(x); x = 0
#define my_pefree(x, p) if(x) {if(p) free(x); else efree(x);} x = 0
#define my_efree(x) if(x) efree(x); x = 0

/* Some stuff to make it easier if we want to become thread safe */
typedef struct _yats_globals {
   HashTable* yats_hash;
   char        bCache;
} yats_globals;

yats_globals t_g = {0};

#define YATS_GLOBALS(var) t_g.var

#define YATS_CACHE_STR "yats_cache"
#define CACHE_TEMPLATES_BOOL ((int)YATS_GLOBALS(bCache))

/* forward decl */
int release_request_data(void** f);

HashTable* hash_init(dtor_func_t dtr, int bPerm) {
   HashTable* ht = pemalloc(sizeof(HashTable), bPerm);
   if(ht) {
      if(zend_hash_init(ht, 0, NULL, dtr, bPerm) != SUCCESS) {
         my_pefree(ht, bPerm);
         ht = 0;
      }
   }
   return ht;
}

static void my_hash_destroy(HashTable* ht, int bPerm) {
   if( ht ) {
      zend_hash_destroy(ht);
      my_pefree(ht, bPerm);
   }
}


/* module entrypoint */
function_entry php_yats_functions[] = {
   PHP_FE(yats_define, NULL)
   PHP_FE(yats_assign, NULL)
   PHP_FE(yats_getbuf, NULL)
   PHP_FE(yats_getvars, NULL)
   PHP_FE(yats_hide, NULL)
   {NULL, NULL, NULL}
};

PHP_INI_BEGIN()
   PHP_INI_ENTRY(YATS_CACHE_STR,"0", PHP_INI_ALL, NULL)
PHP_INI_END()


PHP_MINIT_FUNCTION(yats)
{
	REGISTER_INI_ENTRIES();

	return SUCCESS;
}

PHP_RINIT_FUNCTION(yats) {
   /* determine whether to use yats cacheing or not */
   YATS_GLOBALS(bCache) = INI_INT(YATS_CACHE_STR);
}


/* called on module shutdown */
PHP_MSHUTDOWN_FUNCTION(yats)
{
   HashTable* ht = YATS_GLOBALS(yats_hash);
   if (ht) {
      my_hash_destroy(ht, CACHE_TEMPLATES_BOOL);
      YATS_GLOBALS(yats_hash) = 0;
   }

   UNREGISTER_INI_ENTRIES();

   return SUCCESS;
}



/* called at end of request  */
PHP_RSHUTDOWN_FUNCTION(yats)
{
   HashTable* ht = YATS_GLOBALS(yats_hash);
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


      if(!CACHE_TEMPLATES_BOOL) {
         my_hash_destroy(ht, CACHE_TEMPLATES_BOOL);
         YATS_GLOBALS(yats_hash) = 0;
      }
   }

   return SUCCESS;
}


/* define entrypoints */
zend_module_entry yats_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
   STANDARD_MODULE_HEADER,
#endif
   "yats",                           /* extension name */
   php_yats_functions,               /* function list */
   PHP_MINIT(yats),                  /* process startup */
   PHP_MSHUTDOWN(yats),              /* process shutdown */
   PHP_RINIT(yats),                  /* request startup */
   PHP_RSHUTDOWN(yats),              /* request shutdown */
   PHP_MINFO(yats),                  /* extension info */
#if ZEND_MODULE_API_NO >= 20010901
   YATS_VERSION,                   /* extension version number (string) */
#endif
   NULL,                             /* global startup function */
   NULL,                             /* global shutdown function */
   STANDARD_MODULE_PROPERTIES_EX
};

/* module description */
PHP_MINFO_FUNCTION(yats)
{
   php_info_print_table_start();
   php_info_print_table_header(1, "YATS -- Yet Another Template System");
   php_info_print_table_end();
   
   php_info_print_table_start();
   php_info_print_table_row(2, "version", YATS_VERSION);
   php_info_print_table_row(2, "author", "Dan Libby");
   php_info_print_table_row(2, "homepage", "http://yats.sourceforge.net");
   php_info_print_table_row(2, "open sourced by", "Epinions.com");
   php_info_print_table_end();

   DISPLAY_INI_ENTRIES();
}

void char_ptr_dtor_free(char** val) {
   my_free(*val);
}

void char_ptr_dtor_efree(char** val) {
	// crashes (later) if we use my_efree.
   efree(*val);
}


/******************************
* Begin Simple List Functions *
******************************/

typedef enum {
   variable,
   string,
   section,
   gettext_
} token_type;

typedef struct _simple_list_obj {
   void *val;
   struct _simple_list_obj* next;
} list_obj;

typedef struct _simple_list {
   list_obj* current;
   list_obj* start;
   list_obj* end;
   unsigned count;
} simple_list;

/* create a new simple list */
simple_list* simple_list_new(int bPerm) {
   simple_list* list = pecalloc(1, sizeof(simple_list), bPerm);
   return list;
}

/* add token to list */
void simple_list_add(simple_list* list, void* val, int bPerm) {
   list_obj* obj = pecalloc(1, sizeof(list_obj), bPerm);
   if(obj) {
      obj->val = val;
      obj->next = 0;

      if (list->end) {
         list->end->next = obj;
      }
      list->end = obj;

      if (!list->start) {
         list->start = obj;
      }
      list->count ++;
   }
}

/* advance to next item in list */
void* simple_list_next(simple_list* list) {
   if(list) {
      if(list->current) {
         list->current = list->current->next;
      }
   }
   return list && list->current ? list->current->val : 0;
}

unsigned simple_list_length(simple_list* list) {
   return list ? list->count : 0;
}

/* reset to beginning of list */
void* simple_list_reset(simple_list* list) {
   if(list) {
      list->current = list->start;
   }
   return list && list->current ? list->current->val : 0;
}

typedef void (*list_iter_callback)(void *val, void* data);

/* iterate through a list and call callback func */
void simple_list_iter(simple_list* list, list_iter_callback callback, void* data) {
   void* val = simple_list_reset(list);
   while(val) {
      (*callback)(val, data);
      val = simple_list_next(list);
   }
}

/* destroy a simple list.  free all list associated mem. call callback if present. */
void simple_list_destroy(simple_list* list, list_iter_callback callback, void* data, int bPerm) {
   if(list) {
      simple_list_reset(list);

      while(list->current) {
         list_obj* free_me = list->current;

         if(callback) {
            (*callback)(list->current->val, data);
         }
         simple_list_next(list);
         my_pefree(free_me, bPerm);
      }
      my_pefree(list, bPerm);
   }
}

/****************************
* End Simple List Functions *
****************************/


/************************
* Begin Token Functions *
************************/

typedef struct _token {
   token_type   type;
   char*        buf;
   simple_list* section;
   int          bHidden;
   HashTable*   attrs;
} token;

void token_list_destroy(simple_list* list, int bPerm);

/* free a token. recurses if token contains children */
void token_destroy(token* token, int bPerm) {
   if(token) {
      if (token->section) {
         token_list_destroy(token->section, bPerm);
      }
      if (token->buf) {
         my_pefree(token->buf, bPerm);
      }
      if(token->attrs) {
         my_hash_destroy(token->attrs, bPerm);
      }
      my_pefree(token, bPerm);
   }
}

/* free a list of tokens. recursive */
void token_list_destroy(simple_list* list, int bPerm) {
   simple_list_destroy(list, (list_iter_callback)token_destroy, (void*)bPerm, bPerm);
}

/* allocate a new token */
token* token_new(int bPerm) {
   token* t = pecalloc(1, sizeof(token), bPerm);
   return t;
}

/* utility function to create and add a new token */
void token_add(simple_list* list, char* start, char* end, token_type type, HashTable* attrs, simple_list* toks, int bPerm) {
   int len = end - start + 1;
   if (len > 0) {
      token* tok = token_new(bPerm);
      if(tok) {
         tok->buf = pemalloc(len+1, bPerm);
         strncpy(tok->buf, start, len);
         tok->buf[len] = 0;
         tok->type = type;
         tok->attrs = attrs;
         if (toks) {
            tok->section = toks;
         }
         simple_list_add(list, tok, bPerm);
      }
   }
}


/**********************
* End Token Functions *
**********************/


/*************************
* Begin String Functions *
*************************/

typedef struct _yatsstring {
   char* str;
   int len;
   int size;
} yatsstring;

#define YATSSTRING_INCR 1024


/* initialize string */
void yatsstring_init(yatsstring* string) {
   string->str = emalloc(YATSSTRING_INCR);
   if (string->str) {
      string->str[0] = 0;
      string->len = 0;
      string->size = YATSSTRING_INCR;
   } else {
      string->size = 0;
   }
}

/* clear contents of string. does not free memory */
void yatsstring_clear(yatsstring* string) {
   if(string->len && string->str) {
      string->str[0] = 0;
      string->len = 0;
   }
}

/* release resources for string */
void yatsstring_free(yatsstring* string) {
   if (string && string->str) {
      my_efree(string->str);
   }
}

/* add a string of known length */
void yatsstring_addn(yatsstring* string, char* add, int add_len) {
   if (add && add_len) {
      if (string->len + add_len + 1 > string->size) {

         /* newsize is current length + new length */
         int newsize = string->len + add_len + 1;

         /* align to YATSSTRING_INCR increments */
         newsize = newsize - (newsize % YATSSTRING_INCR) + YATSSTRING_INCR;
         string->str = erealloc(string->str, newsize);

         string->size = string->str ? newsize : 0;
      }

      if (string->str) {
         memcpy(string->str + string->len, add, add_len);
         string->len += add_len;
         string->str[string->len] = 0; /* null terminate */
      }
   }
}

/* add a string of unknown length */
void yatsstring_add(yatsstring* string, char* add) {
   if(add) {
      yatsstring_addn(string, add, strlen(add));
   }
}


/***********************
* End String Functions *
***********************/


/**************************
* Begin Parsing Functions *
**************************/

// This parsing code is nasty. It is fairly efficient, but could be made more so, and
// more maintainable using a real parsing engine / state machine.

/* Parse attributes of a token.
 * examp input:
 *   key="val"  key2 =  "val2"
 */
HashTable* parse_attrs(char* token, int bPerm) {
   HashTable* attrs = 0;
   char* p = token;
   int bSuccess = SUCCESS;

   attrs = hash_init((dtor_func_t)(bPerm ? char_ptr_dtor_free : char_ptr_dtor_efree), bPerm);
   if(!attrs) {
      zend_error(E_CORE_ERROR, "Unable to initialize hash");
   }
   else {
      while((p = strchr(p, ' ')) != NULL && bSuccess == SUCCESS) {
         char* key = 0, *value = 0;
         int newkey = 0;
         bSuccess = FAILURE;
         while((int)*(++p) && isspace(*p)); /* advance to key */
         if(*p) {
            key = p;
            while(*(++p) && isalnum(*p)); /* to end of key.  alpha numerics only */
            if(*p) {
               key = bPerm ? (char*)zend_strndup(key, p - key) : estrndup(key, p - key);
               newkey = 1;
               while(*(p) && isspace(*p)) p++;
               if(*p == '=') {
                  while(*(++p) && isspace(*p)); /* advance to value */
                  if(*p && *p == '"') {
                     value = p + 1;
                     while(*(++p) && isprint(*p) && *p != '"'); /* to end of value. */
                     if(*p && *p == '"') {
                        value = bPerm ? (char*)zend_strndup(value, p - value) : estrndup(value, p - value);
                     }
                     else {
                        // missing the trailing quote.
                        value = 0;
                        bSuccess = FAILURE;
                     }
                  }
               }
            }
         }
         if(key && value) {
            bSuccess = zend_hash_update(attrs, key, strlen(key)+1, (void *)&value, sizeof(char *), NULL);
            /* bSuccess = add_assoc_string(attrs, key, value, 1); */
         }
         if(newkey && key) {
            my_pefree(key, bPerm);
         }
      }
   }

   if(bSuccess == FAILURE) {
      zend_error(E_ERROR,"Parse error in token: %s", token);
      return NULL;
   }

   return attrs;
}


#define TOKEN_START "{{"
#define TOKEN_END "}}"
#define TOKEN_GETTEXT "text"

/* parse a buffer/section.  recursive */
simple_list* parse_buf(char* buf, int bPerm) {
   simple_list* tokens = simple_list_new(bPerm);
   char* start = buf; /* start of current token */
   char* end = 0; /* end of current token */
   char* p = strstr(start, TOKEN_START);
   int slen = strlen(TOKEN_START);
   int elen = strlen(TOKEN_END);
   int glen = strlen(TOKEN_GETTEXT);
   
   while (p) {
      end = p - 1;

      if (end >= start) {
         token_add(tokens, start, end, string, NULL, NULL, bPerm);
      }

      start = p + elen;

      p = strstr(start, TOKEN_END);

      if (p) {
         HashTable* attrs = NULL;
         char tmp, *tok_end;

         end = p - 1;

         tmp = *(end + 1);


         *(end + 1) = 0;

         /* check for start of new token or end of line */
         if(strstr(start, TOKEN_START) || strchr(start, '\n')) {
            zend_error(E_ERROR,"Malformed token:  %s", start);
            *(end + 1) = tmp;
            break;
         }

         tok_end = start;

         while (tok_end < end) {
            if (isspace(*tok_end)) {
               attrs = parse_attrs(tok_end, bPerm);
               tok_end --;
               break;
            }
            tok_end++;
         }

         *(end + 1) = tmp;

         if (!strncmp(start, "section:", 8)) {
            int end_token_len = slen + 1 + (tok_end - start + 1) + elen;
            char* end_token = emalloc(end_token_len + 1);
            char* sec_end = 0;
            sprintf(end_token, "%s/section:", TOKEN_START);
            strncat(end_token, start + 8, tok_end - (start + 8) + 1);
            sprintf(end_token + slen + 1 + (tok_end - start + 1), TOKEN_END);
            sec_end = strstr(end, end_token);
            if (sec_end) {
               char tmp = *sec_end;
               simple_list* sect = 0;
               *sec_end = 0; /* null terminate string temporarily */
               sect = parse_buf(end + elen + 1, bPerm);
               token_add(tokens, start + 8, tok_end, section, attrs, sect, bPerm);
               *sec_end = tmp; /* restore */

               start = sec_end + end_token_len;
            } else {
               zend_error(E_ERROR,"Matching end token not found:  %s", end_token);
               start = end + elen + 1;
            }
            my_efree(end_token);
         }
         else if (!strncmp(start, TOKEN_GETTEXT, glen )) {
            int end_token_len = slen + 1 + (tok_end - start + 1) + elen;
            char* end_token = emalloc(end_token_len + 1);
            char* sec_end = 0;
            sprintf(end_token, "%s/%s", TOKEN_START, TOKEN_GETTEXT);
            strncat(end_token, start + glen, tok_end - (start + glen) + 1);
            sprintf(end_token + slen + 1 + (tok_end - start + 1), TOKEN_END);
            sec_end = strstr(end, end_token);
            if (sec_end) {
               char tmp = *sec_end;
               simple_list* sect = 0;
               *sec_end = 0; /* null terminate string temporarily */

               // We don't parse the buffer here. The token is either {{text}} or {{text parse="yes"}}.
               // In the former case, it is never parsed at all (for performance).  In the latter, we parse it, but
               // later, in the call to yats_getbuf(), after the localized string has been retrieved.
               token_add(tokens, end + elen + 1, sec_end, gettext_, attrs, NULL, bPerm);
               *sec_end = tmp; /* restore */

               start = sec_end + end_token_len;
            } else {
               zend_error(E_ERROR,"Matching end token not found:  %s", end_token);
               start = end + elen + 1;
            }
            my_efree(end_token);
         }
         else {
            token_add(tokens, start, tok_end, variable, attrs, NULL, bPerm);
            start = end + elen + 1;

         }

         p = strstr(start, TOKEN_START);
      }
      else {
         zend_error(E_ERROR,"Malformed token:  %s", start);
         break;
      }
   }

   token_add(tokens, start, start + strlen(start), string, NULL, NULL, bPerm);
   return tokens;
}

/************************
* End Parsing Functions *
************************/


/**********************************
* Begin File / Cacheing Functions *
**********************************/

typedef struct _parsed_file {
   char* filepath;
   char* dir;
   char* filename;
   char* buf;
   simple_list* tokens;
   HashTable* section_options;
   pval* assigned_vars;
   int isValid; /* Must equal 1 exactly. */
   time_t mtime; 
} parsed_file;


/* load a file */
parsed_file* get_file(const char* filepath, int file_len, int bPerm) {
   parsed_file* res = 0;
   FILE* f = fopen(filepath, "r");
   if (f) {
      char* buf;
      res = pecalloc(1, sizeof(parsed_file), bPerm);

      res->dir = pestrdup(filepath, bPerm);
      char* p = strrchr( res->dir, '/');
      if( p ) {
          if( *p != 0 ) {
              res->filename = pestrdup( p + 1, bPerm );
          }
          *p = 0;
      }

      res->filepath = pestrdup(filepath, bPerm);
      buf = emalloc(file_len + 1);

      fread(buf, 1, file_len, f);
      fclose(f);

      buf[file_len] = 0;

      res->tokens = parse_buf(buf, bPerm);
      my_efree(buf);
   } else {
      zend_error(E_WARNING,"Unable to open template %s", filepath);
   }
   return res;
}

int release_request_data(void** file) {
   parsed_file* f = *(parsed_file**)file;


	/* nasty hack.  memory bug somewhere causes this to
	 * crash when cacheing templates. can't find the bug
	 * but it seems to not crash if we don't free these
	 */
	if(!CACHE_TEMPLATES_BOOL) {
		/* These are always per request, so use my_efree */
		if (f->assigned_vars) {
			 zval_dtor(f->assigned_vars);
			 FREE_ZVAL(f->assigned_vars);
		}
		if (f->section_options) {
			my_hash_destroy(f->section_options, 0);
		}
	}

   return 1;
}

/* Releases file resources */
int release_file(parsed_file* f, int bPerm) {
   if (f) {
      if (f->filepath) {
         my_pefree(f->filepath, bPerm);
      }
      if( f->dir ) {
         my_pefree( f->dir, bPerm );
      }
      if( f->filename ) {
         my_pefree( f->filename, bPerm );
      }
      if (f->tokens) {
         token_list_destroy(f->tokens, bPerm);
      }
      my_pefree(f, bPerm);

      return 1;
   }
   return 0;
}

/* file destructor, called by hashtable */
int release_file_perm(void** f) {
   return release_file(*(parsed_file**)f, CACHE_TEMPLATES_BOOL);
}

typedef struct _per_request_section_options {
   signed int bHiddenAll;
   HashTable* hiddenRows;
} per_request_section_options;

int release_section_options(void** op) {
   if(op) {
      per_request_section_options* options = *(per_request_section_options**)op;
      if(options) {
         if(options->hiddenRows) {
            my_hash_destroy(options->hiddenRows, 0);
         }

         my_efree(options);

         return 1;
      }
   }
   return 0;
}



/* Init per request variables for file */
void per_req_init(parsed_file* f) {
   f->isValid = 0;
   MAKE_STD_ZVAL(f->assigned_vars);
   f->section_options = hash_init((dtor_func_t)release_section_options, 0);
   if (f->assigned_vars && array_init(f->assigned_vars) != FAILURE && f->section_options) {
      if (f->tokens) {
         f->isValid = 1;
      }
   }
}

/* Retrieves file. Uses cached version if available, unless timestamp newer */
parsed_file* get_file_possibly_cached(char* filepath) {
   struct stat statbuf;
   parsed_file* f = 0;

   if (!stat(filepath, &statbuf)) {
      /* see if we already have parsed file stored */
      if (YATS_GLOBALS(yats_hash)) {
         parsed_file** f2;
         if (zend_hash_find(YATS_GLOBALS(yats_hash), filepath, strlen(filepath) + 1, (void**)&f2) == SUCCESS) {
            if (f2) {
               if (statbuf.st_mtime <= (*f2)->mtime) {
                  per_req_init(*f2);
                  return *f2;
               } else {
                  /* out of date.  release */
                  zend_hash_del_key_or_index(YATS_GLOBALS(yats_hash), filepath, strlen(filepath) +1, 0, HASH_DEL_KEY);
                  /* release_file(*f2, CACHE_TEMPLATES_BOOL); */
               }
            }
         }
      } else {
         /* This can live till process/module dies */
         YATS_GLOBALS(yats_hash) = hash_init((dtor_func_t)release_file_perm, CACHE_TEMPLATES_BOOL);
         if (!YATS_GLOBALS(yats_hash)) {
            zend_error(E_CORE_ERROR, "Unable to initialize hash");
         }
      }

      f = get_file(filepath, statbuf.st_size, CACHE_TEMPLATES_BOOL);
      if (f) {
         f->mtime = statbuf.st_mtime;
         per_req_init(f);
         zend_hash_update(YATS_GLOBALS(yats_hash), filepath, strlen(filepath)+1, &f, sizeof(parsed_file*), NULL);
      }
   } else {
      zend_error(E_WARNING,"template not found: %s", filepath);
   }
   return f;
}

/***********************************
* End Parsing / Cacheing Functions *
***********************************/


/**************************
* Begin Utility Functions *
**************************/

per_request_section_options* getSectionOptions(HashTable* section_hash, char* id, int id_len, int bCreate) {
   per_request_section_options *sec_ops = 0, **psec_ops;
   if(zend_hash_find(section_hash, id, id_len + 1, 
                     (void**)&psec_ops) == SUCCESS) {
      return *psec_ops;
   }
   else {
      if(bCreate) {
         /* free'd at end of request in release_section_options() */
         sec_ops = ecalloc(1, sizeof(per_request_section_options)); 

         if(sec_ops) {
            sec_ops->hiddenRows = 0;
            sec_ops->bHiddenAll = -1;  // initial state.

            zend_hash_update(section_hash, id, id_len+1, (void *)&sec_ops, sizeof(per_request_section_options *), NULL);
         }
      }
   }
   return sec_ops;
}

HashTable* get_section_hidden_rows(per_request_section_options* sec_ops) {
   HashTable* ht = NULL;
   if(sec_ops) {
      if(!sec_ops->hiddenRows) {
         /* we're just storing ints in here. no destructor necessary */
         sec_ops->hiddenRows = hash_init((dtor_func_t)0, 0);
      }
      ht = sec_ops->hiddenRows;
   }
   return ht;
}

int section_hidden_for_row(per_request_section_options* sec_ops, ulong row) {
   int bReturn = 0;
    
   if(sec_ops && sec_ops->hiddenRows) {
      int *bHidden = 0;
      if(zend_hash_index_find(sec_ops->hiddenRows, row, (void**)&bHidden) == SUCCESS) {

          bReturn = (*bHidden == 1 ? 1 : -1);
      }
   }
   return bReturn;
}

int add_arg(pval *res, pval* arg) {
   pval* tmp;

   if (arg->type == IS_ARRAY) {
      zend_hash_merge(res->value.ht, arg->value.ht, (void (*)(void *)) zval_add_ref, (void *) &tmp, sizeof(zval *), 1);
   } else {
      convert_to_string(arg);
      if (!arg->value.str.val || (add_next_index_string(res, arg->value.str.val, 1) == FAILURE)) {
         return FAILURE;
      }
   }
   return SUCCESS;
}

/* add a key/value pair to file.  */
int add_val(parsed_file *f, char* var_id, pval* arg) {
   pval *res, **pres;
   int bSuccess = FAILURE;

   if (zend_hash_find(f->assigned_vars->value.ht, var_id, strlen(var_id)+1, (void**)&pres) == SUCCESS) {
      if( (*pres)->type == IS_ARRAY) {
         bSuccess = add_arg( *pres, arg);
      }
   }

   else {
      MAKE_STD_ZVAL(res);
      if (res && array_init(res) != FAILURE) {
         if(add_arg(res, arg) == SUCCESS) {
            if (zend_hash_update(f->assigned_vars->value.ht, var_id, strlen(var_id)+1, (void *)&res, sizeof(pval *), NULL) == SUCCESS) {
               bSuccess = SUCCESS;
            }
         }
      }
   }


   return bSuccess;
}

/* Interpolate tokens and variables.  Recursive */
int fill_buf(parsed_file* f, yatsstring* buf, simple_list* tokens, HashTable* attrs, int bLoop, int row, int max_rows) {
   int num_rows = 1;
   int bSuccess = SUCCESS;
   char** attr;

   yatsstring my_buf;
   yatsstring_init(&my_buf);

   do {
      token* tok = simple_list_reset(tokens);
      while (tok) {
         if (tok->type == string) {
            /* Found a string.  Add it */
            yatsstring_add(&my_buf, tok->buf);
         } else if (tok->type == variable) {
            /* Found a variable. Look it up and fill it in */
            pval **res, **res2;
            int bFound = 0;
            if (zend_hash_find(f->assigned_vars->value.ht, tok->buf, strlen(tok->buf) + 1, (void**)&res) == SUCCESS) {
               int num_elem = zend_hash_num_elements((*res)->value.ht);
               int idx = row;
               int bRepeatScalar = 0;

               /* If variable is scalar, then we will repeat it unless repeatscalar="no" is specified */
               if(num_elem == 1) {
                  if(tok->attrs) {
                     if (zend_hash_find(tok->attrs, "repeatscalar", strlen("repeatscalar") + 1, (void**)&attr) == SUCCESS) {
                        if(!strcmp((*attr), "yes")) {
                           bRepeatScalar = 1;
                           idx = 0;
                        }
                     }
                  }
               }

               /* find minimum number */
               if ( (num_elem > bRepeatScalar )  && (num_elem < num_rows || num_rows <= 1)) {
                  num_rows = num_elem;
               }
               if( max_rows >= 0 && num_rows > max_rows ) {
                   num_rows = max_rows;
               }

               /* look up value at index */
               if (zend_hash_index_find((*res)->value.ht, idx, (void**)&res2) == SUCCESS && (*res2)->type == IS_STRING) {
                  yatsstring_add(&my_buf, (*res2)->value.str.val);
                  bFound = 1;
               }
            }
            if (!bFound) {
               char** attr;
               /* check for alt text */
               if (tok->attrs && 
                   zend_hash_find(tok->attrs, "alt", strlen("alt") + 1, (void**)&attr) == SUCCESS) {
                  yatsstring_add(&my_buf, (*attr));
               }
               /* otherwise, don't show this section if autohide="yes" is specified. */
               else {
                  int bClear = 0;
                  if (attrs) {
                     if (zend_hash_find(attrs, "autohide", strlen("autohide") + 1, (void**)&attr) == SUCCESS) {
                        if(!strcmp((*attr), "yes")) {
                           bClear = 1;
                        }
                     }
                  }
                  if(bClear) {
                     yatsstring_clear(&my_buf);
                     break;
                  }
               }
            }
         } else if (tok->type == section) {
            per_request_section_options* sec_ops = getSectionOptions(f->section_options, tok->buf, strlen(tok->buf), 0);
            if (!sec_ops || sec_ops->bHiddenAll < 1 ) {
               /* Found a section.  Recurse */
               int bParentLoop = 0;
               int bHidden = 0, bRowState = 0;
               int i_max_rows = -1;
               if (tok->attrs) {
                  if (zend_hash_find(tok->attrs, "parentloop", strlen("parentloop") + 1, (void**)&attr) == SUCCESS) {
                     if(!strcmp((*attr), "yes")) {
                        bParentLoop = 1;
                     }
                  }
                  if (zend_hash_find(tok->attrs, "maxloops", strlen("maxloops") + 1, (void**)&attr) == SUCCESS) {
                     i_max_rows = atoi( *attr );
                  }
                  // initially set to -1. (meaning unset)  after oscillates between 0,1
                  if( !sec_ops || sec_ops->bHiddenAll == -1 ) {
                     if (zend_hash_find(tok->attrs, "hidden", strlen("hidden") + 1, (void**)&attr) == SUCCESS) {
                        if(!strcmp((*attr), "yes")) {
                           bHidden = 1;
                        }
                     }
                  }
                  bRowState = section_hidden_for_row(sec_ops, row+1);
                  if( bRowState != 0 ) {
                      bHidden = bRowState == 1 ? 1 : 0;
                  }
               }
               if( bHidden == 0 ) {
                  if(bParentLoop) {
                     bSuccess = fill_buf(f, &my_buf, tok->section, tok->attrs, 0, row, i_max_rows);
                  }
                  else {
                     bSuccess = fill_buf(f, &my_buf, tok->section, tok->attrs, 1, 0, i_max_rows);
                  }
                  if(bSuccess == FAILURE) {
                     break;
                  }
               }
            }
         }
         else if( tok->type == gettext_ ) {

             int bParse = 0;

             if (tok->attrs) {
                if (zend_hash_find(tok->attrs, "parse", strlen("parse") + 1, (void**)&attr) == SUCCESS) {
                   if(!strcmp((*attr), "yes")) {
                      bParse = 1;
                   }
                }
             }

             char* localized_text = gettext( tok->buf );

             if( localized_text ) {  // should always be true

                 if( !bParse ) {
                     // If no parsing is requested, then we can just add the text and we are done.  yay!
                     yatsstring_add(&my_buf, localized_text );
                 }
                 else {
                     // In order to do variable replacement within the l10n text
                     // we need to do more complicated processing.
                     
                     // A copy is necessary as gettext returns a statically allocated string
                     // and parse_buf modifies the input string temporarily.  Fixing parse_buf
                     // would be another option.
                     char* localized_text_copy = strdup( localized_text );
                     simple_list* section = parse_buf( localized_text_copy, CACHE_TEMPLATES_BOOL );
                     free( localized_text_copy );

                     bSuccess = fill_buf(f, &my_buf, section, tok->attrs, 0, row, 1);
                     if(bSuccess == FAILURE) {
                        break;
                     }
                 }
             }
         }
         else {
            /* Found an oddity. Complain */
            zend_error(E_ERROR,"Unknown token type:  %s", tok->buf);
            bSuccess = FAILURE;
            break;
         }
         tok = simple_list_next(tokens);
      }
      row++;
   } while (bLoop && row < num_rows);

   yatsstring_addn(buf, my_buf.str, my_buf.len);
   yatsstring_free(&my_buf);

   return bSuccess;
}

/************************
* End Utility Functions *
************************/


/****************
* Begin PHP API *
****************/

/* PHP API: assign one or more variables into a template */
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

/* Steps:
 * 1) Check if file in hash (already opened)
 * 2) Open file otherwise, and parse
 * 3) Store result in newly allocated structure.
 * 4) Return pointer to structure for later use.
 */
PHP_FUNCTION(yats_define)
{
   char *filepath;
   pval* arg;
   parsed_file* f;

   if (ARG_COUNT(ht) != 1 || getParameters(ht, 1, &arg) == FAILURE) {
      WRONG_PARAM_COUNT; /* prints/logs a warning and returns */
   }

   /* validate yats arg */
   convert_to_string(arg);
   filepath = arg->value.str.val;

   f = get_file_possibly_cached(filepath);
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

/**************
* End PHP API *
**************/


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 */
