// Win32ControlsPlugin.cpp
//
//	This control plugin sample demonstrates how to host a Win32 control
//	1. Unlike macOS and Linux the plugin control backing pane always exists on Windows
//	2. Because of this you will have to create your Win32 control in the Open event since
//		you will need to get the control handle of this and it will not be available until then
//	3. When you call CreateWindow, the parent will be the plugin control backing pane, which
//		can be obtained by REALGetControlHandle or via Dynamic API.
//	4. You are responsible for handling changed size and enabled state

#include "rb_plugin.h"
#include <commctrl.h>

static void DateControlDestructor(REALcontrolInstance control);
static void DateControlPaintOffscreen(REALcontrolInstance control, REALgraphics context);
static void DateControlOpenEvent(REALcontrolInstance control);
static void DateControlStateChanged(REALcontrolInstance control, uint32_t changedField);

static REALcontrolBehaviour DateControlBehaviour = {
	nullptr,	// Constructor
	DateControlDestructor,
	nullptr,	// Redraw
	nullptr,	// Click
	nullptr,	// MouseDrag
	nullptr,	// MouseUp
	nullptr,	// GotFocus
	nullptr,	// LostFocus
	nullptr,	// KeyDown
	DateControlOpenEvent,
	nullptr,	// Close
	nullptr,	// BackgroundIdle
	DateControlPaintOffscreen,	// This is called by the IDE to paint in the window editor
	nullptr,	// SpecialBackground
	nullptr,	// ConstantChanging
	nullptr,	// DroppedNewInstance
	nullptr,	// MouseEnter
	nullptr,	// MouseExit
	nullptr,	// MouseMove
	DateControlStateChanged,
};

struct DateControlData
{
	HWND controlHandle;
};

static REALcontrol DateControlDefinition = {
	kCurrentREALControlVersion,
	"Win32DateControl",
	sizeof(DateControlData),
	REALenabledControl | REALdesktopControl,
	0, 0,	// This is the BMP name, so the IDE will try and find 0.bmp in IDE Resources->Controls Palette folder in the RBX plugin
	120, 35,
	nullptr, 0,
	nullptr, 0,
	nullptr, 0,
	&DateControlBehaviour,
};

static void DateControlOpenEvent(REALcontrolInstance control)
{
	INITCOMMONCONTROLSEX icex = { 0 };
	icex.dwSize = sizeof(icex);
	icex.dwICC = ICC_DATE_CLASSES;
	::InitCommonControlsEx(&icex);

	ControlData(DateControlDefinition, control, DateControlData, data);

	Rect r;
	REALGetControlBounds(control, &r);

	RBInteger handle;
	if (REALGetPropValueInteger(control, "Handle", &handle)) {
		HWND parentHandle = (HWND)handle;

		data->controlHandle = ::CreateWindowW(
			DATETIMEPICK_CLASS,
			nullptr,
			WS_BORDER | WS_CHILD | WS_VISIBLE,
			0,
			0,
			r.right - r.left,
			r.bottom - r.top,
			parentHandle,
			nullptr,
			(HINSTANCE)::GetWindowLongPtrW(parentHandle, GWLP_HINSTANCE),
			nullptr);
	}
}

void DateControlDestructor(REALcontrolInstance control)
{
	ControlData(DateControlDefinition, control, DateControlData, data);
	::DestroyWindow(data->controlHandle);
}

void DateControlPaintOffscreen(REALcontrolInstance control, REALgraphics context)
{
	// Painting offscreen is window relative
	Rect r;
	REALGetControlBounds(control, &r);

	int width = r.right - r.left;
	int height = r.bottom - r.top;

	typedef void(*DrawRectFuncTy)(REALgraphics, RBInteger x, RBInteger y, RBInteger w, RBInteger h);
	DrawRectFuncTy DrawRect = (DrawRectFuncTy)REALLoadObjectMethod(context, "DrawRect( x As Integer, y As Integer, w As Integer, h As Integer )");

	typedef void(*DrawStringFuncTy)(REALgraphics, REALstring text, double x, double y, double wrap, RBBoolean condense);
	DrawStringFuncTy DrawString = (DrawStringFuncTy)REALLoadObjectMethod(context, "DrawString( str As String, x As double, y As double, wrap As double, condense As Boolean )");

	// Draw something
	REALSetPropValueColor((REALobject)context, "ForeColor", 0);
	if (DrawRect) DrawRect(context, r.left, r.top, width, height);

	static REALstring text = REALBuildStringWithEncoding("DateControl", strlen("DateControl"), kREALTextEncodingUTF8);
	if (DrawString) DrawString(context, text, r.left, r.top + 14, 0, false);
}

void DateControlStateChanged(REALcontrolInstance control, uint32_t changedField)
{
	if (changedField & kBoundsChanged) {
		Rect r;
		REALGetControlBounds(control, &r);

		ControlData(DateControlDefinition, control, DateControlData, data);
		::SetWindowPos(data->controlHandle, nullptr, 0, 0, r.right - r.left, r.bottom - r.top, SWP_NOMOVE);
	} else if (changedField & kEnabledChanged) {
		ControlData(DateControlDefinition, control, DateControlData, data);
		bool enabled;
		if (REALGetPropValueBoolean(control, "Enabled", &enabled)) {
			::EnableWindow(data->controlHandle, enabled);
		}
	}
}

void PluginEntry(void)
{
	REALRegisterControl(&DateControlDefinition);
}
