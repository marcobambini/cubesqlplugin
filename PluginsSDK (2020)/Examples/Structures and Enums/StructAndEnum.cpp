// Always include rb_plugin.h when you want to get access
// to any of the SDK functionality.  This header will also
// bring REALplugin.h with it.
#include "rb_plugin.h"
#include <string.h>

// Structures expose fields which define the memory layout
// of the structure.  These fields can be any type supported
// by REALbasic.  You expose the fields via the plugin in
// the same way that you epose them in REALbasic.
static const char *testStructFields[] = {
	"Field1 as Integer",
	"Field2 as Boolean",
	"Field3 as String * 255",
	"Field4 as TestEnum",
};

// Now that we have the fields defined, we can stuff
// them into the structure array itself.
static REALstructure testStructs[] = {
	{ "TestStructure", REALScopeGlobal, testStructFields, 4 },
};

// Enumerations also expose fields to the user in the
// form of named data types.  The behavior for enumerations
// defined in plugins is the same as the behavior for
// them defined in REALbasic itself.
static const char *testEnumFields[] = {
	"Apple = 2",
	"Orange",
	"Pear = 100",
};

// Now that we have the fields defined, we can stuff
// them into the enumeration array itself.
static REALenum testEnums[] = {
	{ "TestEnum", NULL, REALScopeGlobal, testEnumFields, 3 },
};

// We're going to demonstrate how to accept a structure
// as a parameter to a method.  Since a structure is
// simply a way to connotate a chunk of arbitrary memory,
// we can use a regular C struct as the declared parameter
// to the method.
typedef struct testStruct {
	  RBInteger Field1;
	  RBBoolean Field2;
	  char Field3[255];
	  RBInteger Field4;
} testStruct;

// Note that since we passed the structure ByRef, we
// will be getting a pointer to the structure here!
static void TestStructMethod( testStruct *test )
{
	test->Field1 = 12345;
	test->Field2 = true;
	strncpy( test->Field3, "Hello world!", sizeof(test->Field3) );
	test->Field4 = 100; // Pear
}

static void MessageBox(REALstring str)
{
	typedef void (*FuncTy)(REALstring);
	FuncTy fp = (FuncTy)REALLoadFrameworkMethod("MsgBox(s As String)");
	if (fp) fp(str);
}

static void TestStructureArrayMethod( REALarray structArray )
{
	long ubound = REALGetArrayUBound( structArray );

	for (long i = 0; i <= ubound; i++) {
		testStruct tmp = { 0 };
		REALGetArrayStructure( structArray, i, &tmp );

		REALstring str = REALBuildStringWithEncoding( tmp.Field3, (int)strnlen( tmp.Field3, sizeof(tmp.Field3) ), kREALTextEncodingASCII );
		MessageBox( str );
		REALUnlockString( str );
	}
}

static REALmethodDefinition testMethods[] = {
	{ (REALproc)TestStructMethod, REALnoImplementation, "TestStructMethod( ByRef test as TestStructure )", REALconsoleSafe | REALScopeGlobal },
	{ (REALproc)TestStructureArrayMethod, REALnoImplementation, "TestStructArrayMethod( test() as TestStructure )", REALconsoleSafe | REALScopeGlobal },
};

// Structures and enumerations live inside of a module,
// and so we will demonstrate how to declare them inside
// of the module.
static REALmoduleDefinition testModule = {
	kCurrentREALControlVersion,
	"TestStructEnumModule",
	testMethods,
	sizeof(testMethods) / sizeof(REALmethodDefinition),
	NULL,		// Constants
	0,			// Constant count
	NULL,		// Properties
	0,			// Property count
	testStructs,
	sizeof( testStructs ) / sizeof( REALstructure ),
	testEnums,
	sizeof( testEnums ) / sizeof( REALenum ),
};

void PluginEntry( void )
{
	REALRegisterModule( &testModule );
}
