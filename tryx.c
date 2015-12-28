#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/cursorfont.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <limits.h>

#define UPDATEXY(thing) (\
    state.dx = (thing).x - state.x, \
    state.dy = (thing).y - state.y, \
    state.x = (thing).x, \
    state.y = (thing).y )
#define MIN(a, b) ((a) < (b)? (a) : (b))
#define MAX(a, b) ((a) > (b)? (a) : (b))

typedef struct {
    size_t end;
    size_t len;
    char *buf;
} mystack;

mystack mkstk(size_t size) {
    return (mystack){ 0, size, malloc(size) };
}

char spop(mystack *rb) {
    if (!rb->end) {
        return 0;
    } else {
        return rb->buf[--rb->end];
    }
}

void spush(mystack *rb, char c) {
    if (rb->end < rb->len) {
        rb->buf[rb->end++] = c;
    } else {
        memmove(rb->buf, rb->buf + 1, rb->len - 1);
        rb->buf[rb->len - 1] = c;
    }
}

void error(char *f, ...) {
    va_list ap;
    va_start(ap, f);
    vfprintf(stderr, f, ap);
    va_end(ap);
    exit(-1);
}

void note(char *f, ...) {
    va_list ap;
    va_start(ap, f);
    vfprintf(stderr, f, ap);
    va_end(ap);
}

size_t updiv(size_t a, size_t b) {
    return a/b + ((a%b)? 1 : 0);
}

void setpixel(unsigned char *pixels, unsigned x, unsigned y) {
    return;
}

unsigned char *newpixgrid(Display *dpy, Drawable d) {
    Window _0;
    int _1, _2;
    unsigned int width, height;
    unsigned int _3, _4;
    XGetGeometry(dpy, d, &_0, &_1, &_2, &width, &height, &_3, &_4);
    return malloc(updiv(width * height, 8));
}

bool getselection(mystack *bsb, XPoint anchors[],
        XPoint *b1, XPoint *b2, XPoint *sz) {
    XPoint _b1, _b2, _sz;
    if (!b1) b1 = &_b1;
    if (!b2) b2 = &_b2;
    if (!sz) sz = &_sz;
    if (bsb->end <= bsb->len - 2) {
        char c1 = bsb->buf[bsb->end];
        char c2 = bsb->buf[bsb->end + 1];
        if (isdigit(c1) && isdigit(c2)) {
            XPoint a1 = anchors[c1 - '0'];
            XPoint a2 = anchors[c2 - '0'];
            b1->x = MIN(a1.x, a2.x);
            b1->y = MIN(a1.y, a2.y);
            b2->x = MAX(a1.x, a2.x);
            b2->y = MAX(a1.y, a2.y);
            sz->x = b2->x - b1->x;
            sz->y = b2->y - b1->y;
            return true;
        } else {
            note("NO SEL: BSB not digits (%.2s)\n", bsb->buf + bsb->end);
            return false;
        }
    } else {
        note("NO SEL: BSB full. (%zu) (%zu)\n", bsb->end, bsb->len);
        return false;
    }
}

typedef struct {
    unsigned char *pixels;
    Display *disp;
    int s;
    unsigned long BLACK;
    unsigned long WHITE;
    Window w;
    XGCValues values;
    GC gc;
    Atom wmDeleteMessage;
    mystack backspacebuf;
    unsigned x, y;
    unsigned focusx, focusy;
    int dx, dy;
    XPoint anchors[10];
    bool isreadyforrcl;
} state;

void lockwindowsize(Display *d, Window w) {
    Window _0;
    int _1, _2;
    unsigned int width, height;
    unsigned int _3, _4;
    XGetGeometry(d, w, &_0, &_1, &_2, &width, &height, &_3, &_4);

    XSizeHints xsh;
    xsh.flags = USSize;
    xsh.min_width = xsh.max_width = width;
    xsh.min_height = xsh.max_height = height;
    XSetNormalHints(d, w, &xsh);
    XSetWMSizeHints(d, w, &xsh, PMinSize | PMaxSize);
    printf("Booyeah!\n");
}

void unlockwindowsize(Display *d, Window w) {
    XSizeHints xsh;
    xsh.flags = USSize;
    xsh.min_width = xsh.min_height = 0;
    xsh.max_width = xsh.max_height = INT_MAX;
    XSetNormalHints(d, w, &xsh);
    XSetWMSizeHints(d, w, &xsh, PMinSize | PMaxSize);
    printf("Booyeah?\n");
}


#define SDS state.disp
#define SW state.w
#define SGC state.gc

int main(void) {
    state state;
    state.pixels = NULL;
    state.disp = XOpenDisplay(NULL);
    if (!state.disp) error("Cannot open display");
    state.s = DefaultScreen(state.disp);
    state.BLACK = BlackPixel(SDS, state.s);
    state.WHITE = WhitePixel(SDS, state.s);
    state.w = XCreateSimpleWindow(
            state.disp, RootWindow(SDS, state.s),
            10, 10, 200, 200, 1,
            state.BLACK, state.WHITE);
    state.values = (XGCValues){ 0 };
    state.gc = XCreateGC(SDS, SW, 0, &state.values);
    XSetForeground(state.disp, state.gc, state.BLACK);
    state.wmDeleteMessage = XInternAtom(state.disp, "WM_DELETE_WINDOW", false);
    XSetWMProtocols(SDS, SW, &state.wmDeleteMessage, 1);
    XGrabPointer(SDS, SW, false, ButtonPressMask, GrabModeAsync,
            GrabModeAsync, None, None, CurrentTime);
    XSelectInput(SDS, SW, ExposureMask | KeyPressMask | ButtonPressMask
            | Button1MotionMask | Button3MotionMask | PointerMotionMask);
    XStoreName(SDS, SW, "Doodle Pro");
    Cursor c = XCreateFontCursor(SDS, XC_tcross);
    XDefineCursor(SDS, SW, c);
    XMapWindow(SDS, SW);
    state.pixels = newpixgrid(SDS, SW);
    state.backspacebuf = mkstk(16);
    spush(&state.backspacebuf, '1');
    spush(&state.backspacebuf, '2');
    spop(&state.backspacebuf);
    spop(&state.backspacebuf);
    state.x = 0, state.y = 0;
    state.dx = 0, state.dy = 0;
    memset(state.anchors, 0, sizeof state.anchors);
    state.isreadyforrcl = false;
    while(true) {
        XEvent e;
        XNextEvent(state.disp, &e);
        KeySym ks;
        switch (e.type) {
            case Expose:
                break;
            case KeyPress:
                ks = XLookupKeysym(&e.xkey, 0);
                if (ks == XK_Escape) {
                    goto close;
                } else if (ks == XK_BackSpace) {
                    char c = spop(&state.backspacebuf);
                    if (c) {
                        XSetForeground(state.disp, state.gc, state.WHITE);
                        char buf[2] = { c, '\0' };
                        state.x -= 6;
                        XDrawString(SDS, SW, SGC, state.x, state.y, buf, 1);
                        XSetForeground(state.disp, state.gc, state.BLACK);
                    }
                } else if (' ' <= ks && ks <= '~') {
                    if (e.xkey.state & ShiftMask) {
                        if (isdigit(ks)) {
                            ks = ")!@#$%^&*("[ks - '0'];
                        } else if (isalpha(ks)) {
                            ks -= 'a' - 'A';
                        } else switch (ks) {
                            case '`': ks='~'; break;
                            case '-': ks='_'; break;
                            case '=': ks='+'; break;
                            case '[': ks='{'; break;
                            case ']': ks='}'; break;
                            case '\\': ks='|'; break;
                            case ';': ks=':'; break;
                            case '\'': ks='"'; break;
                            case ',': ks='<'; break;
                            case '.': ks='>'; break;
                            case '/': ks='?'; break;
                        }
                    }
                    if (e.xkey.state & ControlMask) {
                        XPoint b1, b2, sz;
                        if (isdigit(ks)) {
                            state.anchors[ks - '0'] =
                                (XPoint){ state.x, state.y };
                        } else switch (ks) {
                            case 'w':
                                goto close;
                            case 'i':
                                {   unsigned long tmp = state.BLACK;
                                    state.BLACK = state.WHITE;
                                    state.WHITE = tmp; }
                                XSetForeground(state.disp, state.gc, state.BLACK);
                                break;
                            case 'z':
                                XDrawString(SDS, SW, SGC, state.x, state.y,
                                        "LOL WHAT A NOOB", 15);
                                break;
                            case 'r':
                                state.isreadyforrcl = true;
                                break;
                            case 'l':
                                lockwindowsize(SDS, SW);
                                break;
                            case 'L':
                                unlockwindowsize(SDS, SW);
                            case 'f':
                                if (getselection(&state.backspacebuf,
                                            state.anchors,
                                            &b1, NULL, &sz)) {
                                    XFillRectangle(SDS, SW, SGC, b1.x, b1.y,
                                            sz.x, sz.y);
                                }
                                break;

                            case 'c':
                                if (getselection(&state.backspacebuf,
                                            state.anchors,
                                            &b1, NULL, &sz)) {
                                    XCopyArea(SDS, SW, state.w, state.gc,
                                        b1.x, b1.y,
                                        sz.x, sz.y,
                                        state.x, state.y);
                                }
                                break;
                        }
                    } else if (state.isreadyforrcl && isdigit(ks)) {
                        state.x = state.anchors[ks - '0'].x;
                        state.y = state.anchors[ks - '0'].y;
                        state.isreadyforrcl = false;
                    } else {
                        char buf[2] = { ks, '\0' };
                        XDrawString(SDS, SW, SGC, state.x, state.y, buf, 1);
                        spush(&state.backspacebuf, ks);
                        state.x += 6;
                    }
                }
                break;
            case ButtonPress:
                UPDATEXY(e.xbutton);
                switch (e.xbutton.button) {
                    case Button1:
                        XDrawPoint(SDS, SW, SGC, state.x, state.y);
                        setpixel(state.pixels, state.x, state.y);
                        break;
                    case Button3:
                        UPDATEXY(e.xbutton);
                        state.focusx = state.x;
                        state.focusy = state.y;
                        break;
                }
            case ClientMessage:
                if (e.xclient.data.l[0] == state.wmDeleteMessage) {
                    goto close;
                }
                break;
            case MotionNotify:
                UPDATEXY(e.xmotion);
                if (e.xmotion.state & Button1Mask) {
                    XDrawPoint(SDS, SW, SGC, state.x, state.y);
                    setpixel(state.pixels, state.x, state.y);
                } else if (e.xmotion.state & Button3Mask) {
                    XPoint points[3] = {
                        { state.focusx, state.focusy },
                        { state.x, state.y },
                        { state.x - state.dx, state.y - state.dy }
                    };
                    XFillPolygon(SDS, SW, SGC, points, 3,
                        Convex, CoordModeOrigin);
                }
                break;
        }
    }
close:
    XCloseDisplay(state.disp);
    return 0;
}
