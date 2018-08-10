#include <time.h>
#include <sys/time.h>
#include <X11/Xlib.h>

void
addTime(struct timeval *base, Time t, struct timeval *out) {
	out->tv_sec = base->tv_sec + t / 1000;
	out->tv_usec = base->tv_usec + (t % 1000) * 1000;
	while(out->tv_usec > 1000 * 1000) {
		out->tv_usec -= 1000 * 1000;
		out->tv_sec += 1;
	}
}

void
subTime(struct timeval *base, Time t, struct timeval *out) {
	out->tv_sec = base->tv_sec - t / 1000;
	out->tv_usec = base->tv_usec - (t % 1000) * 1000;
	while(out->tv_usec < 0) {
		out->tv_usec += 1000 * 1000;
		out->tv_sec -= 1;
	}
}

void
getBaseTime(struct timeval *tv, Time t) {
	gettimeofday(tv, NULL);
	subTime(tv, t, tv);
}


