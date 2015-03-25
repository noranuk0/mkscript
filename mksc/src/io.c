#include "mk.h"
#include <stdio.h>

void mk_trace( unsigned int code, MK_CHAR *message )
{
	FILE *out = ( (code & MK_TRACE_TYPE_MASK) & MK_TRACE_TYPE_ERROR ) ? stderr : stdout;

	fprintf( out, "[%08x]", code );
	fputs( message, out );
	fputs( "\n", out );
}

void *mk_open( void*value )
{
	return fopen((const MK_CHAR*)value, "r" );
}

int mk_getc( void*stream )
{
	int result = fgetc((FILE*)stream);
	return result;
}

void mk_close( void*stream )
{
	fclose( (FILE*)stream );
}
