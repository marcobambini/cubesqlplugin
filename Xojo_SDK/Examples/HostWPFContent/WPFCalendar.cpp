//
//  WPFCalendar.cpp
//  HostWPFContent
//
//	A simple implementation of the WPF Calendar control
//	1. Demonstrates everything found in WPFRichTextBox.cpp
//	2. Demonstrates how to render WPF controls offscreen
//	3. Uses SizeToContent to fit control in content area
//	4. Uses InteropServices to convert managed strings/bytes
//
//  Copyright (c) 2018 Xojo Inc. All rights reserved.
//

#include "rb_plugin.h"
#include <vcclr.h>

namespace CalendarControl {
	static REALclassRef sPictureClass;

	static void CalendarOpen(REALcontrolInstance control);
	static void CalendarConstructor(REALcontrolInstance control);
	static void CalendarDestructor(REALcontrolInstance control);
	static void CalendarPaintOffscreen(REALcontrolInstance control, REALgraphics context);
	static RBInteger CalendarModeGetter(REALcontrolInstance control, RBInteger unusedParam);
	static void CalendarModeSetter(REALcontrolInstance control, RBInteger unusedParam, RBInteger value);
	static REALstring CalendarToString(REALcontrolInstance control);

	static REALcontrolBehaviour CalendarBehaviour = {
		CalendarConstructor,
		CalendarDestructor,
		NULL,	// Paint
		NULL,	// MouseClick
		NULL,	// MouseDrag
		NULL,	// MouseUp
		NULL,	// GotFocus
		NULL,	// LostFocus
		NULL,	// KeyDown
		CalendarOpen,
		NULL,	// Close
		NULL,	// BackgroundIdle
		CalendarPaintOffscreen,	// This is called by the IDE to paint in the window editor
	};

	static const char *CalendarModeEnum[] = {
		"Month",
		"Year",
		"Decade"
	};

	// This is a convenience enum to identify which mode we're dealing with
	enum CalendarModes
	{
		kMonth,
		kYear,
		kDecade
	};

	static REALproperty CalendarProperties[] = {
		{ "Appearance", "Mode", "Integer", REALpropInvalidate, (REALproc)CalendarModeGetter, (REALproc)CalendarModeSetter, 0, nil, sizeof(CalendarModeEnum) / sizeof(CalendarModeEnum[0]), CalendarModeEnum },
	};

	static REALmethodDefinition CalendarMethods[] = {
		{ (REALproc)CalendarToString, REALnoImplementation, "ToString() As String" },
	};

	static REALevent CalendarEvents[] = {
		{ "SelectedDatesChanged()" },
	};

	// This is a convenience enum to identify which event we're dealing with
	enum CalendarEventsEnum
	{
		kCalendarTextChanged
	};

	ref class CalendarSubClass : public System::Windows::Controls::Calendar
	{
	private:
		REALcontrolInstance mInstance;

	public:
		CalendarSubClass(REALcontrolInstance control);
		void RaiseSelectedDatesChangedEvent();
		void Paint(REALgraphics context);

		static void SelectedDatesChangedCallback(System::Object^ sender, System::Windows::Controls::SelectionChangedEventArgs^ e);
	};

	class WPFControlData
	{
	public:
		WPFControlData(REALcontrolInstance control);

		gcroot<CalendarSubClass^> Calendar;
	};


	WPFControlData::WPFControlData(REALcontrolInstance control)
	{
		Calendar = gcnew CalendarSubClass(control);
	}

	struct CalendarControlData
	{
		WPFControlData *mControl;
	};

	static REALcontrol CalendarDefinition = {
		kCurrentREALControlVersion,
		"WPFCalendar",
		sizeof(CalendarControlData),
		REALenabledControl,
		1, 1,	// This is the BMP name, so the IDE will try and find 1.bmp in IDE Resources->Controls Palette folder in the RBX plugin
		250, 250,
		CalendarProperties, sizeof(CalendarProperties) / sizeof(REALproperty),
		CalendarMethods, sizeof(CalendarMethods) / sizeof(REALmethodDefinition),
		CalendarEvents, sizeof(CalendarEvents) / sizeof(REALevent),
		&CalendarBehaviour,
	};


	CalendarSubClass::CalendarSubClass(REALcontrolInstance control) : System::Windows::Controls::Calendar()
	{
		mInstance = control;
		SelectedDatesChanged += gcnew System::EventHandler<System::Windows::Controls::SelectionChangedEventArgs^>(&SelectedDatesChangedCallback);
	};

	void CalendarSubClass::Paint(REALgraphics context)
	{
		Rect r;
		REALGetControlBounds(mInstance, &r);

		int width = r.right - r.left;
		int height = r.bottom - r.top;

		if (REALinRuntime()) {
			// Force layout if the control isn't visible
			System::Windows::Size sz;
			sz.Width = width;
			sz.Height = height;
			Measure(sz);
			Arrange(System::Windows::Rect(sz));
			UpdateLayout();

			// Go through the trouble of making this HiDPI aware
			double scaleX = 1;
			double scaleY = 1;
			REALGetPropValueDouble(context, "ScaleX", &scaleX);
			REALGetPropValueDouble(context, "ScaleY", &scaleY);

			int picWidth = width * scaleX;
			int picHeight = height * scaleY;

			System::Windows::Media::Imaging::RenderTargetBitmap^ renderTargetBitmap =
				gcnew System::Windows::Media::Imaging::RenderTargetBitmap(picWidth, picHeight, 96 * scaleX, 96 * scaleX, System::Windows::Media::PixelFormats::Pbgra32);
			renderTargetBitmap->Render(this);

			REALobject pic = REALnewInstanceWithClass(sPictureClass);
			typedef void(*NewPictureFuncTy)(REALobject, RBInteger, RBInteger);
			NewPictureFuncTy NewPicture = (NewPictureFuncTy)REALLoadObjectMethod(
				pic, "Constructor(width As Integer, height As Integer)");
			NewPicture(pic, picWidth, picHeight);
			REALSetPropValueInteger(pic, "HorizontalResolution", 72 * scaleX);
			REALSetPropValueInteger(pic, "VerticalResolution", 72 * scaleY);

			// Support for Direct2D was added in 2016r4 so this particular code will not
			// execute in a previous version, though it is possible to support older versions
			REALpictureDescription desc;
			if (REALLockPictureDescription(pic, &desc, pictureD2DPBGRAPtr)) {
				array<System::Byte>^ pixels = gcnew array<System::Byte>(picWidth * picHeight * 4);
				renderTargetBitmap->CopyPixels(pixels, picWidth * 4, 0);

				System::Runtime::InteropServices::Marshal::Copy(pixels, 0, System::IntPtr(desc.pictureData), picWidth * picHeight * 4);

				REALUnlockPictureDescription(pic);

				typedef void(*DrawPictureFuncTy)(REALobject, REALpicture, RBInteger, RBInteger, RBInteger, RBInteger, RBInteger, RBInteger, RBInteger, RBInteger);
				DrawPictureFuncTy DrawPicture = (DrawPictureFuncTy)REALLoadObjectMethod(context, "DrawPicture( Image as Picture, X as Integer, Y as Integer, DestWidth as Integer , DestHeight as Integer, SourceX as Integer, SourceY as Integer, SourceWidth as Integer, SourceHeight as Integer )");

				DrawPicture(context, pic, r.left, r.top, width, height, 0, 0, picWidth, picHeight);

				REALUnlockObject(pic);
			}
		} else {
			// Render something simple for the IDE
			typedef void(*DrawRectFuncTy)(REALobject, RBInteger, RBInteger, RBInteger, RBInteger);
			DrawRectFuncTy DrawRect = (DrawRectFuncTy)REALLoadObjectMethod(context, "DrawRect( X as Integer, Y as Integer, Width as Integer, Height as Integer )");
			typedef void(*FillRectFuncTy)(REALobject, RBInteger, RBInteger, RBInteger, RBInteger);
			FillRectFuncTy FillRect = (FillRectFuncTy)REALLoadObjectMethod(context, "FillRect( X as Integer, Y as Integer, Width as Integer, Width as Integer )");
			typedef void(*DrawStringFuncTy)(REALobject, REALstring, RBInteger, RBInteger, RBInteger, bool);
			DrawStringFuncTy DrawString = (DrawStringFuncTy)REALLoadObjectMethod(context, "DrawString( Str As String, X as Integer, Y as Integer, Width As Integer, Condense As Boolean )");
			typedef double(*StringWidthFuncTy)(REALobject, REALstring);
			StringWidthFuncTy StringWidth = (StringWidthFuncTy)REALLoadObjectMethod(context, "StringWidth( text As String ) As Double");

			if (DrawRect && FillRect && DrawString && StringWidth) {
				REALSetPropValueColor(context, "ForeColor", 0xFFFFFF);
				FillRect(context, r.left, r.top, width, height);
				REALSetPropValueColor(context, "ForeColor", 0);
				DrawRect(context, r.left, r.top, width, height);

				REALstring str = REALBuildStringWithEncoding(CalendarDefinition.name, strlen(CalendarDefinition.name), kREALTextEncodingUTF8);

				double stringWidth = StringWidth(context, str);

				if (stringWidth < width) DrawString(context, str, (r.right + r.left) / 2 - stringWidth / 2, (r.bottom + r.top) / 2, width - 10, true);
				REALUnlockString(str);
			}
		}
	}

	void CalendarSubClass::RaiseSelectedDatesChangedEvent()
	{
		ControlData(CalendarDefinition, mInstance, CalendarControlData, data);

		typedef void(*EventTy)(REALcontrolInstance);
		EventTy fp = NULL;
		fp = (EventTy)REALGetEventInstance(mInstance, &CalendarDefinition.events[kCalendarTextChanged]);

		if (fp) fp(mInstance);
	}

	void CalendarSubClass::SelectedDatesChangedCallback(System::Object^ sender, System::Windows::Controls::SelectionChangedEventArgs^ e)
	{
		static_cast<CalendarSubClass^>(sender)->RaiseSelectedDatesChangedEvent();
	}

	static void CalendarConstructor(REALcontrolInstance control)
	{
		ControlData(CalendarDefinition, control, CalendarControlData, data);
		data->mControl = new WPFControlData(control);
	}

	static void CalendarDestructor(REALcontrolInstance control)
	{
		ControlData(CalendarDefinition, control, CalendarControlData, data);
		delete data->mControl;
	}

	static RBInteger CalendarModeGetter(REALcontrolInstance control, RBInteger unusedParam)
	{
		ControlData(CalendarDefinition, control, CalendarControlData, data);
		if (System::Windows::Controls::CalendarMode::Year == data->mControl->Calendar->DisplayMode) return kYear;
		if (System::Windows::Controls::CalendarMode::Decade == data->mControl->Calendar->DisplayMode) return kDecade;
		return kMonth;
	}

	static void CalendarModeSetter(REALcontrolInstance control, RBInteger unusedParam, RBInteger value)
	{
		ControlData(CalendarDefinition, control, CalendarControlData, data);
		switch (value) {
		case kYear:
			data->mControl->Calendar->DisplayMode = System::Windows::Controls::CalendarMode::Year;
			break;
		case kDecade:
			data->mControl->Calendar->DisplayMode = System::Windows::Controls::CalendarMode::Decade;
			break;
		default:
			data->mControl->Calendar->DisplayMode = System::Windows::Controls::CalendarMode::Month;
			break;
		}
	}

	static REALstring CalendarToString(REALcontrolInstance control)
	{
		ControlData(CalendarDefinition, control, CalendarControlData, data);
		LPCWSTR dateString = (LPCWSTR)System::Runtime::InteropServices::Marshal::StringToHGlobalAuto(
			data->mControl->Calendar->ToString()).ToPointer();

		REALstring result = REALBuildStringWithEncoding((char *)dateString, wcslen(dateString) * sizeof(wchar_t), kREALTextEncodingUTF16LE);
		return result;
	}

	static void CalendarOpen(REALcontrolInstance control)
	{
		ControlData(CalendarDefinition, control, CalendarControlData, data);

		if (!REALinRuntime()) return;

		RBInteger handle = 0;
		REALGetPropValueInteger(control, "Handle", &handle);

		if (!handle) return;

		Rect r;
		REALGetControlBounds(control, &r);

		System::Windows::Interop::HwndSourceParameters^ sourceParams
			= gcnew System::Windows::Interop::HwndSourceParameters("WPFCalendar");
		sourceParams->PositionX = 0;
		sourceParams->PositionY = 0;
		sourceParams->Height = r.bottom - r.top;
		sourceParams->Width = r.right - r.left;
		sourceParams->ParentWindow = System::IntPtr(handle);
		sourceParams->WindowStyle = WS_VISIBLE | WS_CHILD;

		System::Windows::Interop::HwndSource^ source = gcnew System::Windows::Interop::HwndSource(*sourceParams);
		source->RootVisual = data->mControl->Calendar;
		source->SizeToContent = System::Windows::SizeToContent::WidthAndHeight;
	}

	static void CalendarPaintOffscreen(REALcontrolInstance control, REALgraphics context)
	{
		ControlData(CalendarDefinition, control, CalendarControlData, data);
		data->mControl->Calendar->Paint(context);
	}
}

void RegisterWPFCalendarControl(void)
{
	CalendarControl::sPictureClass = REALGetClassRef("Picture");

	REALRegisterControl(&CalendarControl::CalendarDefinition);
}
