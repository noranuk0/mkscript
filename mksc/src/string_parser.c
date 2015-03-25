#include "mk.h"
#include <stdlib.h>
#include <string.h>
#include <memory.h>

#define MK_SIZEOF_COMPILE_STRING_BUFFER	16

typedef struct tagCompileStringBuffer
{
	MK_CHAR buffer[MK_SIZEOF_COMPILE_STRING_BUFFER];
	int indexCurrent;
	struct tagCompileStringBuffer *previous;
} CompileStringBuffer;

static
int get_next_char_in_string( MK_LEXER *lexer )
{
	int result = 0;
	if( lexer->positionNextToken < lexer->positionNextPlainText )
	{
		result = lexer->plainText[lexer->positionNextToken++];
		if( lexer->positionNextToken == lexer->positionNextPlainText )
			lexer->stream.back = 0;
	}
	else
	{
		if( lexer->stream.back != 0 )
		{
			lexer->stream.back = 0;
			result = lexer->stream.current;
		}
		else
		{
			result = mk_lex_getc( lexer );
		}
		lexer->positionNextToken++;
	}
	return result;
}

static
MK_CHAR *commit_string( CompileStringBuffer *target )
{
	int size = 0;
	CompileStringBuffer *current = target;
	MK_CHAR *result = NULL, *position = NULL;

	while( current != NULL )
	{
		size += current->indexCurrent;
		current = current->previous;
	}
	result = malloc( sizeof( MK_CHAR ) * ( size + 1 ) );
	result[size] = '\0';
	position = result + size;

	current = target;
	while( current != NULL )
	{
		position -= current->indexCurrent;
		memcpy( position, current->buffer, current->indexCurrent );
		current = current->previous;
	}
	return result;
}

static
CompileStringBuffer *push_string( CompileStringBuffer *target, MK_CHAR newCharacter )
{
	if( ( target != NULL ) && 
		( target->indexCurrent < MK_SIZEOF_COMPILE_STRING_BUFFER ) )
	{
		target->buffer[target->indexCurrent] = newCharacter;
	}
	else
	{
		CompileStringBuffer *next = 
			malloc( sizeof( CompileStringBuffer ) );
		memset( next, 0x00, sizeof( CompileStringBuffer ) );
		next->previous = target;
		target = next;
		target->buffer[target->indexCurrent] = newCharacter;
	}
	target->indexCurrent ++;
	return target;
}

static
void delete_compile_string_buffer( CompileStringBuffer *target )
{
	CompileStringBuffer *previous = NULL;
	
	while( target != NULL )
	{
		previous = target->previous;
		free( target );
		target = previous;
	}
}

MK_NODE_EXPR *do_compile_parse_string( MK_VM_STRUCT *vm, MK_LEXER *lexer )
{
	MK_VECTOR *vString = NULL;
	MK_NODE_EXPR *root = NULL;
	MK_CHAR *buffer = NULL;
	int lengthBuffer = 0;
	int isSuccess = 1;
	CompileStringBuffer *stringBuffer = NULL;
	MK_NODE_EXPR *expr = NULL;

	if( lexer->value != MK_LEX_TYPE_RESERVED_MARK_DOUBLE_QUATE )
		return 0;

	lexer->positionNextToken = lexer->positionNextPlainText - 1;
	while( lexer->nextValue != MK_LEX_TYPE_EOF )
	{
		int next = 
			get_next_char_in_string( lexer );

		switch( next)
		{
		case EOF:
		case '\r':
		case '\n':
		case '\0':
			isSuccess = 0;
			break;

		case '%':
			next = 
				get_next_char_in_string( lexer );
			switch( next )
			{
			case '(':
				{
					isSuccess = 0;
					if( root == NULL )
					{
						root = 
							mk_create_object( MK_TYPE_NODE_EXPR );
						root->flags = 
							MK_TYPE_NODE_EXPR | MK_TYPE_NODE_EXPR_STRING_FORMAT_ROOT;
						root->u1.stringParts = 
							mk_create_vector( 
								MK_TYPE_VECTOR | MK_TYPE_VECTOR_ARRAY_MANAGED_PTR | ( MK_SIZEOF_EXTEND_VECTOR_DEFAULT << 10 ),
								MK_SIZEOF_VECTOR_DEFAULT,
								(INT_PTR)NULL );
					}
					if( stringBuffer != NULL )
					{
						expr = mk_create_object( MK_TYPE_NODE_EXPR );
						expr->flags = 
							MK_TYPE_NODE_EXPR | MK_TYPE_NODE_EXPR_STRING;
						expr->u2.value = 
							commit_string( stringBuffer );
						expr->u1.length = 
							strlen( expr->u2.value );
						mk_push_vector( root->u1.stringParts, (INT_PTR)expr );
						delete_compile_string_buffer( stringBuffer );
						stringBuffer = NULL;
					}
					expr = mk_create_object( MK_TYPE_NODE_EXPR );
					lexer->stateLex = MK_LEX_STATE_CODEBLOCK;
					mk_get_token( vm, lexer );
					mk_get_token( vm, lexer );
					expr->u1.left = 
						do_compile_expr( vm, lexer, MK_TYPE_EXPR_METHOD_ARG );

					if( expr->u1.left == NULL )
						break;
					if( lexer->value == MK_LEX_TYPE_RESERVED_MARK_END_PARENTHESIS )
					{
						expr->flags = 
							MK_TYPE_NODE_EXPR | MK_TYPE_NODE_EXPR_STRING_FORMAT_DEFUALT;
						isSuccess = 1;
					}
					else if( lexer->value == MK_LEX_TYPE_RESERVED_MARK_CONMA )
					{
						mk_get_token( vm, lexer );
						if( lexer->value == MK_LEX_TYPE_SYMBOL )
						{
							if( strcmp( lexer->text, "left" ) == 0 )
								expr->flags = 
									MK_TYPE_NODE_EXPR | MK_TYPE_NODE_EXPR_STRING_FORMAT_LEFT;
							else if( strcmp( lexer->text, "right" ) == 0 )
								expr->flags = 
									MK_TYPE_NODE_EXPR | MK_TYPE_NODE_EXPR_STRING_FORMAT_RIGHT;
							else if( strcmp( lexer->text, "time" ) == 0 )
								expr->flags = 
									MK_TYPE_NODE_EXPR | MK_TYPE_NODE_EXPR_STRING_FORMAT_TIME;
							else
								break;	// error.

							mk_get_token( vm, lexer );
							if( lexer->value != MK_LEX_TYPE_RESERVED_MARK_CONMA )
								break;
							mk_get_token( vm, lexer );
							lexer->stateLex = MK_LEX_STATE_CODEBLOCK;
							expr->u1.left = 
								do_compile_expr( vm, lexer, MK_TYPE_EXPR_METHOD_ARG );
							lexer->stateLex = MK_LEX_STATE_IN_STRING;
							if( expr->u2.right == NULL )
								break;
							else if( lexer->value != MK_LEX_TYPE_RESERVED_MARK_END_PARENTHESIS )
								break;
						}
						else
						{
							break;
						}
					}
					lexer->stateLex = MK_LEX_STATE_IN_STRING;
					mk_push_vector( root->u1.stringParts, (INT_PTR)expr );
					isSuccess = 1;
				}
				if( ( isSuccess == 0 ) && ( expr != NULL ) )
				{
					mk_destroy_node( expr );
					expr = NULL;
				}
				break;
			case '%':
				stringBuffer = 
					push_string( stringBuffer, '%' );
				break;
			default:
				break;	// error.
			}
			break;
		case '\\':
			switch( get_next_char_in_string( lexer ) )
			{
			case '\"':
				stringBuffer = 
					push_string( stringBuffer, '\"' );
				break;
			case '\\':
				stringBuffer = 
					push_string( stringBuffer, '\\' );
				break;
			case 'r':
				stringBuffer = 
					push_string( stringBuffer, '\r' );
				break;
			case 'n':
				stringBuffer = 
					push_string( stringBuffer, '\n' );
				break;
			case 't':
				stringBuffer = 
					push_string( stringBuffer, '\t' );
				break;
			default:
				stringBuffer = 
					push_string( stringBuffer, '%' );
				break;
			}
			break;
		case '"':
			break;

		default:
			stringBuffer = 
				push_string( stringBuffer, next );
			break;
		}
		if( isSuccess == 0 )
		{
			break;
		}
		else if( next == '"' )
		{
			lexer->stateLex = MK_LEX_STATE_CODEBLOCK;
			mk_get_token( vm, lexer );
			break;
		}
	}
	if( isSuccess == 0 )
	{
		if( root != NULL )
			mk_destroy_node( root );
		if( expr != NULL )
			mk_destroy_node( expr );
		if( stringBuffer != NULL )
			delete_compile_string_buffer( stringBuffer );
		root = NULL;
		expr = NULL;
		stringBuffer = NULL;
	}
	if( stringBuffer != NULL )
	{
		expr = mk_create_object( MK_TYPE_NODE_EXPR );
		expr->flags = 
			MK_TYPE_NODE_EXPR | MK_TYPE_NODE_EXPR_STRING;
		expr->u2.value = 
			commit_string( stringBuffer );
		expr->u1.length =
			strlen( expr->u2.value );
		if( root != NULL )
			mk_push_vector( root->u1.stringParts, (INT_PTR)expr );
		else
			root = expr;
		delete_compile_string_buffer( stringBuffer );
	}
	return root;
}
