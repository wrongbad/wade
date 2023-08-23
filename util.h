#pragma once

namespace wade {

extern float sample_rate(); // user-defined
extern float tune_a4(); // user-defined

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

#ifdef WADE_USER_MALLOC
extern void * malloc(size_t bytes);
extern void free(void * ptr);
#else
void * malloc(size_t bytes) { return ::malloc(bytes); }
void free(void * ptr) { ::free(ptr); }
#endif


} // wade