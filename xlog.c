/* Logs X11 events
 * 
 * When logging keys, it attempts to resolve actual keysym,
 * currently with some limitations.
 *
 * Unlicensed
 *
 */

#include <string.h>
#include <stdio.h>
#include <X11/Xlib.h>

#include "xlog.h"

enum {
	LogKeys = 1,
	LogWindows = 2,
};

static void
usage(const char *argv0) {
	fprintf(stderr, "\
usage:  %s [-options ...]\n\
where options include:\n\
	-display name		X11 server to contact\n\
	-keys		log keys\n\
	-windows		log focused window name\n\
", argv0);
}

// when we're debugging we want to show errors
// but not exit
#ifdef debug
void exit(int val) {}
#else
static int
errorHandler(Display *dpy, XErrorEvent *e){
	return 0;
}
#endif

// each of these are called to set up listening to various events

static int
select_events(Display *dpy, int options) {
	Window root = DefaultRootWindow(dpy);
	int good = 1;
	if(good && options & LogKeys) good &= select_keys(dpy, root);
	if(good && options & LogWindows) good &= select_windows(dpy, root);
	return good;
}

static int
logkeys(Display *dpy) {
	XEvent event;
	Window activeWindow = None;
	struct timeval baseTime = {0};
	unsigned int mods = 0;

	while(1) {
		XNextEvent(dpy, &event);
	switch(event.type){ // Forever
	case PropertyNotify:
		activeWindow = handlePropertyNotify(dpy, (XPropertyEvent*)&event, &baseTime, activeWindow);
		break;
	case GenericEvent:
		mods = handleGenericEvent(dpy, &event.xcookie, &baseTime, mods);
		break;
	} }
	return 0;
}

int
main(int argc, char *argv[]) {
	const char *display = NULL;
	int options = None;
	for(int i = 1; i < argc; i++) {
		if(!strcmp(argv[i], "-display")) {
			display = argv[++i];
		} else if(!strcmp(argv[i], "-keys")) {
			options |= LogKeys;
		} else if(!strcmp(argv[i], "-windows")) {
			options |= LogWindows;
		} else if(!strcmp(argv[i], "-help")) {
			usage(argv[0]);
			return 0;
		} else {
			fprintf(stderr, "%s: unexpected argument '%s'\n", argv[0], argv[i]);
			usage(argv[0]);
			return 1;
		}
	}

	if(options == None){
		fprintf(stderr, "%s: nothing specified to log\n", argv[0]);
		usage(argv[0]);
		return 1;
	}	

	Display *dpy = XOpenDisplay(display);
	if(dpy == NULL) {
		fprintf(stderr, "%s: cannot open X display: %s\n", argv[0], display);
		return 1;
	}
#ifndef debug
	XSetErrorHandler(errorHandler);
#endif
	if(!select_events(dpy, options)) return 1;
	return logkeys(dpy);
}

