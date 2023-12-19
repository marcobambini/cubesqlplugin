// EyeControlWinUIPlugin.cpp	|	May 11 2023
//
//	This is the WinUI version of the EyeControl plugin.  Note that the EyeDefinition
//  structure must match the structure of the same in EyeControl.cpp
//	1. This demonstrates how to embed XAML Controls using XAML Islands.
//  2. Because Xojo already bundles the necessary DLLs to make this all
//     work, you do not need to provide any additional resources, however
//     you must enable "Use WinUI Framework" in the Windows advanced
//     build settings.
//  3. Events are handled directly by the XAML control, see HookupXamlEvents.
//
//  Notes: some details and event handling may change in future iterations.

#include "rb_plugin.h"

#include <functional>
#include <winrt/base.h>
#include <Windows.UI.Xaml.Hosting.DesktopWindowXamlSource.h>
#include <winrt/Microsoft.Toolkit.Win32.UI.XamlHost.h>
#include <winrt/Microsoft.UI.Xaml.Controls.h>
#include <winrt/Microsoft.UI.Xaml.XamlTypeInfo.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/windows.ui.xaml.controls.h>
#include <winrt/windows.ui.xaml.controls.primitives.h>
#include <winrt/windows.ui.xaml.hosting.h>
#include <winrt/windows.ui.xaml.input.h>
#include <winrt/windows.ui.xaml.markup.h>
#include <winrt/windows.ui.xaml.media.h>
#include <winrt/windows.ui.xaml.media.imaging.h>
#include <winrt/windows.ui.xaml.shapes.h>

using namespace winrt;
using namespace Windows::UI;
using namespace Windows::UI::Composition;
using namespace Windows::UI::Xaml::Hosting;

static RBColor EyeColorGetter(REALcontrolInstance control, RBInteger unusedParam);
static void EyeColorSetter(REALcontrolInstance control, RBInteger unusedParam, RBColor newColor);
static void EyeClose(REALcontrolInstance control);
static void EyeOpen(REALcontrolInstance control);
static void EyeConstructor(REALcontrolInstance control);
static void EyeDestructor(REALcontrolInstance control);
static void EyePaintOffscreen(REALcontrolInstance control, REALgraphics context);
static void EyePaintHelper(REALcontrolInstance control, REALgraphics context, Rect r);
static void EyeOpeningEvent(REALcontrolInstance control);
static void EyeStateChanged(REALcontrolInstance control, uint32_t changedField);
static void SetupXamlEye(REALcontrolInstance control, bool isOpen);
static void HookupXamlEvents(const Windows::Foundation::IInspectable& ins, REALcontrolInstance control);


static REALcontrolBehaviour EyeBehaviour = {
	EyeConstructor,
	EyeDestructor,
	NULL,
	NULL,
	NULL,	// MouseDrag
	NULL,	// MouseUp
	NULL,	// GotFocus
	NULL,	// LostFocus
	NULL,	// KeyDown
	EyeOpeningEvent,	// Open
	NULL,	// Close
	NULL,	// BackgroundIdle
	EyePaintOffscreen,	// This is called by the IDE to paint in the window editor
	NULL,	// ConstantChanging
	NULL,	// DroppedNewInstance
	NULL,	// MouseEnterFunction
	NULL,	// MouseExitFunction
	NULL,	// MouseMoveFunction
	NULL,	// SetSpecialBackground
	EyeStateChanged,	// stateChangedFunction
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

class XamlData
{
  public:
	DesktopWindowXamlSource DesktopSource;
};

// Create a map of each element of the EyeControl that we want to handle events for.
static std::map<Xaml::UIElement, REALcontrolInstance> EventMap;

void CleanEventMap(REALcontrolInstance control)
{
	for (auto it = EventMap.begin(); it != EventMap.end();) {
		if (it->second == control) {
			EventMap.erase(it++);
		} else {
			++it;
		}
	}
}

struct EyeControlData
{
	RBColor EyeColor; // color in RB color format (AARRGGBB)
	bool EyeClosed;

	XamlData* Xaml;
};

static REALcontrol EyeDefinition = {
	kCurrentREALControlVersion,
	"DesktopEyeControl",
	sizeof(EyeControlData),
	REALenabledControl | REALcontrolIsHIViewCompatible | REALdesktopControl | REALxamlControl,
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
	// Since 2023r3 we require any Xaml creation to happen after construction because
	// of the sequence in which we initialize Xaml (mainly so that we can continue to
	// support IME in people's apps alongside Xaml).
	// The ideal creation sequence would be the Opening event, see EyeOpeningEvent.
	data->Xaml = nullptr;
}

static void EyeDestructor(REALcontrolInstance control)
{
	ControlData(EyeDefinition, control, EyeControlData, data);
	CleanEventMap(control);
	if (data->Xaml) delete data->Xaml;
}

static void HookupXamlEvents(const Windows::Foundation::IInspectable& ins, REALcontrolInstance control)
{
	ControlData(EyeDefinition, control, EyeControlData, data);
	CleanEventMap(control);

	for (auto element : ins.as<Xaml::Controls::Grid>().Children()) {
		EventMap[element] = control;
		element.PointerPressed([&](winrt::Windows::Foundation::IInspectable const& sender, Xaml::Input::PointerRoutedEventArgs const& args)
			{
				// Toggle eye closed/opened.
				REALcontrolInstance control = EventMap[sender.as<Xaml::UIElement>()];
				if (!control) return;

				ControlData(EyeDefinition, control, EyeControlData, data);
				data->EyeClosed = not data->EyeClosed;

				typedef void (*EventTy)(REALcontrolInstance);
				EventTy fp = NULL;
				if (data->EyeClosed) {
					SetupXamlEye(control, false);
					fp = (EventTy)REALGetEventInstance(control, &EyeDefinition.events[kEyesClosed]);
				} else {
					SetupXamlEye(control, true);
					fp = (EventTy)REALGetEventInstance(control, &EyeDefinition.events[kEyesOpened]);
				}

				if (fp) fp(control);
			});
	}
}

static void SetupXamlEye(REALcontrolInstance control, bool isOpen)
{
	const wchar_t* kEyesOpen = L"<Grid \
		xmlns=\"http://schemas.microsoft.com/winfx/2006/xaml/presentation\" \
		xmlns:x=\"http://schemas.microsoft.com/winfx/2006/xaml\"> \
		<Viewbox Stretch='Uniform'>\
			<Grid Width='100' Height='50'>\
				<Ellipse Name='Eye' Width='40' Height='40' Fill='White' Stroke='Black' StrokeThickness='2' HorizontalAlignment='Left' VerticalAlignment='Center' Margin='10,0,0,0' />\
				<Ellipse Name='Pupil' Width='20' Height='20' Fill='Black' HorizontalAlignment='Left' VerticalAlignment='Center' Margin='10,0,0,0' />\
				<Ellipse Name='Eye' Width='40' Height='40' Fill='White' Stroke='Black' StrokeThickness='2' HorizontalAlignment='Right' VerticalAlignment='Center' Margin='0,0,10,0' />\
				<Ellipse Name='Pupil' Width='20' Height='20' Fill='Black' HorizontalAlignment='Right' VerticalAlignment='Center' Margin='0,0,10,0' />\
			</Grid>\
		</Viewbox>\
		</Grid>";

	const wchar_t* kEyesClosed = L"<Grid \
		xmlns=\"http://schemas.microsoft.com/winfx/2006/xaml/presentation\" \
		xmlns:x=\"http://schemas.microsoft.com/winfx/2006/xaml\"> \
		<Viewbox Stretch='Uniform'>\
			<Grid Width='100' Height='50'>\
				<Ellipse Name='Eye1' Width='40' Height='40' Fill='Black' Stroke='Black' StrokeThickness='2' HorizontalAlignment='Left' VerticalAlignment='Center' Margin='10,0,0,0' />\
				<Ellipse Name='Eye2' Width='40' Height='40' Fill='Black' Stroke='Black' StrokeThickness='2' HorizontalAlignment='Right' VerticalAlignment='Center' Margin='0,0,10,0' />\
			</Grid>\
		</Viewbox>\
		</Grid>";

	Windows::Foundation::IInspectable ins = Xaml::Markup::XamlReader::Load(isOpen ? kEyesOpen: kEyesClosed);

	static auto FillColorFunc = reinterpret_cast<RBColor(*)()>(REALLoadFrameworkMethod("Color.FillColor() As Color"));
	if (FillColorFunc) {
		RBColor c = FillColorFunc();;

		uint8_t red = (c & 0x00FF0000) >> 16;
		uint8_t green = c & 0x0000FF00 >> 8;
		uint8_t blue = c & 0x000000FF;

		ins.as<Xaml::Controls::Grid>().Background(
			Xaml::Media::SolidColorBrush{ ColorHelper::FromArgb(0xFF, red, green, blue) });
	}

	ControlData(EyeDefinition, control, EyeControlData, data);
	data->Xaml->DesktopSource.Content(ins.as<Xaml::UIElement>());
	EyeColorSetter(control, 0, data->EyeColor);

	HookupXamlEvents(ins, control);
}

static void EyeOpeningEvent(REALcontrolInstance control)
{
	// This is the basic setup of how you can use Xaml Islands to host Xaml UI in the
	// provided Win32 window that we provide.
	HWND wnd = REALGetControlHandle(control);
	if (!wnd) return;

	ControlData(EyeDefinition, control, EyeControlData, data);
	data->Xaml = new XamlData;

	auto interop = data->Xaml->DesktopSource.as<IDesktopWindowXamlSourceNative>();
	if (S_OK != interop->AttachToWindow(wnd)) return;

	HWND hWndXamlIsland;
	if (S_OK != interop->get_WindowHandle(&hWndXamlIsland)) return;

	// It's important to also update the Xaml Island window along with our own Win32 window.
	RECT rect;
	::GetWindowRect(wnd, &rect);
	SetWindowPos(hWndXamlIsland, 0, 0, 0, rect.right - rect.left, rect.bottom - rect.top, SWP_SHOWWINDOW);

	try {
		SetupXamlEye(control, true);
	} catch (winrt::hresult_error const& ex) {
		::MessageBoxW(nullptr, ex.message().c_str(), L"Error", 0);
	}
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

	if (!data->Xaml) return;

	auto content = data->Xaml->DesktopSource.Content();
	if (content && content.try_as<Xaml::Controls::Grid>()) {
		// This lambda function recursively iterates through all controls to see if any are named "Eye"
		// (see the definition for kEyesOpen) and sets up the fill color for it if found.
		std::function<void(Xaml::UIElement const& element, RBColor newColor)> SetupColorFunction;
		SetupColorFunction = [&SetupColorFunction](Xaml::UIElement const& element, RBColor newColor) -> void {
			for (int i = 0; i < Xaml::Media::VisualTreeHelper::GetChildrenCount(element); i++) {
				auto it = Xaml::Media::VisualTreeHelper::GetChild(element, i);
				if (it.try_as<Xaml::Shapes::Shape>()) {
					if (0 == _wcsicmp(L"Eye", it.as<Xaml::Shapes::Shape>().Name().c_str())) {
						uint8_t red = (newColor & 0xFF0000) >> 16;
						uint8_t green = (newColor & 0xFF00) >> 8;
						uint8_t blue = (newColor & 0xFF);
						it.as<Xaml::Shapes::Shape>().Fill(Xaml::Media::SolidColorBrush{ ColorHelper::FromArgb(0xFF, red, green, blue) });
					}
				}
				SetupColorFunction(it.as<Xaml::UIElement>(), newColor);
			}
		};

		SetupColorFunction(content.as<Xaml::UIElement>(), newColor);
	}
}

static void EyeClose(REALcontrolInstance control)
{
	ControlData(EyeDefinition, control, EyeControlData, data);
	data->EyeClosed = true;
	SetupXamlEye(control, false);
}

static void EyeOpen(REALcontrolInstance control)
{
	ControlData(EyeDefinition, control, EyeControlData, data);
	data->EyeClosed = false;
	SetupXamlEye(control, true);
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

void EyeStateChanged(REALcontrolInstance control, uint32_t changedField)
{
	if (changedField == REALControlStateChangedBits::kBoundsChanged) {
		ControlData(EyeDefinition, control, EyeControlData, data);

		auto interop = data->Xaml->DesktopSource.as<IDesktopWindowXamlSourceNative>();
		HWND hWndXamlIsland;
		if (S_OK != interop->get_WindowHandle(&hWndXamlIsland)) return;

		HWND wnd = REALGetControlHandle(control);
		RECT rect;
		::GetWindowRect(wnd, &rect);
		SetWindowPos(hWndXamlIsland, 0, 0, 0, rect.right - rect.left, rect.bottom - rect.top, 0);
	} else if (changedField == REALControlStateChangedBits::kThemeChanged) {
		ControlData(EyeDefinition, control, EyeControlData, data);
		SetupXamlEye(control, !data->EyeClosed);
	}
}

void PluginEntry(void)
{
	REALRegisterControl(&EyeDefinition);
}
