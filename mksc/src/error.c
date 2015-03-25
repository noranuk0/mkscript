#include "mk.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#ifdef __GNUC__
#define isleadbyte(c)	0
#endif

const MK_CHAR errorMessageHeader[] = "%s(%d):E%08x:";

#define ERROR_EOF	"<EOF>"
#define ERROR_CR	"<CR>"

// compile error.
extern const MK_CHAR *compileErrorMessage[];
extern const MK_CHAR *linkErrorMessage[];
extern const MK_CHAR *vmErrorMessage[];
extern const MK_CHAR *compileWarningMessage[];
extern const MK_CHAR *linkWarningMessage[];
extern const MK_CHAR *vmWarningMessage[];

const MK_CHAR **errorMessages[] =
{
	compileErrorMessage,
	linkErrorMessage,
	vmErrorMessage,
	compileWarningMessage,
	linkWarningMessage,
	vmWarningMessage,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
};

static
size_t mk_format_header_length( MK_CHAR *name, int line )
{
	size_t size = strlen( errorMessageHeader );
	int lineTmp = line + 1;
	size += strlen( name ) - 2;	// "%s" => name
	
	// "%d" => line
	while(lineTmp != 0 )
	{
		size ++;
		lineTmp/=10;
	}
	size -= 2;
	
	// error code
	size += 4;	// %08x => 00000000

	return size + 1;
}

static 
unsigned int mk_format_length( MK_CHAR *name, int line, int errorNo, va_list list )
{
	const MK_CHAR *message = 
		errorMessages[MK_ERROR_TYPE_INDEX(errorNo)][MK_ERROR_NUMBER(errorNo)];
	size_t size = 0;
	MK_CHAR *current = NULL;

	size += mk_format_header_length( name, line ) - 1;
	size += strlen( message );
	size ++;	// \0

	do
	{
		current = 
			va_arg(list, MK_CHAR * );
		if( current != NULL )
		{
			if( current[0] == EOF )
				size += strlen(ERROR_EOF);
			else if( current[0] == '\r' ||
				current[0] == '\n' )
				size += strlen(ERROR_CR);
			else
				size += strlen( current );
		}
	} while( current != NULL );
	va_end(list);
	return size;
}

static
unsigned int mk_put_error_message_parts( MK_CHAR *buffer, unsigned int start, unsigned int partsIndex, va_list list )
{
	unsigned int result = start;
	unsigned int index = 0;
	MK_CHAR *target = NULL;

	for( index = 0; index < partsIndex; index ++ )
	{
		target = va_arg( list, MK_CHAR* );
		if( target == NULL )
			break;
	}
	if( target != NULL )
	{
		index = 0;
		if( target[0] == EOF )
			target = ERROR_EOF;
		else if( target[0] == '\r' || target[0] == '\n' )
			target = ERROR_CR;
		while( target[index] != '\0' )
		{
			buffer[result] = target[index];
			index ++;
			result ++;
		}
	}
	buffer[result] = '\0';
	return result;
}

int mk_get_internal_error_message( MK_CHAR *name, int line, MK_CHAR *buffer, size_t size, int errorNo, va_list list )
{
	unsigned int typeIndex = 
		MK_ERROR_TYPE_INDEX(errorNo);
	unsigned int errorIndex = 
		MK_ERROR_NUMBER(errorNo);
	const MK_CHAR *message = 
		errorMessages[typeIndex][errorIndex];	
	unsigned int index = 0;
	va_list base;
#ifdef _MSC_VER
	base = list;
#else
	va_copy( base, list );
#endif
	if( size < mk_format_header_length( name, line ) )
		return 0;

	sprintf( buffer, errorMessageHeader, name, line + 1, errorNo );
	for( index = strlen(buffer); index < size; index ++ )
	{
		if( *message == '\0' )
			break;

		if( isleadbyte(*message) )
		{
			buffer[index] = *message;
			message ++;
			buffer[index+1] = *message;
			message ++;
			index ++;
		}
		else if( *message == '$' )
		{
			if( *(message+1) >= '0' && *(message+1) <= '9' )
			{
#ifdef _MSC_VER
				list = base;
#else
				va_copy( list, base );
#endif
				index =
					mk_put_error_message_parts( buffer, index, *(message+1) - '0', list );
				index --;
				message += 2;
			}
			else if( *(message+1) == '$' )
			{
				buffer[index] = '$';
				message += 2;
			}
			else
			{
				message ++;
			}
		}
		else
		{
			buffer[index] = *message;
			message ++;
		}
	}
	buffer[index] = '\0';
	return index;
}

MK_VM_FRAME_ITEM *mk_create_internal_error_object( MK_VM_STRUCT *vm, MK_CHAR *nameStream, int line, MK_VM_FRAME_ITEM *classException, int errorNo, va_list list )
{
	MK_VM_FRAME_ITEM *result = 
		NULL;
	void *position = NULL;
	void *iterator = NULL;
	const MK_CHAR *key = NULL;
	MK_VARIABLE* value = NULL;
	MK_VM_FRAME_ITEM *newVariable, *variable = NULL;
	MK_CHAR *description = NULL;
	size_t sizeDescription = 0;
	MK_VM_FRAME_ITEM *reference = NULL;

	// create Exception Class
	mk_object_new( vm, classException, &result );

	// Variables( "id", "name", "line", "description" )
	// id
	reference = 
		mk_vm_find_variable_reference( vm, result, mk_get_symbol_name_ptr( vm, "id" ) );
	newVariable = mk_vm_create_int32_frame_item( vm, errorNo );
	mk_vm_push_stack( &vm->localStack, reference );
	mk_vm_push_stack( &vm->localStack, newVariable );
	mk_reference_equal( vm, 0 );
	mk_vm_pop_stack( &vm->localStack );

	// name
	reference = 
		mk_vm_find_variable_reference( vm, result, mk_get_symbol_name_ptr( vm, "name" ) );
	newVariable = 
		mk_create_vm_frame_item_object( &vm->pFrameItemTable );
	newVariable->flags = 
		MK_TYPE_SET_ATTRIBUTE( newVariable->flags, MK_VM_FRAME_ITEM_TYPE_STRING_VALUE );
	mk_copy_string( &newVariable->stringTypeValue, nameStream );
	mk_vm_push_stack( &vm->localStack, reference );
	mk_vm_push_stack( &vm->localStack, newVariable );
	mk_reference_equal( vm, 0 );
	mk_vm_pop_stack( &vm->localStack );

	// line
	reference = 
		mk_vm_find_variable_reference( vm, result, mk_get_symbol_name_ptr( vm, "line" ) );
	newVariable = mk_vm_create_int32_frame_item( vm, errorNo );
	mk_vm_push_stack( &vm->localStack, reference );
	mk_vm_push_stack( &vm->localStack, newVariable );
	mk_reference_equal( vm, 0 );
	mk_vm_pop_stack( &vm->localStack );

	// description
	reference = 
		mk_vm_find_variable_reference( vm, result, mk_get_symbol_name_ptr( vm, "description" ) );
	newVariable = 
		mk_create_vm_frame_item_object( &vm->pFrameItemTable );
	newVariable->flags = 
		MK_TYPE_SET_ATTRIBUTE( newVariable->flags, MK_VM_FRAME_ITEM_TYPE_STRING_VALUE );
	if( errorNo != 0 )
	{
		sizeDescription =
			mk_format_length( nameStream, line, errorNo, list );
		description = malloc( sizeDescription );
		mk_get_internal_error_message( nameStream, line, description, sizeDescription, errorNo, list );
			newVariable->stringTypeValue = description;
	}
	mk_vm_push_stack( &vm->localStack, reference );
	mk_vm_push_stack( &vm->localStack, newVariable );
	mk_reference_equal( vm, 0 );
	mk_vm_pop_stack( &vm->localStack );

	return result;
}

int mk_raise_internal_error( MK_VM_STRUCT *vm, MK_CHAR *name, int line, int errorNo, ... )
{
	MK_VM_FRAME_ITEM *errorInfo = NULL;
	MK_VM_FRAME_ITEM *exceptionClass = NULL;
	va_list list;
	MK_CHAR *className = NULL;
	va_start( list, errorNo );
	
	switch( MK_ERROR_TYPE( errorNo ) )
	{
	case MK_ERROR_TYPE_COMILE_ERROR:
		className = mk_get_symbol_name_ptr( vm, CLASS_INTERNAL_COMPILE_ERROR_EXCEPTION );
		break;

	case MK_ERROR_TYPE_VM_ERROR:
		className = mk_get_symbol_name_ptr( vm, CLASS_INTERNAL_VM_ERROR_EXCEPTION );
		break;

	default:
		break;
	}
	if( className != NULL )
	{
		mk_find_item_hashtable( vm->global, className, (void**)&exceptionClass );
		errorInfo = 
			mk_create_internal_error_object( vm, name, line, exceptionClass, errorNo, list );
		vm->exceptionObject = errorInfo;
	}
	return MK_VM_EXECUTE_EXPR_THROW;
}
