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

#ifndef _yats_H
#define _yats_H

#include "config.h"

#ifndef INIT_FUNC_ARGS
#include "modules.h"
#endif

extern php3_module_entry yats_module_entry;
#define yats_module_ptr &yats_module_entry

PHP_MINFO_FUNCTION(yats);

PHP_FUNCTION(yats_define);
PHP_FUNCTION(yats_assign);
PHP_FUNCTION(yats_getbuf);
PHP_FUNCTION(yats_getvars);
PHP_FUNCTION(yats_hide);

#define phpext_yats_ptr yats_module_ptr

#endif /* _yats_H */
