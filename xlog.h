#include <sys/time.h>

// put.c
void putchar_escaped(char c);
void putstr_escaped(const char *s);
void putstr(const char *s);
void putKeySym(KeySym k);
void putTime(struct timeval *tv);

// time.c
void addTime(struct timeval *base, Time t, struct timeval *out);
void subTime(struct timeval *base, Time t, struct timeval *out);
void getBaseTime(struct timeval *tv, Time t);

// keys.c
int select_keys(Display *dpy, Window root);
int handleGenericEvent(Display *dpy, XGenericEventCookie *cookie, struct timeval *baseTime, int mods);

// windows.c
int select_windows(Display *dpy, Window root);
Window handlePropertyNotify(Display *dpy, XPropertyEvent *ev, struct timeval *baseTime, Window activeWindow);

