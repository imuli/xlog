#include <stdio.h>
#include <X11/Xlib.h>

#include "xlog.h"

static Atom _NET_ACTIVE_WINDOW;
static Atom STRING;
static Atom UTF8_STRING;
static Atom WINDOW;
static Atom WM_NAME;

static Window activeWindow;
static char *activeName;

static int
getAtom(Display *dpy, Atom *atom, const char *name) {
	Atom x = XInternAtom(dpy, name, True);
	if (!x) {
		fprintf(stderr, "Cannot find %s.\n", name);
		return 0;
	}
	*atom = x;
	return 1;
}

int
select_windows(Display *dpy, Window root) {
	if(!XSelectInput(dpy, root, PropertyChangeMask)){;
		fprintf(stderr, "Cannot select Property Change events on root window.\n");
		return 0;
	}
	return 1
		&& getAtom(dpy, &_NET_ACTIVE_WINDOW, "_NET_ACTIVE_WINDOW")
		&& getAtom(dpy, &STRING, "STRING")
		&& getAtom(dpy, &UTF8_STRING, "UTF8_STRING")
		&& getAtom(dpy, &WINDOW, "WINDOW")
		&& getAtom(dpy, &WM_NAME, "WM_NAME")
		;
}

static Window
getActiveWindow(Display *dpy){
	Window root = DefaultRootWindow(dpy);
	Window active;
	Atom actual_type;
	int actual_format;
	unsigned long n_items;
	unsigned long bytes_after;
	unsigned char *rawWindow;
	XGetWindowProperty(dpy, root, _NET_ACTIVE_WINDOW, 0, 1, False, WINDOW,
			&actual_type, &actual_format, &n_items, &bytes_after, &rawWindow);
	if(actual_type == WINDOW){
		active = *(Window*)rawWindow;
		XFree(rawWindow);
		return active;
	}
	return None;
}

// return must be freed via XFree
static char *
getWindowName(Display *dpy, Window w){
	if(w == None) return NULL;
	Atom actual_type;
	int actual_format;
	unsigned long n_items;
	unsigned long bytes_after;
	unsigned char *name;
	XGetWindowProperty(dpy, w, WM_NAME, 0, 1024, False, AnyPropertyType,
			&actual_type, &actual_format, &n_items, &bytes_after, &name);
	if (actual_type == STRING || actual_type == UTF8_STRING) {
			return (char *) name;
	} else {
			return None;
	}
}

static void
handleActiveWindowNameChange(Display *dpy, XPropertyEvent *ev, Window w) {
	char *name = getWindowName(dpy, w);

	if(activeWindow == w && eqstr(activeName, name)){
		if(name) XFree(name);
		return;
	}
	activeWindow = w;

	putTime(ev->time);
	printf("\twindow\t0x%08lx", activeWindow);
	if(name){
		putchar('\t');
		putstr_escaped(name);
	}
	putchar('\n');
	fflush(stdout);

	if(activeName) XFree(activeName);
	activeName = name;
}

static void
handleActiveWindowChange(Display *dpy, XPropertyEvent *ev) {
	Window w = getActiveWindow(dpy);
	if(w == activeWindow) return;

	// switch which window we're listening for name change on
	if(activeWindow) XSelectInput(dpy, activeWindow, None);
	if(w) XSelectInput(dpy, w, PropertyChangeMask);
	
	handleActiveWindowNameChange(dpy, ev, w);
}

void
handlePropertyNotify(Display *dpy, XPropertyEvent *ev) {
	if(ev->atom == _NET_ACTIVE_WINDOW) {
		handleActiveWindowChange(dpy, ev);
	} else if(ev->atom == WM_NAME) {
		handleActiveWindowNameChange(dpy, ev, activeWindow);
	} else {
//		fputs(XGetAtomName(dpy, ev->atom), stdout);
//		putchar('\n');
	}
}

