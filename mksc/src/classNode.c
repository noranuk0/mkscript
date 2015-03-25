#include "mk.h"
#include <stdlib.h>
#include <string.h>
#include <memory.h>

static const MK_CHAR className[] = CLASS_INTERNAL_NODE;
static const MK_CHAR classSuper[] = CLASS_INTERNAL_OBJECT;

static
int mk_node_initialize( struct TAG_MK_VM_STRUCT *vm, unsigned int varArgC )
{
	MK_VM_FRAME_ITEM *node = 
		mk_vm_pop_stack( &vm->localStack );
	MK_VM_FRAME_ITEM *target = 
		mk_vm_pop_stack( &vm->localStack );
	if( MK_TYPE_ATTRIBUTE(target->flags) != MK_VM_FRAME_ITEM_TYPE_NODE_VALUE )
		target = 
			mk_vm_find_instance( vm, 
				target, 
				vm->cache->internalClassSymbolName[MK_VM_FRAME_ITEM_TYPE_NODE_VALUE>>24] );

	if( ( MK_TYPE_ATTRIBUTE( node->flags ) == MK_VM_FRAME_ITEM_TYPE_NODE_VALUE ) )
		target->code = node->code;
	else
		return mk_raise_internal_error( vm, "", 0, MK_ERROR_TYPE_VM_ERROR | 14, "<parameter>", NULL );
	mk_vm_push_stack( &vm->localStack, target );
	return MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP;
}

int mk_node_invoke( struct TAG_MK_VM_STRUCT *vm, unsigned int varArgC )
{
	MK_VM_FRAME_ITEM **param = 
		malloc( sizeof( MK_VM_FRAME_ITEM * ) * varArgC );
	MK_VM_FRAME_ITEM *target = NULL; 
	MK_VARIABLE *nodeWithParam = NULL;
	unsigned int*node = NULL;
	int index = 0;
	int retCode = MK_VM_EXECUTE_EXPR_THROW;
	int sizeParam = varArgC;
	MK_VM_FRAME *newFrame = NULL, *oldFrame = NULL;

	for( index = sizeParam - 1; index >= 0; index -- )
		param[index] = mk_vm_pop_stack( &vm->localStack );
	target = 
		mk_vm_pop_stack( &vm->localStack );

	if( MK_TYPE_ATTRIBUTE( target->flags ) != MK_VM_FRAME_ITEM_TYPE_NODE_VALUE )
		;
	nodeWithParam = 
		(MK_VARIABLE*)target->code.node;

	// todo:Verification required
	if( target->code.pOwner == NULL )
		mk_vm_push_stack( &vm->localStack, vm->pCurrentFrame->pThis );
	else
		mk_vm_push_stack( &vm->localStack, target->code.pOwner );

	for( index = 0; index < sizeParam; index ++ )
		mk_vm_push_stack( &vm->localStack, param[index]);
	
	retCode =
		mk_vm_call_method( vm, nodeWithParam, target->code.definedFrame, varArgC );
	free( param );

	return retCode;
}

MK_CLASS *mk_create_node_class( MK_VM_STRUCT *vm )
{
	MK_CLASS *result =
		mk_create_object( MK_TYPE_CLASS );
	
	result->nameThis = mk_get_symbol_name_ptr( vm, className );
	result->nameSuper = mk_get_symbol_name_ptr( vm, classSuper );

	mk_register_variable( 
		vm,
		mk_create_default_native_method( 
			vm, 
			"initialize", 
			MK_TYPE_VARIABLE | 
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD | 
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD_NATIVE |
				MK_TYPE_ATTRIBUTE_VARIABLE_NEW_NATIVE_METHOD, 
			1,
			(INT_PTR)mk_node_initialize ),
		result );


	mk_register_variable( 
		vm,
		mk_create_method( 
			vm, 
			"invoke", 
			MK_TYPE_VARIABLE | 
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD | 
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD_NATIVE |
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD_VARARGS |
				MK_TYPE_ATTRIBUTE_VARIABLE_NEW_NATIVE_METHOD, 
			0, 
			(INT_PTR)mk_node_invoke ),
		result );

	return result;
}
