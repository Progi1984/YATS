#include "php.h"
#include "php_yats.h"

extern ZEND_DECLARE_MODULE_GLOBALS(yats)


FILE* find_file( const char *file, const char* docroot, const char* curr_dir, char **searchpaths, char* filepath, int filepath_len, int* fsize ) {
    struct stat statbuf;
    int stat_rc = -1;

    if( file[0] != '/' ) {
        snprintf(filepath, filepath_len, "%s/%s", curr_dir, file );
        stat_rc = stat(filepath, &statbuf);
        if( stat_rc == -1 ) {
            char** p2 = searchpaths;
            while( *p2 && stat_rc != 0 ) {
                snprintf(filepath, filepath_len, "%s/%s", *p2, file );
                // zend_error(E_WARNING,"Searching for file in path: %s", filepath);

                stat_rc = stat(filepath, &statbuf);
                p2 ++;
            }
        }
        if( stat_rc != 0 ) {
            zend_error(E_ERROR,"Unable to locate file in path: %s", file);
        }
    }
    else {
        if( docroot ) {
            snprintf(filepath, filepath_len, "%s/%s", docroot, file );
        }
        else {
            strncpy( filepath, file, filepath_len );
        }
        stat_rc = stat(filepath, &statbuf);
        if( stat_rc != 0 ) {
            zend_error(E_ERROR,"Unable to locate file: %s", filepath);
        }
    }
    if( stat_rc == 0 ) {
        FILE* f = fopen(filepath, "r");
        if ( !f ) {
            zend_error(E_ERROR,"Unable to open file: %s", filepath);
        }
        else {
            *fsize = statbuf.st_size;
        }
        return f;
    }
    return 0;
}

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

/* parse a buffer/section.  recursive */
simple_list* parse_buf(parsed_file* pf, char* dir, char* buf, int bPerm) {
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
               sect = parse_buf(pf, dir, end + elen + 1, bPerm);
               token_add(tokens, start + 8, tok_end, section, attrs, sect, bPerm);
               *sec_end = tmp; /* restore */

               start = sec_end + end_token_len;
            } else {
               zend_error(E_ERROR,"Matching end token not found:  %s", end_token);
               start = end + elen + 1;
            }
            my_efree(end_token);
         }
         else if (!strncmp(start, "include ", 8)) {
             if( !dir || !pf ) {
                 zend_error(E_ERROR,"Evaluating dynamic content. Include not allowed here.");
             }
             else {
                 char **attr;
                 if (zend_hash_find(attrs, "file", strlen("file") + 1, (void**)&attr) == SUCCESS) {
                    char* file = *attr;

                    if( strstr( file, "../" ) ) {
                        zend_error(E_ERROR,"Invalid include. \"../\" is not allowed.  Included file was: %s", file);
                    }
                    else {
                        char filepath[1024];
                        int fsize;

                        FILE* fh = find_file( file, pf->docroot, dir, pf->searchpaths, filepath, sizeof(filepath), &fsize );

                        if( fh ) {
                           char* buf = emalloc(fsize + 1);

                           fread(buf, 1, fsize, fh);
                           fclose(fh);

                           buf[fsize] = 0;

                           char* d_end = strrchr( filepath, '/' );
                           if( d_end ) {
                               *d_end = 0;
                           }

                           simple_list* sect = parse_buf(pf, filepath, buf, bPerm);
                           token_add(tokens, filepath, filepath + strlen(filepath)-1, section, attrs, sect, bPerm);
                        }
                    }
                 }
                 else {
                     zend_error(E_ERROR,"Include missing file attribute");
                 }
             }
             start = end + elen + 1;
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
