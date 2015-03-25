#include "mk.h"
#include <string.h>

static const MK_CHAR classNameException[] = CLASS_INTERNAL_EXCEPTION;
static const MK_CHAR classSuper[] = CLASS_INTERNAL_OBJECT;

static
int mk_object_to_s( struct TAG_MK_VM_STRUCT *vm, unsigned int varArgC )
{
	MK_VM_FRAME_ITEM *target = 
		mk_vm_pop_stack( &vm->localStack );
	MK_VM_FRAME_ITEM *result = 
		mk_create_vm_frame_item_object( &vm->pFrameItemTable );
	result->flags = 
		MK_TYPE_SET_ATTRIBUTE( result->flags, MK_VM_FRAME_ITEM_TYPE_STRING_VALUE );


	return MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP;
}


MK_CLASS *mk_create_exception_class( MK_VM_STRUCT *vm )
{
	MK_CLASS *result =
		mk_create_object( MK_TYPE_CLASS );
	
	result->nameThis = mk_get_symbol_name_ptr( vm, classNameException );
	result->nameSuper = mk_get_symbol_name_ptr( vm, classSuper );

	mk_register_variable( vm, 
		mk_create_variable( vm, "id", MK_TYPE_VARIABLE | MK_TYPE_ATTRIBUTE_VARIABLE_VALUE, NULL ),
		result );

	mk_register_variable( vm, 
		mk_create_variable( vm, "name", MK_TYPE_VARIABLE | MK_TYPE_ATTRIBUTE_VARIABLE_VALUE, NULL ),
		result );

	mk_register_variable( vm, 
		mk_create_variable( vm, "line", MK_TYPE_VARIABLE | MK_TYPE_ATTRIBUTE_VARIABLE_VALUE, NULL ),
		result );

	mk_register_variable( vm, 
		mk_create_variable( vm, "description", MK_TYPE_VARIABLE | MK_TYPE_ATTRIBUTE_VARIABLE_VALUE, NULL ),
		result );

	mk_register_variable( vm, 
		mk_create_variable( vm, "target", MK_TYPE_VARIABLE | MK_TYPE_ATTRIBUTE_VARIABLE_VALUE, NULL ),
		result );

	return result;
}

static const MK_CHAR classNameCompileError[] = CLASS_INTERNAL_COMPILE_ERROR_EXCEPTION;

MK_CLASS *mk_create_compile_error_exception_class( MK_VM_STRUCT *vm )
{
	MK_CLASS *result =
		mk_create_object( MK_TYPE_CLASS );
	
	result->nameThis = mk_get_symbol_name_ptr( vm, classNameCompileError );
	result->nameSuper = mk_get_symbol_name_ptr( vm, classNameException );

	return result;
}

static const MK_CHAR classNameVMError[] = CLASS_INTERNAL_VM_ERROR_EXCEPTION;

MK_CLASS *mk_create_vm_error_exception_class( MK_VM_STRUCT *vm )
{
	MK_CLASS *result =
		mk_create_object( MK_TYPE_CLASS );
	
	result->nameThis = mk_get_symbol_name_ptr( vm, classNameVMError );
	result->nameSuper = mk_get_symbol_name_ptr( vm, classNameException );

	return result;
}

static const MK_CHAR classNameArrayOutOfRange[] = CLASS_INTERNAL_ARRAY_OUTOF_RANGE;

MK_CLASS *mk_create_array_outof_range_exception_class( MK_VM_STRUCT *vm )
{
	MK_CLASS *result =
		mk_create_object( MK_TYPE_CLASS );
	
	result->nameThis = mk_get_symbol_name_ptr( vm, classNameArrayOutOfRange );
	result->nameSuper = mk_get_symbol_name_ptr( vm, classNameException );

	return result;
}

static const MK_CHAR classNameGeneralIOException[] = CLASS_INTERNAL_IO_EXCEPTION;

MK_CLASS *mk_create_io_exception( MK_VM_STRUCT *vm )
{
	MK_CLASS *result =
		mk_create_object( MK_TYPE_CLASS );
	
	result->nameThis = mk_get_symbol_name_ptr( vm, classNameGeneralIOException );
	result->nameSuper = mk_get_symbol_name_ptr( vm, classNameException );

	return result;
}

static const MK_CHAR classNameInvalidNameException[] = CLASS_INTERNAL_INVALID_NAME_EXCEPTION;

MK_CLASS *mk_create_name_exception( MK_VM_STRUCT *vm )
{
	MK_CLASS *result =
		mk_create_object( MK_TYPE_CLASS );
	
	result->nameThis = mk_get_symbol_name_ptr( vm, classNameInvalidNameException );
	result->nameSuper = mk_get_symbol_name_ptr( vm, classNameException );

	return result;
}

static const MK_CHAR classNameInvalidTypeException[] = CLASS_INTERNAL_INVALID_TYPE_EXCEPTION;

MK_CLASS *mk_create_type_exception( MK_VM_STRUCT *vm )
{
	MK_CLASS *result =
		mk_create_object( MK_TYPE_CLASS );
	
	result->nameThis = mk_get_symbol_name_ptr( vm, classNameInvalidTypeException );
	result->nameSuper = mk_get_symbol_name_ptr( vm, classNameException );

	return result;
}

static const MK_CHAR classNameInvalidConstraintException[] = CLASS_INTERNAL_INVALID_CONSTRAINT_EXCEPTION;

MK_CLASS *mk_create_costraint_exception( MK_VM_STRUCT *vm )
{
	MK_CLASS *result =
		mk_create_object( MK_TYPE_CLASS );
	
	result->nameThis = mk_get_symbol_name_ptr( vm, classNameInvalidConstraintException );
	result->nameSuper = mk_get_symbol_name_ptr( vm, classNameException );

	return result;
}
