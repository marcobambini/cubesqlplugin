// EyeControlPlugin.cpp	|	Sep 08 2011
//
//	This control plugin sample demonstrates some fundamental principles:
//	1. The difference between drawing offscreen (like the IDE window editor) and drawing onscreen
//	2. Using REALpropInvalidate to invalidate implicitly
//	3. How to fire your own events
//	4. How to use dynamic API
//
//	For practise you can extend this plugin:
//	1. Add a EyePupilColor property
//	2. Add focus for this control
//	3. Add a background idle function to animate the pupils
//
// Document Author:	William Yu
// Document Contributor(s):
//
// Revision history:

// Enable if this platform supports building for iOS
#define SUPPORTS_IOS_TARGET 0

#include "rb_plugin.h"
#if SUPPORTS_IOS_TARGET
    #include "EyeControlMobile.h"
#endif

static RBColor EyeColorGetter(REALcontrolInstance control, RBInteger unusedParam);
static void EyeColorSetter(REALcontrolInstance control, RBInteger unusedParam, RBColor newColor);
static void EyeClose(REALcontrolInstance control);
static void EyeOpen(REALcontrolInstance control);
static void EyeConstructor(REALcontrolInstance control);
static void EyeDestructor(REALcontrolInstance control);
static void EyePaint(REALcontrolInstance control, REALgraphics context);
static RBBoolean EyeClick(REALcontrolInstance control, int x, int y, int modifiers);
static void EyePaintOffscreen(REALcontrolInstance control, REALgraphics context);
static void EyePaintHelper(REALcontrolInstance control, REALgraphics context, Rect r);

static REALcontrolBehaviour EyeBehaviour = {
	EyeConstructor,
	EyeDestructor,
	EyePaint,
	EyeClick,
	NULL,	// MouseDrag
	NULL,	// MouseUp
	NULL,	// GotFocus
	NULL,	// LostFocus
	NULL,	// KeyDown
	NULL,	// Open
	NULL,	// Close
	NULL,	// BackgroundIdle
	EyePaintOffscreen	// This is called by the IDE to paint in the window editor
};

static REALproperty EyeProperties[] = {
	{ "Appearance", "EyeColor", "Color", REALpropInvalidate, (REALproc)EyeColorGetter, (REALproc)EyeColorSetter }
};

static REALmethodDefinition EyeMethods[] = {
	{ (REALproc) EyeClose, REALnoImplementation, "Close" },
	{ (REALproc) EyeOpen, REALnoImplementation, "Open" },
};

static REALevent EyeEvents[] = {
	{ "EyesOpened()" },
	{ "EyesClosed()" }
};

// This is a convenience enum to identify which EyeEvent we're dealing with
enum EyeEventsEnum
{
	kEyesOpened,
	kEyesClosed
};

struct EyeControlData
{
	RBColor EyeColor; // color in RB color format (AARRGGBB)
	bool EyeClosed;
};

static REALcontrol EyeDefinition = {
	kCurrentREALControlVersion,
	"DesktopEyeControl",
	sizeof(EyeControlData),
	REALenabledControl | REALcontrolIsHIViewCompatible | REALdesktopControl,
	0, 0,	// This is the BMP name, so the IDE will try and find 0.bmp in IDE Resources->Controls Palette folder in the RBX plugin
	80, 50,
	EyeProperties, sizeof(EyeProperties) / sizeof(REALproperty),
	EyeMethods, sizeof(EyeMethods) / sizeof(REALmethodDefinition),
	EyeEvents, sizeof(EyeEvents) / sizeof(REALevent),
	&EyeBehaviour,
};


static void EyeConstructor(REALcontrolInstance control)
{
	ControlData(EyeDefinition, control, EyeControlData, data);
	data->EyeColor = 0xFFFFFF;	// White
	data->EyeClosed = false;
}

static void EyeDestructor(REALcontrolInstance control)
{
}

static RBColor EyeColorGetter(REALcontrolInstance control, RBInteger unusedParam)
{
	ControlData(EyeDefinition, control, EyeControlData, data);
	return data->EyeColor;
}

static void EyeColorSetter(REALcontrolInstance control, RBInteger unusedParam, RBColor newColor)
{
	ControlData(EyeDefinition, control, EyeControlData, data);
	data->EyeColor = newColor;
}

static void EyeClose(REALcontrolInstance control)
{
	ControlData(EyeDefinition, control, EyeControlData, data);
	data->EyeClosed = true;

	// Invalidate control
	typedef void (* InvalidateFuncTy)(REALcontrolInstance, RBBoolean eraseBackground);
	InvalidateFuncTy Invalidate = (InvalidateFuncTy)REALLoadObjectMethod((REALobject)control, "Refresh(immediately As Boolean = False)");

	if (Invalidate) Invalidate(control, true);
}

static void EyeOpen(REALcontrolInstance control)
{
	ControlData(EyeDefinition, control, EyeControlData, data);
	data->EyeClosed = false;

	// Invalidate control
	typedef void (* InvalidateFuncTy)(REALcontrolInstance, RBBoolean eraseBackground);
	InvalidateFuncTy Invalidate = (InvalidateFuncTy)REALLoadObjectMethod((REALobject)control, "Refresh(immediately As Boolean = False)");

	if (Invalidate) Invalidate(control, true);
}

static RBBoolean EyeClick(REALcontrolInstance control, int x, int y, int modifiers)
{
	// Toggle eye closed
	ControlData(EyeDefinition, control, EyeControlData, data);
	data->EyeClosed = not data->EyeClosed;

	// Invalidate control
	typedef void (* InvalidateFuncTy)(REALcontrolInstance, RBBoolean eraseBackground);
	InvalidateFuncTy Invalidate = (InvalidateFuncTy)REALLoadObjectMethod((REALobject)control, "Refresh(immediately As Boolean = False)");

	if (Invalidate) Invalidate(control, true);

	typedef void (*EventTy)(REALcontrolInstance);
	EventTy fp = NULL;
	if (data->EyeClosed) {
		fp = (EventTy)REALGetEventInstance(control, &EyeDefinition.events[kEyesClosed]);
	} else {
		fp = (EventTy)REALGetEventInstance(control, &EyeDefinition.events[kEyesOpened]);
	}

	if (fp) fp(control);

	return false;
}

static void EyePaintHelper(REALcontrolInstance control, REALgraphics context, Rect r)
{
	int width = r.right - r.left;
	int height = r.bottom - r.top;

	if (width <= 0 || height <= 0) return;

	ControlData(EyeDefinition, control, EyeControlData, data);

	typedef void (* FillOvalFuncTy)(REALgraphics, RBInteger x, RBInteger y, RBInteger w, RBInteger h);
	FillOvalFuncTy FillOval = (FillOvalFuncTy)REALLoadObjectMethod(context, "FillOval( x As Integer, y As Integer, w As Integer, h As Integer )");

	int oneEyeWidth = width/2;
	// Eye border
	REALSetPropValueColor((REALobject)context, "ForeColor", 0);
	FillOval(context, r.left, r.top, oneEyeWidth, height);
	FillOval(context, r.left + oneEyeWidth, r.top, oneEyeWidth, height);

	if (not data->EyeClosed) {
		// Eye fill
		REALSetPropValueColor((REALobject)context, "ForeColor", data->EyeColor);
		FillOval(context, r.left + 2, r.top + 2, oneEyeWidth - 4, height - 4);
		FillOval(context, r.left + oneEyeWidth + 2, r.top + 2, oneEyeWidth - 4, height - 4);
	
		// Eye Pupil
		REALSetPropValueColor((REALobject)context, "ForeColor", 0);
		FillOval(context, r.left + width/8, r.top + height/4, width/4, height/2);
		FillOval(context, r.left + oneEyeWidth + width/8, r.top + height/4, width/4, height/2);
	}
}

static void EyePaintOffscreen(REALcontrolInstance control, REALgraphics context)
{
	// Painting offscreen is window relative
	Rect r;
	REALGetControlBounds(control, &r);
	EyePaintHelper(control, context, r);
}

static void EyePaint(REALcontrolInstance control, REALgraphics context)
{
	// Painting onscreen is control relative
	Rect r;
	REALGetControlBounds(control, &r);
	// Offset so the bounds are control relative;
	r.right -= r.left;
	r.bottom -= r.top;
	r.left = 0;
	r.top = 0;
	EyePaintHelper(control, context, r);
}

void PluginEntry(void)
{
#if !TARGET_OS_IPHONE
	REALRegisterControl(&EyeDefinition);
#endif
#if SUPPORTS_IOS_TARGET
    RegisteriOSControl();
#endif
}
