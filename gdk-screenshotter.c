#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <dlfcn.h>

#include <gdk-pixbuf-2.0/gdk-pixbuf/gdk-pixbuf.h>

#include <X11/Xlib.h>
#include <X11/extensions/scrnsaver.h>

#define TEMPFILE "/tmp/upwork.png"

int screenshot() {
    // override environment
    int pid = fork();
    if(pid == 0) {
        // child
        printf("in child\n");
        char *wld = getenv("WAYLAND_DISPLAY_REAL");
        char *command = getenv("UPWORK_SCREENSHOT_COMMAND");
        int commandLen = strlen(command);
        int locationLen = strlen(TEMPFILE);
        char *complete_command = malloc(commandLen + locationLen + 2);
        memcpy(complete_command, command, commandLen);
        complete_command[commandLen] = ' ';
        memcpy(complete_command + commandLen + 1, TEMPFILE, locationLen + 1);
        printf("complete command %s\n", complete_command);
        printf("wl disp: %s\n", wld);
        if(wld) {
            setenv("WAYLAND_DISPLAY", wld, 1);
        } else {
            printf("WARNING: no WAYLAND_DISPLAY_REAL\n");
        }
        char *shell = getenv("SHELL");
        execlp("/usr/bin/env","-S",shell,"-c", complete_command,NULL);
        perror("exec");
        free(complete_command);
    } else {
        printf("in parent: %d\n", pid);
        int retval = -1;
        waitpid(pid, &retval, 0);
        return retval;
    }
}


extern GdkPixbuf* gdk_pixbuf_get_from_window(void *window, gint src_x, gint src_y, gint width, gint height) {
    printf("Preparing to take screenshot...\n");
    int res = screenshot();
    if(res != 0) {
        printf("ScreenshotS call failed: %d\n", res);
        return NULL;
    }
    printf("Screenshot success\n");
    GError *err = NULL;
    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(TEMPFILE, &err);
    // clean up grabage
    unlink(TEMPFILE);
    if(err || !pixbuf) {
        printf("pixbuf failure: %s\n", err->message);
        g_error_free(err);
        return NULL;
    }
    printf("Pixbuf success\n");
    return pixbuf;
}

int wanna_break_dimensions = 0;

gboolean (*real_gdk_pixbuf_save_to_callback)(GdkPixbuf*, GdkPixbufSaveFunc, gpointer, const char*, GError**, ...);
extern gboolean gdk_pixbuf_save_to_callback(GdkPixbuf* pb, GdkPixbufSaveFunc fn, gpointer dat, const char* typ, GError** err, ...) {
    if(!real_gdk_pixbuf_save_to_callback) real_gdk_pixbuf_save_to_callback = dlsym(RTLD_NEXT, "gdk_pixbuf_save_to_callback");

    printf("gdk_pixbuf_save_to_callback args: ");
    va_list list;
    va_start(list, err);
    while(1) {
        char *arg = va_arg(list, char*);
        if(!arg) {
            break;
        }
        printf("%s ", (char*)arg);
    }
    va_end(list);
    printf("\n");

    wanna_break_dimensions = 1;

    gboolean res = real_gdk_pixbuf_save_to_callback(pb, fn, dat, typ, err, NULL); // FIXME
    printf("gdk_pixbuf_save_to_callback(%p, %p, %p, %s, %p, ...) => %d\n", pb, fn, dat, typ, err, res);
    return res;
}


Status (*real_XGetWindowAttributes)(Display *display, Window w, XWindowAttributes *attrs);
extern Status XGetWindowAttributes(Display *display, Window w, XWindowAttributes *attrs) {
    if(!real_XGetWindowAttributes) {
        real_XGetWindowAttributes = dlsym(RTLD_NEXT, "XGetWindowAttributes");
    }

    printf("XGetWindowAttrs for 0x%lX\n", w);
    Status res = real_XGetWindowAttributes(display, w, attrs);
    if(!res) {
        printf("Returned error! 0x%X\n", res);
        attrs->x = 0;
        attrs->y = 0;
        attrs->width = 622;
        attrs->height = 450;
        return 1;
    }
    printf("Coords: %dx%d,%dx%d\n", attrs->x, attrs->y, attrs->width, attrs->height);
    if(wanna_break_dimensions) {
        printf("Will break dimensions this time! :evil: Size is zero now! Ha ha ha!\n");
        // For Upwork 5.8.0.33:
        // When running on Xorg, Upwork first takes snapshot using its Native code.
        // But then it tries to take snapshot again using Electron methods.
        // If the latter succeeds then it is used — they name it "enhancing snapshot".
        // Native code uses gdk_pixbuf_get_from_window() which we override.
        // Electron uses something else - I didn't investigate it enough.
        // But first Electron gets screen dimensions (i.e. root window size).
        // If we spoil these values by setting both width and height to be <= 0
        // then Upwork's JS code will return the snapshot it already has,
        // i.e. the one it took with the native code.
        wanna_break_dimensions = 0;
        attrs->width = 0;
        attrs->height = 0;
    }
    return res;
}

int get_active_window_name(char *buf, int bufsize) {
    FILE *fp = popen("swaymsg -t get_tree | jq -r '.. | (.nodes? // empty)[] | select(.focused) | .name'", "r");
    if(!fp) {
        printf("Could not popen\n");
        return 0;
    }
    if(!fgets(buf, bufsize, fp)) {
        printf("Could not read popen\n");
        pclose(fp);
        return 0;
    }
    pclose(fp);
    return strlen(buf)+1;
}
int get_active_window_pid() {
    FILE *fp = popen("swaymsg -t get_tree | jq -r '.. | (.nodes? // empty)[] | select(.focused) | .pid'", "r");
    if(!fp) {
        printf("Could not popen\n");
        return 0;
    }
    char buf[128];
    if(!fgets(buf, sizeof(buf), fp)) {
        printf("Could not read popen\n");
        pclose(fp);
        return 0;
    }
    return atoi(buf);
}


int (*real_XGetWindowProperty)(Display *display, Window w, Atom property, long offset, long length, Bool delete, Atom req_type, Atom *actual_type, int *actual_fmt, unsigned long *nitems, unsigned long *bytes_after, unsigned char **prop);
extern int XGetWindowProperty(Display *display, Window w, Atom property, long offset, long length, Bool delete, Atom req_type, Atom *actual_type, int *actual_fmt, unsigned long *nitems, unsigned long *bytes_after, unsigned char **prop) {
    // property:
    // x+35 = _NET_ACTIVE_WINDOW // nitems should be 1
    // x+36 = _NET_WM_PID // nitems should be 1
    // x+37 = _NET_WM_NAME
    // x+38 = _NAME

    if(!real_XGetWindowProperty) {
        real_XGetWindowProperty = dlsym(RTLD_NEXT, "XGetWindowProperty");
    }

    char *propname = XGetAtomName(display, property);
    printf("Requested property: %s\n", propname);
    if(strcmp(propname, "_NET_WM_PID") == 0) {
        *actual_type = 0;
        *actual_fmt = 32;
        *nitems = 1;
        *bytes_after = 0;
        int *val = malloc(sizeof(int));
        *val = get_active_window_pid();
        printf("Will return pid: %d\n", *val);
        *prop = (char*)val;
    }
    if(!strstr(propname, "NAME")) {
        return real_XGetWindowProperty(display, w, property, offset, length, delete, req_type, actual_type, actual_fmt, nitems, bytes_after, prop);
    }
    XFree(propname);

    *actual_type = 0; // seemingly unused by upw
    *actual_fmt = 8; // list of bytes
    *bytes_after = 0;

    char buf[2048];
    int len = get_active_window_name(buf, sizeof(buf));
    if(!len) {
        *nitems = 0;
        return -1;
    }
    *nitems = len;
    *prop = malloc(len);
    memcpy(*prop, buf, len);
    return Success;
}

// This is seemingly unused. But GnomeIdleTime impl is not used too?..

Bool (*real_XScreenSaverQueryExtension)(Display *dpy, int *event_base_return, int *error_base_return);
extern Bool XScreenSaverQueryExtension(Display *dpy, int *event_base_return, int *error_base_return) {
    if(!real_XScreenSaverQueryExtension) {
        real_XScreenSaverQueryExtension = dlsym(RTLD_NEXT, "XScreenSaverQueryExtension");
    }
    printf("XSSQE: %p %p=%d %p=%d\n", dpy, event_base_return, *event_base_return, error_base_return, *error_base_return);
    Bool res = real_XScreenSaverQueryExtension(dpy, event_base_return, error_base_return);
    printf("XSSQE: %p=%d %p=%d -> %d\n", event_base_return, *event_base_return, error_base_return, *error_base_return, res);
    return res;
}

#ifdef TEST
int main() {
    GdkPixbuf *res = gdk_pixbuf_get_from_window(NULL, 0, 0, 1024, 1024);
    printf("res: %p\n", res);
    gboolean success = gdk_pixbuf_save(res, "/tmp/upwork-out.jpg", "jpeg", NULL, NULL);
    printf("Pixbuf save: %d\n", success);
}
#endif
