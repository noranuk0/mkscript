#ifndef _MK_CONFIG_H_
#define _MK_CONFIG_H_

#if _MSC_VER > 1300
// Microsoft VisualC++
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdlib.h>
#include <stdarg.h>
#ifdef _MSC_VER
#if defined(_DEBUG)
#include <crtdbg.h>
#undef malloc
#undef free
#define malloc(size)	_malloc_dbg(size, _NORMAL_BLOCK, __FILE__, __LINE__)
#define free(p)			_free_dbg(p, _NORMAL_BLOCK)
#endif
#endif

// type definition.

#define MK_SIZEOF_PTR	4
#define MK_CHAR			char
#define MK_SYM_CHAR		MK_CHAR	// must be return value of mk_get_symbol_name_ptr()
#define MK_FLOAT		double	// 8 byte float type

#define INT8		char
#define INT16		short
#define INT32		int
#define INT64		long long
#if __x86_64 != 0
#define INT_PTR		long			// 32bit / 64bit
#else
#define INT_PTR		int
#endif

#ifndef _MSC_VER
#define UINT_MAX	( (INT_PTR)0 - 1 )
#endif

#ifdef _MSC_VER
#define INLINE	__inline
#else
#define INLINE	static inline
#endif

// vm stack size
#define MK_SIZEOF_VM_STACK				64 * 1024
#define MK_SIZEOF_EXTEND_STACK			0	// no extend

// table size
#define MK_SIZEOF_VECTOR_DEFAULT		16
#define MK_SIZEOF_HASH_DEFAULT			16
#define MK_SIZEOF_EXTEND_VECTOR_DEFAULT	16
#define MK_SIZEOF_EXTEND_HASH_DEFAULT	16

#define MK_SIZEOF_SYMBOLNAME_HASH_DEFAULT	256
#define MK_SIZEOF_EXTEND_SYMBOLNAME_HASH	64

#define MK_BITS_MEMORY_POOL_BLOCK		12
#define MK_SIZEOF_MEMORY_POOL_BLOCK		( 1 << ( MK_BITS_MEMORY_POOL_BLOCK - 1 ) )	// 4kb

#define MK_SIZEOF_MANAGED_TABLE			512										// need <this value> % sizeof(unsigned int) == 0 
#define GC_EXECUTE_TIME_PER_CALL		1024
#define LIMIT_OF_TABLE_COUNT			-1

#define MK_SIZEOF_MAX_STACK_SIZE		1024 * 640		// max stack size : 640kb

void mk_trace( unsigned int code, MK_CHAR *message );
void *mk_open( void*value );
int mk_getc( void*stream );
void mk_close( void*stream );

#endif
