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
/*  0 */	"errorはありません。",

/*  1 */	"$1は不明です。",
/*  2 */	"$1は不明です。$2が必要です。",
/*  3 */	"$1の後に$2が見つかりました。$3が必要です。",

/*  4 */	"$1($2)は$3内で既に定義されています。",

/*  5 */	"$1(class)の継承関係が不正です。",
/*  6 */	"$1の基底クラス$2は未定義です。",

/*  7 */	"$1に対応する$2が見つかりません。",

/*  8 */	"$1の終端に$2が見つかりません。",

/*  9 */	"$1($2)の定義のコンパイル時にエラーが発生しました。",
/* 10 */	"$1($2)の実体のコンパイル時にエラーが発生しました。",
/* 11 */	"$1の解析中にエラーが発生しました。",

/* 12 */	"$1の途中で$2を見つけました。",

/* 13 */	"$1演算子の後の式が不正です。",
/* 14 */	"$1演算子の前の式が不正です。",

/* 15 */	"引数名$1が重複しています。",

/* 16 */	"$1の終端に$2が見つかりました。",

/* 17 */	"$1の書式が不正です。",
/* 18 */	"$1の後に続く$2が不正です。",

/* 19 */    "$1の解析中にエラーが発生しました。以降に続くトークンは$2, $3です。",

/* 20 */	"数値以外のトークン$1に符号が付加されました。",

/* 21 */	"不明な文字 $1 が見つかりました。",

/* 22 */	"$1がオープンできません。",

/* 23 */	"内部エラーです $1($2)。",

/* 24 */	"デフォルトパラメータが設定されていません。",

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
/*  0 */	"errorはありません。",

/*  1 */	"シンボル$1は$2内で定義されていません。",
/*  2 */	"クラス$1は定義されていません。",
/*  3 */	"$1に対応するwhile/do..whileブロックが見つかりません。",
/*  4 */	"メソッド$1に渡された引数が多すぎます。",
/*  5 */	"$1シンボル$2にアクセスすることはできません。",

/*  6 */	"内部エラーです。$1メソッド内でスタック位置が書き換えられました。",
/*  7 */	"内部エラーです。サポートされていない型$1($2)が呼び出されました。",

/*  8 */	"メンバとして使用できないオペランドが組み込まれました。",
/*  9 */	"メソッド/ブロック構造呼び出しのネストが深すぎます。",
/* 10 */	"final修飾されたオブジェクトへの代入はできません。",

/* 11 */	"内部エラーです。必要な内部クラスが見つかりません。",

/* 12 */	"左辺で指定されたオブジェクトに対し、$1を呼び出すことはできません。"
/* 13 */	"未定義の例外が発生しました。",

/* 14 */	"$1の型が一致しません",
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
