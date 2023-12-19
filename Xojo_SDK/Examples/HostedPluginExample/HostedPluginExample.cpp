//
//  HostedPluginExample.cpp
//  HostedPluginExample
//
//	This control plugin sample demonstrates how to expose an arbitrary NSView
//  from the plugin and forward events. For the example, a read-only rating level
//  indicator has been wrapped.
//
//  Copyright (c) 2013 Xojo Inc. All rights reserved.
//

#include "rb_plugin.h"
#if COCOA
	// To keep this example simple, this file will need to be compiled as Obj-C++
	// in Xcode. In your shipping plugin, you will most likely want to separate
	// out the Cocoa-specific Objective-C code such that the .cpp file is just
	// C++ and reduce the number of #if preprocessor directives.
	#if !defined(__OBJC__) || !defined(__cplusplus)
			#error This file must be compiled as Objective-C++.
	#endif
	#include "REALExampleLevelIndicator.h"
#endif

static void RatingControlInitializer(REALcontrolInstance);
static void RatingControlFinalizer(REALcontrolInstance);
static void * RatingControlHandleGetter(REALcontrolInstance);
static RBInteger RatingControlValueGetter(REALcontrolInstance);
static void RatingControlValueSetter(REALcontrolInstance, RBInteger, RBInteger);

struct RatingControlData {
#if COCOA
	REALExampleLevelIndicator *view;
#else
	// This padding is only neccessary when supporting versions prior to 2015r3.
	void *view;
#endif
};

static REALproperty sRatingControlProps[] = {
	{ "Value", "Value", "Integer", 0, (REALproc)RatingControlValueGetter, (REALproc)RatingControlValueSetter }
};

static REALcontrolBehaviour sLevelIndicatorBehavior = {
	RatingControlInitializer,
	RatingControlFinalizer,
	NULL, // redrawFunction
	NULL, // clickFunction
	NULL, // mouseDragFunction
	NULL, // mouseUpFunction
	NULL, // gainedFocusFunction
	NULL, // lostFocusFunction
	NULL, // keyDownFunction
	NULL, // openFunction
	NULL, // closeFunction
	NULL, // backgroundIdleFunction
	NULL, // drawOffscreenFunction
	NULL, // setSpecialBackground
	NULL, // constantChanging
	NULL, // droppedNewInstance
	NULL, // mouseEnterFunction
	NULL, // mouseExitFunction
	NULL, // mouseMoveFunction
	NULL, // stateChangedFunction
	NULL, // actionEventFunction
	RatingControlHandleGetter
};

static REALcontrol sRatingControl = {
	kCurrentREALControlVersion,
	"RatingControl",
	sizeof(RatingControlData),
	REALdesktopControl, // flags
	0, 0, // toolbar icons
	65, 13, // width/height
	sRatingControlProps, _countof(sRatingControlProps),
	NULL, 0, // no methods
	NULL, 0, // no events
	&sLevelIndicatorBehavior
};

static void RatingControlInitializer( REALcontrolInstance control )
{
	RatingControlData *data = (RatingControlData *)REALGetControlData( control, &sRatingControl );
	
#if COCOA
	// No need to calculate what frame to intialize the view with - the RB
	// framework will move it around as needed.
	data->view = [[REALExampleLevelIndicator alloc] init];
	
	// Configure it to be a rating control instead of a level indicator
	[[data->view cell] setLevelIndicatorStyle:NSRatingLevelIndicatorStyle];
	[data->view setMaxValue:5];
	
	// Set the default value to 3. Having it be 0 results in nothing drawing
	// at all.
	[data->view setIntegerValue:3];
	
	// And tell the view which control instance it's associated with. The view
	// doesn't do any special locking/unlocking, so we don't have a problem with
	// circular references.
	data->view.controlInstance = control;
#endif
}

static void RatingControlFinalizer( REALcontrolInstance control )
{
	RatingControlData *data = (RatingControlData *)REALGetControlData( control, &sRatingControl );

#if COCOA
	// Since our control is going away, make sure to cleanup the view so that it
	// doesn't refer to it anymore.
	data->view.controlInstance = NULL;
	[data->view release];
#endif
}

static void * RatingControlHandleGetter( REALcontrolInstance control )
{
	RatingControlData *data = (RatingControlData *)REALGetControlData( control, &sRatingControl );

#if COCOA
	return data->view;
#else
	return NULL;
#endif
}

static RBInteger RatingControlValueGetter( REALcontrolInstance control )
{
	RatingControlData *data = (RatingControlData *)REALGetControlData( control, &sRatingControl );

#if COCOA
	return [data->view integerValue];
#else
	return 0;
#endif
}

static void RatingControlValueSetter( REALcontrolInstance control, RBInteger, RBInteger val )
{
	RatingControlData *data = (RatingControlData *)REALGetControlData( control, &sRatingControl );

#if COCOA
	[data->view setIntegerValue:val];
#endif
}

void PluginEntry()
{
	REALRegisterControl( &sRatingControl );
}
