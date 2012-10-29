#include "php.h"
#include "php_yats.h"

extern ZEND_DECLARE_MODULE_GLOBALS(yats)

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

void my_hash_destroy(HashTable* ht, int bPerm) {
   if( ht ) {
      zend_hash_destroy(ht);
      my_pefree(ht, bPerm);
   }
}
