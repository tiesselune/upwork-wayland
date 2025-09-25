Upwork-wayland
==============

This project allows Upwork Screenshots to work under Wayland by replacing its GDK API calls.

It requires the [`flameshot` tool](https://flameshot.org/) tool to be installed and on path, and works on Gnome 49+ under wayland.

Original code (Thank you so much BTW, this is a life-saver) by [@MarSoft](https://github.com/MarSoft).

Install dependencies, run `make`, then launch upwork with `./upwork.sh`.

Dependencies
------------

 - `gcc`
 - `glib`
 - `gdk-pixbuf`
 - `libx11`
 - `pkg-config`
 - [`flameshot` tool](https://flameshot.org/) somewhere in `PATH`
