
/* Constants */
// Version
#define YATS_VERSION "0.97"
// Ini
#define YATS_INI_CACHE "yats_cache"
// Maximum include paths
#define YATS_MAX_INC_PATHS 20
// Parsing
#define TOKEN_START "{{"
#define TOKEN_END "}}"
#define TOKEN_GETTEXT "text"
// String
#define YATSSTRING_INCR 1024

/*
 * Enumerations
 */
typedef enum {
   variable,
   string,
   section,
   gettext_
} token_type;

// Macros
#if ZEND_MODULE_API_NO >= 20001222
#define my_zend_hash_get_current_key(ht, my_key, num_index) zend_hash_get_current_key(ht, my_key, num_index, 0)
#else
#define my_zend_hash_get_current_key(ht, my_key, num_index) zend_hash_get_current_key(ht, my_key, num_index)
#endif

#define my_free(x) if(x) free(x); x = 0
#define my_pefree(x, p) if(x) {if(p) free(x); else efree(x);} x = 0
#define my_efree(x) if(x) efree(x); x = 0

/**
 * Structures
 */
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

typedef struct _parsed_file {
   char* filepath;
   char* dir;
   char* docroot;
   char* filename;
   char* searchpaths[YATS_MAX_INC_PATHS + 1];
   char* buf;
   simple_list* tokens;
   HashTable* section_options;
   pval* assigned_vars;
   int isValid; /* Must equal 1 exactly. */
   time_t mtime;
} parsed_file;

typedef struct _token {
   token_type   type;
   char*        buf;
   simple_list* section;
   int          bHidden;
   HashTable*   attrs;
} token;

typedef struct _yatsstring {
   char* str;
   int len;
   int size;
} yatsstring;

typedef struct _per_request_section_options {
   signed int bHiddenAll;
   HashTable* hiddenRows;
} per_request_section_options;

typedef void (*list_iter_callback)(void *val, void* data);

// Functions
// yats_cache.c
int release_request_data(void** f);
parsed_file* get_file_possibly_cached(char* filepath, const char* docroot, const char* searchpath);
// yats_hash.c
HashTable* hash_init(dtor_func_t dtr, int bPerm);
void my_hash_destroy(HashTable* ht, int bPerm);
// yats_list.c
simple_list* simple_list_new(int bPerm);
void* simple_list_reset(simple_list* list);
void* simple_list_next(simple_list* list);
void simple_list_destroy(simple_list* list, list_iter_callback callback, void* data, int bPerm);
// yats_parser.c
simple_list* parse_buf(parsed_file* pf, char* dir, char* buf, int bPerm);
FILE* find_file( const char *file, const char* docroot, const char* curr_dir, char **searchpaths, char* filepath, int filepath_len, int* fsize );
// yats_token.c
void token_destroy(token* token, size_t bPerm);
void token_list_destroy(simple_list* list, size_t bPerm);
// yats_utils.c
void char_ptr_dtor_free(char** val);
void char_ptr_dtor_efree(char** val);
per_request_section_options* getSectionOptions(HashTable* section_hash, char* id, int id_len, int bCreate);
HashTable* get_section_hidden_rows(per_request_section_options* sec_ops);
