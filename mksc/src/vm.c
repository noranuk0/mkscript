#include "mk.h"
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <stdio.h>
#ifdef _DEBUG_DUMP_DUMP
#include <stdio.h>
#endif

static
int mk_append_using_variable(MK_VM_STRUCT *vm, MK_VM_FRAME_ITEM *newVariable, MK_CLASS *pClass);
static
int mk_execute_expr_default_value( MK_VM_STRUCT *vm, MK_VM_FRAME_ITEM *pOwner, MK_VARIABLE *valueVariable, MK_VM_FRAME_ITEM **defaultValue );
static
int mk_append_variables_to_object( MK_VM_STRUCT *vm, MK_VM_FRAME_ITEM *newVariable, MK_CLASS *pClass, int isStatic );
static
void register_internal_classes( MK_VM_STRUCT *vm );
static
int mk_method_push_parameters( MK_VM_STRUCT *vm, MK_VARIABLE *pMethod, MK_VECTOR *parameters, MK_VM_FRAME_ITEM *pMethodOwner );
static
int mk_execute_expr_raise( MK_VM_STRUCT *vm, MK_NODE_EXPR *pExpr, int hasParent, int reference );
static
int mk_execute_expr_operation( MK_VM_STRUCT *vm, MK_NODE_EXPR *pExpr, int hasParent, int reference );
static
int mk_execute_expr_this( MK_VM_STRUCT *vm, MK_NODE_EXPR *pExpr, int hasParent, int reference );
static
int mk_execute_expr_super( MK_VM_STRUCT *vm, MK_NODE_EXPR *pExpr, int hasParent, int reference );
static
int mk_execute_expr_owner( MK_VM_STRUCT *vm, MK_NODE_EXPR *pOwner, int hasParent, int reference );
static
int mk_execute_expr_constant( MK_VM_STRUCT *vm, MK_NODE_EXPR *pExpr, int hasParent, int reference );
static
int mk_execute_expr_new( MK_VM_STRUCT *vm, MK_NODE_EXPR *pExpr, int hasParent, int reference );
static
int mk_execute_expr_call_method( MK_VM_STRUCT *vm, MK_NODE_EXPR *pExpr, int hasParent, int reference );
static
int mk_execute_expr_super_call( MK_VM_STRUCT *vm, MK_NODE_EXPR *pExpr, int hasParent, int reference );
static 
int mk_execute_expr_return( MK_VM_STRUCT *vm, MK_NODE_EXPR *pExpr, int hasParent, int reference );
static 
int mk_execute_expr_break( MK_VM_STRUCT *vm, MK_NODE_EXPR *pExpr, int hasParent, int reference );
static 
int mk_execute_expr_continue( MK_VM_STRUCT *vm, MK_NODE_EXPR *pExpr, int hasParent, int reference );
static
int mk_execute_expr_multiple( MK_VM_STRUCT *vm, MK_NODE_EXPR *pExpr, int hasParent, int reference );
static
int mk_execute_expr_symbol( MK_VM_STRUCT *vm, MK_NODE_EXPR *pExpr, int hasParent, int reference );
static
int mk_execute_expr_at_symbol( MK_VM_STRUCT *vm, MK_NODE_EXPR *pExpr, int hasParent, int reference );
static
int mk_execute_expr_dbl_at_symbol( MK_VM_STRUCT *vm, MK_NODE_EXPR *pExpr, int hasParent, int reference );
static
int mk_execute_expr_array_definition( MK_VM_STRUCT *vm, MK_NODE_EXPR *pExpr, int hasParent, int reference );
static
int mk_execute_expr_node_with_param_definition( MK_VM_STRUCT *vm, MK_NODE_EXPR *pExpr, int hasParent, int reference );
static
int mk_execute_expr_back_if_condition( MK_VM_STRUCT *vm, MK_NODE_EXPR *pExpr, int hasParent, int reference );
static
int mk_execute_block_node_expr( MK_VM_STRUCT *vm, unsigned int *target );
static
int mk_execute_block_if_block( MK_VM_STRUCT *vm, unsigned int *target );
static
int mk_execute_block_while_block( MK_VM_STRUCT *vm, unsigned int *target );
static
int mk_execute_block_try( MK_VM_STRUCT *vm, unsigned int *target );
static
int mk_execute_expr_me( MK_VM_STRUCT *vm, MK_NODE_EXPR *pExpr, int hasParent, int reference );

static 
int callInvalidateBlock( MK_VM_STRUCT *vm, unsigned int* target)
{
	MK_CHAR buffer[sizeof(void*)*2+3];
	sprintf( buffer, "0x%x", *target );
	return mk_raise_internal_error( vm, "", 0,
								MK_ERROR_TYPE_VM_ERROR | 7,
								buffer, "block", NULL );
}

static 
int callInvalidateNodeExpr( MK_VM_STRUCT *vm, MK_NODE_EXPR *expr, int hasParent, int reference )
{
	MK_CHAR buffer[sizeof(void*)*2+3];
	sprintf( buffer, "0x%x", expr->flags );
	return mk_raise_internal_error( vm, "", 0, 
								MK_ERROR_TYPE_VM_ERROR | 7,
								buffer, "NodeExpr", NULL );
}

static
int mk_raise_invalidate_parent_error( MK_VM_STRUCT *vm )
{
	return mk_raise_internal_error( vm, "", 0, 
								MK_ERROR_TYPE_VM_ERROR | 8, NULL );
}

int mk_vm_initialize( MK_VM_STRUCT *vm )
{
	int index = 0;

	// set top stack
	vm->stackTop = (void*)&vm;	// save stack address

	// create symbol table
	vm->hashSymbolName = 
		mk_create_hashtable( 
			MK_TYPE_ATTRIBUTE_HASH_STRING_KEY_DEFAULT | 
				MK_TYPE_ATTRIBUTE_HASH_VALUETYPE_FLAG(0, MK_TYPE_ATTRIBUTE_HASH_VALUE_NONE_PTR ),
			MK_SIZEOF_HASH_DEFAULT );
	vm->hashSymbolName->flags &= ~MK_TYPE_ATTRIBUTE_HASHKEY_MASK;
	vm->hashSymbolName->flags |= MK_TYPE_ATTRIBUTE_HASHKEY_STRING;

	// create object code.
	vm->code = mk_create_object( MK_OBJECT_TYPE_OBJECTCODE );
	vm->code->classes = 
		mk_create_hashtable( MK_TYPE_ATTRIBUTE_HASH_INTPTR_KEY_DEFAULT |
			MK_TYPE_ATTRIBUTE_HASH_VALUETYPE_FLAG(0, MK_TYPE_ATTRIBUTE_HASH_VALUE_MK_OBJECT_PTR ),
		MK_SIZEOF_HASH_DEFAULT );
	mk_register_internal_classes( vm );

	mk_vm_create_stack( &vm->localStack );

	// create block function table
	for( index = 0; index < 0x10; index ++ )
		vm->callBlock[index] = callInvalidateBlock;
	vm->callBlock[MK_OBJECT_TYPE_INDEX( MK_TYPE_NODE_EXPR)] = 
		mk_execute_block_node_expr;
	vm->callBlock[MK_OBJECT_TYPE_INDEX( MK_TYPE_NODE_BLOCK )] = 
		mk_execute_block;
	vm->callBlock[MK_OBJECT_TYPE_INDEX( MK_TYPE_NODE_IF )] = 
		mk_execute_block_if_block;
	vm->callBlock[MK_OBJECT_TYPE_INDEX( MK_TYPE_NODE_WHILE )] = 
		mk_execute_block_while_block;
	vm->callBlock[MK_OBJECT_TYPE_INDEX( MK_TYPE_TRY_BLOCK )] = 
		mk_execute_block_try;
	
	// create expr function table
	for( index = 0; index < 0xcf; index ++ )
		vm->callNodeExpr[index] = callInvalidateNodeExpr;
	vm->callNodeExpr[MK_TYPE_ATTRIBUTE_INDEX(MK_TYPE_NODE_EXPR_RAISE)] = 
		mk_execute_expr_raise;
	vm->callNodeExpr[MK_TYPE_ATTRIBUTE_INDEX(MK_TYPE_NODE_EXPR_OPERATION)] =
		mk_execute_expr_operation;
	vm->callNodeExpr[MK_TYPE_ATTRIBUTE_INDEX(MK_TYPE_NODE_EXPR_SYMBOL_THIS)] =
		mk_execute_expr_this;
	vm->callNodeExpr[MK_TYPE_ATTRIBUTE_INDEX(MK_TYPE_NODE_EXPR_SYMBOL_SUPER)] = 
		mk_execute_expr_super;
	vm->callNodeExpr[MK_TYPE_ATTRIBUTE_INDEX(MK_TYPE_NODE_EXPR_SYMBOL_OWNER)] = 
		mk_execute_expr_owner;
	vm->callNodeExpr[MK_TYPE_ATTRIBUTE_INDEX(MK_TYPE_NODE_EXPR_INT32)] =
		mk_execute_expr_constant;
	vm->callNodeExpr[MK_TYPE_ATTRIBUTE_INDEX(MK_TYPE_NODE_EXPR_INT64)] =
		mk_execute_expr_constant;
	vm->callNodeExpr[MK_TYPE_ATTRIBUTE_INDEX(MK_TYPE_NODE_EXPR_FLOAT)] =
		mk_execute_expr_constant;
	vm->callNodeExpr[MK_TYPE_ATTRIBUTE_INDEX(MK_TYPE_NODE_EXPR_STRING)] =
		mk_execute_expr_constant;
	vm->callNodeExpr[MK_TYPE_ATTRIBUTE_INDEX(MK_TYPE_NODE_EXPR_RETURN)] = 
		mk_execute_expr_return;
	vm->callNodeExpr[MK_TYPE_ATTRIBUTE_INDEX(MK_TYPE_NODE_EXPR_BREAK)] =
		mk_execute_expr_break;
	vm->callNodeExpr[MK_TYPE_ATTRIBUTE_INDEX(MK_TYPE_NODE_EXPR_CONTINUE)] =
		mk_execute_expr_continue;
	vm->callNodeExpr[MK_TYPE_ATTRIBUTE_INDEX(MK_TYPE_NODE_EXPR_MULTIPLESYMBOL)] =
		mk_execute_expr_multiple;
	vm->callNodeExpr[MK_TYPE_ATTRIBUTE_INDEX(MK_TYPE_NODE_EXPR_SYMBOL)] =
		mk_execute_expr_symbol;
	vm->callNodeExpr[MK_TYPE_ATTRIBUTE_INDEX(MK_TYPE_NODE_EXPR_ATSYMBOL)] =
		mk_execute_expr_at_symbol;
	vm->callNodeExpr[MK_TYPE_ATTRIBUTE_INDEX(MK_TYPE_NODE_EXPR_DBLATSYMBOL)] =
		mk_execute_expr_dbl_at_symbol;
	vm->callNodeExpr[MK_TYPE_ATTRIBUTE_INDEX(MK_TYPE_NODE_EXPR_ARRAY_DEFINITION)] =
		mk_execute_expr_array_definition;
	vm->callNodeExpr[MK_TYPE_ATTRIBUTE_INDEX(MK_TYPE_NODE_EXPR_NEW)] =
		mk_execute_expr_new;
	vm->callNodeExpr[MK_TYPE_ATTRIBUTE_INDEX(MK_TYPE_NODE_EXPR_FUNCTION_CALL)] =
		mk_execute_expr_call_method;
	vm->callNodeExpr[MK_TYPE_ATTRIBUTE_INDEX(MK_TYPE_NODE_EXPR_FUNCTION_CALL_INSTANCE)] =
		mk_execute_expr_call_method;
	vm->callNodeExpr[MK_TYPE_ATTRIBUTE_INDEX(MK_TYPE_NODE_EXPR_FUNCTION_CALL_STATIC)] =
		mk_execute_expr_call_method;
	vm->callNodeExpr[MK_TYPE_ATTRIBUTE_INDEX(MK_TYPE_NODE_EXPR_SUPER_CALL)] =
		mk_execute_expr_super_call;
	vm->callNodeExpr[MK_TYPE_ATTRIBUTE_INDEX(MK_TYPE_NODE_EXPR_NODE_WITH_PARAM)] = 
		mk_execute_expr_node_with_param_definition;
	vm->callNodeExpr[MK_TYPE_ATTRIBUTE_INDEX(MK_TYPE_NODE_EXPR_BACK_IF_CONDITION)] =
		mk_execute_expr_back_if_condition;
	vm->callNodeExpr[MK_TYPE_ATTRIBUTE_INDEX(MK_TYPE_NODE_EXPR_ME)] =
		mk_execute_expr_me;
	return MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP;
}

int mk_vm_run( MK_VM_STRUCT *vm, int *pResult )
{
	int retCode = MK_VM_EXECUTE_EXPR_RETURN_FAILED;
#ifdef _DEBUG_DUMP
	dump_node( vm->code, 0 );
#endif

	// execute Kernel::mk_main
	{
		// create Kernel::mk_main node
		MK_NODE_EXPR *pCallMethod = NULL, *pKernelSymbol = NULL, *pRoot = NULL;

		pRoot = 
			mk_create_object( MK_TYPE_NODE_EXPR );
		pRoot->flags |= MK_TYPE_NODE_EXPR_MULTIPLESYMBOL;
		pRoot->u1.multipleSymbols = 
			mk_create_vector( 
				MK_TYPE_VECTOR | MK_TYPE_VECTOR_ARRAY_MANAGED_PTR | ( MK_SIZEOF_EXTEND_VECTOR_DEFAULT << 10 ),
				MK_SIZEOF_VECTOR_DEFAULT,
				(INT_PTR)NULL );

		pKernelSymbol = 
			mk_create_object( MK_TYPE_NODE_EXPR );
		pKernelSymbol->flags |= MK_TYPE_NODE_EXPR_SYMBOL;
		pKernelSymbol->u1.symbolName = 
			mk_get_symbol_name_ptr( vm, CLASS_INTERNAL_KERNEL );
		mk_push_vector( pRoot->u1.multipleSymbols, (INT_PTR)pKernelSymbol );

		pCallMethod = 
			mk_create_object( MK_TYPE_NODE_EXPR );
		pCallMethod->flags |= MK_TYPE_NODE_EXPR_FUNCTION_CALL;
		pCallMethod->u1.symbolName = 
			mk_get_symbol_name_ptr( vm, FUNCTIONNAME_ENTRYPOINT );
		pCallMethod->u2.args = NULL;
		mk_push_vector( pRoot->u1.multipleSymbols, (INT_PTR)pCallMethod );

		// crete root frame
		vm->pCurrentFrame = 
			mk_create_vm_frame_object( &vm->pFrameTable );
		vm->pCurrentFrame->localVariables = NULL;
		vm->pCurrentFrame->pMethod = NULL;
		vm->pCurrentFrame->pThis = NULL;
		vm->pCurrentFrame->pOwnerFrame = NULL;
		vm->pCurrentFrame->previous = NULL;
		vm->pCurrentFrame->pLastResultBlock = (MK_VM_FRAME_ITEM*)MK_VM_FRAME_ITEM_TYPE_DIRECT_VALUE;
		vm->pTopFrame =
			vm->pCurrentFrame;

		// set this
		{
			MK_VM_FRAME_ITEM *pItem = 
				mk_create_vm_frame_item_object( &vm->pFrameItemTable );
			pItem->flags = MK_TYPE_SET_ATTRIBUTE( pItem->flags, MK_VM_FRAME_ITEM_TYPE_CLASS );
			pItem->classTypeValue.typeName = 
				mk_get_symbol_name_ptr( vm, CLASS_INTERNAL_KERNEL );
			pItem->classTypeValue.child = NULL;
			pItem->classTypeValue.variables = NULL;
			vm->pCurrentFrame->pThis = pItem;
		}

		// run Kernel.mk_main
		retCode = 
			mk_execute_expr( vm, pRoot, 0, 0 );

		if( retCode == MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP && pResult != NULL)
		{
			MK_VM_FRAME_ITEM *pMainResult = mk_vm_pop_stack(&vm->localStack);
			*pResult = mk_vm_frame_item_to_int32(vm, pMainResult);
		}
		// destroy entrypoint node
		mk_destroy_node( pRoot );
	}
	return retCode;
}

int mk_vm_initialize_object_variables( MK_VM_STRUCT *vm, MK_CLASS *pClass, MK_VM_FRAME_ITEM *target, int isStatic )
{
	MK_VM_FRAME *frame = NULL;
	int size = 0, index = 0;
	int retCode = MK_VM_EXECUTE_EXPR_THROW;
	const MK_CHAR *keyOwner = NULL;

	// setup frame
	frame = 
		mk_create_vm_frame_object( &vm->pFrameTable );
	frame->localVariables = NULL;
	frame->pMethod = ( vm->pCurrentFrame != NULL ) ? vm->pCurrentFrame->pMethod : NULL;
	frame->pThis = target;
	frame->pOwnerFrame = NULL;
	frame->previous = vm->pCurrentFrame;
	vm->pCurrentFrame = frame;
	do
	{
		if( mk_append_variables_to_object( vm, target, pClass, isStatic ) != MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP )
			break;
		retCode = MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP;
	}
	while( 0 );

	//rollback frame
	vm->pCurrentFrame = vm->pCurrentFrame->previous;

	return retCode;
}

static
int mk_execute_expr_default_value( MK_VM_STRUCT *vm, MK_VM_FRAME_ITEM *pOwner, MK_VARIABLE *valueVariable, MK_VM_FRAME_ITEM **defaultValue )
{
	int retCode = MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP;
	if( !( valueVariable->flags & MK_TYPE_ATTRIBUTE_VARIABLE_METHOD ) )
	{
		if( valueVariable->defaultValue != NULL )
		{
			// setup default value.
			retCode = mk_execute_expr( vm, valueVariable->defaultValue, 0, 0 );
			if( retCode == MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP )
				*defaultValue = mk_vm_pop_stack( &vm->localStack );
		}
	}
	else
	{
		*defaultValue = 
			mk_create_vm_frame_item_object( &vm->pFrameItemTable );
		(*defaultValue)->flags |= MK_VM_FRAME_ITEM_TYPE_NODE_VALUE;
		(*defaultValue)->code.definedFrame = NULL;
		(*defaultValue)->code.pOwner = pOwner;
		(*defaultValue)->code.node = (unsigned int*)valueVariable;
	}
	return retCode;
}

// setup variables.
static
int mk_append_variables_to_object( MK_VM_STRUCT *vm, MK_VM_FRAME_ITEM *newVariable, MK_CLASS *pClass, int isStatic )
{
	MK_HASHTABLE *variables = 
		pClass->variables;
	void *handle = 
		( variables != NULL ) ? mk_enum_item_hashtable_begin( variables ) : NULL;
	while( handle != NULL )
	{
		const MK_CHAR *keyVariable = NULL;
		MK_VARIABLE *valueVariable = NULL;
		MK_VM_FRAME_ITEM *defaultValue = NULL;
		INT_PTR extends = 0;
		void *handleDefaultValue = NULL;
		handle = 
			mk_enum_item_hashtable_next( variables, handle, &keyVariable, (void**)&valueVariable );
		if(newVariable->classTypeValue.variables != NULL && 
			mk_find_item_hashtable(newVariable->classTypeValue.variables, keyVariable, &defaultValue) != NULL)
			continue;

		if( !(valueVariable->flags & MK_TYPE_ATTRIBUTE_VARIABLE_METHOD) && 
			(valueVariable->flags & MK_TYPE_ATTRIBUTE_VARIABLE_MODULE))
		{
			// import module variable.
			
			void *handleOwner = NULL;
			defaultValue = 
				mk_create_vm_frame_item_object( &vm->pFrameItemTable );
			defaultValue->flags |= 
				MK_VM_FRAME_ITEM_TYPE_MODULE | ( ( isStatic != 0 ) ? MK_VM_FRAME_ITEM_TYPE_STATIC_INSTANCE : 0 );
			defaultValue->classTypeValue.typeName = valueVariable->moduleVariables->nameThis;
			defaultValue->classTypeValue.child = NULL;
			defaultValue->classTypeValue.variables = NULL;
			if( mk_append_variables_to_object( vm, defaultValue, valueVariable->moduleVariables, isStatic ) != MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP )
				return MK_VM_EXECUTE_EXPR_THROW;
			extends |= MK_VM_FRAME_ITEM_EXTEND_TYPE_FINAL;

			// register owner
			if( defaultValue->classTypeValue.variables == NULL )
				defaultValue->classTypeValue.variables = 
					mk_allocate_vm_managed_hashtable( MK_TYPE_ATTRIBUTE_HASH_INTERPRETER_DEFAULT, MK_SIZEOF_HASH_DEFAULT, &vm->pHashTable );
			handleOwner = mk_insert_item_hashtable( 
				defaultValue->classTypeValue.variables, mk_get_symbol_name_ptr( vm, "owner" ), newVariable );
			mk_set_extend_value_hashtable( 
				defaultValue->classTypeValue.variables, 
				handleOwner, 
				1, 
				MK_VM_FRAME_ITEM_EXTEND_TYPE_FINAL );
		}
		else
		{
			if((valueVariable->flags & MK_TYPE_ATTRIBUTE_VARIABLE_METHOD) &&
				(((isStatic != 0) && (valueVariable->flags & MK_TYPE_ATTRIBUTE_VARIABLE_STATIC)) ||
				((isStatic == 0) && !(valueVariable->flags & MK_TYPE_ATTRIBUTE_VARIABLE_STATIC))))
			{
				// normal method
				defaultValue = 
					mk_create_vm_frame_item_object(&vm->pFrameItemTable);

				defaultValue->flags |= 
					MK_VM_FRAME_ITEM_TYPE_NODE_VALUE;
				defaultValue->code.definedFrame = NULL;
				defaultValue->code.pOwner = newVariable;
				defaultValue->code.node = (unsigned int*)valueVariable;
				extends |= MK_VM_FRAME_ITEM_EXTEND_TYPE_FINAL;
			}
			else if(!(valueVariable->flags & MK_TYPE_ATTRIBUTE_VARIABLE_METHOD) &&
				(((isStatic != 0) && (valueVariable->flags & MK_TYPE_ATTRIBUTE_VARIABLE_STATIC)) ||
				((isStatic == 0) && !(valueVariable->flags & MK_TYPE_ATTRIBUTE_VARIABLE_STATIC))))
			{
				// normal variable.
				if( mk_execute_expr_default_value( vm, newVariable, valueVariable, &defaultValue ) != MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP )
					return MK_VM_EXECUTE_EXPR_THROW;
			} else {
				continue;
			}
			switch( valueVariable->flags & MK_TYPE_ATTRIBUTE_VARIABLE_READ_MASK )
			{
			case MK_TYPE_ATTRIBUTE_VARIABLE_READ_PUBLIC:
				extends |= MK_VM_FRAME_ITEM_EXTEND_TYPE_READ_PUBLIC;
				break;
			case MK_TYPE_ATTRIBUTE_VARIABLE_READ_PROTECTED:
				extends |= MK_VM_FRAME_ITEM_EXTEND_TYPE_READ_PROTECTED;
				break;
			case MK_TYPE_ATTRIBUTE_VARIABLE_READ_PRIVATE:
				extends |= MK_VM_FRAME_ITEM_EXTEND_TYPE_READ_PRIVATE;
				break;
			}
			switch( valueVariable->flags & MK_TYPE_ATTRIBUTE_VARIABLE_WRITE_MASK )
			{
			case MK_TYPE_ATTRIBUTE_VARIABLE_WRITE_PUBLIC:
				extends |= MK_VM_FRAME_ITEM_EXTEND_TYPE_WRITE_PUBLIC;
				break;
			case MK_TYPE_ATTRIBUTE_VARIABLE_WRITE_PROTECTED:
				extends |= MK_VM_FRAME_ITEM_EXTEND_TYPE_WRITE_PROTECTED;
				break;
			case MK_TYPE_ATTRIBUTE_VARIABLE_WRITE_PRIVATE:
				extends |= MK_VM_FRAME_ITEM_EXTEND_TYPE_WRITE_PRIVATE;
				break;
			}
		}
		if( newVariable->classTypeValue.variables == NULL )
			newVariable->classTypeValue.variables = 
				mk_allocate_vm_managed_hashtable(MK_TYPE_ATTRIBUTE_HASH_INTERPRETER_DEFAULT, MK_SIZEOF_HASH_DEFAULT, &vm->pHashTable);
		handleDefaultValue = 
			mk_insert_item_hashtable(
				newVariable->classTypeValue.variables,
				keyVariable,
				defaultValue);
		mk_set_extend_value_hashtable(newVariable->classTypeValue.variables, handleDefaultValue, 1, extends);
	}
	return mk_append_using_variable(vm, newVariable, pClass);
}

static
int mk_append_using_variable(MK_VM_STRUCT *vm, MK_VM_FRAME_ITEM *newVariable, MK_CLASS *pClass)
{
	MK_CHAR *keyOwner = mk_get_symbol_name_ptr( vm, "owner" );
	int index = 0;
	int size = 0;
	// register module variable of using style.
	size = ( pClass->usingNames != NULL ) ? mk_size_vector( pClass->usingNames ) : 0;
	for( index = 0; index < size; index ++ )
	{
		MK_VM_FRAME_ITEM *targetModule = NULL;
		if( mk_find_item_hashtable( newVariable->classTypeValue.variables, 
				(MK_CHAR*)mk_get_at_vector( pClass->usingNames, index ), 
				(void**)&targetModule ) != 0 &&
			( targetModule->flags & MK_VM_FRAME_ITEM_TYPE_MODULE ) )
		{
			const MK_CHAR *keyModule = NULL;
			MK_VM_FRAME_ITEM *valueModule = NULL;
			void *handleModule = 
				mk_enum_item_hashtable_begin( targetModule->classTypeValue.variables );
			while( handleModule != NULL )
			{
				handleModule = 
					mk_enum_item_hashtable_next( targetModule->classTypeValue.variables, handleModule, 
						&keyModule, (void**)&valueModule );
				if( keyModule != keyOwner )
				{
					MK_VM_FRAME_ITEM **pTargetValue = NULL;
					MK_VM_FRAME_ITEM *pValueModule = NULL;
					mk_find_item_pointer_hashtable( targetModule->classTypeValue.variables, keyModule, (void**)&pTargetValue );
					pValueModule = (MK_VM_FRAME_ITEM *)((INT_PTR)(pTargetValue) | MK_VM_FRAME_ITEM_TYPE_DIRECT_POINTER);
					mk_insert_item_hashtable( newVariable->classTypeValue.variables, keyModule, pValueModule );
				}
			}
			if( handleModule != NULL )
				break;
		}
		else
		{
			break;
		}
	}
	return MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP;
}

MK_VM_FRAME_ITEM* mk_vm_has_module( MK_VM_STRUCT *vm, MK_VM_FRAME_ITEM *target, MK_SYM_CHAR *nameModule )
{
	if( (INT_PTR)target & MK_VM_FRAME_ITEM_TYPE_DIRECT_VALUE )
		return NULL;
	if( MK_TYPE_ATTRIBUTE(target->flags) == MK_VM_FRAME_ITEM_TYPE_MODULE )
	{
		return ( nameModule == target->classTypeValue.typeName ) ? target : NULL;
	}
	else if( MK_TYPE_ATTRIBUTE(target->flags) == MK_VM_FRAME_ITEM_TYPE_CLASS )
	{
		while( target != NULL )
		{
			MK_VM_FRAME_ITEM *super = NULL;
			void *handle = 
				mk_enum_item_hashtable_begin( target->classTypeValue.variables );
			const MK_CHAR *key = NULL;
			MK_VM_FRAME_ITEM *value = NULL;
			MK_SYM_CHAR *chSuper = 
				mk_get_symbol_name_ptr( vm, "super" );
			while( handle != NULL )
			{
				handle = 
					mk_enum_item_hashtable_next( target->classTypeValue.variables, 
						handle, &key, (void**)&value );
				if( key == chSuper )
				{
					super = value;
				}
				else if( !( (INT_PTR)value & MK_VM_FRAME_ITEM_TYPE_DIRECT_VALUE ) &&
					MK_TYPE_ATTRIBUTE( value->flags ) == MK_VM_FRAME_ITEM_TYPE_MODULE ) 
				{
					if( value->classTypeValue.typeName == nameModule )
						return value;
				}
			}
			if( super != NULL )
				target = super;
			else
				target = NULL;
		}
	}
	return NULL;
}

MK_VM_FRAME_ITEM *mk_vm_find_instance( MK_VM_STRUCT *vm, MK_VM_FRAME_ITEM *instance, MK_SYM_CHAR *className )
{
	int index = 0;
	MK_SYM_CHAR *super = NULL, *owner = NULL;

	for( index = 0; index < 0x10; index ++ )
	{
		if( className == vm->cache->internalClassSymbolName[index] )
			break;
	}
	if( index < 0x10 )
	{
		// value class
		unsigned int attribute = index << 24;
		while( instance != NULL )
		{
			if( (INT_PTR)instance & MK_VM_FRAME_ITEM_TYPE_DIRECT_VALUE )
			{
				if( attribute != MK_VM_FRAME_ITEM_TYPE_INT_VALUE )
					instance = NULL;
				break;
			}
			else if( ( MK_TYPE_ATTRIBUTE( instance->flags ) >> 24 ) == index )
			{
				break;
			}
			else if( MK_TYPE_ATTRIBUTE( instance->flags ) == MK_VM_FRAME_ITEM_TYPE_CLASS )
			{
				if( super == NULL )
					super = mk_get_symbol_name_ptr( vm, "super" );
				mk_find_item_hashtable( instance->classTypeValue.variables, super, (void**)&instance );
			}
			else if( MK_TYPE_ATTRIBUTE( instance->flags ) == MK_VM_FRAME_ITEM_TYPE_MODULE )
			{
				if( owner == NULL )
					owner = mk_get_symbol_name_ptr( vm, "owner" );
				mk_find_item_hashtable( instance->classTypeValue.variables, owner, (void**)&instance );
			}
			else
			{
				instance = NULL;
			}
		}
	}
	else
	{
		while( instance != NULL )
		{
			if( (INT_PTR)instance & MK_VM_FRAME_ITEM_TYPE_DIRECT_VALUE )
			{
				instance = NULL;
			}
			else if( MK_TYPE_ATTRIBUTE( instance->flags ) == MK_VM_FRAME_ITEM_TYPE_CLASS )
			{
				if( instance->classTypeValue.typeName == className )
					break;
				if( super == NULL )
					super = mk_get_symbol_name_ptr( vm, "super" );
				mk_find_item_hashtable( instance->classTypeValue.variables, super, (void**)&instance );
			}
			else if( MK_TYPE_ATTRIBUTE( instance->flags ) == MK_VM_FRAME_ITEM_TYPE_MODULE )
			{
				if( instance->classTypeValue.typeName == className )
					break;
				if( owner == NULL )
					owner = mk_get_symbol_name_ptr( vm, "owner" );
				mk_find_item_hashtable( instance->classTypeValue.variables, owner, (void**)&instance );
			}
			else
			{
				instance = NULL;
			}
		}
	}
	return instance;
}

MK_VM_FRAME_ITEM *mk_vm_is_class( MK_VM_STRUCT* vm, MK_VM_FRAME_ITEM *target, MK_SYM_CHAR *nameClass )
{
	MK_SYM_CHAR *chSuper = NULL;
	if( (INT_PTR)target & MK_VM_FRAME_ITEM_TYPE_DIRECT_VALUE ||
		( MK_TYPE_ATTRIBUTE(target->flags) != MK_VM_FRAME_ITEM_TYPE_CLASS &&
		  MK_TYPE_ATTRIBUTE(target->flags) != MK_VM_FRAME_ITEM_TYPE_MODULE ) )
	{
		MK_CLASS *pClass = 
			mk_vm_get_class( vm, target );
		while( pClass != NULL )
		{
			if( pClass->nameThis == nameClass )
				return target;
			pClass = mk_vm_get_class_by_name( vm, pClass->nameSuper );
		}
	}
	else
	{
		MK_SYM_CHAR *chSuper = NULL;
		if( MK_TYPE_ATTRIBUTE(target->flags) == MK_VM_FRAME_ITEM_TYPE_MODULE &&
			mk_find_item_hashtable( target->classTypeValue.variables, 
				mk_get_symbol_name_ptr( vm, "owner" ), (void**)&target ) == 0 )
			return NULL;
		while( target != NULL )
		{
			if( target->classTypeValue.typeName == nameClass )
				return target;
			if( chSuper == NULL )
				chSuper = mk_get_symbol_name_ptr( vm, "super" );
			if( mk_find_item_hashtable( 
				target->classTypeValue.variables,
				chSuper,
				(void**)&target ) == 0 )
				break;
		}
	}
	return NULL;
}

MK_CLASS *mk_vm_get_class_by_name( MK_VM_STRUCT *vm, const MK_SYM_CHAR *name )
{
	int index = 0;
	MK_HASHTABLE *classes = vm->code->classes;
	MK_CLASS *result = NULL;

	mk_find_item_hashtable( 
		classes, 
		name, 
		(void**)&result );
	return result;
}

MK_CLASS *mk_vm_get_class( MK_VM_STRUCT *vm, MK_VM_FRAME_ITEM *pTarget )
{
	unsigned int attribute = 0;
	unsigned int index = 0;
	MK_CLASS *result = NULL;
	MK_HASHTABLE *classes = NULL;
	if( (INT_PTR)pTarget & MK_VM_FRAME_ITEM_TYPE_DIRECT_VALUE )
	{
		result = vm->cache->pConstantClass[ MK_TYPE_ATTRIBUTE(MK_VM_FRAME_ITEM_TYPE_INT_VALUE) >> 24 ];
	}
	else
	{
		attribute = MK_TYPE_ATTRIBUTE( pTarget->flags );
		index = attribute >> 24;
		result = vm->cache->pConstantClass[index];
		if( result == NULL &&
			( attribute == MK_VM_FRAME_ITEM_TYPE_CLASS || attribute == MK_VM_FRAME_ITEM_TYPE_MODULE ) )
		{
			mk_find_item_hashtable( 
				vm->code->classes, 
				pTarget->classTypeValue.typeName, 
				(void**)&result );
		}
	}
	return result;
}

static 
int mk_create_object_instance( MK_VM_STRUCT *vm, MK_VECTOR *arguments, MK_VM_FRAME_ITEM **result )
{
	int retCode = 0;
	int sizeParameters = 0, sizeArguments = 0;
	MK_VARIABLE *pMethod = NULL;
	MK_CHAR *pObjectClassName = mk_get_symbol_name_ptr( vm, CLASS_INTERNAL_OBJECT );
	MK_VM_FRAME_ITEM *pTarget = mk_vm_pop_stack( &vm->localStack );

	// call Object.new method
	if( mk_object_new( vm, 
		pTarget, result ) != MK_VM_EXECUTE_EXPR_RETURN_RETURN )
	{
		if( vm->exceptionObject == NULL )
			return mk_raise_internal_error( vm, "", 0, MK_ERROR_TYPE_VM_ERROR | 13, NULL );
		else
			return MK_VM_EXECUTE_EXPR_THROW;	// todo: create error object.
	}

	// call initialize
	sizeArguments = 
		( arguments != NULL ) ? mk_size_vector( arguments ) : 0;

	pMethod = 
		mk_vm_find_method( vm, 
			*result, 
			mk_get_symbol_name_ptr( vm, "initialize" ), 
			sizeArguments );
	if( pMethod != NULL )
	{
		sizeParameters = 
			( pMethod->args != NULL ) ? mk_size_vector( pMethod->args ) : 0;
		if( mk_method_push_parameters( vm, pMethod, arguments, *result ) == MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP )
		{
			retCode = 
				mk_vm_call_method( vm, pMethod, NULL, sizeArguments - sizeParameters );
			if( retCode == MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP )
			{
				mk_vm_pop_stack( &vm->localStack );	// drop initialize result.
				mk_vm_push_stack( 
					&vm->localStack, 
					*result );
			}
		}
		else
		{
			if( vm->exceptionObject == NULL )
				return mk_raise_internal_error( vm, "", 0, MK_ERROR_TYPE_VM_ERROR | 13, NULL );
			else
				return MK_VM_EXECUTE_EXPR_THROW;	// todo: create exception objec
		}
	}
	else
	{
		mk_vm_push_stack( &vm->localStack, *result );
		retCode = MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP;
	}
	return retCode;
}

static
int mk_execute_block_internal_catch( MK_VM_STRUCT *vm, MK_CATCH_BLOCK *catchBlock, int *isHandled )
{
	int retCode = MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP;
	MK_VARIABLE *condition =
		(MK_VARIABLE*)catchBlock->paramCatch->u1.node;

	*isHandled = 0;
	if( condition->entryPoint != NULL )
	{
		mk_vm_push_stack( &vm->localStack, vm->pCurrentFrame->pThis );	// pThis
		mk_vm_push_stack( &vm->localStack, vm->exceptionObject );		// exception object
		retCode = 
			mk_vm_call_method( vm, condition, vm->pCurrentFrame, 1 );
		if( retCode == MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP )
		{
			MK_VM_FRAME_ITEM *pResult = 
				mk_vm_pop_stack( &vm->localStack );
			if( mk_vm_frame_item_is_true(pResult) )
				*isHandled = 1;
		}
	}
	else
	{
		*isHandled = 1;
	}

	if( *isHandled != 0 )
	{
		mk_vm_push_stack( &vm->localStack, vm->pCurrentFrame->pThis );	// pThis
		mk_vm_push_stack( &vm->localStack, vm->exceptionObject );		// exception object
		vm->exceptionObject = NULL;
		retCode = 
			mk_vm_call_method( vm, (MK_VARIABLE*)catchBlock->blockCatch->u1.node, vm->pCurrentFrame, 1 );
	}
	return retCode;
}

static
int mk_execute_block_try( MK_VM_STRUCT *vm, unsigned int *target )
{
	MK_TRY_BLOCK *tryBlock = ( MK_TRY_BLOCK * )target;
	int retCode = MK_VM_EXECUTE_EXPR_RETURN_FAILED;
	int isHandled = 0;

	// execute try
	retCode = 
		mk_execute_block( vm, ( unsigned int * )tryBlock->blockTry );

	if( retCode == MK_VM_EXECUTE_EXPR_THROW )
	{
		// execute catch
		unsigned int sizeCatch = 
			tryBlock->blockCatch != NULL ? mk_size_vector( tryBlock->blockCatch ) : 0;
		unsigned int index = 0;
		if( vm->exceptionObject == NULL )
			mk_raise_internal_error( vm, "", 0, MK_ERROR_TYPE_VM_ERROR | 13, NULL );
		for( index = 0; index < sizeCatch; index ++ )
		{
			MK_CATCH_BLOCK *catchBlock = 
				( MK_CATCH_BLOCK * )mk_get_at_vector( tryBlock->blockCatch, index );
			retCode = 
				mk_execute_block_internal_catch( vm, catchBlock, &isHandled );
			if( isHandled != 0 )
				break;
		}
		if( index == sizeCatch )
			retCode = MK_VM_EXECUTE_EXPR_THROW;
	}
	else if( retCode == MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP &&
		tryBlock->blockNoException != NULL )
	{
		retCode = mk_execute_block( vm, ( unsigned int * )tryBlock->blockNoException );
	}

	// execute finally
	if( retCode == MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP &&
		tryBlock->blockFinally != NULL )
		retCode = mk_execute_block( vm, ( unsigned int * )tryBlock->blockFinally );

	return retCode;
}

static
int mk_execute_block_node_expr( MK_VM_STRUCT *vm, unsigned int *target )
{
	MK_NODE_EXPR *pExpr = (MK_NODE_EXPR*)target;
	int result = mk_execute_expr( vm, pExpr, 0, 0 );
	if( result == MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP )
	{
		// save last expr result.
		vm->pCurrentFrame->pLastResultBlock = mk_vm_pop_stack( &vm->localStack );
		result = MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP;
	}
	return result;
}

static
int mk_execute_block_if_block( MK_VM_STRUCT *vm, unsigned int *target )
{
	MK_VM_FRAME_ITEM *condResult = NULL;
	MK_NODE_IF *pIf = (MK_NODE_IF*) target;
	int result = MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP;
	while( pIf )
	{
		if( pIf->expr )
		{
			result = 
				mk_execute_expr( vm, pIf->expr, 0, 0 );
			if( result == MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP )
				condResult = mk_vm_pop_stack( &vm->localStack );
			else
				break;
		}
		if( !pIf->expr || ( condResult && mk_vm_frame_item_is_true(condResult) ) )
		{
			result = mk_execute_block( vm, ( unsigned int * )pIf->block );
			break;
		}
		pIf = pIf->next;
	}
	return result;
}

static
int mk_execute_block_while_block( MK_VM_STRUCT *vm, unsigned int *target )
{
	MK_NODE_WHILE *pWhile = (MK_NODE_WHILE*)target;
	int isFront =  MK_TYPE_ATTRIBUTE( pWhile->flags ) & MK_TYPE_ATTRIBUTE_NODE_WHILE_FRONT;
	int result = MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP;

	while( 1 )
	{
		if( isFront )
		{
			result = 
				mk_execute_expr( vm, pWhile->expr, 0, 0 );
			if( result == MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP )
			{
				MK_VM_FRAME_ITEM *condResult = NULL;
				condResult = mk_vm_pop_stack( &vm->localStack );
				if( !condResult || !mk_vm_frame_item_is_true(condResult) )	// todo : true or false
				{
					result = MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP;
					break;
				}
			}
			else
			{
				break;	// error or exception
			}
		}
		result = mk_execute_block( vm, ( unsigned int * )pWhile->block );
		if( result == MK_VM_EXECUTE_EXPR_RETURN_BREAK )
		{
			result = MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP;
			break;
		}
		else if( result == MK_VM_EXECUTE_EXPR_RETURN_CONTINUE )
		{
			result = MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP;
			continue;
		}
		else if( result == MK_VM_EXECUTE_EXPR_RETURN_RETURN )
		{
			break;
		}
		else if( result == MK_VM_EXECUTE_EXPR_THROW )
		{
			break;
		}
		else if( result == MK_VM_EXECUTE_EXPR_RETURN_FAILED )
		{
			break;
		}
		if( !isFront )
		{
			result = 
				mk_execute_expr( vm, pWhile->expr, 0, 0 );
			if( result == MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP )
			{
				MK_VM_FRAME_ITEM *condResult = 
					mk_vm_pop_stack( &vm->localStack );
				if( !condResult || !mk_vm_frame_item_is_true(condResult) )	// todo : true or false
				{
					result = MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP;
					break;
				}
			}
			else
			{
				break;	// error or exception
			}
		}

	}
	return result;
}

int mk_execute_block( MK_VM_STRUCT *vm, unsigned int *target )
{
	MK_NODE_BLOCK *pBlock = ( MK_NODE_BLOCK * ) target;
	unsigned int size = 
		pBlock->exprs != NULL ? mk_size_vector( pBlock->exprs ) : 0;
	unsigned int index = 0;
	int result = MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP;
	unsigned int currentStack =
		mk_vm_stack_get_current( &vm->localStack );
	unsigned int attribute = MK_TYPE_ATTRIBUTE(*target);

	for( index = 0; index < size; index ++ )
	{
		unsigned int *flags = 
			(unsigned int*)mk_get_at_vector( pBlock->exprs, index );
		// execute gc
		if( ( ++ vm->gcPhase ) % GC_EXECUTE_TIME_PER_CALL == 0 )
			mk_gc_run( vm, 0 );

		result = vm->callBlock[(*flags)>>28](vm, flags );

		// check result.
		if( result == MK_VM_EXECUTE_EXPR_RETURN_CONTINUE )
		{
			if( attribute == MK_TYPE_ATTRIBUTE_BLOCK_FUNCTION_ROOT ||
				attribute == MK_TYPE_ATTRIBUTE_BLOCK_NONAME_METHOD )
				return mk_raise_internal_error( vm, "", 0, 
											MK_ERROR_TYPE_VM_ERROR | 3,
											"continue", NULL );
			else
				break;
		}
		else if( result == MK_VM_EXECUTE_EXPR_RETURN_BREAK )
		{
			if( attribute == MK_TYPE_ATTRIBUTE_BLOCK_FUNCTION_ROOT ||
				attribute == MK_TYPE_ATTRIBUTE_BLOCK_NONAME_METHOD )
				return mk_raise_internal_error( vm, "", 0, 
											MK_ERROR_TYPE_VM_ERROR | 3,
											"break", NULL );
			else
				break;
		}
		else if( result == MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP )
		{
			continue;
		}
		else	// THROW, RETURN
		{
			break;
		}

	}

#ifdef _DEBUG
	if( ( result == MK_VM_EXECUTE_EXPR_RETURN_RETURN &&
		mk_vm_stack_get_current( &vm->localStack ) != currentStack + 1 ) ||
		( ( result != MK_VM_EXECUTE_EXPR_RETURN_RETURN && result != MK_VM_EXECUTE_EXPR_THROW ) &&
		mk_vm_stack_get_current( &vm->localStack ) != currentStack ) )
		fprintf( stderr, 
				"invalid stack length before:%d after:%d\n", 
				currentStack, mk_vm_stack_get_current( &vm->localStack ) );
#endif
	if( result == MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP &&
		( MK_TYPE_ATTRIBUTE( pBlock->flags ) == MK_TYPE_ATTRIBUTE_BLOCK_FUNCTION_ROOT ||
		  MK_TYPE_ATTRIBUTE( pBlock->flags ) == MK_TYPE_ATTRIBUTE_BLOCK_NONAME_METHOD ) )
	{
		// return last block expr
		MK_VM_FRAME_ITEM *pReturn = 
			vm->pCurrentFrame->pLastResultBlock;
		mk_vm_push_stack( 
			&vm->localStack, 
			pReturn );
		result = MK_VM_EXECUTE_EXPR_RETURN_RETURN;
	}
	else if( result == MK_VM_EXECUTE_EXPR_THROW )
	{
		mk_vm_stack_set_current( &vm->localStack, currentStack );
	}
	return result;
}

MK_VARIABLE* mk_vm_find_operator_method( MK_VM_STRUCT *vm, MK_CLASS *pOwner, unsigned int index )
{
	MK_CLASS *pTop = NULL;
	MK_VARIABLE *pResult = NULL;

	pTop = pOwner;
	while( pOwner != NULL )
	{
		if( pOwner->operatorMethod != NULL )
		{
			pResult = 
				(MK_VARIABLE*)pOwner->operatorMethod[index];
			if( pResult != NULL )
				break;
		}
		// search superclass
		if( pOwner->nameSuper != NULL &&
			pOwner->nameSuper[0] != '\0' )
			pOwner = 
				mk_vm_get_class_by_name( vm, pOwner->nameSuper );
		else
			pOwner = NULL;
	}
	return pResult;
}

MK_VARIABLE* mk_vm_find_method( MK_VM_STRUCT *vm, 
								 MK_VM_FRAME_ITEM *pOwner, const MK_SYM_CHAR *name, 
								 unsigned int sizeArgs )
{
	MK_VARIABLE *variable = NULL;
	MK_VM_FRAME_ITEM *result = 
		mk_vm_find_variable( vm, pOwner, name );
	if( ( result != NULL ) &&
		!( (INT_PTR)result & MK_VM_FRAME_ITEM_TYPE_DIRECT_VALUE ) )
	{
		MK_VARIABLE*target = NULL;
		if( MK_OBJECT_TYPE(result->flags) == MK_TYPE_VARIABLE )
		{
			target = 
				(MK_VARIABLE*)result;
		}
		else if( ( MK_OBJECT_TYPE(result->flags) == MK_OBJECT_TYPE_VM_FRAME_ITEM ) &&
			( MK_TYPE_ATTRIBUTE(result->flags) == MK_VM_FRAME_ITEM_TYPE_NODE_VALUE ) )
		{
			target = 
				(MK_VARIABLE*)result->code.node;
		}
		if( target != NULL )
		{
			if( target->flags & MK_TYPE_ATTRIBUTE_VARIABLE_METHOD )
			{
				unsigned int sizeArgTarget = target->args != NULL ? mk_size_vector( target->args ) : 0;
				int isVarargs = target->flags & MK_TYPE_ATTRIBUTE_VARIABLE_METHOD_VARARGS;
				if( ( sizeArgTarget >= sizeArgs ) ||
					( ( sizeArgTarget < sizeArgs ) && ( isVarargs != 0 ) ) )
					variable = target;
			}
		}
	}
	return variable;
}

MK_VM_FRAME_ITEM* mk_vm_find_variable_reference( MK_VM_STRUCT *vm, 
								 MK_VM_FRAME_ITEM *pOwner, const MK_SYM_CHAR *name )
{
	MK_VM_FRAME_ITEM *pResult = NULL;
	MK_VM_FRAME_ITEM *pTop = pOwner;
	MK_CHAR *chSuper = NULL;
	while( pOwner != NULL )
	{
		if( ( (INT_PTR)pOwner & MK_VM_FRAME_ITEM_TYPE_DIRECT_VALUE ) ||
			( MK_TYPE_ATTRIBUTE( pOwner->flags ) != MK_VM_FRAME_ITEM_TYPE_CLASS &&
			  MK_TYPE_ATTRIBUTE( pOwner->flags ) != MK_VM_FRAME_ITEM_TYPE_MODULE ) )
		{
			pOwner = NULL;
		}
		else
		{
			void *handle = NULL;
			unsigned int extend = 0;
			if( pOwner->classTypeValue.variables != NULL )
			{
				handle = 
					mk_find_item_hashtable( pOwner->classTypeValue.variables, name, (void**)&pResult );
				if( handle != NULL )
				{
					extend = mk_get_extend_value_hashtable( pOwner->classTypeValue.variables, handle, 1 );
					pResult = mk_create_vm_frame_item_object( &vm->pFrameItemTable );
					pResult->flags |= MK_VM_FRAME_ITEM_TYPE_REFERENCE_VALUE;
					if( extend & MK_VM_FRAME_ITEM_EXTEND_TYPE_FINAL )
						pResult->flags |= MK_VM_FRAME_ITEM_TYPE_FINAL;
					pResult->referenceTypeValue.target = pOwner->classTypeValue.variables;
					pResult->referenceTypeValue.symbolName = (MK_SYM_CHAR*)name;
					break;
				}
				if( MK_TYPE_ATTRIBUTE( pOwner->flags ) == MK_VM_FRAME_ITEM_TYPE_MODULE )
				{
					mk_find_item_hashtable( pOwner->classTypeValue.variables, 
						mk_get_symbol_name_ptr( vm, "owner" ),
						(void**)&pOwner );
				}
				else
				{
					if( chSuper == NULL )
						chSuper = 
							mk_get_symbol_name_ptr( vm, "super" );
					mk_find_item_hashtable( pOwner->classTypeValue.variables, chSuper, (void**)&pOwner );
				}
			}
		}
	}
	return pResult;
}

MK_VM_FRAME_ITEM* mk_vm_find_variable( MK_VM_STRUCT *vm, 
								 MK_VM_FRAME_ITEM *pOwner, const MK_SYM_CHAR *name )
{
	void *handleItem = NULL;
	MK_VM_FRAME_ITEM *pResult = NULL;
	MK_VM_FRAME_ITEM *pTop = pOwner;
	MK_CHAR *chSuper = NULL;
	while( pOwner != NULL )
	{
		if( ( (INT_PTR)pOwner & MK_VM_FRAME_ITEM_TYPE_DIRECT_VALUE ) ||
			( MK_TYPE_ATTRIBUTE( pOwner->flags ) != MK_VM_FRAME_ITEM_TYPE_CLASS &&
			  MK_TYPE_ATTRIBUTE( pOwner->flags ) != MK_VM_FRAME_ITEM_TYPE_MODULE ) )
		{
			MK_CLASS *pTop = NULL, *pCurrent = NULL;
			pTop = mk_vm_get_class( vm, pOwner );
			pCurrent = pTop;
			while( pCurrent != NULL )
			{
				if( pCurrent->variables != NULL )
					handleItem = 
						mk_find_item_hashtable( pCurrent->variables, name, (void**)&pResult );
				if( pResult != NULL )
					break;

				// search superclass
				if( pCurrent->nameSuper != NULL &&
					pCurrent->nameSuper[0] != '\0' )
					pCurrent = 
						mk_vm_get_class_by_name( vm, pCurrent->nameSuper );
				else
					pCurrent = NULL;
			}
			pOwner = NULL;
		}
		else
		{
			if( pOwner->classTypeValue.variables != NULL )
			{
				handleItem = 
					mk_find_item_hashtable( pOwner->classTypeValue.variables, name, (void**)&pResult );
			}
			if( pResult != NULL )
			{
				INT_PTR extend = 
					mk_get_extend_value_hashtable( pOwner->classTypeValue.variables, handleItem, 1 );
				if( ( ( (INT_PTR)pResult & MK_VM_FRAME_ITEM_TYPE_DIRECT_VALUE ) == 0 ) &&
					( ( (INT_PTR)pResult & MK_VM_FRAME_ITEM_TYPE_DIRECT_POINTER ) != 0 ) )
				{
					pResult = 
						*(MK_VM_FRAME_ITEM**)( (INT_PTR)pResult & ~MK_VM_FRAME_ITEM_TYPE_DIRECT_POINTER );
				}
				switch( extend & MK_VM_FRAME_ITEM_EXTEND_TYPE_READ_ACCESS_MASK )
				{
				case MK_VM_FRAME_ITEM_EXTEND_TYPE_READ_PUBLIC:
					break;
				case MK_VM_FRAME_ITEM_EXTEND_TYPE_READ_PROTECTED:
					{
						MK_VM_FRAME_ITEM *pInstance = 
							mk_vm_find_instance( vm, vm->pCurrentFrame->pThis, pOwner->classTypeValue.typeName );
						if( pInstance == NULL )
							pResult = NULL;
					}
					break;

				case MK_VM_FRAME_ITEM_EXTEND_TYPE_READ_PRIVATE:
					{
						MK_CLASS *pClass = 
							mk_vm_get_class( vm, vm->pCurrentFrame->pThis );
						if( pOwner->classTypeValue.typeName != pClass->nameThis )
							pResult = NULL;
					}
					break;
				}
				break;
			}
			if( MK_TYPE_ATTRIBUTE( pOwner->flags ) == MK_VM_FRAME_ITEM_TYPE_MODULE )
			{
				mk_find_item_hashtable( pOwner->classTypeValue.variables, 
					mk_get_symbol_name_ptr( vm, "owner" ),
					(void**)&pOwner );
			}
			else
			{
				if( chSuper == NULL )
					chSuper = 
						mk_get_symbol_name_ptr( vm, "super" );
				mk_find_item_hashtable( pOwner->classTypeValue.variables, chSuper, (void**)&pOwner );
			}
		}
	}
	return pResult;
}

static
int mk_mk_method_call( MK_VM_STRUCT *vm, MK_VARIABLE *pMethod, MK_VM_FRAME *pOwnerFrame )
{
	int retCode = MK_VM_EXECUTE_EXPR_RETURN_FAILED;
	MK_VM_FRAME *newFrame = NULL;
	MK_VM_STACK *params =
		&vm->localStack;
	int size = pMethod->args != NULL ? mk_size_vector( ( pMethod->args ) ) : 0;
	int index = 0;

	newFrame =
		mk_create_vm_frame_object( &vm->pFrameTable );
	// args
	if( size > 0 )
	{
		newFrame->localVariables = 
			mk_allocate_vm_managed_hashtable( 
				MK_TYPE_ATTRIBUTE_HASH_INTERPRETER_DEFAULT, 
				MK_SIZEOF_HASH_DEFAULT, 
				&vm->pHashTable );
		for( index = size - 1; index >= 0; index -- )
		{
			// insert local variables hashtable
			mk_insert_item_hashtable(
				newFrame->localVariables,
				( (MK_VARIABLE*)mk_get_at_vector( pMethod->args, (unsigned int)index ) )->name,
				mk_vm_pop_stack( params ) );
		}
	}
	else
	{
		newFrame->localVariables = NULL;
	}

	// ptr [this] 
	newFrame->pThis = mk_vm_pop_stack( params );

	if( pMethod->entryPoint != NULL )
	{
		newFrame->previous = vm->pCurrentFrame;
		newFrame->pOwnerFrame = pOwnerFrame;
		newFrame->pMethod = pMethod;
		vm->pCurrentFrame = newFrame;

		if( MK_OBJECT_TYPE( *( (unsigned int*)pMethod->entryPoint) ) == MK_TYPE_NODE_EXPR )
		{
			vm->pCurrentFrame->flags |= MK_VM_FRAME_TYPE_NODE_WITH_PARAM;
			retCode = 
				mk_execute_expr( vm, (MK_NODE_EXPR*)pMethod->entryPoint, 0, 0 );
			if( retCode == MK_VM_EXECUTE_EXPR_RETURN_RETURN )
				retCode = MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP;
		}
		else	// MK_OBJECT_TYPE( pMethod->impl->flags ) == MK_NODE_BLOCK )
		{
			if( ( MK_TYPE_ATTRIBUTE( *( (unsigned int*)pMethod->entryPoint) ) == MK_TYPE_ATTRIBUTE_BLOCK_NONAME_METHOD ) ||
				( MK_TYPE_ATTRIBUTE( *( (unsigned int*)pMethod->entryPoint) ) == MK_TYPE_ATTRIBUTE_BLOCK_CATCH_BLOCK ) )
				newFrame->flags |= MK_VM_FRAME_TYPE_NODE_WITH_PARAM;
			retCode = mk_execute_block( vm, ( (unsigned int*)pMethod->entryPoint) );
			if( ( ( MK_TYPE_ATTRIBUTE( *( (unsigned int*)pMethod->entryPoint) ) == MK_TYPE_ATTRIBUTE_BLOCK_FUNCTION_ROOT ) ||
				( MK_TYPE_ATTRIBUTE( *( (unsigned int*)pMethod->entryPoint) ) == MK_TYPE_ATTRIBUTE_BLOCK_NONAME_METHOD ) ) &&
				( retCode == MK_VM_EXECUTE_EXPR_RETURN_RETURN ) )
				retCode = MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP;
		}
		vm->pCurrentFrame = vm->pCurrentFrame->previous;
	}
	else
	{
		mk_vm_push_stack( &vm->localStack, 
			(MK_VM_FRAME_ITEM*)MK_VM_FRAME_ITEM_TYPE_DIRECT_VALUE );
		retCode = MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP;
	}
	return retCode;
}

static
int mk_method_push_parameters( MK_VM_STRUCT *vm, MK_VARIABLE *pMethod, MK_VECTOR *parameters, MK_VM_FRAME_ITEM *pMethodOwner )
{
	unsigned int index = 0, 
		size = parameters != NULL ? mk_size_vector( parameters ) : 0,
		targetSize = 0, 
		argResult = MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP;

	mk_vm_push_stack( 
		&vm->localStack, 
		pMethodOwner );
	targetSize = pMethod->args != NULL ? mk_size_vector( pMethod->args ) : 0;
	for( index = 0; index < size; index ++ )
	{
		MK_NODE_EXPR *pTarget = 
			(MK_NODE_EXPR *)mk_get_at_vector( parameters, index );
		argResult = 
			mk_execute_expr( vm, pTarget, 0, 0 );
		if( argResult != MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP )
			break;	// error
	}
	if( argResult == MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP )
	{
		for( index = size; index < targetSize; index ++ )
		{
			MK_VARIABLE *arg = 
				(MK_VARIABLE *)mk_get_at_vector( pMethod->args, index );
			if( arg->defaultValue == NULL )
			{
				argResult = MK_VM_EXECUTE_EXPR_THROW;// Todo :not set default value
				break;
			}
			argResult = 
				mk_execute_expr( vm, arg->defaultValue, 0, 0 );
			if( argResult != MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP )
				break;	// error
		}
	}
	return argResult;
}

int mk_vm_call_method( MK_VM_STRUCT *vm, MK_VARIABLE *pMethod, MK_VM_FRAME *pOwnerFrame, int sizeVarArgs )
{
	if( pMethod->flags & MK_TYPE_ATTRIBUTE_VARIABLE_METHOD_NATIVE )
		return ( (FP_MK_NATIVE_METHOD)(pMethod->entryPoint) ) ( 
			vm,
			sizeVarArgs );
	else
		return mk_mk_method_call(
			vm,
			pMethod,
			pOwnerFrame );
}

static
MK_VM_FRAME_ITEM *mk_vm_get_super_class_instance( MK_VM_STRUCT *vm, MK_VM_FRAME_ITEM *pTarget )
{
	MK_VM_FRAME_ITEM *result = NULL;
	if( ( (INT_PTR)pTarget & MK_VM_FRAME_ITEM_TYPE_DIRECT_VALUE ) ||
		MK_TYPE_ATTRIBUTE( pTarget->flags ) != MK_VM_FRAME_ITEM_TYPE_CLASS )
	{
		result = 
			mk_create_vm_frame_item_object( &vm->pFrameItemTable );
		result->flags = 
			MK_TYPE_SET_ATTRIBUTE( result->flags, MK_VM_FRAME_ITEM_TYPE_CLASS );
		result->classTypeValue.typeName = 
			mk_get_symbol_name_ptr( vm, "Object" );
		result->classTypeValue.child = pTarget;
	}
	else
	{
		mk_find_item_hashtable( pTarget->classTypeValue.variables,
			mk_get_symbol_name_ptr( vm, "super" ), 
			(void**)&result );
	}
	return result;
}

static
int mk_execute_expr_raise( MK_VM_STRUCT *vm, MK_NODE_EXPR *pExpr, int hasParent, int reference )
{
	int retCode = mk_execute_expr( vm, pExpr->u1.left, 0, 0 );
#ifdef _DEBUG_DUMP
	fprintf( stdout, "raise\n" );
#endif
	if( hasParent != 0 )
		return mk_raise_invalidate_parent_error( vm );

	if( retCode == MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP )
	{
		MK_VM_FRAME_ITEM *pException = 
			mk_vm_pop_stack( &vm->localStack );
		vm->exceptionObject = (MK_VM_FRAME_ITEM*)pException;
		return MK_VM_EXECUTE_EXPR_THROW;
	}
	else
	{
		return retCode;
	}
}

static
int mk_execute_expr_operation( MK_VM_STRUCT *vm, MK_NODE_EXPR *pExpr, int hasParent, int reference )
{
	MK_NODE_EXPR *pLeft = pExpr->u1.left;
	MK_NODE_EXPR *pRight = pExpr->u2.right;
	int retCodeLeft = 0;
	int retCodeRight = 0;
	MK_VM_FRAME_ITEM *pLeftValue = NULL;
	int isLeftReference = 0;
	MK_CLASS *pClass = NULL;
	int retCode = 0;
	MK_VARIABLE *pMethod = NULL;

#ifdef _DEBUG_DUMP
	fprintf( stdout, "call %s\n", operation_int_to_string( pExpr->flags ) );
#endif
	if( hasParent != 0 )
		return mk_raise_invalidate_parent_error( vm );

	if( MK_RESERVED_MARK_INDEX( pExpr->flags ) == MK_LEX_RESERVED_MARK_INDEX( MK_LEX_TYPE_RESERVED_MARK_EQUAL ) )
		isLeftReference = 1;
	retCodeLeft = 
		mk_execute_expr( 
			vm, 
			pLeft, 
			0, 
			isLeftReference );	// ptr[this]
	if( retCodeLeft != MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP )
		return retCodeLeft;	// error
	retCodeRight =
		mk_execute_expr( vm, pRight, 0, 0 );	// args[0]
	if( retCodeRight != MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP )
		return retCodeRight;	// error

	pLeftValue  = 
		(MK_VM_FRAME_ITEM*)mk_vm_get_at_stack( &vm->localStack, 1 );
	pClass = mk_vm_get_class( vm, pLeftValue );
	if( pClass == NULL )
		return MK_VM_EXECUTE_EXPR_THROW;	// set error object in mk_vm_get_class

	if( reference == 1 && 
		MK_RESERVED_MARK_INDEX( pExpr->flags ) == MK_LEX_RESERVED_MARK_INDEX( MK_LEX_TYPE_RESERVED_MARK_BRACKET ) )
		pMethod = 
			mk_vm_find_operator_method( vm,
				pClass, 
				MK_RESERVED_MARK_INDEX( MK_LEX_TYPE_RESERVED_MARK_BRACKET_REF ) );
	else
		pMethod = 
			mk_vm_find_operator_method( vm,
				pClass, 
				MK_RESERVED_MARK_INDEX( pExpr->flags ) );
	if( pMethod == NULL )
		return MK_VM_EXECUTE_EXPR_THROW;	// set error object in mk_vm_find_operator_method

	return mk_vm_call_method( vm, pMethod, NULL, 0 );
}

static
int mk_execute_expr_owner( MK_VM_STRUCT *vm, MK_NODE_EXPR *pExpr, int hasParent, int reference )
{
	MK_VM_FRAME_ITEM *pResult = 
		mk_create_vm_frame_item_object( &vm->pFrameItemTable );
#ifdef _DEBUG_DUMP
	fprintf( stdout, "owner\n" );
#endif
	if( hasParent != 0 )
		return mk_raise_invalidate_parent_error( vm );

	pResult->flags = 
		MK_TYPE_SET_ATTRIBUTE( pResult->flags, MK_VM_FRAME_ITEM_TYPE_OWNER );
	mk_vm_push_stack( 
		&vm->localStack, 
		pResult );
	return MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP;
}

static
int mk_execute_expr_this( MK_VM_STRUCT *vm, MK_NODE_EXPR *pExpr, int hasParent, int reference )
{
	MK_VM_FRAME_ITEM *pResult = NULL;
#ifdef _DEBUG_DUMP
	fprintf( stdout, "this\n" );
#endif
	if( hasParent == 0 )
		mk_vm_push_stack( 
			&vm->localStack, 
			vm->pCurrentFrame->pThis );
	return MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP;
}

int mk_execute_expr_super( MK_VM_STRUCT *vm, MK_NODE_EXPR *pExpr, int hasParent, int reference )
{
	MK_VM_FRAME_ITEM *pThis = NULL, *pSuper = NULL;
#ifdef _DEBUG_DUMP
	fprintf( stdout, "super\n" );
#endif
	if( hasParent == 0 )
		pThis = vm->pCurrentFrame->pThis;
	else
		pThis = mk_vm_pop_stack( &vm->localStack );

	if( ( INT_PTR )pThis & MK_VM_FRAME_ITEM_TYPE_DIRECT_VALUE )
	{
		return mk_raise_internal_error( vm, "", 0, MK_ERROR_TYPE_VM_ERROR | 13, NULL );
	}
	else
	{
		mk_find_item_hashtable( 
			pThis->classTypeValue.variables, 
			mk_get_symbol_name_ptr( vm, "super" ), 
			(void**)&pSuper );
	}

	if( pSuper != NULL )
		mk_vm_push_stack( 
			&vm->localStack, 
			pSuper );
	else
		return mk_raise_internal_error( vm, "", 0, 
									MK_ERROR_TYPE_VM_ERROR | 1,
									"super",
									pThis->classTypeValue.typeName, NULL );

	return MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP;
}

static
int mk_execute_expr_constant( MK_VM_STRUCT *vm, MK_NODE_EXPR *pExpr, int hasParent, int reference )
{
	MK_VM_FRAME_ITEM *result = NULL;

#ifdef _DEBUG_DUMP
	fprintf( stdout, "loadconst %08x-%08x\n", pExpr->u1.constantValue, pExpr->u2.constantValue );
#endif

	if( hasParent != 0 )
		return mk_raise_invalidate_parent_error( vm );

	if( ( MK_TYPE_ATTRIBUTE( pExpr->flags ) & MK_TYPE_NODE_EXPR_CONSTANT ) == MK_TYPE_NODE_EXPR_CONSTANT )
	{
		switch( MK_TYPE_ATTRIBUTE( pExpr->flags ) )
		{
		case MK_TYPE_NODE_EXPR_INT32:
			result = mk_vm_create_int32_frame_item( vm, pExpr->u2.constantValue );
			break;

		case MK_TYPE_NODE_EXPR_INT64:
			// not supported yet.
			break;

		case MK_TYPE_NODE_EXPR_FLOAT:
			result =
				mk_vm_create_float_frame_item( vm, pExpr->floatValue );
			break;

		case MK_TYPE_NODE_EXPR_STRING:
			result = 
				mk_create_vm_frame_item_object( &vm->pFrameItemTable );
			result->flags |= MK_VM_FRAME_ITEM_TYPE_STRING_VALUE;
			if(  pExpr->u2.value != NULL )
				mk_copy_string( &result->stringTypeValue, pExpr->u2.value );
			else
				result->stringTypeValue = NULL;
			break;

		}
	}
	mk_vm_push_stack( 
		&vm->localStack, 
		result );
	if( result != NULL )
	{
		return MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP;
	}
	else
	{
		MK_CHAR buffer[sizeof(void*)*2+3];
		sprintf( buffer, "0x%x", pExpr->flags );
		return mk_raise_internal_error( vm, "", 0, 
									MK_ERROR_TYPE_VM_ERROR | 7,
									buffer, "NodeExpr", NULL );
	}
}

static 
int mk_execute_expr_return( MK_VM_STRUCT *vm, MK_NODE_EXPR *pExpr, int hasParent, int reference )
{
	int retCode = MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP;
#ifdef _DEBUG_DUMP
	fprintf( stdout, "retrun\n" );
#endif

	if( hasParent != 0 )
		return mk_raise_invalidate_parent_error( vm );

	retCode = mk_execute_expr( vm, pExpr->u1.left, 0, 0 );
	if( retCode == MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP )
		retCode = MK_VM_EXECUTE_EXPR_RETURN_RETURN;
	return retCode;
}

static 
int mk_execute_expr_break( MK_VM_STRUCT *vm, MK_NODE_EXPR *pExpr, int hasParent, int reference )
{
#ifdef _DEBUG_DUMP
	fprintf( stdout, "break\n" );
#endif

	if( hasParent != 0 )
		return mk_raise_invalidate_parent_error( vm );

	return MK_VM_EXECUTE_EXPR_RETURN_BREAK;
}

static 
int mk_execute_expr_continue( MK_VM_STRUCT *vm, MK_NODE_EXPR *pExpr, int hasParent, int reference )
{
#ifdef _DEBUG_DUMP
	fprintf( stdout, "continue\n" );
#endif

	if( hasParent != 0 )
		return mk_raise_invalidate_parent_error( vm );

	return MK_VM_EXECUTE_EXPR_RETURN_CONTINUE;
}


static
int mk_execute_expr_symbol( MK_VM_STRUCT *vm, MK_NODE_EXPR *pExpr, int hasParent, int reference )
{
	MK_CHAR *symbolName = pExpr->u1.symbolName;
	MK_VM_FRAME *pCurrentFrame = 
		vm->pCurrentFrame;
	MK_VM_FRAME_ITEM *result = NULL;
	void* handle = 0;
	MK_VM_FRAME_ITEM *pOwner = NULL;

#ifdef _DEBUG_DUMP
	fprintf( stdout, "loadsym %s\n", pExpr->u1.symbolName );
#endif

	if( hasParent != 0 )
	{
		pOwner = 
			mk_vm_pop_stack( &vm->localStack );

		if( (INT_PTR)pOwner != MK_VM_FRAME_ITEM_TYPE_DIRECT_VALUE &&
			MK_TYPE_ATTRIBUTE(pOwner->flags) == MK_VM_FRAME_ITEM_TYPE_OWNER )
		{
			// change context to owner.
			pCurrentFrame = 
				( pCurrentFrame->pOwnerFrame != NULL ) ? pCurrentFrame->pOwnerFrame : pCurrentFrame;
			pOwner = NULL;
		}
	}
	if( pOwner == NULL )
	{
		do
		{
			// search local variables(and arguments).
			if( pCurrentFrame->localVariables != NULL )
			{
				handle = 
					mk_find_item_hashtable( pCurrentFrame->localVariables, symbolName, (void**)&result );
				if( handle != NULL )
				{
					if( reference != 0 )
					{
						INT_PTR extend = 
							mk_get_extend_value_hashtable( pCurrentFrame->localVariables, handle, 1 );
						result = mk_create_vm_frame_item_object( &vm->pFrameItemTable );
						result->flags |= MK_VM_FRAME_ITEM_TYPE_REFERENCE_VALUE;
						if( extend & MK_VM_FRAME_ITEM_EXTEND_TYPE_FINAL )
							result->flags |= MK_VM_FRAME_ITEM_TYPE_FINAL;
						result->referenceTypeValue.target = pCurrentFrame->localVariables;
						result->referenceTypeValue.symbolName = symbolName;
					}
					break;
				}
			}

			// search owner variables
			if( pCurrentFrame->pOwnerFrame != NULL &&
				pCurrentFrame->pOwnerFrame->localVariables != NULL )
			{
				handle = mk_find_item_hashtable( pCurrentFrame->pOwnerFrame->localVariables, symbolName, (void**)&result );
				if( handle != NULL )
				{
					if( reference != 0 )
					{
						INT_PTR extend = 
							mk_get_extend_value_hashtable( pCurrentFrame->pOwnerFrame->localVariables, handle, 1 );
						result = mk_create_vm_frame_item_object( &vm->pFrameItemTable );
						result->flags |= MK_VM_FRAME_ITEM_TYPE_REFERENCE_VALUE;
						if( extend & MK_VM_FRAME_ITEM_EXTEND_TYPE_FINAL )
							result->flags |= MK_VM_FRAME_ITEM_TYPE_FINAL;
						result->referenceTypeValue.target = pCurrentFrame->pOwnerFrame->localVariables;
						result->referenceTypeValue.symbolName = symbolName;
					}
					break;
				}
			}

			// search global variables
			if( hasParent == 0 )
			{
				handle =
					mk_find_item_hashtable( vm->global, symbolName, (void**)&result );
				if( handle != 0 )
				{
					if( reference != 0 )
					{
						INT_PTR extend = 
							mk_get_extend_value_hashtable( vm->global, handle, 1 );
						result = mk_create_vm_frame_item_object( &vm->pFrameItemTable );
						result->flags |= MK_VM_FRAME_ITEM_TYPE_REFERENCE_VALUE;
						if( extend & MK_VM_FRAME_ITEM_EXTEND_TYPE_FINAL )
							result->flags |= MK_VM_FRAME_ITEM_TYPE_FINAL;
						result->referenceTypeValue.target = vm->global;
						result->referenceTypeValue.symbolName = symbolName;
					}
					break;
				}
			}

			// create new local variable.
			result = (MK_VM_FRAME_ITEM*)MK_VM_FRAME_ITEM_TYPE_DIRECT_VALUE;
			if( pCurrentFrame->localVariables == NULL )
				pCurrentFrame->localVariables = 
					mk_allocate_vm_managed_hashtable( 
						MK_TYPE_ATTRIBUTE_HASH_INTERPRETER_DEFAULT, 
						MK_SIZEOF_HASH_DEFAULT, 
						&vm->pHashTable );
			handle = 
				mk_insert_item_hashtable(
					pCurrentFrame->localVariables, 
					symbolName, 
					result );
			mk_set_extend_value_hashtable( pCurrentFrame->localVariables, handle, 1, 0 );
			if( reference != 0 )
			{
				INT_PTR extend = 
					mk_get_extend_value_hashtable( pCurrentFrame->localVariables, handle, 1 );
				result = mk_create_vm_frame_item_object( &vm->pFrameItemTable );
				result->flags |= MK_VM_FRAME_ITEM_TYPE_REFERENCE_VALUE;
				result->referenceTypeValue.target = pCurrentFrame->localVariables;
				result->referenceTypeValue.symbolName = symbolName;
			}
		}
		while( 0 );
		mk_vm_push_stack( 
			&vm->localStack, 
			result );
		return MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP;
	}
	else
	{
		return mk_raise_invalidate_parent_error( vm );
	}
}

static
int mk_execute_expr_multiple( MK_VM_STRUCT *vm, MK_NODE_EXPR *pExpr, int hasParent, int reference )	// hasParent need 0
{
	unsigned int retCode = MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP;
	MK_VECTOR *multiple = 
		pExpr->u1.multipleSymbols;
	unsigned int size = multiple != NULL ? mk_size_vector( multiple ) : 0;
	unsigned int index = 0;

#ifdef _DEBUG_DUMP
	fprintf( stdout, "multiple->\n" );
#endif

	if( hasParent != 0 )
		return mk_raise_invalidate_parent_error( vm );

	for( index = 0; index < size; index ++ )
	{
		MK_NODE_EXPR *pTarget = 
			(MK_NODE_EXPR *)mk_get_at_vector( multiple, index );
		retCode = 
			mk_execute_expr( 
				vm, 
				pTarget,
				hasParent,
				reference != 0 && index == size - 1 );
		if( retCode != MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP )
			break;
		hasParent = 1;
	}
	return retCode;
}

static
int mk_execute_expr_dbl_at_symbol( MK_VM_STRUCT *vm, MK_NODE_EXPR *pExpr, int hasParent, int reference )
{
	MK_CHAR *symbolName = pExpr->u1.symbolName;
	MK_VM_FRAME_ITEM *result = NULL;
	MK_VM_FRAME_ITEM *pOwner = ( hasParent == 0 ) ? 
		pOwner = vm->pCurrentFrame->pThis : 
			mk_vm_pop_stack( &vm->localStack );

#ifdef _DEBUG_DUMP
	fprintf( stdout, "loadsym @@%s\n", pExpr->u1.symbolName );
#endif

	if( ( (INT_PTR)pOwner & ( MK_VM_FRAME_ITEM_TYPE_DIRECT_VALUE | MK_VM_FRAME_ITEM_TYPE_DIRECT_POINTER ) ) != 0 )
		return MK_VM_EXECUTE_EXPR_THROW;	// error;
	if( !( pOwner->flags & MK_VM_FRAME_ITEM_TYPE_STATIC_INSTANCE ) )
		return MK_VM_EXECUTE_EXPR_THROW;	// error;

	if( reference == 0 )
		result = 
			mk_vm_find_variable( vm, pOwner, symbolName );
	else
		result = 
			mk_vm_find_variable_reference( vm, pOwner, symbolName );
	if( result == NULL )
	{
		MK_CLASS *pClass = mk_vm_get_class( vm, pOwner );
		return mk_raise_internal_error( vm, "", 0, 
			MK_ERROR_TYPE_VM_ERROR | 1, 
			symbolName, pClass->nameThis, NULL );
	}
	else
	{
		mk_vm_push_stack( 
			&vm->localStack, 
			result );
	}
	return MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP;
}

static
int mk_execute_expr_at_symbol( MK_VM_STRUCT *vm, MK_NODE_EXPR *pExpr, int hasParent, int reference )
{
	MK_CHAR *symbolName = pExpr->u1.symbolName;
	MK_VM_FRAME_ITEM *result = NULL;
	MK_VM_FRAME_ITEM *pOwner = ( hasParent == 0 ) ? 
		pOwner = vm->pCurrentFrame->pThis : 
			mk_vm_pop_stack( &vm->localStack );

#ifdef _DEBUG_DUMP
	fprintf( stdout, "loadsym @%s\n", pExpr->u1.symbolName );
#endif

	if( ( (INT_PTR)pOwner & ( MK_VM_FRAME_ITEM_TYPE_DIRECT_VALUE | MK_VM_FRAME_ITEM_TYPE_DIRECT_POINTER ) ) != 0 )
		return MK_VM_EXECUTE_EXPR_THROW;	// error;
	if( pOwner->flags & MK_VM_FRAME_ITEM_TYPE_STATIC_INSTANCE )
		return MK_VM_EXECUTE_EXPR_THROW;	// error;

	if( reference == 0 )
		result = 
			mk_vm_find_variable( vm, pOwner, symbolName );
	else
		result = 
			mk_vm_find_variable_reference( vm, pOwner, symbolName );
	if( result == NULL )
	{
		MK_CLASS *pClass = mk_vm_get_class( vm, pOwner );
		return mk_raise_internal_error( vm, "", 0, 
			MK_ERROR_TYPE_VM_ERROR | 1, 
			symbolName, pClass->nameThis, NULL );
	}
	else
	{
		mk_vm_push_stack( 
			&vm->localStack, 
			result );
	}
	return MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP;
}

static
int mk_execute_expr_array_definition( MK_VM_STRUCT *vm, MK_NODE_EXPR *pExpr, int hasParent, int reference )
{
	MK_VM_FRAME_ITEM *newItem = NULL;
	MK_VECTOR *newArray = NULL, *definedArray = NULL;
	unsigned int size = 0, index = 0;
	int retCode = MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP;

#ifdef _DEBUG_DUMP
	fprintf( stdout, "DEF_ARRAY %s\n", pExpr->u1.symbolName );
#endif
	if( hasParent != 0 )
		return mk_raise_invalidate_parent_error( vm );
	
	definedArray = pExpr->u1.arrayDefinition;
	size = definedArray != NULL ? mk_size_vector( definedArray ) : 0;
	newArray = 
		mk_allocate_vm_managed_vector(
			&vm->pVectorTable, 
			MK_TYPE_VECTOR | MK_TYPE_VECTOR_ARRAY_MANAGED_PTR | ( MK_SIZEOF_EXTEND_VECTOR_DEFAULT << 10 ),
			MK_SIZEOF_VECTOR_DEFAULT,
			(INT_PTR)NULL );
	for( index = 0; index < size; index ++ )
	{
		MK_VM_FRAME_ITEM *targetItem = NULL;
		retCode = 
			mk_execute_expr( vm, 
				(MK_NODE_EXPR*)mk_get_at_vector( definedArray, index ), 0, 0 );
		if( retCode != MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP )
			break;
		targetItem = 
			mk_vm_pop_stack( &vm->localStack );
		mk_push_vector( newArray, (INT_PTR)targetItem );
	}
	if( index == size )
	{
		newItem = mk_create_vm_frame_item_object( &vm->pFrameItemTable );
		newItem->flags |= MK_VM_FRAME_ITEM_TYPE_ARRAY_VALUE;
		newItem->arrayTypeValue = newArray;
	}
	mk_vm_push_stack( 
		&vm->localStack, 
		newItem );
	return MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP;
}

static
int mk_execute_expr_node_with_param_definition( MK_VM_STRUCT *vm, MK_NODE_EXPR *pExpr, int hasParent, int reference )
{
	MK_VM_FRAME_ITEM *result = NULL;

	if( hasParent != 0 )
		return mk_raise_invalidate_parent_error( vm );

	result = 
		mk_create_vm_frame_item_object( &vm->pFrameItemTable );
	result->flags = 
		MK_TYPE_SET_ATTRIBUTE( result->flags, MK_VM_FRAME_ITEM_TYPE_NODE_VALUE );
	result->code.definedFrame = vm->pCurrentFrame;
	result->code.pOwner = vm->pCurrentFrame->pThis;
	result->code.node = pExpr->u1.node;

	mk_vm_push_stack( &vm->localStack, result );
	return MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP;
}

int mk_execute_expr_new( MK_VM_STRUCT *vm, MK_NODE_EXPR *pExpr, int hasParent, int reference )
{
	int retCode = 0;
	MK_VM_FRAME_ITEM *pOwner = NULL, *pInstance = NULL;
	
#ifdef _DEBUG_DUMP
	fprintf( stdout, "new\n" );
#endif

	// find target class
	if( hasParent == 0 )
	{
		pOwner = vm->pCurrentFrame->pThis;
		mk_vm_push_stack( 
			&vm->localStack, 
			pOwner );
	}
	else
	{
		pOwner = (MK_VM_FRAME_ITEM*)mk_vm_get_at_stack( 
			&vm->localStack, 
			0 );
	}

	return
		mk_create_object_instance( vm, pExpr->u2.args, &pInstance );
}

static
int mk_execute_expr_call_method( MK_VM_STRUCT *vm, MK_NODE_EXPR *pExpr, int hasParent, int reference )
{
	MK_VM_FRAME *pOwnerFrame = NULL;
	MK_VM_FRAME_ITEM *pOwner = NULL, *pMethodOwner = NULL;
	MK_VARIABLE *pMethod = NULL;
	unsigned int size = 
		( pExpr->u2.args != NULL ) ? mk_size_vector( pExpr->u2.args ) : 0;

#ifdef _DEBUG_DUMP
	fprintf( stdout, "call %s\n", pExpr->u1.symbolName );
#endif

	if( hasParent == 0 )
		pOwner = vm->pCurrentFrame->pThis;
	else
		pOwner = mk_vm_pop_stack( &vm->localStack );

	if( MK_TYPE_ATTRIBUTE( pExpr->flags ) == MK_TYPE_NODE_EXPR_FUNCTION_CALL )
	{
		MK_VM_FRAME_ITEM *result = NULL;
		void* retCode = 0;

		// search method in local variables of current-frame.
		if( hasParent == 0 &&
			vm->pCurrentFrame->localVariables != NULL )
			retCode = mk_find_item_hashtable( 
				vm->pCurrentFrame->localVariables, 
				pExpr->u1.symbolName, 
				(void**)&result );

		// search method in localvariables of owner-frame
		if( retCode == 0 )
		{
			if( hasParent == 0 ||
				( !( (INT_PTR)pOwner & MK_VM_FRAME_ITEM_TYPE_DIRECT_VALUE ) &&
				MK_TYPE_ATTRIBUTE( pOwner->flags ) == MK_VM_FRAME_ITEM_TYPE_OWNER ) )
			{
				if( vm->pCurrentFrame->pOwnerFrame != NULL )
					retCode = mk_find_item_hashtable( 
						vm->pCurrentFrame->pOwnerFrame->localVariables, 
						pExpr->u1.symbolName, 
						(void**)&result );
			}
		}

		if( retCode != 0 )
		{
			if( result != NULL &&
				( !( (INT_PTR)result & MK_VM_FRAME_ITEM_TYPE_DIRECT_VALUE ) ) )
			{
				if( MK_TYPE_ATTRIBUTE( result->flags ) == MK_VM_FRAME_ITEM_TYPE_NODE_VALUE )
				{
					pMethod = (MK_VARIABLE*)result->code.node;
					pOwnerFrame = result->code.definedFrame;
				}
				pMethodOwner = result->code.pOwner;
			}
		}
		else
		{
			// search method in variables of pOwner
			pMethod = 
				mk_vm_find_method( vm, 
					pOwner,
					pExpr->u1.symbolName, 
					size );
			pMethodOwner = pOwner;
		}
	}
	else if( MK_TYPE_ATTRIBUTE( pExpr->flags ) == MK_TYPE_NODE_EXPR_FUNCTION_CALL_INSTANCE )
	{
		// search method in variables of pOwner
		pMethod = 
			mk_vm_find_method( vm, 
				pOwner,
				pExpr->u1.symbolName, 
				size );

		pMethodOwner = pOwner;
	}
	else if( MK_TYPE_ATTRIBUTE( pExpr->flags ) == MK_TYPE_NODE_EXPR_FUNCTION_CALL_STATIC )
	{
		// search method in variables of pOwner
		pMethod = 
			mk_vm_find_method( vm, 
				pOwner,
				pExpr->u1.symbolName, 
				size );

		pMethodOwner = pOwner;
	}

	if( pMethod == NULL )
	{
		MK_CLASS *pClass = 
			mk_vm_get_class( vm, pOwner );
		return mk_raise_internal_error( vm, "", 0, 
									MK_ERROR_TYPE_VM_ERROR | 1, 
									pExpr->u1.symbolName, pClass->nameThis, NULL);
	}

	if( mk_method_push_parameters( vm, pMethod, pExpr->u2.args, pMethodOwner ) == MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP )
	{
		int sizeParameters = 0, sizeArguments = 0;
		sizeArguments = 
			( pExpr->u2.args != NULL ) ? mk_size_vector( pExpr->u2.args ) : 0;
		sizeParameters = 
			( pMethod->args != NULL ) ? mk_size_vector( pMethod->args ) : 0;
		return mk_vm_call_method( vm, pMethod, pOwnerFrame, sizeArguments - sizeParameters );
	}
	else
	{
		if( vm->exceptionObject == NULL )
			return mk_raise_internal_error( vm, "", 0, MK_ERROR_TYPE_VM_ERROR | 13, NULL );
		else
			return MK_VM_EXECUTE_EXPR_THROW; 
	}
}

int mk_execute_expr_super_call( MK_VM_STRUCT *vm, MK_NODE_EXPR *pExpr, int hasParent, int reference )
{
	MK_VM_FRAME_ITEM *pOwner = NULL, *pTop = NULL;
	MK_VARIABLE *pMethod = NULL;

	if( hasParent == 0 )
		pTop = vm->pCurrentFrame->pThis;
	else
		pTop = mk_vm_pop_stack( &vm->localStack );

	pOwner = 
		mk_vm_get_super_class_instance( vm, pTop );
	if( pOwner == NULL )
	{
		MK_CLASS *pClass = 
			mk_vm_get_class( vm, pTop );
		return mk_raise_internal_error( vm, "", 0, MK_ERROR_TYPE_VM_ERROR | 1, "super", pClass->nameThis );
	}

	pMethod = 
		mk_vm_find_method( vm, 
			pOwner, 
			vm->pCurrentFrame->pMethod->name, 
			pExpr->u2.args != NULL ? mk_size_vector( pExpr->u2.args ) : 0 );
	if( pMethod == NULL )
		return MK_VM_EXECUTE_EXPR_THROW;	// set error object in mk_vm_find_function

	if( mk_method_push_parameters( vm, pMethod, pExpr->u2.args, pOwner ) == MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP )
	{
		int sizeParameters = 0, sizeArguments = 0;
		sizeArguments = 
			( pExpr->u2.args != NULL ) ? mk_size_vector( pExpr->u2.args ) : 0;
		sizeParameters = 
			( pMethod->args != NULL ) ? mk_size_vector( pMethod->args ) : 0;
		return mk_vm_call_method( vm, pMethod, NULL, sizeArguments - sizeParameters );
	}
	else
	{
		MK_CLASS *pClass = 
			mk_vm_get_class( vm, pOwner );
		return mk_raise_internal_error( vm, "", 0, MK_ERROR_TYPE_VM_ERROR | 1, "super", pClass->nameThis );
	}
}

static
int mk_execute_expr_back_if_condition( MK_VM_STRUCT *vm, MK_NODE_EXPR *pExpr, int hasParent, int reference )
{
	int retCode = MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP;

	if( hasParent != 0 )
		return mk_raise_invalidate_parent_error( vm );

	retCode = mk_execute_expr( vm, pExpr->u2.right, 0, 0 );
	if( retCode == MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP )
	{
		if( mk_vm_frame_item_is_true( mk_vm_get_at_stack( &vm->localStack, 0 ) ) != 0 )
		{
			mk_vm_pop_stack( &vm->localStack );
			retCode = mk_execute_expr( vm, pExpr->u1.left, 0, 0 );
		}
	}
	return retCode;
}

static
int mk_execute_expr_me( MK_VM_STRUCT *vm, MK_NODE_EXPR *pExpr, int hasParent, int reference )
{
	int retCode = MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP;
	MK_VARIABLE *pMethod = vm->pCurrentFrame->pMethod;

	if( hasParent != 0 )
		return mk_raise_invalidate_parent_error( vm );

	if( mk_method_push_parameters( vm, pMethod, pExpr->u2.args, vm->pCurrentFrame->pThis ) 
		== MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP )
	{
		int sizeParameters = 0, sizeArguments = 0;
		sizeArguments = 
			( pExpr->u2.args != NULL ) ? mk_size_vector( pExpr->u2.args ) : 0;
		sizeParameters = 
			( pMethod->args != NULL ) ? mk_size_vector( pMethod->args ) : 0;
		return mk_vm_call_method( vm, pMethod, NULL, sizeArguments - sizeParameters );
	}
	else
	{
		if( vm->exceptionObject == NULL )
			return mk_raise_internal_error( vm, "", 0, MK_ERROR_TYPE_VM_ERROR | 13, NULL );
		else
			return MK_VM_EXECUTE_EXPR_THROW; 
	}
}
