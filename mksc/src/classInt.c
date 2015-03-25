#include "mk.h"
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <stdio.h>

static const MK_CHAR className[] = CLASS_INTERNAL_INTEGER;
static const MK_CHAR classSuper[] = CLASS_INTERNAL_OBJECT;

static
int mk_int_to_s( struct TAG_MK_VM_STRUCT *vm, unsigned int varArgC )
{
	MK_CHAR value[32];
	MK_VM_FRAME_ITEM *result = 
		mk_create_vm_frame_item_object( &vm->pFrameItemTable );
	MK_VM_FRAME_ITEM *target =
		mk_vm_pop_stack( &vm->localStack );
	result->flags |= MK_VM_FRAME_ITEM_TYPE_STRING_VALUE;
	sprintf( value, "%d", mk_vm_frame_item_to_int32( vm, target ) );
	mk_copy_string( &result->stringTypeValue, 
		(const char*)value );
	mk_vm_push_stack( &vm->localStack, result );
	return MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP;
}

static
int mk_int_to_i( struct TAG_MK_VM_STRUCT *vm, unsigned int varArgC )
{
	mk_vm_push_stack( &vm->localStack,
		mk_vm_create_int32_frame_item( vm, 
			mk_vm_frame_item_to_int32( vm, mk_vm_pop_stack( &vm->localStack ) ) ) );
	return MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP;
}

static
int mk_int_to_f( struct TAG_MK_VM_STRUCT *vm, unsigned int varArgC )
{
	mk_vm_push_stack( &vm->localStack,
		mk_vm_create_float_frame_item( vm, 
			mk_vm_frame_item_to_int32( vm, mk_vm_pop_stack( &vm->localStack ) ) ) );
	return MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP;
}

static
int mk_int_plus( struct TAG_MK_VM_STRUCT *vm, unsigned int varArgC )
{
	int newValue = 
		mk_vm_frame_item_to_int32( vm, 
			mk_vm_pop_stack( &vm->localStack ) ) + mk_vm_frame_item_to_int32( vm, mk_vm_pop_stack( &vm->localStack ) );
	mk_vm_push_stack( &vm->localStack, 
		mk_vm_create_int32_frame_item( vm, newValue ) );
	return MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP;
}

static
int mk_int_minus( struct TAG_MK_VM_STRUCT *vm, unsigned int varArgC )
{
	int r = mk_vm_frame_item_to_int32( vm, mk_vm_pop_stack( &vm->localStack ) );
	int l = mk_vm_frame_item_to_int32( vm, mk_vm_pop_stack( &vm->localStack ) );
	int newValue = l - r;
	mk_vm_push_stack( &vm->localStack, 
		mk_vm_create_int32_frame_item( vm, newValue ) );
	return MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP;
}

static 
int mk_int_mul( struct TAG_MK_VM_STRUCT *vm, unsigned int varArgC )
{
	int newValue = 
		mk_vm_frame_item_to_int32( vm, 
			mk_vm_pop_stack( &vm->localStack ) ) * mk_vm_frame_item_to_int32( vm, mk_vm_pop_stack( &vm->localStack ) );
	mk_vm_push_stack( &vm->localStack, 
		mk_vm_create_int32_frame_item( vm, newValue ) );
	return MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP;
}

static 
int mk_int_div( struct TAG_MK_VM_STRUCT *vm, unsigned int varArgC )
{
	int r = mk_vm_frame_item_to_int32( vm, mk_vm_pop_stack( &vm->localStack ) );
	int l = mk_vm_frame_item_to_int32( vm, mk_vm_pop_stack( &vm->localStack ) );
	int newValue = ( r != 0 ) ? l / r : 0;
	mk_vm_push_stack( &vm->localStack, mk_vm_create_int32_frame_item( vm, newValue ) );
	return MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP;
}

static 
int mk_int_mod( struct TAG_MK_VM_STRUCT *vm, unsigned int varArgC )
{
	int r = mk_vm_frame_item_to_int32( vm, mk_vm_pop_stack( &vm->localStack ) );
	int l = mk_vm_frame_item_to_int32( vm, mk_vm_pop_stack( &vm->localStack ) );
	int newValue = ( r != 0 ) ? l % r : 0;
	mk_vm_push_stack( &vm->localStack, mk_vm_create_int32_frame_item( vm, newValue ) );
	return MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP;
}


static 
int mk_int_small( struct TAG_MK_VM_STRUCT *vm, unsigned int varArgC )
{
	MK_VM_FRAME_ITEM *right = mk_vm_pop_stack( &vm->localStack );
	MK_VM_FRAME_ITEM *target = mk_vm_pop_stack( &vm->localStack );

	mk_vm_push_stack( &vm->localStack, 
		mk_vm_create_bool_frame_item( 
			mk_vm_frame_item_to_int32( vm, target ) < mk_vm_frame_item_to_int32( vm, right ) ) );

	return MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP;
}


static 
int mk_int_large( struct TAG_MK_VM_STRUCT *vm, unsigned int varArgC )
{
	MK_VM_FRAME_ITEM *right = mk_vm_pop_stack( &vm->localStack );
	MK_VM_FRAME_ITEM *target = mk_vm_pop_stack( &vm->localStack );

	mk_vm_push_stack( &vm->localStack, 
		mk_vm_create_bool_frame_item( 
			mk_vm_frame_item_to_int32( vm, target ) > mk_vm_frame_item_to_int32( vm, right ) ) );

	return MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP;
}

static
int mk_int_small_same( struct TAG_MK_VM_STRUCT *vm, unsigned int varArgC )
{
	MK_VM_FRAME_ITEM *right = mk_vm_pop_stack( &vm->localStack );
	MK_VM_FRAME_ITEM *target = mk_vm_pop_stack( &vm->localStack );

	mk_vm_push_stack( &vm->localStack, 
		mk_vm_create_bool_frame_item( 
			mk_vm_frame_item_to_int32( vm, target ) <= mk_vm_frame_item_to_int32( vm, right ) ) );

	return MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP;
}


static 
int mk_int_large_same( struct TAG_MK_VM_STRUCT *vm, unsigned int varArgC )
{
	MK_VM_FRAME_ITEM *right = mk_vm_pop_stack( &vm->localStack );
	MK_VM_FRAME_ITEM *target = mk_vm_pop_stack( &vm->localStack );

	mk_vm_push_stack( &vm->localStack, 
		mk_vm_create_bool_frame_item( 
			mk_vm_frame_item_to_int32( vm, target ) >= mk_vm_frame_item_to_int32( vm, right ) ) );

	return MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP;
}

static
int mk_int_initialize( MK_VM_STRUCT *vm, int varArgC )
{
	MK_VM_FRAME_ITEM *initVlue = 
		mk_vm_pop_stack( &vm->localStack );
	MK_VM_FRAME_ITEM *target = 
		mk_vm_pop_stack( &vm->localStack );
	int i = mk_vm_frame_item_to_int32( vm, initVlue );
	if( MK_TYPE_ATTRIBUTE(target->flags) != MK_VM_FRAME_ITEM_TYPE_INT_VALUE )
		target = 
			mk_vm_find_instance( vm, 
				target, 
				vm->cache->internalClassSymbolName[MK_VM_FRAME_ITEM_TYPE_INT_VALUE>>24] );
	target->int32TypeValue = i;
	mk_vm_push_stack( &vm->localStack, target );
	return MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP;
}

MK_CLASS *mk_create_int_class( MK_VM_STRUCT *vm )
{
	MK_CLASS *result =
		mk_create_object( MK_TYPE_CLASS );
	
	result->nameThis = mk_get_symbol_name_ptr( vm, className );
	result->nameSuper = mk_get_symbol_name_ptr( vm, classSuper );

	// create initialize method.
	{
		MK_VARIABLE *pMethod = NULL;

		MK_VECTOR *parameters =
			mk_create_vector( 
				MK_TYPE_VECTOR | 
					MK_TYPE_VECTOR_ARRAY_MANAGED_PTR | 
					( MK_SIZEOF_EXTEND_VECTOR_DEFAULT << 10 ),
				1,
				(INT_PTR)NULL );

		// param1:size
		MK_VARIABLE *param = 
			( MK_VARIABLE * )mk_create_object( MK_TYPE_VARIABLE );

		param->name = 
			mk_get_symbol_name_ptr( vm, "init" );
		param->defaultValue = 
			mk_create_object( MK_TYPE_NODE_EXPR | MK_TYPE_NODE_EXPR_INT32 );
		mk_push_vector( parameters, (INT_PTR)param );

		pMethod = 
			mk_create_method(
				vm, 
				"initialize",
				MK_TYPE_VARIABLE | 
					MK_TYPE_ATTRIBUTE_VARIABLE_METHOD | 
					MK_TYPE_ATTRIBUTE_VARIABLE_METHOD_NATIVE |
					MK_TYPE_ATTRIBUTE_VARIABLE_NEW_NATIVE_METHOD,
				parameters,
				(INT_PTR)mk_int_initialize );

		mk_register_variable(
			vm,
			pMethod,
			result );
	}

	mk_register_variable(
		vm,
		mk_create_default_native_method(
			vm, 
			"to_s",
			MK_TYPE_VARIABLE | 
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD | 
				MK_TYPE_ATTRIBUTE_VARIABLE_STATIC |
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD_NATIVE |
				MK_TYPE_ATTRIBUTE_VARIABLE_NEW_NATIVE_METHOD,
			0,
			(INT_PTR)mk_int_to_s ),
		result );

	mk_register_variable(
		vm,
		mk_create_default_native_method(
			vm,
			"to_i",
			MK_TYPE_VARIABLE | 
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD | 
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD_NATIVE |
				MK_TYPE_ATTRIBUTE_VARIABLE_NEW_NATIVE_METHOD,
			0,
			(INT_PTR)mk_int_to_i ),
		result );

	mk_register_variable(
		vm,
		mk_create_default_native_method(
			vm,
			"to_f",
			MK_TYPE_VARIABLE | 
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD | 
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD_NATIVE |
				MK_TYPE_ATTRIBUTE_VARIABLE_NEW_NATIVE_METHOD,
			0,
			(INT_PTR)mk_int_to_f ),
		result );

	mk_register_variable( 
		vm,
		mk_create_default_native_method( 
			vm, 
			"+",
			MK_TYPE_VARIABLE | 
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD | 
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD_NATIVE |
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD_OPERATOR |
				MK_LEX_TYPE_RESERVED_MARK_PLUS |
				MK_TYPE_ATTRIBUTE_VARIABLE_NEW_NATIVE_METHOD, 
			1, 
			(INT_PTR)mk_int_plus ),
		result);

	mk_register_variable( 
		vm,
		mk_create_default_native_method( 
			vm, 
			"-", 
			MK_TYPE_VARIABLE | 
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD | 
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD_NATIVE |
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD_OPERATOR |
				MK_LEX_TYPE_RESERVED_MARK_MINUS |
				MK_TYPE_ATTRIBUTE_VARIABLE_NEW_NATIVE_METHOD, 
			1, 
			(INT_PTR)mk_int_minus ),
		result );

	mk_register_variable( 
		vm,
		mk_create_default_native_method( 
			vm, 
			"*", 
			MK_TYPE_VARIABLE | 
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD | 
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD_NATIVE |
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD_OPERATOR |
				MK_LEX_TYPE_RESERVED_MARK_MUL |
				MK_TYPE_ATTRIBUTE_VARIABLE_NEW_NATIVE_METHOD, 
			1, 
			(INT_PTR)mk_int_mul ),
		result );

	mk_register_variable( 
		vm,
		mk_create_default_native_method( 
			vm, 
			"/", 
			MK_TYPE_VARIABLE | 
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD | 
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD_NATIVE |
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD_OPERATOR |
				MK_LEX_TYPE_RESERVED_MARK_DIV |
				MK_TYPE_ATTRIBUTE_VARIABLE_NEW_NATIVE_METHOD, 
			1, 
			(INT_PTR)mk_int_div ),
		result );

	mk_register_variable( 
		vm,
		mk_create_default_native_method( 
			vm, 
			"%", 
			MK_TYPE_VARIABLE | 
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD | 
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD_NATIVE |
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD_OPERATOR |
				MK_LEX_TYPE_RESERVED_MARK_MOD |
				MK_TYPE_ATTRIBUTE_VARIABLE_NEW_NATIVE_METHOD, 
			1, 
			(INT_PTR)mk_int_mod ),
		result );

	mk_register_variable( 
		vm,
		mk_create_default_native_method(
			vm, 
			"<",
			MK_TYPE_VARIABLE | 
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD | 
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD_NATIVE |
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD_OPERATOR |
				MK_LEX_TYPE_RESERVED_MARK_SMALL |
				MK_TYPE_ATTRIBUTE_VARIABLE_NEW_NATIVE_METHOD, 
			1,
			(INT_PTR)mk_int_small ),
		result );

	mk_register_variable( 
		vm,
		mk_create_default_native_method(
			vm, 
			">",
			MK_TYPE_VARIABLE | 
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD | 
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD_NATIVE |
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD_OPERATOR |
				MK_LEX_TYPE_RESERVED_MARK_BIG |
				MK_TYPE_ATTRIBUTE_VARIABLE_NEW_NATIVE_METHOD, 
			1,
			(INT_PTR)mk_int_large ),
		result );

	mk_register_variable( 
		vm,
		mk_create_default_native_method(
			vm, 
			"<=",
			MK_TYPE_VARIABLE | 
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD | 
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD_NATIVE |
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD_OPERATOR |
				MK_LEX_TYPE_RESERVED_MARK_SE |
				MK_TYPE_ATTRIBUTE_VARIABLE_NEW_NATIVE_METHOD, 
			1,
			(INT_PTR)mk_int_small_same ),
		result );

	mk_register_variable( 
		vm,
		mk_create_default_native_method(
			vm, 
			">=",
			MK_TYPE_VARIABLE | 
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD | 
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD_NATIVE |
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD_OPERATOR |
				MK_LEX_TYPE_RESERVED_MARK_BE |
				MK_TYPE_ATTRIBUTE_VARIABLE_NEW_NATIVE_METHOD, 
			1,
			(INT_PTR)mk_int_large_same ),
		result );

	return result;
}
