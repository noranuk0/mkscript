#include "mk.h"
#include <stdio.h>
#include <stdlib.h>

int mk_vm_native_method_direct_call( MK_VM_STRUCT *vm, FP_MK_NATIVE_METHOD entryPoint, unsigned int argc )
{
	return (entryPoint)(vm, argc);
}

#if 
// support __cdecl only.
unsigned int mk_native_method_call(  MK_VM_STRUCT *vm, 
									 MK_VARIABLE *pMethod, 
									 unsigned int sizeParam )
{
	int retCode = 0;
	MK_VM_STACK *stack = &vm->localStack;
	const unsigned int **ptrLocalStack = (const unsigned int **)(stack->current - 1);
	void *function = pMethod->entryPoint;
	MK_VM_FRAME_ITEM *result = NULL, **pResult = &result;

	sizeParam ++;								// for [this]

#ifdef __GNUC__
	// gnu c compiler (gcc)
	{
		// gcc
		asm volatile(
			"push %6			\t\n"	// push pResult
			"movl %3, %%eax		\t\n"	// eax <= ptrLocalStack
			"movl %2, %%edx		\t\n"	// ecx <= sizeParam
			"inc %%edx			\t\n"
			"1:					\t\n"
			"dec %%edx			\t\n"
			"jz 2f				\t\n"	// if( edx == 0 ) jmp 2f
			"push (%%eax)		\t\n"	// push arg.
			"sub $0x04, %%eax	\t\n"	// sub eax, MK_SIZEOF_PTR
			"jmp 1b				\t\n"
			"2:					\t\n"
			"push %5			\t\n"	// push [vm]
			"call *%4			\t\n"	// function( vm, arg1, arg2, ..., &result )
			"movl %%eax,%1		\t\n"	// retCode <= eax
			"movl %2, %%edx		\t\n"	// edx <= sizeParam
			"addl $0x02, %%edx	\t\n"
			"shl $0x02, %%edx	\t\n"	// 32bit:2 64bit:3
			"add %%edx, %%esp	\t\n"	// fix stack( need _cdecl call only. )
			:	"=m"(result), "=m"(retCode)												// output ope 
			:	"m"(sizeParam), "m"(ptrLocalStack),"m"(function), "m"(vm), "m"(pResult)	// input ope
			:	"%eax", "%ecx", "%edx", "memory"
		);
	}
#elif _MSC_VER
	// Microsoft Visual C++ Compiler.
	__asm
	{
		push pResult
		mov eax, ptrLocalStack
		mov edx, sizeParam
		inc edx
	label_a:
		dec edx
		jz label_b
		push [eax]
		sub eax, MK_SIZEOF_PTR
		jmp label_a
	label_b:
		push dword ptr[vm]
		call function
		mov retCode, eax
		mov edx, sizeParam
		add edx, 2
		shl edx, 2
		add esp, edx
	}
#endif

	mk_vm_trim_stack( stack, sizeParam );
	mk_vm_push_stack(
		&vm->localStack, 
		result );
	if( retCode == MK_VM_EXECUTE_EXPR_RETURN_RETURN )
		retCode = MK_VM_EXECUTE_EXPR_RETURN_NEXTSTEP;
	return retCode;
}
