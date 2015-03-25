#include "mk.h"


#if 1
// compile error.
const MK_CHAR *compileErrorMessage[] = 
{
/* 0 */ 	"There is no error. ", 

/* 1 */ 	"$1 is unidentified. ", 
/* 2 */ 	"$1 is unidentified. $2 is necessary. ", 
/* 3 */ 	"$2 was found after $1. $3 is necessary. ", 

/* 4 */ 	"$1 ($2) is already defined in $3. ", 

/* 5 */ 	"Succession relations of $1(class) are unjust. ", 
/* 6 */ 	"SuperClass $2 of $1 is undefined. ", 

/* 7 */ 	"$2 corresponding to $1 is not found. ", 

/* 8 */ 	"$2 is not found in the terminal of $1. ", 

/* 9 */ 	"An error occurred at the time of the compilation of the definition of $1 ($2). ", 
/*10 */ 	"An error occurred at the time of a compilation of the substance of $1 ($2). ", 
/*11 */ 	"An error occurred during analysis of $1. ", 

/*12 */ 	"I found $2 in the middle of $1. ", 

/*13 */ 	"A ceremony after the $1 operator is unjust. ", 
/*14 */ 	"A ceremony before the $1 operator is unjust. ", 

/*15 */ 	"Argument name $1 repeats. ", 

/*16 */ 	"$2 was found in the terminal of $1. ", 

/*17 */ 	"A format of $1 is unjust. ", 
/*18 */ 	"$2 to continue after $1 is dirty. ", 

/*19 */     "An error occurred during analysis of $1. The token following after that is $2, $3. ", 

/*20 */ 	"A mark was added to token $1 except the numerical value. ", 

/*21 */ 	"Unidentified letter $1 was found. ", 

/*22 */ 	"$1 : No such file. ", 

/*23 */ 	"There is internal error[$1:$2]. ",

/*24 */		"Not set to default parameters.",
};

#else
// compile error.
const MK_CHAR *compileErrorMessage[] = 
{
/*  0 */	"error�͂���܂���B",

/*  1 */	"$1�͕s���ł��B",
/*  2 */	"$1�͕s���ł��B$2���K�v�ł��B",
/*  3 */	"$1�̌��$2��������܂����B$3���K�v�ł��B",

/*  4 */	"$1($2)��$3���Ŋ��ɒ�`����Ă��܂��B",

/*  5 */	"$1(class)�̌p���֌W���s���ł��B",
/*  6 */	"$1�̊��N���X$2�͖���`�ł��B",

/*  7 */	"$1�ɑΉ�����$2��������܂���B",

/*  8 */	"$1�̏I�[��$2��������܂���B",

/*  9 */	"$1($2)�̒�`�̃R���p�C�����ɃG���[���������܂����B",
/* 10 */	"$1($2)�̎��̂̃R���p�C�����ɃG���[���������܂����B",
/* 11 */	"$1�̉�͒��ɃG���[���������܂����B",

/* 12 */	"$1�̓r����$2�������܂����B",

/* 13 */	"$1���Z�q�̌�̎����s���ł��B",
/* 14 */	"$1���Z�q�̑O�̎����s���ł��B",

/* 15 */	"������$1���d�����Ă��܂��B",

/* 16 */	"$1�̏I�[��$2��������܂����B",

/* 17 */	"$1�̏������s���ł��B",
/* 18 */	"$1�̌�ɑ���$2���s���ł��B",

/* 19 */    "$1�̉�͒��ɃG���[���������܂����B�ȍ~�ɑ����g�[�N����$2, $3�ł��B",

/* 20 */	"���l�ȊO�̃g�[�N��$1�ɕ������t������܂����B",

/* 21 */	"�s���ȕ��� $1 ��������܂����B",

/* 22 */	"$1���I�[�v���ł��܂���B",

/* 23 */	"�����G���[�ł� $1($2)�B",

/* 24 */	"�f�t�H���g�p�����[�^���ݒ肳��Ă��܂���B",

};
#endif

const MK_CHAR *linkErrorMessage[] = 
{
	"",
};

// vm error
const MK_CHAR *vmErrorMessage[] =
{
/* 0 */ 	"There is no error. ", 

/* 1 */ 	"Symbol $1 is not defined in $2. ", 
/* 2 */ 	"Class $1 is not defined. ", 
/* 3 */ 	"The \"while\" or \"do...while\" block corresponding to $1 is not found. ", 
/* 4 */ 	"There are too many arguments handed to method $1. ", 
/* 5 */ 	"cannot access $1 symbol $2. ", 

/* 6 */ 	"It is internal error. A stack position was renewed in a $1 method. ", 
/* 7 */ 	"It is internal error. Unsupported type [$2:$1] was called. ", 
/* 8 */ 	"The operand that I cannot use as a member was incorporated. ", 
/* 9 */ 	"The nest of the method / block structure call is too deep. ",
/*10 */     "Can not be assigned to the object with final type.",

/*11 */		"It is internal error. The class definition as needed is not found.",

/*12 */		"$1 can not be called for objects that are specified in the left side.",

/*13 */		"Undefined exception.",

/*14 */		"The type of $1 mismatch."
};

#if 0
// vm error
const MK_CHAR *vmErrorMessage[] =
{
/*  0 */	"error�͂���܂���B",

/*  1 */	"�V���{��$1��$2���Œ�`����Ă��܂���B",
/*  2 */	"�N���X$1�͒�`����Ă��܂���B",
/*  3 */	"$1�ɑΉ�����while/do..while�u���b�N��������܂���B",
/*  4 */	"���\�b�h$1�ɓn���ꂽ�������������܂��B",
/*  5 */	"$1�V���{��$2�ɃA�N�Z�X���邱�Ƃ͂ł��܂���B",

/*  6 */	"�����G���[�ł��B$1���\�b�h���ŃX�^�b�N�ʒu�������������܂����B",
/*  7 */	"�����G���[�ł��B�T�|�[�g����Ă��Ȃ��^$1($2)���Ăяo����܂����B",

/*  8 */	"�����o�Ƃ��Ďg�p�ł��Ȃ��I�y�����h���g�ݍ��܂�܂����B",
/*  9 */	"���\�b�h/�u���b�N�\���Ăяo���̃l�X�g���[�����܂��B",
/* 10 */	"final�C�����ꂽ�I�u�W�F�N�g�ւ̑���͂ł��܂���B",

/* 11 */	"�����G���[�ł��B�K�v�ȓ����N���X��������܂���B",

/* 12 */	"���ӂŎw�肳�ꂽ�I�u�W�F�N�g�ɑ΂��A$1���Ăяo�����Ƃ͂ł��܂���B"
/* 13 */	"����`�̗�O���������܂����B",

/* 14 */	"$1�̌^����v���܂���",
};
#endif

const MK_CHAR *compileWarningMessage[] = 
{
	"",
};

const MK_CHAR *linkWarningMessage[] = 
{
	"",
};

// vm error
const MK_CHAR *vmWarningMessage[] =
{
	"",
};
