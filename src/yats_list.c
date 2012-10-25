#include "php.h"
#include "php_yats.h"

extern ZEND_DECLARE_MODULE_GLOBALS(yats)

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
