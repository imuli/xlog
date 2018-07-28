/* Logs X11 key events
 * 
 * Attempts to resolve actual keysym,
 * currently with some limitations.
 *
 * Unlicensed
 *
 */

#include <X11/Xlib.h>
#include <X11/XKBlib.h>
#include <X11/extensions/XInput2.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

// general purpose

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
usage(const char *argv0, int code) {
	fprintf(stderr, "\
usage:  %s [-options ...]\n\
where options include:\n\
	-display name		X11 server to contact\n\
", argv0);
	exit(code);
}

static void
select_events(Display *dpy) {
	Window root = DefaultRootWindow(dpy);
	XIEventMask m;
	m.deviceid = XIAllMasterDevices;
	m.mask_len = XIMaskLen(XI_LASTEVENT);
	m.mask = calloc(m.mask_len, sizeof(char));
	XISetMask(m.mask, XI_RawKeyPress);
	XISetMask(m.mask, XI_RawKeyRelease);
	XISelectEvents(dpy, root, &m, 1);
	XSync(dpy, 0);
	free(m.mask);
}

static void
putEvent(struct timeval *time, unsigned int mods, XIRawEvent *ev, KeySym keysym) {
	char *str = XKeysymToString(keysym);
	putTime(time);
	printf("\t%02x\t%02x\t", mods, ev->detail);
	fputs(ev->evtype == XI_RawKeyPress ? "press" : "release", stdout);
	putchar('\t');
	if(str == NULL) {
		printf("%ld", keysym);
	} else {
		fputs(str, stdout);
	}
	putchar('\n');
	fflush(stdout);
}

static int
getXIOpcode(Display *dpy) {
	// Test for XInput 2 extension
	int xi_opcode;
	int queryEvent, queryError;
	if(!XQueryExtension(dpy, "XInputExtension",
				&xi_opcode, &queryEvent, &queryError)) {
		fprintf(stderr, "X Input extension not available\n");
		return 0;
	}
	// FIXME check for version 2
	return xi_opcode;
}

static int
logkeys(Display *dpy) {
	XEvent event;
	XGenericEventCookie *cookie = &event.xcookie;
	struct timeval baseTime = {0}, time;
	unsigned int mods = 0;
	KeySym keysym = NoSymbol;

	int xi_opcode = getXIOpcode(dpy);
	if(!xi_opcode) return 1;

	while(1) { // Forever
		XNextEvent(dpy, &event);
		if(!XGetEventData(dpy, cookie)) continue;
		if(cookie->type != GenericEvent) continue;
		if(cookie->extension != xi_opcode) continue;
		XIRawEvent *ev = event.xcookie.data;

		// get the time (more or less) for the event
		if(baseTime.tv_sec == 0)
			getBaseTime(&baseTime, ev->time);
		addTime(&baseTime, ev->time, &time);

		// translate into a keysym
		XkbLookupKeySym(dpy, ev->detail, mods, NULL, &keysym);
		if(keysym == NoSymbol) continue;

		// output
		putEvent(&time, mods, ev, keysym);

		// update the state/modifiers
		// FIXME support capslock, etc.
		if(ev->evtype == XI_RawKeyPress)
			mods |= XkbKeysymToModifiers(dpy, keysym);
		else
			mods &= ~XkbKeysymToModifiers(dpy, keysym);
	}
	return 0;
}

int
main(int argc, char *argv[]) {
	const char *display = NULL;
	for(int i = 1; i < argc; i++) {
		if(!strcmp(argv[i], "-display")) {
			display = argv[++i];
		} else if(!strcmp(argv[i], "-help")) {
			usage(argv[0], 0);
		} else {
			fprintf(stderr, "%s: unexpected argument '%s'\n", argv[0], argv[i]);
			usage(argv[0], 1);
		}
	}

	Display *dpy = XOpenDisplay(display);
	if(dpy == NULL) {
		fprintf(stderr, "%s: cannot open X display: %s\n", argv[0], display);
		return 1;
	}
	select_events(dpy);
	return logkeys(dpy);
}

