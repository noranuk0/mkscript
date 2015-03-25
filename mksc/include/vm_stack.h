#include "mk.h"
#include <memory.h>
#include <stdio.h>

INLINE
int mk_execute_expr( MK_VM_STRUCT *vm, MK_NODE_EXPR *pTarget, int hasParent, int reference )
{
	vm->step ++;
#ifdef _DEBUG
	fprintf(stdout, "[EXPR:0x%08x - command:%08x\n", vm->step, pTarget->flags);
#endif
	return ( (abs( (INT_PTR)((vm)->stackTop) - (INT_PTR)&(vm) ) < MK_SIZEOF_MAX_STACK_SIZE ) ?
		vm->callNodeExpr[MK_TYPE_ATTRIBUTE_INDEX((pTarget)->flags)]((vm), (pTarget), (hasParent), (reference ) ) :
		mk_raise_internal_error( vm, "", 0, MK_ERROR_TYPE_VM_ERROR | 9, NULL ) );
}

INLINE
void mk_vm_create_stack( MK_VM_STACK *stack )
{
	stack->start = 
		malloc( sizeof( INT_PTR ) * MK_SIZEOF_VM_STACK );
	memset( stack->start, 0x00, sizeof( INT_PTR ) * MK_SIZEOF_VM_STACK );
	stack->current = 
		stack->start;
	stack->last = 
		stack->start + MK_SIZEOF_VM_STACK;
}

INLINE
void mk_vm_extend_stack( MK_VM_STACK *stack )
{
	unsigned int sizeCurrent = 
		stack->last - stack->start;
	MK_VM_FRAME_ITEM **newStack = 
		malloc( sizeof(MK_VM_FRAME_ITEM *) * ( sizeCurrent + MK_SIZEOF_EXTEND_STACK ) );
	memcpy( newStack, stack->start, sizeCurrent * sizeof(MK_VM_FRAME_ITEM *) );
	free( stack->start );
	stack->current = newStack + sizeCurrent;
	stack->last = newStack + ( sizeCurrent + MK_SIZEOF_EXTEND_STACK );
	stack->start = newStack;
}

INLINE
void mk_vm_push_stack( MK_VM_STACK *stack , MK_VM_FRAME_ITEM *value )
{
	if( stack->current == stack->last )
		mk_vm_extend_stack( stack );
	(*stack->current) = value;
	stack->current ++;
#ifdef _DEBUG_DUMP
	fprintf( stdout, "PUSH %d:<%08x>", stack->current - stack->start, (INT_PTR)value );
	dump_node( value, 0 );
#endif
}

INLINE
MK_VM_FRAME_ITEM *mk_vm_pop_stack( MK_VM_STACK *stack )
{
	MK_VM_FRAME_ITEM *result = NULL;
	stack->current --;
	result = *(stack->current);
	if( ( (INT_PTR)result & ( MK_VM_FRAME_ITEM_TYPE_DIRECT_VALUE | MK_VM_FRAME_ITEM_TYPE_REFERENCE_VALUE ) ) == MK_VM_FRAME_ITEM_TYPE_REFERENCE_VALUE )
		result =
			*( MK_VM_FRAME_ITEM ** )( (INT_PTR)result & ~MK_VM_FRAME_ITEM_TYPE_REFERENCE_VALUE );

#ifdef _DEBUG_DUMP
	fprintf( stdout, "POP %d:<%08x>", stack->current - stack->start, (INT_PTR)result );
	dump_node( result, 0 );
#endif
	return result;
}

INLINE
MK_VM_FRAME_ITEM *mk_vm_get_at_stack( MK_VM_STACK *stack, unsigned int position /* 0:current */ )
{
	MK_VM_FRAME_ITEM *result = *( stack->current - ( position + 1 ) );
	if( ( (INT_PTR)result & ( MK_VM_FRAME_ITEM_TYPE_DIRECT_VALUE | MK_VM_FRAME_ITEM_TYPE_REFERENCE_VALUE ) ) == MK_VM_FRAME_ITEM_TYPE_REFERENCE_VALUE )
		result =
			*( MK_VM_FRAME_ITEM ** )( (INT_PTR)result & ~MK_VM_FRAME_ITEM_TYPE_REFERENCE_VALUE );
	return result;
}

INLINE
void mk_vm_set_at_stack( MK_VM_STACK *stack, unsigned int position /* 0:current */,MK_VM_FRAME_ITEM *newValue )
{
	*( stack->current - ( position + 1 ) ) = newValue;
}

INLINE
void mk_vm_trim_stack( MK_VM_STACK *stack, unsigned int sizeTrim )
{
	stack->current -= sizeTrim;
}

INLINE
unsigned int mk_vm_stack_get_current( MK_VM_STACK *stack )
{
	return stack->current - stack->start;
}

INLINE
void mk_vm_stack_set_current( MK_VM_STACK *stack, unsigned int current )
{
	stack->current = stack->start + current;
}
