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
#include "php3_tmpl.h"
#include "php_ini.h"

#include <libintl.h>
#include <sys/stat.h>


#ifdef COMPILE_DL_TMPL
ZEND_GET_MODULE(tmpl)
#endif

#if ZEND_MODULE_API_NO >= 20001222
#define my_zend_hash_get_current_key(ht, my_key, num_index) zend_hash_get_current_key(ht, my_key, num_index, 0)
#else
#define my_zend_hash_get_current_key(ht, my_key, num_index) zend_hash_get_current_key(ht, my_key, num_index)
#endif 


/* Some stuff to make it easier if we want to become thread safe */
typedef struct _tmpl_globals {
   HashTable* tmpl_hash;
} tmpl_globals;

tmpl_globals t_g = {0};

#define TMPL_GLOBALS(var) t_g.var

#define TMPL_CACHE_STR "tmpl_cache"
// #define CACHE_TEMPLATES_BOOL INI_INT(TMPL_CACHE_STR)
#define CACHE_TEMPLATES_BOOL 0

/* forward decl */
int release_request_data(void** f);

HashTable* hash_init(dtor_func_t dtr, int bPerm) {
   HashTable* ht = pemalloc(sizeof(HashTable), bPerm);
   if(ht) {
      if(zend_hash_init(ht, 0, NULL, dtr, bPerm) != SUCCESS) {
         pefree(ht, bPerm);
         ht = 0;
      }
   }
   return ht;
}

void hash_destroy(HashTable* ht, int bPerm) {
   zend_hash_destroy(ht);
   pefree(ht, bPerm);
}


/* module entrypoint */
function_entry php_tmpl_functions[] = {
   PHP_FE(tmpl_define, NULL)
   PHP_FE(tmpl_assign, NULL)
   PHP_FE(tmpl_getbuf, NULL)
   PHP_FE(tmpl_getvars, NULL)
   PHP_FE(tmpl_hide, NULL)
   {NULL, NULL, NULL}
};

PHP_INI_BEGIN()
   PHP_INI_ENTRY(TMPL_CACHE_STR,"1", PHP_INI_ALL, NULL)
PHP_INI_END()


PHP_MINIT_FUNCTION(tmpl)
{
	REGISTER_INI_ENTRIES();
	return SUCCESS;
}


/* called on module shutdown */
PHP_MSHUTDOWN_FUNCTION(tmpl)
{
   HashTable* ht = TMPL_GLOBALS(tmpl_hash);
   if (ht) {
      hash_destroy(ht, CACHE_TEMPLATES_BOOL);
      TMPL_GLOBALS(tmpl_hash) = 0;
   }

   UNREGISTER_INI_ENTRIES();

   return SUCCESS;
}



/* called at end of request  */
PHP_RSHUTDOWN_FUNCTION(tmpl)
{
   HashTable* ht = TMPL_GLOBALS(tmpl_hash);
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
      } while(zend_hash_move_forward(ht));

      if(!CACHE_TEMPLATES_BOOL) {
         hash_destroy(ht, CACHE_TEMPLATES_BOOL);
         TMPL_GLOBALS(tmpl_hash) = 0;
      }
   }

   return SUCCESS;
}


/* define entrypoints */
php3_module_entry tmpl_module_entry = {
   "tmpl",                           /* extension name */
   php_tmpl_functions,               /* function list */
   PHP_MINIT(tmpl),                  /* process startup */
   PHP_MSHUTDOWN(tmpl),              /* process shutdown */
   NULL,                             /* request startup */
   PHP_RSHUTDOWN(tmpl),              /* request shutdown */
   PHP_MINFO(tmpl),                  /* extension info */
   NULL,                             /* global startup function */
   NULL,                             /* global shutdown function */   
   STANDARD_MODULE_PROPERTIES_EX
};

/* module description */
PHP_MINFO_FUNCTION(tmpl)
{
   php_printf("tmpl: Template Support. (a.k.a. YATS -- Yet Another Template System)<br>\n"
              "author: Dan Libby. dan@libby.com<br>\n"
              "Epinions.com<br>\n");
}

void char_ptr_dtor_free(char** val) {
   free(*val);
}

void char_ptr_dtor_efree(char** val) {
   efree(*val);
}


/******************************
* Begin Simple List Functions *
******************************/

typedef enum {
   variable,
   string,
   section
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
         pefree(free_me, bPerm);
      }
      pefree(list, bPerm);
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

/* free a token. recurses if token contains children */
void token_destroy(token* token, int bPerm) {
   if(token) {
      if (token->section) {
         simple_list_destroy(token->section, (list_iter_callback)token_destroy, (void*)bPerm, bPerm);
      }
      if (token->buf) {
         pefree(token->buf, bPerm);
      }
      if(token->attrs) {
         zend_hash_destroy(token->attrs);
         pefree(token->attrs, bPerm);
      }
      pefree(token, bPerm);
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

/* find token with a given id. recursive */
void token_find_worker(simple_list* result_list, simple_list* search_list, char* id, int bRecurse) {
   token* tok = (token*)simple_list_reset(search_list);
   while (tok) {
      if (!strcmp(tok->buf, id)) {
         simple_list_add(result_list, tok, 0);
      }
      if (bRecurse && simple_list_length(tok->section) > 0) {
         token_find_worker(result_list, tok->section, id, bRecurse);
      }
      tok = simple_list_next(search_list);
   }
}

/* find token with a given id, caller should free results, if any. */
simple_list* token_find(simple_list* search_list, char* id, int bRecurse)
{
   simple_list* res = simple_list_new(0);
   token_find_worker(res, search_list, id, bRecurse);
   if (res->start) {
      return res;
   }
   simple_list_destroy(res, 0, 0, 0);
   return NULL;
}

/**********************
* End Token Functions *
**********************/


/*************************
* Begin String Functions *
*************************/

typedef struct _tmplstring {
   char* str;
   int len;
   int size;
} tmplstring;

#define TMPLSTRING_INCR 1024


/* initialize string */
void tmplstring_init(tmplstring* string) {
   string->str = emalloc(TMPLSTRING_INCR);
   if (string->str) {
      string->str[0] = 0;
      string->len = 0;
      string->size = TMPLSTRING_INCR;
   } else {
      string->size = 0;
   }
}

/* clear contents of string. does not free memory */
void tmplstring_clear(tmplstring* string) {
   string->str = 0;
   string->len = 0;
}

/* release resources for string */
void tmplstring_free(tmplstring* string) {
   if (string && string->str) {
      efree(string->str);
   }
}

/* add a string of known length */
void tmplstring_addn(tmplstring* string, char* add, int add_len) {
   if (add && add_len) {
      if (string->len + add_len + 1 > string->size) {
         /* newsize is current length + new length */
         int newsize = string->len + add_len + 1;

         /* align to TMPLSTRING_INCR increments */
         newsize = newsize - (newsize % TMPLSTRING_INCR) + TMPLSTRING_INCR;
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
void tmplstring_add(tmplstring* string, char* add) {
   if(add) {
      tmplstring_addn(string, add, strlen(add));
   }
}


/***********************
* End String Functions *
***********************/


/**************************
* Begin Parsing Functions *
**************************/

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
         bSuccess = FAILURE;
         while((int)*(++p) && isspace(*p)); /* advance to key */
         if(*p) {
            key = p;
            while(*(++p) && isalnum(*p)); /* to end of key.  alpha numerics only */
            if(*p) {
               key = bPerm ? (char*)zend_strndup(key, p - key) : estrndup(key, p - key);
               while(*(p) && isspace(*p)) p++;
               if(*p == '=') {
                  while(*(++p) && isspace(*p)); /* advance to value */
                  if(*p && *p == '"') {
                     value = p + 1;
                     while(*(++p) && isprint(*p) && *p != '"'); /* to end of value. */
                     if(*p && *p == '"') {
                        value = bPerm ? (char*)zend_strndup(value, p - value) : estrndup(value, p - value);
                     }
                  }
               }
            }
         }
         if(key && value) {
            bSuccess = zend_hash_update(attrs, key, strlen(key)+1, (void *)&value, sizeof(char *), NULL);
            /* bSuccess = add_assoc_string(attrs, key, value, 1); */
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
/* parse a buffer/section.  recursive */
simple_list* parse_buf(char* buf, int bPerm) {
   simple_list* tokens = simple_list_new(CACHE_TEMPLATES_BOOL);
   char* start = buf; /* start of current token */
   char* end = 0; /* end of current token */
   char* p = strstr(start, TOKEN_START);
   int slen = strlen(TOKEN_START);
   int elen = strlen(TOKEN_END);
   
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
            efree(end_token);
         } else {
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
   char* filename;
   char* buf;
   simple_list* tokens;
   HashTable* section_options;
   pval* assigned_vars;
   int isValid; /* Must equal 1 exactly. */
   time_t mtime; 
} parsed_file;


/* load a file */
parsed_file* get_file(const char* filename, int file_len, int bPerm) {
   parsed_file* res = 0;
   FILE* f = fopen(filename, "r");
   if (f) {
      char* buf;
      res = pecalloc(1, sizeof(parsed_file), bPerm);

      res->filename = pestrdup(filename, bPerm);
      buf = emalloc(file_len + 1);

      fread(buf, 1, file_len, f);
      fclose(f);

      buf[file_len] = 0;

      res->tokens = parse_buf(buf, bPerm);
      efree(buf);
   } else {
      zend_error(E_WARNING,"Unable to open template %s", filename);
   }
   return res;
}

int release_request_data(void** file) {
   parsed_file* f = *(parsed_file**)file;

   /* These are always per request, so use efree */
   if (f->assigned_vars) {
      zval_dtor(f->assigned_vars);
      FREE_ZVAL(f->assigned_vars);
   }
   if (f->section_options) {
      hash_destroy(f->section_options, 0);
   }
   return 1;
}

/* Releases file resources */
int release_file(parsed_file* f, int bPerm) {
   if (f) {
      if (f->filename) {
         pefree(f->filename, bPerm);
      }
      if (f->tokens) {
         token_list_destroy(f->tokens, bPerm);
      }
      pefree(f, bPerm);

      return 1;
   }
   return 0;
}

/* file destructor, called by hashtable */
int release_file_perm(void** f) {
   return release_file(*(parsed_file**)f, CACHE_TEMPLATES_BOOL);
}

typedef struct _per_request_section_options {
   int bHidden;
} per_request_section_options;

int release_section_options(void** op) {
   if(op) {
      per_request_section_options* options = *(per_request_section_options**)op;
      if(options) {
         efree(options);
         return 1;
      }
   }
   return 0;
}

/* Init per request variables for file */
void per_req_init(parsed_file* f) {
   f->isValid = 0;
   MAKE_STD_ZVAL(f->assigned_vars);
   f->section_options = hash_init((dtor_func_t)release_section_options, CACHE_TEMPLATES_BOOL);
   if (f->assigned_vars && array_init(f->assigned_vars) != FAILURE && f->section_options) {
      if (f->tokens) {
         f->isValid = 1;
      }
   }
}

/* Retrieves file. Uses cached version if available, unless timestamp newer */
parsed_file* get_file_possibly_cached(char* filename) {
   struct stat statbuf;
   parsed_file* f = 0;

   if (!stat(filename, &statbuf)) {
      /* see if we already have parsed file stored */
      if (TMPL_GLOBALS(tmpl_hash)) {
         parsed_file** f2;
         if (zend_hash_find(TMPL_GLOBALS(tmpl_hash), filename, strlen(filename) + 1, (void**)&f2) == SUCCESS) {
            if (f2) {
               if (statbuf.st_mtime <= (*f2)->mtime) {
                  per_req_init(*f2);
                  return *f2;
               } else {
                  /* out of date.  release */
                  zend_hash_del_key_or_index(TMPL_GLOBALS(tmpl_hash), filename, strlen(filename) +1, 0, HASH_DEL_KEY);
                  /* release_file(*f2, CACHE_TEMPLATES_BOOL); */
               }
            }
         }
      } else {
         /* This can live till process/module dies */
         TMPL_GLOBALS(tmpl_hash) = hash_init((dtor_func_t)release_file_perm, CACHE_TEMPLATES_BOOL);
         if (!TMPL_GLOBALS(tmpl_hash)) {
            zend_error(E_CORE_ERROR, "Unable to initialize hash");
         }
      }

      f = get_file(filename, statbuf.st_size, CACHE_TEMPLATES_BOOL);
      if (f) {
         f->mtime = statbuf.st_mtime;
         per_req_init(f);
         zend_hash_update(TMPL_GLOBALS(tmpl_hash), filename, strlen(filename)+1, &f, sizeof(parsed_file*), NULL);
      }
   } else {
      zend_error(E_WARNING,"template not found: %s", filename);
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
   per_request_section_options* sec_ops = 0;
   if(zend_hash_find(section_hash, 
                     id, 
                     id_len + 1, 
                     (void**)&sec_ops) != SUCCESS) {

      if(bCreate) {
         /* free'd at end of request in release_section_options() */
         sec_ops = ecalloc(1, sizeof(per_request_section_options)); 

         if(sec_ops) {
            zend_hash_update(section_hash, id, id_len+1, (void *)&sec_ops, sizeof(per_request_section_options *), NULL);
         }
      }
   }
   return sec_ops;
}


/* add a key/value pair to file.  */
int add_val(parsed_file *f, char* var_id, pval* arg) {
   pval *res, *tmp;

   MAKE_STD_ZVAL(res);

   if (res && array_init(res) != FAILURE) {
      if (arg->type == IS_ARRAY) {
         zend_hash_copy(res->value.ht, arg->value.ht, (void (*)(void *)) zval_add_ref, (void *) &tmp, sizeof(zval *));
      } else {
         convert_to_string(arg);
         if (!arg->value.str.val || (add_next_index_string(res, arg->value.str.val, 1) == FAILURE)) {
            return FAILURE;
         }
      }
      if (zend_hash_update(f->assigned_vars->value.ht, var_id, strlen(var_id)+1, (void *)&res, sizeof(pval *), NULL) == SUCCESS) {
         return SUCCESS;
      }
   }
   return FAILURE;
}

/* Interpolate tokens and variables.  Recursive */
int fill_buf(parsed_file* f, tmplstring* buf, simple_list* tokens, HashTable* attrs) {
   int iter = 0;
   int num_iters = 1;
   int bSuccess = SUCCESS;

   tmplstring my_buf;
   tmplstring_init(&my_buf);

   while (iter < num_iters) {
      token* tok = simple_list_reset(tokens);
      while (tok) {
         if (tok->type == string) {
            /* Found a string.  Add it */
            tmplstring_add(&my_buf, tok->buf);
         } else if (tok->type == variable) {
            /* Found a variable. Look it up and fill it in */
            pval **res, **res2;
            int bFound = 0;
            if (zend_hash_find(f->assigned_vars->value.ht, tok->buf, strlen(tok->buf) + 1, (void**)&res) == SUCCESS) {
               int num_elem = zend_hash_num_elements((*res)->value.ht);
               int idx = iter;
               int bRepeatScalar = 0;

               /* If variable is scalar, then we will repeat it unless repeatscalar="no" is specified */
               if(num_elem == 1) {
                  if(tok->attrs) {
                     char** attr;
                     if (zend_hash_find(tok->attrs, "repeatscalar", strlen("repeatscalar") + 1, (void**)&attr) == SUCCESS) {
                        if(!strcmp((*attr), "yes")) {
                           bRepeatScalar = 1;
                           idx = 0;
                        }
                     }
                  }
               }

               /* find minimum number */
               if ( (num_elem > bRepeatScalar )  && (num_elem < num_iters || num_iters <= 1)) {
                  num_iters = num_elem;
               }

               /* look up value at index */
               if (zend_hash_index_find((*res)->value.ht, idx, (void**)&res2) == SUCCESS && (*res2)->type == IS_STRING) {
                  tmplstring_add(&my_buf, (*res2)->value.str.val);
                  bFound = 1;
               }
            }
            if (!bFound) {
               char** attr;
               /* check for alt text */
               if (tok->attrs && 
                   zend_hash_find(tok->attrs, "alt", strlen("alt") + 1, (void**)&attr) == SUCCESS) {
                  tmplstring_add(&my_buf, (*attr));
               }
               /* otherwise, don't show this section, unless autohide="no" is specified. */
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
                     tmplstring_clear(&my_buf);
                     break;
                  }
               }
            }
         } else if (tok->type == section) {
            per_request_section_options* sec_ops = getSectionOptions(f->section_options, tok->buf, strlen(tok->buf), 0);
            if (!sec_ops || !sec_ops->bHidden) {
               /* Found a section.  Recurse */
               if (fill_buf(f, &my_buf, tok->section, tok->attrs) == FAILURE) {
                  bSuccess = FAILURE;
                  break;
               }
            }
         } else {
            /* Found an oddity. Complain */
            zend_error(E_ERROR,"Unknown token type:  %s", tok->buf);
            bSuccess = FAILURE;
            break;
         }
         tok = simple_list_next(tokens);
      }
      iter++;
   }

   tmplstring_addn(buf, my_buf.str, my_buf.len);
   tmplstring_free(&my_buf);

   return bSuccess;
}

/************************
* End Utility Functions *
************************/


/****************
* Begin PHP API *
****************/

/* PHP API: assign one or more variables into a template */
PHP_FUNCTION(tmpl_assign)
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
            if(key) {
               pefree(key, arg2->value.ht->persistent);
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

      /* validate tmpl arg */
      f = (parsed_file*)arg1->value.lval;
      if (f && f->isValid == 1) {
         char* var_id;
         convert_to_string(arg2);
         var_id = arg2->value.str.val;

         /* add value to tmpl */
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
PHP_FUNCTION(tmpl_define)
{
   char *filename;
   pval* arg;
   parsed_file* f;

   if (ARG_COUNT(ht) != 1 || getParameters(ht, 1, &arg) == FAILURE) {
      WRONG_PARAM_COUNT; /* prints/logs a warning and returns */
   }

   /* validate tmpl arg */
   convert_to_string(arg);
   filename = arg->value.str.val;

   f = get_file_possibly_cached(filename);
   if (!f) {
      /* error getting data from server */
      RETURN_FALSE; 
   }

   RETVAL_RESOURCE((long)f);
}

/* PHP API: interpolate and return buffer */
PHP_FUNCTION(tmpl_getbuf)
{
   pval *arg;
   parsed_file* f;

   tmplstring buf;
   tmplstring_init(&buf);

   RETVAL_FALSE;

   if (ARG_COUNT(ht) != 1 || getParameters(ht, 1, &arg) == FAILURE) {
      WRONG_PARAM_COUNT; /* prints/logs a warning and returns */
   }

   /* validate tmpl arg */
   f = (parsed_file*)arg->value.lval;
   if (f && f->isValid == 1) {
      if (fill_buf(f, &buf, f->tokens, NULL) == SUCCESS) {
         RETVAL_STRING(buf.str, 0); /* do not duplicate */
      }
   } else {
      zend_error(E_ERROR,"Invalid template object passed as param 1 for %s()",get_active_function_name());
   }

   /* This will be free'd by PHP since it was allocated with emalloc.
    * We don't free it here because we didn't make a copy for
    * the return value. A copy would be cleaner, but slower.
    *
    tmplstring_free(&buf);
    */

   return;
}

/* PHP API: hide/un-hide a section */
PHP_FUNCTION(tmpl_hide)
{
   pval *arg1, *arg2, *arg3;
   parsed_file* f;


   if (ARG_COUNT(ht) != 3 || getParameters(ht, 3, &arg1, &arg2, &arg3) == FAILURE) {
      WRONG_PARAM_COUNT; /* prints/logs a warning and returns */
   }

   /* validate tmpl arg */
   f = (parsed_file*)arg1->value.lval;
   if (f && f->isValid == 1) {
      per_request_section_options* sec_ops = 0;

      convert_to_string(arg2);
      convert_to_long(arg3);

      sec_ops = getSectionOptions(f->section_options, arg2->value.str.val, arg2->value.str.len, 1);
      if(sec_ops) {
         sec_ops->bHidden = arg3->value.lval ? 1 : 0;
      }
      RETURN_TRUE;

   } else {
      zend_error(E_ERROR,"Invalid template object passed as param 1 for %s()",get_active_function_name());
   }

   RETURN_FALSE;
}

/* PHP API: return list of vars */
PHP_FUNCTION(tmpl_getvars)
{
   pval* arg1;
   parsed_file* f;

   if (ARG_COUNT(ht) != 1 || getParameters(ht, 1, &arg1) == FAILURE) {
      WRONG_PARAM_COUNT; /* prints/logs a warning and returns */
   }

   /* validate tmpl arg */
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
