#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>

#define UPDATEXY(thing) (\
    dx = (thing).x - x, \
    dy = (thing).y - y, \
    x = (thing).x, \
    y = (thing).y )
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

int main(void) {
    unsigned char *pixels = NULL;
    Display *disp = XOpenDisplay(NULL);
    if (!disp) error("Cannot open display");
    int s = DefaultScreen(disp);
    unsigned long BLACK = BlackPixel(disp, s);
    unsigned long WHITE = WhitePixel(disp, s);
    Window w = XCreateSimpleWindow(
            disp, RootWindow(disp, s),
            10, 10, 200, 200, 1,
            BLACK, WHITE);
    XGCValues values = { 0 };
    GC gc = XCreateGC(disp, w, 0, &values);
    XSetForeground(disp, gc, BLACK);
    Atom wmDeleteMessage = XInternAtom(disp, "WM_DELETE_WINDOW", false);
    XSetWMProtocols(disp, w, &wmDeleteMessage, 1);
    XGrabPointer(disp, w, false, ButtonPressMask, GrabModeAsync,
            GrabModeAsync, None, None, CurrentTime);
    XSelectInput(disp, w, ExposureMask | KeyPressMask | ButtonPressMask
            | Button1MotionMask | Button3MotionMask | PointerMotionMask);
    XMapWindow(disp, w);
    pixels = newpixgrid(disp, w);
    mystack backspacebuf = mkstk(16);
    spush(&backspacebuf, '1');
    spush(&backspacebuf, '2');
    spop(&backspacebuf);
    spop(&backspacebuf);
    unsigned x = 0, y = 0;
    unsigned focusx, focusy;
    int dx = 0, dy = 0;
    XPoint anchors[10] = { { 0, 0 } };
    bool isreadyforrcl = false;
    while(true) {
        XEvent e;
        XNextEvent(disp, &e);
        KeySym ks;
        switch (e.type) {
            case Expose:
                break;
            case KeyPress:
                ks = XLookupKeysym(&e.xkey, 0);
                if (ks == XK_Escape) {
                    goto close;
                } else if (ks == XK_BackSpace) {
                    char c = spop(&backspacebuf);
                    if (c) {
                        XSetForeground(disp, gc, WHITE);
                        char buf[2] = { c, '\0' };
                        x -= 6;
                        XDrawString(disp, w, gc, x, y, buf, 1);
                        XSetForeground(disp, gc, BLACK);
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
                            anchors[ks - '0'] = (XPoint){ x, y };
                        } else switch (ks) {
                            case 'w':
                                goto close;
                            case 'z':
                                XDrawString(disp, w, gc, x, y,
                                        "LOL WHAT A NOOB", 15);
                                break;
                            case 'r':
                                isreadyforrcl = true;
                                break;
                            case 'f':
                                if (getselection(&backspacebuf, anchors,
                                            &b1, NULL, &sz)) {
                                    XFillRectangle(disp, w, gc, b1.x, b1.y,
                                            sz.x, sz.y);
                                }
                                break;

                            case 'c':
                                if (getselection(&backspacebuf, anchors,
                                            &b1, NULL, &sz)) {
                                    XCopyArea(disp, w, w, gc,
                                        b1.x, b1.y,
                                        sz.x, sz.y,
                                        x, y);
                                }
                                break;
                        }
                    } else if (isreadyforrcl && isdigit(ks)) {
                        x = anchors[ks - '0'].x;
                        y = anchors[ks - '0'].y;
                        isreadyforrcl = false;
                    } else {
                        char buf[2] = { ks, '\0' };
                        XDrawString(disp, w, gc, x, y, buf, 1);
                        spush(&backspacebuf, ks);
                        x += 6;
                    }
                }
                break;
            case ButtonPress:
                UPDATEXY(e.xbutton);
                switch (e.xbutton.button) {
                    case Button1:
                        XDrawPoint(disp,w,gc, x, y);
                        setpixel(pixels, x, y);
                        break;
                    case Button3:
                        UPDATEXY(e.xbutton);
                        focusx = x; focusy = y;
                        break;
                }
            case ClientMessage:
                if (e.xclient.data.l[0] == wmDeleteMessage) {
                    goto close;
                }
                break;
            case MotionNotify:
                UPDATEXY(e.xmotion);
                if (e.xmotion.state & Button1Mask) {
                    XDrawPoint(disp,w,gc, x, y);
                    setpixel(pixels, x, y);
                } else if (e.xmotion.state & Button3Mask) {
                    XPoint points[3] = {
                        { focusx, focusy },
                        { x, y },
                        { x - dx, y - dy }
                    };
                    XFillPolygon(disp, w, gc, points, 3,
                        Convex, CoordModeOrigin);
                }
                break;
        }
    }
close:
    XCloseDisplay(disp);
    return 0;
}
