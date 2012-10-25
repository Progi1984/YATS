#include "php.h"
#include "php_yats.h"

extern ZEND_DECLARE_MODULE_GLOBALS(yats)

/* load a file */
parsed_file* get_file(const char* filepath, const char* docroot, const char* searchpaths, int bPerm) {
   parsed_file* res = 0;
   char foundfilepath[1024];
   int fsize;

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
   res->docroot = docroot ? pestrdup( docroot, bPerm) : pestrdup( res->dir, bPerm );

   if( searchpaths && strlen( searchpaths ) ) {
       char* allpaths = strdup(searchpaths);
       char* ptr;
       int i = 0;

       char* next = strtok_r(allpaths, ":", &ptr);
       while( next && i < YATS_MAX_INC_PATHS ) {
           // zend_error(E_WARNING,"Found path: %s", next);

           res->searchpaths[i] = pestrdup(next, bPerm);
           next = strtok_r(0, ":", &ptr);
           i ++;
       }
       free( allpaths );
   }

   FILE* fh = find_file( filepath, NULL, res->dir, res->searchpaths, foundfilepath, sizeof(foundfilepath), &fsize );

   if( fh ) {
       res->filepath = pestrdup(foundfilepath, bPerm);
       buf = emalloc(fsize + 1);

       fread(buf, 1, fsize, fh);
       fclose(fh);

       buf[fsize] = 0;

       res->tokens = parse_buf(res, res->dir, buf, bPerm);
       my_efree(buf);
   }
   else {
       release_file( res, bPerm );
   }

   return res;
}

int release_request_data(void** file) {
   parsed_file* f = *(parsed_file**)file;


	/* nasty hack.  memory bug somewhere causes this to
	 * crash when cacheing templates. can't find the bug
	 * but it seems to not crash if we don't free these
	 */
	if(!YATS_G(iCache)) {
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
      if( f->docroot ) {
         my_pefree( f->docroot, bPerm );
      }
      if( f->filename ) {
         my_pefree( f->filename, bPerm );
      }
      if (f->tokens) {
         token_list_destroy(f->tokens, bPerm);
      }
      char** p = f->searchpaths;
      while( p && *p ) {
          my_pefree( *p, bPerm );
      }
      my_pefree(f, bPerm);

      return 1;
   }
   return 0;
}

/* file destructor, called by hashtable */
int release_file_perm(void** f) {
   return release_file(*(parsed_file**)f, YATS_G(iCache));
}

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
parsed_file* get_file_possibly_cached(char* filepath, const char* docroot, const char* searchpath) {
   struct stat statbuf;
   parsed_file* f = 0;

   if (!stat(filepath, &statbuf)) {
      /* see if we already have parsed file stored */
      if (YATS_G(htFileCache)) {
         parsed_file** f2;
         if (zend_hash_find(YATS_G(htFileCache), filepath, strlen(filepath) + 1, (void**)&f2) == SUCCESS) {
            if (f2) {
               if (statbuf.st_mtime <= (*f2)->mtime) {
                  per_req_init(*f2);
                  return *f2;
               } else {
                  /* out of date.  release */
                  zend_hash_del_key_or_index(YATS_G(htFileCache), filepath, strlen(filepath) +1, 0, HASH_DEL_KEY);
                  /* release_file(*f2, CACHE_TEMPLATES_BOOL); */
               }
            }
         }
      } else {
         /* This can live till process/module dies */
    	 YATS_G(htFileCache) = hash_init((dtor_func_t)release_file_perm, YATS_G(iCache));
         if (!YATS_G(htFileCache)) {
        	 zend_error(E_CORE_ERROR, "Unable to initialize hash");
         }
      }

      f = get_file(filepath, docroot, searchpath, YATS_G(iCache));
      if (f) {
         f->mtime = statbuf.st_mtime;
         per_req_init(f);
         zend_hash_update(YATS_G(htFileCache), filepath, strlen(filepath)+1, &f, sizeof(parsed_file*), NULL);
      }
   } else {
      f = get_file(filepath, docroot, searchpath, YATS_G(iCache));
      if (f) {
         per_req_init(f);
      }
   }
   return f;
}
