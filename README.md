## x11 logging and analysis tools

### xlog

Log X11 events with their time.

* keys: records press/release, modifiers, keycode, and keysym.

	Attempts to resolve actual keysym, currently with limitations around latching and locking modifiers and (probably) groups.

* windows: records window id and name of currently focused window

#### Motivation

Most key loggers for Linux use the evdev subsystem,
which is a much simpler interface than X11,
but requires root access and doesn't ensure you only log your own keystrokes on a shared machine.

Available keyloggers for X11 don't save the time, and generally don't do a good job of getting keysyms correct, often ignoring `Shift` entirely.

#### Usage

Start at X startup with something like:

```bash
xlog -keys >> ~/.xkeylog &
```

