#include "../include/mk.h"
#include <stdio.h>

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
int mk_int_small( struct TAG_MK_VM_STRUCT *vm, unsigned int varArgC )
{
	MK_VM_FRAME_ITEM *right = mk_vm_pop_stack( &vm->localStack );
	MK_VM_FRAME_ITEM *target = mk_vm_pop_stack( &vm->localStack );

	mk_vm_push_stack( &vm->localStack, 
		mk_vm_create_bool_frame_item( 
			mk_vm_frame_item_to_int32( vm, target ) < mk_vm_frame_item_to_int32( vm, right ) ) );

	return MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP;
}


int fib( MK_VM_STRUCT *vm )
{
	MK_VM_FRAME_ITEM *value = mk_vm_pop_stack( &vm->localStack );
	int intValue = mk_vm_frame_item_to_int32( vm, value );
	mk_vm_push_stack( &vm->localStack, value );
	mk_vm_push_stack( &vm->localStack, mk_vm_create_int32_frame_item( vm, 3 ) );
	mk_int_small( vm, 0 );

	if( mk_vm_frame_item_to_int32( vm, mk_vm_pop_stack( &vm->localStack ) ) != 0 )
	{
		mk_vm_push_stack( &vm->localStack, mk_vm_create_int32_frame_item( vm, 1 ) );
	}
	else
	{
		int newValue = 0;
		mk_vm_push_stack( &vm->localStack, value );
		mk_vm_push_stack( &vm->localStack, mk_vm_create_int32_frame_item( vm, 1 ) );
		mk_int_minus( vm, 0 );
		fib( vm );

		mk_vm_push_stack( &vm->localStack, value );
		mk_vm_push_stack( &vm->localStack, mk_vm_create_int32_frame_item( vm, 2 ) );
		mk_int_minus( vm, 0 );
		fib( vm );

		mk_int_plus( vm, 0 );
	}
	return 0;
}

int fib_native( )
{
	MK_VM_STRUCT *vm = 
		mk_create_object( MK_OBJECT_TYPE_VM_STRUCT );
	mk_vm_initialize( vm );

	mk_register_internal_classes( vm );


	mk_vm_push_stack( &vm->localStack, mk_vm_create_int32_frame_item( vm, 36 ) );
	fib( vm );
	printf( "%d", mk_vm_frame_item_to_int32( vm, mk_vm_pop_stack( &vm->localStack ) ) );
	
	mk_destroy_node( vm->memoryPool, vm );
	return 0;
}

int main( )
{
	fib_native( );
	return 0;
}
