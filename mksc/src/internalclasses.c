#include "mk.h"

extern MK_CLASS *mk_create_kernel_class( MK_VM_STRUCT *vm );
extern MK_CLASS *mk_create_int_class( MK_VM_STRUCT *vm );
extern MK_CLASS *mk_create_float_class( MK_VM_STRUCT *vm );
extern MK_CLASS *mk_create_null_class( MK_VM_STRUCT *vm );
extern MK_CLASS *mk_create_object_class( MK_VM_STRUCT *vm );
extern MK_CLASS *mk_create_io_class( MK_VM_STRUCT *vm );
extern MK_CLASS *mk_create_console_class( MK_VM_STRUCT *vm );
extern MK_CLASS *mk_create_exception_class( MK_VM_STRUCT *vm );
extern MK_CLASS *mk_create_vm_error_exception_class( MK_VM_STRUCT *vm );
extern MK_CLASS *mk_create_compile_error_exception_class( MK_VM_STRUCT *vm );
extern MK_CLASS *mk_create_container_class( MK_VM_STRUCT *vm );
extern MK_CLASS *mk_create_array_class( MK_VM_STRUCT *vm );
extern MK_CLASS *mk_create_array_outof_range_exception_class( MK_VM_STRUCT *vm );
extern MK_CLASS *mk_create_node_class( MK_VM_STRUCT *vm );
extern MK_CLASS *mk_create_string_class( MK_VM_STRUCT *vm );
extern MK_CLASS *mk_create_referecne_class( MK_VM_STRUCT *vm );

static
int create_vm_cache( MK_VM_STRUCT *vm )
{
	// allocate cache
	if(vm->cache == NULL)
	{
		vm->cache = malloc( sizeof(MK_VM_CACHE) );
		memset( vm->cache, 0x00, sizeof(MK_VM_CACHE) );
	}

	// constant class symbol name.
	vm->cache->internalClassSymbolName[MK_VM_FRAME_ITEM_TYPE_NIL >> 24] =
		mk_get_symbol_name_ptr( vm, CLASS_INTERNAL_NULL );
//	vm->cache->internalClassSymbolName[MKVM_FRAME_ITEM_TYPE_CLASS >> 24] =
//		mk_get_symbol_name_ptr( vm, CLASS_INTERNAL_CLASS );
//	vm->cache->internalClassSymbolName[MKVM_FRAME_ITEM_TYPE_MODULE >> 24] =
//		mk_get_symbol_name_ptr( vm, CLASS_INTERNAL_MODULE );
	vm->cache->internalClassSymbolName[MK_VM_FRAME_ITEM_TYPE_INT_VALUE >> 24] =
		mk_get_symbol_name_ptr( vm, CLASS_INTERNAL_INTEGER );
	vm->cache->internalClassSymbolName[MK_VM_FRAME_ITEM_TYPE_FLOAT_VALUE >> 24] =
		mk_get_symbol_name_ptr( vm, CLASS_INTERNAL_FLOAT );
	vm->cache->internalClassSymbolName[MK_VM_FRAME_ITEM_TYPE_ARRAY_VALUE >> 24] =
		mk_get_symbol_name_ptr( vm, CLASS_INTERNAL_ARRAY );
	vm->cache->internalClassSymbolName[MK_VM_FRAME_ITEM_TYPE_STRING_VALUE >> 24] =
		mk_get_symbol_name_ptr( vm, CLASS_INTERNAL_STRING );
	vm->cache->internalClassSymbolName[MK_VM_FRAME_ITEM_TYPE_NODE_VALUE >> 24] =
		mk_get_symbol_name_ptr( vm, CLASS_INTERNAL_NODE );

	// constant value MK_CLASS type
	mk_find_item_hashtable( vm->code->classes, 
		mk_get_symbol_name_ptr( vm, CLASS_INTERNAL_NULL ), 
		(void**)&vm->cache->pConstantClass[MK_VM_FRAME_ITEM_TYPE_NIL >> 24] );
//	mk_find_item_hashtable( vm->code->classes, 
//		mk_get_symbol_name_ptr( vm, CLASS_INTERNAL_CLASS ), 
//		&vm->cache->pConstantClass[MK_VM_FRAME_ITEM_TYPE_CLASS >> 24] );
//	mk_find_item_hashtable( vm->code->classes, 
//		mk_get_symbol_name_ptr( vm, CLASS_INTERNAL_MODULE ), 
//		&vm->cache->pConstantClass[MK_VM_FRAME_ITEM_TYPE_MODULE >> 24] );
	mk_find_item_hashtable( vm->code->classes, 
		mk_get_symbol_name_ptr( vm, CLASS_INTERNAL_INTEGER ), 
		(void**)&vm->cache->pConstantClass[MK_VM_FRAME_ITEM_TYPE_INT_VALUE >> 24] );
	mk_find_item_hashtable( vm->code->classes, 
		mk_get_symbol_name_ptr( vm, CLASS_INTERNAL_FLOAT ), 
		(void**)&vm->cache->pConstantClass[MK_VM_FRAME_ITEM_TYPE_FLOAT_VALUE >> 24] );
	mk_find_item_hashtable( vm->code->classes, 
		mk_get_symbol_name_ptr( vm, CLASS_INTERNAL_ARRAY ), 
		(void**)&vm->cache->pConstantClass[MK_VM_FRAME_ITEM_TYPE_ARRAY_VALUE >> 24] );
	mk_find_item_hashtable( vm->code->classes, 
		mk_get_symbol_name_ptr( vm, CLASS_INTERNAL_STRING ), 
		(void**)&vm->cache->pConstantClass[MK_VM_FRAME_ITEM_TYPE_STRING_VALUE >> 24] );
	mk_find_item_hashtable( vm->code->classes, 
		mk_get_symbol_name_ptr( vm, CLASS_INTERNAL_NODE ), 
		(void**)&vm->cache->pConstantClass[MK_VM_FRAME_ITEM_TYPE_NODE_VALUE >> 24] );
	mk_find_item_hashtable( vm->code->classes,
		mk_get_symbol_name_ptr( vm, CLASS_INTERNAL_REFERENCE ),
		(void**)&vm->cache->pConstantClass[MK_VM_FRAME_ITEM_TYPE_REFERENCE_VALUE >> 24] );
	return MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP;
}

void mk_register_internal_classes( MK_VM_STRUCT *vm )
{
	MK_CLASS *target = NULL;
	MK_VARIABLE *entryPoint = NULL;

	// create Object class
	target = mk_create_object_class( vm );
	mk_register_class( vm, target );

	// create Kernel class
	target = mk_create_kernel_class( vm );

	// create entrypoint function
	entryPoint = 
		(MK_VARIABLE*)mk_create_object( MK_TYPE_VARIABLE );
	entryPoint->flags |= 
		MK_TYPE_ATTRIBUTE_VARIABLE_METHOD | 
		MK_TYPE_ATTRIBUTE_VARIABLE_METHOD_SCRIPT | 
		MK_TYPE_ATTRIBUTE_VARIABLE_STATIC;

	entryPoint->name = 
		mk_get_symbol_name_ptr( vm, FUNCTIONNAME_ENTRYPOINT );
	mk_register_variable( vm, entryPoint, target );

	mk_register_class( vm, target );

	// create Integer class
	target = mk_create_int_class( vm );
	mk_register_class( vm, target );

	// create String class
	target = mk_create_string_class( vm );
	mk_register_class( vm, target );

	// create IO class
	target = mk_create_io_class( vm );
	mk_register_class( vm, target );

	// create Console class
	target = mk_create_console_class( vm );
	mk_register_class( vm, target );

	// create Float class
	target = mk_create_float_class( vm );
	mk_register_class( vm, target );

	// create Container class
	target = mk_create_container_class( vm );
	mk_register_class( vm, target );
	
	// create Array class
	target = mk_create_array_class( vm );
	mk_register_class( vm, target );

	// create Node class
	target = mk_create_node_class( vm );
	mk_register_class( vm, target );

	// create Null class
	target = mk_create_null_class( vm );
	mk_register_class( vm, target );

	// create Exception class
	target = mk_create_exception_class( vm );
	mk_register_class( vm, target );

	// create CompileErrorException class
	target = mk_create_compile_error_exception_class( vm );
	mk_register_class( vm, target );

	// create VMErrorException class
	target = mk_create_vm_error_exception_class( vm );
	mk_register_class( vm, target );

	// create ArrayOutOfRangeException class
	target = mk_create_array_outof_range_exception_class( vm );
	mk_register_class( vm, target );

	// create Reference class
	target = mk_create_referecne_class( vm );
	mk_register_class( vm, target );

	// generate static instance of internal class.
	mk_link_register_class_static_instance(vm);

	// create internal class cache.
	create_vm_cache(vm);
}
