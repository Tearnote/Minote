/*
 * xassert - C/C++ custom assertion macros.
 *
 * Written in 2014 by Ricardo Garcia <r@rg3.name>
 *
 * To the extent possible under law, the author(s) have dedicated all copyright
 * and related and neighboring rights to this software to the public domain
 * worldwide. This software is distributed without any warranty. 
 *
 * You should have received a copy of the CC0 Public Domain Dedication along
 * with this software. If not, see
 * <http://creativecommons.org/publicdomain/zero/1.0/>.
 */
#include <stdio.h>
#include <stdlib.h>
#include "xassert.h"

#ifdef __cplusplus
extern "C" {
#endif

assert_ptr_t
set_assert_handler(assert_ptr_t handler)
{
   assert_ptr_t old = assert_handler_ptr;
   assert_handler_ptr = handler;
   return old;
}

static int
default_handler(const char *expr, const char *file, int line, const char *msg)
{
   fprintf(stderr, "Assertion failed on %s line %d: %s\n", file, line, expr);
   if (msg != NULL)
      fprintf(stderr, "Diagnostic: %s\n", msg);
   return 1;
}

assert_ptr_t assert_handler_ptr = default_handler;

#ifdef __cplusplus
}
#endif

