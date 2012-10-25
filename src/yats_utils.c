#include "php.h"
#include "php_yats.h"
#include <libintl.h>

extern ZEND_DECLARE_MODULE_GLOBALS(yats)

void char_ptr_dtor_free(char** val) {
   my_free(*val);
}

void char_ptr_dtor_efree(char** val) {
	// crashes (later) if we use my_efree.
   efree(*val);
}

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
                     simple_list* section = parse_buf( 0, 0, localized_text_copy, YATS_G(iCache) );
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
