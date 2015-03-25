#include "mk.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static const MK_CHAR className[] = CLASS_INTERNAL_ARRAY;
static const MK_CHAR classSuper[] = CLASS_INTERNAL_CONTAINER;

static
int mk_array_size( struct TAG_MK_VM_STRUCT *vm, unsigned int varArgC )
{
	MK_VM_FRAME_ITEM *target = 
		mk_vm_pop_stack( &vm->localStack );
	MK_VECTOR *targetArray = NULL;
	if( MK_TYPE_ATTRIBUTE(target->flags) != MK_VM_FRAME_ITEM_TYPE_ARRAY_VALUE )
		target = 
			mk_vm_find_instance( vm, 
				target, 
				vm->cache->internalClassSymbolName[MK_VM_FRAME_ITEM_TYPE_ARRAY_VALUE>>24] );
	targetArray =
		target->arrayTypeValue;
	mk_vm_push_stack( 
		&vm->localStack, 
		mk_vm_create_int32_frame_item( vm, mk_size_vector( targetArray ) ) );
	return MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP;
}

static
int mk_array_grow( struct TAG_MK_VM_STRUCT *vm, unsigned int varArgC )
{
	return MK_VM_EXECUTE_EXPR_THROW;	// not implements.
}

static
int mk_array_each( struct TAG_MK_VM_STRUCT *vm, unsigned int varArgC )
{
	MK_VM_FRAME_ITEM *method = 
		mk_vm_pop_stack( &vm->localStack );
	MK_VM_FRAME_ITEM *target = 
		mk_vm_pop_stack( &vm->localStack );
	int size = 0;
	int index = 0;

	if( MK_TYPE_ATTRIBUTE(target->flags) != MK_VM_FRAME_ITEM_TYPE_ARRAY_VALUE )
		target = 
			mk_vm_find_instance( vm, 
				target, 
				vm->cache->internalClassSymbolName[MK_VM_FRAME_ITEM_TYPE_ARRAY_VALUE>>24] );
	size =
		mk_size_vector( target->arrayTypeValue );
	for( index = 0; index < size; index ++ )
	{
		MK_VM_FRAME_ITEM *current = 
			(MK_VM_FRAME_ITEM *)mk_get_at_vector( target->arrayTypeValue, index );
		MK_VM_FRAME_ITEM *result = NULL;
		mk_vm_push_stack( &vm->localStack, method );
		mk_vm_push_stack( &vm->localStack, current );
		if( mk_node_invoke( vm, 1 ) != MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP )
			break;
		result = mk_vm_pop_stack( &vm->localStack );
	}
	mk_vm_push_stack( &vm->localStack, mk_create_vm_frame_item_object( &vm->pFrameItemTable ) );
	if( index == size )
		return MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP;
	else
		return MK_VM_EXECUTE_EXPR_THROW;
}

static
int mk_array_operator_bracket( struct TAG_MK_VM_STRUCT *vm, unsigned int varArgC )
{
	int retCode = MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP;
	MK_VM_FRAME_ITEM *right = 
		mk_vm_pop_stack( &vm->localStack );
	MK_VM_FRAME_ITEM *target = 
		mk_vm_pop_stack( &vm->localStack );
	MK_VECTOR *targetArray = NULL;
	int index = 
		mk_vm_frame_item_to_int32( vm, right );

	if( MK_TYPE_ATTRIBUTE(target->flags) != MK_VM_FRAME_ITEM_TYPE_ARRAY_VALUE )
		target = 
			mk_vm_find_instance( vm, 
				target, 
				vm->cache->internalClassSymbolName[MK_VM_FRAME_ITEM_TYPE_ARRAY_VALUE>>24] );
	targetArray =
		target->arrayTypeValue;

	if( targetArray != NULL && 
		index >= 0 &&
		index < mk_size_vector( targetArray ) )
	{
		MK_VM_FRAME_ITEM *result = 
			(MK_VM_FRAME_ITEM*)mk_get_at_vector( targetArray, index );
		mk_vm_push_stack( &vm->localStack, result );
	}
	else
	{
		MK_VM_FRAME_ITEM *execptionClass = NULL;
		MK_VM_FRAME_ITEM *exception = NULL;

		mk_find_item_hashtable( vm->global, 
			mk_get_symbol_name_ptr( vm, CLASS_INTERNAL_ARRAY_OUTOF_RANGE ), 
			(void**)&execptionClass );
		exception = 
			mk_create_internal_error_object( 
				vm, 
				"", 
				0,
				execptionClass,
				0,
				NULL );
		vm->exceptionObject = exception;
		retCode = MK_VM_EXECUTE_EXPR_THROW;
	}
	return retCode;
}

static
int mk_array_operator_bracket_ref( struct TAG_MK_VM_STRUCT *vm, unsigned int varArgC )
{
	int retCode = MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP;
	MK_VM_FRAME_ITEM *right = 
		mk_vm_pop_stack( &vm->localStack );
	MK_VM_FRAME_ITEM *target = 
		mk_vm_pop_stack( &vm->localStack );
	MK_VECTOR *targetArray = NULL;
	int index = 
		mk_vm_frame_item_to_int32( vm, right );

	if( MK_TYPE_ATTRIBUTE(target->flags) != MK_VM_FRAME_ITEM_TYPE_ARRAY_VALUE )
		target = 
			mk_vm_find_instance( vm, 
				target, 
				vm->cache->internalClassSymbolName[MK_VM_FRAME_ITEM_TYPE_ARRAY_VALUE>>24] );
	targetArray =
		target->arrayTypeValue;

	if( targetArray != NULL && 
		index >= 0 &&
		index < mk_size_vector( targetArray ) )
	{
		MK_VM_FRAME_ITEM *result = NULL;
		result = mk_create_vm_frame_item_object( &vm->pFrameItemTable );
		result->flags |= MK_VM_FRAME_ITEM_TYPE_REFERENCE_ARRAY_VALUE;
		result->referenceArrayTypeValue.target = targetArray;
		result->referenceArrayTypeValue.index = index;
		mk_vm_push_stack( &vm->localStack, result );
	}
	else
	{
		MK_VM_FRAME_ITEM *execptionClass = NULL;
		MK_VM_FRAME_ITEM *exception = NULL;

		mk_find_item_hashtable( vm->global, 
			mk_get_symbol_name_ptr( vm, CLASS_INTERNAL_ARRAY_OUTOF_RANGE ), 
			(void**)&execptionClass );
		exception = 
			mk_create_internal_error_object( 
				vm, 
				"", 
				0,
				execptionClass,
				0,
				NULL );
		vm->exceptionObject = exception;
		retCode = MK_VM_EXECUTE_EXPR_THROW;
	}
	return retCode;
}

static
int mk_array_initialize( MK_VM_STRUCT *vm, int varArgC )
{
	MK_VM_FRAME_ITEM *sz = 
		mk_vm_pop_stack( &vm->localStack );
	MK_VM_FRAME_ITEM *target = 
		mk_vm_pop_stack( &vm->localStack );
	int size = mk_vm_frame_item_to_int32( vm, sz );
	MK_VECTOR *newVector = 
		mk_allocate_vm_managed_vector( &vm->pVectorTable,
			MK_TYPE_VECTOR | MK_TYPE_VECTOR_ARRAY_MANAGED_PTR | ( MK_SIZEOF_EXTEND_VECTOR_DEFAULT << 10 ),
			size,
			(INT_PTR)NULL );
	int index = 0;
	if( MK_TYPE_ATTRIBUTE(target->flags) != MK_VM_FRAME_ITEM_TYPE_ARRAY_VALUE )
		target = 
			mk_vm_find_instance( vm, 
				target, 
				vm->cache->internalClassSymbolName[MK_VM_FRAME_ITEM_TYPE_ARRAY_VALUE>>24] );

	for( index = 0; index < size; index ++ )
		mk_push_vector( newVector, (INT_PTR)MK_VM_FRAME_ITEM_TYPE_DIRECT_VALUE );
	target->flags |= MK_VM_FRAME_ITEM_TYPE_ARRAY_VALUE;
	target->arrayTypeValue = newVector;
	mk_vm_push_stack( &vm->localStack, target );
	return MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP;
}

MK_CLASS *mk_create_array_class( MK_VM_STRUCT *vm )
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
			mk_get_symbol_name_ptr( vm, "size" );
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
				(INT_PTR)mk_array_initialize );

		mk_register_variable(
			vm,
			pMethod,
			result );
	}

	mk_register_variable(
		vm,
		mk_create_default_native_method(
			vm, 
			"size",
			MK_TYPE_VARIABLE | 
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD | 
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD_NATIVE |
				MK_TYPE_ATTRIBUTE_VARIABLE_NEW_NATIVE_METHOD,
			0,
			(INT_PTR)mk_array_size ),
		result );

	mk_register_variable(
		vm,
		mk_create_default_native_method(
			vm,
			"grow",
			MK_TYPE_VARIABLE | 
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD | 
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD_NATIVE |
				MK_TYPE_ATTRIBUTE_VARIABLE_NEW_NATIVE_METHOD,
			1,
			(INT_PTR)mk_array_grow ),
		result );
	
	mk_register_variable(
		vm,
		mk_create_default_native_method(
			vm,
			"[]",
			MK_TYPE_VARIABLE | 
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD | 
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD_NATIVE | 
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD_OPERATOR |
				MK_LEX_TYPE_RESERVED_MARK_BRACKET |
				MK_TYPE_ATTRIBUTE_VARIABLE_NEW_NATIVE_METHOD,
			1,
			(INT_PTR)mk_array_operator_bracket ),
		result );

	mk_register_variable(
		vm,
		mk_create_default_native_method(
			vm,
			"ref[]",
			MK_TYPE_VARIABLE | 
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD | 
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD_NATIVE | 
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD_OPERATOR |
				MK_LEX_TYPE_RESERVED_MARK_BRACKET_REF |
				MK_TYPE_ATTRIBUTE_VARIABLE_NEW_NATIVE_METHOD,
			1,
			(INT_PTR)mk_array_operator_bracket_ref ),
		result );

	mk_register_variable(
		vm,
		mk_create_default_native_method(
			vm,
			"each",
			MK_TYPE_VARIABLE | 
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD | 
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD_NATIVE |
				MK_TYPE_ATTRIBUTE_VARIABLE_NEW_NATIVE_METHOD,
			1,
			(INT_PTR)mk_array_each ),
		result );

	return result;
}
