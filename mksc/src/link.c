#include "mk.h"

static
int register_class_static_instance( MK_VM_STRUCT *vm, MK_CLASS *pClass )
{
	MK_VM_FRAME_ITEM *newVariable = NULL;
	void *handleVariables = NULL;
	const MK_CHAR *keyVariable = NULL;
	MK_VARIABLE *valueVariable = NULL;
	MK_VM_FRAME_ITEM *defaultValue = NULL;
	void *handleNewClass = NULL;

	if (mk_find_item_hashtable(vm->global, pClass->nameThis, &newVariable) == 0)
	{
		// create new class

		// register new instance.
		newVariable =
			mk_create_vm_frame_item_object( &vm->pFrameItemTable );
		if( MK_TYPE_ATTRIBUTE( pClass->flags ) & MK_TYPE_ATTRIBUTE_CLASS_MODULE )
			newVariable->flags |= MK_VM_FRAME_ITEM_TYPE_MODULE;
		newVariable->flags |= 
			MK_VM_FRAME_ITEM_TYPE_CLASS | MK_VM_FRAME_ITEM_TYPE_STATIC_INSTANCE;
		newVariable->classTypeValue.typeName = pClass->nameThis;
		newVariable->classTypeValue.child = NULL;
		newVariable->classTypeValue.variables = 
			mk_allocate_vm_managed_hashtable( 
				MK_TYPE_ATTRIBUTE_HASH_INTERPRETER_DEFAULT, 
				MK_SIZEOF_HASH_DEFAULT, 
				&vm->pHashTable );
		handleNewClass = mk_insert_item_hashtable(
			vm->global, 
			pClass->nameThis, 
			newVariable );
		mk_set_extend_value_hashtable( vm->global, handleNewClass, 1, MK_VM_FRAME_ITEM_EXTEND_TYPE_FINAL );

		// set super class
		if( pClass->nameSuper != NULL )
		{
			void *handleSuper = NULL;
			mk_find_item_hashtable( 
				vm->global, 
				pClass->nameSuper,
				(void**)&defaultValue );
			handleSuper = mk_insert_item_hashtable(
				newVariable->classTypeValue.variables,
				mk_get_symbol_name_ptr( vm, "super" ),
				defaultValue );
			mk_set_extend_value_hashtable( newVariable->classTypeValue.variables, handleSuper, 1, MK_VM_FRAME_ITEM_EXTEND_TYPE_FINAL );
		}

		if( MK_TYPE_ATTRIBUTE( pClass->flags ) & MK_TYPE_ATTRIBUTE_CLASS_MODULE )
		{
			MK_VM_FRAME_ITEM *pOwner = NULL;
			void *handleOwner = NULL;
			mk_find_item_hashtable( vm->global, 
				mk_get_symbol_name_ptr( vm, CLASS_INTERNAL_OBJECT ), (void**)&pOwner );
			handleOwner = mk_insert_item_hashtable( newVariable->classTypeValue.variables, 
				mk_get_symbol_name_ptr( vm, "owner" ), pOwner );
			mk_set_extend_value_hashtable( newVariable->classTypeValue.variables, handleOwner, 1, MK_VM_FRAME_ITEM_EXTEND_TYPE_FINAL );
		}
	}
	if( mk_vm_initialize_object_variables( vm, pClass, newVariable, 1 ) == 0 )
		return MK_VM_EXECUTE_EXPR_THROW;

	return MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP;
}

int mk_link_register_class_static_instance(MK_VM_STRUCT *vm) {
	void *it = mk_enum_item_hashtable_begin(vm->code->classes);

	// create global instance
	if( vm->global == NULL )
		vm->global = 
			mk_allocate_vm_managed_hashtable( 
				MK_TYPE_ATTRIBUTE_HASH_INTERPRETER_DEFAULT, 
				MK_SIZEOF_HASH_DEFAULT, 
				&vm->pHashTable );

	while (it != NULL)
	{
		MK_CHAR *key = NULL;
		MK_CLASS *clazz = NULL;

		it = mk_enum_item_hashtable_next(vm->code->classes, it, &key, &clazz);
		register_class_static_instance(vm, clazz);
	}
	return MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP;
}
