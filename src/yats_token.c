#include "php.h"
#include "php_yats.h"

extern ZEND_DECLARE_MODULE_GLOBALS(yats)

/* free a token. recurses if token contains children */
void token_destroy(token* token, size_t bPerm) {
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
void token_list_destroy(simple_list* list, size_t bPerm) {
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
