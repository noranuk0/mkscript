#include "mk.h"
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <stdio.h>

static
void mk_extend_hashtable( MK_HASHTABLE *table );
static
int mk_find_item_position_string( MK_HASHTABLE *table, const MK_CHAR *key );
static
int mk_seek_insert_position_string( MK_HASHTABLE *table, const MK_CHAR *key, int *isKey );
static
int mk_find_item_position_ptr( MK_HASHTABLE *table, const INT_PTR key );
static
int mk_seek_insert_position_ptr( MK_HASHTABLE *table, const INT_PTR key, int *isKey );
static
void mk_remove_key_hashtable_string( MK_HASHTABLE *table, const MK_CHAR *key );
static
void mk_remove_key_hashtable_intptr( MK_HASHTABLE *table, const INT_PTR key );

MK_HASHTABLE *mk_create_hashtable( unsigned int createFlag, unsigned int defaultSize )
{
	int sizeValue = 
		MK_TYPE_ATTRIBUTE_HASH_SIZE_EXTEND_VALUE(createFlag);
	size_t sizeElem = 
		( sizeof(MK_MAP_ITEM) + sizeof(void*) * sizeValue )  * defaultSize;
	MK_HASHTABLE *result = 
		malloc( sizeof(MK_HASHTABLE) );
	result->size = defaultSize;
	result->used = 0;
	result->flags = createFlag;
	result->elems = malloc( sizeElem );
	return result;
}

unsigned int mk_size_item_value_hashtable( MK_HASHTABLE *table )
{
	return MK_TYPE_ATTRIBUTE_HASH_SIZE_EXTEND_VALUE(table->flags);
}

unsigned int mk_size_item_hashtable( MK_HASHTABLE *table )
{
	return table->used;
}

MK_HASHTABLE *mk_allocate_vm_managed_hashtable( unsigned int createFlag, unsigned int defaultSize, MK_MANAGED_HASH_TABLE **managedHashTable )
{
	int sizeValue = 
		MK_TYPE_ATTRIBUTE_HASH_SIZE_EXTEND_VALUE(createFlag);
	size_t sizeElem = 
		( sizeof(MK_MAP_ITEM) + sizeof(void*) * sizeValue )  * defaultSize;
	MK_MANAGED_HASH_TABLE *topHashTable = NULL, *previous = NULL;
	MK_HASHTABLE *pCurrent = NULL;

	topHashTable = *managedHashTable;
	while( topHashTable != NULL )
	{
		if( topHashTable->freeSpace > 0 )
			break;
		previous = topHashTable;
		topHashTable = topHashTable->previous;
	}
	if( topHashTable == NULL )
	{
		topHashTable = ( MK_MANAGED_HASH_TABLE * )malloc( sizeof( MK_MANAGED_HASH_TABLE ) );
		memset( topHashTable, 0x00, sizeof( MK_MANAGED_HASH_TABLE ) );
		topHashTable->freeSpace = MK_SIZEOF_MANAGED_TABLE;
		topHashTable->nextIndex = 0;
		topHashTable->previous = *managedHashTable;
		*managedHashTable = topHashTable;
	}
	else if( previous != NULL )
	{
		previous->previous = topHashTable->previous;
		topHashTable->previous = *managedHashTable;
		*managedHashTable = topHashTable;
	}
	pCurrent = topHashTable->hashTables + topHashTable->nextIndex;
	while( pCurrent < topHashTable->hashTables + MK_SIZEOF_MANAGED_TABLE )
	{
		if( MK_GC_IS_ALLOCATED_OBJECT( pCurrent->flags ) == 0 )
			break;
		pCurrent ++;
	}
	if( pCurrent >= topHashTable->hashTables + MK_SIZEOF_MANAGED_TABLE )
	{
		pCurrent = topHashTable->hashTables;
		while( pCurrent != topHashTable->hashTables + topHashTable->nextIndex )
		{
			if( MK_GC_IS_ALLOCATED_OBJECT( pCurrent->flags ) == 0 )
				break;
			pCurrent ++;
		}
	}

	if( pCurrent->size != defaultSize || 
		MK_TYPE_ATTRIBUTE_HASH_SIZE_EXTEND_VALUE(pCurrent->flags) != sizeValue )
	{
		if( pCurrent->elems != NULL )
			free( pCurrent->elems );
		pCurrent->elems = malloc( sizeElem );
		pCurrent->size = defaultSize;
	}
	pCurrent->flags = createFlag;
	MK_GC_SET_MANAGED_BIT( pCurrent->flags );
	MK_GC_ALLOCATE_OBJECT( pCurrent->flags );
	MK_GC_SET_CN_BIT( pCurrent->flags );
	pCurrent->used = 0;

	topHashTable->freeSpace --;
	topHashTable->nextIndex = pCurrent - topHashTable->hashTables + 1;
	return pCurrent;
}

void* mk_insert_item_hashtable( MK_HASHTABLE *table, const MK_CHAR *key, void *value )
{
	int sizeValue = 
		MK_TYPE_ATTRIBUTE_HASH_SIZE_EXTEND_VALUE(table->flags);
	size_t sizeElem = 
		( sizeof(MK_MAP_ITEM) + sizeof(void*) * sizeValue );
	unsigned int insertIndex = 0;
	int isKey = 0;

	if( table->used > 0 )
	{
		switch( MK_TYPE_ATTRIBUTE_HASH_KEYTYPE( table->flags ) )
		{
		case MK_TYPE_ATTRIBUTE_HASHKEY_STRING:
		case MK_TYPE_ATTRIBUTE_HASHKEY_FREEABLE_STRING:
			insertIndex = mk_seek_insert_position_string( table, key, &isKey );
			break;
		case MK_TYPE_ATTRIBUTE_HASHKEY_INTPTR:
		case MK_TYPE_ATTRIBUTE_HASHKEY_FREEABLE_PTR:
			insertIndex = mk_seek_insert_position_ptr( table, (INT_PTR)key, &isKey );
			break;
		}
	}
	if( isKey == 0 )
	{
		MK_MAP_ITEM *target = NULL;
		if( table->size == table->used )
			mk_extend_hashtable( table );
		target = 
			( MK_MAP_ITEM * ) ( (char*)table->elems + sizeElem * insertIndex );
		if( insertIndex < table->used )
			memmove( (char*)target + sizeElem, 
				(char*)target, 
				sizeElem * ( table->size - insertIndex - 1 ) );

		switch( MK_TYPE_ATTRIBUTE_HASH_KEYTYPE( table->flags ) )
		{
		case MK_TYPE_ATTRIBUTE_HASHKEY_STRING:
			target->key = (MK_CHAR*)key;
			break;
		case MK_TYPE_ATTRIBUTE_HASHKEY_FREEABLE_STRING:
			mk_copy_string( &target->key, key );
			break;
		case MK_TYPE_ATTRIBUTE_HASHKEY_INTPTR:
		case MK_TYPE_ATTRIBUTE_HASHKEY_FREEABLE_PTR:
			target->intPtrKey = (INT_PTR)key;
			break;
		}
		table->used ++;
	}
	if( sizeValue > 0 )
	{
		MK_MAP_ITEM *target = 
			( MK_MAP_ITEM * ) ( (char*)table->elems + sizeElem * insertIndex );
		target->value[0] = value;
	}
	return ( void* )( INT_PTR )( insertIndex + 1 );
}

void* mk_is_key_hashtable( MK_HASHTABLE *table, const MK_CHAR *key )
{
	int ret = 0;

	switch( MK_TYPE_ATTRIBUTE_HASH_KEYTYPE( table->flags ) )
	{
	case MK_TYPE_ATTRIBUTE_HASHKEY_STRING:
	case MK_TYPE_ATTRIBUTE_HASHKEY_FREEABLE_STRING:
		ret = mk_find_item_position_string( table, key );
		break;
	case MK_TYPE_ATTRIBUTE_HASHKEY_INTPTR:
	case MK_TYPE_ATTRIBUTE_HASHKEY_FREEABLE_PTR:
		ret = mk_find_item_position_ptr( table, (const INT_PTR)key );
		break;
	}
	return ( ret >= 0 ) ? (void*)( INT_PTR )( ret + 1 ) : NULL;
}

static
void mk_remove_key_hashtable_string( MK_HASHTABLE *table, const MK_CHAR *key )
{
 	int index = mk_find_item_position_string( table, key );
	int sizeValue = 
		MK_TYPE_ATTRIBUTE_HASH_SIZE_EXTEND_VALUE(table->flags);
	size_t sizeElem = 
		( sizeof(MK_MAP_ITEM) + sizeof(void*) * sizeValue );
	if( index >= 0 )
	{
		memmove( 
			(char*)table->elems + sizeElem * index, 
			(char*)table->elems + sizeElem * ( index + 1 ), 
			sizeElem * ( table->used - index - 1 ) );
		table->used --;
	}
}

static
void mk_remove_key_hashtable_intptr( MK_HASHTABLE *table, const INT_PTR key )
{
	int index = mk_find_item_position_ptr( table, key );
	int sizeValue = 
		MK_TYPE_ATTRIBUTE_HASH_SIZE_EXTEND_VALUE(table->flags);
	size_t sizeElem = 
		( sizeof(MK_MAP_ITEM) + sizeof(void*) * sizeValue );
	if( index >= 0 )
	{
		memmove( 
			(char*)table->elems + sizeElem * index, 
			(char*)table->elems + sizeElem * ( index + 1 ), 
			sizeElem * ( table->used - index - 1 ) );
		table->used --;
	}
}

void mk_remove_key_hashtable( MK_HASHTABLE *table, const MK_CHAR *key )
{
	switch( MK_TYPE_ATTRIBUTE_HASH_KEYTYPE( table->flags ) )
	{
	case MK_TYPE_ATTRIBUTE_HASHKEY_STRING:
		mk_remove_key_hashtable_string( table, key );
		break;
	case MK_TYPE_ATTRIBUTE_HASHKEY_FREEABLE_STRING:
		mk_remove_key_hashtable_string( table, key );
		free( (char*)key );
		break;
	case MK_TYPE_ATTRIBUTE_HASHKEY_INTPTR:
		mk_remove_key_hashtable_intptr( table, (const INT_PTR)key );
		break;
	case MK_TYPE_ATTRIBUTE_HASHKEY_FREEABLE_PTR:
		mk_remove_key_hashtable_intptr( table, (const INT_PTR)key );
		free( (void*)key );
		break;
	}
}

void* mk_find_item_pointer_hashtable( MK_HASHTABLE *table, const MK_CHAR *key, void **value )
{
	int ret = 0;
	MK_MAP_ITEM *target = NULL;
	int sizeValue = 
		MK_TYPE_ATTRIBUTE_HASH_SIZE_EXTEND_VALUE(table->flags);
	size_t sizeElem = 
		( sizeof(MK_MAP_ITEM) + sizeof(void*) * sizeValue );

	switch( MK_TYPE_ATTRIBUTE_HASH_KEYTYPE( table->flags ) )
	{
	case MK_TYPE_ATTRIBUTE_HASHKEY_STRING:
	case MK_TYPE_ATTRIBUTE_HASHKEY_FREEABLE_STRING:
		ret = mk_find_item_position_string( table, key );
		break;
	case MK_TYPE_ATTRIBUTE_HASHKEY_INTPTR:
	case MK_TYPE_ATTRIBUTE_HASHKEY_FREEABLE_PTR:
		ret = mk_find_item_position_ptr( table, (const INT_PTR)key );
		break;
	}

	if( ret < 0 || sizeValue == 0 )
	{
		*value = NULL;
	}
	else
	{
		target = 
			( MK_MAP_ITEM * ) ( (char*)table->elems + sizeElem * ret );
		*value = &( target->value[0] );
	}
	return ( ret >= 0 ) ? ( void* )( INT_PTR )( ret + 1 ) : NULL;
}

void* mk_find_item_hashtable( MK_HASHTABLE *table, const MK_CHAR *key, void **value )
{
	void *result = mk_find_item_pointer_hashtable(table, key, value);
	if (*value != NULL)
		*value = *((void**)*value);
	return result;
}

void mk_set_extend_value_hashtable( MK_HASHTABLE *table, void *handle, int index, INT_PTR newValue )
{
	int sizeValue = 
		MK_TYPE_ATTRIBUTE_HASH_SIZE_EXTEND_VALUE(table->flags);
	size_t sizeElem = 
		( sizeof(MK_MAP_ITEM) + sizeof(void*) * sizeValue );
	MK_MAP_ITEM *targetElem = 
		( MK_MAP_ITEM * )( (char*)(table->elems) + sizeElem * ( (INT_PTR)handle - 1 ) );
	if( index >= 0 && index < sizeValue )
		targetElem->value[index] = (void*)newValue;
}

INT_PTR mk_get_extend_value_hashtable( MK_HASHTABLE *table, void *handle, int index )
{
	int sizeValue = 
		MK_TYPE_ATTRIBUTE_HASH_SIZE_EXTEND_VALUE(table->flags);
	size_t sizeElem = 
		( sizeof(MK_MAP_ITEM) + sizeof(void*) * sizeValue );
	MK_MAP_ITEM *targetElem = 
		( MK_MAP_ITEM * )( (char*)(table->elems) + sizeElem * ( (INT_PTR)handle - 1 ) );
	if( index >= 0 && index < sizeValue )
		return (INT_PTR)targetElem->value[index];
	else
		return 0;
}

void* mk_enum_item_hashtable_begin( MK_HASHTABLE *table )
{
	if( table->used == 0 )
		return NULL;
	else
		return (void*)( INT_PTR )1;
}

void* mk_enum_item_hashtable_next( MK_HASHTABLE *table, void *iterator, const MK_CHAR **key, void **value )
{
	INT_PTR index = 
		( INT_PTR )iterator - 1;
	int sizeValue = 
		MK_TYPE_ATTRIBUTE_HASH_SIZE_EXTEND_VALUE(table->flags);
	size_t sizeElem = 
		( sizeof(MK_MAP_ITEM) + sizeof(void*) * sizeValue );
	MK_MAP_ITEM *target = NULL;

	if( index >= table->used )
		return NULL;
	target = 
		( MK_MAP_ITEM * ) ( (char*)table->elems + sizeElem * index );

	*key = target->key;
	*value = target->value[0];
	return index + 1 < table->used ? ( void * )( INT_PTR )( index + 2 ) : NULL;
}

static
int mk_find_item_position_ptr( MK_HASHTABLE *table, const INT_PTR key )
{
	int high = table->used - 1, 
		low = 0, 
		mid = 0;
	int ret = 0;
	int sizeValue = 
		MK_TYPE_ATTRIBUTE_HASH_SIZE_EXTEND_VALUE(table->flags);
	size_t sizeElem = 
		( sizeof(MK_MAP_ITEM) + sizeof(void*) * sizeValue );
	MK_MAP_ITEM *elems = 
		table->elems;

	while( low <= high )
	{
		int target = 0;
		MK_MAP_ITEM *elemTarget = NULL;
		mid = ( low + high ) / 2;
		elemTarget = 
			( MK_MAP_ITEM* )( (char*)elems + mid * sizeElem );
		target = elemTarget->intPtrKey;
		if( target == key )
		{
			return mid;
		}
		else if(target < key )
		{
			low = mid + 1;
		}
		else
		{
			high = mid - 1;
		}
	}
	return -1;
}

static
int mk_find_item_position_string( MK_HASHTABLE *table, const MK_CHAR *key )
{
	int high = table->used - 1, 
		low = 0, 
		mid = 0;
	int ret = 0;
	int sizeValue = 
		MK_TYPE_ATTRIBUTE_HASH_SIZE_EXTEND_VALUE(table->flags);
	size_t sizeElem = 
		( sizeof(MK_MAP_ITEM) + sizeof(void*) * sizeValue );
	MK_MAP_ITEM *elems = 
		table->elems;

	while( low <= high )
	{
		int ret = 0;
		MK_MAP_ITEM *elemTarget = NULL;
		mid = ( low + high ) / 2;
		elemTarget = 
			( MK_MAP_ITEM* )( (char*)elems + mid * sizeElem );
		ret = strcmp( elemTarget->key, key );
		if( ret == 0 )
		{
			return mid;
		}
		else if( ret < 0 )
		{
			low = mid + 1;
		}
		else
		{
			high = mid - 1;
		}
	}
	return -1;
}

static
int mk_seek_insert_position_ptr( MK_HASHTABLE *table, const INT_PTR key, int *isKey )
{
	int high = table->used - 1, 
		low = 0, 
		mid = 0;
	int ret = 0;
	int sizeValue = 
		MK_TYPE_ATTRIBUTE_HASH_SIZE_EXTEND_VALUE(table->flags);
	size_t sizeElem = 
		( sizeof(MK_MAP_ITEM) + sizeof(void*) * sizeValue );
	MK_MAP_ITEM *elems = 
		table->elems;

	while( low <= high )
	{
		int target = 0;
		MK_MAP_ITEM *elemTarget = NULL;
		mid = ( low + high ) / 2;
		elemTarget = 
			( MK_MAP_ITEM* )( (char*)elems + mid * sizeElem );
		target = elemTarget->intPtrKey;
		if( target == key )
		{
			*isKey = 1;
			return mid;
		}
		else if(target < key )
		{
			low = mid + 1;
		}
		else
		{
			high = mid - 1;
		}
	}
	*isKey = 0;
	return low;
}

static
int mk_seek_insert_position_string( MK_HASHTABLE *table, const MK_CHAR *key, int *isKey )
{
	int high = table->used - 1, 
		low = 0, 
		mid = 0;
	int ret = 0;
	int sizeValue = 
		MK_TYPE_ATTRIBUTE_HASH_SIZE_EXTEND_VALUE(table->flags);
	size_t sizeElem = 
		( sizeof(MK_MAP_ITEM) + sizeof(void*) * sizeValue );
	MK_MAP_ITEM *elems = 
		table->elems;

	while( low <= high )
	{
		int ret = 0;
		MK_MAP_ITEM *elemTarget = NULL;
		mid = ( low + high ) / 2;
		elemTarget = 
			( MK_MAP_ITEM* )( (char*)elems + mid * sizeElem );
		ret = strcmp( elemTarget->key, key );
		if( ret == 0 )
		{
			*isKey = 1;
			return mid;
		}
		else if( ret < 0 )
		{
			low = mid + 1;
		}
		else
		{
			high = mid - 1;
		}
	}
	*isKey = 0;
	return low;	// key not found.
}

static
void mk_extend_hashtable( MK_HASHTABLE *table )
{
	int sizeValue = 
		MK_TYPE_ATTRIBUTE_HASH_SIZE_EXTEND_VALUE(table->flags);
	size_t sizeElem = 
		( sizeof(MK_MAP_ITEM) + sizeof(void*) * sizeValue );
	MK_MAP_ITEM *elems = 
		table->elems;
	MK_MAP_ITEM *newMapItem = NULL, *oldMapItem = NULL;
	unsigned int extend = MK_TYPE_ATTRIBUTE_HASH_SIZE_GROW_VALUE(table->flags);

	newMapItem = 
		malloc( sizeElem * ( table->size + extend ) );
	memcpy( newMapItem,
		table->elems,
		sizeElem * table->size );
	free( table->elems );
	table->elems = newMapItem;
	table->size += extend;
}
