#include "mk.h"
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <ctype.h>
#include <string.h>

#ifdef __GNUC__
#define isleadbyte(c)	0

#define iscsym(c)	(isalnum(c)|(c=='_'))
#define iscsymf(c)	(isalpha(c)|(c=='_'))
#endif

int mk_lex_getc( MK_LEXER *lexer )
{
	int result = 
		(*lexer->stream.getc)( 
			lexer->stream.stream );
	if( lexer->sizePlainText == lexer->positionNextPlainText )
	{
		MK_CHAR *newBuffer = 
			malloc( sizeof( MK_CHAR ) * lexer->sizePlainText + MK_LEX_SIZEOF_EXTEND_PLAINTEXT_BUFFER );
		memcpy( newBuffer, lexer->plainText, lexer->sizePlainText );
		free( lexer->plainText );
		lexer->plainText = newBuffer;
		lexer->sizePlainText += MK_LEX_SIZEOF_EXTEND_PLAINTEXT_BUFFER;
	}
	lexer->plainText[lexer->positionNextPlainText] = result;
	lexer->positionNextPlainText ++;

	return result;
}

static
void shift_lexer_position( MK_LEXER *lexer )
{
	MK_CHAR *tempText = lexer->text;

	lexer->line = lexer->nextLine;

	lexer->text = lexer->nextText;
	lexer->nextText = tempText;
	lexer->length = lexer->nextLength;
	lexer->nextLength = 0;

	lexer->value = lexer->nextValue;	
	lexer->nextValue = 0;

	lexer->object = lexer->nextObject;
	lexer->nextObject = NULL;

	lexer->positionCurrentToken = lexer->positionNextToken;
	lexer->positionNextToken = lexer->positionNextPlainText;
	if( lexer->stream.back != 0 )
		lexer->positionNextToken --;
}

static
void extend_token_buffer( MK_LEXER *lexer )
{
	MK_CHAR *newBuffer = NULL;
	
	newBuffer = malloc( lexer->maxBufferLength + lexer->sizeExtendBufferLength );
	if( lexer->text != NULL )
		memcpy( newBuffer, lexer->text, lexer->maxBufferLength );
	free( lexer->text );
	lexer->text = newBuffer;

	newBuffer = malloc( lexer->maxBufferLength + lexer->sizeExtendBufferLength );
	if( lexer->nextText != NULL )
		memcpy( newBuffer, lexer->nextText, lexer->maxBufferLength );
	free( lexer->nextText );
	lexer->nextText = newBuffer;

	lexer->maxBufferLength += lexer->sizeExtendBufferLength;
}

static 
int get_token_eol( MK_LEXER *lexer )
{
	MK_CHAR previous = 
		( MK_CHAR )( lexer->stream.current );
	do
	{
		lexer->stream.current = 
			mk_lex_getc( lexer );
		lexer->nextText[lexer->nextLength] = lexer->stream.current;
		lexer->nextLength ++;
		if( lexer->nextLength == lexer->maxBufferLength )
			extend_token_buffer( lexer );
		if( previous == '\n' )
			lexer->nextLine ++;
		else if( previous == '\r' &&
			lexer->stream.current == '\r' )
			lexer->nextLine ++;
	}while( lexer->stream.current == '\r' ||
		lexer->stream.current == '\n' );
	lexer->nextLength --;
	lexer->nextText[lexer->nextLength] = '\0';
	lexer->nextValue = MK_LEX_TYPE_EOL;
	lexer->stream.back = 1;
	return 1;
}


static
int skip_single_line_comment( MK_LEXER *lexer )
{
	do
	{
		lexer->stream.current = 
			mk_lex_getc( lexer );
		if( lexer->stream.current == '\r' ||
			lexer->stream.current == '\n' )
			return get_token_eol( lexer );
	}while(lexer->stream.current != (unsigned char)EOF );

	lexer->nextLength = 0;
	lexer->nextText[0] = EOF;
	lexer->nextValue = MK_LEX_TYPE_EOF;
	return 1;
}

static 
int get_token_number( MK_LEXER *lexer, int isReal )
{
	do
	{
		lexer->stream.current = 
			mk_lex_getc( lexer );
		lexer->nextText[lexer->nextLength] = lexer->stream.current;
		lexer->nextLength ++;
		if( lexer->nextLength == lexer->maxBufferLength )
			extend_token_buffer( lexer );
	}while( isdigit( lexer->stream.current ) );
	if( lexer->stream.current == '.' )
	{
		if( isReal != 0 )
		{
			return get_token_number( lexer, 0 );
		}
		else
		{
			lexer->stream.back = 1;
			lexer->nextLength --;
			lexer->nextText[lexer->nextLength] = '\0';
			lexer->nextValue = MK_LEX_TYPE_FLOAT_VALUE;
			return 1;
		}
	}
	else
	{
		lexer->stream.back = 1;
		lexer->nextLength --;
		lexer->nextText[lexer->nextLength] = '\0';
		if( isReal != 0 )
		{
			lexer->nextValue = MK_LEX_TYPE_INT_VALUE;
			return 1;
		}
		else
		{
			lexer->nextValue = MK_LEX_TYPE_FLOAT_VALUE;
			return 1;
		}
	}
}

static 
int get_token_symbol( MK_LEXER *lexer )
{
	int index = 0;
	int ismbb = 0;

	if( isleadbyte( (unsigned char)lexer->nextText[0] ) != 0 )
		ismbb = 1;
	do
	{
		lexer->stream.current = 
			mk_lex_getc( lexer );
		lexer->nextText[lexer->nextLength] = lexer->stream.current;
		lexer->nextLength ++;
		if( lexer->nextLength == lexer->maxBufferLength )
			extend_token_buffer( lexer );

		if( ismbb == 0 )
		{
			ismbb = isleadbyte( (unsigned char)lexer->stream.current );
			if( ismbb == 0 && iscsym( lexer->stream.current ) == 0 )
				break;
		}
		else
		{
			ismbb = 0;
		}
	}while( 1 );

	lexer->nextLength --;
	lexer->nextText[lexer->nextLength] = '\0';
	lexer->nextValue = MK_LEX_TYPE_SYMBOL;
	lexer->stream.back = 1;

	while( MK_RESERVED_SYMBOL[index].name[0] != '\0' )
	{
		if( strcmp( lexer->nextText, MK_RESERVED_SYMBOL[index].name ) == 0 )
		{
			lexer->nextValue = MK_RESERVED_SYMBOL[index].id;
			break;
		}
		index ++;
	}
	if( MK_RESERVED_SYMBOL[index].name[0] == '\0' )
		lexer->nextValue = MK_LEX_TYPE_SYMBOL;

	return 1;
}

static 
int get_token_mark( MK_LEXER *lexer )
{
	int index = 0, isFound = 0;
	do
	{
		isFound = 0;
		index = 0;
		while( MK_RESERVED_MARK[index].name[0] != '\0' )
		{
			if( strncmp( lexer->nextText, MK_RESERVED_MARK[index].name, lexer->nextLength ) == 0 )
			{
				lexer->nextValue = MK_RESERVED_MARK[index].id;
				isFound = 1;
			}
			index ++;
		}
		if( isFound == 0 )
			break;
		lexer->stream.current = 
			mk_lex_getc( lexer );
		lexer->nextText[lexer->nextLength] = lexer->stream.current;
		lexer->nextLength ++;
	}while( isgraph( lexer->stream.current ) &&
		!isalnum( lexer->stream.current ) );

	if( lexer->nextLength > 1 )
	{
		lexer->nextLength --;
		lexer->nextText[lexer->nextLength] = '\0';
		lexer->stream.back = 1;
		return 1;
	}
	else
	{
		lexer->nextValue = MK_LEX_TYPE_INVALID;
		return 0;
	}
}

static
int skip_space( MK_LEXER *lexer )
{
	if( lexer->stream.back == 0 )
	{
		lexer->stream.current =
			mk_lex_getc( lexer );
		lexer->stream.back = 1;
	}

	while( isspace( lexer->stream.current ) &&
		lexer->stream.current != '\r' &&
		lexer->stream.current != '\n' )
	{
		lexer->stream.current =
			mk_lex_getc( lexer );
		lexer->hasNextSpace = 1;
	}
	return 1;
}

static 
int get_token_in_buffer( MK_LEXER *lexer, MK_CHAR terminate )
{
	int ismbc = 0;
	int isError = 0;
	while( 1 )
	{
		if( lexer->stream.current == '\r' ||
			lexer->stream.current == '\n' ||
			lexer->stream.current == '\0' ||
			lexer->stream.current == (unsigned char)EOF )
		{	
			isError = 1;
			break;
		}
		if( lexer->stream.current == terminate )
			break;
		if( lexer->nextLength == lexer->maxBufferLength )
			extend_token_buffer( lexer );
		if( ismbc == 0 && 
			isleadbyte(lexer->stream.current) != 0 )
		{
			lexer->nextText[lexer->nextLength] = 
				lexer->stream.current;
			lexer->nextLength ++;
			ismbc = 1;
		}
		else
		{
			ismbc = 0;
			if( lexer->stream.current == '\\' )
			{
				lexer->stream.current =
			mk_lex_getc( lexer );
				switch( lexer->stream.current )
				{
				case '\\':
					lexer->nextText[lexer->nextLength] = 
						lexer->stream.current;
					break;

				case 'r':
					lexer->nextText[lexer->nextLength] = '\r';
					break;

				case 'n':
					lexer->nextText[lexer->nextLength] = '\n';
					break;

				case 't':
					lexer->nextText[lexer->nextLength] = '\t';
					break;

				default:
					isError = 1;
					break;
				}
			}
			else
			{
				lexer->nextText[lexer->nextLength] = 
					lexer->stream.current;
			}
			lexer->nextLength ++;
		}
		if( isError != 0 )
			break;

		lexer->stream.current =
			mk_lex_getc( lexer );
	}
	if( lexer->stream.current == terminate )
		lexer->stream.current =
			mk_lex_getc( lexer );
	return !isError;
}

static
int get_token_in_character( MK_LEXER *lexer )
{
	int result = 0;

	lexer->nextValue = 
		MK_LEX_TYPE_CHARCTER;
	lexer->nextLength = 0;
	result = 
		get_token_in_buffer( lexer, MK_LEX_CHARACTER_MARK );
	if( result == 0 )
	{
//		mk_raise_internal_error( vm, lexer->name, lexer->line, MK_ERROR_TYPE_COMILE_ERROR | 8, "<char>", "'", NULL );
		lexer->nextValue = 
			MK_LEX_TYPE_INVALID;
		lexer->hasError |= 1;
		return 0;
	}
	else
	{
		return 1;
	}
}

static
void show_unknown_character( MK_LEXER *lexer, char ch )
{
	char buffer[16];
	sprintf( buffer, "0x%02x", (unsigned char)ch );
//	mk_raise_internal_error( vm, lexer->name, lexer->line, MK_ERROR_TYPE_COMILE_ERROR | 21, buffer, NULL );
	lexer->nextValue = MK_LEX_TYPE_EOF;
	lexer->hasError |= 1;
}

int mk_get_token( MK_VM_STRUCT *vm, MK_LEXER *lexer )
{
	int result = 0;
	MK_CHAR **target = NULL;
	MK_CHAR *temp = NULL;

	if( lexer->maxBufferLength == 0 )
		extend_token_buffer( lexer );

	shift_lexer_position( lexer );
	
	switch( lexer->stateLex )
	{
	case MK_LEX_STATE_CODEBLOCK:
		lexer->hasNextSpace = 0;
		skip_space( lexer );

		if( lexer->stream.back != 0 )
		{
			lexer->stream.back = 0;
		}
		else
		{
			lexer->stream.current = 
				mk_lex_getc( lexer );
		}
		lexer->nextText[0] = 
			lexer->stream.current;
		lexer->nextLength ++;

		if( ( lexer->stream.current == '\r' ||				// <CR>
			lexer->stream.current == '\n' ) )
		{
			result = get_token_eol( lexer );
		}
		else if( isdigit( lexer->stream.current ) != 0 )	// number
		{
			result = get_token_number( lexer, 1 );
		}
		else if( iscsymf( lexer->stream.current ) != 0 ||	// symbol(accept multibyte character)
			isleadbyte( (unsigned char)lexer->stream.current ) != 0 )
		{
			result = get_token_symbol( lexer );
		}
		else if( lexer->stream.current == '#' )				// comment
		{
			lexer->nextText[0] = '\0';
			result = skip_single_line_comment( lexer );
		}
		else if( isprint( lexer->stream.current ) )			// operator
		{
			result = get_token_mark( lexer );
		}
		else if( (char)lexer->stream.current == EOF )		// EndOfFile
		{
			lexer->nextValue = MK_LEX_TYPE_EOF;
			result = 1;
		}
		else												// unknown character
		{
			show_unknown_character( lexer, lexer->nextText[0] );
		}

		if( lexer->nextValue ==								// <string>
			MK_RESERVED_MARK[ MK_LEX_RESERVED_MARK_INDEX( MK_LEX_TYPE_RESERVED_MARK_DOUBLE_QUATE ) ].id )
		{
			if( lexer->stateLex == MK_LEX_STATE_CODEBLOCK )
				lexer->stateLex = MK_LEX_STATE_IN_STRING;
		}
		else if( lexer->nextValue ==						// <character>
			MK_RESERVED_MARK[ MK_LEX_RESERVED_MARK_INDEX( MK_LEX_TYPE_RESERVED_MARK_SINGLE_QUATE ) ].id )
		{
			result = get_token_in_character( lexer );
		}
		break;

	case MK_LEX_STATE_IN_STRING:
	default:
		break;
	}
	return result;
}


MK_LEXER * mk_create_lexer( MK_VM_STRUCT *vm)
{
	MK_LEXER *lexer = 
		malloc( sizeof(MK_LEXER) );
	memset( lexer, 0x00, sizeof(MK_LEXER) );

	// initialize lexer default parameter
	lexer->defaultBufferLength = SIZEOF_TOKENBUFFER;
	lexer->maxBufferLength = 0;
	lexer->sizeExtendBufferLength = SIZEOF_TOKENBUFFER;
	lexer->stateLex = MK_LEX_STATE_CODEBLOCK;

	// plaintext buffer
	lexer->sizePlainText = MK_LEX_SIZEOF_PLAINTEXT_BUFFER;
	lexer->positionNextPlainText = 0;
	lexer->plainText = malloc( sizeof( MK_CHAR ) * MK_LEX_SIZEOF_PLAINTEXT_BUFFER );
	lexer->positionCurrentToken = 0;
	lexer->positionNextToken = 0;
	return lexer;
}

void mk_prepare_lexer_for_file( MK_LEXER *lexer )
{
	// initialize stream for file.
	lexer->stream.open = mk_open;
	lexer->stream.getc = mk_getc;
	lexer->stream.close = mk_close;
	lexer->stream.trace = mk_trace;
}

int mk_open_stream( MK_VM_STRUCT *vm, MK_LEXER *lexer, MK_CHAR *nameOfStream, MK_CHAR *target )
{
	lexer->name = mk_allocate_memory_pool( 
		&vm->memoryPool, 
		nameOfStream, 
		strlen( (MK_CHAR*)nameOfStream) + 1 );
	lexer->stream.stream = ( lexer->stream.open )(lexer->name);
	if ( lexer->stream.stream == NULL )
	{
		if( vm->exceptionObject == NULL )
			return mk_raise_internal_error( vm, lexer->name, lexer->line, MK_ERROR_TYPE_COMILE_ERROR | 22, target, NULL );
		else
			return MK_VM_EXECUTE_EXPR_RETURN_FAILED;
	}
	else
	{
		return MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP;
	}
}

void mk_close_stream( MK_LEXER *lexer )
{
	( lexer->stream.close )( lexer->stream.stream );
	lexer->name = NULL;
}

void mk_clear_lexer( MK_LEXER *lexer )
{
	free( lexer->text );
	free( lexer->nextText );
	free( lexer->plainText );
	mk_destroy_node( lexer->object );
	mk_destroy_node( lexer->nextObject );
	free( lexer );
}
