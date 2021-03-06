#include "interpreter.h"
#include "scripthelpers.h"

void UnixSetupFunc()
{
}

void Ctest (struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs) 
{
    printf("test(%d)\n", Param[0]->Val->Integer);
    Param[0]->Val->Integer = 1234;
}

void Clineno (struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs) 
{
    ReturnValue->Val->Integer = Parser->Line;
}

/* list of all library functions and their prototypes */
struct LibraryFunction UnixFunctions[] =
{
    { Ctest,            "void test(int);" },
    { Clineno,          "int lineno();" },
    { gui_fpsCheck,     "int gui_fpsCheck();" },
    { gui_choice,       "int gui_choice(char* lineOne, char* lineTwo);" },
    { gui_warn,         "void gui_warn(char* lineOne, char* lineTwo);"},
    { gui_menu6x5,      "int gui_menu6x5(char* question, int options, char** labels, struct pkx* pokemon, int generation);" },
    { gui_menu20x2,     "int gui_menu20x2(char* question, int options, char** labels);" },
    { NULL,         NULL }
};

void PlatformLibraryInit(Picoc *pc)
{
    IncludeRegister(pc, "picoc_unix.h", &UnixSetupFunc, &UnixFunctions[0], "struct pkx { int species; int form; };");
}
