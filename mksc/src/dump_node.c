#include "mk.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_space( unsigned int num );

static
void print_space( unsigned int num )
{
	unsigned int index = 0;
	for( index = 0; index < num; index ++ )
		putc( ' ', stdout );
}

static
void dump_vector_node( MK_VECTOR *target, unsigned int rank )
{
	int index = 0;
	int size = 0;
	
	print_space( rank );
	fprintf( stdout, "MK_VECTOR\n" );
	if( target == NULL )return;

	print_space( rank );
	switch( MK_TYPE_ATTRIBUTE_VECTOR_STYLE(target->flags) )
	{
	case MK_TYPE_VECTOR_ARRAY_MANAGED_PTR:
		fprintf( stdout, "(MANAGED_PTR)\n" );
		break;
	case MK_TYPE_VECTOR_ARRAY_UNMANAGED_PTR:
		fprintf( stdout, "(UNMANAGED_PTR)\n" );
		break;
	case MK_TYPE_VECTOR_ARRAY_INT8:
		fprintf( stdout, "(INT8)\n" );
		break;
	case MK_TYPE_VECTOR_ARRAY_INT16:
		fprintf( stdout, "(INT16)\n" );
		break;
	case MK_TYPE_VECTOR_ARRAY_INT32:
		fprintf( stdout, "(INT32)\n" );
		break;
	case MK_TYPE_VECTOR_ARRAY_INT64:
		fprintf( stdout, "(INT64)\n" );
		break;
	}

	size = 	target->used;
	for( index = 0; index < size; index ++ )
	{
		print_space( rank );
		switch( MK_TYPE_ATTRIBUTE_VECTOR_STYLE(target->flags) )
		{
		case MK_TYPE_VECTOR_ARRAY_MANAGED_PTR:
		case MK_TYPE_VECTOR_ARRAY_UNMANAGED_PTR:
			{
				fprintf( stdout, "ITEM(%d)\n", index );
				if( target->items[index] != (INT_PTR)NULL )
				{
					dump_node( (void*)target->items[index], rank + 1 );
				}
				else
				{
					print_space( rank + 1 );
					fprintf( stdout, "( NULL )\n", index );
				}
			}
			break;
		case MK_TYPE_VECTOR_ARRAY_INT8:
			fprintf( stdout, "%02x ", target->items8[index] );
			break;
		case MK_TYPE_VECTOR_ARRAY_INT16:
			fprintf( stdout, "%04x ", target->items16[index] );
			break;
		case MK_TYPE_VECTOR_ARRAY_INT32:
			fprintf( stdout, "%08x ", target->items32[index] );
			break;
		case MK_TYPE_VECTOR_ARRAY_INT64:
			fprintf( stdout, "%016x ", target->items64[index] );
			break;
		}
	}

}

static
void dump_hashtable_node( MK_HASHTABLE *target, unsigned int rank )
{
	void *handle = NULL;
	if( target == NULL )
		return;

	print_space( rank );
	fprintf( stdout, "MK_HASHTABLE\n" );
	handle = 
		mk_enum_item_hashtable_begin( target );
	while( handle != NULL )
	{
		const MK_CHAR *key = NULL;
		void *value = NULL;
		handle = mk_enum_item_hashtable_next( target, handle, &key, &value );

		print_space( rank );
		fprintf( stdout, "ITEM:%s\n", key );
		if( value != NULL )
		{
			dump_node( value, rank + 1 );
		}
		else
		{
			print_space( rank + 1 );
			fprintf( stdout, "( NULL )\n" );
		}
	}
}

static
void dump_expr( MK_NODE_EXPR *target, unsigned int rank )
{
	unsigned int attribute = 0;
	if( target == NULL )
	{
		print_space( rank );
		fprintf( stdout, "(NULL)\n" );
		return;
	}

	attribute = MK_TYPE_ATTRIBUTE( target->flags );
	switch( attribute )
	{
	case MK_TYPE_NODE_EXPR_FUNCTION_CALL:
	case MK_TYPE_NODE_EXPR_FUNCTION_CALL_INSTANCE:
	case MK_TYPE_NODE_EXPR_FUNCTION_CALL_STATIC:
		print_space( rank );
		if( attribute == MK_TYPE_NODE_EXPR_FUNCTION_CALL_INSTANCE )
			fprintf( stdout, "@" );
		else if( attribute == MK_TYPE_NODE_EXPR_FUNCTION_CALL_STATIC )
			fprintf( stdout, "@@" );
		fprintf( stdout, 
			"CALL:%s\n", 
			target->u1.symbolName );
		print_space( rank );
		fprintf( stdout, 
			"ARGS\n" );
		if( target->u2.args != NULL )
			dump_node( target->u2.args, rank + 1 );
		break;
	
	case MK_TYPE_NODE_EXPR_NODE_WITH_PARAM:
		print_space( rank );
		fprintf( stdout,"NODE_WITH_PARAM\n" );
		dump_node( target->u1.node, rank + 1 );
		break;
	
	case MK_TYPE_NODE_EXPR_ARRAY_DEFINITION:
		print_space( rank );
		fprintf( stdout, "DEF_ARRAY\n" );
		dump_vector_node( target->u1.arrayDefinition, rank + 1 );
		break;

	case MK_TYPE_NODE_EXPR_MULTIPLESYMBOL:
		print_space( rank );
		fprintf( stdout, "SYMBOL(M):\n" );
		dump_node( target->u1.multipleSymbols, rank + 1 );
		break;

	case MK_TYPE_NODE_EXPR_SYMBOL:
		print_space( rank );
		fprintf( stdout, "SYMBOL:%s\n", target->u1.symbolName );
		break;

	case MK_TYPE_NODE_EXPR_ATSYMBOL:
		print_space( rank );
		fprintf( stdout, "SYMBOL:@%s\n", target->u1.symbolName );
		break;
	
	case MK_TYPE_NODE_EXPR_DBLATSYMBOL:
		print_space( rank );
		fprintf( stdout, "SYMBOL:@@%s\n", target->u1.symbolName );
		break;

	case MK_TYPE_NODE_EXPR_OPERATION:
		print_space( rank );
		fprintf( stdout, "OPERATION:%s\n",
			MK_RESERVED_MARK[MK_LEX_RESERVED_MARK_INDEX( target->flags )].name );
		dump_node( target->u1.left, rank + 1 );
		dump_node( target->u2.right, rank + 1 );
		break;

	case MK_TYPE_NODE_EXPR_RETURN:
		print_space( rank );
		fprintf( stdout, "RETURN:\n" );
		dump_node( target->u1.left, rank + 1 );
		break;

	case MK_TYPE_NODE_EXPR_RAISE:
		print_space( rank );
		fprintf( stdout, "RAISE:\n" );
		dump_node( target->u1.left, rank + 1 );
		break;

	case MK_TYPE_NODE_EXPR_BREAK:
		print_space( rank );
		fprintf( stdout, "BREAK:\n" );
		break;

	case MK_TYPE_NODE_EXPR_CONTINUE:
		print_space( rank );
		fprintf( stdout, "CONTINUE:\n" );
		break;

	case MK_TYPE_NODE_EXPR_NEW:
		print_space( rank );
		fprintf( stdout, "NEW:%s\n",  target->u1.symbolName );
		dump_node( target->u2.args, rank + 1 );
		break;

	case MK_TYPE_NODE_EXPR_SYMBOL_THIS:
		print_space( rank );
		fprintf( stdout, "THIS:\n" );
		break;

	case MK_TYPE_NODE_EXPR_SYMBOL_SUPER:
		print_space( rank );
		fprintf( stdout, "SUPER:\n" );
		break;

	case MK_TYPE_NODE_EXPR_SYMBOL_OWNER:
		print_space( rank );
		fprintf( stdout, "OWNER:\n" );
		break;

	case MK_TYPE_NODE_EXPR_SUPER_CALL:
		print_space( rank );
		fprintf( stdout, "CALL SUPER:\n" );
		dump_node( target->u2.args, rank + 1 );
		break;
	case MK_TYPE_NODE_EXPR_SYMBOL_NIL:
		print_space( rank );
		fprintf( stdout, "NIL:\n" );
		break;
	case MK_TYPE_NODE_EXPR_INT32:
		print_space( rank );
		fprintf( stdout, "INT32:%d\n", target->u2.constantValue );
		break;
	case MK_TYPE_NODE_EXPR_INT64:
		print_space( rank );
		fprintf( stdout, "INT64:%d %d\n",target->u1.constantValue, target->u2.constantValue );
		break;
	case MK_TYPE_NODE_EXPR_FLOAT:
		{
			unsigned int value[2] = 
				{ target->u1.constantValue, target->u2.constantValue };
			MK_FLOAT *d = ( MK_FLOAT *)value;
			print_space( rank );
			fprintf( stdout, "FLOAT:%f\n", *d );
		}
		break;
	case MK_TYPE_NODE_EXPR_STRING:
		print_space( rank );
		fprintf( stdout, "STRING:%s\n", target->u2.value );
		break;
	case MK_TYPE_NODE_EXPR_BACK_IF_CONDITION:
		print_space( rank );
		fprintf( stdout, "BACK_IF_CONDITION\n" );
		print_space( rank );
		fprintf( stdout, "DO\n" );
		dump_node( target->u1.left, rank + 1 );
		print_space( rank );
		fprintf( stdout, "CONDITION\n" );
		dump_node( target->u2.right, rank + 1 );
		break;
	case MK_TYPE_NODE_EXPR_STRING_FORMAT_ROOT:
		print_space( rank );
		fprintf( stdout, "STRING_FORMAT_ROOT\n" );
		dump_vector_node( target->u1.stringParts, rank +1 );
		break;
	case MK_TYPE_NODE_EXPR_STRING_FORMAT_DEFUALT:
		print_space( rank );
		fprintf( stdout, "STRING_FORMAT_DEFAULT\n" );
		dump_expr( target->u1.left, rank +1 );
		break;
	case MK_TYPE_NODE_EXPR_STRING_FORMAT_LEFT:
		print_space( rank );
		fprintf( stdout, "STRING_FORMAT_LEFT\n" );
		dump_expr( target->u1.left, rank +1 );
		break;
	case MK_TYPE_NODE_EXPR_STRING_FORMAT_RIGHT:
		print_space( rank );
		fprintf( stdout, "STRING_FORMAT_RIGHT\n" );
		dump_expr( target->u1.left, rank +1 );
		break;
	case MK_TYPE_NODE_EXPR_STRING_FORMAT_TIME:
		print_space( rank );
		fprintf( stdout, "STRING_FORMAT_TIME\n" );
		dump_expr( target->u1.left, rank +1 );
		break;
	default:
		print_space( rank );
		fprintf( stdout, "UNKNOWN EXPR:%08x\n", target->flags );
		break;
	}
}

void dump_node( void *target, unsigned int rank )
{
	unsigned int flags = 0;
	
	if( target == NULL )
	{
		print_space( rank );
		fprintf( stdout, "(NULL)\n" );
		return;
	}
	if( rank > 30 )
	{
		print_space( rank );
		fprintf( stdout, "...\n" );
		return;
	}

	if( !( (INT_PTR)target & MK_VM_FRAME_ITEM_TYPE_DIRECT_VALUE ) &&
		( (INT_PTR)target & MK_VM_FRAME_ITEM_TYPE_DIRECT_POINTER ) )
	{
		target = (unsigned int *)((INT_PTR)target & ~MK_VM_FRAME_ITEM_TYPE_DIRECT_POINTER ); 
		target = *( (unsigned int**)target );
	}

	if( target == NULL )
	{
		flags = 0;
	}
	else if( (INT_PTR)target & MK_VM_FRAME_ITEM_TYPE_DIRECT_VALUE )
	{
		flags = MK_OBJECT_TYPE_VM_FRAME_ITEM;
	}
	else 
	{
		flags =	*( (unsigned int*)target );
	}
	switch( MK_OBJECT_TYPE( flags ) )
	{
	case MK_TYPE_CLASS:
		{
			MK_CLASS *classTarget = 
				( MK_CLASS * )target;
			print_space( rank );
			fprintf( stdout, "CLASS:%s:%s\n", 
				classTarget->nameThis, 
				( classTarget->nameSuper == NULL ) ? "" : classTarget->nameSuper );
			print_space( rank );
			fprintf( stdout, "MEMBERS\n" );
			dump_node( classTarget->variables, rank + 1 );
			print_space( rank );
		}
		break;
	
	case MK_TYPE_VARIABLE:
		{
			MK_VARIABLE *variableTarget = 
				( MK_VARIABLE * )target;
			if( variableTarget->flags & MK_TYPE_ATTRIBUTE_VARIABLE_METHOD )
			{
				print_space( rank );
				fprintf( stdout, "FUNCTION:%s\n", variableTarget->name );
				print_space( rank );
				fprintf( stdout, "ARG\n" );
				dump_node( variableTarget->args, rank + 1 );
				if( variableTarget->flags & MK_TYPE_ATTRIBUTE_VARIABLE_METHOD_NATIVE )
				{
					print_space( rank );
					fprintf( stdout, "NATIVE:0x%08p\n", variableTarget->entryPoint );
				}
				else
				{
					print_space( rank );
					fprintf( stdout, "IMPL\n" );
					dump_node( variableTarget->entryPoint, rank + 1 );
				}
			}
			else if( !( variableTarget->flags & MK_TYPE_ATTRIBUTE_VARIABLE_MODULE ) )
			{
				print_space( rank );
				fprintf( stdout, "VARIABLE:%s\n", 
					variableTarget->name );
				dump_node( variableTarget->defaultValue, rank + 1 );
			}
			else
			{
				print_space( rank );
				fprintf( stdout, "MODULE_VARIABLE:%s\n",
					variableTarget->name );
				dump_node( variableTarget->moduleVariables, rank + 1 );
			}
		}
		break;
	
	case MK_TYPE_NODE_BLOCK:
		{
			MK_NODE_BLOCK *blockTarget =
				( MK_NODE_BLOCK * )target;
			print_space( rank );
			fprintf( stdout, "BLOCK:\n" );
			dump_node( blockTarget->exprs, rank + 1 );
		}
		break;

	case MK_TYPE_NODE_IF:
		{
			MK_NODE_IF *ifTarget =
				( MK_NODE_IF * )target;
			MK_NODE_IF *previous = NULL;
			int index = 0;
			while( ifTarget != NULL )
			{
				print_space( rank );
				fprintf( stdout, "IF(%d)\n", index );
				print_space( rank );
				fprintf( stdout, "CONDITION:\n" );
				dump_node( ifTarget->expr, rank + 1 );
				print_space( rank );
				fprintf( stdout, "IMPL:\n" );
				dump_node( ifTarget->block, rank + 1 );
				previous = ifTarget;
				ifTarget = ifTarget->next;
				index ++;
			}
		}
		break;

	case MK_TYPE_NODE_WHILE:
		{
			MK_NODE_WHILE *whileTarget =
				( MK_NODE_WHILE * )target;
			print_space( rank );
			fprintf( stdout, "WHILE(DO WHILE)\n" );
			print_space( rank );
			fprintf( stdout, "CONDITION\n" );
			dump_node( whileTarget->block, rank + 1 );
			print_space( rank );
			fprintf( stdout, "IMPL\n" );
			dump_node( whileTarget->expr, rank + 1 );
		}
		break;

	case MK_TYPE_NODE_EXPR:
		dump_expr( (MK_NODE_EXPR*)target, rank + 1 );
		break;

	case MK_OBJECT_TYPE_OBJECTCODE:
		{
			MK_OBJECTCODE *objectCode = 
				( MK_OBJECTCODE * )target;
			print_space( rank );
			fprintf( stdout, "OBJECTCODE:\n" );

			print_space( rank + 1 );
			fprintf( stdout, "CLASSES:\n" );
			dump_node( objectCode->classes, rank + 1 );
		}
		break;

	case MK_OBJECT_TYPE_VM_FRAME:
		{
			MK_VM_FRAME *pFrame = 
				( MK_VM_FRAME * )target;
			while( pFrame != NULL )
			{
				print_space( rank + 1 );
				fprintf( stdout, "LocalVariables:\n" );
				dump_node( pFrame->localVariables, rank + 1 );
				print_space( rank + 1 );
				fprintf( stdout, "CurrentFunction:\n" );
				dump_node( pFrame->pMethod, rank + 1 );
				pFrame = pFrame->previous;
			}
		}
		break;

	case MK_OBJECT_TYPE_VM_FRAME_ITEM:
		{
			MK_VM_FRAME_ITEM *pFrameItem = 
				(MK_VM_FRAME_ITEM *)target;
			print_space( rank + 1 );
			fprintf( stdout, "VM_FRAME_ITEM:" );
			if( (INT_PTR)pFrameItem & MK_VM_FRAME_ITEM_TYPE_DIRECT_VALUE )
			{
				fprintf( stdout, "DIRECT_VALUE_TYPE:0x%x(value:%d)\n", 
					pFrameItem, 
					( (INT_PTR)target & MK_VM_FRAME_ITEM_DIRECT_INT_VALUE_MASK ) >> 1 );
			}
			else
			{
				switch( MK_TYPE_ATTRIBUTE( pFrameItem->flags ) )
				{
				case MK_VM_FRAME_ITEM_TYPE_NIL:
					fprintf( stdout, "NIL_TYPE\n" );
					break;
					
				case MK_VM_FRAME_ITEM_TYPE_CLASS:
					fprintf( stdout, "CLASS_TYPE:%s\n", pFrameItem->classTypeValue.typeName );
					if( pFrameItem->classTypeValue.variables != NULL )
						dump_node( pFrameItem->classTypeValue.variables, rank + 1 );
					break;

				case MK_VM_FRAME_ITEM_TYPE_MODULE:
					fprintf( stdout, "MODULE_TYPE:%s\n", pFrameItem->classTypeValue.typeName );
					if( pFrameItem->classTypeValue.variables != NULL )
						dump_node( pFrameItem->classTypeValue.variables, rank + 1 );
					break;

				case MK_VM_FRAME_ITEM_TYPE_FLOAT_VALUE:
					fprintf( stdout, "FLOAT_TYPE(%f)\n", pFrameItem->floatTypeValue );
					break;

				case MK_VM_FRAME_ITEM_TYPE_INT_VALUE:
					fprintf( stdout, "INT32_TYPE(%d)\n", pFrameItem->int32TypeValue );
					break;

				case MK_VM_FRAME_ITEM_TYPE_STRING_VALUE:
					fprintf( stdout, "STRING_TYPE(\"%s\")\n", pFrameItem->stringTypeValue );
					break;
				case MK_VM_FRAME_ITEM_TYPE_ARRAY_VALUE:
					fprintf( stdout, "ARRAY_TYPE:\n" );
					dump_node( pFrameItem->arrayTypeValue, rank + 1 );
					break;
				case MK_VM_FRAME_ITEM_TYPE_NODE_VALUE:
					fprintf( stdout, "NODE_TYPE:\n" );
					break;
				default:
					fprintf( stdout, "\n" );
					break;
				}
			}
		}
		break;
	
	case MK_TYPE_TRY_BLOCK:
		{
			MK_TRY_BLOCK *tryBlock = 
				( MK_TRY_BLOCK * )target;
			int size = tryBlock->blockCatch != NULL ? mk_size_vector( tryBlock->blockCatch ) : 0;
			int index = 0;
			print_space( rank );
			fprintf( stdout, "TRY:\n" );
			dump_node( tryBlock->blockTry, rank + 1 );
			for( index = 0; index < size; index ++ )
			{
				MK_CATCH_BLOCK *catchBlock = 
					(MK_CATCH_BLOCK *)mk_get_at_vector( tryBlock->blockCatch, index );
				dump_node( catchBlock, rank + 1 );
			}
			print_space( rank );
			fprintf( stdout, "FINALLY:\n" );
			dump_node( tryBlock->blockFinally, rank + 1 );
			
		}
		break;
	case MK_TYPE_CATCH_BLOCK:
		{
			MK_CATCH_BLOCK *catchBlock = 
				(MK_CATCH_BLOCK*)target;
			if( catchBlock->paramCatch == NULL )
			{
				print_space( rank );
				fprintf( stdout, "ELSE:\n" );
			}
			else
			{
				print_space( rank );
				fprintf( stdout, "CATCH:\n" );
				dump_node( catchBlock->paramCatch, rank + 1 );
			}
			dump_node( catchBlock->blockCatch, rank + 1 );
		}
		break;

	case MK_OBJECT_TYPE_VM_STRUCT:
		{
			MK_VM_STRUCT *pStruct = 
				( MK_VM_STRUCT * )target;
			print_space( rank );
			fprintf( stdout, "VMSTRUCT:\n" );
			dump_node( pStruct->code, rank + 1 );
			dump_node( pStruct->global, rank + 1 );
			dump_node( pStruct->pCurrentFrame, rank + 1 );
			print_space( rank + 1 );
			fprintf( stdout, "LocalStack:\n" );
//			dump_node( pStruct->localStack, rank + 1 );
		}
		break;

	case MK_TYPE_HASHTABLE:
		dump_hashtable_node( target, rank + 1 );
		break;

	case MK_TYPE_VECTOR:
		dump_vector_node( target, rank + 1 );
		break;

	default:
		print_space( rank );
		fprintf( stdout, "UNKNOWN NODE:%08x\n", flags );
		break;
	}
}

void dump_stack(MK_VM_STRUCT *vm, int maxSize)
{
	MK_VM_STACK *pStack = &vm->localStack;
	int current = maxSize;
	MK_VM_FRAME_ITEM **pCurrent = pStack->current;
	
	do
	{
		current --;
		fprintf(stdout, "[% 2d]\n", current);
		dump_node(*pCurrent, 0);
	}while (pCurrent != pStack->start && current > 0);
}
