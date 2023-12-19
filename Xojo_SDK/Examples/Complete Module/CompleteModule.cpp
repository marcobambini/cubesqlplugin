// On Windows, when using Visual Studio, winheader++.h
// must always be included before including any other
// file.
#if WIN32
	#include "WinHeader++.h"
#endif

// Always include rb_plugin.h when you want to get access
// to any of the SDK functionality.  This header will also
// bring REALplugin.h with it.
#include "rb_plugin.h"

// Unlike classes, modules do not specify any backing storage.
// This is because a module does not have any instance data
// associated with it.  The module is simply a wrapper around
// a set of functionality; a namespace.
//
// Everything you define in a module automatically gets a
// public (not global) scope.  So from REALbasic, you will
// access the module members using ModuleName.ModuleMember
// syntax.

// This is where we put all of the forward declarations for
// functions needed by the plugin SDK.
static void PlayWithCat( REALstring toyName );
static REALstring PlayWithMoose( void );
static void DanceWithWolves( unsigned char numDances, REALstring msg );
static unsigned long long PuppyValueGetter( void );
static void PuppyValueSetter( unsigned long long val );
static REALstring PuppyNameGetter( void );
static void PuppyNameSetter( REALstring name );
static REALstring FolderItemExtensionGetter( REALobject file );
static void MessageBox(REALstring str) ;

static void MessageBox(REALstring str)
{
	typedef void (*FuncTy)(REALstring);
	FuncTy fp = (FuncTy)REALLoadFrameworkMethod("MsgBox(s As String)");
	if (fp) fp(str);
}

static RBInteger Ticks()
{
    typedef RBInteger (*FuncTy)();
    FuncTy fp = (FuncTy)REALLoadFrameworkMethod("Ticks() As Integer");
    if (fp) return fp();
    return 0;
}

// Defining properties in modules is new to RB2006r4.
//
// Define the properties which our module is going to expose.
// All module properties are computed properties in which
// we define the setter and getter manually.  This is because there
// is no way to use the FieldOffset macro to make use of the
// REALstandardGetter and REALstandardSetter helpers.
//
// TIP: If you want to make a read-only property, the you 
// can set the setter method for it to nil.
//
// TIP: If you want your module property to be accessible
// in a console project, be sure to set the REALconsoleSafe
// flag when defining the property.  Note that the property's
// type must also be console safe (so, for instance, you cannot
// expose a Sound property since the Sound class isn't console safe).
//
// NOTE: Module properties do not get any instane data passed into
// then like Class properties do.  So the signature of the getter is: 
//		DataType GetterMethodName( void );
// And the signature of the setter is:
//		void SetterMethodName( DataType value );
// So module properties behave the same as a shared class property.

REALproperty TestModuleProperties[] = {
	{ "", "PuppyName", "String", REALconsoleSafe | REALScopeGlobal, (REALproc)PuppyNameGetter, (REALproc)PuppyNameSetter },
	{ "", "PuppyValue", "UInt64", REALconsoleSafe, (REALproc)PuppyValueGetter, (REALproc)PuppyValueSetter },
};

// Now we are going to define the methods that we want our module
// to expose to the user.  We will define some simple methods which
// demonstrate how to receive parameters, as well as how to return
// data.
//
// TIP: If you want your module method  be accessible in a console 
// project, then be sure to specify the REALconsoleSafe flag when
// defining the method.  Note that all of the method parameters and 
// return type must be console safe as well.
//
// NOTE: You can extend intrinsic classes by using the extends
// keyword in the parameter list.  This means that the first parameter
// passed to your C function will be the object instance of the item
// being extended.
REALmethodDefinition TestModuleMethods[] = {
	{ (REALproc)PlayWithCat, REALnoImplementation, "PlayWithCat( toyName as String )", REALconsoleSafe },
	{ (REALproc)PlayWithMoose, REALnoImplementation, "PlayWithMoose() as String", REALconsoleSafe },
	{ (REALproc)DanceWithWolves, REALnoImplementation, "DanceWithWolves( num as Integer, msg as String )", REALconsoleSafe | REALScopeGlobal },
	{ (REALproc)FolderItemExtensionGetter, REALnoImplementation, "Extension( extends file as FolderItem ) as String", REALconsoleSafe | REALScopeGlobal },
};

// Constants are simple to define -- you express them via their
// declaration.
REALconstant TestModuleConstants[] = {
	{ "kNumberOfBones = 206" },
	{ "kMagicColor = &cFF00FF" },
	{ "kDefaultCatToyName = \"Catnip\"" },
	{ "kPi = 3.14159265", nil, REALScopeGlobal },
};

// Define the actual module itself.  This is the part that ties
// everything together.  Our module is going to define an example
// of how to do everything
REALmoduleDefinition TestModuleDefinition = {
	// This field specifies the current Plugin SDK version.  You 
	// should always set the value to kCurrentREALControlVersion.
	kCurrentREALControlVersion,

	// This field specifies the name of the class which will be
	// exposed to the user
	"TestModule",

	// Methods
	TestModuleMethods,

	// How many methods are there?
	//
	// TIP: You can use the sizeof operator to calculate this for
	// you automatically
	sizeof( TestModuleMethods ) / sizeof( REALmethodDefinition ),

	// Constants
	TestModuleConstants,
	sizeof( TestModuleConstants ) / sizeof( REALconstant ),

	// Properties
	TestModuleProperties,
	sizeof( TestModuleProperties ) / sizeof( REALproperty ),

	// Structures and Enums can also be defined in a module, but
	// it is currently futile to do so since there is no dataype
	// namespace support in REALbasic.  Instead, you need to register
	// the structure or enumeration globally.
};

static void ShowMsgBox( const char *msg )
{
	REALstring msgString = REALBuildString( msg, (int)strlen( msg ), kREALTextEncodingASCII );
	MessageBox( msgString );
	REALUnlockString( msgString );
}

static void PlayWithCat( REALstring toyName )
{
	// Our cat plays stupidly -- it simply echos back
	// the name of the toy it's playing with.
	MessageBox( toyName );
}

static REALstring PlayWithMoose( void )
{
	// When you play with a moose, you may or may not
	// survive.
	static bool bInited = false;
	if (not bInited) {
		bInited = true;
		::srand( (uint32_t)Ticks() );
	}

	// You have a 50/50 chance of making it.  But, the
	// moose is like 4000 lbs, so I don't know how you'll
	// manage to survive it...
	if (((double)::rand() / (double)RAND_MAX) < .5) {
		return REALBuildStringWithEncoding( "You survived the moose!", ::strlen( "You survived the moose!" ), kREALTextEncodingASCII );
	} else {
		return REALBuildStringWithEncoding( "The moose killed you...", ::strlen( "The moose killed you..." ), kREALTextEncodingASCII );
	}
}

static void DanceWithWolves( unsigned char numDances, REALstring msg )
{
	for (unsigned char i = 0; i < numDances; i++) {
		MessageBox( msg );
	}
}

// Because modules have no instance data, they need to store their
// properties in static data (assuming the data is to be persistent
// instead of computed).
static unsigned long long sPuppyValue;

static unsigned long long PuppyValueGetter( void )
{
	// Return our static data
	return sPuppyValue;
}

static void PuppyValueSetter( unsigned long long val )
{
	// Assign to our static our data
	sPuppyValue = val;
}

static REALstring sPuppyName;
static REALstring PuppyNameGetter( void )
{
	// When returning a string value, you should lock
	// your reference to it before returning it to the
	// user.  Forgetting to do so can cause the string
	// to be released by the caller, leaving you with a
	// dangling pointer.
	REALLockString( sPuppyName );

	return sPuppyName;
}

static void PuppyNameSetter( REALstring name )
{
	// When assigning a string reference that you wish
	// to keep, you must hold a reference to it.  But,
	// you can't forget to unlock and references you
	// were previously holding!

	// Release our current reference
	REALUnlockString( sPuppyName );

	// Store the value
	sPuppyName = name;

	// Grab a reference
	REALLockString( sPuppyName );
}

static REALstring FolderItemExtensionGetter( REALobject file )
{
	// We're going to extend the FolderItem class and provide
	// a new method called "Extension" which returns the file's
	// extension.

	// First, using the dynamic access APIs, let's find out whether
	// the item passed to us is a folder or a file.  If it's a folder,
	// then we know it has no valid extension.
	bool bIsFolder = false;
	if (not REALGetPropValueBoolean( file, "Directory", &bIsFolder )) {
		// We couldn't find the property, so it's not safe to proceed.
		ShowMsgBox( "Couldn't get the Directory field" );
		return nil;
	}

	// If we have a folder, then bail out
	if (bIsFolder) {
		ShowMsgBox( "It's a folder, not a file" );
		return nil;
	}

	// We have a file -- so get the file name itself
	REALstring fileName = nil;
	if (not REALGetPropValueString( file, "Name", &fileName )) {
		ShowMsgBox( "Couldn't get the Name field" );
		// Couldn't get the name, so it's not safe to proceed
		return nil;
	}

	// Now that we have the file's name, let's find the last
	// field in the file.  
	long (*fpSplit)( REALstring, REALstring ) = 
		(long (*)( REALstring, REALstring ))REALLoadFrameworkMethod( "CountFields( source as String, delim as String ) as String" );

	// Make sure we were able to load the Split method
	if (not fpSplit) {
		ShowMsgBox( "Couldn't get the Split method" );

		// Release the file name string
		REALUnlockString( fileName );
		// Bail out
		return nil;
	}
/*
	// Now call the split method using the name we've
	// retrieved.
	REALstring dot = REALBuildString( ".", 1 );
	REALstringArray fields = fpSplit( fileName, dot );

	// We no longer require the dot or the file name, so release those
	REALUnlockString( fileName );
	REALUnlockString( dot );

	// If we don't have an array, we're in trouble
	if (not fields) {
		::OutputDebugStr( L"Split returned nil" );
		return nil;
	}

	// Now we want to return the last index in the array
	return REALGetArrayString( fields, REALGetArrayUBound( fields ) );
*/
	return nil;
}

// The PluginEntry method is where you register all of the
// code items which your plugin exposes.
//
// TIP: A single plugin can expose multiple project items.
// For example, your plugin could expose three RB classes,
// one module and a global method.
void PluginEntry( void )
{
	// Since a module is simply a namespace, the module itself
	// is always considered console-safe.  However, you still 
	// have to mark individual code items within the module for
	// their console safety.

	// Register our module
	REALRegisterModule( &TestModuleDefinition );

}
