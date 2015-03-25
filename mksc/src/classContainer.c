#include "mk.h"
#include <string.h>

static const MK_CHAR className[] = CLASS_INTERNAL_CONTAINER;
static const MK_CHAR classSuper[] = CLASS_INTERNAL_OBJECT;

MK_CLASS *mk_create_container_class( MK_VM_STRUCT *vm )
{
	MK_CLASS *result =
		mk_create_object( MK_TYPE_CLASS );
	
	result->nameThis = mk_get_symbol_name_ptr( vm, className );
	result->nameSuper = mk_get_symbol_name_ptr( vm, classSuper );

	mk_register_variable(
		vm,
		mk_create_method(
			vm, 
			"size",
			MK_TYPE_VARIABLE | 
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD | 
				MK_TYPE_ATTRIBUTE_VARIABLE_VIRTUAL,
			0,
			(INT_PTR)NULL ),
		result );

	mk_register_variable(
		vm,
		mk_create_default_native_method(
			vm,
			"[]",
			MK_TYPE_VARIABLE | 
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD | 
				MK_TYPE_ATTRIBUTE_VARIABLE_VIRTUAL |
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD_OPERATOR |
				MK_LEX_TYPE_RESERVED_MARK_BRACKET,
			1,
			(INT_PTR)NULL ),
		result );

	return result;
}
