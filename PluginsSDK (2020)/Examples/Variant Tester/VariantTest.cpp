// On Windows, when using Visual Studio, winheader++.h
// must always be included before including any other
// file.
#if WIN32
	#include "WinHeader++.h"
#endif
#include "rb_plugin.h"

// All currencies are expressed in mils, so
// this is our base for our conversions
const long kCurrencyBase = 10000;

REALobject MakeNewVariantImp( void ) ;
void DisplayVariantImp( REALobject var ) ;
void RunVariantUnitTests( void ) ;
static void MessageBox(REALstring str) ;

// This is a helper class which wraps a REALstring object and
// handles all of the reference counting.  Very handy!
class SimpleString {
private:
	// The string data we're wrapping
	REALstring mData;

	// Don't allow empty SimpleStrings
	SimpleString();

public:
	// The only way to use the wrapper is to pass in a
	// constant string.  We'll build a REALstring out
	// of it and store it.
	SimpleString( const char *str ) { mData = REALBuildStringWithEncoding( str, (int)strlen( str ), kREALTextEncodingASCII ); }

	// When the class instance goes out of scope, the
	// destructor is automatically called, which then
	// destroys our string.
	~SimpleString()	{ REALUnlockString( mData ); }

	// Make life even easier by doing an automatic 
	// conversion to a REALstring.  This allows us to
	// pass a SimpleString to functions which take 
	// a REALstring.
	operator REALstring() const { return mData; }
};


static void MessageBox(REALstring str)
{
	typedef void (*FuncTy)(REALstring);
	FuncTy fp = (FuncTy)REALLoadFrameworkMethod("MsgBox(s As String)");
	if (fp) fp(str);
}

// The purpose of this function is to make a big 
// variant within the plugin (in this case, a UInt64),
// and return it to RB-land.  It demonstrates how to
// properly create a new variant within the plugin.
REALobject MakeNewVariantImp( void )
{
	// Make a variant which wraps a UInt64
	// and return it to the caller.  Note that
	// the caller is responsible for unlocking
	// the variant object.
	return REALNewVariantUInt64( 0xF0032AB10067CD78 );
}

// The purpose of this function is to display
// a variant's value as a string.  It demonstrates
// how to query a variant for its various type
// information (in this case, a string).
void DisplayVariantImp( REALobject var )
{
	// We'll stuff the string value into this variable
	REALstring str = nil;

	// Attempt to get the StringValue "property" of 
	// the variant object.  If it works, it'll stuff
	// the value into our str variable.
	if (REALGetPropValueString( var, "StringValue", &str )) {
		// Display the string value to the user
		MessageBox( str );

		// Release the string's memory so that we
		// don't cause a leak
		REALUnlockString( str );
	} else {
		// Uh oh, things have gone bad!  This is
		// most likely a bug in REALbasic, assuming
		// that the REALobject passed into the function
		// truly is a variant object.
		MessageBox( SimpleString( "Could not find a StringValue property" ) );
	}
}

// Handy macro for creating the various unit tests.  Saves
// on me having to type out the (essentially) same thing
// a bunch of different times.  Thank goodness for the
// stringify and macro abilities in C.  This is essentially
// the guts of what happens in the RunVariantUnitTests function.
#define	TYPETOUNITTEST( var, type, RBFromtStr, RBtypeStr, expectedResult, accessorMethod )		{							\
		type val = (type)0;																					\
		if (not accessorMethod( var, RBtypeStr, &val )) {													\
			MessageBox( SimpleString( RBFromtStr "->" RBtypeStr " failed to find property" ) );			\
		} else if (val != expectedResult) {																	\
			MessageBox( SimpleString( RBFromtStr "->" RBtypeStr " failed to convert properly" ) );		\
		}																									\
	}

void RunVariantUnitTests( void )
{
	// The purpose to this method is to run a set of unit
	// tests on variants to make sure their support is 
	// functional.
	REALobject var = nil;

	// Start off simple: make a variant, set it as a string
	// and then query it as an integer.
	var = REALNewVariantString( SimpleString( "12345" ) );

	// Now let's make sure the string we've set can be
	// queried as each of the various integer types
	TYPETOUNITTEST( var, RBUInt64, "String", "UInt64Value", 12345, REALGetPropValueUInt64 );
	TYPETOUNITTEST( var, RBInt64, "String","Int64Value", 12345, REALGetPropValueInt64 );
	TYPETOUNITTEST( var, int32_t, "String","Int32Value", 12345, REALGetPropValueInt32 ); // <<<<<<<<<<<<<<<<<<
	TYPETOUNITTEST( var, uint32_t, "String","UInt32Value", 12345, REALGetPropValueUInt32 );
	TYPETOUNITTEST( var, int16_t, "String","Int16Value", 12345, REALGetPropValueInt16 );
	TYPETOUNITTEST( var, uint16_t, "String","UInt16Value", 12345, REALGetPropValueUInt16 );
	TYPETOUNITTEST( var, char, "String","Int8Value", 57, REALGetPropValueInt8 );		// The value would be 57 due to data truncation
	TYPETOUNITTEST( var, unsigned char, "String", "UInt8Value", 57, REALGetPropValueUInt8 );	// Ditto
	TYPETOUNITTEST( var, REALcurrency, "String", "CurrencyValue", 12345 * kCurrencyBase, REALGetPropValueCurrency );
	TYPETOUNITTEST( var, double, "String", "DoubleValue", 12345.0, REALGetPropValueDouble );
	TYPETOUNITTEST( var, float, "String", "SingleValue", 12345.0, REALGetPropValueSingle );

	// Destroy our variant so we can start over
	REALUnlockObject( var );

	// Now do it all over again, this time testing a negative number
	var = REALNewVariantInteger( -6543 );
	TYPETOUNITTEST( var, RBUInt64, "Int32", "UInt64Value", 0xFFFFFFFFFFFFE671, REALGetPropValueUInt64 );
	TYPETOUNITTEST( var, RBInt64, "Int32","Int64Value", -6543, REALGetPropValueInt64 );
	TYPETOUNITTEST( var, int32_t, "Int32","Int32Value", -6543, REALGetPropValueInt32 ); // <<<<<<<<<<<<<<<<<<
	TYPETOUNITTEST( var, uint32_t, "Int32","UInt32Value", 0xFFFFE671, REALGetPropValueUInt32 );
	TYPETOUNITTEST( var, int16_t, "Int32","Int16Value", -6543, REALGetPropValueInt16 );
	TYPETOUNITTEST( var, uint16_t, "Int32","UInt16Value", 0xE671, REALGetPropValueUInt16 );
	TYPETOUNITTEST( var, char, "Int32","Int8Value", 113, REALGetPropValueInt8 );
	TYPETOUNITTEST( var, unsigned char, "Int32", "UInt8Value", 113, REALGetPropValueUInt8 );
	TYPETOUNITTEST( var, REALcurrency, "Int32", "CurrencyValue", -6543 * kCurrencyBase, REALGetPropValueCurrency );
	TYPETOUNITTEST( var, double, "Int32", "DoubleValue", -6543.0, REALGetPropValueDouble) ;
	TYPETOUNITTEST( var, float, "Int32", "SingleValue", -6543.0, REALGetPropValueSingle );

	// Destroy our variant so we can start over
	REALUnlockObject( var );

	// Now, convert from a floating-point to integer
	var = REALNewVariantDouble( 5555.0 );
	TYPETOUNITTEST( var, RBUInt64, "Double", "UInt64Value", 5555, REALGetPropValueUInt64 );
	TYPETOUNITTEST( var, RBInt64, "Double","Int64Value", 5555, REALGetPropValueInt64 );
	TYPETOUNITTEST( var, int32_t, "Double","Int32Value", 5555, REALGetPropValueInt32 ); // <<<<<<<<<<<<<<<<<<
	TYPETOUNITTEST( var, uint32_t, "Double","UInt32Value", 5555, REALGetPropValueUInt32 );
	TYPETOUNITTEST( var, int16_t, "Double","Int16Value", 5555, REALGetPropValueInt16 );
	TYPETOUNITTEST( var, uint16_t, "Double","UInt16Value", 5555, REALGetPropValueUInt16 );
	TYPETOUNITTEST( var, char, "Double","Int8Value", -77, REALGetPropValueInt8 );
	TYPETOUNITTEST( var, unsigned char, "Double", "UInt8Value", 179, REALGetPropValueUInt8 );
	TYPETOUNITTEST( var, REALcurrency, "Double", "CurrencyValue", 5555 * kCurrencyBase, REALGetPropValueCurrency );
	TYPETOUNITTEST( var, double, "Double", "DoubleValue", 5555.0, REALGetPropValueDouble );
	TYPETOUNITTEST( var, float, "Double", "SingleValue", 5555.0, REALGetPropValueSingle );

	// Destroy our variant so we can start over
	REALUnlockObject( var );

	// Make a new boolean value and test it
	var = REALNewVariantBoolean( true );
	char test = false;
	if (not REALGetPropValueInt8( var, "BooleanValue", &test )) {
		MessageBox( SimpleString( "Boolean->Boolean failed to find property" ) );
	} else if (test != true) {
		MessageBox( SimpleString( "Boolean->Boolean failed to convert properly" ) );
	}

	// Destroy our variant so we can start over
	REALUnlockObject( var );

	// Time to test currency (oooh!)
	var = REALNewVariantCurrency( 8000 * kCurrencyBase );
	TYPETOUNITTEST( var, RBUInt64, "Currency", "UInt64Value", 8000, REALGetPropValueUInt64 );
	TYPETOUNITTEST( var, RBInt64, "Currency","Int64Value", 8000, REALGetPropValueInt64 );
	TYPETOUNITTEST( var, int32_t, "Currency", "Int32Value", 8000, REALGetPropValueInt32 ); // <<<<<<<<<<<<<<<<<<
	TYPETOUNITTEST( var, uint32_t, "Currency","UInt32Value", 8000, REALGetPropValueUInt32 );
	TYPETOUNITTEST( var, int16_t, "Currency","Int16Value", 8000, REALGetPropValueInt16 );
	TYPETOUNITTEST( var, uint16_t, "Currency","UInt16Value", 8000, REALGetPropValueUInt16 );
	TYPETOUNITTEST( var, char, "Currency","Int8Value", 64, REALGetPropValueInt8 );
	TYPETOUNITTEST( var, unsigned char, "Currency", "UInt8Value", 64, REALGetPropValueUInt8 );
	TYPETOUNITTEST( var, REALcurrency, "Currency", "CurrencyValue", 8000 * kCurrencyBase, REALGetPropValueCurrency );
	TYPETOUNITTEST( var, double, "Currency", "DoubleValue", 8000.0, REALGetPropValueDouble );
	TYPETOUNITTEST( var, float, "Currency", "SingleValue", 8000.0, REALGetPropValueSingle );

	// Destroy our variant so we can start over
	REALUnlockObject( var );

}

REALmethodDefinition myModuleMethods[] = {
	{ (REALproc)RunVariantUnitTests, REALnoImplementation, "RunVariantUnitTests()", REALconsoleSafe| REALScopeGlobal },
	{ (REALproc)MakeNewVariantImp, REALnoImplementation, "MakeNewVariant() as Variant", REALconsoleSafe | REALScopeGlobal},
	{ (REALproc)DisplayVariantImp, REALnoImplementation, "DisplayVariant( v as Variant )", REALconsoleSafe | REALScopeGlobal },
};

REALmoduleDefinition myModule = {

	kCurrentREALControlVersion,
	"myModule",			// name of the module
	// Methods
	myModuleMethods,

	// How many methods are there?
	//
	// TIP: You can use the sizeof operator to calculate this for
	// you automatically
	sizeof( myModuleMethods ) / sizeof( REALmethodDefinition )
} ;

void PluginEntry( void )
{
	REALRegisterModule(&myModule);

}
