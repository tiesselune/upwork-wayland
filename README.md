Upwork-wayland
==============

This project allows Upwork Screenshots to work under Wayland by replacing its GDK API calls.

It requires the [`flameshot` tool](https://flameshot.org/) tool to be installed and on path, and works on Gnome 49+ under wayland, but it can be configured to use another tool in the `upwork.sh` file; just change the `UPWORK_SCREENSHOT_COMMAND` env variable.

Original code (Thank you so much BTW, this is a life-saver) by [@MarSoft](https://github.com/MarSoft).

Install dependencies, run `make`, then launch upwork with `./upwork.sh`, which redirects GDK calls to the compiled shared library.

Dependencies
------------

 - `gcc`
 - `glib`
 - `gdk-pixbuf`
 - `libx11`
 - `pkg-config`
 - [`flameshot` tool](https://flameshot.org/) somewhere in `PATH`
