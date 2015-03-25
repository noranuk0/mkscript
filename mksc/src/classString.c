#include "mk.h"
#include <string.h>
static const MK_CHAR className[] = CLASS_INTERNAL_STRING;
static const MK_CHAR classSuper[] = CLASS_INTERNAL_OBJECT;


static
int mk_string_concatenate( struct TAG_MK_VM_STRUCT *vm, unsigned int varArgC )
{
	MK_VM_FRAME_ITEM *target = 
		mk_vm_pop_stack( &vm->localStack );
	MK_VM_FRAME_ITEM *result =
		mk_create_vm_frame_item_object( &vm->pFrameItemTable );
	MK_VM_FRAME_ITEM **targetStrings = 
		malloc( sizeof( MK_VM_FRAME_ITEM* ) * varArgC );
	int index = 0;
	int totalSize = 0;

	for( index = 0; index < varArgC; index ++ )
	{
		MK_VM_FRAME_ITEM *classString = NULL;
		targetStrings[index] = mk_vm_pop_stack( &vm->localStack );
		classString = 
			mk_vm_find_instance( vm, 
								 target,
								 vm->cache->internalClassSymbolName[MK_VM_FRAME_ITEM_TYPE_STRING_VALUE>>24] );
		if( classString != NULL )
		{
			targetStrings[index] = classString;
		}
		else
		{
			// call to_s method.
		}
		totalSize += targetStrings[index]->sizeString;
	}
	result->flags = 
		MK_TYPE_SET_ATTRIBUTE(result->flags, MK_VM_FRAME_ITEM_TYPE_STRING_VALUE);
	result->stringTypeValue = 
		malloc( sizeof(MK_CHAR) * ( totalSize + 1 ) );
	result->stringTypeValue[0] = '\0';
	for( index = 0; index < varArgC; index ++ )
		strcat( result->stringTypeValue, targetStrings[index]->stringTypeValue );
	mk_vm_push_stack( &vm->localStack, result );
	return 0;
}

static
int mk_string_format( struct TAG_MK_VM_STRUCT *vm, unsigned int varArgC )
{
	MK_VM_FRAME_ITEM *target = 
		mk_vm_pop_stack( &vm->localStack );
	MK_VM_FRAME_ITEM *result = 
		mk_create_vm_frame_item_object( &vm->pFrameItemTable );
	int index = 0;
	int size = 0;
	if( MK_TYPE_ATTRIBUTE(target->flags) != MK_VM_FRAME_ITEM_TYPE_STRING_VALUE )
		target = 
			mk_vm_find_instance( vm, 
				target, 
				vm->cache->internalClassSymbolName[MK_VM_FRAME_ITEM_TYPE_STRING_VALUE>>24] );

	return 0;
}

static
int mk_string_sub( struct TAG_MK_VM_STRUCT *vm, unsigned int varArgC )
{
	MK_VM_FRAME_ITEM *size = 
		mk_vm_pop_stack( &vm->localStack );
	MK_VM_FRAME_ITEM *start = 
		mk_vm_pop_stack( &vm->localStack );
	MK_VM_FRAME_ITEM *target = 
		mk_vm_pop_stack( &vm->localStack );
	MK_VM_FRAME_ITEM *result = NULL;
	int sizeTotal = 0;
	int posStart = 0;
	int sizeSub = 0;

	if( MK_TYPE_ATTRIBUTE(target->flags) != MK_VM_FRAME_ITEM_TYPE_STRING_VALUE )
		target = 
			mk_vm_find_instance( vm, 
				target, 
				vm->cache->internalClassSymbolName[MK_VM_FRAME_ITEM_TYPE_STRING_VALUE>>24] );
	result = mk_create_vm_frame_item_object( &vm->pFrameItemTable );
	sizeTotal = strlen( target->stringTypeValue );
	posStart = mk_vm_frame_item_to_int32( vm, start );
	sizeSub = mk_vm_frame_item_to_int32( vm, size );

	if( posStart + sizeSub > sizeTotal )
		;	// out of range.

	result->flags |= MK_VM_FRAME_ITEM_TYPE_STRING_VALUE;
	result->stringTypeValue = malloc( sizeSub + 1 );
	strncpy( result->stringTypeValue, target->stringTypeValue + posStart, sizeSub + 1 );
	result->stringTypeValue[sizeSub] = '\0';
	mk_vm_push_stack( 
		&vm->localStack, 
		result );
	return MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP;
}

static
int mk_string_size( struct TAG_MK_VM_STRUCT *vm, unsigned int varArgC )
{
	MK_VM_FRAME_ITEM *target = 
		mk_vm_pop_stack( &vm->localStack );
	if( MK_TYPE_ATTRIBUTE(target->flags) != MK_VM_FRAME_ITEM_TYPE_STRING_VALUE )
		target = 
			mk_vm_find_instance( vm, 
				target, 
				vm->cache->internalClassSymbolName[MK_VM_FRAME_ITEM_TYPE_STRING_VALUE>>24] );
	mk_vm_push_stack( 
		&vm->localStack, 
		mk_vm_create_int32_frame_item( vm, strlen( target->stringTypeValue ) ) );
	return MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP;
}

static 
int mk_string_plus( struct TAG_MK_VM_STRUCT *vm, unsigned int varArgC )
{
	MK_VM_FRAME_ITEM *right = 
		mk_vm_pop_stack( &vm->localStack );
	MK_VM_FRAME_ITEM *target = 
		mk_vm_pop_stack( &vm->localStack );
	MK_VM_FRAME_ITEM *result = 
		mk_create_vm_frame_item_object( &vm->pFrameItemTable );
	result->flags |= MK_VM_FRAME_ITEM_TYPE_STRING_VALUE;

	if( MK_TYPE_ATTRIBUTE(target->flags) != MK_VM_FRAME_ITEM_TYPE_STRING_VALUE )
		target = 
			mk_vm_find_instance( vm, 
				target, 
				vm->cache->internalClassSymbolName[MK_VM_FRAME_ITEM_TYPE_STRING_VALUE>>24] );

	if( ( (INT_PTR)right & MK_VM_FRAME_ITEM_TYPE_DIRECT_VALUE ) ||
		MK_TYPE_ATTRIBUTE( right->flags ) != MK_VM_FRAME_ITEM_TYPE_STRING_VALUE )
	{
		MK_VARIABLE *pMethod = NULL;
		int retCode = 0;

		// call right.to_s()
		pMethod = 
			mk_vm_find_method( vm, 
				right, 
				mk_get_symbol_name_ptr( vm, "to_s" ), 
				0 );
		mk_vm_push_stack( &vm->localStack, right );
		retCode = 
			mk_vm_call_method( vm, pMethod, NULL, 0 );
		if( retCode == MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP )
			right = mk_vm_pop_stack( &vm->localStack );
		else
			return retCode;
	}
	result->stringTypeValue = 
		malloc( strlen( target->stringTypeValue ) + strlen( right->stringTypeValue ) + 1 );
	strcpy( result->stringTypeValue, target->stringTypeValue );
	strcat( result->stringTypeValue, right->stringTypeValue );
	mk_vm_push_stack( 
		&vm->localStack, 
		result );
	return MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP;
}

static 
int mk_string_div( struct TAG_MK_VM_STRUCT *vm, unsigned int varArgC )
{
	MK_VM_FRAME_ITEM *right = 
		mk_vm_pop_stack( &vm->localStack );
	MK_VM_FRAME_ITEM *target = 
		mk_vm_pop_stack( &vm->localStack );
	int length = strlen( target->stringTypeValue );
	MK_VM_FRAME_ITEM *result = 
		mk_create_vm_frame_item_object( &vm->pFrameItemTable );
	if( MK_TYPE_ATTRIBUTE(target->flags) != MK_VM_FRAME_ITEM_TYPE_STRING_VALUE )
		target = 
			mk_vm_find_instance( vm, 
				target, 
				vm->cache->internalClassSymbolName[MK_VM_FRAME_ITEM_TYPE_STRING_VALUE>>24] );

	result->flags |= MK_VM_FRAME_ITEM_TYPE_ARRAY_VALUE;
	result->arrayTypeValue = 
		mk_allocate_vm_managed_vector(
			&vm->pVectorTable, 
			MK_TYPE_VECTOR | MK_TYPE_VECTOR_ARRAY_MANAGED_PTR | ( MK_SIZEOF_EXTEND_VECTOR_DEFAULT << 10 ),
			MK_SIZEOF_VECTOR_DEFAULT,
			(INT_PTR)NULL );
	if( ( (INT_PTR)right & MK_VM_FRAME_ITEM_TYPE_DIRECT_VALUE ) ||
		MK_TYPE_ATTRIBUTE( right->flags ) == MK_VM_FRAME_ITEM_TYPE_INT_VALUE )
	{
		int size = mk_vm_frame_item_to_int32( vm, right );
		MK_CHAR *buffer = 
				malloc( sizeof(MK_CHAR) * ( size + 1 ) ), 
			*current = target->stringTypeValue;
		buffer[size] = '\0';
		while( current < target->stringTypeValue + length )
		{
			MK_VM_FRAME_ITEM *newItem = 
				mk_create_vm_frame_item_object( &vm->pFrameItemTable );
			newItem->flags |= MK_VM_FRAME_ITEM_TYPE_STRING_VALUE;
			strncpy( buffer, current, size );
			mk_copy_string( &newItem->stringTypeValue, buffer );
			mk_push_vector( result->arrayTypeValue, (INT_PTR)newItem );
			current += size;
		}
		free( buffer );

	}
	else if( MK_TYPE_ATTRIBUTE( right->flags ) == MK_VM_FRAME_ITEM_TYPE_STRING_VALUE )
	{
		MK_CHAR *buffer = malloc( sizeof(MK_CHAR) * ( length + 1 ) ), *current = NULL;
		strcpy( buffer, target->stringTypeValue );
		current = strtok( buffer, right->stringTypeValue );
		while( current != NULL )
		{
			MK_VM_FRAME_ITEM *newItem = 
				mk_create_vm_frame_item_object( &vm->pFrameItemTable );
			newItem->flags |= MK_VM_FRAME_ITEM_TYPE_STRING_VALUE;
			mk_copy_string( &newItem->stringTypeValue, current );
			mk_push_vector( result->arrayTypeValue, (INT_PTR)newItem );
			current = strtok( NULL, right->stringTypeValue );
		}
		free( buffer );
	}
	mk_vm_push_stack( 
		&vm->localStack, 
		result );
	return MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP;
}

static
int mk_string_bracket( struct TAG_MK_VM_STRUCT *vm, unsigned int varArgC )
{
	MK_VM_FRAME_ITEM *right = 
		mk_vm_pop_stack( &vm->localStack );
	MK_VM_FRAME_ITEM *target = 
		mk_vm_pop_stack( &vm->localStack );
	int sizeTotal = 0;
	int position = 0;
	MK_VM_FRAME_ITEM *result = NULL;

	if( MK_TYPE_ATTRIBUTE(target->flags) != MK_VM_FRAME_ITEM_TYPE_STRING_VALUE )
		target = 
			mk_vm_find_instance( vm, 
				target, 
				vm->cache->internalClassSymbolName[MK_VM_FRAME_ITEM_TYPE_STRING_VALUE>>24] );
	sizeTotal = strlen( target->stringTypeValue );
	position = mk_vm_frame_item_to_int32( vm, right );
	result = mk_create_vm_frame_item_object( &vm->pFrameItemTable );
	if( position > sizeTotal )
		result = mk_vm_create_int32_frame_item( vm, -1 );
	else
		result = mk_vm_create_int32_frame_item( vm, target->stringTypeValue[position] );
	mk_vm_push_stack( 
		&vm->localStack, 
		result );
	return MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP;
}

static
int mk_string_operator_bracket_ref( struct TAG_MK_VM_STRUCT *vm, unsigned int varArgC )
{
	int retCode = MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP;
	MK_VM_FRAME_ITEM *right = 
		mk_vm_pop_stack( &vm->localStack );
	MK_CHAR *targetString = NULL;
	int index = 
		mk_vm_frame_item_to_int32( vm, right );
	MK_VM_FRAME_ITEM *target = 
		mk_vm_pop_stack( &vm->localStack );

	if( MK_TYPE_ATTRIBUTE(target->flags) != MK_VM_FRAME_ITEM_TYPE_STRING_VALUE )
		target = 
			mk_vm_find_instance( vm, 
				target, 
				vm->cache->internalClassSymbolName[MK_VM_FRAME_ITEM_TYPE_STRING_VALUE>>24] );
	targetString =
		target->stringTypeValue;

	if( targetString != NULL && 
		index >= 0 &&
		index < strlen( targetString ) )
	{
		MK_VM_FRAME_ITEM *result = NULL;
		result = mk_create_vm_frame_item_object( &vm->pFrameItemTable );
		result->flags |= MK_VM_FRAME_ITEM_TYPE_REFERENCE_STRING_VALUE;
		result->referenceStringTypeValue.target = target;
		result->referenceStringTypeValue.index = index;
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
int mk_string_initialize( MK_VM_STRUCT *vm, int varArgC )
{
	MK_VM_FRAME_ITEM *initValue = 
		mk_vm_pop_stack( &vm->localStack );
	MK_VM_FRAME_ITEM *target = 
		mk_vm_pop_stack( &vm->localStack );
	int retCode = MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP;
	if( MK_TYPE_ATTRIBUTE(target->flags) != MK_VM_FRAME_ITEM_TYPE_STRING_VALUE )
		target = 
			mk_vm_find_instance( vm, 
				target, 
				vm->cache->internalClassSymbolName[MK_VM_FRAME_ITEM_TYPE_STRING_VALUE>>24] );

	if( ( ( (INT_PTR)initValue & MK_VM_FRAME_ITEM_TYPE_DIRECT_VALUE ) != 0 ) ||
		( MK_TYPE_ATTRIBUTE( initValue->flags ) != MK_VM_FRAME_ITEM_TYPE_STRING_VALUE ) )
	{
		MK_VARIABLE *pMethod = 
			mk_vm_find_method( vm, initValue, mk_get_symbol_name_ptr( vm, "to_s" ), 0 );
		if( pMethod != NULL )
		{
			mk_vm_push_stack( &vm->localStack, initValue );
			retCode = mk_vm_call_method( vm, pMethod, NULL, 0 );
			if( retCode == MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP )
				initValue = mk_vm_pop_stack( &vm->localStack );
		}
	}
	if( retCode == MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP )
	{
		if( MK_TYPE_ATTRIBUTE( initValue->flags ) == MK_VM_FRAME_ITEM_TYPE_STRING_VALUE )
		{
			if( initValue->stringTypeValue != NULL )
				mk_copy_string( &target->stringTypeValue, initValue->stringTypeValue );
			else
				mk_copy_string( &target->stringTypeValue, "" );
			mk_vm_push_stack( &vm->localStack, target );
		}
		else
		{
			retCode = MK_VM_EXECUTE_EXPR_THROW;	//todo: type error
		}
	}
	return retCode;
}

MK_CLASS *mk_create_string_class( MK_VM_STRUCT *vm )
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
			mk_create_object( MK_TYPE_NODE_EXPR | MK_TYPE_NODE_EXPR_STRING );
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
				(INT_PTR)mk_string_initialize );

		mk_register_variable(
			vm,
			pMethod,
			result );
	}

	mk_register_variable(
		vm,
		mk_create_default_native_method(
			vm,
			"sub",
			MK_TYPE_VARIABLE | 
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD | 
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD_NATIVE |
				MK_TYPE_ATTRIBUTE_VARIABLE_NEW_NATIVE_METHOD,
			2,
			(INT_PTR)mk_string_sub ),
		result );

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
			(INT_PTR)mk_string_size ),
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
			(INT_PTR)mk_string_plus ),
		result);

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
			(INT_PTR)mk_string_div ),
		result);

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
			(INT_PTR)mk_string_bracket ),
		result);

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
			(INT_PTR)mk_string_operator_bracket_ref ),
		result);
	return result;
}

