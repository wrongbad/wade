#pragma once

namespace wade {

#ifdef WADE_DEBUG
#ifdef WADE_USER_ASSERT
extern void assert_failed(char const* expr, char const* file, char const* line); // user-defined
#define wade_assert(e) (!(e) ? assert_failed(#e, __FILE__, __LINE__) : (void)0)
#else
#define wade_assert(e) assert(e)
#endif
#else
#define wade_assert(e) 
#endif

} // wade