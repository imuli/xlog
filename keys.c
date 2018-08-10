#include <stdio.h>
#include <X11/XKBlib.h>
#include <X11/extensions/XInput2.h>
#include <stdlib.h>

#include "xlog.h"

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

int
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

int
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

