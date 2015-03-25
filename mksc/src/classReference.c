#include "mk.h"
#include <string.h>

static const MK_CHAR className[] = CLASS_INTERNAL_REFERENCE;
static const MK_CHAR classSuper[] = CLASS_INTERNAL_OBJECT;

MK_VM_FRAME_ITEM *mk_reference_to_value( MK_VM_STRUCT *vm, MK_VM_FRAME_ITEM *source )
{
	if( (INT_PTR)source & MK_VM_FRAME_ITEM_TYPE_DIRECT_VALUE )
	{
		return NULL;
	}
	else if( (INT_PTR)source & MK_VM_FRAME_ITEM_TYPE_DIRECT_POINTER )
	{
		return *(MK_VM_FRAME_ITEM**)( (INT_PTR)source & ~MK_VM_FRAME_ITEM_TYPE_DIRECT_POINTER );
	}
	else
	{
		if( MK_TYPE_ATTRIBUTE(source->flags) == MK_VM_FRAME_ITEM_TYPE_REFERENCE_VALUE )
		{
			return
				mk_vm_find_instance( vm, 
					source, 
					vm->cache->internalClassSymbolName[MK_VM_FRAME_ITEM_TYPE_REFERENCE_VALUE>>24] );
		}
		else if( MK_TYPE_ATTRIBUTE(source->flags) == MK_VM_FRAME_ITEM_TYPE_REFERENCE_ARRAY_VALUE )
		{
			return
				(MK_VM_FRAME_ITEM*)mk_get_at_vector( 
					source->referenceArrayTypeValue.target, 
					source->referenceArrayTypeValue.index );
		}
		else if( MK_TYPE_ATTRIBUTE(source->flags) == MK_VM_FRAME_ITEM_TYPE_REFERENCE_STRING_VALUE )
		{
			MK_VM_FRAME_ITEM *targetObject =
				source->referenceStringTypeValue.target;
			int index = 
				source->referenceStringTypeValue.index;
			int length = 
				strlen( targetObject->stringTypeValue );
			if( index >= 0 && index < length )
				return mk_vm_create_int32_frame_item( vm, targetObject->stringTypeValue[index] );
			else
				;	// todo: outof range.
		}
		return NULL;
	}
}

int mk_reference_equal( struct TAG_MK_VM_STRUCT *vm, unsigned int varArgC )
{
	void *handle = NULL;
	MK_VM_FRAME_ITEM *right = 
		mk_vm_pop_stack( &vm->localStack );
	MK_VM_FRAME_ITEM *target = 
		mk_vm_pop_stack( &vm->localStack );
	MK_VM_FRAME_ITEM *result = NULL;
	if( MK_VM_FRAME_ITEM_TYPE_GROUP(target->flags) != MK_VM_FRAME_ITEM_TYPE_REFERENCE_VALUE )
		target = 
			mk_vm_find_instance( vm, 
				target, 
				vm->cache->internalClassSymbolName[MK_VM_FRAME_ITEM_TYPE_REFERENCE_VALUE>>24] );
	if( target == NULL )
	{
		return MK_VM_EXECUTE_EXPR_THROW;
	}
	else
	{
		if( target->flags & MK_VM_FRAME_ITEM_TYPE_FINAL )
			return mk_raise_internal_error( vm, "", 0, MK_ERROR_TYPE_VM_ERROR | 5, "", NULL );

		if( MK_TYPE_ATTRIBUTE(target->flags) == MK_VM_FRAME_ITEM_TYPE_REFERENCE_VALUE )
		{
			mk_insert_item_hashtable(
				target->referenceTypeValue.target, 
				target->referenceTypeValue.symbolName, right );
		}
		else if( MK_TYPE_ATTRIBUTE(target->flags) == MK_VM_FRAME_ITEM_TYPE_REFERENCE_ARRAY_VALUE )
		{
			mk_set_at_vector( 
				target->referenceArrayTypeValue.target, 
				target->referenceArrayTypeValue.index, 
				(INT_PTR)right );
		}
		else if( MK_TYPE_ATTRIBUTE(target->flags) == MK_VM_FRAME_ITEM_TYPE_REFERENCE_STRING_VALUE )
		{
			MK_VM_FRAME_ITEM *targetObject =
				target->referenceStringTypeValue.target;
			int newValue = 
				mk_vm_frame_item_to_int32( vm, right );
			if( ( target->referenceStringTypeValue.index >= 0 ) && 
				( target->referenceStringTypeValue.index < strlen( targetObject->stringTypeValue ) ) )
				targetObject->stringTypeValue[target->referenceStringTypeValue.index] = newValue;
			else
				;	// todo: outof range.
		}
		mk_vm_push_stack( &vm->localStack, target );
		return MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP;
	}
}

MK_CLASS *mk_create_referecne_class( MK_VM_STRUCT *vm )
{
	MK_CLASS *result =
		mk_create_object( MK_TYPE_CLASS );
	MK_VARIABLE *insertMethod = NULL;
	result->nameThis = mk_get_symbol_name_ptr( vm, className );
	result->nameSuper = mk_get_symbol_name_ptr( vm, classSuper );

	mk_register_variable(
		vm,
		mk_create_default_native_method(
			vm,
			"=",
			MK_TYPE_VARIABLE | 
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD | 
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD_NATIVE |
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD_OPERATOR |
				MK_LEX_TYPE_RESERVED_MARK_EQUAL |
				MK_TYPE_ATTRIBUTE_VARIABLE_NEW_NATIVE_METHOD, 
			1,
			(INT_PTR)mk_reference_equal ),
		result );

	return result;
}
