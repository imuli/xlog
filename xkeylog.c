/* Logs X11 key events
 * 
 * Attempts to resolve actual keysym,
 * currently with some limitations.
 *
 * Unlicensed
 *
 */

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/XKBlib.h>
#include <X11/extensions/XInput2.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

enum {
	LogKeys = 1,
	LogWindows = 2,
};

// general purpose

static const char escaped[] = "\
0ooooooabtnvfroooooooooooooooooo\
                                \
                            \\   \
                                \
                                \
                                \
                                \
";
                                
static void
putchar_escaped(char c){
	char style = escaped[(unsigned char)c];
	switch(style){
	case 'o':
		printf("\\%03hho", c);
		break;
	case ' ':
		putchar(c);
		break;
	default:
		putchar('\\');
		putchar(style);
	}
}

static void
putstr_escaped(const char *s){
	for(;*s != '\0';s++)
		putchar_escaped(*s);
}

static void
putTime(struct timeval *tv) {
	char buf[64];
	struct tm *gm = gmtime(&tv->tv_sec);
	strftime(buf, sizeof(buf), "%Y-%m-%d_%H:%M:%S", gm);
	fputs(buf, stdout);
	printf(".%03ld", tv->tv_usec / 1000);
}

static void
addTime(struct timeval *base, Time t, struct timeval *out) {
	out->tv_sec = base->tv_sec + t / 1000;
	out->tv_usec = base->tv_usec + (t % 1000) * 1000;
	while(out->tv_usec > 1000 * 1000) {
		out->tv_usec -= 1000 * 1000;
		out->tv_sec += 1;
	}
}

static void
subTime(struct timeval *base, Time t, struct timeval *out) {
	out->tv_sec = base->tv_sec - t / 1000;
	out->tv_usec = base->tv_usec - (t % 1000) * 1000;
	while(out->tv_usec < 0) {
		out->tv_usec += 1000 * 1000;
		out->tv_sec -= 1;
	}
}

static void
getBaseTime(struct timeval *tv, Time t) {
	gettimeofday(tv, NULL);
	subTime(tv, t, tv);
}

// start of program

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

/*
// uncomment this for debbugging xlib errors
void exit(int val) {}
*/

int
errorHandler(Display *dpy, XErrorEvent *e){
/*
// uncomment this for debbugging xlib errors
	_XDefaultError(dpy,e);
*/
	return 0;
}

// this is a constant once it's set for the first time

static int xi_opcode = 0;
static int
getXIOpcode(Display *dpy) {
	// Test for XInput 2 extension
	int queryEvent, queryError;
	if(!XQueryExtension(dpy, "XInputExtension",
				&xi_opcode, &queryEvent, &queryError)) {
		fprintf(stderr, "X Input extension not available\n");
		return 0;
	}
	// FIXME check for version 2
	return 1;
}

// each of these are called to set up listening to various events

static int
select_keys(Display *dpy, Window root) {
	if(!getXIOpcode(dpy)) return 0;
	XIEventMask m;
	m.deviceid = XIAllMasterDevices;
	m.mask_len = XIMaskLen(XI_LASTEVENT);
	m.mask = calloc(m.mask_len, sizeof(char));
	XISetMask(m.mask, XI_RawKeyPress);
	XISetMask(m.mask, XI_RawKeyRelease);
	XISelectEvents(dpy, root, &m, 1);
	XSync(dpy, 0);
	free(m.mask);
	return 1;
}

static Atom NET_ACTIVE_WINDOW;
static Atom WM_NAME;
static Atom WINDOW;
static Atom STRING;
static int
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

static int
select_events(Display *dpy, int options) {
	Window root = DefaultRootWindow(dpy);
	int good = 1;
	if(good && options & LogKeys) good &= select_keys(dpy, root);
	if(good && options & LogWindows) good &= select_windows(dpy, root);
	return good;
}

static void
putKeySym(KeySym k){
	char *str = XKeysymToString(k);
	if(str == NULL) {
		printf("%ld", k);
	} else {
		fputs(str, stdout);
	}
}

static void
putEvent(struct timeval *time, unsigned int mods, XIRawEvent *ev, KeySym keysym, KeySym keysymNoMods) {
	putTime(time);
	printf("\t%02x\t%02x\t", mods, ev->detail);
	fputs(ev->evtype == XI_RawKeyPress ? "press" : "release", stdout);
	putchar('\t');
  putKeySym(keysym);
	putchar('\t');
  putKeySym(keysymNoMods);
	putchar('\n');
	fflush(stdout);
}

static int
handleGenericEvent(Display *dpy, XGenericEventCookie *cookie, struct timeval *baseTime, int mods) {
	struct timeval time;
	KeySym keysym = NoSymbol;
	KeySym keysymNoMods = NoSymbol;

	if(!XGetEventData(dpy, cookie)) return mods;
	if(cookie->type != GenericEvent) return mods;
	if(cookie->extension != xi_opcode) return mods;
	XIRawEvent *ev = cookie->data;

	// get the time (more or less) for the event
	if(baseTime->tv_sec == 0)
		getBaseTime(baseTime, ev->time);
	addTime(baseTime, ev->time, &time);

	// translate into a keysym
	XkbLookupKeySym(dpy, ev->detail, mods, NULL, &keysym);
	if(keysym == NoSymbol) return mods;
	XkbLookupKeySym(dpy, ev->detail, 0, NULL, &keysymNoMods);
	if(keysymNoMods == NoSymbol) return mods;

	// output
	putEvent(&time, mods, ev, keysym, keysymNoMods);

	// update the state/modifiers
	// FIXME support capslock, etc.
	if(ev->evtype == XI_RawKeyPress)
		return mods | XkbKeysymToModifiers(dpy, keysym);
	else
		return mods & ~XkbKeysymToModifiers(dpy, keysym);
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

static Window
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
	XSetErrorHandler(errorHandler);
	if(!select_events(dpy, options)) return 1;
	return logkeys(dpy);
}

