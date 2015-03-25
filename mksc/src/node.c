#include "mk.h"
#include <stdlib.h>
#include <string.h>
#ifdef __APPLE__
#include <sys/malloc.h>
#else 
#include <malloc.h>
#endif

MK_CHAR *mk_get_symbol_name_ptr( MK_VM_STRUCT *vm, const MK_CHAR *target )
{
	MK_CHAR *result = NULL;
	MK_CHAR *keyPtr = NULL;
	if( mk_find_item_hashtable(vm->hashSymbolName, target, (void**)&keyPtr ) == 0 )
	{
		result = mk_allocate_memory_pool( &vm->memoryPool,
			target, 
			sizeof( MK_CHAR ) * ( strlen( target ) + 1 ) );
		mk_insert_item_hashtable( vm->hashSymbolName, result, result );
	}
	else
	{
		result = (MK_CHAR*)keyPtr;
	}
	return result;
}

void *mk_allocate_memory_pool( MK_MEMORY_POOL **target, const void *ptr, unsigned int size )
{
	void *result = NULL;
	
	if( target == NULL || 
		size == 0 || 
		size > sizeof( (*target)->memory ) )
		return NULL;
	while( *target != NULL )
	{
		if( (*target)->next + size <= (*target)->memory + MK_SIZEOF_MEMORY_POOL_BLOCK )
			break;
		*target = (*target)->previous;
	}
	if( *target == NULL )
	{
		MK_MEMORY_POOL *newMemoryPool = NULL;
		newMemoryPool = malloc( sizeof(MK_MEMORY_POOL) );
		memset( newMemoryPool, 0x00, sizeof(MK_MEMORY_POOL) );
		newMemoryPool->next = newMemoryPool->memory;
		newMemoryPool->previous = *target;
		*target = newMemoryPool;
	}
	result = (*target)->next;
	memcpy( (*target)->next, ptr, size );
	(*target)->next += ( ( size + 3 ) >> 2 ) << 2;
	return result;
}

void mk_copy_string( MK_CHAR **dest, const MK_CHAR *target )
{
	if( dest && target )
	{
		int length = strlen( target );
		*dest = malloc( length + 1 );
		strcpy( *dest, target );
	}
}

static
MK_MANAGED_VM_FRAME_ITEM_TABLE *mk_create_new_frame_item_table( )
{
	MK_MANAGED_VM_FRAME_ITEM_TABLE *result = 
		(MK_MANAGED_VM_FRAME_ITEM_TABLE *)malloc( sizeof(MK_MANAGED_VM_FRAME_ITEM_TABLE) );
	memset( result, 0x00, sizeof(MK_MANAGED_VM_FRAME_ITEM_TABLE) );
	result->freeSpace = MK_SIZEOF_MANAGED_TABLE;
	return result;
}

MK_VM_FRAME_ITEM *mk_create_vm_frame_item_object( MK_MANAGED_VM_FRAME_ITEM_TABLE **itemTable )
{
	MK_MANAGED_VM_FRAME_ITEM_TABLE *current = *itemTable, *previous = NULL;
	MK_VM_FRAME_ITEM *ptr = NULL;

	while( current != NULL )
	{
		if( current->freeSpace > 0 )
			break;
		previous = current;
		current = current->previous;
	}
	if( current == NULL )
	{
		current = mk_create_new_frame_item_table( );
		current->previous = *itemTable;
		*itemTable = current;
	}
	else if( previous != NULL )
	{
		previous->previous = current->previous;
		current->previous = *itemTable;
		*itemTable = current;
	}

	ptr = current->table + current->nextIndex;
	while( ptr < current->table + MK_SIZEOF_MANAGED_TABLE )
	{
		if( MK_GC_IS_ALLOCATED_OBJECT( ptr->flags ) == 0 )
			break;
		ptr ++;
	}
	if( ptr >= current->table + MK_SIZEOF_MANAGED_TABLE )
	{
		ptr = current->table;
		while( ptr != current->table + current->nextIndex )
		{
			if( MK_GC_IS_ALLOCATED_OBJECT( ptr->flags ) == 0 )
				break;
			ptr ++;
		}
	}
	ptr->flags = MK_TYPE_SET_ATTRIBUTE( MK_OBJECT_TYPE_VM_FRAME_ITEM, MK_VM_FRAME_ITEM_TYPE_UNINITIALIZED );
	MK_GC_SET_MANAGED_BIT( ptr->flags );
	MK_GC_ALLOCATE_OBJECT( ptr->flags );
	MK_GC_SET_CN_BIT( ptr->flags );
	current->freeSpace --;
	current->nextIndex = ptr - current->table + 1;

	return ptr;
}

static
MK_MANAGED_VM_FRAME_TABLE *mk_create_new_frame_table( )
{
	MK_MANAGED_VM_FRAME_TABLE *result = 
		(MK_MANAGED_VM_FRAME_TABLE *)malloc( sizeof(MK_MANAGED_VM_FRAME_TABLE) );
	memset( result, 0x00, sizeof(MK_MANAGED_VM_FRAME_TABLE) );
	result->freeSpace = MK_SIZEOF_MANAGED_TABLE;
	return result;
}

MK_VM_FRAME *mk_create_vm_frame_object( MK_MANAGED_VM_FRAME_TABLE **frameTable )
{
	MK_MANAGED_VM_FRAME_TABLE *current = *frameTable;
	MK_VM_FRAME *ptr = NULL;

	while( current != NULL )
	{
		if( current->freeSpace > 0 )
			break;
		current = current->previous;
	}
	if( current == NULL )
	{
		current = mk_create_new_frame_table( );
		current->previous = *frameTable;
		*frameTable = current;
	}
	ptr = current->table + current->nextIndex;
	while( ptr != current->table + MK_SIZEOF_MANAGED_TABLE )
	{
		if( MK_GC_IS_ALLOCATED_OBJECT( ptr->flags ) == 0 )
			break;
		ptr ++;
	}
	if( ptr == current->table + MK_SIZEOF_MANAGED_TABLE )
	{
		ptr = current->table;
		while( ptr != current->table + current->nextIndex )
		{
			if( MK_GC_IS_ALLOCATED_OBJECT( ptr->flags ) == 0 )
				break;
			ptr ++;
		}
	}
	ptr->flags = MK_OBJECT_TYPE_VM_FRAME;
	MK_GC_SET_MANAGED_BIT( ptr->flags );
	MK_GC_ALLOCATE_OBJECT( ptr->flags );
	MK_GC_SET_CN_BIT( ptr->flags );
	// initialize after this function ptr->previous, ptr->pMethod, ptr->pThis
	current->freeSpace --;
	current->nextIndex = ptr - current->table;

	return ptr;
}

void *mk_create_object( unsigned int nsType )
{
	void *result = NULL;
	unsigned int size = 0;

	switch( MK_OBJECT_TYPE( nsType ) )
	{
	case MK_OBJECT_TYPE_VM_STRUCT:
		size = sizeof( MK_VM_STRUCT );
		break;
	case MK_OBJECT_TYPE_VM_FRAME:
		size = sizeof( MK_VM_FRAME );			// use mk_create_vm_frame_object insted of mk_create_object.
		break;
	case MK_OBJECT_TYPE_OBJECTCODE:
		size = sizeof( MK_OBJECTCODE );
		break;
	case MK_OBJECT_TYPE_VM_FRAME_ITEM:
		size = sizeof( MK_VM_FRAME_ITEM );	// use mk_create_vm_frame_item_object insted of mk_create_object.
		break;
	case MK_TYPE_CLASS:
		size = sizeof( MK_CLASS );
		break;
	case MK_TYPE_VARIABLE:
		size = sizeof( MK_VARIABLE );
		break;
	case MK_TYPE_NODE_BLOCK:
		size = sizeof( MK_NODE_BLOCK );
		break;
	case MK_TYPE_NODE_EXPR:
		size = sizeof( MK_NODE_EXPR );
		break;
	case MK_TYPE_NODE_IF:
		size = sizeof( MK_NODE_IF );
		break;
	case MK_TYPE_NODE_WHILE:
		size = sizeof( MK_NODE_WHILE );
		break;
	case MK_TYPE_TRY_BLOCK:
		size = sizeof( MK_TRY_BLOCK );
		break;
	case MK_TYPE_CATCH_BLOCK:
		size = sizeof( MK_CATCH_BLOCK );
		break;
	default:
		size = 0;
		break;
	}
	if( size > 0 )
	{
		result = 
			malloc( size );
		memset( result, 0x00, size );
		*((unsigned int *)result) = nsType;
	}
	return result;
}

MK_VM_FRAME_ITEM *mk_vm_create_bool_frame_item( int isTrue )
{
	return ( MK_VM_FRAME_ITEM * )
		( ( isTrue ) ? 
			( INT_PTR )( MK_VM_FRAME_ITEM_TYPE_DIRECT_VALUE | 0x00000002 ) : 
			( INT_PTR )( MK_VM_FRAME_ITEM_TYPE_DIRECT_VALUE | 0x00000000 ) );
}

int mk_vm_frame_item_is_true( MK_VM_FRAME_ITEM *target )
{
	if( (INT_PTR)target & MK_VM_FRAME_ITEM_TYPE_DIRECT_VALUE )
	{
		return ( (INT_PTR)target & MK_VM_FRAME_ITEM_DIRECT_INT_VALUE_MASK );
	}
	else
	{
		switch( MK_TYPE_ATTRIBUTE( target->flags ) )
		{
		case MK_VM_FRAME_ITEM_TYPE_NIL:
			return 0;

		case MK_VM_FRAME_ITEM_TYPE_INT_VALUE:
			return target->int32TypeValue != 0;

		case MK_VM_FRAME_ITEM_TYPE_FLOAT_VALUE:
			return target->floatTypeValue != 0;
		
		default:
			return 1;
		}
	}
}

MK_VM_FRAME_ITEM *mk_vm_create_float_frame_item( MK_VM_STRUCT *vm, MK_FLOAT value )
{
	MK_VM_FRAME_ITEM *result = NULL;
	result = mk_create_vm_frame_item_object( &vm->pFrameItemTable );
	result->flags = 
		MK_TYPE_SET_ATTRIBUTE( result->flags, MK_VM_FRAME_ITEM_TYPE_FLOAT_VALUE );
	result->floatTypeValue = value;
	return result;
}

MK_VM_FRAME_ITEM *mk_vm_create_int32_frame_item( MK_VM_STRUCT *vm, int value )
{
	MK_VM_FRAME_ITEM *result = NULL;
	if( abs( value ) < 0x4000000 )
	{
		unsigned int intResult = 0;
		intResult = 
			MK_VM_FRAME_ITEM_TYPE_DIRECT_VALUE | ( abs( value ) << 1 );
		if( value < 0 )
			intResult |= MK_VM_FRAME_ITEM_TYPE_INT_MINUS_VALUE;
		result = ( MK_VM_FRAME_ITEM* )( INT_PTR )intResult;
	}
	else
	{
		result = mk_create_vm_frame_item_object( &vm->pFrameItemTable );
		result->flags = 
			MK_TYPE_SET_ATTRIBUTE( result->flags, MK_VM_FRAME_ITEM_TYPE_INT_VALUE );
		result->int32TypeValue = value;
	}
	return result;
}

MK_FLOAT mk_vm_frame_item_to_float( MK_VM_STRUCT *vm, MK_VM_FRAME_ITEM *target )
{
	MK_VM_FRAME_ITEM *realTarget = NULL;
	MK_FLOAT num = 0.0;
	if( (INT_PTR)target & MK_VM_FRAME_ITEM_TYPE_DIRECT_VALUE )
	{
		num = ( MK_FLOAT )( ( (INT_PTR)target & MK_VM_FRAME_ITEM_DIRECT_INT_VALUE_MASK ) >> 4 );
		if( (INT_PTR)target & MK_VM_FRAME_ITEM_TYPE_INT_MINUS_VALUE )
			num *= -1.0;
	}
	else
	{
		switch( MK_TYPE_ATTRIBUTE( target->flags ) )
		{
		case MK_VM_FRAME_ITEM_TYPE_INT_VALUE:
			num = (MK_FLOAT)target->int32TypeValue;
			break;

		case MK_VM_FRAME_ITEM_TYPE_FLOAT_VALUE:
			num = target->floatTypeValue;
			break;
		case MK_VM_FRAME_ITEM_TYPE_CLASS:
		case MK_VM_FRAME_ITEM_TYPE_MODULE:
			realTarget = 
				mk_vm_find_instance( vm, 
					target, 
					vm->cache->internalClassSymbolName[MK_VM_FRAME_ITEM_TYPE_FLOAT_VALUE >> 24] );
			if( realTarget == NULL )
				realTarget = 
					mk_vm_find_instance( vm, 
						target, 
						vm->cache->internalClassSymbolName[MK_VM_FRAME_ITEM_TYPE_INT_VALUE >> 24] );
			num = mk_vm_frame_item_to_float( vm, realTarget );
		}
	}
	return num;
}

int mk_vm_frame_item_to_int32( MK_VM_STRUCT *vm, MK_VM_FRAME_ITEM *target )
{
	MK_VM_FRAME_ITEM *realTarget = NULL;
	if( (INT_PTR)target & MK_VM_FRAME_ITEM_TYPE_DIRECT_VALUE )
	{
		int result = 
			( (INT_PTR)target & MK_VM_FRAME_ITEM_DIRECT_INT_VALUE_MASK ) >> 1;
		if( (INT_PTR)target & MK_VM_FRAME_ITEM_TYPE_INT_MINUS_VALUE )
			result *= -1;
		return result;
	}
	else
	{
		int result = 0;
		switch( MK_TYPE_ATTRIBUTE( target->flags ) )
		{
		case MK_VM_FRAME_ITEM_TYPE_INT_VALUE:
			result = target->int32TypeValue;
			break;

		case MK_VM_FRAME_ITEM_TYPE_FLOAT_VALUE:
			result = (int)target->floatTypeValue;
			break;
		case MK_VM_FRAME_ITEM_TYPE_CLASS:
		case MK_VM_FRAME_ITEM_TYPE_MODULE:
			realTarget = 
				mk_vm_find_instance( vm, 
					target, 
					vm->cache->internalClassSymbolName[MK_VM_FRAME_ITEM_TYPE_INT_VALUE >> 24] );
			if( realTarget == NULL )
				realTarget = 
					mk_vm_find_instance( vm, 
						target, 
						vm->cache->internalClassSymbolName[MK_VM_FRAME_ITEM_TYPE_FLOAT_VALUE >> 24] );
			result = mk_vm_frame_item_to_int32( vm, realTarget );
		}
		return result;
	}
}

void mk_destroy_vector_node( MK_VECTOR *target )
{
	if( target != NULL )
	{
		unsigned int size = 0;
		unsigned int index = 0;
		size = target->used;
		if( MK_TYPE_ATTRIBUTE_VECTOR_STYLE(target->flags) == MK_TYPE_VECTOR_ARRAY_MANAGED_PTR )
		{
			for( index = 0; index < size; index ++ )
				mk_destroy_node( (void*)target->items[index] );
		}
		if( ( target->flags & MK_TYPE_VECTOR_POINT_MEMORY_DIRECT ) == 0 )
			free( target->items );
		free( target );
	}
}

void mk_destroy_hashtable_node( MK_HASHTABLE *target )
{
	if( target != NULL )
	{
		int extendSize = 
			MK_TYPE_ATTRIBUTE_HASH_SIZE_EXTEND_VALUE(target->flags);
		int isDeleteKey = 
			( MK_TYPE_ATTRIBUTE_HASH_KEYTYPE( target->flags ) == MK_TYPE_ATTRIBUTE_HASHKEY_FREEABLE_STRING ) ||
			( MK_TYPE_ATTRIBUTE_HASH_KEYTYPE( target->flags ) == MK_TYPE_ATTRIBUTE_HASHKEY_FREEABLE_PTR );
		int isDeleteValue[4] = 
		{
			( extendSize > 0 ) ? MK_TYPE_ATTRIBUTE_HASH_VALUETYPE_VALUE(0, target->flags) : 0,
			( extendSize > 1 ) ? MK_TYPE_ATTRIBUTE_HASH_VALUETYPE_VALUE(1, target->flags) : 0,
			( extendSize > 2 ) ? MK_TYPE_ATTRIBUTE_HASH_VALUETYPE_VALUE(2, target->flags) : 0,
			( extendSize > 3 ) ? MK_TYPE_ATTRIBUTE_HASH_VALUETYPE_VALUE(3, target->flags) : 0,
		};
		if( ( isDeleteKey != 0 ) ||
			( isDeleteValue[0] & 0x00000001 ) ||
			( isDeleteValue[1] & 0x00000001 ) ||
			( isDeleteValue[2] & 0x00000001 ) ||
			( isDeleteValue[3] & 0x00000001 ) )
		{
			void *handle = mk_enum_item_hashtable_begin( target );
			while( handle != NULL )
			{
				int index = 0;
				const MK_CHAR *key = NULL;
				void *value = NULL;
				handle = mk_enum_item_hashtable_next( target, handle, &key, &value );
				if( isDeleteKey )
					free( ( void* ) key );
				if( isDeleteValue[0] == MK_TYPE_ATTRIBUTE_HASH_VALUE_FREEABLE_PTR )
					free( value );
				else if( isDeleteValue[0] == MK_TYPE_ATTRIBUTE_HASH_VALUE_MK_OBJECT_PTR )
					mk_destroy_node( value );
				for( index = 1; index < 4; index ++ )
					;
			}
		}
		free( target->elems );
		if( MK_GC_IS_MANAGED_BIT( target->flags ) == 0 )
		{
			free( target );
		}
		else
		{
			memset( target, 0x00, sizeof(MK_HASHTABLE) );
		}
	}
}

void mk_destroy_vm_frame_item( MK_VM_FRAME_ITEM *target )
{
	switch( MK_TYPE_ATTRIBUTE( target->flags ) )
	{
	case MK_VM_FRAME_ITEM_TYPE_STRING_VALUE:
		free( target->stringTypeValue );
		break;

	case MK_VM_FRAME_ITEM_TYPE_CLASS:
		break;
	}
	target->flags = 0;
}

static
void mk_destroy_expr( MK_NODE_EXPR *target )
{
	unsigned int attribute = 0;
	if( target == NULL )
		return;

	attribute = MK_TYPE_ATTRIBUTE_MASK & target->flags;
	switch( attribute )
	{
	case MK_TYPE_NODE_EXPR_FUNCTION_CALL:
	case MK_TYPE_NODE_EXPR_FUNCTION_CALL_INSTANCE:
	case MK_TYPE_NODE_EXPR_FUNCTION_CALL_STATIC:
	case MK_TYPE_NODE_EXPR_NEW:
	case MK_TYPE_NODE_EXPR_SUPER_CALL:
	case MK_TYPE_NODE_EXPR_ME:
		mk_destroy_vector_node( target->u2.args );
		break;

	case MK_TYPE_NODE_EXPR_NODE_WITH_PARAM:
		mk_destroy_node( target->u1.node );
		break;

	case MK_TYPE_NODE_EXPR_SYMBOL:
	case MK_TYPE_NODE_EXPR_ATSYMBOL:
	case MK_TYPE_NODE_EXPR_DBLATSYMBOL:
		break;
	case MK_TYPE_NODE_EXPR_MULTIPLESYMBOL:
		mk_destroy_vector_node( target->u1.multipleSymbols );
		break;

	case MK_TYPE_NODE_EXPR_OPERATION:
	case MK_TYPE_NODE_EXPR_BACK_IF_CONDITION:
		mk_destroy_node( target->u1.left );
		mk_destroy_node( target->u2.right );
		break;

	case MK_TYPE_NODE_EXPR_RETURN:
		mk_destroy_node( target->u1.left );
		break;

	case MK_TYPE_NODE_EXPR_RAISE:
		mk_destroy_node( target->u1.left );
		break;

	case MK_TYPE_NODE_EXPR_BREAK:
		break;

	case MK_TYPE_NODE_EXPR_CONTINUE:
		break;

	case MK_TYPE_NODE_EXPR_SYMBOL_THIS:
		break;

	case MK_TYPE_NODE_EXPR_SYMBOL_SUPER:
		break;

	case MK_TYPE_NODE_EXPR_SYMBOL_NIL:
		break;

	case MK_TYPE_NODE_EXPR_SYMBOL_TRUE:
		break;

	case MK_TYPE_NODE_EXPR_SYMBOL_FALSE:
		break;

	case MK_TYPE_NODE_EXPR_STRING:
		free( target->u2.value );
		break;

	case MK_TYPE_NODE_EXPR_INT32:
	case MK_TYPE_NODE_EXPR_INT64:
	case MK_TYPE_NODE_EXPR_FLOAT:
		break;
	
	case MK_TYPE_NODE_EXPR_ARRAY_DEFINITION:
		mk_destroy_vector_node( target->u1.arrayDefinition );
		break;

	case MK_TYPE_NODE_EXPR_STRING_FORMAT_ROOT:
		mk_destroy_vector_node( target->u1.stringParts );
		break;

	case MK_TYPE_NODE_EXPR_STRING_FORMAT_DEFUALT:
	case MK_TYPE_NODE_EXPR_STRING_FORMAT_LEFT:
	case MK_TYPE_NODE_EXPR_STRING_FORMAT_RIGHT:
	case MK_TYPE_NODE_EXPR_STRING_FORMAT_TIME:
		mk_destroy_node( target->u1.left );
		mk_destroy_node( target->u2.right );
		break;
	}
}

void mk_destroy_node( void *target )
{

	unsigned int flags = 0;
	unsigned int switchFlag = 0;
	int isFail = 0;

	if( target == NULL )
		return;
	if( (INT_PTR)target & MK_VM_FRAME_ITEM_TYPE_DIRECT_VALUE )
		return;

	flags =	*( (unsigned int*)target );
	switchFlag = MK_OBJECT_TYPE( flags );
	switch( switchFlag )
	{
	case MK_OBJECT_TYPE_VM_STRUCT:
		{
			MK_VM_STRUCT *vm = 
				(MK_VM_STRUCT*)target;
			MK_MANAGED_VM_FRAME_TABLE *pFrameTable = vm->pFrameTable;
			MK_MANAGED_VM_FRAME_ITEM_TABLE *pTable = vm->pFrameItemTable;
			MK_MANAGED_HASH_TABLE *pHash = vm->pHashTable;
			MK_MANAGED_VECTOR_TABLE *pVector = vm->pVectorTable;
			MK_MEMORY_POOL *memoryPool = vm->memoryPool;
			mk_destroy_node( vm->code );
			free( vm->cache );
			mk_destroy_hashtable_node( vm->hashSymbolName );
			free( vm->localStack.start );
			while( pTable )
			{
				int index = 0;
				MK_MANAGED_VM_FRAME_ITEM_TABLE *pNext = NULL;
				for( index = 0; index < MK_SIZEOF_MANAGED_TABLE; index ++ )
				{
					if( MK_GC_IS_ALLOCATED_OBJECT( pTable->table[index].flags ) )
						mk_destroy_vm_frame_item( &pTable->table[index] );
				}
				pNext = pTable->previous;
				free( pTable );
				pTable = pNext;
			}
			while( pFrameTable )
			{
				MK_MANAGED_VM_FRAME_TABLE *pNext = pFrameTable->previous;
				free( pFrameTable );
				pFrameTable = pNext;
			}
			while( pHash )
			{
				int index = 0;
				MK_MANAGED_HASH_TABLE *pNext = NULL;
				for( index = 0; index < MK_SIZEOF_MANAGED_TABLE; index ++ )
				{
					MK_HASHTABLE *pTarget = 
						&pHash->hashTables[index];
					if( MK_OBJECT_TYPE( pTarget->flags ) == MK_TYPE_HASHTABLE )
						mk_destroy_hashtable_node( pTarget );
				}
				pNext = pHash->previous;
				free( pHash );
				pHash = pNext;
			}
			while( pVector )
			{
				int index = 0;
				MK_MANAGED_VECTOR_TABLE *pNext = NULL;
				for( index = 0; index < MK_SIZEOF_MANAGED_TABLE; index ++ )
				{
					MK_VECTOR *pTarget = 
						&pVector->vectors[index];
					if( MK_GC_IS_ALLOCATED_OBJECT( pTarget->flags ) )
						free( pTarget->items );
				}
				pNext = pVector->previous;
				free( pVector );
				pVector = pNext;
			}
			while( memoryPool )
			{
				MK_MEMORY_POOL *previous = memoryPool->previous;
				free( memoryPool );
				memoryPool = previous;
			}
			vm->memoryPool = NULL;
		}
		break;

	case MK_OBJECT_TYPE_OBJECTCODE:
		{
			MK_OBJECTCODE *objectCode = 
				( MK_OBJECTCODE * )target;
			mk_destroy_hashtable_node( objectCode->classes );
		}
		break;

	case MK_OBJECT_TYPE_VM_FRAME:
		break;
	
	case MK_OBJECT_TYPE_VM_FRAME_ITEM:
		isFail = 1;		// managed object.
		break;

	case MK_TYPE_CLASS:
		{
			MK_CLASS *classTarget = 
				( MK_CLASS * )target;
			mk_destroy_vector_node( classTarget->usingNames );

			mk_destroy_hashtable_node( classTarget->variables );

			if( classTarget->operatorMethod != NULL )
			{
				int index = 0;
				for( index = 0; index < MK_SIZEOF_ENABELD_OPELATORS; index ++ )
					mk_destroy_node( (void*)classTarget->operatorMethod[index] );
				free( classTarget->operatorMethod );
			}
		}
		break;
	
	case MK_TYPE_VARIABLE:
		{
			MK_VARIABLE *variableTarget = 
				( MK_VARIABLE * )target;
			if( variableTarget->flags & MK_TYPE_ATTRIBUTE_VARIABLE_METHOD )
			{	// method
				mk_destroy_vector_node( variableTarget->args );
				if( !( variableTarget->flags & MK_TYPE_ATTRIBUTE_VARIABLE_VIRTUAL ) &&
					!( variableTarget->flags & MK_TYPE_ATTRIBUTE_VARIABLE_METHOD_NATIVE ) )
					mk_destroy_node( variableTarget->entryPoint );
			}
			else if( !( variableTarget->flags & MK_TYPE_ATTRIBUTE_VARIABLE_MODULE ) )
			{	// value
				mk_destroy_node( variableTarget->defaultValue );
			}
		}
		break;
	
	case MK_TYPE_NODE_BLOCK:
		{
			MK_NODE_BLOCK *blockTarget =
				( MK_NODE_BLOCK * )target;
			mk_destroy_vector_node( blockTarget->exprs );
		}
		break;

	case MK_TYPE_NODE_IF:
		{
			MK_NODE_IF *ifTarget =
				( MK_NODE_IF * )target;
			MK_NODE_IF *previous = NULL;
			while( ifTarget != NULL )
			{
				mk_destroy_node( ifTarget->expr );
				mk_destroy_node( ifTarget->block );
				previous = ifTarget;
				ifTarget = ifTarget->next;
				if( previous != target )
					free( previous );
			}
		}
		break;

	case MK_TYPE_NODE_WHILE:
		{
			MK_NODE_WHILE *whileTarget =
				( MK_NODE_WHILE * )target;
			mk_destroy_node( whileTarget->block );
			mk_destroy_node( whileTarget->expr );
		}
		break;

	case MK_TYPE_NODE_EXPR:
		mk_destroy_expr( target );
		break;

	case MK_TYPE_TRY_BLOCK:
		{
			MK_TRY_BLOCK *tryBlock = 
				( MK_TRY_BLOCK *) target;
			mk_destroy_node( tryBlock->blockTry );
			mk_destroy_vector_node( tryBlock->blockCatch );
			mk_destroy_node( tryBlock->blockNoException );
			mk_destroy_node( tryBlock->blockFinally );
		}
		break;
	
	case MK_TYPE_CATCH_BLOCK:
		{
			MK_CATCH_BLOCK *catchBlock = 
				(MK_CATCH_BLOCK*) target;
			if(  catchBlock->blockCatch != NULL )
			{
				MK_VARIABLE *variable = 
					( MK_VARIABLE * )( catchBlock->blockCatch->u1.node );
				variable->args = NULL;
			}
			mk_destroy_node( catchBlock->paramCatch );
			mk_destroy_node( catchBlock->blockCatch );
		}
		break;

	case MK_TYPE_HASHTABLE:	// call	mk_destroy_hashtable_node, mk_destroy_map_item
	case MK_TYPE_VECTOR:	// call mk_destroy_vector_node
	default:
		isFail = 1;
		break;
	}
	if( isFail == 0 )
		free( target );
}
