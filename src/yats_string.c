#include "php.h"
#include "php_yats.h"

extern ZEND_DECLARE_MODULE_GLOBALS(yats)


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
