// GtkSearchEntryExample.cpp
//
//	This control plugin sample demonstrates how to host a GtkSearchEntry control (requires GTK 3.6+)
//	1. Like macOS you will create the widget and return it in the controlHandle getter function
//	2. The Xojo framework will take responsibility in cleaning up the GtkWidget
//	3. Visibility and bound changes are all handled by the framework
//	4. This example also demonstrates how to draw the widget offscreen

#include "rb_plugin.h"
#include <gtk/gtk.h>

static void SearchEntryConstructor(REALcontrolInstance control);
static void SearchEntryPaintOffscreen(REALcontrolInstance control, REALgraphics context);
static void *SearchEntryHandleGetter(REALcontrolInstance control);

static REALcontrolBehaviour SearchEntryBehaviour = {
	SearchEntryConstructor,	// Constructor
	nullptr,	// Destructor
	nullptr,	// Redraw
	nullptr,	// Click
	nullptr,	// MouseDrag
	nullptr,	// MouseUp
	nullptr,	// GotFocus
	nullptr,	// LostFocus
	nullptr,	// KeyDown
	nullptr,	// Open
	nullptr,	// Close
	nullptr,	// BackgroundIdle
	SearchEntryPaintOffscreen,	// This is called by the IDE to paint in the window editor
	nullptr,	// SpecialBackground
	nullptr,	// ConstantChanging
	nullptr,	// DroppedNewInstance
	nullptr,	// MouseEnter
	nullptr,	// MouseExit
	nullptr,	// MouseMove
	nullptr,	// State changed
	nullptr, 	// actionEventFunction
	SearchEntryHandleGetter
};

struct SearchEntryData
{
	GtkWidget *widget;
};

static REALcontrol SearchEntryDefinition = {
	kCurrentREALControlVersion,
	"SearchEntry",
	sizeof(SearchEntryData),
	REALenabledControl,
	0, 0,
	170, 30,
	nullptr, 0,
	nullptr, 0,
	nullptr, 0,
	&SearchEntryBehaviour,
};


void SearchEntryConstructor(REALcontrolInstance control)
{
	ControlData(SearchEntryDefinition, control, SearchEntryData, data);
	data->widget = gtk_search_entry_new();
}

void *SearchEntryHandleGetter(REALcontrolInstance control)
{
	ControlData(SearchEntryDefinition, control, SearchEntryData, data);
	return data->widget;
}

void SearchEntryPaintOffscreen(REALcontrolInstance control, REALgraphics context)
{
	// Painting offscreen is window relative.
	Rect r;
	REALGetControlBounds(control, &r);

	int width = r.right - r.left;
	int height = r.bottom - r.top;

	auto handleGetter = reinterpret_cast<cairo_t *(*)(REALobject, RBInteger)>(
		REALLoadObjectMethod(context, "Handle(graphicsType As Integer) As Integer"));

	if (!handleGetter) return;

	// 7 = Graphics.HandleTypes.CairoContext
	cairo_t *cr = (cairo_t *)handleGetter(context, 7);
	if (!cr) return;

	ControlData(SearchEntryDefinition, control, SearchEntryData, data);
	if (gtk_widget_is_drawable(data->widget)) {
		cairo_save(cr);
		cairo_translate(cr, r.left, r.top);
		cairo_rectangle(cr, 0, 0, width, height);
		cairo_clip(cr);
		gtk_widget_draw(data->widget, cr);
		cairo_restore(cr);
	} else {
		// Force it to draw by hosting it into our offscreen window.
		// For optimization this could be cached.
		GtkWidget *offscreen = gtk_offscreen_window_new();
		GtkWidget *entry = gtk_search_entry_new();
		gtk_container_add(GTK_CONTAINER(offscreen), entry);
		gtk_widget_show_all(offscreen);

		cairo_save(cr);
		cairo_translate(cr, r.left, r.top);
		cairo_rectangle(cr, 0, 0, width, height);
		cairo_clip(cr);
		// Bypass gtk_widget_draw because we know this isn't "drawable"
		// so we're essentially forcing it to draw, ignoring all warning signs.
		GTK_WIDGET_GET_CLASS(entry)->draw(entry, cr);
		cairo_restore(cr);
		
		gtk_widget_destroy(offscreen);
	}
}

void PluginEntry(void)
{
	REALRegisterControl(&SearchEntryDefinition);
}
