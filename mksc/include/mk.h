#ifndef __MK_H__
#define __MK_H__

#include "mk_config.h"

// --- common data

// hashtable item
typedef struct TAG_MK_MAP_ITEM
{
	union
	{
		MK_CHAR *key;
		INT32	*int32Key;
		INT64	*int64Key;
		INT_PTR	intPtrKey;
	};
	void *value[0];
}MK_MAP_ITEM;

// hashtable
typedef struct TAG_MK_HASHTABLE
{
	unsigned int flags;
	unsigned int size;
	unsigned int used;
	MK_MAP_ITEM *elems;
} MK_HASHTABLE;

// vector
typedef struct TAG_MK_VECTOR
{
	unsigned int flags;
	unsigned int size;
	unsigned int used;
	union
	{
		INT_PTR*	items;
		INT8 *		items8;
		INT16*		items16;
		INT32*		items32;
		INT64*		items64;
	};
} MK_VECTOR;

// memory pool.
typedef struct TAG_MK_MEMORY_POOL
{
	unsigned char memory[MK_SIZEOF_MEMORY_POOL_BLOCK];
	struct TAG_MK_MEMORY_POOL *previous;
	unsigned char *next;
} MK_MEMORY_POOL;

// --- code object

// variable definition object
struct TAG_MK_NODE_EXPR;
struct TAG_MK_CLASS;
struct TAG_MK_VM_STRUCT;
typedef int (*FP_MK_NATIVE_METHOD)(struct TAG_MK_VM_STRUCT *vm, unsigned int varArgC );
typedef struct TAG_MK_VARIABLE
{
	unsigned int flags;
	MK_SYM_CHAR *name;
	union
	{
		// for module variable
		struct TAG_MK_CLASS *moduleVariables;

		struct
		{
			// for variable
			struct TAG_MK_NODE_EXPR *defaultValue;

		};
		
		// for method
		struct
		{
			MK_VECTOR* args;
			void *entryPoint;	// script:MK_NODE_BLOCK* native:FP_MK_NATIVE_METHOD
		};
	};
} MK_VARIABLE;

// namespace object
typedef struct TAG_MK_NAMESPACE
{
	unsigned int flags;
	MK_SYM_CHAR *name;
	MK_HASHTABLE *child;	// class, module, namespace
} MK_NAMESPACE;

// class code object
typedef struct TAG_MK_CLASS
{
	unsigned int flags;
	MK_SYM_CHAR *nameThis;
	MK_SYM_CHAR *nameSuper;
	MK_HASHTABLE* variables;
	MK_VECTOR* usingNames;
	INT_PTR *operatorMethod;
} MK_CLASS;

// expr code object
typedef struct TAG_MK_NODE_EXPR
{
	unsigned int flags;

	MK_CHAR *nameStream;
	unsigned int line;

	union
	{
		struct
		{
			union
			{
				MK_SYM_CHAR *symbolName;			// method_call, symbol, new, delete
				MK_SYM_CHAR *typeName;				// MK_VM_VARIABLE_INSTANCE
				MK_VECTOR *multipleSymbols;			// multiple symbol top
				MK_VECTOR *stringParts;				// string parts.
				MK_VECTOR *arrayDefinition;			// array definition.
				unsigned int length;				// string
				unsigned int constantValue;			// constant_value(high)
				struct TAG_MK_NODE_EXPR *left;		// operation, return
				unsigned int *node;					// node_with_param(MK_VARIABLE)
			}u1;
			union
			{
				struct TAG_MK_NODE_EXPR *right;		// operation
				unsigned int constantValue;			// constant_value(low)
				void *value;						// string
				MK_VECTOR *args;					// method_call, new
			}u2;
		};
		MK_FLOAT floatValue;
	};

} MK_NODE_EXPR;

// block( exprs ) code object
typedef struct TAG_MK_NODE_BLOCK
{
	unsigned int flags;
	MK_VECTOR *exprs;
} MK_NODE_BLOCK;

// if block code object
typedef struct TAG_MK_NODE_IF
{
	unsigned int flags;
	struct TAG_MK_NODE_EXPR *expr;		// condition
	struct TAG_MK_NODE_BLOCK *block;	// internal block
	struct TAG_MK_NODE_IF *next;		// next if block
}MK_NODE_IF;

// while block code object
typedef struct TAG_MK_NODE_WHILE
{
	unsigned int flags;
	struct TAG_MK_NODE_EXPR *expr;		// condition
	struct TAG_MK_NODE_BLOCK *block;	// internal block
}MK_NODE_WHILE;

// catch block struct
typedef struct TAG_MK_CATCH_BLOCK
{
	unsigned int flags;
	MK_NODE_EXPR  *paramCatch;	// MK_TYPE_NODE_EXPR_NODE_WITH_PARAM
	MK_NODE_EXPR *blockCatch;
} MK_CATCH_BLOCK;

// try ... ( catch ... )+ finally ...
typedef struct TAG_MK_TRY_BLOCK
{
	unsigned int flags;
	MK_NODE_BLOCK *blockTry;
	MK_VECTOR *blockCatch;				// MK_CATCH_BLOCK
	MK_NODE_BLOCK *blockNoException;	// else
	MK_NODE_BLOCK *blockFinally;
} MK_TRY_BLOCK;

// --- vm execute data

struct TAG_MK_VM_FRAME_ITEM;
typedef struct TAG_MK_VM_FRAME_ITEM_CLASS
{
	MK_SYM_CHAR *typeName;
	struct TAG_MK_VM_FRAME_ITEM *child;
	MK_HASHTABLE *variables;
} MK_VM_FRAME_ITEM_CLASS;

struct TAG_MK_VM_FRAME;
#ifdef _MSC_VER
# pragma pack(push, 4)
#elif __GNUC__
# pragma pack(4)
#endif
typedef struct TAG_MK_VM_FRAME_ITEM
{
	unsigned int flags;
	union
	{
		// integer
		union	
		{
			struct
			{
				INT32 int32TypeValue;
				INT32 int32TypeHighValue;
			};
			INT64 int64TypeValue;
		};
		
		// string
		struct	
		{
			INT32 sizeString;
			MK_CHAR *stringTypeValue;
		};

		// float
		struct
		{
			unsigned int padding;
			MK_FLOAT floatTypeValue;	
		};

		// class
		struct
		{
			MK_SYM_CHAR *typeName;
			struct TAG_MK_VM_FRAME_ITEM *child;
			MK_HASHTABLE *variables;
		} classTypeValue;
		
		// array
		MK_VECTOR *arrayTypeValue;

		// node
		struct
		{
			struct TAG_MK_VM_FRAME *definedFrame;
			struct TAG_MK_VM_FRAME_ITEM *pOwner;
			unsigned int *node;
		} code;

		// native object
		struct
		{
			INT32 sizeNativePtr;
			void *nativePtr;
			void (*native_free)(void);
		} nativeObjectTypeValue;

		// reference.
		struct
		{
			union
			{
				struct
				{
					MK_HASHTABLE *target;
					MK_SYM_CHAR *symbolName;
				} referenceTypeValue;

				// reference array.
				struct
				{
					INT32 index;
					MK_VECTOR *target;
				} referenceArrayTypeValue;

				// reference string
				struct
				{
					INT32 index;
					struct TAG_MK_VM_FRAME_ITEM *target;
				} referenceStringTypeValue;
			};
			struct MK_VM_FRAME_ITEM *requiredType;
		};
	};
} MK_VM_FRAME_ITEM;
#ifdef _MSC_VER
# pragma pack(pop)
#elif __GNUC__
# pragma pack(0)
#endif

typedef struct TAG_MK_VM_FRAME
{
	unsigned int flags;
	MK_VM_FRAME_ITEM *pThis;
	MK_HASHTABLE *localVariables;
	MK_VARIABLE *pMethod;
	MK_VM_FRAME_ITEM *pLastResultBlock;
	struct TAG_MK_VM_FRAME *pOwnerFrame;
	struct TAG_MK_VM_FRAME *previous;
} MK_VM_FRAME;

typedef struct TAG_MK_OBJECTCODE
{
	unsigned int flags;
	MK_HASHTABLE* classes;
} MK_OBJECTCODE;

typedef struct TAG_MK_MANAGED_VM_FRAME_ITEM_TABLE
{
	unsigned int freeSpace;
	unsigned int nextIndex;
	struct TAG_MK_MANAGED_VM_FRAME_ITEM_TABLE *previous;
	MK_VM_FRAME_ITEM table[MK_SIZEOF_MANAGED_TABLE];
} MK_MANAGED_VM_FRAME_ITEM_TABLE;

typedef struct TAG_MK_MANAGED_VM_FRAME_TABLE
{
	unsigned int freeSpace;
	unsigned int nextIndex;
	struct TAG_MK_MANAGED_VM_FRAME_TABLE *previous;
	MK_VM_FRAME table[MK_SIZEOF_MANAGED_TABLE];
} MK_MANAGED_VM_FRAME_TABLE;

typedef struct TAG_MK_MANAGED_HASH_TABLE
{
	unsigned int freeSpace;
	unsigned int nextIndex;
	struct TAG_MK_MANAGED_HASH_TABLE *previous;
	MK_HASHTABLE hashTables[MK_SIZEOF_MANAGED_TABLE];
} MK_MANAGED_HASH_TABLE;

typedef struct TAG_MK_MANAGED_VECTOR_TABLE
{
	unsigned int freeSpace;
	unsigned int nextIndex;
	struct TAG_MK_MANAGED_VECTOR_TABLE *previous;
	MK_VECTOR vectors[MK_SIZEOF_MANAGED_TABLE];
} MK_MANAGED_VECTOR_TABLE;


// --- lexer
#define SIZEOF_TOKENBUFFER		32

typedef struct TAG_MK_STREAM
{
	void *stream;
	unsigned char current;
	MK_CHAR back;
	void*(*open)(void*);
	int(*getc)(void*);
	void(*close)(void*);
	void(*trace)(unsigned int code, MK_CHAR*message);
}MK_STREAM;

#define MK_LEX_SIZEOF_PLAINTEXT_BUFFER			128
#define MK_LEX_SIZEOF_EXTEND_PLAINTEXT_BUFFER	64

#define MK_LEX_STATE_CODEBLOCK					0
#define MK_LEX_STATE_IN_STRING					1

typedef struct TAG_MK_LEXER
{
	MK_STREAM stream;	// stream access interface
	MK_CHAR *name;
	unsigned int stateLex;

	int sizePlainText;
	int positionNextPlainText;
	int positionCurrentToken;
	int positionNextToken;
	MK_CHAR *plainText;

	unsigned int nextLine;
	MK_CHAR *nextText;
	unsigned int nextLength;
	unsigned int nextValue;
	void *nextObject;

	unsigned int line;
	MK_CHAR *text;
	unsigned int length;
	unsigned int value;
	void *object;

	unsigned int maxBufferLength;
	unsigned int sizeExtendBufferLength;
	unsigned int defaultBufferLength;

	unsigned char hasNextSpace;
	int hasError;
} MK_LEXER;

typedef struct TAG_MK_KEYWORD_TABLE
{
	unsigned int id;
	MK_CHAR	*name;
}MK_KEYWORD_TABLE;

extern const MK_KEYWORD_TABLE MK_RESERVED_SYMBOL[];
extern const MK_KEYWORD_TABLE MK_RESERVED_MARK[];

// --- vm cache data
typedef struct TAG_MK_VM_CACHE
{
	MK_SYM_CHAR			*internalClassSymbolName[0x0f];	// constant class symbolnae.
	MK_CLASS			*pConstantClass[0x0f];			// constant class type.
} MK_VM_CACHE;

// ---- vm local stack
typedef struct TAG_MK_VM_STACK
{
	MK_VM_FRAME_ITEM	**start;
	MK_VM_FRAME_ITEM	**current;
	MK_VM_FRAME_ITEM	**last;
} MK_VM_STACK;

// ---- vm core
typedef struct TAG_MK_VM_STRUCT
{
	unsigned int flags;

	MK_VM_CACHE	*cache;
	MK_MEMORY_POOL *memoryPool;			// memory pool
	MK_HASHTABLE *hashSymbolName;		// symbol name => address of memory pool.

	int (*callBlock[0x10])		(struct TAG_MK_VM_STRUCT *, unsigned int*);									// block call function table
	int (*callNodeExpr[0xcf])	(struct TAG_MK_VM_STRUCT *, MK_NODE_EXPR*, int hasParent, int reference );	// expr call function table

	MK_OBJECTCODE *code;

	// vm execute environment.
	MK_HASHTABLE *global;			// global variables
	MK_VM_FRAME *pTopFrame;			// top frame
	MK_VM_FRAME *pCurrentFrame;		// current frame

	MK_VM_STACK localStack;		// stack

	MK_MANAGED_VM_FRAME_ITEM_TABLE *pFrameItemTable;
	MK_MANAGED_VM_FRAME_TABLE *pFrameTable;
	MK_MANAGED_HASH_TABLE *pHashTable;
	MK_MANAGED_VECTOR_TABLE *pVectorTable;

	MK_VM_FRAME_ITEM *exceptionObject;	// current exception object

	unsigned int step;				// total step count for expr
	unsigned int gcPhase;			// gc phase
	void **stackTop;				// _main() stack position
} MK_VM_STRUCT;

// ---- object flags( objet.flag )

// object type id
#define MK_OBJECT_TYPE_MASK					0xf0000000
#define MK_OBJECT_TYPE(value)				( (value) & MK_OBJECT_TYPE_MASK )
#define MK_OBJECT_TYPE_INDEX(value)			( value >> 28 )
#define MK_OBJECT_TYPE_INVALID				0x00000000
#define MK_OBJECT_TYPE_VM_STRUCT			0x10000000		// MK_VM_STRUCT
#define MK_OBJECT_TYPE_OBJECTCODE			0x20000000		// MK_OBJECTCODE
#define MK_OBJECT_TYPE_VM_FRAME				0x30000000		// MK_VM_FRAME
#define MK_OBJECT_TYPE_VM_FRAME_ITEM		0x40000000		// MK_VM_FRAME_ITEM
#define MK_TYPE_NAMESPACE					0x50000000		// MK_NAMESPACE
#define MK_TYPE_CLASS						0x60000000		// MK_CLASS
#define MK_TYPE_VARIABLE					0x70000000		// MK_VARIABLE
#define MK_TYPE_NODE_BLOCK					0x80000000		// MK_NODE_BLOCK
#define MK_TYPE_NODE_IF						0x90000000		// MK_NODE_IF
#define MK_TYPE_NODE_WHILE					0xa0000000		// MK_NODE_WHILE
#define MK_TYPE_TRY_BLOCK					0xb0000000		// MK_TRY_BLOCK
#define MK_TYPE_CATCH_BLOCK					0xc0000000		// MK_CATCH_BLOCK
#define MK_TYPE_NODE_EXPR					0xd0000000		// MK_NODE_EXPR
#define MK_TYPE_HASHTABLE					0xe0000000		// MK_HASHTABLE
#define MK_TYPE_VECTOR						0xf0000000		// MK_VECTOR

// object attribute
#define MK_TYPE_ATTRIBUTE_MASK					0x0fe00000
#define MK_TYPE_ATTRIBUTE(value)				( (value) & MK_TYPE_ATTRIBUTE_MASK )
#define MK_TYPE_SET_ATTRIBUTE(target, value)	( ( (target) & ~MK_TYPE_ATTRIBUTE_MASK ) | MK_TYPE_ATTRIBUTE(value) )
#define MK_TYPE_ATTRIBUTE_INDEX(value)			( MK_TYPE_ATTRIBUTE(value) >> 21 )

// attribute of MK_OBJECT_TYPE_VM_STRUCT

// attribute of MK_OBJECT_TYPE_OBJECTCODE

// attribute of MK_OBJECT_TYPE_VM_FRAME
#define MK_VM_FRAME_TYPE_ROOT					0x00000000
#define MK_VM_FRAME_TYPE_NODE_WITH_PARAM		0x01000000

#define MK_VM_FRAME_ITEM_TYPE_GROUP(value)		( (value) & 0x0f000000 )
// attribute of MK_OBJECT_TYPE_VM_FRAME_ITEM
#define MK_VM_FRAME_ITEM_TYPE_UNINITIALIZED		0x00000000
#define MK_VM_FRAME_ITEM_TYPE_NIL				0x01000000
#define MK_VM_FRAME_ITEM_TYPE_CLASS				0x02000000
#define MK_VM_FRAME_ITEM_TYPE_MODULE			0x02800000
#define MK_VM_FRAME_ITEM_TYPE_INT_VALUE			0x03000000
#define MK_VM_FRAME_ITEM_TYPE_FLOAT_VALUE		0x04000000
#define MK_VM_FRAME_ITEM_TYPE_ARRAY_VALUE		0x05000000
#define MK_VM_FRAME_ITEM_TYPE_STRING_VALUE		0x06000000
#define MK_VM_FRAME_ITEM_TYPE_NODE_VALUE		0x07000000
#define MK_VM_FRAME_ITEM_TYPE_REFERENCE_VALUE	0x0d000000
#define MK_VM_FRAME_ITEM_TYPE_REFERENCE_ARRAY_VALUE			\
												0x0d800000
#define MK_VM_FRAME_ITEM_TYPE_REFERENCE_STRING_VALUE		\
												0x0d400000
#define MK_VM_FRAME_ITEM_TYPE_NATIVE_PTR		0x0e000000
#define MK_VM_FRAME_ITEM_TYPE_THIS				0x0f200000
#define MK_VM_FRAME_ITEM_TYPE_SUPER				0x0f400000
#define MK_VM_FRAME_ITEM_TYPE_OWNER				0x0f600000

#define MK_VM_FRAME_ITEM_TYPE_STATIC_INSTANCE	0x00080000
#define MK_VM_FRAME_ITEM_TYPE_FINAL				0x00040000

// direct int value.
#define MK_VM_FRAME_ITEM_TYPE_DIRECT_VALUE		0x00000001 
#define MK_VM_FRAME_ITEM_TYPE_DIRECT_MASK		0x70000000
#define MK_VM_FRAME_ITEM_TYPE_INT_MINUS_VALUE	0x80000000
#define MK_VM_FRAME_ITEM_DIRECT_INT_VALUE_MASK	0x7ffffffe	// -1,073,741,823 ~ +1,073,741,823

// direct pointer to MK_VM_FRAME_ITEM ( use module )
#define MK_VM_FRAME_ITEM_TYPE_DIRECT_POINTER	0x00000002

// variable hashtable extend[0]		constraints
#define MK_VM_FRAME_ITEM_EXTEND_TYPE_READ_ACCESS_MASK	0x30000000
#define MK_VM_FRAME_ITEM_EXTEND_TYPE_READ_PUBLIC		0x10000000
#define MK_VM_FRAME_ITEM_EXTEND_TYPE_READ_PROTECTED		0x20000000
#define MK_VM_FRAME_ITEM_EXTEND_TYPE_READ_PRIVATE		0x30000000

#define MK_VM_FRAME_ITEM_EXTEND_TYPE_WRITE_ACCESS_MASK	0xc0000000
#define MK_VM_FRAME_ITEM_EXTEND_TYPE_WRITE_PUBLIC		0x40000000
#define MK_VM_FRAME_ITEM_EXTEND_TYPE_WRITE_PROTECTED	0x80000000
#define MK_VM_FRAME_ITEM_EXTEND_TYPE_WRITE_PRIVATE		0xc0000000

#define MK_VM_FRAME_ITEM_EXTEND_TYPE_FINAL			0x08000000
#define MK_VM_FRAME_ITEM_TYPE_USE_TYPE_CONSTRAINTS	0x04000000	// enable extend[1]

// variable hashtable extend[1]		type constraints

// attribute of MK_TYPE_CLASS
#define MK_TYPE_ATTRIBUTE_CLASS_CLASS				0x00000000
#define MK_TYPE_ATTRIBUTE_CLASS_MODULE				0x01000000

// attribute of MK_TYPE_VARIABLE
#define MK_TYPE_ATTRIBUTE_VARIABLE_VALUE			0x00000000	// variable type
#define MK_TYPE_ATTRIBUTE_VARIABLE_METHOD			0x08000000	// metod type

// style only variable
#define MK_TYPE_ATTRIBUTE_VARIABLE_MODULE			0x04000000	// module instance

// style only method
#define MK_TYPE_ATTRIBUTE_VARIABLE_METHOD_SCRIPT	0x00000000
#define MK_TYPE_ATTRIBUTE_VARIABLE_METHOD_NATIVE	0x04000000
#define MK_TYPE_ATTRIBUTE_VARIABLE_METHOD_OPERATOR	0x02000000
#define MK_TYPE_ATTRIBUTE_VARIABLE_METHOD_VARARGS	0x01000000

#define MK_TYPE_ATTRIBUTE_VARIABLE_STATIC			0x00800000

#define MK_TYPE_ATTRIBUTE_VARIABLE_FINAL			0x00400000	// used by variable type.	!
#define MK_TYPE_ATTRIBUTE_VARIABLE_VIRTUAL			0x00400000	// used by method type.

// read access style
#define MK_TYPE_ATTRIBUTE_VARIABLE_READ_MASK		0x00300000
#define MK_TYPE_ATTRIBUTE_VARIABLE_READ_PUBLIC		0x00100000	// +
#define MK_TYPE_ATTRIBUTE_VARIABLE_READ_PROTECTED	0x00200000	// *
#define MK_TYPE_ATTRIBUTE_VARIABLE_READ_PRIVATE		0x00300000	// -

// write access style( variable only ).
#define MK_TYPE_ATTRIBUTE_VARIABLE_WRITE_MASK		0x000c0000
#define MK_TYPE_ATTRIBUTE_VARIABLE_WRITE_PUBLIC		0x00040000	// +
#define MK_TYPE_ATTRIBUTE_VARIABLE_WRITE_PROTECTED	0x00080000	// *
#define MK_TYPE_ATTRIBUTE_VARIABLE_WRITE_PRIVATE	0x000c0000	// -

#define MK_TYPE_ATTRIBUTE_FUNCTION_OPERATOR_ID		0x0004f000

// method arguments param(testing)
#define MK_TYPE_ATTRIBUTE_VARIABLE_CALL_BY_VALUE	0x00000800

// native method new style(tesiting)
#define MK_TYPE_ATTRIBUTE_VARIABLE_NEW_NATIVE_METHOD	0x00000010

// attribute of MK_TYPE_BLOCK
#define MK_TYPE_ATTRIBUTE_BLOCK_FUNCTION_ROOT		0x00000000
#define MK_TYPE_ATTRIBUTE_BLOCK_IF_BLOCK			0x01000000
#define MK_TYPE_ATTRIBUTE_BLOCK_WHILE_BLOCK			0x02000000
#define MK_TYPE_ATTRIBUTE_BLOCK_TRY_BLOCK			0x03000000
#define MK_TYPE_ATTRIBUTE_BLOCK_CATCH_BLOCK			0x04000000
#define MK_TYPE_ATTRIBUTE_BLOCK_FINALLY_BLOCK		0x05000000
#define MK_TYPE_ATTRIBUTE_BLOCK_NONAME_METHOD		0x06000000

// attribute of MK_TYPE_IF

// attribute of MK_TYPE_NODE_WHILE
#define MK_TYPE_ATTRIBUTE_NODE_WHILE_FRONT			0x01000000
#define MK_TYPE_ATTRIBUTE_NODE_WHILE_BACK			0x02000000

// attribute of MK_TYPE_NODE_EXPR
#define MK_TYPE_NODE_EXPR_SYMBOL			0x02000000
#define MK_TYPE_NODE_EXPR_SYMBOL_THIS		0x02200000
#define MK_TYPE_NODE_EXPR_SYMBOL_OWNER		0x02400000
#define MK_TYPE_NODE_EXPR_SYMBOL_SUPER		0x02600000
#define MK_TYPE_NODE_EXPR_SYMBOL_NIL		0x02800000	// not implemented yet
#define MK_TYPE_NODE_EXPR_SYMBOL_TRUE		0x02a00000	// not implemented yet
#define MK_TYPE_NODE_EXPR_SYMBOL_FALSE		0x02c00000	// not implemented yet
#define MK_TYPE_NODE_EXPR_ATSYMBOL			0x03000000
#define MK_TYPE_NODE_EXPR_DBLATSYMBOL		0x03200000
#define MK_TYPE_NODE_EXPR_STRING_FORMAT_ROOT	0x04000000
#define MK_TYPE_NODE_EXPR_STRING_FORMAT_DEFUALT	0x04200000
#define MK_TYPE_NODE_EXPR_STRING_FORMAT_LEFT	0x04400000
#define MK_TYPE_NODE_EXPR_STRING_FORMAT_RIGHT	0x04600000
#define MK_TYPE_NODE_EXPR_STRING_FORMAT_TIME	0x04800000
#define MK_TYPE_NODE_EXPR_MULTIPLESYMBOL		0x05000000
#define MK_TYPE_NODE_EXPR_OPERATION					0x06000000
#define MK_TYPE_NODE_EXPR_FUNCTION_CALL				0x06200000
#define MK_TYPE_NODE_EXPR_FUNCTION_CALL_INSTANCE	0x06400000
#define MK_TYPE_NODE_EXPR_FUNCTION_CALL_STATIC		0x06600000
#define MK_TYPE_NODE_EXPR_SUPER_CALL				0x06800000	
#define MK_TYPE_NODE_EXPR_RETURN			0x07000000
#define MK_TYPE_NODE_EXPR_BREAK				0x08000000
#define MK_TYPE_NODE_EXPR_CONTINUE			0x09000000
#define MK_TYPE_NODE_EXPR_NEW				0x0a000000
#define MK_TYPE_NODE_EXPR_RAISE				0x0b000000
#define MK_TYPE_NODE_EXPR_NODE_WITH_PARAM	0x0c000000
#define MK_TYPE_NODE_EXPR_BACK_IF_CONDITION	0x0d200000
#define MK_TYPE_NODE_EXPR_ARRAY_DEFINITION	0x0e000000
#define MK_TYPE_NODE_EXPR_ME				0x0e200000

#define MK_TYPE_NODE_EXPR_CONSTANT		0x00e00000
#define MK_TYPE_NODE_EXPR_NIL			0x1e000000
#define MK_TYPE_NODE_EXPR_INT32			0x05e00000
#define MK_TYPE_NODE_EXPR_INT64			0x06e00000
#define MK_TYPE_NODE_EXPR_FLOAT			0x07e00000
#define MK_TYPE_NODE_EXPR_STRING		0x08e00000

#define MK_TYPE_NODE_EXPR_CONST_FINAL	0x00100000	// never changed value

#define MK_TYPE_NODE_EXPR_OPERATIONTYPE_MASK		0x000fc000
#define MK_TYPE_NODE_EXPR_OPERATIONPRIORITY_MASK	0x00003c00

#define MK_MAKE_LEX_RESERVED_MARK( priority, index, isLeftConst, isRightConst )	\
	( MK_LEX_TYPE_RESERVED_MARK + ( ( priority ) << 10 ) + ( ( index ) << 14 ) + ( isLeftConst << 9 ) + ( isRightConst << 8 ) )
#define MK_LEX_RESERVED_MARK_INDEX( id )	\
	( ( ( id ) & 0x000fc000 ) >> 14 )
#define MK_LEX_RESERVED_MARK_PRIORITY( id )	\
	( ( ( id ) & 0x00003c00 ) >> 10 )
#define MKLEX_RESERVED_MARK_IS_LEFT_CONST( id )	\
	( ( id ) & MK_TYPE_NODE_EXPR_OPERATION_LEFT_CONST )
#define MKLEX_RESERVED_MARK_IS_RIGHT_CONST( id )	\
	( ( id ) & MK_TYPE_NODE_EXPR_OPERATION_RIGHT_CONST )

#define MK_RESERVED_MARK_INDEX( id )	MK_LEX_RESERVED_MARK_INDEX( id )
#define MK_RESERVED_MARK_PRIORITY( id )	MK_LEX_RESERVED_MARK_PRIORITY( id )

#define MK_RESERVED_MARK_NAME( id )			\
	( MK_RESERVED_MARK[ MK_RESERVED_MARK_INDEX( id ) ].name )
#define MK_LEX_RESERVED_MARK_NAME( id )			\
	( MK_RESERVED_MARK[ MK_LEX_RESERVED_MARK_INDEX( id ) ].name )

#define MK_LEX_TYPE_RESERVED_MARK_AND_AND	MK_MAKE_LEX_RESERVED_MARK( 0x10 - 9, 0, 1, 1 )
#define MK_LEX_TYPE_RESERVED_MARK_OR_OR		MK_MAKE_LEX_RESERVED_MARK( 0x10 - 10, 1, 1, 1 )
#define MK_LEX_TYPE_RESERVED_MARK_PLUS		MK_MAKE_LEX_RESERVED_MARK( 0x10 - 3, 2, 1, 1 )
#define MK_LEX_TYPE_RESERVED_MARK_MINUS		MK_MAKE_LEX_RESERVED_MARK( 0x10 - 3, 3, 1, 1 )
#define MK_LEX_TYPE_RESERVED_MARK_MUL		MK_MAKE_LEX_RESERVED_MARK( 0x10 - 2, 4, 1, 1 )
#define MK_LEX_TYPE_RESERVED_MARK_DIV		MK_MAKE_LEX_RESERVED_MARK( 0x10 - 2, 5, 1, 1 )
#define MK_LEX_TYPE_RESERVED_MARK_MOD		MK_MAKE_LEX_RESERVED_MARK( 0x10 - 2, 6, 1, 1 )
#define MK_LEX_TYPE_RESERVED_MARK_RSHIFT	MK_MAKE_LEX_RESERVED_MARK( 0x10 - 4, 7, 1, 1 )
#define MK_LEX_TYPE_RESERVED_MARK_LSHIFT	MK_MAKE_LEX_RESERVED_MARK( 0x10 - 4, 8, 1, 1 )
#define MK_LEX_TYPE_RESERVED_MARK_AND		MK_MAKE_LEX_RESERVED_MARK( 0x10 - 5, 9, 1, 1 )
#define MK_LEX_TYPE_RESERVED_MARK_OR		MK_MAKE_LEX_RESERVED_MARK( 0x10 - 6, 10, 1, 1 )
#define MK_LEX_TYPE_RESERVED_MARK_XOR		MK_MAKE_LEX_RESERVED_MARK( 0x10 - 6, 11, 1, 1 )
#define MK_LEX_TYPE_RESERVED_MARK_SAME		MK_MAKE_LEX_RESERVED_MARK( 0x10 - 11, 12, 1, 1 )
#define MK_LEX_TYPE_RESERVED_MARK_NOT_SAME	MK_MAKE_LEX_RESERVED_MARK( 0x10 - 8, 13, 1, 1 )
#define MK_LEX_TYPE_RESERVED_MARK_SE		MK_MAKE_LEX_RESERVED_MARK( 0x10 - 7, 14, 1, 1 )
#define MK_LEX_TYPE_RESERVED_MARK_BE		MK_MAKE_LEX_RESERVED_MARK( 0x10 - 7, 15, 1, 1 )
#define MK_LEX_TYPE_RESERVED_MARK_BIG		MK_MAKE_LEX_RESERVED_MARK( 0x10 - 7, 16, 1, 1 )
#define MK_LEX_TYPE_RESERVED_MARK_SMALL		MK_MAKE_LEX_RESERVED_MARK( 0x10 - 7, 17, 1, 1 )
#define MK_LEX_TYPE_RESERVED_MARK_EQUAL		MK_MAKE_LEX_RESERVED_MARK( 0x10 - 8, 18, 0, 1 )
#define MK_LEX_TYPE_RESERVED_MARK_DOT		MK_MAKE_LEX_RESERVED_MARK( 0x10 - 1, 19, 0, 0 )
#define MK_LEX_TYPE_RESERVED_MARK_PARENTHESIS		MK_MAKE_LEX_RESERVED_MARK( 0x10 - 1,	20, 0, 0 )	// (
#define MK_LEX_TYPE_RESERVED_MARK_END_PARENTHESIS	MK_MAKE_LEX_RESERVED_MARK( 0,			21, 0, 0 )	// )
#define MK_LEX_TYPE_RESERVED_MARK_BRACKET_REF		MK_MAKE_LEX_RESERVED_MARK( 0,			22, 0, 0 )  // [ reference.
#define MK_LEX_TYPE_RESERVED_MARK_BRACKET			MK_MAKE_LEX_RESERVED_MARK( 0x10 - 1,	23, 0, 0 )	// [
#define MK_LEX_TYPE_RESERVED_MARK_END_BRACKET		MK_MAKE_LEX_RESERVED_MARK( 0,			24, 0, 0 )	// ]
#define MK_LEX_TYPE_RESERVED_MARK_BRACE				MK_MAKE_LEX_RESERVED_MARK( 0x10 - 1,	25, 0, 0 )	// {
#define MK_LEX_TYPE_RESERVED_MARK_END_BRACE			MK_MAKE_LEX_RESERVED_MARK( 0,			26, 0, 0 )	// }

#define MK_LEX_TYPE_RESERVED_MARK_DBLATMARK		MK_MAKE_LEX_RESERVED_MARK( 0, 27, 0, 0 )
#define MK_LEX_TYPE_RESERVED_MARK_ATMARK		MK_MAKE_LEX_RESERVED_MARK( 0, 28, 0, 0 )

#define MK_LEX_TYPE_RESERVED_MARK_EXCLAMATION	MK_MAKE_LEX_RESERVED_MARK( 0, 29, 0, 0 )
#define MK_LEX_TYPE_RESERVED_MARK_QUESTION		MK_MAKE_LEX_RESERVED_MARK( 0, 30, 0, 0 )
#define MK_LEX_TYPE_RESERVED_MARK_CORON			MK_MAKE_LEX_RESERVED_MARK( 0, 31, 0, 0 )
#define MK_LEX_TYPE_RESERVED_MARK_DOUBLE_QUATE	MK_MAKE_LEX_RESERVED_MARK( 0, 32, 0, 0 )
#define MK_LEX_TYPE_RESERVED_MARK_SINGLE_QUATE	MK_MAKE_LEX_RESERVED_MARK( 0, 33, 0, 0 )
#define MK_LEX_TYPE_RESERVED_MARK_CONMA			MK_MAKE_LEX_RESERVED_MARK( 0, 34, 0, 0 )
#define MK_LEX_TYPE_RESERVED_MARK_CHILDA		MK_MAKE_LEX_RESERVED_MARK( 0, 35, 0, 0 )
#define MK_LEX_TYPE_RESERVED_MARK_SHARP			MK_MAKE_LEX_RESERVED_MARK( 0, 36, 0, 0 )

#define MK_SIZEOF_ENABELD_OPELATORS	27

// hashtable

// key style.
#define MK_TYPE_ATTRIBUTE_HASHKEY_MASK					0x0c000000
#define MK_TYPE_ATTRIBUTE_HASH_KEYTYPE(flag)			( (flag) & MK_TYPE_ATTRIBUTE_HASHKEY_MASK )
#define MK_TYPE_ATTRIBUTE_HASHKEY_STRING				0x00000000
#define MK_TYPE_ATTRIBUTE_HASHKEY_FREEABLE_STRING		0x04000000
#define MK_TYPE_ATTRIBUTE_HASHKEY_INTPTR				0x08000000
#define MK_TYPE_ATTRIBUTE_HASHKEY_FREEABLE_PTR			0x0C000000

// value style.
#define MK_TYPE_ATTRIBUTE_HASH_SIZE_EXTEND_MASK			0x03000000
#define MK_TYPE_ATTRIBUTE_HASH_SIZE_EXTEND_VALUE(flag)	( ( (flag) & MK_TYPE_ATTRIBUTE_HASH_SIZE_EXTEND_MASK ) >> 24 ) 
#define MK_TYPE_ATTRIBUTE_HASH_SIZE_EXTEND_FLAG(value)	( ( (value) << 24 ) & MK_TYPE_ATTRIBUTE_HASH_SIZE_EXTEND_MASK )

#define MK_TYPE_ATTRIBUTE_HASH_SIZE_MAX_VALUE			4

#define MK_TYPE_ATTRIBUTE_HASH_VALUETYPE_MASK				0x00ff0000
#define MK_TYPE_ATTRIBUTE_HASH_VALUETYPE_FLAG(index,value)	( ( value & 3 ) << ( 16 + ( index * 2 ) ) )
#define MK_TYPE_ATTRIBUTE_HASH_VALUETYPE_VALUE(index,flag)	( ( flag >> (16 + ( index * 2 ) ) ) & 3 )

#define MK_TYPE_ATTRIBUTE_HASH_VALUE_NONE_PTR				0
#define MK_TYPE_ATTRIBUTE_HASH_VALUE_FREEABLE_PTR			1
#define MK_TYPE_ATTRIBUTE_HASH_VALUE_GC_PTR					2
#define MK_TYPE_ATTRIBUTE_HASH_VALUE_MK_OBJECT_PTR			3

#define MK_TYPE_ATTRIBUTE_HASH_SIZE_GROW_MASK			0x0000ffc0	// 0 ~ 1023
#define MK_TYPE_ATTRIBUTE_HASH_SIZE_GROW_VALUE(flag)	( ( (flag) & MK_TYPE_ATTRIBUTE_HASH_SIZE_GROW_MASK ) >> 6 )
#define MK_TYPE_ATTRIBUTE_HASH_SIZE_GROW_FLAG(value)	( ( (value) << 6 ) & MK_TYPE_ATTRIBUTE_HASH_SIZE_GROW_MASK )

#define MK_TYPE_ATTRIBUTE_HASH_FREEABLE_STRING_KEY_DEFAULT						\
	MK_TYPE_HASHTABLE |															\
	MK_TYPE_ATTRIBUTE_HASHKEY_FREEABLE_STRING |									\
	MK_TYPE_ATTRIBUTE_HASH_SIZE_GROW_FLAG( MK_SIZEOF_EXTEND_HASH_DEFAULT ) |	\
	MK_TYPE_ATTRIBUTE_HASH_SIZE_EXTEND_FLAG(1)

#define MK_TYPE_ATTRIBUTE_HASH_STRING_KEY_DEFAULT								\
	MK_TYPE_HASHTABLE |															\
	MK_TYPE_ATTRIBUTE_HASHKEY_STRING |											\
	MK_TYPE_ATTRIBUTE_HASH_SIZE_GROW_FLAG( MK_SIZEOF_EXTEND_HASH_DEFAULT ) |	\
	MK_TYPE_ATTRIBUTE_HASH_SIZE_EXTEND_FLAG(1)

#define MK_TYPE_ATTRIBUTE_HASH_INTPTR_KEY_DEFAULT								\
	MK_TYPE_HASHTABLE |															\
	MK_TYPE_ATTRIBUTE_HASHKEY_INTPTR |											\
	MK_TYPE_ATTRIBUTE_HASH_SIZE_GROW_FLAG( MK_SIZEOF_EXTEND_HASH_DEFAULT ) |	\
	MK_TYPE_ATTRIBUTE_HASH_SIZE_EXTEND_FLAG(1)

#define MK_TYPE_ATTRIBUTE_HASH_INTERPRETER_DEFAULT								\
	MK_TYPE_HASHTABLE |															\
	MK_TYPE_ATTRIBUTE_HASHKEY_INTPTR |											\
	MK_TYPE_ATTRIBUTE_HASH_SIZE_GROW_FLAG( MK_SIZEOF_EXTEND_HASH_DEFAULT ) |	\
	MK_TYPE_ATTRIBUTE_HASH_SIZE_EXTEND_FLAG(3) |								\
	MK_TYPE_ATTRIBUTE_HASH_VALUETYPE_FLAG(0, MK_TYPE_ATTRIBUTE_HASH_VALUE_GC_PTR ) |	\
	MK_TYPE_ATTRIBUTE_HASH_VALUETYPE_FLAG(1, MK_TYPE_ATTRIBUTE_HASH_VALUE_NONE_PTR) |	\
	MK_TYPE_ATTRIBUTE_HASH_VALUETYPE_FLAG(2, MK_TYPE_ATTRIBUTE_HASH_VALUE_NONE_PTR )

//vector
#define MK_TYPE_VECTOR_SIZE_EXTEND_MASK		0x000ffc00	// 0 ~ 1023
#define MK_TYPE_VECTOR_SIZE_EXTEND(flag)	((flag & MK_TYPE_VECTOR_SIZE_EXTEND_MASK)>>10)

#define MK_TYPE_ATTRIBUTE_VECTOR_STYLE(flag)	( (flag) & 0x0f000000 )
#define MK_TYPE_ATTRIBUTE_VECTOR_STYLE_MASK		0x0f000000
#define MK_TYPE_VECTOR_ARRAY_MANAGED_PTR		0x00000000	// need destroy_node
#define MK_TYPE_VECTOR_ARRAY_UNMANAGED_PTR		0x01000000	// need not destroy_node
#define MK_TYPE_VECTOR_ARRAY_INT8				0x02000000
#define MK_TYPE_VECTOR_ARRAY_INT16				0x03000000
#define MK_TYPE_VECTOR_ARRAY_INT32				0x04000000
#define MK_TYPE_VECTOR_ARRAY_INT64				0x05000000

#define MK_TYPE_VECTOR_SIZE_FIX					0x00800000
#define MK_TYPE_VECTOR_POINT_MEMORY_DIRECT		0x00400000


// gc bit
#define MK_TYPE_GC_MASk				0x00000003
#define MK_TYPE_GC( flag )			((flag) & MK_TYPE_GC_MASk )
#define MK_GC_OBJECT_USE			0x00000001
#define MK_GC_OBJECT_GC_BIT			0x00000002
#define MK_GC_CREATE_NEW_BIT		0x00000004
#define MK_GC_MANAGED_OBJECT		0x00000008

#define MK_GC_CLEAR_GC_BIT(flag)	((flag) &= ~MK_GC_OBJECT_GC_BIT )
#define MK_GC_SET_GC_BIT(flag)		((flag) |=  MK_GC_OBJECT_GC_BIT )
#define MK_GC_IS_SET_GC_BIT(flag)	((flag) &   MK_GC_OBJECT_GC_BIT )

#define MK_GC_FREE_OBJECT(flag)		((flag) &= ~MK_GC_OBJECT_USE )
#define MK_GC_ALLOCATE_OBJECT(flag)	((flag) |=  MK_GC_OBJECT_USE )
#define MK_GC_IS_ALLOCATED_OBJECT(flag)		((flag) & MK_GC_OBJECT_USE )

#define MK_GC_CLEAR_CN_BIT(flag)	((flag) &= ~MK_GC_CREATE_NEW_BIT )
#define MK_GC_SET_CN_BIT(flag)		((flag) |=  MK_GC_CREATE_NEW_BIT )
#define MK_GC_IS_SET_CN_BIT(flag)	((flag)  &   MK_GC_CREATE_NEW_BIT )

#define MK_GC_SET_MANAGED_BIT(flag)	((flag) |=  MK_GC_MANAGED_OBJECT )
#define MK_GC_IS_MANAGED_BIT(flag)	((flag)  &   MK_GC_MANAGED_OBJECT )

// lex
#define MK_LEX_TYPE_MASK			0xf0000000
#define MK_LEX_TYPE_RESERVED_SYMBOL	0x10000000
#define MK_LEX_TYPE_RESERVED_MARK	0x20000000
#define MK_LEX_TYPE_SYMBOL			0x30000000
#define MK_LEX_TYPE_MARK			0x40000000
#define MK_LEX_TYPE_INT_VALUE		0x50000000
#define MK_LEX_TYPE_FLOAT_VALUE		0x60000000
#define MK_LEX_TYPE_EOL				0x70000000
#define MK_LEX_TYPE_EOF				0x80000000
#define MK_LEX_TYPE_CHARCTER		0x90000000

#define MK_LEX_TYPE_INVALID			0xf0000000

#define MK_LEX_RESERVED_SYMBOL_INDEX( id )	\
	( id & 0x000000ff )
#define MK_LEX_RESERVED_SYMBOL_NAME( id )	\
	( MK_RESERVED_SYMBOL[ MK_LEX_RESERVED_SYMBOL_INDEX( id ) ].name )

#define MK_LEX_TYPE_RESERVED_SYMBOL_THIS	MK_LEX_TYPE_RESERVED_SYMBOL + 0
#define MK_LEX_TYPE_RESERVED_SYMBOL_CLASS	MK_LEX_TYPE_RESERVED_SYMBOL + 1
#define MK_LEX_TYPE_RESERVED_SYMBOL_SUPER	MK_LEX_TYPE_RESERVED_SYMBOL + 2
#define MK_LEX_TYPE_RESERVED_SYMBOL_OWNER	MK_LEX_TYPE_RESERVED_SYMBOL + 3
#define MK_LEX_TYPE_RESERVED_SYMBOL_DEF		MK_LEX_TYPE_RESERVED_SYMBOL + 4
#define MK_LEX_TYPE_RESERVED_SYMBOL_RETURN	MK_LEX_TYPE_RESERVED_SYMBOL + 5
#define MK_LEX_TYPE_RESERVED_SYMBOL_END		MK_LEX_TYPE_RESERVED_SYMBOL + 6
#define MK_LEX_TYPE_RESERVED_SYMBOL_IF		MK_LEX_TYPE_RESERVED_SYMBOL + 7
#define MK_LEX_TYPE_RESERVED_SYMBOL_THEN	MK_LEX_TYPE_RESERVED_SYMBOL + 8
#define MK_LEX_TYPE_RESERVED_SYMBOL_ELSEIF	MK_LEX_TYPE_RESERVED_SYMBOL + 9
#define MK_LEX_TYPE_RESERVED_SYMBOL_ELSE	MK_LEX_TYPE_RESERVED_SYMBOL + 10
#define MK_LEX_TYPE_RESERVED_SYMBOL_WHILE	MK_LEX_TYPE_RESERVED_SYMBOL + 11
#define MK_LEX_TYPE_RESERVED_SYMBOL_BREAK	MK_LEX_TYPE_RESERVED_SYMBOL + 12
#define MK_LEX_TYPE_RESERVED_SYMBOL_CONTINUE	MK_LEX_TYPE_RESERVED_SYMBOL + 13
#define MK_LEX_TYPE_RESERVED_SYMBOL_DO		MK_LEX_TYPE_RESERVED_SYMBOL + 14
#define MK_LEX_TYPE_RESERVED_SYMBOL_NEW		MK_LEX_TYPE_RESERVED_SYMBOL + 15
#define MK_LEX_TYPE_RESERVED_SYMBOL_NIL		MK_LEX_TYPE_RESERVED_SYMBOL + 16
#define MK_LEX_TYPE_RESERVED_SYMBOL_TRUE	MK_LEX_TYPE_RESERVED_SYMBOL + 17
#define MK_LEX_TYPE_RESERVED_SYMBOL_FALSE	MK_LEX_TYPE_RESERVED_SYMBOL + 18
#define MK_LEX_TYPE_RESERVED_SYMBOL_RAISE	MK_LEX_TYPE_RESERVED_SYMBOL + 19
#define MK_LEX_TYPE_RESERVED_SYMBOL_TRY		MK_LEX_TYPE_RESERVED_SYMBOL + 20
#define MK_LEX_TYPE_RESERVED_SYMBOL_CATCH	MK_LEX_TYPE_RESERVED_SYMBOL + 21
#define MK_LEX_TYPE_RESERVED_SYMBOL_FINALLY	MK_LEX_TYPE_RESERVED_SYMBOL + 22
#define MK_LEX_TYPE_RESERVED_SYMBOL_PUBLIC	MK_LEX_TYPE_RESERVED_SYMBOL + 23
#define MK_LEX_TYPE_RESERVED_SYMBOL_PROTECTED	MK_LEX_TYPE_RESERVED_SYMBOL + 24
#define MK_LEX_TYPE_RESERVED_SYMBOL_PRIVATE		MK_LEX_TYPE_RESERVED_SYMBOL + 25
#define MK_LEX_TYPE_RESERVED_SYMBOL_READ		MK_LEX_TYPE_RESERVED_SYMBOL + 26
#define MK_LEX_TYPE_RESERVED_SYMBOL_WRITE		MK_LEX_TYPE_RESERVED_SYMBOL + 27
#define MK_LEX_TYPE_RESERVED_SYMBOL_INITIALIZE	MK_LEX_TYPE_RESERVED_SYMBOL + 28
#define MK_LEX_TYPE_RESERVED_SYMBOL_BLOCK		MK_LEX_TYPE_RESERVED_SYMBOL + 29
#define MK_LEX_TYPE_RESERVED_SYMBOL_NAMESPACE	MK_LEX_TYPE_RESERVED_SYMBOL + 30
#define MK_LEX_TYPE_RESERVED_SYMBOL_IMPORT		MK_LEX_TYPE_RESERVED_SYMBOL + 31
#define MK_LEX_TYPE_RESERVED_SYMBOL_USING		MK_LEX_TYPE_RESERVED_SYMBOL + 32
#define MK_LEX_TYPE_RESERVED_SYMBOL_MODULE		MK_LEX_TYPE_RESERVED_SYMBOL + 33
#define MK_LEX_TYPE_RESERVED_SYMBOL_ME			MK_LEX_TYPE_RESERVED_SYMBOL + 34

#define MK_LEX_STRING_MARK					'\"'
#define MK_LEX_CHARACTER_MARK				'\''
#define MK_LEX_STRING_MARK_PTR				"\""
#define MK_LEX_CHARACTER_MARK_PTR			"\'"

// mk_trace
#define MK_TRACE_TYPE_MASK				0xf0000000
#define MK_TRACE_TYPE_ERROR				0x10000000
#define MK_TRACE_TYPE_WARNING			0x20000000
#define MK_TRACE_TYPE_INFORMATION		0x30000000
#define MK_TRACE_TYPE_DEBUG				0x40000000

#define MK_TRACE_LEVEL_MASK				0x0f000000

// error type
#define MK_ERROR_TYPE_MASK				0xf0000000
#define MK_ERROR_NUMBER_MASK			0x000000ff

#define MK_ERROR_TYPE(error)			( error & MK_ERROR_TYPE_MASK)
#define MK_ERROR_TYPE_INDEX(error)		(MK_ERROR_TYPE(error)>>28)
#define MK_ERROR_NUMBER(error)			(error & MK_ERROR_NUMBER_MASK)

#define MK_ERROR_TYPE_COMILE_ERROR		0x00000000
#define MK_ERROR_TYPE_LINK_ERROR		0x10000000
#define MK_ERROR_TYPE_VM_ERROR			0x20000000
#define MK_ERROR_TYPE_COMILE_WARNING	0x30000000
#define MK_ERROR_TYPE_LINK_WARNING		0x40000000
#define MK_ERROR_TYPE_VM_WARNING		0x50000000

//return code
#define MK_VM_EXECUTE_EXPR_RETURN_FAILED	0	// fatal error
#define MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP	1	// to next step
#define MK_VM_EXECUTE_EXPR_RETURN_RETURN	2	// return called
#define MK_VM_EXECUTE_EXPR_RETURN_BREAK		3	// break
#define MK_VM_EXECUTE_EXPR_RETURN_CONTINUE	4	// continue
#define MK_VM_EXECUTE_EXPR_THROW			5	// throw exception

// util
void mk_copy_string( MK_CHAR **dest, const MK_CHAR *target );

// hashtable
MK_HASHTABLE *mk_create_hashtable( unsigned int createFlag, unsigned int defaultSize );
MK_HASHTABLE *mk_allocate_vm_managed_hashtable( unsigned int createFlag, unsigned int defaultSize, MK_MANAGED_HASH_TABLE **managedHashTable );
void mk_destroy_hashtable_node( MK_HASHTABLE *target );
void mk_destroy_map_item( MK_MAP_ITEM *items, unsigned int sizeElem, unsigned int sizeItem );

void mk_set_extend_value_hashtable( MK_HASHTABLE *table, void *handle, int index, INT_PTR newValue );
INT_PTR mk_get_extend_value_hashtable( MK_HASHTABLE *table, void *handle, int index );

unsigned int mk_size_item_hashtable( MK_HASHTABLE *ptable );
unsigned int mk_size_item_value_hashtable( MK_HASHTABLE *table );

void* mk_insert_item_hashtable( MK_HASHTABLE *table, const MK_CHAR *key, void *value );

void* mk_is_key_hashtable( MK_HASHTABLE *table, const MK_CHAR *key );
void* mk_find_item_hashtable( MK_HASHTABLE *ptable, const MK_CHAR *key, void **value );
void* mk_find_item_pointer_hashtable( MK_HASHTABLE *ptable, const MK_CHAR *key, void **value );

void mk_remove_key_hashtable( MK_HASHTABLE *table, const MK_CHAR *key );

void* mk_enum_item_hashtable_begin( MK_HASHTABLE *ptable );
void* mk_enum_item_hashtable_next( MK_HASHTABLE *ptable, void *iterator, const MK_CHAR **key, void **value );

MK_SYM_CHAR *mk_get_symbol_name_ptr( MK_VM_STRUCT *vm, const MK_CHAR *target );

// memory pool
void *mk_allocate_memory_pool( MK_MEMORY_POOL **target, const void *ptr, unsigned int size );
int mk_is_ptr_in_memory_pool ( const void *ptr, MK_MEMORY_POOL *top );

// raise critical error
int mk_get_internal_error_message( MK_CHAR *name, int line, MK_CHAR *buffer, size_t size, int errorNo, va_list list );
MK_VM_FRAME_ITEM *mk_create_internal_error_object( MK_VM_STRUCT *vm, MK_CHAR *nameStream, int line, MK_VM_FRAME_ITEM *classException, int errorNo, va_list list );
int mk_raise_internal_error( MK_VM_STRUCT *vm, MK_CHAR *name, int line, int errorNo, ... );

// lexer
MK_LEXER * mk_create_lexer( MK_VM_STRUCT *vm);
void mk_prepare_lexer_for_file( MK_LEXER *lexer );
int mk_open_stream( MK_VM_STRUCT *vm, MK_LEXER *lexer, MK_CHAR *nameOfStream, MK_CHAR *target );
void mk_close_stream( MK_LEXER *lexer );
void mk_clear_lexer( MK_LEXER *lexer );

int mk_get_token( MK_VM_STRUCT *vm, MK_LEXER *lexer );
int mk_lex_getc( MK_LEXER *lexer );

// compile
// compile expression style flag.
#define MK_TYPE_EXPR_IF_CONDITION			1		// if expr then
#define MK_TYPE_EXPR_WHILE_CONDITION		2		// while expr \n
#define MK_TYPE_EXPR_DOWHILE_CONDITION		2		// end while expr \n
#define MK_TYPE_EXPR_METHOD_ARG				3		// function( expr, expr, ... )
#define MK_TYPE_EXPR_METHOD_DEFAULT_PARAM	4		// Method Parameters...
#define MK_TYPE_EXPR_ROOT_SEGMENT			5		// expr \n, expr if .. \n
#define MK_TYPE_EXPR_INNER_EXPR				6		// ... op ( expr ) op ...
#define MK_TYPE_EXPR_NONAME_EXPR			7		// { expr }
#define MK_TYPE_EXPR_NODE_PARAM_DEFAULT		8		// | param = expr, param = expr ... |
#define MK_TYPE_EXPR_ARRAY_DEFINITION		9		// [expr, expr, expr, ... ]
#define MK_TYPE_EXPR_ARRAY_INDEX			10		// val[expr]

// compile segment style flag.
#define MK_TYPE_SEGMENT_ROOT			1		// top level segment
#define MK_TYPE_SEGMENT_BLOCK			2		// function top level segment
#define MK_TYPE_SEGMENT_IF				3		// if block segmnet
#define MK_TYPE_SEGMENT_WHILE			4		// while block segment
#define MK_TYPE_SEGMENT_DOWHILE			5		// do while block segment
#define MK_TYPE_SEGMENT_TRY				6		// try .. segment
#define MK_TYPE_SEGMENT_CATCH			7		// catch arg ... segment
#define MK_TYPE_SEGMENT_CATCH_ELSE		8		// catch ... else ... segment
#define MK_TYPE_SEGMENT_FINALLY			9		// finally .. segment
#define MK_TYPE_SEGMENT_NONAME_BLOCK	10		// block ... end

// compile
int mk_do_compile( MK_VM_STRUCT *vm, MK_LEXER *lexer );
MK_NODE_EXPR *do_compile_expr( MK_VM_STRUCT *vm, MK_LEXER *lexer,  unsigned int state );
MK_NODE_EXPR *do_compile_parse_string( MK_VM_STRUCT *vm, MK_LEXER *lexer );

// link
int mk_link_register_class_static_instance(MK_VM_STRUCT *vm);

// dynamic code generate
int mk_register_class( MK_VM_STRUCT *vm, MK_CLASS *target );
int mk_register_variable( MK_VM_STRUCT *vm, MK_VARIABLE *targetVariable, MK_CLASS *targetClass );
int mk_register_variable_arg( MK_VM_STRUCT *vm, MK_VECTOR *targetArgs, MK_VARIABLE *arg );
MK_VARIABLE *mk_create_variable( MK_VM_STRUCT *vm, MK_CHAR *mk_name, unsigned int type, MK_NODE_EXPR *defaultValue );
MK_VARIABLE *mk_create_method( MK_VM_STRUCT *vm, MK_CHAR *name, unsigned int type, MK_VECTOR* arg, INT_PTR entryPoint );
MK_VARIABLE *mk_create_default_native_method( MK_VM_STRUCT *vm, MK_CHAR *name, unsigned int type, int sizeArg, INT_PTR entryPoint );
// dump object(for debug)
void dump_node( void *target, unsigned int rank );
void dump_stack(MK_VM_STRUCT *vm, int maxSize);

// vm
int mk_vm_initialize_object_variables( MK_VM_STRUCT *vm, MK_CLASS *pClass, MK_VM_FRAME_ITEM *target, int isStatic );
int mk_vm_initialize( MK_VM_STRUCT *vm );
void mk_register_internal_classes( MK_VM_STRUCT *vm );
int mk_vm_run( MK_VM_STRUCT *vm, int *ppResult );
int mk_execute_block( MK_VM_STRUCT *vm, unsigned int *target );
int mk_vm_frame_item_to_int32( MK_VM_STRUCT *vm, MK_VM_FRAME_ITEM *target );
MK_FLOAT mk_vm_frame_item_to_float( MK_VM_STRUCT *vm, MK_VM_FRAME_ITEM *target );
MK_VM_FRAME_ITEM *mk_vm_create_int32_frame_item( MK_VM_STRUCT *vm, int value );
MK_VM_FRAME_ITEM *mk_vm_create_float_frame_item( MK_VM_STRUCT *vm, MK_FLOAT value );
MK_VM_FRAME_ITEM *mk_vm_create_bool_frame_item( int isTrue );
int mk_vm_frame_item_is_true( MK_VM_FRAME_ITEM *target );
MK_VM_FRAME_ITEM *mk_vm_find_instance( MK_VM_STRUCT *vm, MK_VM_FRAME_ITEM *instance, MK_SYM_CHAR *className );

void mk_destroy_vm_frame_item( MK_VM_FRAME_ITEM *target );

#include "vm_stack.h"

MK_CLASS *mk_vm_get_class_by_name( MK_VM_STRUCT *vm, const MK_SYM_CHAR *pname );
MK_CLASS *mk_vm_get_class( MK_VM_STRUCT *vm, MK_VM_FRAME_ITEM *pTarget );
int mk_vm_call_method( MK_VM_STRUCT *vm, MK_VARIABLE *pMethod, MK_VM_FRAME *pOwnerFrame, int sizeVarArgs );

// special class method
int mk_object_new( MK_VM_STRUCT *vm, MK_VM_FRAME_ITEM *target, MK_VM_FRAME_ITEM **result );
int mk_reference_equal( struct TAG_MK_VM_STRUCT *vm, unsigned int varArgC );
MK_VM_FRAME_ITEM *mk_reference_to_value( MK_VM_STRUCT *vm, MK_VM_FRAME_ITEM *source );

int mk_node_invoke( struct TAG_MK_VM_STRUCT *vm, unsigned int varArgC );


// object create/destroy
void *mk_create_object( unsigned int nsType );
void mk_destroy_node( void *target );
MK_VM_FRAME *mk_create_vm_frame_object( MK_MANAGED_VM_FRAME_TABLE **frameTable );
MK_VM_FRAME_ITEM *mk_create_vm_frame_item_object( MK_MANAGED_VM_FRAME_ITEM_TABLE **itemTable );

// vector
void mk_destroy_vector_node( MK_VECTOR *target );
MK_VECTOR *mk_create_vector( unsigned int flag, unsigned int defaultSize, INT_PTR addressOfMemory );
MK_VECTOR *mk_allocate_vm_managed_vector( MK_MANAGED_VECTOR_TABLE **managedVectorTable, 
										 unsigned int flag, unsigned int defaultSize, INT_PTR addressOfMemory );
unsigned int mk_insert_at_vector( MK_VECTOR *target, unsigned int index, INT_PTR elem );
INT_PTR mk_set_at_vector( MK_VECTOR *ptarget, unsigned int index, INT_PTR newItem );
INT_PTR mk_get_at_vector( MK_VECTOR *ptarget, unsigned int index );


#define mk_get_item_ptr_vector( ptarget )				\
	( (const INT_PTR*)ptarget->items )

#define mk_trim_size_vector( ptarget, newSize )			\
{														\
	if ( ptarget->used > newSize )						\
		ptarget->used = newSize;						\
}

#define mk_push_vector( ptarget, elem )					\
	mk_insert_at_vector( ptarget, UINT_MAX, elem )

#define mk_size_vector( ptarget )						\
	( ptarget->used )

#define mk_pop_vector( ptarget )						\
	mk_get_at_vector( ptarget, --ptarget->used )

const MK_CHAR *symbol_int_to_string( unsigned int value );
const MK_CHAR *operation_int_to_string( unsigned int value );
MK_VARIABLE* mk_vm_find_method( MK_VM_STRUCT *vm, 
								 MK_VM_FRAME_ITEM *pOwner, const MK_SYM_CHAR *name, 
								 unsigned int sizeArgs );
MK_VM_FRAME_ITEM* mk_vm_find_variable( MK_VM_STRUCT *vm, 
								 MK_VM_FRAME_ITEM *pOwner, const MK_SYM_CHAR *name );
MK_VM_FRAME_ITEM* mk_vm_find_variable_reference( MK_VM_STRUCT *vm, 
								 MK_VM_FRAME_ITEM *pOwner, const MK_SYM_CHAR *name );

// gc
int mk_gc_run( MK_VM_STRUCT *vm, int force );

#define CLASS_INTERNAL_OBJECT		"Object"

#define CLASS_INTERNAL_DEBUG		"Debug"

#define CLASS_INTERNAL_NULL			"Null"

#define CLASS_INTERNAL_REFERENCE	"Reference"
#define CLASS_INTERNAL_KERNEL		"Kernel"

#define CLASS_INTERNAL_STRING		"String"

#define CLASS_INTERNAL_INTEGER		"Integer"
#define	CLASS_INTERNAL_FLOAT		"Float"

#define CLASS_INTERNAL_IO			"IO"
#define CLASS_INTERNAL_CONSOLE		"Console"

#define CLASS_INTERNAL_CONTAINER	"Container"
#define CLASS_INTERNAL_ARRAY		"Array"

#define CLASS_INTERNAL_NODE			"Node"

#define CLASS_INTERNAL_EXCEPTION	"Exception"
#define CLASS_INTERNAL_COMPILE_ERROR_EXCEPTION	"CompileErrorException"
#define CLASS_INTERNAL_VM_ERROR_EXCEPTION	"VmErrorException"
#define CLASS_INTERNAL_ARRAY_OUTOF_RANGE	"ArrayOutOfRangeException"
#define CLASS_INTERNAL_IO_EXCEPTION	"IOException"
#define CLASS_INTERNAL_INVALID_NAME_EXCEPTION "InvalidNameException"
#define CLASS_INTERNAL_INVALID_TYPE_EXCEPTION "InvalidTypeException"
#define CLASS_INTERNAL_INVALID_CONSTRAINT_EXCEPTION "InvalidConstraintException"

#define FUNCTIONNAME_ENTRYPOINT "mk_main"

#endif
