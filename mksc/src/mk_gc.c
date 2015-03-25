#include <memory.h>
#ifdef _DEBUG
#include <stdio.h>
#endif
#include "mk.h"

static
 void mk_gc_scan_vector( MK_VECTOR *target, int isSearchTop );
static
void mk_gc_scan_hashtable( MK_HASHTABLE *target, int isSearchTop );
static 
void mk_gc_scan_vm_frame_item( MK_VM_FRAME_ITEM *target );
static
void mk_gc_scan_vm_frame( MK_VM_FRAME *target );
static
void mk_gc_sweep_frame_item_table( MK_VM_STRUCT *vm );
static
void mk_gc_sweep_vector_table( MK_VM_STRUCT *vm );
static
void mk_gc_sweep_hashtable_table( MK_VM_STRUCT *vm );
static
void mk_gc_sweep_frame_table( MK_VM_STRUCT *vm );
static
MK_VM_FRAME_ITEM *mk_gc_get_vm_frame_item_top( MK_VM_FRAME_ITEM *target);

// gc mark & sweep implements.

int mk_gc_run( MK_VM_STRUCT *vm, int force )
{
	MK_VM_FRAME *pFrame = NULL;

	MK_MANAGED_VM_FRAME_ITEM_TABLE *pCurrentFrameTable = vm->pFrameItemTable;
	MK_MANAGED_HASH_TABLE *pCurrentHash = vm->pHashTable;

#ifdef _DEBUG_DUMP
	fprintf( stdout, "mk_gc_run\n" );
#endif

	if( vm->gcPhase == GC_EXECUTE_TIME_PER_CALL || force != 0 )
	{
		int index = 0;
		MK_VM_FRAME_ITEM **current = vm->localStack.start;
		// scan local stack
		while( current != vm->localStack.current )
		{
			mk_gc_scan_vm_frame_item( *current );
			current ++;
		}

		// exception object
		mk_gc_scan_vm_frame_item( vm->exceptionObject );

		// scan global
		mk_gc_scan_hashtable( vm->global, 1 );

		// scan vm_frame
		pFrame = vm->pCurrentFrame;
		mk_gc_scan_vm_frame( pFrame );

		// sweep
		// todo: optimize gc timing.
		mk_gc_sweep_vector_table( vm );
		mk_gc_sweep_hashtable_table( vm );
		mk_gc_sweep_frame_item_table( vm );
		mk_gc_sweep_frame_table( vm );
		vm->gcPhase = 0;
	}
	return 0;
}

static
MK_VM_FRAME_ITEM *mk_gc_get_vm_frame_item_top( MK_VM_FRAME_ITEM *target)
{
	if( target != NULL )
	{
		while( ( ( (INT_PTR)target & MK_VM_FRAME_ITEM_TYPE_DIRECT_VALUE ) == 0 ) &&
			( MK_TYPE_ATTRIBUTE( target->flags ) == MK_VM_FRAME_ITEM_TYPE_CLASS ) &&
			( target->classTypeValue.child != NULL ) )
		{
			if( ( (INT_PTR)target->classTypeValue.child & MK_VM_FRAME_ITEM_TYPE_DIRECT_VALUE ) == 0 )
				target = target->classTypeValue.child;
			else
				break;
		}
	}
	return target;
}

static
void mk_gc_scan_vm_frame( MK_VM_FRAME *target )
{
	while( target )
	{
		if( MK_GC_IS_SET_GC_BIT( target->flags ) == 0 )
		{
			// set gc bit
			MK_GC_SET_GC_BIT( target->flags );

			// pLastResultBlock
			mk_gc_scan_vm_frame_item( target->pLastResultBlock );

			// pThis
			mk_gc_scan_vm_frame_item( target->pThis );

			// local variables
			mk_gc_scan_hashtable( target->localVariables, 1 );

			// owner frame
			mk_gc_scan_vm_frame( target->pOwnerFrame );
		}
		target = target->previous;
	}
}

static
void mk_gc_scan_vector( MK_VECTOR *target, int isSearchTop )
{
	if( target != NULL && MK_GC_IS_SET_GC_BIT(target->flags) == 0 )
	{
		unsigned int index = 0;
		unsigned int sizeVector = mk_size_vector( target );
		MK_GC_SET_GC_BIT( target->flags );
		if( MK_TYPE_ATTRIBUTE_VECTOR_STYLE( target->flags ) ==
			MK_TYPE_VECTOR_ARRAY_MANAGED_PTR )
		{
			for( index = 0; index < sizeVector; index ++ )
			{
				MK_VM_FRAME_ITEM *frameItem = 
					(MK_VM_FRAME_ITEM *)mk_get_at_vector( target, index );
				if( isSearchTop != 0 )
					frameItem = mk_gc_get_vm_frame_item_top( frameItem );
				mk_gc_scan_vm_frame_item( frameItem );
			}
		}
	}
}

static
void mk_gc_scan_hashtable( MK_HASHTABLE *target, int isSearchTop )
{
	if( target != NULL && MK_GC_IS_SET_GC_BIT(target->flags) == 0 )
	{
		void *position = 
			mk_enum_item_hashtable_begin( target );
		MK_GC_SET_GC_BIT( target->flags );
		while( position != NULL )
		{
			const MK_CHAR *key = NULL;
			MK_VM_FRAME_ITEM *value = NULL;
			position = mk_enum_item_hashtable_next( target, position, &key, (void**)&value );
			if( isSearchTop != 0 )
				value = mk_gc_get_vm_frame_item_top( value );
			mk_gc_scan_vm_frame_item( value );
		}
	}
}

static
void mk_gc_scan_vm_frame_item( MK_VM_FRAME_ITEM *target )
{
	if( target != NULL )
	{
		if ( ( (INT_PTR)target & MK_VM_FRAME_ITEM_TYPE_DIRECT_VALUE ) == 0 )
		{
			if ( ( (INT_PTR)target & MK_VM_FRAME_ITEM_TYPE_DIRECT_POINTER ) == 0 )
			{
				if( !MK_GC_IS_SET_GC_BIT( target->flags ) )
				{
					if( MK_TYPE_ATTRIBUTE( target->flags ) == MK_VM_FRAME_ITEM_TYPE_CLASS ||
						MK_TYPE_ATTRIBUTE( target->flags ) == MK_VM_FRAME_ITEM_TYPE_MODULE )
					{
						mk_gc_scan_hashtable( target->classTypeValue.variables, 0 );	// child class
						mk_gc_scan_vm_frame_item( target->classTypeValue.child );	// parent class
					}
					else if( MK_TYPE_ATTRIBUTE( target->flags ) == MK_VM_FRAME_ITEM_TYPE_ARRAY_VALUE )
					{
						mk_gc_scan_vector( target->arrayTypeValue, 1 );
					}
					else if( MK_TYPE_ATTRIBUTE( target->flags ) == MK_VM_FRAME_ITEM_TYPE_NODE_VALUE )
					{
						mk_gc_scan_vm_frame( target->code.definedFrame );
						mk_gc_scan_vm_frame_item( target->code.pOwner );
					}
					else if( MK_TYPE_ATTRIBUTE( target->flags ) == MK_VM_FRAME_ITEM_TYPE_REFERENCE_VALUE )
					{
						mk_gc_scan_hashtable( target->referenceTypeValue.target, 1 );
					}
					else if( MK_TYPE_ATTRIBUTE( target->flags ) == MK_VM_FRAME_ITEM_TYPE_REFERENCE_ARRAY_VALUE )
					{
						mk_gc_scan_vector( target->referenceArrayTypeValue.target, 1 );
					}
					else if( MK_TYPE_ATTRIBUTE( target->flags ) == MK_VM_FRAME_ITEM_TYPE_REFERENCE_STRING_VALUE )
					{
						mk_gc_scan_vm_frame_item( target->referenceStringTypeValue.target );
					}
					MK_GC_SET_GC_BIT( target->flags );
				}
			}
			else
			{
				target = 
					*( MK_VM_FRAME_ITEM ** )( (INT_PTR)(target) & ~MK_VM_FRAME_ITEM_TYPE_DIRECT_POINTER );
				mk_gc_scan_vm_frame_item( target );
			}
		}
	}
}

static
void mk_gc_sweep_vector_table( MK_VM_STRUCT *vm )
{
	MK_MANAGED_VECTOR_TABLE *vectorTables = vm->pVectorTable;
	while( vectorTables != NULL )
	{
		int count = 0;
		int countRemove = 0;
		MK_VECTOR *pItem = vectorTables->vectors;
		MK_VECTOR *pLast = vectorTables->vectors + MK_SIZEOF_MANAGED_TABLE;
		while( pItem != pLast )
		{
			if( MK_GC_IS_ALLOCATED_OBJECT( pItem->flags ) )
			{
				if( !MK_GC_IS_SET_GC_BIT(pItem->flags ) && !MK_GC_IS_SET_CN_BIT(pItem->flags ) )
				{
					free( pItem->items );
					pItem->items = NULL;
					pItem->size = 0;
					pItem->flags = 0;
					pItem->used = 0;
					countRemove ++;
				}
				else
				{
					MK_GC_CLEAR_GC_BIT( pItem->flags );
					MK_GC_CLEAR_CN_BIT( pItem->flags );
				}
			}
			pItem ++;
		}
		vectorTables->freeSpace += countRemove;
		vectorTables = vectorTables->previous;
	}
}

static
void mk_gc_sweep_hashtable_table( MK_VM_STRUCT *vm )
{
	MK_MANAGED_HASH_TABLE *hashTables = vm->pHashTable;
	while( hashTables != NULL )
	{
		int count = 0;
		MK_HASHTABLE *pItem = hashTables->hashTables;
		MK_HASHTABLE *pLast = hashTables->hashTables + MK_SIZEOF_MANAGED_TABLE;
		while( pItem != pLast )
		{
			if( MK_GC_IS_ALLOCATED_OBJECT( pItem->flags ) )
			{
				if( !MK_GC_IS_SET_GC_BIT(pItem->flags ) && !MK_GC_IS_SET_CN_BIT(pItem->flags ) )
				{
					MK_GC_FREE_OBJECT( pItem->flags );
					pItem->used = 0;
					hashTables->freeSpace ++;
				}
				else
				{
					MK_GC_CLEAR_GC_BIT( pItem->flags );
					MK_GC_CLEAR_CN_BIT( pItem->flags );
				}
			}
			pItem ++;
		}
		hashTables = hashTables->previous;
	}
}

static
void mk_gc_sweep_frame_item_table( MK_VM_STRUCT *vm )
{
	MK_MANAGED_VM_FRAME_ITEM_TABLE *frameItemTables = vm->pFrameItemTable;
	while( frameItemTables != NULL )
	{
		MK_VM_FRAME_ITEM *pItem = frameItemTables->table;
		const MK_VM_FRAME_ITEM *pLast = frameItemTables->table + MK_SIZEOF_MANAGED_TABLE;
		while( pItem != pLast )
		{
			if( MK_GC_IS_ALLOCATED_OBJECT( pItem->flags ) )
			{
				if( !MK_GC_IS_SET_GC_BIT(pItem->flags) && !MK_GC_IS_SET_CN_BIT(pItem->flags) )
				{
					if( MK_TYPE_ATTRIBUTE( pItem->flags ) == MK_VM_FRAME_ITEM_TYPE_STRING_VALUE )
						free( pItem->stringTypeValue );
					pItem->flags = 0;
					frameItemTables->freeSpace ++;
				}
				else
				{
					MK_GC_CLEAR_GC_BIT( pItem->flags );
					MK_GC_CLEAR_CN_BIT( pItem->flags );
				}
			}
			pItem ++;
		}
		frameItemTables = frameItemTables->previous;
	}
}

static
void mk_gc_sweep_frame_table( MK_VM_STRUCT *vm )
{
	MK_MANAGED_VM_FRAME_TABLE *pFrameTable = vm->pFrameTable;
	while( pFrameTable != NULL )
	{
		int countRemove = 0;
		MK_VM_FRAME *pFrame = pFrameTable->table;
		const MK_VM_FRAME *pLast = pFrameTable->table + MK_SIZEOF_MANAGED_TABLE;
		while( pFrame != pLast )
		{
			if( MK_GC_IS_ALLOCATED_OBJECT( pFrame->flags ) )
			{
				if( !MK_GC_IS_SET_GC_BIT(pFrame->flags) && !MK_GC_IS_SET_CN_BIT(pFrame->flags) )
				{
					pFrame->flags = 0;
					countRemove ++;
				}
				else
				{
					MK_GC_CLEAR_GC_BIT( pFrame->flags );
					MK_GC_CLEAR_CN_BIT( pFrame->flags );
				}
			}
			pFrame ++;
		}
		pFrameTable->freeSpace += countRemove;
		pFrameTable = pFrameTable->previous;
	}
}
