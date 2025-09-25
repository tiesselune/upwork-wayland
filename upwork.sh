#!/usr/bin/env bash

UPWORK=${UPWORK:-$(which upwork)}

if [ -z "$DISPLAY" ]; then
    echo "Looks like Xwayland is not available!"
    exit 1
fi
if [ -z "$WAYLAND_DISPLAY" ]; then
    echo "This wrapper is not needed when running in X11 environment!"
    echo
    # safely fallthrough
    exec "$UPWORK" "$@"
fi

# export command to be used for screenshots.
# This can be edited with your favourite tool as long
# as the very next argument of the command is the file path to be written to.
# example : `flameshot full -p <path>` would be `flameshot full -p`
export UPWORK_SCREENSHOT_COMMAND="flameshot full -p"

# override session type detection
export XDG_SESSION_TYPE=x11
# save real wayland display for our .so to use
export WAYLAND_DISPLAY_REAL=$WAYLAND_DISPLAY
# make upwork run in Xorg mode, not Wayland mode
export WAYLAND_DISPLAY=
# load our .so
export LD_PRELOAD=$(dirname "$(realpath "$0")")/gdk-screenshotter.so
# enable debug logging
#LOG4JS_CONFIG=debug.json
exec "$UPWORK" "$@"
