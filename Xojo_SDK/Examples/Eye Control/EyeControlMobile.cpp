// EyeControlMobile.cpp    |    Sep 08 2020
//
//  Mobile version of the EyeControl plugin

#include "rb_plugin.h"

static RBColor EyeColorGetter(REALcontrolInstance control, RBInteger unusedParam);
static void EyeColorSetter(REALcontrolInstance control, RBInteger unusedParam, RBColor newColor);
static void EyeClose(REALcontrolInstance control);
static void EyeOpen(REALcontrolInstance control);
static void EyeConstructor(REALcontrolInstance control);
static void EyePaint(REALcontrolInstance control, REALobject context);
static RBBoolean EyePressed(REALcontrolInstance control, REALobject position, REALobject pointerInfoArray);

static REALmobileControlBehaviour EyeBehaviour = {
	EyeConstructor,
	nullptr,
    nullptr,
	EyePaint,
    nullptr,
    EyePressed
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

static REALmobileControl EyeDefinition = {
	kCurrentREALControlVersion,
	"MobileEyeControl",
	sizeof(EyeControlData),
	REALenabledControl | REALcontrolIsHIViewCompatible,
	0, 0,	// This is the BMP name, so the IDE will try and find 0.bmp in IDE Resources->Controls Palette folder in the RBX plugin
	80, 50,
	EyeProperties, sizeof(EyeProperties) / sizeof(REALproperty),
	EyeMethods, sizeof(EyeMethods) / sizeof(REALmethodDefinition),
	EyeEvents, sizeof(EyeEvents) / sizeof(REALevent),
	&EyeBehaviour,
};


static void EyeConstructor(REALcontrolInstance control)
{
	MobileControlData(EyeDefinition, control, EyeControlData, data);
	data->EyeColor = 0xFFFFFF;	// White
	data->EyeClosed = false;
}

static RBColor EyeColorGetter(REALcontrolInstance control, RBInteger unusedParam)
{
    MobileControlData(EyeDefinition, control, EyeControlData, data);
	return data->EyeColor;
}

static void EyeColorSetter(REALcontrolInstance control, RBInteger unusedParam, RBColor newColor)
{
    MobileControlData(EyeDefinition, control, EyeControlData, data);
	data->EyeColor = newColor;
}

static void EyeClose(REALcontrolInstance control)
{
    MobileControlData(EyeDefinition, control, EyeControlData, data);
	data->EyeClosed = true;

	// Invalidate control
	typedef void (* InvalidateFuncTy)(REALcontrolInstance);
	InvalidateFuncTy Invalidate = (InvalidateFuncTy)REALLoadObjectMethod((REALobject)control, "Refresh()");

	Invalidate(control);
}

static void EyeOpen(REALcontrolInstance control)
{
    MobileControlData(EyeDefinition, control, EyeControlData, data);
	data->EyeClosed = false;

	// Invalidate control
	typedef void (* InvalidateFuncTy)(REALcontrolInstance);
	InvalidateFuncTy Invalidate = (InvalidateFuncTy)REALLoadObjectMethod((REALobject)control, "Refresh()");

	Invalidate(control);
}

static RBBoolean EyePressed(REALcontrolInstance control, REALobject position, REALobject pointerInfoArray)
{
    // Toggle eye closed
    MobileControlData(EyeDefinition, control, EyeControlData, data);
    data->EyeClosed = not data->EyeClosed;

    typedef void (* InvalidateFuncTy)(REALcontrolInstance);
    InvalidateFuncTy Invalidate = (InvalidateFuncTy)REALLoadObjectMethod((REALobject)control, "Refresh()");

    Invalidate(control);

    typedef void (*EventTy)(REALcontrolInstance);
    EventTy fp = NULL;
    if (data->EyeClosed) {
        fp = (EventTy)REALGetEventInstance(control, &EyeDefinition.events[kEyesClosed]);
    } else {
        fp = (EventTy)REALGetEventInstance(control, &EyeDefinition.events[kEyesOpened]);
    }

    if (fp) {
        fp(control);
        return true;
    }
    
    return false;
}

static void EyePaintHelper(REALcontrolInstance control, REALgraphics context, Rect r)
{
	int width = r.right - r.left;
	int height = r.bottom - r.top;

	if (width <= 0 || height <= 0) return;

    MobileControlData(EyeDefinition, control, EyeControlData, data);

	typedef void (* FillOvalFuncTy)(REALgraphics, double x, double y, double w, double h);
	FillOvalFuncTy FillOval = (FillOvalFuncTy)REALLoadObjectMethod(context, "FillOval( x As Double, y As Double, w As Double, h As Double )");

	int oneEyeWidth = width/2;
	// Eye border
	REALSetPropValueColor((REALobject)context, "DrawingColor", 0);
	FillOval(context, r.left, r.top, oneEyeWidth, height);
	FillOval(context, r.left + oneEyeWidth, r.top, oneEyeWidth, height);

	if (not data->EyeClosed) {
		// Eye fill
		REALSetPropValueColor((REALobject)context, "DrawingColor", data->EyeColor);
		FillOval(context, r.left + 2, r.top + 2, oneEyeWidth - 4, height - 4);
		FillOval(context, r.left + oneEyeWidth + 2, r.top + 2, oneEyeWidth - 4, height - 4);
	
		// Eye Pupil
		REALSetPropValueColor((REALobject)context, "DrawingColor", 0);
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
    Rect r = {0};
    
    double w = 0, h = 0;
    REALGetPropValueDouble((REALobject)context, "Width", &w);
    REALGetPropValueDouble((REALobject)context, "Height", &h);

    r.right = w;
    r.bottom = h;
    
	EyePaintHelper(control, context, r);
}

void RegisteriOSControl(void)
{
	REALRegisterMobileControl(&EyeDefinition);
}
