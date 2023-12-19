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

// Generally, classes allocate storage for each instance
// of the class.  This storage can hold all of the information
// that an individual class instance will use.  We're going 
// to allocate storage for our class instances so that they
// can hold properties, etc.
struct TestClassData {
	// This string is the backing storage for the CatName
	// property that we expose to the user.
	REALstring mCatName;

	// This string is backing storage for the MooseName
	// property.  However, we won't be using the FieldOffset
	// macro to access it.
	REALstring mMooseName;

	// This field is going to hold our read-only MooseWeight
	// property.  Note that just because it's a read-only
	// property doesn't mean that you can't still use the
	// FieldOffset macro.
	unsigned long mMooseWeight;
};

// This is where we put all of the forward declarations for
// functions needed by the plugin SDK.
static void TestClassInitializer( REALobject instance );
static void TestClassFinalizer( REALobject instance );
static REALstring HumanNameGetter( REALobject instance );
static REALstring MooseNameGetter( REALobject instance );
static void MooseNameSetter( REALobject instance, long, REALstring value );
static void PlayWithCat( REALobject instance, REALstring toyName );
static REALstring PlayWithMoose( REALobject instance );
static REALstring FireActionEvent( REALobject instance, long i );
static void ThrowMonkey( REALobject, long i );
static unsigned long long PuppyValueGetter( void );
static void PuppyValueSetter( unsigned long long val );
static void DanceWithWolves( unsigned char numDances, REALstring msg );
static REALstring MyGetString( REALobject instance );
static void MsgBox( REALstring msg ) ;
static RBInteger Ticks();

// Define the properties which our class is going to expose.
// Some of the properties are going to be "automatic" in
// that we'll use the standard getter and setter for them.
// Other properties are going to be computed properties which
// we define the setter and getter manually.
//
// TIP: If you want to make a read-only property, the you 
// can set the setter method for it to nil.
//
// TIP: If you want your class property to be accessible
// in a console project, be sure to set the REALconsoleSafe
// flag when defining the property.  Note that the property's
// type must also be console safe (so, for instance, you cannot
// expose a Sound property since the Sound class isn't console safe).
//
// TIP: If you're going to use the REALstandardGetter or REALstandardSetter
// prodecures, then you can use the FieldOffset macro to automatically 
// specify which field in the class instance storage you're referring
// to.
REALproperty TestClassProperties[] = {
	{ "", "CatName", "String", REALconsoleSafe, REALstandardGetter, REALstandardSetter, FieldOffset( TestClassData, mCatName ) },
	{ "", "HumanName", "String", REALconsoleSafe, (REALproc)HumanNameGetter, nil },
	{ "", "MooseName", "String", REALconsoleSafe, (REALproc)MooseNameGetter, (REALproc)MooseNameSetter },
	{ "", "MooseWeight", "UInt32", REALconsoleSafe, REALstandardGetter, nil, FieldOffset( TestClassData, mMooseWeight ) },
};

// Now we are going to define the methods that we want our class
// to expose to the user.  We will define some simple methods which
// demonstrate how to receive parameters, as well as how to return
// data.
//
// TIP: If you want your class method  be accessible in a console 
// project, then be sure to specify the REALconsoleSafe flag when
// defining the method.  Note that all of the method parameters and 
// return type must be console safe as well.
REALmethodDefinition TestClassMethods[] = {
	{ (REALproc)PlayWithCat, REALnoImplementation, "PlayWithCat( toyName as String )", REALconsoleSafe },
	{ (REALproc)PlayWithMoose, REALnoImplementation, "PlayWithMoose() as String", REALconsoleSafe },
	{ (REALproc)ThrowMonkey, REALnoImplementation, "ThrowMonkey( i as Integer )", REALconsoleSafe },
	{ (REALproc)MyGetString, REALnoImplementation, "GetString() as String", REALconsoleSafe },
};

// The next code item we're going to work on is the event.  Your
// class can expose events for the REALbasic user to implement
// as a way for you to query the user for more information, or
// let the user know something of interest has happened.  For
// instance, you could be letting the user know that there is
// some new data available.  Or you could be asking the user to
// supply you with some data so that you can complete an operation.
//
// Events have no code that you need to write in the plugin.  When
// you wish to fire an event, you call REALGetEventInstance on the
// event you'd like to query.  If the user has implemented it, the
// call will return non-nil.  Otherwise it will return a function
// pointer that you can then use to call the user's event.
REALevent TestClassEvents[] = {
	{ "Action( i as Integer ) as String", },
};

// Constants are simple to define -- you express them via their
// declaration.
REALconstant TestClassConstants[] = {
	{ "kNumberOfBones as UInt32 = 206" },
	{ "kMagicColor as Color = &cFF00FF" },
	{ "kDefaultCatName as String = \"Pixel\"" },
	{ "kPi as Double = 3.14159265" },
};

// Now we're going to define shared properties for the class.  A 
// shared property is a property which doesn't have any class instance
// associated with it.  A reasonable way to visual a shared property
// is like a property of a module.  Because there's no instance
// associated with it, you have to treat the getter and setter
// procedures differently.  They do not get a REALobject passed
// to them, and they cannot (repeat: CANNOT) use the REALstandardGetter,
// REALstandardSetter or FieldOffset helpers.  You must specify
// the getter and setter methods yourself as demonstrated here.
REALproperty TestClassSharedProperties[] = {
	{ "", "PuppyValue", "UInt64", REALconsoleSafe, (REALproc) PuppyValueGetter, (REALproc)PuppyValueSetter },
};

// The last thing you can add to a class are shared methods.  Just
// like shared properties, shared methods do not have any instance
// data associated with them (you can think of them like module
// methods if you'd like).  So the method signatures for shared
// methods are going to be different from the signatures for
// instance methods in that there's not going to be a REALobject
// passed into the method.
REALmethodDefinition TestClassSharedMethods[] = {
	{ (REALproc)DanceWithWolves, REALnoImplementation, "DanceWithWolves( numDances as UInt8, msg as String )", REALconsoleSafe },
};

// Define the actual class itself.  This is the part that ties
// everything together.  Our class is going to define an example
// of how to do everything (with the exception of implementing
// events from the superclass; see the NOTE).
//
// NOTE: There are times when you may want to implement a
// class which is intrinsic to REALbasic, such as a Timer.
// in that case, you would want to specify a REALeventInstance
// which provides the event implementation (such as the 
// Action event from the Timer).
REALclassDefinition TestClassDefinition = {
	// This field specifies the current Plugin SDK version.  You 
	// should always set the value to kCurrentREALControlVersion.
	kCurrentREALControlVersion,

	// This field specifies the name of the class which will be
	// exposed to the user
	"TestClass",

	// If your class has a Super, you can set the super here.  For
	// instance, if you want your class to inherit from the 
	// REALbasic Dictionary class, you would put "Dictionary" here.
	// A value of nil specifies that this class has no super.
	nil,
	
	// This field specifies the size of the class instance storage
	// we defined above.
	sizeof( TestClassData ),

	// This field is reserved and should always be 0
	0,

	// If your class needs to initialize any of its instance
	// data, then you can specify an initializer method.
	//
	// TIP: You can specify the default values for your class instance
	// data by providing an Initializer method in the REALclassDefinition.
	(REALproc)TestClassInitializer,

	// If your class needs to finalize any of its instance
	// data, the you can specify a finalizer method.
	//
	// TIP: If you've got any object references (including string
	// references) stored in your class instance data, you should
	// specify a finalizer method which unlocks all of the
	// object references (so that you do not leak them).
	(REALproc)TestClassFinalizer,

	// Properties
	TestClassProperties,
	
	// How many properties are there?
	//
	// TIP: You can use the sizeof operator to calculate this for
	// you automatically
	_countof( TestClassProperties ),

	// Methods
	TestClassMethods,
	_countof( TestClassMethods ),

	// Events which the user implements
	TestClassEvents,
	_countof( TestClassEvents ),

	// Event instances, which we are skipping over.  Whenever
	// you want to skip over a field within the structure, you
	// can replace it with nil values, like this:
	nil, 
	0,

	// If the class implements any interfaces, then you can
	// list the interface names here (separate multiple names
	// with a comma, just like in REALbasic).
	nil,

	// The next two fields are for bindings, which are a mystery.
	nil,
	0,

	// Back to things which get used qith some frequency: Constants!
	TestClassConstants,
	_countof( TestClassConstants ),

	// The next field is for flags.  You don't have worry about
	// setting these.  There are helper methods for any flags
	// you'd like to set.
	0,

	// The next field defines shared properties.
	TestClassSharedProperties,
	_countof( TestClassSharedProperties ),

	// The final field defined shared methods.
	TestClassSharedMethods,
	_countof( TestClassSharedMethods ),
};

void MsgBox( REALstring msg )
{
	void (*REALMsgBox)( REALstring ) = (void (*)( REALstring ))REALLoadFrameworkMethod( "MsgBox( s as String )" );
	if (REALMsgBox) {
		REALMsgBox( msg );
	}
}

RBInteger Ticks()
{
    typedef RBInteger (*FuncTy)();
    FuncTy fp = (FuncTy)REALLoadFrameworkMethod("Ticks() As Integer");
    if (fp) return fp();
    return 0;
}

static REALstring HumanNameGetter( REALobject instance )
{
	// We need to get our class instance data from the object
	// we've been passed.
	ClassData( TestClassDefinition, instance, TestClassData, me );

	// We will always append this to the end of the
	// cat name
	const char *kOwner = "'s Owner";

	// Construct a string to return based on the CatName property
	long bufferSize =  REALStringLength(me->mCatName) + ::strlen( kOwner ) + 1;
	char *name = new char[ bufferSize ];
	
	// Clear the buffer out
	::memset( name, 0, bufferSize );

	// Copy the cat name into the buffer
	size_t tmp = REALStringLength(me->mCatName);
	::strncpy( name,  (char *)REALGetStringContents(me->mCatName,&tmp),  REALStringLength(me->mCatName));
	
	// Now copy the suffix
	::strncat( name, kOwner, ::strlen( kOwner ) );

	// Now build our return string
	REALstring ret = REALBuildStringWithEncoding( name, bufferSize, kREALTextEncodingASCII  );

	// Free the memory we've allocated, and return the
	// string object we just built
	delete [] name;
	return ret;
}

static REALstring MooseNameGetter( REALobject instance )
{
	// We need to get our class instance data from the object
	// we've been passed.
	ClassData( TestClassDefinition, instance, TestClassData, me );

	// Take the data we have, lock it, and return it to the caller
	REALLockString( me->mMooseName );

	return me->mMooseName;
}

static void MooseNameSetter( REALobject instance, long, REALstring value )
{
	// We need to get our class instance data from the object
	// we've been passed.
	ClassData( TestClassDefinition, instance, TestClassData, me );

	// Whenever you have an object reference, or a string reference,
	// you must always follow these general steps.  First, unlock the
	// old reference you're holding.  Failing to do so will cause you
	// to leak the old object.  Then, make the object assignment so that
	// you're now storing the new object referece.  Finally, lock the 
	// object reference, because you're holding on to it.
	//
	// If you are not going to hold onto the object reference (because
	// you only need it temporarily), then you do not have to do these
	// steps.  These steps are only for when you are going to store a
	// reference.

	// Unlock our old data
	REALUnlockString( me->mMooseName );

	// Store the new data
	me->mMooseName = value;

	// And lock it
	REALLockString( me->mMooseName );
}

static void PlayWithCat( REALobject instance, REALstring toyName )
{
	// Our cat plays stupidly -- it simply echos back
	// the name of the toy it's playing with.
	MsgBox( toyName );
}

static REALstring PlayWithMoose( REALobject instance )
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

static void ThrowMonkey( REALobject instance, long i )
{
	// The user wants to throw the monkey.  I'd say that's
	// a good indication that an Action has happened.  So
	// let's call the user's Action event.
	REALstring val = FireActionEvent( instance, i );

	// If the user returned something to us, then let's
	// display it
	if (val and REALStringLength(val))	MsgBox( val );

	// Since the user returned an object to us (a string
	// is an object, essentially), we want to unlock the
	// reference to it, otherwise it will leak.
	REALUnlockString( val );
}

static REALstring FireActionEvent( REALobject instance, long i )
{
	// You can use this helper function to fire the user's
	// Action event.  If the user doesn't implement the event,
	// then this function will return nil for you.  But if
	// the user does implement the event, the the function will
	// return the data the caller returns.

	// Try to load the function pointer for our method from
	// the object instance passed in.
	REALstring (*fpAction)(REALobject instance, long i) = 
		(REALstring (*)(REALobject, long))REALGetEventInstance( (REALcontrolInstance)instance, &TestClassEvents[ 0 ] );

	if (fpAction) {
		// The user has implemented the Action event, so now
		// we can call it and return the data.
		return fpAction( instance, i );
	} else {
		// The user didn't implement the event, so bail out
		return nil;
	}
}

// We're going to store our shared property data in a static
// variable here
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

static void DanceWithWolves( unsigned char numDances, REALstring msg )
{
	for (unsigned char i = 0; i < numDances; i++) {
		MsgBox( msg );
	}
}

static REALstring MyGetString( REALobject instance )
{
	// We need to get our class instance data from the object
	// we've been passed.
	ClassData( TestClassDefinition, instance, TestClassData, me );

	// Just return the cat's name as our string value
	REALLockString( me->mCatName );
	return me->mCatName;
}


// Initializes the data for our TestClass object
static void TestClassInitializer( REALobject instance )
{
	// Get the TestClassData from our object
	ClassData( TestClassDefinition, instance, TestClassData, me );

	// We're going to initialize the cat's name
	me->mCatName = REALBuildStringWithEncoding( "Pixel", 5, kREALTextEncodingASCII );
	me->mMooseName = nil;
	me->mMooseWeight = 4000;  // 4000 lbs is one big moose!
}

// Finalize the data for our TestClass object
static void TestClassFinalizer( REALobject instance )
{
	// Get the TestClassData from our object
	ClassData( TestClassDefinition, instance, TestClassData, me );

	// Be sure to clean up any object references which
	// we may have
	REALUnlockString( me->mCatName );
	REALUnlockString( me->mMooseName );
}


// The PluginEntry method is where you register all of the
// code items which your plugin exposes.
//
// TIP: A single plugin can expose multiple project items.
// For example, your plugin could expose three RB classes,
// one module and a global method.
void PluginEntry( void )
{

	// Since we want our class to be console safe,
	// we need to register it as such.  This sets
	// the mFlags field on the REALclassDefinition
	// properly.
	SetClassConsoleSafe( &TestClassDefinition );

	// Register our class
	REALRegisterClass( &TestClassDefinition );
}
