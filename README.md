## keystoke logging and analysis tools

### xkeylog

Logs X11 key events, includes
time, modifiers, keycode, press/release, and keysym.

Attempts to resolve actual keysym, currently with limitations around latching and locking modifiers and (probably) groups.

#### Motivation

Most key loggers for Linux use the evdev subsystem,
which is a much simpler interface than X11,
but requires root access and doesn't ensure you only log your own keystrokes on a shared machine.

Available keyloggers for X11 don't save the time, and generally don't do a good job of getting keysyms correct, often ignoring `Shift` entirely.

#### Usage

Start at X startup with something like:

```bash
xkeylog >> ~/.xkeylog &
```

