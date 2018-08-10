// string.c
int eqstr(const char *a, const char *b);
void putchar_escaped(char c);
void putstr_escaped(const char *s);
void putstr(const char *s);

#ifdef _X11_XLIB_H_
// time.c
void putTime(Time time);

// keys.c
int select_keys(Display *dpy, Window root);
void handleGenericEvent(Display *dpy, XGenericEventCookie *cookie);

// windows.c
int select_windows(Display *dpy, Window root);
void handlePropertyNotify(Display *dpy, XPropertyEvent *ev);
#endif

