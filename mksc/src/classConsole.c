#include "mk.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static const MK_CHAR className[] = CLASS_INTERNAL_CONSOLE;
static const MK_CHAR classSuper[] = CLASS_INTERNAL_IO;

static
int mk_console_print( struct TAG_MK_VM_STRUCT *vm, unsigned int varArgC )
{
	MK_VM_FRAME_ITEM *display = 
		mk_vm_pop_stack( &vm->localStack );
	MK_VM_FRAME_ITEM *console =
		mk_vm_pop_stack( &vm->localStack );

	if( (INT_PTR)display & MK_VM_FRAME_ITEM_TYPE_DIRECT_VALUE ||
		MK_TYPE_ATTRIBUTE( display->flags ) != MK_VM_FRAME_ITEM_TYPE_STRING_VALUE )
	{
		MK_VARIABLE *pMethod = NULL;
		int retCode = 0;

		// call display.to_s()
		pMethod = 
			mk_vm_find_method( vm, 
				display, 
				mk_get_symbol_name_ptr( vm, "to_s" ), 
				0 );
		if( pMethod != NULL )
		{
			mk_vm_push_stack( &vm->localStack, display );
			retCode = 
				mk_vm_call_method( vm, pMethod, NULL, 0 );
			if( retCode == MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP )
			{
				MK_VM_FRAME_ITEM *converted = 
					mk_vm_pop_stack( &vm->localStack );
				if( converted != NULL && 
					MK_TYPE_ATTRIBUTE( converted->flags ) == MK_VM_FRAME_ITEM_TYPE_STRING_VALUE )
				{
					mk_vm_push_stack( &vm->localStack, console );
					mk_vm_push_stack( &vm->localStack, converted );
					return mk_console_print( vm, varArgC );
				}
			}
		}
		return MK_VM_EXECUTE_EXPR_THROW;	// TODO: cannot convert to String.
	}
	else
	{
		fputs( display->stringTypeValue, stdout );
		mk_vm_push_stack( &vm->localStack,
			mk_create_vm_frame_item_object( &vm->pFrameItemTable ) );
	}
	return MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP;
}

static
int mk_console_print_ln( struct TAG_MK_VM_STRUCT *vm, unsigned int varArgC )
{
	int retCode = 
		mk_console_print( vm, varArgC );
	if( retCode == MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP )
		fputs( "\n", stdout );
	return retCode;
}

static
int mk_console_lshift( struct TAG_MK_VM_STRUCT *vm, unsigned int varArgC )
{
	MK_VM_FRAME_ITEM *console = 
		( MK_VM_FRAME_ITEM * )mk_vm_get_at_stack( &vm->localStack, 1 );
	int retCode =
		mk_console_print( vm, varArgC );
	if( retCode == MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP )
		mk_vm_set_at_stack( &vm->localStack, 0, console );
	return retCode;
}

static
int mk_console_getc( struct TAG_MK_VM_STRUCT *vm, unsigned int varArgC )
{
	MK_VM_FRAME_ITEM *result = 
		mk_create_vm_frame_item_object( &vm->pFrameItemTable );
	result->flags = MK_TYPE_SET_ATTRIBUTE( result->flags, MK_VM_FRAME_ITEM_TYPE_INT_VALUE );
	result->int32TypeValue = fgetc( stdin );
	return MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP;
}

MK_CLASS *mk_create_console_class( MK_VM_STRUCT *vm )
{
	MK_CLASS *result =
		mk_create_object( MK_TYPE_CLASS );
	
	result->nameThis = mk_get_symbol_name_ptr( vm, className );
	result->nameSuper = mk_get_symbol_name_ptr( vm, classSuper );

	mk_register_variable(
		vm,
		mk_create_default_native_method(
			vm, 
			"print",
			MK_TYPE_VARIABLE | 
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD | 
				MK_TYPE_ATTRIBUTE_VARIABLE_STATIC |
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD_NATIVE |
				MK_TYPE_ATTRIBUTE_VARIABLE_NEW_NATIVE_METHOD,
			1,
			(INT_PTR)mk_console_print ),
		result );

	mk_register_variable(
		vm,
		mk_create_default_native_method(
			vm, 
			"println",
			MK_TYPE_VARIABLE | 
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD | 
				MK_TYPE_ATTRIBUTE_VARIABLE_STATIC |
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD_NATIVE |
				MK_TYPE_ATTRIBUTE_VARIABLE_NEW_NATIVE_METHOD,
			1,
			(INT_PTR)mk_console_print_ln ),
		result );

	mk_register_variable( 
		vm,
		mk_create_default_native_method( 
			vm, 
			"<<", 
			MK_TYPE_VARIABLE | 
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD | 
				MK_TYPE_ATTRIBUTE_VARIABLE_STATIC |
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD_NATIVE |
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD_OPERATOR |
				MK_LEX_TYPE_RESERVED_MARK_LSHIFT |
				MK_TYPE_ATTRIBUTE_VARIABLE_NEW_NATIVE_METHOD,
			1, 
			(INT_PTR)mk_console_lshift ),
		result);

	mk_register_variable(
		vm,
		mk_create_default_native_method(
			vm, 
			"getc",
			MK_TYPE_VARIABLE | 
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD | 
				MK_TYPE_ATTRIBUTE_VARIABLE_STATIC |
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD_NATIVE |
				MK_TYPE_ATTRIBUTE_VARIABLE_NEW_NATIVE_METHOD,
			0,
			(INT_PTR)mk_console_getc ),
		result );
	return result;
}
