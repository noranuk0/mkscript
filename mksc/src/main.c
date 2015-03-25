#ifdef _MSC_VER
#if defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#endif
#endif

#include "mk.h"
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <ctype.h>
#include <string.h>
#ifdef _MSC_VER
#include <locale.h>
#endif

void show_profile_data( MK_VM_STRUCT *vm )
{
	// show profile.
	int count = 0;
	MK_MANAGED_VM_FRAME_ITEM_TABLE *pFrameItemTable = vm->pFrameItemTable;
	MK_MANAGED_VM_FRAME_TABLE *pFrameTable = vm->pFrameTable;
	MK_MANAGED_HASH_TABLE *pHashTable = vm->pHashTable;

	fprintf( stdout, "symbolneme count:%d\n", mk_size_item_hashtable( vm->hashSymbolName) );
	fprintf( stdout, "sizeof memorypool:%d\n", vm->memoryPool->next - vm->memoryPool->memory );
	count = 0;
	do
	{
		count ++;
		pFrameItemTable = pFrameItemTable->previous;
	}
	while( pFrameItemTable );
	fprintf( stdout, "frameitemtable count:%d\n", count );
	count = 0;
	do
	{
		count ++;
		pFrameTable = pFrameTable->previous;
	}
	while( pFrameTable );
	fprintf( stdout, "frametable count:%d\n", count );
	count = 0;
	do
	{
		count ++;
		pHashTable = pHashTable->previous;
	}
	while( pHashTable );
	fprintf( stdout, "hashtable count:%d\n", count );
}

int main( int argc, char **argv )
{
	MK_LEXER *lexer = NULL;
	MK_VM_STRUCT *vm = NULL;
	MK_CHAR *fileName = NULL;
	int retCode = 0;

#ifdef _MSC_VER
#if defined(_DEBUG)
	_CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif
	setlocale( LC_ALL, ".OCP" );
#endif
	if( argc < 2 )
	{
		fprintf( stdout, "%s  Build - %s %s\n", argv[0], __DATE__, __TIME__ );
		return 0;
	}

	fileName = argv[1];
	
	vm = 
		mk_create_object( MK_OBJECT_TYPE_VM_STRUCT );
	mk_vm_initialize( vm );

	lexer = mk_create_lexer(vm);
	mk_prepare_lexer_for_file(lexer);

	retCode = mk_open_stream(vm, lexer, fileName, fileName);
	if (retCode == MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP) 
	{
		retCode = mk_do_compile( vm, lexer );
		mk_close_stream( lexer );
	}
	mk_clear_lexer( lexer );

	if (retCode == MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP)
	{
		fprintf(stdout, "compile:successful\n");
	}
	else
	{
		fprintf(stdout, "compile:failed\n");
	}

	if( retCode == MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP )
	{
		retCode = -1;
		mk_vm_run( vm , &retCode );
		fprintf( stdout, "mk_main() return %d\n", retCode);
		show_profile_data( vm );
	}

	if( retCode == MK_VM_EXECUTE_EXPR_THROW || 
		vm->exceptionObject != NULL )
	{
		MK_VM_FRAME_ITEM *pErrorDescription = NULL;
		fprintf( stderr, "retCode:%d\n", retCode );
		fprintf( stderr, "unhandle error.\n" );
		if( vm->exceptionObject != NULL )
		{
			pErrorDescription  = 
				mk_vm_find_variable( vm, vm->exceptionObject, mk_get_symbol_name_ptr( vm, "description" ) );
			if( pErrorDescription != NULL )
				fprintf( stderr, "%s\n", pErrorDescription->stringTypeValue );
		}
		else
		{
			fprintf( stderr, "exception object not set.\n" );
		}
	}

	mk_destroy_node( vm );

	return retCode;
}
