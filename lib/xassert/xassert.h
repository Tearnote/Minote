#ifndef XASSERT_HEADER_
#define XASSERT_HEADER_
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

typedef int (*assert_ptr_t)(const char *, const char *, int, const char *);

#ifdef __cplusplus
extern "C" {
#endif
assert_ptr_t set_assert_handler(assert_ptr_t);
extern assert_ptr_t assert_handler_ptr;
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
#define ASSERT_HALT() (std::abort())
#else
#define ASSERT_HALT() (abort())
#endif

#define ASSERT_HANDLER(x, y, z, t) ((*assert_handler_ptr)(x, y, z, t))
#define XASSERT(x, m) (!(x) && ASSERT_HANDLER(#x, __FILE__, __LINE__, m) && (ASSERT_HALT(), 1))
#define ASSERT(x) XASSERT(x, NULL)

#ifndef NDEBUG
#define DASSERT(x) ASSERT(x)
#else
#define DASSERT(x) (0)
#endif

#endif
