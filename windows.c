#include <stdio.h>
#include <X11/Xlib.h>

#include "xlog.h"

static Atom NET_ACTIVE_WINDOW;
static Atom WM_NAME;
static Atom WINDOW;
static Atom STRING;

int
select_windows(Display *dpy, Window root) {
	if(!(WM_NAME = XInternAtom(dpy, "WM_NAME", True))){
		fprintf(stderr, "Cannot find WM_NAME.\n");
		return 0;
	}
	if(!(STRING = XInternAtom(dpy, "STRING", True))){
		fprintf(stderr, "Cannot find STRING.\n");
		return 0;
	}
	if(!(WINDOW = XInternAtom(dpy, "WINDOW", True))){
		fprintf(stderr, "Cannot find WINDOW.\n");
		return 0;
	}
	if(!(NET_ACTIVE_WINDOW = XInternAtom(dpy, "_NET_ACTIVE_WINDOW", True))){
		fprintf(stderr, "Cannot find _NET_ACTIVE_WINDOW.\n");
		return 0;
	}
	if(!XSelectInput(dpy, root, PropertyChangeMask)){;
		fprintf(stderr, "Cannot select Property Change events on root window.\n");
		return 0;
	}
	return 1;
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
	XGetWindowProperty(dpy, root, NET_ACTIVE_WINDOW, 0, 1, False, WINDOW,
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
	XGetWindowProperty(dpy, w, WM_NAME, 0, 1024, False, STRING,
			&actual_type, &actual_format, &n_items, &bytes_after, &name);
	if(actual_type == STRING){
		return (char *)name;
	}
	return None;
}

static Window
handleActiveWindowNameChange(Display *dpy, XPropertyEvent *ev, struct timeval *baseTime, Window w) {
	char *name = getWindowName(dpy, w);
	struct timeval time;

	// get the time (more or less) for the event
	if(baseTime->tv_sec == 0)
		getBaseTime(baseTime, ev->time);
	addTime(baseTime, ev->time, &time);

	putTime(&time);
	printf("\twindow\t0x%08lx", w);
	if(name){
		putchar('\t');
		putstr_escaped(name);
		XFree(name);
	}
	putchar('\n');
	fflush(stdout);

	return w;
}

static Window
handleActiveWindowChange(Display *dpy, XPropertyEvent *ev, struct timeval *baseTime, Window oldWindow) {
	Window w = getActiveWindow(dpy);
	if(w == oldWindow) return oldWindow;

	// switch which window we're listening for name change on
	if(oldWindow) XSelectInput(dpy, oldWindow, None);
	if(w) XSelectInput(dpy, w, PropertyChangeMask);
	
	return handleActiveWindowNameChange(dpy, ev, baseTime, w);
}

Window
handlePropertyNotify(Display *dpy, XPropertyEvent *ev, struct timeval *baseTime, Window activeWindow) {
	if(ev->atom == NET_ACTIVE_WINDOW) {
		return handleActiveWindowChange(dpy, ev, baseTime, activeWindow);
	} else if(ev->atom == WM_NAME) {
		return handleActiveWindowNameChange(dpy, ev, baseTime, activeWindow);
	} else {
//		fputs(XGetAtomName(dpy, ev->atom), stdout);
//		putchar('\n');
		return activeWindow;
	}
}

