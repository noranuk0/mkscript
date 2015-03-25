#include "mk.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>

static const MK_CHAR className[] = CLASS_INTERNAL_OBJECT;
static const MK_CHAR classSuper[] = "";

static
int mk_object_class( struct TAG_MK_VM_STRUCT *vm, unsigned int varArgC )
{
	MK_VM_FRAME_ITEM *target = 
		mk_vm_pop_stack( &vm->localStack );
	MK_CLASS *targetClass = 
		mk_vm_get_class(vm, target );
	if( targetClass == NULL )
	{
		if( MK_TYPE_ATTRIBUTE( target->flags) == MK_VM_FRAME_ITEM_TYPE_CLASS )
			return mk_raise_internal_error( vm, "", 0, 
										MK_ERROR_TYPE_VM_ERROR | 2, 
										target->classTypeValue.typeName, NULL );
		else
			return mk_raise_internal_error( vm, "", 0, 
										MK_ERROR_TYPE_VM_ERROR | 1, 
										NULL );
	}
	else
	{
		MK_VM_FRAME_ITEM *result = NULL;
		if( mk_find_item_hashtable( vm->global, targetClass->nameThis, (void**)&result ) == 0 )
		{
			return mk_raise_internal_error( vm, "", 0, 
										MK_ERROR_TYPE_VM_ERROR | 1, 
										NULL );
		}
		else
		{
			mk_vm_push_stack( &vm->localStack, result );
			return MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP;
		}
	}
}

static
int mk_object_to_s( struct TAG_MK_VM_STRUCT *vm, unsigned int varArgC )
{
	MK_VM_FRAME_ITEM *target = mk_vm_pop_stack( &vm->localStack );
	unsigned int length = 0;
	MK_SYM_CHAR *className = NULL;
	MK_CLASS *pClass = NULL;
	int isStatic = 0;
	MK_VM_FRAME_ITEM *result = 
		mk_create_vm_frame_item_object( &vm->pFrameItemTable );

	result->flags = 
		MK_TYPE_SET_ATTRIBUTE( result->flags, MK_VM_FRAME_ITEM_TYPE_STRING_VALUE );
	
	if( ( (INT_PTR)target & ( MK_VM_FRAME_ITEM_TYPE_DIRECT_VALUE | MK_VM_FRAME_ITEM_TYPE_DIRECT_POINTER ) ) == 0 )
	{
		if( ( target->flags & MK_VM_FRAME_ITEM_TYPE_STATIC_INSTANCE ) != 0 )
			isStatic = 1;
	}
	pClass = mk_vm_get_class( vm, target );
	if( pClass == NULL )
	{
		if( MK_TYPE_ATTRIBUTE( target->flags) == MK_VM_FRAME_ITEM_TYPE_CLASS ||
			MK_TYPE_ATTRIBUTE(target->flags) == MK_VM_FRAME_ITEM_TYPE_MODULE )
			return mk_raise_internal_error( vm, "", 0, 
										MK_ERROR_TYPE_VM_ERROR | 2, 
										target->classTypeValue.typeName, NULL );
		else
			return mk_raise_internal_error( vm, "", 0, 
										MK_ERROR_TYPE_VM_ERROR | 1, 
										NULL );
	}
	else
	{
		className = pClass->nameThis;
		// #<className:[class|instance]:0xaddress>"
		length = sizeof(MK_CHAR) * 
			( 2 /* #< */ + 
				strlen( className ) /* className */ +
				1 /* : */ +
				8 /* instance */ +
				1 /* : */ +
				2 /* 0x */ + 
				sizeof( void* ) * 2 /* addresss */ + 
				1 /* > */ +
				1 /* \0 */ );
		result->stringTypeValue = malloc( length );
		sprintf( result->stringTypeValue, "#<%s:%s:0x%x>", className, isStatic != 0 ? "class" : "instance", target );
		mk_vm_push_stack( &vm->localStack, result );
		return MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP;
	}
}

static
MK_VM_FRAME_ITEM *mk_object_value_new( MK_VM_STRUCT *vm, MK_SYM_CHAR *symbolName )
{
	int index = 0;
	MK_SYM_CHAR **sym = vm->cache->internalClassSymbolName;
	MK_VM_FRAME_ITEM *result = NULL;
	MK_VM_FRAME_ITEM *initValue = NULL;

	for( index = 0; 
		index < sizeof( vm->cache->internalClassSymbolName ) / sizeof( MK_SYM_CHAR* ); 
		index ++ )
	{
		if( sym[index] == symbolName )
			break;
	}
	if( index < sizeof( vm->cache->internalClassSymbolName ) / sizeof( MK_SYM_CHAR* ) )
	{
		switch( index << 24 )
		{
		case MK_VM_FRAME_ITEM_TYPE_NIL:
			result = 
				mk_create_vm_frame_item_object( &vm->pFrameItemTable );
			break;

		case MK_VM_FRAME_ITEM_TYPE_CLASS:
		case MK_VM_FRAME_ITEM_TYPE_MODULE:
			break;

		case MK_VM_FRAME_ITEM_TYPE_INT_VALUE:
			result = 
				mk_create_vm_frame_item_object( &vm->pFrameItemTable );
			result->flags |= MK_VM_FRAME_ITEM_TYPE_INT_VALUE;
			break;

		case MK_VM_FRAME_ITEM_TYPE_FLOAT_VALUE:
			result = 
				mk_create_vm_frame_item_object( &vm->pFrameItemTable );
			result->flags |= MK_VM_FRAME_ITEM_TYPE_FLOAT_VALUE;
			break;

		case MK_VM_FRAME_ITEM_TYPE_ARRAY_VALUE:
			result = 
				mk_create_vm_frame_item_object( &vm->pFrameItemTable );
			result->flags |= MK_VM_FRAME_ITEM_TYPE_ARRAY_VALUE;
			break;

		case MK_VM_FRAME_ITEM_TYPE_STRING_VALUE:
			result = 
				mk_create_vm_frame_item_object( &vm->pFrameItemTable );
			result->flags |= MK_VM_FRAME_ITEM_TYPE_STRING_VALUE;
			break;

		case MK_VM_FRAME_ITEM_TYPE_NODE_VALUE:
			result = 
				mk_create_vm_frame_item_object( &vm->pFrameItemTable );
			result->flags |= MK_VM_FRAME_ITEM_TYPE_NODE_VALUE;
			break;
		}
	}
	return result;
}

int mk_object_new( MK_VM_STRUCT *vm, MK_VM_FRAME_ITEM *target, MK_VM_FRAME_ITEM **result )
{
	int retCode = MK_VM_EXECUTE_EXPR_RETURN_RETURN;
	MK_VM_FRAME *pInitializeFrame = NULL;
	MK_HASHTABLE *classes = 
		vm->code->classes;
	MK_CLASS *thisClass = NULL, *currentClass = NULL;
	MK_VM_FRAME_ITEM *currentFrameItem = NULL, *previousFrameItem = NULL;
	MK_SYM_CHAR *nameSuper = NULL;
	MK_SYM_CHAR *nameClassObject = NULL;

	if( target == NULL )
		return MK_VM_EXECUTE_EXPR_THROW;
	else if( ( (INT_PTR)target & ( MK_VM_FRAME_ITEM_TYPE_DIRECT_POINTER | MK_VM_FRAME_ITEM_TYPE_DIRECT_VALUE ) ) != 0 )
		return MK_VM_EXECUTE_EXPR_THROW;
	else if( !( target->flags & MK_VM_FRAME_ITEM_TYPE_STATIC_INSTANCE ) )
		return MK_VM_EXECUTE_EXPR_THROW;

	*result = NULL;
	thisClass = mk_vm_get_class( vm, target );

	if( ( thisClass == NULL ) ||
		( MK_TYPE_ATTRIBUTE( thisClass->flags ) & MK_TYPE_ATTRIBUTE_CLASS_MODULE ) )
	{
		if( !( (INT_PTR)target & MK_VM_FRAME_ITEM_TYPE_DIRECT_VALUE ) &&
			( MK_TYPE_ATTRIBUTE( target->flags) == MK_VM_FRAME_ITEM_TYPE_CLASS ||
			( MK_TYPE_ATTRIBUTE( target->flags) == MK_VM_FRAME_ITEM_TYPE_MODULE ) ) )
			return mk_raise_internal_error( vm, "", 0, 
										MK_ERROR_TYPE_VM_ERROR | 2, 
										target->classTypeValue.typeName, NULL );
		else
			return mk_raise_internal_error( vm, "", 0, 
										MK_ERROR_TYPE_VM_ERROR | 1, 
										NULL );
	}


	currentClass = 
		thisClass;

	// setup variables
	while(currentClass)
	{
		const MK_CHAR *key = NULL;
		MK_VARIABLE* value = NULL;
		MK_VM_FRAME_ITEM *insertValue = NULL;
		void *iterator = NULL;
		int isValueClass = 0;
		currentFrameItem = mk_object_value_new( vm, currentClass->nameThis );
		if( currentFrameItem == NULL )
		{
			currentFrameItem = 
				mk_create_vm_frame_item_object( &vm->pFrameItemTable );
			currentFrameItem->flags |= MK_VM_FRAME_ITEM_TYPE_CLASS;
			currentFrameItem->classTypeValue.typeName = mk_get_symbol_name_ptr( vm, currentClass->nameThis );
			currentFrameItem->classTypeValue.variables = 
				mk_allocate_vm_managed_hashtable( 
					MK_TYPE_ATTRIBUTE_HASH_INTERPRETER_DEFAULT, 
					MK_SIZEOF_HASH_DEFAULT, 
					&vm->pHashTable );
		}
		else
		{
			isValueClass = 1;
		}
		if( previousFrameItem != NULL )
		{
			void *handleSuper = NULL;
			if( nameSuper == NULL )
				nameSuper = mk_get_symbol_name_ptr( vm, "super" );
			handleSuper = mk_insert_item_hashtable( 
				previousFrameItem->classTypeValue.variables, 
				nameSuper, 
				currentFrameItem );
			mk_set_extend_value_hashtable( 
				previousFrameItem->classTypeValue.variables, 
				handleSuper, 
				1,
				MK_VM_FRAME_ITEM_EXTEND_TYPE_FINAL );
		}
		else
		{
			*result = currentFrameItem;
		}
		if( isValueClass )
		{
			break;
		}
		else
		{
			currentFrameItem->classTypeValue.child = previousFrameItem;
			if( nameClassObject == NULL )
				nameClassObject = mk_get_symbol_name_ptr( vm, CLASS_INTERNAL_OBJECT );
			if( currentClass->nameThis !=  nameClassObject )
			{
				if( nameSuper == NULL )
					nameSuper = mk_get_symbol_name_ptr( vm, "super" );
				mk_insert_item_hashtable( 
					currentFrameItem->classTypeValue.variables, 
					nameSuper, 
					NULL );
			}
			if( mk_vm_initialize_object_variables( vm, currentClass, currentFrameItem, 0 ) != MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP )
				return MK_VM_EXECUTE_EXPR_THROW;
			previousFrameItem = currentFrameItem;
			mk_find_item_hashtable( classes, currentClass->nameSuper, (void**)&currentClass );
		}
	}
	return retCode;
}

static 
int mk_object_equal_equal( struct TAG_MK_VM_STRUCT *vm, unsigned int varArgC )
{
	MK_VM_FRAME_ITEM *right = 
		mk_vm_pop_stack( &vm->localStack );
	MK_VM_FRAME_ITEM *target = 
		mk_vm_pop_stack( &vm->localStack );

	int isTrue = 0;
	int retCode = MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP;

	if( ( (INT_PTR)target & MK_VM_FRAME_ITEM_TYPE_DIRECT_VALUE ) == 0 &&
		( (INT_PTR)right & MK_VM_FRAME_ITEM_TYPE_DIRECT_VALUE ) != 0 )
	{
		MK_VM_FRAME_ITEM *swap = target;
		target = right;
		right = swap;
	}
	if( target == NULL || right == NULL )
	{
		if( target == NULL && right == NULL )
			isTrue = 1;
	}
	else if( (INT_PTR)target & MK_VM_FRAME_ITEM_TYPE_DIRECT_VALUE )
	{
		if( (INT_PTR)right & MK_VM_FRAME_ITEM_TYPE_DIRECT_VALUE )
		{
			isTrue = ( target == right );
		}
		else
		{
			switch( MK_TYPE_ATTRIBUTE(right->flags ) )
			{
			case MK_VM_FRAME_ITEM_TYPE_INT_VALUE:
				isTrue = 
					mk_vm_frame_item_to_int32( vm, target) == mk_vm_frame_item_to_int32( vm, right );
				break;

			case MK_VM_FRAME_ITEM_TYPE_FLOAT_VALUE:
				isTrue = 
					mk_vm_frame_item_to_float( vm, target ) == mk_vm_frame_item_to_float( vm, right);
				break;
			}
		}
	}
	else if( MK_TYPE_ATTRIBUTE( target->flags ) == MK_TYPE_ATTRIBUTE( right->flags ) )
	{
		void *position = NULL;
		const MK_CHAR *key = NULL;
		MK_VM_FRAME_ITEM *value = NULL, *rightValue = NULL;
		switch( MK_TYPE_ATTRIBUTE(target->flags) )
		{
		case MK_VM_FRAME_ITEM_TYPE_CLASS:
			// get top level object.
			while( target->classTypeValue.child != NULL )
				target = target->classTypeValue.child;
			while( right->classTypeValue.child != NULL )
				right = right->classTypeValue.child;
			if( memcmp( &target->classTypeValue, 
						&right->classTypeValue, 
						sizeof( target->classTypeValue ) ) == 0 )
				isTrue = 1;
			break;

		case MK_VM_FRAME_ITEM_TYPE_NIL:
			isTrue = 1;
			break;

		case MK_VM_FRAME_ITEM_TYPE_INT_VALUE:
			isTrue =
				mk_vm_frame_item_to_int32( vm, target ) == mk_vm_frame_item_to_int32( vm, right );
			break;

		case MK_VM_FRAME_ITEM_TYPE_FLOAT_VALUE:
			isTrue =
				mk_vm_frame_item_to_float( vm, target ) == mk_vm_frame_item_to_float( vm, right );
			break;

		case MK_VM_FRAME_ITEM_TYPE_STRING_VALUE:
			if( strcmp( target->stringTypeValue, right->stringTypeValue ) == 0 )
				isTrue = 1;
			break;
		}
		while( 0 );
	}
	mk_vm_push_stack( &vm->localStack, mk_vm_create_bool_frame_item( isTrue ) );

	return retCode;
}

static
int mk_object_equal( struct TAG_MK_VM_STRUCT *vm, unsigned int varArgC )
{
	MK_VM_FRAME_ITEM *right = 
		mk_vm_pop_stack( &vm->localStack );
	MK_VM_FRAME_ITEM *target = 
		mk_vm_pop_stack( &vm->localStack );

	int retCode = MK_VM_EXECUTE_EXPR_RETURN_RETURN;
	if( target->flags & MK_VM_FRAME_ITEM_EXTEND_TYPE_FINAL )
	{
		return mk_raise_internal_error( vm, "", 0, 
										MK_ERROR_TYPE_VM_ERROR | 10, 
										NULL );	// final object
	}

	if( MK_TYPE_ATTRIBUTE(target->flags) == MK_VM_FRAME_ITEM_TYPE_STRING_VALUE )
		free( target->stringTypeValue );

	if( (INT_PTR)right & MK_VM_FRAME_ITEM_TYPE_DIRECT_VALUE )
	{
		INT_PTR directValue = (INT_PTR)right;
		INT_PTR value = ( directValue & MK_VM_FRAME_ITEM_DIRECT_INT_VALUE_MASK ) >> 1;
		if( directValue & MK_VM_FRAME_ITEM_TYPE_INT_MINUS_VALUE )
			value *= -1;
		target->flags = 
			MK_TYPE_SET_ATTRIBUTE( target->flags, MK_VM_FRAME_ITEM_TYPE_INT_VALUE );
		target->int32TypeValue = value;
	}
	else
	{
		switch( MK_TYPE_ATTRIBUTE( right->flags ) )
		{
		case MK_VM_FRAME_ITEM_TYPE_MODULE:
			mk_find_item_hashtable( 
				right->classTypeValue.variables, 
				mk_get_symbol_name_ptr( vm, "owner" ), 
				(void**)&right );
		case MK_VM_FRAME_ITEM_TYPE_CLASS:
			while( right->classTypeValue.child != NULL )
				right = right->classTypeValue.child;
			target->classTypeValue.typeName = 
				mk_get_symbol_name_ptr( vm, right->classTypeValue.typeName );
			target->classTypeValue.variables = right->classTypeValue.variables;
			break;

		case MK_VM_FRAME_ITEM_TYPE_NIL:
			break;

		case MK_VM_FRAME_ITEM_TYPE_INT_VALUE:
			target->int32TypeValue = right->int32TypeValue;
			break;

		case MK_VM_FRAME_ITEM_TYPE_FLOAT_VALUE:
			target->floatTypeValue = right->floatTypeValue;
			break;

		case MK_VM_FRAME_ITEM_TYPE_STRING_VALUE:
			mk_copy_string( &target->stringTypeValue, right->stringTypeValue );
			break;

		case MK_VM_FRAME_ITEM_TYPE_ARRAY_VALUE:
			target->arrayTypeValue = right->arrayTypeValue;
			break;

		case MK_VM_FRAME_ITEM_TYPE_NODE_VALUE:
			target->code.node = right->code.node;
			target->code.pOwner = right->code.pOwner;
			target->code.definedFrame = right->code.definedFrame;
			break;
		}
		target->flags = 
			MK_TYPE_SET_ATTRIBUTE( target->flags, right->flags );
	}

	mk_vm_push_stack( &vm->localStack, target );
	return MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP;
}

int mk_object_update_variable_value( MK_VM_STRUCT *vm, MK_HASHTABLE *table, MK_SYM_CHAR *symbolName, MK_VM_FRAME_ITEM *value )
{
	mk_insert_item_hashtable( table, symbolName, value );
	return MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP;
}

static
int mk_object_and_and( struct TAG_MK_VM_STRUCT *vm, unsigned int varArgC )
{
	MK_VM_FRAME_ITEM *right = 
		mk_vm_pop_stack( &vm->localStack );
	MK_VM_FRAME_ITEM *target = 
		mk_vm_pop_stack( &vm->localStack );
	MK_VM_FRAME_ITEM *result = 
		mk_vm_create_bool_frame_item( mk_vm_frame_item_is_true( target ) && mk_vm_frame_item_is_true( right ) );
	mk_vm_push_stack( &vm->localStack, result );
	return MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP;
}

static
int mk_object_or_or( struct TAG_MK_VM_STRUCT *vm, unsigned int varArgC )
{
	MK_VM_FRAME_ITEM *right = 
		mk_vm_pop_stack( &vm->localStack );
	MK_VM_FRAME_ITEM *target = 
		mk_vm_pop_stack( &vm->localStack );
	MK_VM_FRAME_ITEM *result = 
		mk_vm_create_bool_frame_item( mk_vm_frame_item_is_true( target ) || mk_vm_frame_item_is_true( right ) );
	mk_vm_push_stack( &vm->localStack, result );
	return MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP;
}

static 
int mk_object_not_equal( struct TAG_MK_VM_STRUCT *vm, unsigned int varArgC )
{
	int retCode =
		mk_object_equal_equal( vm, varArgC );
	if( retCode == MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP )
	{
		MK_VM_FRAME_ITEM *isTrue = 
			mk_vm_pop_stack( &vm->localStack );
		mk_vm_push_stack( &vm->localStack, 
			mk_vm_create_bool_frame_item( !mk_vm_frame_item_is_true( isTrue ) ) );
	}
	return retCode;
}

MK_CLASS *mk_create_object_class( MK_VM_STRUCT *vm )
{
	MK_CLASS *result =
		mk_create_object( MK_TYPE_CLASS );
	MK_VARIABLE *insertMethod = NULL;
	result->nameThis = mk_get_symbol_name_ptr( vm, className );
	result->nameSuper = NULL;

	mk_register_variable(
		vm,
		mk_create_method(
			vm,
			"class",
			MK_TYPE_VARIABLE | 
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD | 
				MK_TYPE_ATTRIBUTE_VARIABLE_STATIC |
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD_NATIVE |
				MK_TYPE_ATTRIBUTE_VARIABLE_NEW_NATIVE_METHOD,
			0,
			(INT_PTR)mk_object_class ),
		result );

	mk_register_variable(
		vm,
		mk_create_method(
			vm,
			"to_s",
			MK_TYPE_VARIABLE | 
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD | 
				MK_TYPE_ATTRIBUTE_VARIABLE_STATIC |
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD_NATIVE |
				MK_TYPE_ATTRIBUTE_VARIABLE_NEW_NATIVE_METHOD,
			0,
			(INT_PTR)mk_object_to_s ),
		result );

	mk_register_variable(
		vm,
		mk_create_method(
			vm,
			"new",
			MK_TYPE_VARIABLE | 
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD | 
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD_NATIVE,
			0,
			(INT_PTR)mk_object_new ),
		result );

	/*
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
			(INT_PTR)mk_object_equal ),
		result );
	*/

	mk_register_variable(
		vm,
		mk_create_default_native_method(
			vm,
			"==",
			MK_TYPE_VARIABLE | 
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD | 
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD_NATIVE |
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD_OPERATOR |
				MK_LEX_TYPE_RESERVED_MARK_SAME |
				MK_TYPE_ATTRIBUTE_VARIABLE_NEW_NATIVE_METHOD, 
			1,
			(INT_PTR)mk_object_equal_equal ),
		result );

	mk_register_variable(
		vm,
		mk_create_default_native_method(
			vm,
			"!=",
			MK_TYPE_VARIABLE | 
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD | 
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD_NATIVE |
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD_OPERATOR |
				MK_LEX_TYPE_RESERVED_MARK_NOT_SAME |
				MK_TYPE_ATTRIBUTE_VARIABLE_NEW_NATIVE_METHOD, 
			1,
			(INT_PTR)mk_object_not_equal ),
		result );

	mk_register_variable(
		vm,
		mk_create_default_native_method(
			vm,
			"&&",
			MK_TYPE_VARIABLE | 
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD | 
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD_NATIVE |
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD_OPERATOR |
				MK_LEX_TYPE_RESERVED_MARK_AND_AND |
				MK_TYPE_ATTRIBUTE_VARIABLE_NEW_NATIVE_METHOD, 
			1,
			(INT_PTR)mk_object_and_and ),
		result );

	mk_register_variable(
		vm,
		mk_create_default_native_method(
			vm,
			"||",
			MK_TYPE_VARIABLE | 
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD | 
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD_NATIVE |
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD_OPERATOR |
				MK_LEX_TYPE_RESERVED_MARK_OR_OR |
				MK_TYPE_ATTRIBUTE_VARIABLE_NEW_NATIVE_METHOD, 
			1,
			(INT_PTR)mk_object_or_or ),
		result );

	return result;
}
