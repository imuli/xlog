#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <X11/Xlib.h>

static struct timeval baseTime;

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

void
fromTime(struct timeval *time, Time t) {
	if(baseTime.tv_sec == 0)
		getBaseTime(&baseTime, t);
	addTime(&baseTime, t, time);
}

void
putTime(Time time) {
	char buf[64];
	struct timeval tv;
	fromTime(&tv, time);
	struct tm *gm = gmtime(&tv.tv_sec);
	strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", gm);
	fputs(buf, stdout);
	printf(".%03ld", tv.tv_usec / 1000);
}

