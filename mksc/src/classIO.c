#include "mk.h"
#include <string.h>
#include <stdio.h>

static const MK_CHAR className[] = CLASS_INTERNAL_IO;
static const MK_CHAR classSuper[] = CLASS_INTERNAL_OBJECT;

static 
int mk_io_open( struct TAG_MK_VM_STRUCT *vm, unsigned int varArgC )
{
	MK_VM_FRAME_ITEM *mode = 
		mk_vm_pop_stack( &vm->localStack );
	MK_VM_FRAME_ITEM *targetName = 
		mk_vm_pop_stack( &vm->localStack );
	MK_VM_FRAME_ITEM *target = 
		mk_vm_pop_stack( &vm->localStack );
	MK_VM_FRAME_ITEM *result = NULL;
	MK_VM_FRAME_ITEM *fp = NULL;

	FILE *file = NULL;
	mk_find_item_hashtable( target->classTypeValue.variables,
		mk_get_symbol_name_ptr( vm, "fp" ), (void**)&fp );
	file = 
		fopen(targetName->stringTypeValue, mode->stringTypeValue );
	if( fp->nativeObjectTypeValue.nativePtr != NULL )
		fclose( fp->nativeObjectTypeValue.nativePtr );
	fp->nativeObjectTypeValue.nativePtr = file;
	if( fp == NULL )
		;	// TODO:IOException
	mk_vm_push_stack( &vm->localStack,
		mk_vm_create_bool_frame_item( file != NULL ) );
	return MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP;
}

static 
int mk_io_is_open( struct TAG_MK_VM_STRUCT *vm, unsigned int varArgC )
{
	MK_VM_FRAME_ITEM *target = 
		mk_vm_pop_stack( &vm->localStack );
	MK_VM_FRAME_ITEM *fp = NULL;
	FILE *file = NULL;

	mk_find_item_hashtable( target->classTypeValue.variables,
		mk_get_symbol_name_ptr( vm, "fp" ), (void**)&fp );
	mk_vm_push_stack( &vm->localStack,
		mk_vm_create_bool_frame_item( fp->nativeObjectTypeValue.nativePtr != NULL ) );
	return MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP;
}

static
int mk_io_reopen( struct TAG_MK_VM_STRUCT *vm, unsigned int varArgC )
{
	MK_VM_FRAME_ITEM *mode = 
		mk_vm_pop_stack( &vm->localStack );
	MK_VM_FRAME_ITEM *targetName = 
		mk_vm_pop_stack( &vm->localStack );
	MK_VM_FRAME_ITEM *target = 
		mk_vm_pop_stack( &vm->localStack );
	MK_VM_FRAME_ITEM *result = NULL;
	MK_VM_FRAME_ITEM *fp = NULL;

	mk_find_item_hashtable( target->classTypeValue.variables,
		mk_get_symbol_name_ptr( vm, "fp" ), (void**)&fp );
	if( fp->nativeObjectTypeValue.nativePtr != NULL )
	{
		fp->nativeObjectTypeValue.nativePtr = 
			freopen( 
				targetName->stringTypeValue, 
				mode->stringTypeValue, 
				fp->nativeObjectTypeValue.nativePtr );
	}
	else
	{
		// file do not opened exception.
	}
	mk_vm_push_stack( &vm->localStack,
		mk_vm_create_bool_frame_item( fp->nativeObjectTypeValue.nativePtr != NULL ) );
	return MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP;
}

static
int mk_io_print( struct TAG_MK_VM_STRUCT *vm, unsigned int varArgC )
{
	MK_VM_FRAME_ITEM *string = 
		mk_vm_pop_stack( &vm->localStack );
	MK_VM_FRAME_ITEM *target = 
		mk_vm_pop_stack( &vm->localStack );
	MK_VM_FRAME_ITEM *fp = NULL;

	mk_find_item_hashtable( target->classTypeValue.variables,
		mk_get_symbol_name_ptr( vm, "fp" ), (void**)&fp );
	if( fp->nativeObjectTypeValue.nativePtr != NULL )
	{
		fputs( string->stringTypeValue, fp->nativeObjectTypeValue.nativePtr );
	}
	else
	{
		// file do not opened exception.
	}
	mk_vm_push_stack( &vm->localStack,
		mk_create_vm_frame_item_object( &vm->pFrameItemTable ) );
	return MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP;
}

static
int mk_io_println( struct TAG_MK_VM_STRUCT *vm, unsigned int varArgC )
{
	MK_VM_FRAME_ITEM *string = 
		mk_vm_pop_stack( &vm->localStack );
	MK_VM_FRAME_ITEM *target = 
		mk_vm_pop_stack( &vm->localStack );
	MK_VM_FRAME_ITEM *fp = NULL;

	mk_find_item_hashtable( target->classTypeValue.variables,
		mk_get_symbol_name_ptr( vm, "fp" ), (void**)&fp );
	if( fp->nativeObjectTypeValue.nativePtr != NULL )
	{
		fputs( string->stringTypeValue, fp->nativeObjectTypeValue.nativePtr );
		fputs( "\n", fp->nativeObjectTypeValue.nativePtr );
	}
	else
	{
		// file do not opened exception.
	}
	mk_vm_push_stack( &vm->localStack,
		mk_create_vm_frame_item_object( &vm->pFrameItemTable ) );
	return MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP;
}

static
int mk_io_read( MK_VM_STRUCT *vm, MK_VM_FRAME_ITEM *target, MK_VM_FRAME_ITEM *fname, MK_VM_FRAME_ITEM *mode, MK_VM_FRAME_ITEM **result )
{
	return MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP;
}

static
int mk_io_write( MK_VM_STRUCT *vm, MK_VM_FRAME_ITEM *target, MK_VM_FRAME_ITEM *fname, MK_VM_FRAME_ITEM *mode, MK_VM_FRAME_ITEM **result )
{
	return MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP;
}

static
int mk_io_getc( MK_VM_STRUCT *vm, MK_VM_FRAME_ITEM *target, MK_VM_FRAME_ITEM *fname, MK_VM_FRAME_ITEM *mode, MK_VM_FRAME_ITEM **result )
{
	return MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP;
}

static
int mk_io_close( struct TAG_MK_VM_STRUCT *vm, unsigned int varArgC )
{
	MK_VM_FRAME_ITEM *target = 
		mk_vm_pop_stack( &vm->localStack );
	MK_VM_FRAME_ITEM *fp = NULL;

	mk_find_item_hashtable( target->classTypeValue.variables,
		mk_get_symbol_name_ptr( vm, "fp" ), (void**)&fp );
	if( fp != NULL &&
		fp->nativeObjectTypeValue.nativePtr != NULL )
	{
		fclose( (FILE*)fp->nativeObjectTypeValue.nativePtr );
		if( fp != NULL )
			fp->nativeObjectTypeValue.nativePtr = NULL;
	}
	else
	{
		;	// TODO: file do not open exception.
	}
	mk_vm_push_stack( &vm->localStack,
		mk_create_vm_frame_item_object( &vm->pFrameItemTable ) );
	return MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP;
}

static
int mk_io_lshift( MK_VM_STRUCT *vm, MK_VM_FRAME_ITEM *target, MK_VM_FRAME_ITEM *fname, MK_VM_FRAME_ITEM *mode, MK_VM_FRAME_ITEM **result )
{
	return MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP;
}


MK_CLASS *mk_create_io_class( MK_VM_STRUCT *vm )
{
	MK_CLASS *result =
		mk_create_object( MK_TYPE_CLASS );
	
	result->nameThis = mk_get_symbol_name_ptr( vm, className );
	result->nameSuper = mk_get_symbol_name_ptr( vm, classSuper );

	mk_register_variable( 
		vm,
		mk_create_default_native_method( 
			vm, 
			"open", 
			MK_TYPE_VARIABLE | 
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD | 
				MK_TYPE_ATTRIBUTE_VARIABLE_FINAL | 
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD_NATIVE |
				MK_TYPE_ATTRIBUTE_VARIABLE_NEW_NATIVE_METHOD,
			2, 
			(INT_PTR)mk_io_open ),
		result );

	mk_register_variable( 
		vm,
		mk_create_default_native_method( 
			vm, 
			"is_open", 
			MK_TYPE_VARIABLE | 
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD | 
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD_NATIVE |
				MK_TYPE_ATTRIBUTE_VARIABLE_NEW_NATIVE_METHOD,
			0, 
			(INT_PTR)mk_io_is_open ),
		result );

	mk_register_variable( 
		vm,
		mk_create_default_native_method( 
			vm, 
			"reopen", 
			MK_TYPE_VARIABLE | 
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD | 
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD_NATIVE |
				MK_TYPE_ATTRIBUTE_VARIABLE_NEW_NATIVE_METHOD,
			2, 
			(INT_PTR)mk_io_reopen ),
		result );

	mk_register_variable( 
		vm,
		mk_create_default_native_method( 
			vm, 
			"print", 
			MK_TYPE_VARIABLE | 
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD | 
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD_NATIVE |
				MK_TYPE_ATTRIBUTE_VARIABLE_NEW_NATIVE_METHOD |
				MK_TYPE_ATTRIBUTE_VARIABLE_NEW_NATIVE_METHOD,
			1, 
			(INT_PTR)mk_io_print ),
		result );

	mk_register_variable( 
		vm,
		mk_create_default_native_method( 
			vm, 
			"println", 
			MK_TYPE_VARIABLE | 
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD | 
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD_NATIVE |
				MK_TYPE_ATTRIBUTE_VARIABLE_NEW_NATIVE_METHOD,
			1, 
			(INT_PTR)mk_io_println ),
		result );

	mk_register_variable( 
		vm,
		mk_create_default_native_method( 
			vm, 
			"read", 
			MK_TYPE_VARIABLE | 
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD | 
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD_NATIVE |
				MK_TYPE_ATTRIBUTE_VARIABLE_NEW_NATIVE_METHOD,
			2, 
			(INT_PTR)mk_io_read ),
		result );

	mk_register_variable( 
		vm,
		mk_create_default_native_method( 
			vm, 
			"write", 
			MK_TYPE_VARIABLE | 
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD | 
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD_NATIVE |
				MK_TYPE_ATTRIBUTE_VARIABLE_NEW_NATIVE_METHOD,
			2, 
			(INT_PTR)mk_io_write ),
		result );

	mk_register_variable( 
		vm,
		mk_create_default_native_method( 
			vm, 
			"getc", 
			MK_TYPE_VARIABLE | 
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD | 
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD_NATIVE |
				MK_TYPE_ATTRIBUTE_VARIABLE_NEW_NATIVE_METHOD,
			0, 
			(INT_PTR)mk_io_getc ),
		result );

	mk_register_variable( 
		vm,
		mk_create_default_native_method( 
			vm, 
			"close", 
			MK_TYPE_VARIABLE | 
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD | 
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD_NATIVE |
				MK_TYPE_ATTRIBUTE_VARIABLE_NEW_NATIVE_METHOD,
			0, 
			(INT_PTR)mk_io_close ),
		result );

	mk_register_variable( 
		vm,
		mk_create_default_native_method( 
			vm, 
			"<<", 
			MK_TYPE_VARIABLE | 
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD | 
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD_NATIVE |
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD_OPERATOR |
				MK_LEX_TYPE_RESERVED_MARK_LSHIFT |
				MK_TYPE_ATTRIBUTE_VARIABLE_NEW_NATIVE_METHOD, 
			1, 
			(INT_PTR)mk_io_lshift ),
		result);

	mk_register_variable( 
		vm,
		mk_create_variable( vm, 
			"fp", 
			MK_TYPE_VARIABLE | 
				MK_TYPE_ATTRIBUTE_VARIABLE_VALUE |
				MK_TYPE_ATTRIBUTE_VARIABLE_READ_PROTECTED |
				MK_TYPE_ATTRIBUTE_VARIABLE_WRITE_PRIVATE |
				MK_TYPE_ATTRIBUTE_VARIABLE_FINAL,
			NULL ),
		result );

	return result;
}
