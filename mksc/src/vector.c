#include "mk.h"
#include <stdlib.h>
#include <memory.h>
#include <string.h>

static
unsigned int mk_get_sizeof_vector_item( int flag )
{
	int realSize = 0;
	switch( MK_TYPE_ATTRIBUTE_VECTOR_STYLE(flag) )
	{
	case MK_TYPE_VECTOR_ARRAY_MANAGED_PTR:
	case MK_TYPE_VECTOR_ARRAY_UNMANAGED_PTR:
		realSize = sizeof(INT_PTR);
		break;
	case MK_TYPE_VECTOR_ARRAY_INT8:
		realSize = 1;
		break;
	case MK_TYPE_VECTOR_ARRAY_INT16:
		realSize = 2;
		break;
	case MK_TYPE_VECTOR_ARRAY_INT32:
		realSize = 4;
		break;
	case MK_TYPE_VECTOR_ARRAY_INT64:
		realSize = 8;
		break;
	}
	return realSize;
}

static
int mk_initialize_vector( MK_VECTOR *target, 
						 unsigned int flag, unsigned int defaultSize, INT_PTR addressOfMemory )
{
	target->flags = 
		target->flags | flag;

	target->size = defaultSize;
	if( flag & MK_TYPE_VECTOR_POINT_MEMORY_DIRECT )
	{
		target->items = (INT_PTR*)addressOfMemory;
		target->used = defaultSize;
	}
	else
	{
		int realSize = 
			mk_get_sizeof_vector_item( flag ) * defaultSize;
		target->items = malloc( realSize );
		memset( target->items, 0x00, realSize );
		target->used = 0;
	}
	return 1;
}

MK_VECTOR *mk_create_vector( unsigned int flag, unsigned int defaultSize, INT_PTR addressOfMemory )
{
	MK_VECTOR *result = 
		malloc( sizeof( MK_VECTOR ) );
	result->flags = MK_TYPE_VECTOR;
	if( mk_initialize_vector( result, flag, defaultSize, addressOfMemory ) )
	{
		return result;
	}
	else
	{
		free( result );
		return NULL;
	}
}

MK_VECTOR *mk_allocate_vm_managed_vector( MK_MANAGED_VECTOR_TABLE **managedVectorTable, 
										 unsigned int flag, unsigned int defaultSize, INT_PTR addressOfMemory )
{
	MK_MANAGED_VECTOR_TABLE *topVectorTable = NULL;
	MK_VECTOR *pCurrent = NULL;

	topVectorTable = *managedVectorTable;
	while( topVectorTable != NULL )
	{
		if( topVectorTable->freeSpace > 0 )
			break;
		topVectorTable = topVectorTable->previous;
	}
	if( topVectorTable == NULL )
	{
		topVectorTable = ( MK_MANAGED_VECTOR_TABLE * )malloc( sizeof( MK_MANAGED_VECTOR_TABLE ) );
		memset( topVectorTable, 0x00, sizeof( MK_MANAGED_VECTOR_TABLE ) );
		topVectorTable->freeSpace = MK_SIZEOF_MANAGED_TABLE;
		topVectorTable->nextIndex = 0;
		topVectorTable->previous = *managedVectorTable;
		*managedVectorTable = topVectorTable;
	}
	pCurrent = topVectorTable->vectors + topVectorTable->nextIndex;
	while( pCurrent < topVectorTable->vectors + MK_SIZEOF_MANAGED_TABLE )
	{
		if( MK_GC_IS_ALLOCATED_OBJECT( pCurrent->flags ) == 0 )
			break;
		pCurrent ++;
	}
	if( pCurrent >= topVectorTable->vectors + MK_SIZEOF_MANAGED_TABLE )
	{
		pCurrent = topVectorTable->vectors;
		while( pCurrent != topVectorTable->vectors + topVectorTable->nextIndex )
		{
			if( MK_GC_IS_ALLOCATED_OBJECT( pCurrent->flags ) == 0 )
				break;
			pCurrent ++;
		}
	}
	pCurrent->flags = MK_TYPE_VECTOR;
	mk_initialize_vector( pCurrent, flag, defaultSize, addressOfMemory );
	MK_GC_SET_MANAGED_BIT( pCurrent->flags );
	MK_GC_ALLOCATE_OBJECT( pCurrent->flags );
	MK_GC_SET_CN_BIT( pCurrent->flags );
	return pCurrent;
}

unsigned int mk_insert_at_vector( MK_VECTOR *target, unsigned int index, INT_PTR elem )
{
	unsigned int itemSize = 
		mk_get_sizeof_vector_item(target->flags);

	if( index > target->used )
		index = target->used ;

	if( target->size == target->used )
	{
		if( target->flags & MK_TYPE_VECTOR_SIZE_FIX )	// do not extend.
		{
			return UINT_MAX;
		}
		else
		{
			unsigned int extend = 
				MK_TYPE_VECTOR_SIZE_EXTEND(target->flags);
			if( extend == 0 )
				return UINT_MAX;
			if( target->flags & MK_TYPE_VECTOR_POINT_MEMORY_DIRECT )
			{
				target->size += extend;
			}
			else
			{
				INT_PTR *newItems =
						malloc( itemSize * ( target->size + extend ) ), 
					*oldItems = NULL;
				memcpy( newItems, 
					target->items, 
					itemSize * ( (index < target->used) ? index : target->used ) );
				if( target->used > index )
					memcpy( &newItems[index+1], 
					&(target->items[index]),
					itemSize * (target->used-index) );
				oldItems = target->items;
				target->items = newItems;
				target->size += extend;
				free( oldItems );
			}
		}
	}

	if( target->used - index > 0 )
	{
		INT8 *items = 
			(INT8*)target->items;
		memmove( items + itemSize * (index+1 ), 
			items + itemSize * index, 
			itemSize * ( target->used-index ) );
	}
	target->used ++;
	switch( MK_TYPE_ATTRIBUTE_VECTOR_STYLE(target->flags) )
	{
	case MK_TYPE_VECTOR_ARRAY_MANAGED_PTR:
	case MK_TYPE_VECTOR_ARRAY_UNMANAGED_PTR:
		target->items[index] = elem;
		break;
	case MK_TYPE_VECTOR_ARRAY_INT8:
		target->items8[index] = (INT8)elem;
		break;
	case MK_TYPE_VECTOR_ARRAY_INT16:
		target->items16[index] = (INT16)elem;
		break;
	case MK_TYPE_VECTOR_ARRAY_INT32:
		target->items32[index] = (INT32)elem;
		break;
	case MK_TYPE_VECTOR_ARRAY_INT64:
		target->items64[index] = (INT64)elem;
		break;
	}
	return index;
}

INT_PTR mk_set_at_vector( MK_VECTOR *ptarget, unsigned int index, INT_PTR newItem )
{
	INT_PTR result = 0;

	unsigned int itemSize = 
		mk_get_sizeof_vector_item(ptarget->flags);
	INT_PTR old = (INT_PTR)NULL;

	if( ptarget->used <= index )
		return result;
	switch( MK_TYPE_ATTRIBUTE_VECTOR_STYLE(ptarget->flags) )
	{
	case MK_TYPE_VECTOR_ARRAY_MANAGED_PTR:
	case MK_TYPE_VECTOR_ARRAY_UNMANAGED_PTR:
		old = ptarget->items[index];
		ptarget->items[index] = newItem;
		break;
	case MK_TYPE_VECTOR_ARRAY_INT8:
		old = ptarget->items8[index];
		ptarget->items8[index] = (INT8)newItem;
		break;
	case MK_TYPE_VECTOR_ARRAY_INT16:
		old = ptarget->items16[index];
		ptarget->items16[index] = (INT16)newItem;
		break;
	case MK_TYPE_VECTOR_ARRAY_INT32:
		old = ptarget->items32[index];
		ptarget->items32[index] = (INT32)newItem;
		break;
	case MK_TYPE_VECTOR_ARRAY_INT64:
		old = (INT_PTR)ptarget->items64[index];
		ptarget->items64[index] = (INT64)newItem;
		break;
	}
	return old;
}

INT_PTR mk_get_at_vector( MK_VECTOR *ptarget, unsigned int index )
{
	INT_PTR result = 0;
	if( index < ptarget->used )
	{
		switch( MK_TYPE_ATTRIBUTE_VECTOR_STYLE(ptarget->flags) )
		{
		case MK_TYPE_VECTOR_ARRAY_MANAGED_PTR:
		case MK_TYPE_VECTOR_ARRAY_UNMANAGED_PTR:
			result = ptarget->items[index];
			break;
		case MK_TYPE_VECTOR_ARRAY_INT8:
			result= ptarget->items8[index];
			break;
		case MK_TYPE_VECTOR_ARRAY_INT16:
			result = ptarget->items16[index];
			break;
		case MK_TYPE_VECTOR_ARRAY_INT32:
			result = ptarget->items32[index];
			break;
		case MK_TYPE_VECTOR_ARRAY_INT64:
			result = (INT_PTR)ptarget->items64[index];
			break;
		}
	}
	return result;
}

int mk_grow_up_vector( MK_VECTOR *target, unsigned int newSize )
{
	int size = 
		mk_get_sizeof_vector_item( target->flags );
	if( target->flags & MK_TYPE_VECTOR_SIZE_FIX )
		return 0;		// cannot grow up

	if( target->used >= newSize )
		return 1;		// need not grow up

	if( target->size >= newSize )			
	{
		memset( target->items8 + size * target->used, 0x00, size * ( target->size - target->used ) );
		target->used = newSize;
	}
	else
	{
		void *newPtr = 
			malloc( size * newSize );
		memcpy( newPtr, target->items, size * target->used );
		target->items = (INT_PTR*)newPtr;
		memset( target->items8 + size * target->used, 0x00, size * ( newSize - target->used ) );
		target->size = newSize;
		target->used = newSize;
	}
	return 1;
}
