//
//  WPFRichTextBox.cpp
//  HostWPFContent
//
//	A simple implementation of the WPF RichTextBox control
//	1. Demonstrates how to setup a WPF control (RichTextBoxOpen)
//	2. Provides a managed code wrapper class (WPFControlData)
//	3. Demonstrates how to hook up events (RichTextBoxSubClass)
//
//  Copyright (c) 2018 Xojo Inc. All rights reserved.
//

#include "rb_plugin.h"
#include <vcclr.h>

namespace RichTextBoxControl {

	static void RichTextBoxOpen(REALcontrolInstance control);
	static void RichTextBoxConstructor(REALcontrolInstance control);
	static void RichTextBoxDestructor(REALcontrolInstance control);
	static void RichTextBoxPaintOffscreen(REALcontrolInstance control, REALgraphics context);
	static RBBoolean RichTextBoxSpellCheckGetter(REALcontrolInstance control, RBInteger unusedParam);
	static void RichTextBoxSpellCheckSetter(REALcontrolInstance control, RBInteger unusedParam, RBBoolean value);
	static void RichTextBoxUndo(REALcontrolInstance control);

	static REALcontrolBehaviour RichTextBoxBehaviour = {
		RichTextBoxConstructor,
		RichTextBoxDestructor,
		NULL,	// Paint
		NULL,	// MouseClick
		NULL,	// MouseDrag
		NULL,	// MouseUp
		NULL,	// GotFocus
		NULL,	// LostFocus
		NULL,	// KeyDown
		RichTextBoxOpen,
		NULL,	// Close
		NULL,	// BackgroundIdle
		RichTextBoxPaintOffscreen,	// This is called by the IDE to paint in the window editor
	};

	static REALproperty RichTextBoxProperties[] = {
		{ "Appearance", "SpellCheck", "Boolean", 0, (REALproc)RichTextBoxSpellCheckGetter, (REALproc)RichTextBoxSpellCheckSetter }
	};

	static REALmethodDefinition RichTextBoxMethods[] = {
		{ (REALproc)RichTextBoxUndo, REALnoImplementation, "Undo" },
	};

	static REALevent RichTextBoxEvents[] = {
		{ "TextChanged()" },
	};

	// This is a convenience enum to identify which event we're dealing with
	enum RichTextBoxEventsEnum
	{
		kRichTextBoxTextChanged
	};

	ref class RichTextBoxSubClass : public System::Windows::Controls::RichTextBox
	{
	private:
		REALcontrolInstance mInstance;

	public:
		RichTextBoxSubClass(REALcontrolInstance control);
		void RaiseTextChangedEvent();

		static void TextChangedCallback(System::Object^ sender, System::Windows::Controls::TextChangedEventArgs^ e);
	};

	class WPFControlData
	{
	public:
		WPFControlData(REALcontrolInstance control);

		gcroot<RichTextBoxSubClass^> TextBox;
	};


	WPFControlData::WPFControlData(REALcontrolInstance control)
	{
		TextBox = gcnew RichTextBoxSubClass(control);
	}

	struct RichTextBoxControlData
	{
		WPFControlData *mControl;
	};

	static REALcontrol RichTextBoxDefinition = {
		kCurrentREALControlVersion,
		"WPFRichTextBox",
		sizeof(RichTextBoxControlData),
		REALenabledControl,
		0, 0,	// This is the BMP name, so the IDE will try and find 0.bmp in IDE Resources->Controls Palette folder in the RBX plugin
		150, 150,
		RichTextBoxProperties, sizeof(RichTextBoxProperties) / sizeof(REALproperty),
		RichTextBoxMethods, sizeof(RichTextBoxMethods) / sizeof(REALmethodDefinition),
		RichTextBoxEvents, sizeof(RichTextBoxEvents) / sizeof(REALevent),
		&RichTextBoxBehaviour,
	};


	RichTextBoxSubClass::RichTextBoxSubClass(REALcontrolInstance control) : System::Windows::Controls::RichTextBox()
	{
		mInstance = control;
		TextChanged += gcnew System::Windows::Controls::TextChangedEventHandler(&TextChangedCallback);
	};

	void RichTextBoxSubClass::RaiseTextChangedEvent()
	{
		ControlData(RichTextBoxDefinition, mInstance, RichTextBoxControlData, data);

		typedef void(*EventTy)(REALcontrolInstance);
		EventTy fp = NULL;
		fp = (EventTy)REALGetEventInstance(mInstance, &RichTextBoxDefinition.events[kRichTextBoxTextChanged]);

		if (fp) fp(mInstance);
	}

	void RichTextBoxSubClass::TextChangedCallback(System::Object^ sender, System::Windows::Controls::TextChangedEventArgs^ e)
	{
		static_cast<RichTextBoxSubClass^>(sender)->RaiseTextChangedEvent();
	}

	static void RichTextBoxConstructor(REALcontrolInstance control)
	{
		ControlData(RichTextBoxDefinition, control, RichTextBoxControlData, data);
		data->mControl = new WPFControlData(control);
	}

	static void RichTextBoxDestructor(REALcontrolInstance control)
	{
		ControlData(RichTextBoxDefinition, control, RichTextBoxControlData, data);
		delete data->mControl;
	}

	static RBBoolean RichTextBoxSpellCheckGetter(REALcontrolInstance control, RBInteger unusedParam)
	{
		ControlData(RichTextBoxDefinition, control, RichTextBoxControlData, data);
		return data->mControl->TextBox->SpellCheck->IsEnabled;
	}

	static void RichTextBoxSpellCheckSetter(REALcontrolInstance control, RBInteger unusedParam, RBBoolean value)
	{
		ControlData(RichTextBoxDefinition, control, RichTextBoxControlData, data);
		data->mControl->TextBox->SpellCheck->IsEnabled = value;
	}

	static void RichTextBoxOpen(REALcontrolInstance control)
	{
		ControlData(RichTextBoxDefinition, control, RichTextBoxControlData, data);

		if (!REALinRuntime()) return;

		RBInteger handle = 0;
		REALGetPropValueInteger(control, "Handle", &handle);

		if (!handle) return;

		Rect r;
		REALGetControlBounds(control, &r);

		System::Windows::Interop::HwndSourceParameters^ sourceParams
			= gcnew System::Windows::Interop::HwndSourceParameters("WPFRichTextBox");
		sourceParams->PositionX = 0;
		sourceParams->PositionY = 0;
		sourceParams->Height = r.bottom - r.top;
		sourceParams->Width = r.right - r.left;
		sourceParams->ParentWindow = System::IntPtr(handle);
		sourceParams->WindowStyle = WS_VISIBLE | WS_CHILD;

		System::Windows::Interop::HwndSource^ source = gcnew System::Windows::Interop::HwndSource(*sourceParams);
		source->RootVisual = data->mControl->TextBox;
	}

	static void RichTextBoxPaintOffscreen(REALcontrolInstance control, REALgraphics context)
	{
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
			Rect r;
			REALGetControlBounds(control, &r);

			int width = r.right - r.left;
			int height = r.bottom - r.top;

			REALSetPropValueColor(context, "ForeColor", 0xFFFFFF);
			FillRect(context, r.left, r.top, width, height);
			REALSetPropValueColor(context, "ForeColor", 0);
			DrawRect(context, r.left, r.top, width, height);

			REALstring str = REALBuildStringWithEncoding(RichTextBoxDefinition.name, strlen(RichTextBoxDefinition.name), kREALTextEncodingUTF8);

			double stringWidth = StringWidth(context, str);

			if (stringWidth < width) DrawString(context, str, (r.right + r.left) / 2 - stringWidth / 2, (r.bottom + r.top) / 2, width - 10, true);
			REALUnlockString(str);
		}
	}

	static void RichTextBoxUndo(REALcontrolInstance control)
	{
		ControlData(RichTextBoxDefinition, control, RichTextBoxControlData, data);
		data->mControl->TextBox->Undo();
	}
}

void RegisterWPFRichTextBoxControl(void)
{
	REALRegisterControl(&RichTextBoxControl::RichTextBoxDefinition);
}
