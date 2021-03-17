//
//  WPFControls.cpp
//  HostWPFContent
//
//	This control plugin sample demonstrates how to host WPF controls in Xojo
//	1. Make sure you enable /clr in your VS project
//	2. Add References to the components you need to use
//	3. Make sure to call [System::STAThreadAttribute]
//
//  Copyright (c) 2018 Xojo Inc. All rights reserved.
//

#include "rb_plugin.h"
#include "WPFCalendar.h"
#include "WPFRichTextBox.h"


[System::STAThreadAttribute]
void PluginEntry(void)
{
	RegisterWPFCalendarControl();
	RegisterWPFRichTextBoxControl();
}
