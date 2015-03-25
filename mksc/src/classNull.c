#include "mk.h"
#include <stdlib.h>
#include <string.h>
#include <memory.h>

static const MK_CHAR className[] = CLASS_INTERNAL_NULL;
static const MK_CHAR classSuper[] = CLASS_INTERNAL_OBJECT;

MK_CLASS *mk_create_null_class( MK_VM_STRUCT *vm )
{
	MK_CLASS *result =
		mk_create_object( MK_TYPE_CLASS );
	
	result->nameThis = mk_get_symbol_name_ptr( vm, className );
	result->nameSuper = mk_get_symbol_name_ptr( vm, classSuper );

	return result;
}
