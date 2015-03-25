#include "mk.h"
#include <stdlib.h>
#include <memory.h>
#include <string.h>

void *mk_open( void*value );
int mk_getc( void*stream );
void mk_close( void*stream );
void mk_trace( unsigned int code, MK_CHAR *message );

static
int mk_insert_variables_to_object( MK_VM_STRUCT *vm, MK_VM_FRAME_ITEM *newVariable, MK_CLASS *pClass, int isStatic );
static
MK_NODE_BLOCK *create_mk_node_block( unsigned int type );
static
MK_NODE_EXPR *create_numeric_expr( unsigned int value, MK_CHAR *text, unsigned int isNeg );
static
MK_NODE_EXPR *create_string_expr( MK_CHAR *text, unsigned int length );
static
MK_NODE_EXPR *create_operation_expr( unsigned int value );
static
MK_NODE_EXPR *create_member_variable_expr( MK_VM_STRUCT *vm, MK_LEXER *lexer );
static
MK_NODE_EXPR *create_static_member_variable_expr( MK_VM_STRUCT *vm, MK_LEXER *lexer );
static
MK_NODE_EXPR *create_symbol_expr( MK_VM_STRUCT *vm, MK_LEXER *lexer );
static
MK_NODE_EXPR *create_new_expr( MK_VM_STRUCT *vm, MK_LEXER *lexer );
static
MK_NODE_EXPR *create_this_expr( );
static
MK_NODE_EXPR *create_super_expr( );
static
MK_NODE_EXPR *create_owner_expr( );
static
MK_NODE_EXPR *do_compile_break_state( MK_VM_STRUCT *vm, MK_LEXER *lexer );
static
MK_NODE_EXPR *do_compile_continue_state( MK_VM_STRUCT *vm, MK_LEXER *lexer );
static
MK_NODE_EXPR *do_compile_raise_state( MK_VM_STRUCT *vm, MK_LEXER *lexer );
static
MK_NODE_EXPR *create_nil_expr( );
static
MK_NODE_EXPR *create_true_expr( );
static
MK_NODE_EXPR *create_false_expr( );
static
MK_NODE_EXPR *create_multiple_top_expr( );
static
MK_NODE_EXPR *do_compile_expr_root( MK_VM_STRUCT *vm, MK_LEXER *lexer, MK_NODE_EXPR *statement );
static
MK_NODE_EXPR *do_compile_return_state( MK_VM_STRUCT *vm, MK_LEXER *lexer );
static
MK_NODE_IF *do_compile_if_block( MK_VM_STRUCT *vm, MK_LEXER *lexer );
static
MK_NODE_WHILE *do_compile_while_block( MK_VM_STRUCT *vm, MK_LEXER *lexer );
static
MK_NODE_WHILE *do_compile_do_while_block( MK_VM_STRUCT *vm, MK_LEXER *lexer );
static
MK_NODE_EXPR *do_compile_node_with_param( MK_VM_STRUCT *vm, MK_LEXER *lexer );
static
MK_TRY_BLOCK *do_compile_try_catch_finally_block( MK_VM_STRUCT *vm, MK_LEXER *lexer );
static
MK_NODE_EXPR *do_compile_method_call( MK_VM_STRUCT *vm, MK_LEXER *lexer );
static
MK_NODE_EXPR *do_compile_method_call( MK_VM_STRUCT *vm, MK_LEXER *lexer );
static
int do_compile_segment( MK_VM_STRUCT *vm, MK_LEXER *lexer, MK_NODE_BLOCK *block, unsigned int typeSegment );
static
MK_VARIABLE* do_compile_method( MK_VM_STRUCT *vm, MK_LEXER *lexer, int isModule );
static
int do_compile_parameters( MK_VM_STRUCT *vm, MK_LEXER *lexer, MK_VECTOR *arguments, unsigned int endOfMark );
static
int do_compile_arguments( MK_VM_STRUCT *vm, MK_LEXER *lexer, MK_VECTOR *arguments, unsigned int styleExpr, unsigned int endOfMark );
static
int do_compile_property( MK_VM_STRUCT *vm, MK_LEXER *lexer, MK_CLASS *targetClass );
static
int do_compile_inner_class( MK_VM_STRUCT *vm, MK_LEXER *lexer, MK_CLASS *targetClass );
static
MK_CLASS *do_compile_class_definition( MK_VM_STRUCT *vm, MK_LEXER *lexer );
static
int do_compile_root( MK_VM_STRUCT *vm, MK_LEXER *lexer );


int mk_register_class( MK_VM_STRUCT *vm, MK_CLASS *target )
{
	void *super = NULL;

	if( target == NULL || target->nameThis == NULL )
		return MK_VM_EXECUTE_EXPR_THROW;

	if( mk_is_key_hashtable( vm->code->classes, target->nameThis ) != 0 )
		return MK_VM_EXECUTE_EXPR_THROW;

	// return 0 if superclass not defined.
	if( ( target->nameSuper != NULL && target->nameSuper[0] != '\0' ) &&
		mk_find_item_hashtable( vm->code->classes,
			mk_get_symbol_name_ptr( vm, target->nameSuper ),
			&super ) == 0 )
		return MK_VM_EXECUTE_EXPR_THROW;

	mk_insert_item_hashtable( 
		vm->code->classes, 
		mk_get_symbol_name_ptr( vm, target->nameThis ), 
		target );

	return MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP;
}

int mk_register_variable( MK_VM_STRUCT *vm, MK_VARIABLE *targetVariable, MK_CLASS *targetClass )
{
	MK_CHAR *name = NULL;

	if( targetVariable == NULL || targetVariable->name == NULL )
		return MK_VM_EXECUTE_EXPR_THROW;

	name = 
		mk_get_symbol_name_ptr( vm, targetVariable->name );
	
	if( ( targetVariable->flags & MK_TYPE_ATTRIBUTE_VARIABLE_METHOD ) &&
		( targetVariable->flags & MK_TYPE_ATTRIBUTE_VARIABLE_METHOD_OPERATOR ) )
	{
		unsigned int index = MK_RESERVED_MARK_INDEX(targetVariable->flags);
		if( index >= MK_SIZEOF_ENABELD_OPELATORS )
			return MK_VM_EXECUTE_EXPR_THROW;	// incorrect operator.
		if( targetClass->operatorMethod == NULL )
		{
			targetClass->operatorMethod = 
				malloc( sizeof(INT_PTR) * MK_SIZEOF_ENABELD_OPELATORS );
			memset( targetClass->operatorMethod, 0x00, sizeof(INT_PTR) * MK_SIZEOF_ENABELD_OPELATORS );
		}
		if( targetClass->operatorMethod[index] != (INT_PTR)NULL )
			return MK_VM_EXECUTE_EXPR_THROW;	// operator aleady defined.
		else
			targetClass->operatorMethod[index] = (INT_PTR)targetVariable;
	}
	else
	{
		if( targetClass->variables == NULL )
			targetClass->variables = 
				mk_create_hashtable( MK_TYPE_ATTRIBUTE_HASH_INTPTR_KEY_DEFAULT |
					MK_TYPE_ATTRIBUTE_HASH_VALUETYPE_FLAG(0, MK_TYPE_ATTRIBUTE_HASH_VALUE_MK_OBJECT_PTR ),
				MK_SIZEOF_HASH_DEFAULT );
		else if( mk_is_key_hashtable( targetClass->variables, name ) != 0 )
			return MK_VM_EXECUTE_EXPR_THROW;	// variable aleady defined.

		// register variable to MK_CLASS
		mk_insert_item_hashtable( targetClass->variables, name, targetVariable );
	}
	return MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP;
}

int mk_register_variable_arg( MK_VM_STRUCT *vm, MK_VECTOR *targetArgs, MK_VARIABLE *arg )
{
	unsigned int size = 
		mk_size_vector( targetArgs );
	unsigned int index = 0;

	for( index = 0; index < size; index ++ )
	{
		MK_VARIABLE *value = 
			(MK_VARIABLE*)mk_get_at_vector( targetArgs, index );
		if( value->name == arg->name )		// these ptr is in symbol name table.
			break;
	}
	if( index < size )
	{
		return MK_VM_EXECUTE_EXPR_THROW;		// already defined same name argment.
	}
	else
	{
		mk_push_vector( targetArgs, (INT_PTR)arg );
		return MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP;
	}
}

MK_VARIABLE *mk_create_variable( MK_VM_STRUCT *vm, MK_CHAR *name, unsigned int type, MK_NODE_EXPR *defaultValue )
{
	MK_VARIABLE *result = NULL;
	if( MK_OBJECT_TYPE(type) != MK_TYPE_VARIABLE )
		return NULL;	// type incorrect.
	if( type & MK_TYPE_ATTRIBUTE_VARIABLE_METHOD )
		return NULL;

	result = mk_create_object( type );
	result->name = mk_get_symbol_name_ptr( vm, name );
	result->defaultValue = defaultValue;
	return result;
}

MK_VARIABLE *mk_create_default_native_method( MK_VM_STRUCT *vm, MK_CHAR *name, unsigned int type, int sizeArg, INT_PTR entryPoint )
{
	MK_VECTOR *arguments = NULL;
	if( sizeArg != 0 )
	{
		int index = 0;
		arguments =
			mk_create_vector( 
				MK_TYPE_VECTOR | MK_TYPE_VECTOR_ARRAY_MANAGED_PTR | ( MK_SIZEOF_EXTEND_VECTOR_DEFAULT << 10 ),
				sizeArg,
				(INT_PTR)NULL );
		for( index = 0; index < sizeArg; index ++ )
		{
			MK_CHAR nameBuffer[16];
			MK_VARIABLE *arg = 
				mk_create_object( MK_TYPE_VARIABLE );
			sprintf( nameBuffer, "arg_%d", index );
			arg->name = mk_get_symbol_name_ptr( vm, nameBuffer );
			arg->defaultValue = NULL;
			mk_push_vector( arguments, (INT_PTR)arg );
		}
	}
	return mk_create_method( vm, name, type, arguments, entryPoint );
}

MK_VARIABLE *mk_create_method( MK_VM_STRUCT *vm, MK_CHAR *name, unsigned int type, MK_VECTOR* arg, INT_PTR entryPoint )
{
	MK_VARIABLE *result = NULL;
	if( MK_OBJECT_TYPE(type) != MK_TYPE_VARIABLE )
		return NULL;	// type incorrect.
	if( !( type & MK_TYPE_ATTRIBUTE_VARIABLE_METHOD ) )
		return NULL;
	result = mk_create_object( type );
	result->name = mk_get_symbol_name_ptr( vm, name );
	result->args = (MK_VECTOR*)arg;
	result->entryPoint = (void*)entryPoint;
	return result;
}

static
MK_NODE_BLOCK *create_mk_node_block( unsigned int type )
{
	MK_NODE_BLOCK *result = 
		mk_create_object( MK_TYPE_NODE_BLOCK );
	result->flags = MK_TYPE_SET_ATTRIBUTE( result->flags, type );
	return result;
}

static
MK_NODE_EXPR *create_numeric_expr( unsigned int value, MK_CHAR *text, unsigned int isNeg )
{
	MK_NODE_EXPR *result = 
		( MK_NODE_EXPR * )mk_create_object( MK_TYPE_NODE_EXPR );
	if( value == MK_LEX_TYPE_INT_VALUE )
	{
		unsigned int num = 
			atoi( text );
		if( isNeg )
			num *= -1;
		result->flags = 
			MK_TYPE_NODE_EXPR | MK_TYPE_NODE_EXPR_INT32;
		result->u1.constantValue = 0;
		result->u2.constantValue = 
			num;
	}
	else if( value == MK_LEX_TYPE_FLOAT_VALUE )
	{
		MK_FLOAT num =
			atof( text );
		if( isNeg )
			num = num * -1.0;
		result->flags = 
			MK_TYPE_NODE_EXPR | MK_TYPE_NODE_EXPR_FLOAT;
		result->floatValue = num;
	}
	else
	{
		free( result );
		result = NULL;
	}
	return result;
}

static 
MK_NODE_EXPR *create_string_expr( MK_CHAR *text, unsigned int length )
{
	MK_NODE_EXPR *result = 
		( MK_NODE_EXPR * )mk_create_object( MK_TYPE_NODE_EXPR );
	result->flags = 
		MK_TYPE_NODE_EXPR | MK_TYPE_NODE_EXPR_STRING;

	result->u1.length = length;
	result->u2.value = malloc( sizeof(MK_CHAR) * (length + 1) );
	memset( result->u2.value, 0x00, sizeof(MK_CHAR) * (length + 1) );
	memcpy( result->u2.value, text, sizeof(MK_CHAR) * (length) );

	return result;
}

static
MK_NODE_EXPR *create_operation_expr( unsigned int value )
{
	MK_NODE_EXPR *result = 
		( MK_NODE_EXPR * )mk_create_object( MK_TYPE_NODE_EXPR );
	result->flags = 
		MK_TYPE_NODE_EXPR | 
		MK_TYPE_NODE_EXPR_OPERATION |
		( value & ( MK_TYPE_NODE_EXPR_OPERATIONTYPE_MASK | 
				   MK_TYPE_NODE_EXPR_OPERATIONPRIORITY_MASK ) );
	return result;
}

static
MK_NODE_EXPR *create_static_member_variable_expr( MK_VM_STRUCT *vm, MK_LEXER *lexer )
{
	void *keyPtr = NULL;

	MK_NODE_EXPR *result = 
		( MK_NODE_EXPR * )mk_create_object( MK_TYPE_NODE_EXPR );
	result->flags = 
		MK_TYPE_NODE_EXPR |
		MK_TYPE_NODE_EXPR_DBLATSYMBOL;
	result->u1.symbolName = mk_get_symbol_name_ptr( vm, lexer->text );
	result->u2.constantValue = 0;

	return result;
}

static
MK_NODE_EXPR *create_member_variable_expr( MK_VM_STRUCT *vm, MK_LEXER *lexer )
{
	void *keyPtr = NULL;
	MK_NODE_EXPR *result = 
		( MK_NODE_EXPR * )mk_create_object( MK_TYPE_NODE_EXPR );
	result->flags = 
		MK_TYPE_NODE_EXPR |
		MK_TYPE_NODE_EXPR_ATSYMBOL;
	result->u1.symbolName = mk_get_symbol_name_ptr( vm, lexer->text );
	result->u2.constantValue = 0;
	return result;
}

static
MK_NODE_EXPR *create_symbol_expr( MK_VM_STRUCT *vm, MK_LEXER *lexer )
{
	void *keyPtr = NULL;
	MK_NODE_EXPR *result = 
		( MK_NODE_EXPR * )mk_create_object( MK_TYPE_NODE_EXPR );
	result->flags = 
		MK_TYPE_NODE_EXPR |
		MK_TYPE_NODE_EXPR_SYMBOL;
	result->u1.symbolName = mk_get_symbol_name_ptr( vm, lexer->text );
	result->u2.constantValue = 0;
	return result;
}

static
MK_NODE_EXPR *create_me_expr( MK_VM_STRUCT *vm, MK_LEXER *lexer )
{
	MK_NODE_EXPR *result =
		do_compile_method_call( vm, lexer );
	if( result != NULL )
		result->flags = 
			MK_TYPE_NODE_EXPR | MK_TYPE_NODE_EXPR_ME;
	return result;
}

static
MK_NODE_EXPR *create_new_expr( MK_VM_STRUCT *vm, MK_LEXER *lexer )
{
	MK_NODE_EXPR *result =
		do_compile_method_call( vm, lexer );
	if( result != NULL )
		result->flags = 
			MK_TYPE_NODE_EXPR | MK_TYPE_NODE_EXPR_NEW;
	return result;
}

static
MK_NODE_EXPR *create_this_expr( )
{
	MK_NODE_EXPR *result = 
		( MK_NODE_EXPR * )mk_create_object( MK_TYPE_NODE_EXPR );
	result->flags = MK_TYPE_NODE_EXPR | MK_TYPE_NODE_EXPR_SYMBOL_THIS;
	result->u1.constantValue = 0;
	result->u2.constantValue = 0;
	return result;
}

static
MK_NODE_EXPR *create_super_expr( )
{
	MK_NODE_EXPR *result = 
		( MK_NODE_EXPR * )mk_create_object( MK_TYPE_NODE_EXPR );
	result->flags = MK_TYPE_NODE_EXPR | MK_TYPE_NODE_EXPR_SYMBOL_SUPER;
	result->u1.constantValue = 0;
	result->u2.constantValue = 0;
	return result;
}


static
MK_NODE_EXPR *create_owner_expr( )
{
	MK_NODE_EXPR *result = 
		( MK_NODE_EXPR * )mk_create_object( MK_TYPE_NODE_EXPR );
	result->flags = MK_TYPE_NODE_EXPR | MK_TYPE_NODE_EXPR_SYMBOL_OWNER;
	result->u1.constantValue = 0;
	result->u2.constantValue = 0;
	return result;
}

static
MK_NODE_EXPR *create_nil_expr( )
{
	MK_NODE_EXPR *result = 
		( MK_NODE_EXPR * )mk_create_object( MK_TYPE_NODE_EXPR );
	result->flags = MK_TYPE_NODE_EXPR | MK_TYPE_NODE_EXPR_SYMBOL_NIL;
	result->u1.constantValue = 0;
	result->u2.constantValue = 0;
	return result;
}

static
MK_NODE_EXPR *create_true_expr( )
{
	MK_NODE_EXPR *result = 
		( MK_NODE_EXPR * )mk_create_object( MK_TYPE_NODE_EXPR );
	result->flags = MK_TYPE_NODE_EXPR | MK_TYPE_NODE_EXPR_SYMBOL_TRUE;
	result->u1.constantValue = 0;
	result->u2.constantValue = 0;
	return result;
}

static
MK_NODE_EXPR *create_false_expr( )
{
	MK_NODE_EXPR *result = 
		( MK_NODE_EXPR * )mk_create_object( MK_TYPE_NODE_EXPR );
	result->flags = MK_TYPE_NODE_EXPR | MK_TYPE_NODE_EXPR_SYMBOL_FALSE;
	result->u1.constantValue = 0;
	result->u2.constantValue = 0;
	return result;
}

static
MK_NODE_EXPR *create_multiple_top_expr( )
{
	MK_NODE_EXPR *result = 
		( MK_NODE_EXPR * )mk_create_object( MK_TYPE_NODE_EXPR );
	result->flags = MK_TYPE_NODE_EXPR | MK_TYPE_NODE_EXPR_MULTIPLESYMBOL;
	result->u1.constantValue = 0;
	result->u2.constantValue = 0;
	return result;
}

static
MK_NODE_EXPR *do_compile_break_state( MK_VM_STRUCT *vm, MK_LEXER *lexer )
{
	MK_NODE_EXPR *result = 
		( MK_NODE_EXPR * )mk_create_object( MK_TYPE_NODE_EXPR );
	result->flags = MK_TYPE_NODE_EXPR | MK_TYPE_NODE_EXPR_BREAK;
	result->u1.constantValue = 0;
	result->u2.constantValue = 0;
	
	// lexer->text == "break"

	mk_get_token( vm, lexer );
	if( lexer->value != MK_LEX_TYPE_EOL &&
		lexer->value != MK_LEX_TYPE_EOF &&
		lexer->value != MK_LEX_TYPE_RESERVED_SYMBOL_IF )
	{
		if( vm->exceptionObject == NULL )
			mk_raise_internal_error( vm, 
				lexer->name, 
				lexer->line, 
				MK_ERROR_TYPE_COMILE_ERROR | 3, 
				"break", 
				lexer->text, 
				"<CR>|if",
				NULL );
		mk_destroy_node( result );
		result = NULL;
	}

	return result;
}

static
MK_NODE_EXPR *do_compile_continue_state( MK_VM_STRUCT *vm, MK_LEXER *lexer )
{
	MK_NODE_EXPR *result = 
		( MK_NODE_EXPR * )mk_create_object( MK_TYPE_NODE_EXPR );
	result->flags = MK_TYPE_NODE_EXPR | MK_TYPE_NODE_EXPR_CONTINUE;
	result->u1.constantValue = 0;
	result->u2.constantValue = 0;

	// lexer->text == "continue"

	mk_get_token( vm, lexer );
	if( lexer->value != MK_LEX_TYPE_EOL &&
		lexer->value != MK_LEX_TYPE_EOF &&
		lexer->value != MK_LEX_TYPE_RESERVED_SYMBOL_IF )
	{
		if( vm->exceptionObject == NULL )
			mk_raise_internal_error( vm, 
				lexer->name, 
				lexer->line, 
				MK_ERROR_TYPE_COMILE_ERROR | 3, 
				"continue", 
				lexer->text, 
				"<CR>|if",
				NULL );
		mk_destroy_node( result );
		result = NULL;
	}

	return result;
}

static
MK_NODE_EXPR *do_compile_raise_state( MK_VM_STRUCT *vm, MK_LEXER *lexer )
{
	int isSuccess = 0;
	MK_NODE_EXPR *result = 
		( MK_NODE_EXPR * )mk_create_object( MK_TYPE_NODE_EXPR );
	result->flags = MK_TYPE_NODE_EXPR | MK_TYPE_NODE_EXPR_RAISE;

	// lexer->text == "raise"
	do
	{
		mk_get_token( vm, lexer );
		if( lexer->value != MK_LEX_TYPE_EOL )
		{
			result->u1.left = do_compile_expr( vm, lexer, MK_TYPE_EXPR_ROOT_SEGMENT );
			if( result->u1.left == NULL )
			{
				// error object creation error.
				if( vm->exceptionObject == NULL )
					mk_raise_internal_error( vm, lexer->name, lexer->line, MK_ERROR_TYPE_COMILE_ERROR | 18, "raise", "<expr>", NULL );
				break;
			}
		}
		isSuccess = 1;
	}while( 0 );
	if( lexer->hasError != 0 )
		isSuccess = 0;
	if( isSuccess == 0 )
	{
		mk_destroy_node( result );
		result = NULL;
	}
	return result;
}

static
MK_NODE_EXPR *do_compile_return_state( MK_VM_STRUCT *vm, MK_LEXER *lexer )
{
	int isSuccess = 0;
	MK_NODE_EXPR *result = 
		( MK_NODE_EXPR * )mk_create_object( MK_TYPE_NODE_EXPR );
	result->flags = MK_TYPE_NODE_EXPR | MK_TYPE_NODE_EXPR_RETURN;

	// lexer->text == return

	do
	{
		mk_get_token( vm, lexer );
		if( lexer->value != MK_LEX_TYPE_EOL )
		{
			result->u1.left = do_compile_expr( vm, lexer, MK_TYPE_EXPR_ROOT_SEGMENT );
			if( result->u1.left == NULL )
			{
				if( vm->exceptionObject == NULL )
					mk_raise_internal_error( vm, lexer->name, lexer->line, MK_ERROR_TYPE_COMILE_ERROR | 18, "return", "<expr>", NULL );
				break;
			}
		}
		isSuccess = 1;
	}while( 0 );
	if( lexer->hasError != 0 )
		isSuccess = 0;
	if( isSuccess == 0 )
	{
		mk_destroy_node( result );
		result = NULL;
	}
	return result;
}

static
MK_NODE_IF *do_compile_if_block( MK_VM_STRUCT *vm, MK_LEXER *lexer )
{
	unsigned int status = 0;	// 0:parsing if... 1: parsing elseif... 2:parsing else...
	unsigned int isSuccess = 0;
	MK_NODE_IF *result = NULL, 
		*current = NULL, 
		*previous = NULL;

	while( lexer->value != MK_LEX_TYPE_EOF )
	{
		if( status == 0 )
		{
			if( lexer->value == MK_LEX_TYPE_RESERVED_SYMBOL_IF )
			{
				status = 1;
			}
			else
			{
				if( vm->exceptionObject == NULL )
					mk_raise_internal_error( vm, lexer->name, lexer->line, MK_ERROR_TYPE_COMILE_ERROR | 2, lexer->text, "if", NULL );
				break;
			}
		}
		else if( status == 1 )
		{
			if( lexer->value != MK_LEX_TYPE_RESERVED_SYMBOL_ELSEIF ||
				lexer->value != MK_LEX_TYPE_RESERVED_SYMBOL_ELSE )
			{
				if( lexer->value == MK_LEX_TYPE_RESERVED_SYMBOL_ELSE )
					status = 2;
			}
			else
			{
				if( vm->exceptionObject == NULL )
					mk_raise_internal_error( vm, lexer->name, lexer->line, MK_ERROR_TYPE_COMILE_ERROR | 2, lexer->text, "[elseif|else]", NULL );
				break;
			}
		}
		else	// status == 2
		{
			if( vm->exceptionObject == NULL )
			{
				if( lexer->value == MK_LEX_TYPE_RESERVED_SYMBOL_ELSE )
					mk_raise_internal_error( vm, lexer->name, lexer->line, MK_ERROR_TYPE_COMILE_ERROR | 15, "else", NULL );
				else
					mk_raise_internal_error( vm, lexer->name, lexer->line, MK_ERROR_TYPE_COMILE_ERROR | 2, lexer->text, "end", NULL );
			}
			break;
		}

		current = 
			( MK_NODE_IF * )mk_create_object( MK_TYPE_NODE_IF );
		if( previous != NULL )
			previous->next = current;
		else
			result = current;
		previous = current;

		mk_get_token( vm, lexer );
		if( status != 2 )
		{
			current->expr = do_compile_expr( vm, lexer, MK_TYPE_EXPR_IF_CONDITION );	// if ... then
			if( current->expr == NULL )
			{
				if( vm->exceptionObject == NULL )
					mk_raise_internal_error( vm, lexer->name, lexer->line, MK_ERROR_TYPE_COMILE_ERROR | 11, "<if condition>", NULL );
				break;
			}
			else if( lexer->value != MK_LEX_TYPE_RESERVED_SYMBOL_THEN )
			{
				if( vm->exceptionObject == NULL )
					mk_raise_internal_error( vm, lexer->name, lexer->line, MK_ERROR_TYPE_COMILE_ERROR | 2, lexer->text, "then", NULL );
				break;
			}
			mk_get_token( vm, lexer );	// lexer->text == "then"
		}
		if( lexer->value != MK_LEX_TYPE_EOL )
		{
			mk_raise_internal_error( vm, lexer->name, lexer->line, MK_ERROR_TYPE_COMILE_ERROR | 2, lexer->text, "<CR>", NULL );
			break;
		}

		current->block = create_mk_node_block( MK_TYPE_ATTRIBUTE_BLOCK_IF_BLOCK );
		if( do_compile_segment( vm, lexer, current->block, MK_TYPE_SEGMENT_IF ) == 0 )
		{
			if( vm->exceptionObject == NULL )
				mk_raise_internal_error( vm, lexer->name, lexer->line, MK_ERROR_TYPE_COMILE_ERROR | 11, "<if segment>", NULL );
			break;
		}
		if( lexer->value == MK_LEX_TYPE_RESERVED_SYMBOL_END )
		{
			mk_get_token( vm, lexer );
			if( lexer->value != MK_LEX_TYPE_EOL )
			{
				mk_raise_internal_error( vm, lexer->name, lexer->line, MK_ERROR_TYPE_COMILE_ERROR | 2, lexer->text, "<CR>", NULL );
				break;
			}
			isSuccess = 1;
			break;
		}
	}
	if( lexer->hasError != 0 )
		isSuccess = 0;
	if( isSuccess == 0 )
	{
		mk_destroy_node( result );
		result = NULL;
	}
	return ( isSuccess != 0 ) ? result : NULL;
}

static
MK_NODE_WHILE *do_compile_while_block( MK_VM_STRUCT *vm, MK_LEXER *lexer )
{
	unsigned int isSuccess = 0;
	MK_NODE_WHILE *result = 
			( MK_NODE_WHILE * )mk_create_object( MK_TYPE_NODE_WHILE );
	result->flags = MK_TYPE_NODE_WHILE | MK_TYPE_ATTRIBUTE_NODE_WHILE_FRONT;
	do
	{
		// lexer->text = "while"
		mk_get_token( vm, lexer );
		result->expr = do_compile_expr( vm, lexer, MK_TYPE_EXPR_WHILE_CONDITION );
		if( result->expr == NULL ||
			lexer->value != MK_LEX_TYPE_EOL )
		{
			if( vm->exceptionObject == NULL )
				mk_raise_internal_error( vm, lexer->name, lexer->line, MK_ERROR_TYPE_COMILE_ERROR | 11, "<while condition>", NULL );
			break;
		}
		mk_get_token( vm, lexer );
		result->block = create_mk_node_block( MK_TYPE_ATTRIBUTE_BLOCK_WHILE_BLOCK );
		if( do_compile_segment( vm, lexer, result->block, MK_TYPE_SEGMENT_WHILE ) == 0 )
		{
			if( vm->exceptionObject == NULL )
				mk_raise_internal_error( vm, lexer->name, lexer->line, MK_ERROR_TYPE_COMILE_ERROR | 11, "<while segment>", NULL );
		}
	}
	while( 0 );
	if( lexer->value == MK_LEX_TYPE_RESERVED_SYMBOL_END )
	{
		isSuccess = 1;
		mk_get_token( vm, lexer );
	}
	else
	{
		if( vm->exceptionObject == NULL )
			mk_raise_internal_error( vm, lexer->name, lexer->line, MK_ERROR_TYPE_COMILE_ERROR | 2, lexer->text, "end", NULL );
	}
	if( lexer->hasError != 0 )
		isSuccess = 0;
	if( isSuccess == 0 )
		mk_destroy_node( result );
	return ( isSuccess != 0 ) ? result : NULL;
}

static
MK_NODE_WHILE *do_compile_do_while_block( MK_VM_STRUCT *vm, MK_LEXER *lexer )
{
	unsigned int isSuccess = 0;
	MK_NODE_WHILE *result = 
		( MK_NODE_WHILE * )mk_create_object( MK_TYPE_NODE_WHILE );
	result->flags = MK_TYPE_NODE_WHILE | MK_TYPE_ATTRIBUTE_NODE_WHILE_BACK;
	do
	{
		// lexer->text == "do"
		mk_get_token( vm, lexer );

		if( lexer->value != MK_LEX_TYPE_EOL )
		{
			if( vm->exceptionObject == NULL )
				mk_raise_internal_error( vm, lexer->name, lexer->line, MK_ERROR_TYPE_COMILE_ERROR | 2, lexer->text, "<CR>", NULL );
			break;
		}

		result->block = create_mk_node_block( MK_TYPE_ATTRIBUTE_BLOCK_WHILE_BLOCK );
		if( do_compile_segment( vm, lexer, result->block, MK_TYPE_SEGMENT_DOWHILE ) == 0 )
		{
			if( vm->exceptionObject == NULL )
				mk_raise_internal_error( vm, lexer->name, lexer->line, MK_ERROR_TYPE_COMILE_ERROR | 11, "<do...while segment>", NULL );
			break;
		}
		// lexer->text == "end"
		mk_get_token( vm, lexer );
		if( lexer->value != MK_LEX_TYPE_RESERVED_SYMBOL_WHILE )
		{
			if( vm->exceptionObject == NULL )
				mk_raise_internal_error( vm, lexer->name, lexer->line, MK_ERROR_TYPE_COMILE_ERROR | 2, lexer->text, "while", NULL );
			break;
		}
		mk_get_token( vm, lexer );
		result->expr = do_compile_expr( vm, lexer, MK_TYPE_EXPR_WHILE_CONDITION );
		if( result->expr == NULL )
		{
			if( vm->exceptionObject == NULL )
				mk_raise_internal_error( vm, lexer->name, lexer->line, MK_ERROR_TYPE_COMILE_ERROR | 11, "do...while condition", NULL );
			break;
		}
		isSuccess = 1;
	}
	while( 0 );
	if( lexer->hasError != 0 )
		isSuccess = 0;
	return ( isSuccess != 0 ) ? result : NULL;
}

static
MK_NODE_EXPR *do_compile_node_with_param( MK_VM_STRUCT *vm, MK_LEXER *lexer )
{
	// lexer->value = '|' or '{' or 'block'
	int argCount = 0;
	int isError = 0;

	MK_NODE_EXPR *newNode = 
		mk_create_object( MK_TYPE_NODE_EXPR | MK_TYPE_NODE_EXPR_NODE_WITH_PARAM );
	MK_VARIABLE *targetNode = 
		mk_create_object( MK_TYPE_VARIABLE );
	targetNode->flags |= MK_TYPE_ATTRIBUTE_VARIABLE_METHOD |
						MK_TYPE_ATTRIBUTE_VARIABLE_METHOD_SCRIPT;
	newNode->u1.node = (unsigned int*)targetNode;

	// do compile_parameter
	if( lexer->value == MK_LEX_TYPE_RESERVED_MARK_OR )
	{
		targetNode->args =
			mk_create_vector( 
				MK_TYPE_VECTOR | MK_TYPE_VECTOR_ARRAY_MANAGED_PTR | ( MK_SIZEOF_EXTEND_VECTOR_DEFAULT << 10 ),
				MK_SIZEOF_VECTOR_DEFAULT,
				(INT_PTR)NULL );
		if( do_compile_parameters( vm, lexer, targetNode->args, MK_LEX_TYPE_RESERVED_MARK_OR ) != 0 &&
			lexer->value == MK_LEX_TYPE_RESERVED_MARK_OR )
			mk_get_token( vm, lexer );
		else
			isError = 1;
		if( mk_size_vector( targetNode->args ) == 0 )
		{
			mk_destroy_vector_node( targetNode->args );
			targetNode->args = NULL;
		}
	}
	
	// do compile node impl
	if( isError == 0 )
	{
		if( lexer->value == MK_LEX_TYPE_RESERVED_MARK_BRACE )
		{
			MK_NODE_EXPR * expr = NULL;
			mk_get_token( vm, lexer );
			expr = do_compile_expr( vm, lexer, MK_TYPE_EXPR_NONAME_EXPR );
			if( expr == NULL )
			{
				isError = 1;
			}
			else
			{
				targetNode->entryPoint = (void*)expr;
			}
		}
		else if( lexer->value == MK_LEX_TYPE_RESERVED_SYMBOL_BLOCK )
		{
			MK_NODE_BLOCK * block = mk_create_object( MK_TYPE_NODE_BLOCK );
			block->flags |= MK_TYPE_ATTRIBUTE_BLOCK_NONAME_METHOD;
			mk_get_token( vm, lexer );
			if( do_compile_segment( vm, lexer, block, MK_TYPE_SEGMENT_NONAME_BLOCK ) == 0 )
			{
				mk_destroy_node( block );
				isError = 1;
			}
			else
			{
				targetNode->entryPoint = (void*)block;
			}
		}
	}
	if( isError != 0 )
	{
		mk_destroy_node( newNode );
		newNode = NULL;
	}
	return newNode;
}

static
MK_TRY_BLOCK *do_compile_try_catch_finally_block( MK_VM_STRUCT *vm, MK_LEXER *lexer )
{
	unsigned int isFail = 0;
	MK_TRY_BLOCK *tryBlock = 
		mk_create_object( MK_TYPE_TRY_BLOCK );

	// try block
	do
	{
		// lexer->text = "try"
		mk_get_token( vm, lexer );
		if( lexer->value != MK_LEX_TYPE_EOL )
		{
			if( vm->exceptionObject == NULL )
				mk_raise_internal_error( vm, lexer->name, 
					lexer->line, 
					MK_ERROR_TYPE_COMILE_ERROR | 2, 
					lexer->text, 
					"<CR>", NULL );
			isFail = 1;
			break;
		}
		tryBlock->blockTry = 
			create_mk_node_block( MK_TYPE_ATTRIBUTE_BLOCK_TRY_BLOCK );
		if( do_compile_segment( vm, lexer, tryBlock->blockTry, MK_TYPE_SEGMENT_TRY ) == 0 )
		{
			if( vm->exceptionObject == NULL )
				mk_raise_internal_error( vm, 
					lexer->name, lexer->line, 
					MK_ERROR_TYPE_COMILE_ERROR | 11, 
					"<try...catch...finally...segment>", NULL );
			isFail = 1;
			break;
		}
	}
	while( 0 );

	if( isFail == 0 )
	{
		// catch block
		MK_CATCH_BLOCK *catchBlock = NULL;

		tryBlock->blockCatch = 
			mk_create_vector( 
				MK_TYPE_VECTOR | MK_TYPE_VECTOR_ARRAY_MANAGED_PTR | ( MK_SIZEOF_EXTEND_VECTOR_DEFAULT << 10 ),
				MK_SIZEOF_VECTOR_DEFAULT,
				(INT_PTR)NULL );
		do
		{
			MK_VARIABLE *param = NULL;
			MK_VARIABLE *block = NULL;
			MK_NODE_BLOCK *blockImpl = NULL;

			catchBlock = 
				mk_create_object( MK_TYPE_CATCH_BLOCK );
			mk_push_vector( tryBlock->blockCatch, (INT_PTR)catchBlock );

			// lexer->text = "catch"
			mk_get_token( vm, lexer );

			catchBlock->paramCatch = 
				do_compile_node_with_param( vm, lexer );
			if( catchBlock->paramCatch == NULL )
			{
				isFail = 1;
				break;
			}
			param = 
				(MK_VARIABLE*)catchBlock->paramCatch->u1.node;
			if( mk_size_vector( param->args ) != 1 )
			{
				isFail = 1; // the size of argument in catch condition must equal 1
				break;
			}
			if(  param->entryPoint != NULL &&
				MK_OBJECT_TYPE( *( (unsigned int*)param->entryPoint) ) != MK_TYPE_NODE_EXPR )
			{
				isFail = 1; // catch condition must be MK_NODE_EXPR type.
				break;
			}
			mk_get_token( vm, lexer );
			
			blockImpl =
				create_mk_node_block( MK_TYPE_ATTRIBUTE_BLOCK_CATCH_BLOCK );
			block = 
				mk_create_object( MK_TYPE_VARIABLE );
			block->flags |=
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD | 
				MK_TYPE_ATTRIBUTE_VARIABLE_METHOD_SCRIPT | 
				MK_TYPE_ATTRIBUTE_VARIABLE_STATIC;
			block->args = param->args;
			block->entryPoint = blockImpl;

			catchBlock->blockCatch = 
				mk_create_object( MK_TYPE_NODE_EXPR );
			catchBlock->blockCatch->flags = 
				MK_TYPE_SET_ATTRIBUTE( catchBlock->blockCatch->flags, MK_TYPE_NODE_EXPR_NODE_WITH_PARAM );
			catchBlock->blockCatch->u1.node = 
				(unsigned int*)block;

			if( do_compile_segment( vm, lexer, blockImpl, MK_TYPE_SEGMENT_CATCH ) == 0 )
			{
				isFail = 1;	// error
				break;
			}

			if( lexer->value == MK_LEX_TYPE_RESERVED_SYMBOL_ELSE ||
				lexer->value == MK_LEX_TYPE_RESERVED_SYMBOL_FINALLY ||
				lexer->value == MK_LEX_TYPE_RESERVED_SYMBOL_END )
			{
				break;
			}
			else if( lexer->value != MK_LEX_TYPE_RESERVED_SYMBOL_CATCH )
			{
				isFail = 1;
				break;
			}
		}
		while( 1 );
	}

	// else block ( no exception )
	if( isFail == 0 && lexer->value == MK_LEX_TYPE_RESERVED_SYMBOL_ELSE )
	{
		// lexer->text == "else" 
		mk_get_token( vm, lexer );
		tryBlock->blockNoException =
			create_mk_node_block( MK_TYPE_ATTRIBUTE_BLOCK_CATCH_BLOCK );
		if( do_compile_segment( vm, lexer, tryBlock->blockNoException, MK_TYPE_SEGMENT_CATCH_ELSE ) == 0 )
			isFail = 1;
	}

	// finally block
	if( isFail == 0 && lexer->value == MK_LEX_TYPE_RESERVED_SYMBOL_FINALLY )
	{
		// lexer-> text == "finally"
		mk_get_token( vm, lexer );
		
		tryBlock->blockFinally =
			create_mk_node_block( MK_TYPE_ATTRIBUTE_BLOCK_CATCH_BLOCK );
		if( do_compile_segment( vm, lexer, tryBlock->blockFinally, MK_TYPE_SEGMENT_FINALLY ) == 0 )
			isFail = 1;
	}

	if( isFail == 0 )
	{
		// lexer->text == "end"
		mk_get_token( vm, lexer );
	}
	else
	{
		mk_destroy_node( tryBlock );
		tryBlock = NULL;
	}
	return tryBlock;
}

static
int do_compile_arguments( MK_VM_STRUCT *vm, MK_LEXER *lexer, MK_VECTOR *arguments, unsigned int styleExpr, unsigned int endOfMark )
{
	MK_NODE_EXPR *arg = NULL;
	int isError = 0;

	mk_get_token( vm, lexer );
	do
	{
		if( lexer->value == endOfMark )
			break;

		arg = do_compile_expr( vm, lexer, styleExpr );
		if( arg == NULL )
		{
			if( vm->exceptionObject == NULL )
				mk_raise_internal_error( vm, lexer->name, lexer->line, MK_ERROR_TYPE_COMILE_ERROR | 11, "<method argument>", NULL );
			isError = 1;
			break;
		}
		mk_push_vector( arguments, (INT_PTR)arg );
		if( lexer->value == MK_LEX_TYPE_RESERVED_MARK_CONMA )
			mk_get_token( vm, lexer );
	}
	while( lexer->value != MK_LEX_TYPE_EOF );

	return isError == 0;
}

static
MK_NODE_EXPR *do_compile_method_call( MK_VM_STRUCT *vm, MK_LEXER *lexer )
{
	void *keyPtr = NULL;
	unsigned int isSuccess = 0;
	MK_NODE_EXPR *result = 
		( MK_NODE_EXPR * )mk_create_object( MK_TYPE_NODE_EXPR );
	result->flags = 
		MK_TYPE_NODE_EXPR | 
		MK_TYPE_NODE_EXPR_FUNCTION_CALL;
	do
	{
		result->u1.symbolName = mk_get_symbol_name_ptr( vm, lexer->text );
		mk_get_token( vm, lexer );
		if( lexer->value != MK_LEX_TYPE_RESERVED_MARK_PARENTHESIS )
		{
			if( vm->exceptionObject == NULL )
				mk_raise_internal_error( vm, lexer->name, lexer->line, MK_ERROR_TYPE_COMILE_ERROR | 2, lexer->text, "(", NULL );
			break;
		}

		result->u2.args = 
			mk_create_vector( 
				MK_TYPE_VECTOR | MK_TYPE_VECTOR_ARRAY_MANAGED_PTR | ( MK_SIZEOF_EXTEND_VECTOR_DEFAULT << 10 ),
				MK_SIZEOF_VECTOR_DEFAULT,
				(INT_PTR)NULL );
		if( ( do_compile_arguments( vm, 
								lexer, 
								result->u2.args,
								MK_TYPE_EXPR_METHOD_ARG, 
								MK_LEX_TYPE_RESERVED_MARK_END_PARENTHESIS ) == 0 ) ||
			lexer->value != MK_LEX_TYPE_RESERVED_MARK_END_PARENTHESIS )
		{
			if( vm->exceptionObject == NULL )
				mk_raise_internal_error( vm, lexer->name, lexer->line, MK_ERROR_TYPE_COMILE_ERROR | 2, lexer->text, ")", NULL );
			break;
		}
		else if( mk_size_vector( result->u2.args ) == 0 )
		{
			mk_destroy_vector_node( result->u2.args );
			result->u2.args = NULL;
		}
		isSuccess = 1;
	}
	while( 0 );

	if( lexer->hasError != 0 )
		isSuccess = 0;

	if( isSuccess == 0 && result != NULL )
		mk_destroy_node( result );

	return ( isSuccess != 0 ) ? result : NULL;
}

static
MK_NODE_EXPR *do_compile_symbol( MK_VM_STRUCT *vm, MK_LEXER *lexer, unsigned int state )
{
	MK_NODE_EXPR *multipleTop = NULL;
	MK_NODE_EXPR *current = NULL;
	int neg = 0;
	int isError = 0;
	do
	{
		current = NULL;
		if( lexer->value == MK_LEX_TYPE_RESERVED_MARK_PARENTHESIS )
		{
			mk_get_token( vm, lexer );
			current = 
				do_compile_expr( vm, lexer, MK_TYPE_EXPR_INNER_EXPR );
		}
		else
		{
			if( ( lexer->value == MK_LEX_TYPE_RESERVED_MARK_PLUS ||
					lexer->value == MK_LEX_TYPE_RESERVED_MARK_MINUS ) )
			{
				if( lexer->value == MK_LEX_TYPE_RESERVED_MARK_MINUS )
					neg = 1;
				else if( lexer->value == MK_LEX_TYPE_RESERVED_MARK_PLUS )
					neg = 2;
				mk_get_token( vm, lexer );
			}
			if( lexer->value != MK_LEX_TYPE_INT_VALUE &&
				lexer->value != MK_LEX_TYPE_FLOAT_VALUE &&
				neg != 0 )
			{
				if( vm->exceptionObject == NULL )
					mk_raise_internal_error( vm, 
											lexer->name, 
											lexer->line, 
											MK_ERROR_TYPE_COMILE_ERROR | 20, 
											lexer->text, 
											NULL );
				isError = 1;
			}
			else if( lexer->value == MK_LEX_TYPE_RESERVED_SYMBOL_CLASS )
			{
				lexer->value = MK_LEX_TYPE_SYMBOL;
			}

			switch( lexer->value )
			{
			case MK_LEX_TYPE_RESERVED_MARK_BRACE:
			case MK_LEX_TYPE_RESERVED_MARK_OR:
			case MK_LEX_TYPE_RESERVED_SYMBOL_BLOCK:
				current = do_compile_node_with_param( vm, lexer );
				break;
				
			case MK_LEX_TYPE_RESERVED_MARK_BRACKET:
				current = 
					mk_create_object( MK_TYPE_NODE_EXPR );
				current->flags |= MK_TYPE_NODE_EXPR_ARRAY_DEFINITION;
				current->u1.arrayDefinition = 
					mk_create_vector( 
						MK_TYPE_VECTOR | MK_TYPE_VECTOR_ARRAY_MANAGED_PTR | ( MK_SIZEOF_EXTEND_VECTOR_DEFAULT << 10 ),
						MK_SIZEOF_VECTOR_DEFAULT,
						(INT_PTR)NULL );
				if( ( do_compile_arguments( vm,
									lexer,
									current->u1.arrayDefinition,
									MK_TYPE_EXPR_ARRAY_DEFINITION, 
									MK_LEX_TYPE_RESERVED_MARK_END_BRACKET ) == 0 ) ||
					lexer->value != MK_LEX_TYPE_RESERVED_MARK_END_BRACKET )
				{
					if( vm->exceptionObject == NULL )
						mk_raise_internal_error( vm, lexer->name, lexer->line, MK_ERROR_TYPE_COMILE_ERROR | 11, "<array definition>", NULL );
					mk_destroy_node( current );
					current = NULL;
				}
				else if( mk_size_vector( current->u1.arrayDefinition ) == 0 )
				{
					mk_destroy_vector_node( current->u1.arrayDefinition );
					current->u1.arrayDefinition = NULL;
				}
				break;

			default:
				switch( lexer->value & MK_LEX_TYPE_MASK )
				{

				case MK_LEX_TYPE_INT_VALUE:
				case MK_LEX_TYPE_FLOAT_VALUE:
					if( multipleTop != NULL )
						mk_raise_internal_error( vm, 
												lexer->name,
												lexer->line,
												MK_ERROR_TYPE_COMILE_ERROR | 3,
												".",
												lexer->text,
												"<symbol>",
												NULL );	// error
					else
						current = create_numeric_expr( lexer->value, lexer->text, neg );
					break;

				case MK_LEX_TYPE_CHARCTER:
					current = lexer->object;
					lexer->object = NULL;
					break;

				case MK_LEX_TYPE_SYMBOL:
					if( lexer->nextValue == MK_LEX_TYPE_RESERVED_MARK_PARENTHESIS )
						current = do_compile_method_call( vm, lexer );
					else
						current = create_symbol_expr( vm, lexer );
					break;

				case MK_LEX_TYPE_RESERVED_MARK:
					switch( lexer->value )
					{
					case MK_LEX_TYPE_RESERVED_MARK_DOUBLE_QUATE:
						current = do_compile_parse_string( vm, lexer );
						break;

					case MK_LEX_TYPE_RESERVED_MARK_DBLATMARK:
						if( ( lexer->nextValue & MK_LEX_TYPE_MASK ) == MK_LEX_TYPE_SYMBOL )
						{
							mk_get_token( vm, lexer );
							if( lexer->nextValue == MK_LEX_TYPE_RESERVED_MARK_PARENTHESIS )
							{
								current = do_compile_method_call( vm, lexer );
								current->flags = 
									MK_TYPE_SET_ATTRIBUTE(current->flags, MK_TYPE_NODE_EXPR_FUNCTION_CALL_STATIC );
							}
							else
							{
								current = create_static_member_variable_expr( vm, lexer );
							}
						}
						break;
					case MK_LEX_TYPE_RESERVED_MARK_ATMARK:
						if( ( lexer->nextValue & MK_LEX_TYPE_MASK ) == MK_LEX_TYPE_SYMBOL )
						{
							mk_get_token( vm, lexer );
							if( lexer->nextValue == MK_LEX_TYPE_RESERVED_MARK_PARENTHESIS )
							{
								current = do_compile_method_call( vm, lexer );
								current->flags = 
									MK_TYPE_SET_ATTRIBUTE(current->flags, MK_TYPE_NODE_EXPR_FUNCTION_CALL_INSTANCE );
							}
							else
							{
								current = create_member_variable_expr( vm, lexer );
							}
						}
						break;
					}
					break;
				case MK_LEX_TYPE_RESERVED_SYMBOL:
					switch( lexer->value )
					{
					case MK_LEX_TYPE_RESERVED_SYMBOL_NEW:
						current = create_new_expr( vm, lexer );
						break;
					
					case MK_LEX_TYPE_RESERVED_SYMBOL_ME:
						current = create_me_expr( vm, lexer );
						break;

					case MK_LEX_TYPE_RESERVED_SYMBOL_THIS:
						current = create_this_expr( );
						break;

					case MK_LEX_TYPE_RESERVED_SYMBOL_SUPER:
						if( lexer->nextValue == MK_LEX_TYPE_RESERVED_MARK_PARENTHESIS )
						{
							current = do_compile_method_call( vm, lexer );
							if( current != NULL )
								current->flags = 
									MK_TYPE_NODE_EXPR | MK_TYPE_NODE_EXPR_SUPER_CALL;
						}
						else
						{
							current = create_super_expr( );
						}
						break;

					case MK_LEX_TYPE_RESERVED_SYMBOL_OWNER:
						current = create_owner_expr( );
						break;

					case MK_LEX_TYPE_RESERVED_SYMBOL_NIL:
						current = create_nil_expr( );
						break;
					case MK_LEX_TYPE_RESERVED_SYMBOL_TRUE:
						current = create_true_expr( );
						break;
					case MK_LEX_TYPE_RESERVED_SYMBOL_FALSE:
						current = create_false_expr( );
						break;
					}
					break;
				default:
					break;
				}
			}
		}
		if( current == NULL )
		{
			if( vm->exceptionObject == NULL )
				mk_raise_internal_error( vm, lexer->name, lexer->line, 
										MK_ERROR_TYPE_COMILE_ERROR | 19, "<symbol name>", lexer->text, lexer->nextText, NULL );
			isError = 1;
			break;
		}
		mk_get_token( vm, lexer );

		while( lexer->value == MK_LEX_TYPE_RESERVED_MARK_BRACKET )
		{
			MK_NODE_EXPR *rightValue = NULL;
			MK_NODE_EXPR *pOperator = 
				create_operation_expr( lexer->value );
			mk_get_token( vm, lexer );
			rightValue = 
				do_compile_expr( vm, lexer, MK_TYPE_EXPR_ARRAY_INDEX );
			if( rightValue != 0 )
			{
				if( multipleTop != NULL )
				{
					mk_push_vector( multipleTop->u1.multipleSymbols, (INT_PTR)current );
					pOperator->u1.left = multipleTop;
					multipleTop = NULL;
				}
				else
				{
					pOperator->u1.left = current;
				}
				pOperator->u2.right = rightValue;
				current = pOperator;
			}
			else
			{
				// error
				mk_destroy_node(  pOperator );
				isError = 1;
				break;
			}
			mk_get_token( vm, lexer );
		}
		if( isError != 0 )
			break;

		if( lexer->value == MK_LEX_TYPE_RESERVED_MARK_DOT &&
			multipleTop == NULL )
		{
			multipleTop = create_multiple_top_expr( );
			multipleTop->u1.multipleSymbols = 
				mk_create_vector( 
					MK_TYPE_VECTOR | MK_TYPE_VECTOR_ARRAY_MANAGED_PTR | ( MK_SIZEOF_EXTEND_VECTOR_DEFAULT << 10 ),
					MK_SIZEOF_VECTOR_DEFAULT,
					(INT_PTR)NULL );
		}
		if( multipleTop != NULL )
		{
			mk_push_vector( multipleTop->u1.multipleSymbols, (INT_PTR)current );
		}
		if( lexer->value != MK_LEX_TYPE_RESERVED_MARK_DOT )
			break;
		mk_get_token( vm, lexer );
	}while( lexer->value != MK_LEX_TYPE_EOF );

	if( lexer->hasError != 0 )
		isError = 1;

	if( isError != 0 )
	{
		if( multipleTop != NULL )
			mk_destroy_node( multipleTop );
		else if( current != NULL )
			mk_destroy_node( current );
		multipleTop = NULL;
		current = NULL;
	}
	return ( multipleTop != NULL ) ? multipleTop : current;
}

static
MK_NODE_EXPR *do_compile_expr_root( MK_VM_STRUCT *vm, MK_LEXER *lexer, MK_NODE_EXPR *statement )
{
	if( statement == NULL )
		statement = do_compile_expr( vm, lexer, MK_TYPE_EXPR_ROOT_SEGMENT );
	while( lexer->value != MK_LEX_TYPE_EOL && lexer->value != MK_LEX_TYPE_EOF )
	{
		if( statement != NULL )
		{
			MK_NODE_EXPR *parent = NULL;
			if( lexer->value == MK_LEX_TYPE_RESERVED_SYMBOL_IF )
				parent = mk_create_object( MK_TYPE_NODE_EXPR | MK_TYPE_NODE_EXPR_BACK_IF_CONDITION );
			if( parent != NULL )
			{
				parent->u1.left = statement;
				mk_get_token( vm, lexer );
				statement = do_compile_expr( vm, lexer, MK_TYPE_EXPR_ROOT_SEGMENT );
				parent->u2.right = statement;
				statement = parent;
			}
		}
		else
		{
			break;
		}
	}
	return statement;
}

MK_NODE_EXPR *do_compile_expr( MK_VM_STRUCT *vm, MK_LEXER *lexer,  unsigned int state )
{
	int isError = 0;
	int isEndExpr = 0;
	unsigned int type = 0;
	MK_VECTOR *stack = 
		mk_create_vector( 
			MK_TYPE_VECTOR | MK_TYPE_VECTOR_ARRAY_MANAGED_PTR | ( MK_SIZEOF_EXTEND_VECTOR_DEFAULT << 10 ),
			MK_SIZEOF_VECTOR_DEFAULT,
			(INT_PTR)NULL );
	MK_NODE_EXPR *result = NULL;
	MK_NODE_EXPR *current = NULL;
	do
	{
		unsigned int sizeStack = 
			mk_size_vector( stack );
		current = do_compile_symbol( vm, lexer, state );
		if( current == NULL )
		{
			isError = 1;
			break;
		}

		// check expr end.
		switch( state )
		{
		case MK_TYPE_EXPR_IF_CONDITION:
			if( lexer->value == MK_LEX_TYPE_RESERVED_SYMBOL_THEN )
				isEndExpr = 1;
			break;
		case MK_TYPE_EXPR_WHILE_CONDITION:
			if( lexer->value == MK_LEX_TYPE_EOL ||
				lexer->value == MK_LEX_TYPE_EOF )
				isEndExpr = 1;
			break;
		case MK_TYPE_EXPR_METHOD_ARG:
			if( lexer->value == MK_LEX_TYPE_RESERVED_MARK_END_PARENTHESIS ||
				lexer->value == MK_LEX_TYPE_RESERVED_MARK_CONMA )
				isEndExpr = 1;
			break;
		case MK_TYPE_EXPR_METHOD_DEFAULT_PARAM:
			if( lexer->value == MK_LEX_TYPE_RESERVED_MARK_END_PARENTHESIS ||
				lexer->value == MK_LEX_TYPE_RESERVED_MARK_CONMA )
				isEndExpr = 1;
			break;
		case MK_TYPE_EXPR_ROOT_SEGMENT:
			if( lexer->value == MK_LEX_TYPE_EOL ||
				lexer->value == MK_LEX_TYPE_EOF ||
				lexer->value == MK_LEX_TYPE_RESERVED_SYMBOL_IF )
			{
				isEndExpr = 1;
			}
			break;
		case MK_TYPE_EXPR_INNER_EXPR:
			if( lexer->value == MK_LEX_TYPE_RESERVED_MARK_END_PARENTHESIS )
				isEndExpr = 1;
			break;
		case MK_TYPE_EXPR_NONAME_EXPR:
			if( lexer->value == MK_LEX_TYPE_RESERVED_MARK_END_BRACE )
				isEndExpr = 1;
			break;
		case MK_TYPE_EXPR_ARRAY_DEFINITION:
			if( lexer->value == MK_LEX_TYPE_RESERVED_MARK_END_BRACKET ||
				lexer->value == MK_LEX_TYPE_RESERVED_MARK_CONMA )
				isEndExpr = 1;
			break;
		case MK_TYPE_EXPR_ARRAY_INDEX:
			if( lexer->value == MK_LEX_TYPE_RESERVED_MARK_END_BRACKET )
				isEndExpr = 1;
			break;
		case MK_TYPE_EXPR_NODE_PARAM_DEFAULT:
			if( ( lexer->value == MK_LEX_TYPE_RESERVED_MARK_OR ) &&
				( lexer->nextValue == MK_LEX_TYPE_RESERVED_MARK_BRACE ||
				  lexer->nextValue == MK_LEX_TYPE_RESERVED_SYMBOL_BLOCK ) )
				isEndExpr = 1;
			else if( lexer->value == MK_LEX_TYPE_RESERVED_MARK_CONMA )
				isEndExpr = 1;
			break;

		default:
			isError = 1;
			break;
		}
		if( isEndExpr != 0 )
		{
			if( sizeStack != 0 )
			{
				MK_NODE_EXPR *top = 
					(MK_NODE_EXPR*)mk_get_at_vector( stack, 0 );
				MK_NODE_EXPR *last = 
					(MK_NODE_EXPR*)mk_get_at_vector( stack, sizeStack - 1 );
				last->u2.right = current;
				result = top;
			}
			else
			{
				result = current;
			}
			current = NULL;
			break;
		}
		if( ( lexer->value & MK_LEX_TYPE_MASK ) == MK_LEX_TYPE_RESERVED_MARK &&
			MK_LEX_RESERVED_MARK_PRIORITY( lexer->value ) != 0 )
		{
			int index = 0;
			MK_NODE_EXPR *pExpr = NULL;
			MK_NODE_EXPR *pOperator = 
				create_operation_expr( lexer->value );
			int currentPriority = MK_LEX_RESERVED_MARK_PRIORITY( lexer->value );
			for( index = sizeStack - 1; index >= 0; index -- )
			{
				int targetPriority = 0;
				pExpr = 
					(MK_NODE_EXPR*)mk_get_at_vector( stack, index );
				targetPriority = MK_LEX_RESERVED_MARK_PRIORITY( pExpr->flags );
				if( targetPriority < currentPriority )
					break;
			}
			if( index == -1 )
				pExpr = NULL;

			if( pExpr == NULL )
			{
				if( sizeStack == 0 )
				{
					pOperator->u1.left = current;
				}
				else
				{
					MK_NODE_EXPR *last = 
						(MK_NODE_EXPR*)mk_get_at_vector( stack, sizeStack - 1 );
					last->u2.right = current;
					pOperator->u1.left = 
						(MK_NODE_EXPR*)mk_get_at_vector( stack, 0 );
					stack->used = 0;
				}
				mk_insert_at_vector( stack, 0, (INT_PTR)pOperator );
			}
			else
			{
				MK_NODE_EXPR *last = 
					(MK_NODE_EXPR*)mk_get_at_vector( stack, sizeStack - 1 );
				last->u2.right = current;
				pOperator->u1.left = pExpr->u2.right;
				pExpr->u2.right = pOperator;
				stack->used = index + 1;
				mk_push_vector( stack, (INT_PTR)pOperator );
			}
			current = NULL;
		}
		else
		{
			if( vm->exceptionObject == NULL )
				mk_raise_internal_error( vm, lexer->name, lexer->line, MK_ERROR_TYPE_COMILE_ERROR | 2, lexer->text, "<operator>", NULL );
			isError = 1;
			break;
		}
		mk_get_token( vm, lexer );
	}
	while ( lexer->value != MK_LEX_TYPE_EOF );

	if( lexer->hasError != 0 )
		isError = 1;

	if( isError != 0 && result != NULL )
	{
		mk_destroy_node( result );
		result = NULL;
	}
	if( result == NULL )
	{
		if( current != NULL )
			mk_destroy_node( current );
	}
	else
	{
		mk_trim_size_vector( stack, 0 );
	}
	mk_destroy_vector_node( stack );
	return ( isEndExpr != 0 ) ? result : NULL;
}

static
int do_compile_segment( MK_VM_STRUCT *vm, MK_LEXER *lexer, MK_NODE_BLOCK *block, unsigned int typeSegment )
{
	unsigned int isSuccess = 0;
	MK_VECTOR *target = block->exprs;
	if( target == NULL )
	{
		target = 
			mk_create_vector( 
				MK_TYPE_VECTOR | MK_TYPE_VECTOR_ARRAY_MANAGED_PTR | ( MK_SIZEOF_EXTEND_VECTOR_DEFAULT << 10 ),
				MK_SIZEOF_VECTOR_DEFAULT,
				(INT_PTR)NULL );
		block->exprs = target;
	}
	do
	{
		void *statement = NULL;

		switch( typeSegment )
		{
		case MK_TYPE_SEGMENT_IF:
			if( lexer->value == MK_LEX_TYPE_RESERVED_SYMBOL_END ||
				lexer->value == MK_LEX_TYPE_RESERVED_SYMBOL_ELSEIF ||
				lexer->value == MK_LEX_TYPE_RESERVED_SYMBOL_ELSE )
				isSuccess = 1;
			break;
		case MK_TYPE_SEGMENT_WHILE:
			if( lexer->value == MK_LEX_TYPE_RESERVED_SYMBOL_END )
				isSuccess = 1;
			break;
		case MK_TYPE_SEGMENT_BLOCK:
			if( lexer->value == MK_LEX_TYPE_RESERVED_SYMBOL_END )
				isSuccess = 1;
			break;
		case MK_TYPE_SEGMENT_ROOT:
			if( lexer->value == MK_LEX_TYPE_RESERVED_SYMBOL_CLASS ||
				lexer->value == MK_LEX_TYPE_RESERVED_SYMBOL_MODULE ||
				lexer->value == MK_LEX_TYPE_RESERVED_SYMBOL_DEF ||
				lexer->value == MK_LEX_TYPE_EOF )
				isSuccess = 1;
			break;
		case MK_TYPE_SEGMENT_DOWHILE:
			if( lexer->value == MK_LEX_TYPE_RESERVED_SYMBOL_END )
				isSuccess = 1;
			break;
		case MK_TYPE_SEGMENT_TRY:
			if( lexer->value == MK_LEX_TYPE_RESERVED_SYMBOL_CATCH )
				isSuccess = 1;
			break;
		case MK_TYPE_SEGMENT_CATCH:
			if( lexer->value == MK_LEX_TYPE_RESERVED_SYMBOL_CATCH ||
				lexer->value == MK_LEX_TYPE_RESERVED_SYMBOL_ELSE ||
				lexer->value == MK_LEX_TYPE_RESERVED_SYMBOL_FINALLY ||
				lexer->value == MK_LEX_TYPE_RESERVED_SYMBOL_END )
				isSuccess = 1;
		case MK_TYPE_SEGMENT_CATCH_ELSE:
			if( lexer->value == MK_LEX_TYPE_RESERVED_SYMBOL_FINALLY ||
				lexer->value == MK_LEX_TYPE_RESERVED_SYMBOL_END )
				isSuccess = 1;
			break;
		case MK_TYPE_SEGMENT_FINALLY:
			if( lexer->value == MK_LEX_TYPE_RESERVED_SYMBOL_END )
				isSuccess = 1;
			break;
		case MK_TYPE_SEGMENT_NONAME_BLOCK:
			if( lexer->value == MK_LEX_TYPE_RESERVED_SYMBOL_END )
				isSuccess = 1;
			break;
		default:
			break;
		}
		if( isSuccess != 0 )
			break;

		if( lexer->value == MK_LEX_TYPE_EOF )
		{
			if( vm->exceptionObject == NULL )
				mk_raise_internal_error( vm, lexer->name, lexer->line, MK_ERROR_TYPE_COMILE_ERROR | 12, "<segment>", lexer->text, NULL );
			break;
		}

		switch( lexer->value )
		{
		case MK_LEX_TYPE_RESERVED_SYMBOL_IF:
			statement = do_compile_if_block( vm, lexer );
			break;
		case MK_LEX_TYPE_RESERVED_SYMBOL_DO:
			statement = do_compile_do_while_block( vm, lexer );
			break;
		case MK_LEX_TYPE_RESERVED_SYMBOL_WHILE:
			statement = do_compile_while_block( vm, lexer );
			break;
		case MK_LEX_TYPE_RESERVED_SYMBOL_RETURN:
			statement = do_compile_return_state( vm, lexer );
			if( statement != NULL &&
				lexer->value != MK_LEX_TYPE_EOF &&
				lexer->value != MK_LEX_TYPE_EOL )
				statement = do_compile_expr_root( vm, lexer, statement );
			break;
		case MK_LEX_TYPE_RESERVED_SYMBOL_BREAK:
			statement = do_compile_break_state( vm, lexer );
			if( statement != NULL &&
				lexer->value != MK_LEX_TYPE_EOF &&
				lexer->value != MK_LEX_TYPE_EOL )
				statement = do_compile_expr_root( vm, lexer, statement );
			break;
		case MK_LEX_TYPE_RESERVED_SYMBOL_CONTINUE:
			statement = do_compile_continue_state( vm, lexer );
			if( statement != NULL &&
				lexer->value != MK_LEX_TYPE_EOF &&
				lexer->value != MK_LEX_TYPE_EOL )
				statement = do_compile_expr_root( vm, lexer, statement );
			break;
		case MK_LEX_TYPE_RESERVED_SYMBOL_RAISE:
			statement = do_compile_raise_state( vm, lexer );
			if( statement != NULL &&
				lexer->value != MK_LEX_TYPE_EOF &&
				lexer->value != MK_LEX_TYPE_EOL )
				statement = do_compile_expr_root( vm, lexer, statement );
			break;
		case MK_LEX_TYPE_RESERVED_SYMBOL_TRY:
			statement = do_compile_try_catch_finally_block( vm, lexer );
			break;
		case MK_LEX_TYPE_EOL:
			mk_get_token( vm, lexer );
			continue;
		default:
			statement = do_compile_expr_root( vm, lexer, NULL );
			break;
		}
		if( statement == NULL )
		{
			if( vm->exceptionObject == NULL )
				mk_raise_internal_error( vm, lexer->name, lexer->line, MK_ERROR_TYPE_COMILE_ERROR | 11, "<segment>", NULL );
			break;
		}
		mk_push_vector( target, (INT_PTR)statement );
	}
	while( 1 );

	if( lexer->hasError != 0 )
		isSuccess = 0;
	
	if( isSuccess == 0 && vm->exceptionObject == NULL)
		mk_raise_internal_error( vm, lexer->name, lexer->line, MK_ERROR_TYPE_COMILE_ERROR | 23, NULL );

	return ( isSuccess != 0 ) ? 1 : 0;
}

static
int do_compile_parameters( MK_VM_STRUCT *vm, MK_LEXER *lexer, MK_VECTOR *parameters, unsigned int endOfMark )
{
	int argCount = 0;
	int isError = 0;
	int hasDefaultParam = 0;
	
	// lexer->text == "(", "[", ...
	mk_get_token( vm, lexer );

	while( 1 )
	{
		MK_VARIABLE *arg = NULL;
		if( lexer->value == endOfMark )
			break;
		if( argCount > 0 )
		{
			if( lexer->value != MK_LEX_TYPE_RESERVED_MARK_CONMA )
			{
				if( vm->exceptionObject == NULL )
					mk_raise_internal_error( vm, lexer->name, lexer->line, MK_ERROR_TYPE_COMILE_ERROR | 2, lexer->text, ",", NULL );
				isError = 1;
				break;
			}
			mk_get_token( vm, lexer );
		}
		if( ( lexer->value & MK_LEX_TYPE_MASK ) != MK_LEX_TYPE_SYMBOL )
		{
			if( vm->exceptionObject == NULL )
				mk_raise_internal_error( vm, lexer->name, lexer->line, MK_ERROR_TYPE_COMILE_ERROR | 2, lexer->text, "<symbol name>", NULL );
			isError = 1;
			break;
		}
		arg = 
			( MK_VARIABLE * )mk_create_object( MK_TYPE_VARIABLE );
		arg->name = 
			mk_get_symbol_name_ptr( vm, lexer->text );
		if( lexer->nextValue == MK_LEX_TYPE_RESERVED_MARK_EXCLAMATION )
		{
			arg->flags |= MK_TYPE_ATTRIBUTE_VARIABLE_CALL_BY_VALUE;
			mk_get_token( vm, lexer );
		}
		mk_get_token( vm, lexer );
		if( lexer->value == MK_LEX_TYPE_RESERVED_MARK_EQUAL )
		{
			mk_get_token( vm, lexer );
			if( endOfMark == MK_LEX_TYPE_RESERVED_MARK_END_PARENTHESIS )
			{
				arg->defaultValue = 
					do_compile_expr( vm, lexer, MK_TYPE_EXPR_METHOD_DEFAULT_PARAM );
			}
			else if( endOfMark == MK_LEX_TYPE_RESERVED_MARK_OR )
			{
				arg->defaultValue = 
					do_compile_expr( vm, lexer, MK_TYPE_EXPR_NODE_PARAM_DEFAULT );
			}
			else
			{
				isError = 1;
				break;
			}
			if( arg->defaultValue == NULL )
			{
				isError = 1;
				break;
			}
			hasDefaultParam = 1;
		}
		else if( hasDefaultParam != 0 )
		{
			isError = 1;
			mk_raise_internal_error( vm, lexer->name, lexer->line, MK_ERROR_TYPE_COMILE_ERROR | 24, NULL );
			break;
		}
		if( mk_register_variable_arg( vm, parameters, arg ) != MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP )
		{
			if( vm->exceptionObject == NULL )
				mk_raise_internal_error( vm, lexer->name, lexer->line, MK_ERROR_TYPE_COMILE_ERROR | 15, arg->name, NULL );
			mk_destroy_node( arg );
			isError = 1;
			break;
		}
		argCount ++;
	}
	return isError == 0 ;
}

static
MK_VARIABLE* do_compile_method( MK_VM_STRUCT *vm, MK_LEXER *lexer, int isModule )
{
	int isSuccess = 0;
	int isError = 0;
	MK_VARIABLE *newMethod = 
		(MK_VARIABLE*)mk_create_object( MK_TYPE_VARIABLE );

	newMethod->flags |= ( MK_TYPE_ATTRIBUTE_VARIABLE_METHOD );
	do
	{
		if( lexer->value != MK_LEX_TYPE_RESERVED_SYMBOL_DEF )
			break;
		mk_get_token( vm, lexer );
		switch( lexer->value & MK_LEX_TYPE_MASK )
		{
		case MK_LEX_TYPE_RESERVED_SYMBOL:
			switch( lexer->value )
			{
			case MK_LEX_TYPE_RESERVED_SYMBOL_CLASS:
			case MK_LEX_TYPE_RESERVED_SYMBOL_MODULE:
				if( ( lexer->value == MK_LEX_TYPE_RESERVED_SYMBOL_CLASS &&
					isModule != 0 ) ||
					( lexer->value == MK_LEX_TYPE_RESERVED_SYMBOL_MODULE &&
					isModule == 0 ) )
				{
					isError = 1;
					break;
				}
				newMethod->flags |= MK_TYPE_ATTRIBUTE_VARIABLE_STATIC;
				mk_get_token( vm, lexer );
				if( lexer->value != MK_LEX_TYPE_RESERVED_MARK_DOT )
				{
					if( vm->exceptionObject == NULL )
						mk_raise_internal_error( vm, lexer->name, lexer->line, MK_ERROR_TYPE_COMILE_ERROR | 2, lexer->text, ".", NULL );
					isError = 1;
					break;
				}
				mk_get_token( vm, lexer );
				break;

			case MK_LEX_TYPE_RESERVED_SYMBOL_INITIALIZE:
				break;

			default:
				isError = 1;
				break;
			}
			break;

		case MK_LEX_TYPE_SYMBOL:
			break;

		case MK_LEX_TYPE_RESERVED_MARK:
			switch( lexer->value )
			{	// cannot overwrite.
			case MK_LEX_TYPE_RESERVED_MARK_SAME:
			case MK_LEX_TYPE_RESERVED_MARK_NOT_SAME:
			case MK_LEX_TYPE_RESERVED_MARK_EQUAL:
			case MK_LEX_TYPE_RESERVED_MARK_DOT:
			case MK_LEX_TYPE_RESERVED_MARK_PARENTHESIS:
			case MK_LEX_TYPE_RESERVED_MARK_END_PARENTHESIS:
			case MK_LEX_TYPE_RESERVED_MARK_END_BRACKET:
			case MK_LEX_TYPE_RESERVED_MARK_BRACE:
			case MK_LEX_TYPE_RESERVED_MARK_END_BRACE:
				isError = 1;
				break;

			default:
				{
					unsigned int index = MK_LEX_RESERVED_MARK_INDEX(lexer->value);
					unsigned int priority = MK_LEX_RESERVED_MARK_PRIORITY(lexer->value);
					if( priority == 0 )
					{
						isError = 1;
						break;
					}
					newMethod->flags |= MK_TYPE_ATTRIBUTE_VARIABLE_METHOD_OPERATOR;
					newMethod->flags |= ( index << 14 );
				}
			}
			break;

		default:
			isError = 1;
			break;
		}
		if( isError == 1 )
		{
			if( vm->exceptionObject == NULL )
				mk_raise_internal_error( vm, 
					lexer->name, lexer->line, 
					MK_ERROR_TYPE_COMILE_ERROR | 2, 
					lexer->text, "[class|initialize|<operator>|<symbol name>]", NULL );
		}

		newMethod->name = 
				mk_get_symbol_name_ptr( vm, lexer->text );

		mk_get_token( vm, lexer );
		
		switch( lexer->value )
		{
		case MK_LEX_TYPE_RESERVED_MARK_PARENTHESIS:
			newMethod->args = mk_create_vector( 
				MK_TYPE_VECTOR | MK_TYPE_VECTOR_ARRAY_MANAGED_PTR | ( MK_SIZEOF_EXTEND_VECTOR_DEFAULT << 10 ),
				MK_SIZEOF_VECTOR_DEFAULT,
				(INT_PTR)NULL );
			if( do_compile_parameters( vm, lexer, newMethod->args, MK_LEX_TYPE_RESERVED_MARK_END_PARENTHESIS ) == 0 )
				isError = 1;
			break;

		default:
			if( vm->exceptionObject == NULL )
				mk_raise_internal_error( vm, lexer->name, lexer->line, MK_ERROR_TYPE_COMILE_ERROR | 2, lexer->text, "[(|<CR>]", NULL );
			isError = 1;
			break;
		}
		if( isError != 0 )
			break;
		
		mk_get_token( vm, lexer );
		if( lexer->value == MK_LEX_TYPE_EOL )
		{
			newMethod->flags |= MK_TYPE_ATTRIBUTE_VARIABLE_READ_PUBLIC;
		}
		else
		{
			switch( lexer->value )
			{
			case MK_LEX_TYPE_RESERVED_MARK_PLUS:
				newMethod->flags |= MK_TYPE_ATTRIBUTE_VARIABLE_READ_PUBLIC;
				break;
			case MK_LEX_TYPE_RESERVED_MARK_MUL:
				newMethod->flags |= MK_TYPE_ATTRIBUTE_VARIABLE_READ_PROTECTED;
				break;
			case MK_LEX_TYPE_RESERVED_MARK_MINUS:
				newMethod->flags |= MK_TYPE_ATTRIBUTE_VARIABLE_READ_PRIVATE;
				break;
			default:
				if( vm->exceptionObject == NULL )
					mk_raise_internal_error( vm, lexer->name, lexer->line, MK_ERROR_TYPE_COMILE_ERROR | 2, lexer->text, "<CR>", NULL );
				isError = 1;
				break;
			}
			mk_get_token( vm, lexer );
			if( lexer->value != MK_LEX_TYPE_EOL )
				isError = 1;
		}
		if( isError != 0 )
			break;

		newMethod->entryPoint =
			create_mk_node_block( MK_TYPE_ATTRIBUTE_BLOCK_FUNCTION_ROOT );
		if( do_compile_segment( vm, lexer, (MK_NODE_BLOCK*)newMethod->entryPoint, MK_TYPE_SEGMENT_BLOCK ) == 0 )
		{
			if( vm->exceptionObject == NULL )
				mk_raise_internal_error( vm, lexer->name, lexer->line, MK_ERROR_TYPE_COMILE_ERROR | 10, newMethod->name, "method", NULL );
			break;
		}
		if( lexer->value != MK_LEX_TYPE_RESERVED_SYMBOL_END )
		{
			if( vm->exceptionObject == NULL )
				mk_raise_internal_error( vm, lexer->name, lexer->line, MK_ERROR_TYPE_COMILE_ERROR | 2, lexer->text, "end", NULL );
			break;
		}
		mk_get_token( vm, lexer );
		if( lexer->value != MK_LEX_TYPE_EOL &&
			lexer->value != MK_LEX_TYPE_EOF )
		{
			if( vm->exceptionObject == NULL )
				mk_raise_internal_error( vm, lexer->name, lexer->line, MK_ERROR_TYPE_COMILE_ERROR | 3, "end", lexer->text, "<CR>", NULL );
			break;
		}
		isSuccess = 1;
	}
	while( 0 );

	if( lexer->hasError != 0 )
		isSuccess = 0;

	if( isSuccess == 0 )
	{
		mk_destroy_node( newMethod );
		newMethod = NULL;
	}
	return newMethod;
}

static
int do_compile_property( MK_VM_STRUCT *vm, MK_LEXER *lexer, MK_CLASS *targetClass )
{
	int isError = 0;
	int isDefinitionEnd = 0;
	unsigned int flags = MK_TYPE_VARIABLE;
	MK_VARIABLE *newVariable =
		( MK_VARIABLE * )mk_create_object( MK_TYPE_VARIABLE );
	do
	{
		if( lexer->value == MK_LEX_TYPE_RESERVED_MARK_DBLATMARK )
		{
			flags |= MK_TYPE_ATTRIBUTE_VARIABLE_STATIC;
		}
		else if( lexer->value != MK_LEX_TYPE_RESERVED_MARK_ATMARK )
		{
			if( vm->exceptionObject == NULL )
				mk_raise_internal_error( vm, lexer->name, lexer->line, MK_ERROR_TYPE_COMILE_ERROR | 11, "property", NULL );
			isError = 1;
		}
		if( isError == 1 )
			break;

		 mk_get_token( vm, lexer );
		if( ( lexer->value & MK_LEX_TYPE_MASK ) != MK_LEX_TYPE_SYMBOL )
		{
			if( vm->exceptionObject == NULL )
				mk_raise_internal_error( vm, lexer->name, lexer->line, MK_ERROR_TYPE_COMILE_ERROR | 2, lexer->text, "<symbol name>", NULL ); 
			isError = 1;
		}
		if( isError == 1 )
			break;
		newVariable->name = 
			mk_get_symbol_name_ptr( vm, lexer->text );

		mk_get_token( vm, lexer );
		switch( lexer->value )
		{
		case MK_LEX_TYPE_RESERVED_MARK_PLUS:
			flags |= MK_TYPE_ATTRIBUTE_VARIABLE_READ_PUBLIC;
			break;
		case MK_LEX_TYPE_RESERVED_MARK_MUL:
			flags |= MK_TYPE_ATTRIBUTE_VARIABLE_READ_PROTECTED;
			break;
		case MK_LEX_TYPE_RESERVED_MARK_MINUS:
			flags |= MK_TYPE_ATTRIBUTE_VARIABLE_READ_PRIVATE;
			break;
		case MK_LEX_TYPE_EOL:
		case MK_LEX_TYPE_RESERVED_MARK_EQUAL:
		case MK_LEX_TYPE_EOF:
			flags |= MK_TYPE_ATTRIBUTE_VARIABLE_READ_PUBLIC;
			flags |= MK_TYPE_ATTRIBUTE_VARIABLE_WRITE_PROTECTED;
			isDefinitionEnd = 1;
			break;
		default:
			if( vm->exceptionObject == NULL )
				mk_raise_internal_error( vm, lexer->name, lexer->line, MK_ERROR_TYPE_COMILE_ERROR | 2, lexer->text, "[!|?|.|=|<CR>]", NULL );
			isError = 1;
			break;
		}
		if( isError != 0 )
			break;
		if( isDefinitionEnd == 0 )
		{
			mk_get_token( vm, lexer );
			switch( lexer->value )
			{
			case MK_LEX_TYPE_RESERVED_MARK_PLUS:
				flags |= MK_TYPE_ATTRIBUTE_VARIABLE_WRITE_PUBLIC;
				break;
			case MK_LEX_TYPE_RESERVED_MARK_MUL:
				flags |= MK_TYPE_ATTRIBUTE_VARIABLE_WRITE_PROTECTED;
				break;
			case MK_LEX_TYPE_RESERVED_MARK_MINUS:
				flags |= MK_TYPE_ATTRIBUTE_VARIABLE_WRITE_PRIVATE;
				break;
			case MK_LEX_TYPE_EOL:
			case MK_LEX_TYPE_RESERVED_MARK_EQUAL:
			case MK_LEX_TYPE_EOF:
				flags |= MK_TYPE_ATTRIBUTE_VARIABLE_WRITE_PROTECTED;
				isDefinitionEnd = 1;
				break;
			default:
				if( vm->exceptionObject == NULL )
					mk_raise_internal_error( vm, lexer->name, lexer->line, MK_ERROR_TYPE_COMILE_ERROR | 2, lexer->text, "[!|?|.|=|<CR>]", NULL );
				isError = 1;
				break;
			}
		}
		if( isError != 0 )
			break;
		if( isDefinitionEnd == 0 )
			mk_get_token( vm, lexer );
		
		if( lexer->value == MK_LEX_TYPE_RESERVED_MARK_EQUAL )
		{
			// compile default value
			mk_get_token( vm, lexer );
			newVariable->defaultValue = do_compile_expr_root( vm, lexer, NULL );
			if( newVariable->defaultValue == NULL )
			{
				if( vm->exceptionObject == NULL )
					mk_raise_internal_error( vm, lexer->name, lexer->line, MK_ERROR_TYPE_COMILE_ERROR | 13, "=", NULL );
				isError = 1;
			}
		}
		else if( lexer->value != MK_LEX_TYPE_EOL && lexer->value != MK_LEX_TYPE_EOF )
		{
			if( vm->exceptionObject == NULL )
				mk_raise_internal_error( vm, lexer->name, lexer->line, MK_ERROR_TYPE_COMILE_ERROR | 2, lexer->text, "[=|<CR>]", NULL );
			isError = 1;
		}
		else
		{
			// lexer->value == MK_LEX_TYPE_EOL
		}
	}while( 0 );
	newVariable->flags = flags;

	if( lexer->hasError != 0 )
		isError = 1;

	if( isError != 0 )
	{
		mk_destroy_node( newVariable );
	}
	else
	{
		if( mk_register_variable( vm, newVariable, targetClass ) != MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP )
		{
			if( vm->exceptionObject == NULL )
				mk_raise_internal_error( vm, lexer->name, lexer->line, MK_ERROR_TYPE_COMILE_ERROR | 4, newVariable->name, "variable", targetClass->nameThis, NULL );
			mk_destroy_node( newVariable );
			isError = 1;
		}
	}

	if( isError != 0 && vm->exceptionObject == NULL )
		mk_raise_internal_error( vm, lexer->name, lexer->line, MK_ERROR_TYPE_COMILE_ERROR | 23, NULL );
	return !isError;
}

static
int do_compile_using_module( MK_VM_STRUCT *vm, MK_LEXER *lexer, MK_CLASS *targetClass )
{
	MK_VARIABLE *targetModule = NULL;
	void *handle =NULL;
	MK_CHAR *key = NULL;
	MK_VARIABLE *value = NULL;

	if( lexer->value != MK_LEX_TYPE_RESERVED_SYMBOL_USING )
		;

	if( targetClass->usingNames == NULL )
		targetClass->usingNames = 
			mk_create_vector( 
				MK_TYPE_VECTOR | MK_TYPE_VECTOR_ARRAY_UNMANAGED_PTR | ( MK_SIZEOF_EXTEND_VECTOR_DEFAULT << 10 ),
					MK_SIZEOF_VECTOR_DEFAULT,
					(INT_PTR)NULL );

	mk_get_token( vm, lexer );
	if( lexer->value !=MK_LEX_TYPE_SYMBOL )
		;
	
	if( targetClass->variables == NULL )
		;

	if( mk_find_item_hashtable( targetClass->variables, 
		mk_get_symbol_name_ptr( vm, lexer->text ),
		(void**)&targetModule ) == 0 )
		;

	if( !( targetModule->flags & MK_TYPE_ATTRIBUTE_VARIABLE_METHOD ) &&
		( targetModule->flags & MK_TYPE_ATTRIBUTE_VARIABLE_MODULE ) )
		;

	mk_push_vector( 
		targetClass->usingNames, 
		(INT_PTR)mk_get_symbol_name_ptr( vm, lexer->text ) );

	mk_get_token( vm, lexer );
	if( lexer->value != MK_LEX_TYPE_EOL )
		return 0;
	return 1;
}

static
int do_compile_import_module( MK_VM_STRUCT *vm, MK_LEXER *lexer, MK_CLASS *targetClass )
{
	MK_VARIABLE *newModule = NULL;
	MK_CLASS *pClass = NULL;

	if( lexer->value != MK_LEX_TYPE_RESERVED_SYMBOL_IMPORT )
		;	// error

	mk_get_token( vm, lexer );
	if( lexer->value != MK_LEX_TYPE_SYMBOL )
		;	// error

	newModule = 
		mk_create_variable( vm, lexer->text, MK_TYPE_VARIABLE | MK_TYPE_ATTRIBUTE_VARIABLE_MODULE, NULL );
	mk_register_variable( vm, newModule, targetClass );

	mk_get_token( vm, lexer );
	if( lexer->value != MK_LEX_TYPE_RESERVED_MARK_EQUAL )
		;

	mk_get_token( vm, lexer );
	if( lexer->value != MK_LEX_TYPE_SYMBOL )
		;
	pClass = 
		mk_vm_get_class_by_name( vm, mk_get_symbol_name_ptr( vm, lexer->text ) );
	if( pClass == NULL || 
		!( pClass->flags & MK_TYPE_ATTRIBUTE_CLASS_MODULE ) )
		;
	newModule->moduleVariables = pClass;
	
	mk_get_token( vm, lexer );
	if( lexer->value != MK_LEX_TYPE_EOL )
		return 0;
	return 1;
}

static
int do_compile_inner_class( MK_VM_STRUCT *vm, MK_LEXER *lexer, MK_CLASS *targetClass )
{
	int isModule = 
		( targetClass->flags & MK_TYPE_ATTRIBUTE_CLASS_MODULE );
	int isError = 0;
	do
	{
		switch( lexer->value )
		{
		case MK_LEX_TYPE_RESERVED_SYMBOL_IMPORT:
			if( isModule == 0 )
			{
				isError = !do_compile_import_module( vm, lexer, targetClass );
			}
			else
			{
				// cannot use import in module.
			}
			break;

		case MK_LEX_TYPE_RESERVED_SYMBOL_USING:
			if( isModule == 0 )
			{
				isError = !do_compile_using_module( vm, lexer, targetClass );
			}
			else
			{
				// cannot use using in module.
			}
			break;

		case MK_LEX_TYPE_RESERVED_SYMBOL_DEF:
			{
				MK_VARIABLE *newMethod =
					do_compile_method( vm, lexer, isModule );
				if( newMethod != NULL )
				{
					if( mk_register_variable( vm, newMethod, targetClass ) != MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP )
						isError = 1;
				}
				else
				{
					isError = 1;
				}
				if( isError != 0 &&
					vm->exceptionObject == NULL )
				{
					mk_raise_internal_error( vm, lexer->name, lexer->line, MK_ERROR_TYPE_COMILE_ERROR | 11, "method", NULL );
					break;
				}
				if( lexer->value != MK_LEX_TYPE_EOL )
				{
					if( vm->exceptionObject == NULL )
						mk_raise_internal_error( vm, lexer->name, lexer->line, MK_ERROR_TYPE_COMILE_ERROR | 3, "end", lexer->text, "<CR>", NULL );
					isError = 1;
					break;
				}
			}
			break;
	
		case MK_LEX_TYPE_RESERVED_SYMBOL_MODULE:
			if( isModule != 0 )
			{
				mk_get_token( vm, lexer );
				if( lexer->value != MK_LEX_TYPE_RESERVED_MARK_DOT )
				{
					isError = 1;
					mk_raise_internal_error( vm, lexer->name, lexer->line, MK_ERROR_TYPE_COMILE_ERROR | 2, lexer->text, "[.]", NULL );
					break;
				}
				mk_get_token( vm, lexer );
				if( lexer->value!= MK_LEX_TYPE_RESERVED_MARK_DBLATMARK )
				{
					isError = 1;
					mk_raise_internal_error( vm, lexer->name, lexer->line, MK_ERROR_TYPE_COMILE_ERROR | 2, lexer->text, "[@@]", NULL );
					break;
				}
				continue;
			}
			else
			{
				// cannot use module variable in cass.
			}
			break;

		case MK_LEX_TYPE_RESERVED_SYMBOL_CLASS:
			if( isModule == 0 )
			{
				mk_get_token( vm, lexer );
				if( lexer->value != MK_LEX_TYPE_RESERVED_MARK_DOT )
				{
					isError = 1;
					mk_raise_internal_error( vm, lexer->name, lexer->line, MK_ERROR_TYPE_COMILE_ERROR | 2, lexer->text, "[.]", NULL );
					break;
				}
				mk_get_token( vm, lexer );
				if( lexer->value!= MK_LEX_TYPE_RESERVED_MARK_DBLATMARK )
				{
					isError = 1;
					mk_raise_internal_error( vm, lexer->name, lexer->line, MK_ERROR_TYPE_COMILE_ERROR | 2, lexer->text, "[@@]", NULL );
					break;
				}
				continue;
			}
			else
			{
				// cannot use class variable in module.
			}
			break;

		case MK_LEX_TYPE_RESERVED_MARK_ATMARK:
		case MK_LEX_TYPE_RESERVED_MARK_DBLATMARK:
			if( do_compile_property( vm, lexer, targetClass ) == 0 )
			{
				if( vm->exceptionObject == NULL )
					mk_raise_internal_error( vm, lexer->name, lexer->line, MK_ERROR_TYPE_COMILE_ERROR | 11, "property", NULL );
				isError = 1;
				break;
			}
			if( lexer->value != MK_LEX_TYPE_EOL )
			{
				if( vm->exceptionObject == NULL )
					mk_raise_internal_error( vm, lexer->name, lexer->line, MK_ERROR_TYPE_COMILE_ERROR | 2, lexer->text, "<CR>", NULL );
				isError = 1;
				break;
			}
			break;

		case MK_LEX_TYPE_EOL:
			mk_get_token( vm, lexer );
			continue;

		case MK_LEX_TYPE_RESERVED_SYMBOL_END:
			break;

		default:
			if( vm->exceptionObject == NULL )
				mk_raise_internal_error( vm, lexer->name, lexer->line, MK_ERROR_TYPE_COMILE_ERROR | 2, lexer->text, "[end|def|class|@|@@]", NULL );
			isError = 1;
			break;
		}
		if( isError != 0 )
			break;
		if( lexer->value == MK_LEX_TYPE_RESERVED_SYMBOL_END )
		{
			mk_get_token( vm, lexer );
			break;
		}
	}
	while( lexer->value != MK_LEX_TYPE_EOF );
	if( lexer->hasError != 0 )
		isError = 1;

	return !isError;
}

static
MK_CLASS *do_compile_class_definition( MK_VM_STRUCT *vm, MK_LEXER *lexer )
{
	int isSuccess = 0;
	int isModule = 0;
	MK_CLASS *newClass = mk_create_object( MK_TYPE_CLASS );
	do
	{
		if( lexer->value == MK_LEX_TYPE_RESERVED_SYMBOL_MODULE )
		{
			newClass->flags = 
				MK_TYPE_SET_ATTRIBUTE( newClass->flags, MK_TYPE_ATTRIBUTE_CLASS_MODULE );
			isModule = 1;
		}
		else if( lexer->value == MK_LEX_TYPE_RESERVED_SYMBOL_CLASS )
		{
			newClass->flags = 
				MK_TYPE_SET_ATTRIBUTE( newClass->flags, MK_TYPE_ATTRIBUTE_CLASS_CLASS );
		}
		else
		{
			if( vm->exceptionObject == NULL )
				mk_raise_internal_error( vm, lexer->name, lexer->line, MK_ERROR_TYPE_COMILE_ERROR | 2, lexer->text, "class", NULL );
			break;	// $error	1
		}

		mk_get_token( vm, lexer );
		if( ( lexer->value & MK_LEX_TYPE_MASK ) != MK_LEX_TYPE_SYMBOL )
		{
			if( vm->exceptionObject == NULL )
				mk_raise_internal_error( vm, lexer->name, lexer->line, MK_ERROR_TYPE_COMILE_ERROR | 2, lexer->text, "<class name>", NULL );
			break;	// $error	1
		}
		newClass->nameThis = 
			mk_get_symbol_name_ptr( vm, lexer->text );

		mk_get_token( vm, lexer );
		if( lexer->value == MK_LEX_TYPE_RESERVED_MARK_CORON )
		{
			if( isModule == 0 )
			{
				mk_get_token( vm, lexer );
				if( ( lexer->value & MK_LEX_TYPE_MASK ) != MK_LEX_TYPE_SYMBOL )
				{
					if( vm->exceptionObject == NULL )
						mk_raise_internal_error( vm, lexer->name, lexer->line, MK_ERROR_TYPE_COMILE_ERROR | 3, ":", lexer->text, "<super class name>", NULL );
					break;
				}
				newClass->nameSuper = 
					mk_get_symbol_name_ptr( vm, lexer->text );
				mk_get_token( vm, lexer );
			}
			else
			{
				// error module can not have super.
			}
		}
		if( lexer->value != MK_LEX_TYPE_EOL )
		{
			if( vm->exceptionObject == NULL )
				mk_raise_internal_error( vm, lexer->name, lexer->line, MK_ERROR_TYPE_COMILE_ERROR | 2, lexer->text, "[:|<CR>]", NULL );
			break;
		}
		else if( isModule == 0 && newClass->nameSuper == NULL )
		{
			// super class not defined.
			newClass->nameSuper = 
				mk_get_symbol_name_ptr( vm, CLASS_INTERNAL_OBJECT );
		}
		isSuccess = 1;
	}while( 0 );

	mk_get_token( vm,lexer);
	if( isSuccess != 0 )
		isSuccess = 
			do_compile_inner_class( vm, lexer, newClass );

	if( isSuccess != 0 )
	{
		if( lexer->value != MK_LEX_TYPE_EOL &&
			lexer->value != MK_LEX_TYPE_EOF )
		{
			if( vm->exceptionObject == NULL )
				mk_raise_internal_error( vm, lexer->name, lexer->line, MK_ERROR_TYPE_COMILE_ERROR | 3, "end", lexer->text, "<CR>", NULL );
			isSuccess = 0;
		}
		if( lexer->hasError != 0 )
			isSuccess = 0;
	}

	if( isSuccess == 0 )
	{
		mk_destroy_node( newClass );
		newClass = NULL;
		if( vm->exceptionObject == NULL )
			mk_raise_internal_error( vm, lexer->name, lexer->line, MK_ERROR_TYPE_COMILE_ERROR | 11, "class", NULL );
	}
	return newClass;
}

static
int do_compile_root( MK_VM_STRUCT *vm, MK_LEXER *lexer )
{
	int isError = 0;
	MK_CLASS *rootClass = NULL;
	MK_VARIABLE *entryPoint = NULL;
	MK_NODE_BLOCK *block = NULL;
	MK_VM_FRAME *newFrame = NULL;

	mk_find_item_hashtable( vm->code->classes, 
		mk_get_symbol_name_ptr( vm, mk_get_symbol_name_ptr( vm, CLASS_INTERNAL_KERNEL ) ), 
		(void**)&rootClass );
	mk_find_item_hashtable( rootClass->variables, 
		mk_get_symbol_name_ptr( vm, mk_get_symbol_name_ptr( vm, FUNCTIONNAME_ENTRYPOINT ) ), 
		(void**)&entryPoint );
	block = 
		create_mk_node_block( MK_TYPE_ATTRIBUTE_BLOCK_FUNCTION_ROOT );
	entryPoint->entryPoint = block;

	// read 2 tokens.
	mk_get_token( vm, lexer );
	mk_get_token( vm, lexer );

	do
	{
		switch( lexer->value )
		{
		case MK_LEX_TYPE_RESERVED_SYMBOL_MODULE:
			isError = 
				( mk_register_class( vm, do_compile_class_definition( vm, lexer ) ) != MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP );
			break;

		case MK_LEX_TYPE_RESERVED_SYMBOL_CLASS:
			if( lexer->nextValue == MK_LEX_TYPE_RESERVED_MARK_PARENTHESIS )
			{
				lexer->value = MK_LEX_TYPE_SYMBOL;	// class() method.
				isError = 
					!do_compile_segment( vm, lexer, block, MK_TYPE_SEGMENT_ROOT );
			}
			else if( lexer->nextValue == MK_LEX_TYPE_RESERVED_MARK_DOT )
			{
				mk_get_token( vm, lexer );
				mk_get_token( vm, lexer );

				if( lexer->value!= MK_LEX_TYPE_RESERVED_MARK_DBLATMARK )
				{
					isError = 1;
					mk_raise_internal_error( vm, lexer->name, lexer->line, MK_ERROR_TYPE_COMILE_ERROR | 2, lexer->text, "[@@]", NULL );
					break;
				}
				isError = !do_compile_property( vm, lexer, rootClass );
			}
			else
				isError = 
					mk_register_class( vm, do_compile_class_definition( vm, lexer ) ) != MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP;
			break;
		case MK_LEX_TYPE_RESERVED_SYMBOL_DEF:
			{
				MK_VARIABLE *newMethod =
					do_compile_method( vm, lexer, 0 );
				if( newMethod != NULL )
				{
					newMethod->flags |= MK_TYPE_ATTRIBUTE_VARIABLE_STATIC;
					if( mk_register_variable( vm, newMethod, rootClass ) != MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP )
						isError = 1;
				}
				else
				{
					isError = 1;
				}
				if( isError != 0 &&
					vm->exceptionObject == NULL )
				{
					mk_raise_internal_error( vm, lexer->name, lexer->line, MK_ERROR_TYPE_COMILE_ERROR | 11, "method", NULL );
					break;
				}
				if( lexer->value != MK_LEX_TYPE_EOL &&
					lexer->value != MK_LEX_TYPE_EOF )
				{
					if( vm->exceptionObject == NULL )
						mk_raise_internal_error( vm, lexer->name, lexer->line, MK_ERROR_TYPE_COMILE_ERROR | 3, "end", lexer->text, "<CR>", NULL );
					isError = 1;
					break;
				}
			}
			break;
		case MK_LEX_TYPE_EOL:
			{
				mk_get_token( vm, lexer );
			}
			continue;
		default:
			isError = 
				do_compile_segment( vm, lexer, block, MK_TYPE_SEGMENT_ROOT ) == 0;
			break;
		}
		if( isError != 0 )
			break;
	}
	while( ( lexer->value & MK_LEX_TYPE_MASK ) != MK_LEX_TYPE_EOF );

	if(isError != 0 && vm->exceptionObject == NULL)
		mk_raise_internal_error( vm, lexer->name, lexer->line, MK_ERROR_TYPE_COMILE_ERROR | 23, NULL );

	return !isError && !lexer->hasError;
}

int mk_do_compile( MK_VM_STRUCT *vm, MK_LEXER *lexer )
{
	int result = 0;

	MK_VM_FRAME_ITEM *exceptionObjectOld = NULL;

	if( vm == NULL || lexer == NULL || lexer->stream.stream == NULL )
		return MK_VM_EXECUTE_EXPR_RETURN_FAILED;
	
	result = do_compile_root( vm, lexer );
	if(result != 0)
		result = mk_link_register_class_static_instance(vm) == MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP ? 1 : 0; 

	return result != 0 ? MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP : MK_VM_EXECUTE_EXPR_RETURN_FAILED;
}
