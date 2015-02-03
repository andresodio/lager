#include <boost/interprocess/ipc/message_queue.hpp>
#include <iostream>
#include <vector>
#include <boost/archive/text_iarchive.hpp>
#include "liblager_connect.h"

#include <X11/Xlib.h>
#include <X11/Intrinsic.h>
#include <X11/extensions/XTest.h>
#include <unistd.h>

using namespace boost::interprocess;
using std::cout;
using std::endl;
using std::string;
using std::stringstream;

/* Based on https://bharathisubramanian.wordpress.com/2010/03/14/x11-fake-key-event-generation-using-xtest-ext/ab */
static void SendKey(Display * display, KeySym key_symbol, KeySym mod_symbol) {
  KeyCode key_code = 0, mod_code = 0;

  key_code = XKeysymToKeycode(display, key_symbol);
  if (key_code == 0)
    return;

  XTestGrabControl(display, True);
  /* Generate modkey press */
  if (mod_symbol != 0) {
    mod_code = XKeysymToKeycode(display, mod_symbol);
    XTestFakeKeyEvent(display, mod_code, True, 0);
  }
  /* Generate regular key press and release */
  XTestFakeKeyEvent(display, key_code, True, 0);
  XTestFakeKeyEvent(display, key_code, False, 0);

  /* Generate modkey release */
  if (mod_symbol != 0) {
    XTestFakeKeyEvent(display, mod_code, False, 0);
  }

  XSync(display, False);
  XTestGrabControl(display, False);
}

void OpenChrome(Display* display) {
  struct timespec sleep_interval = { 0, 750000 };  // microseconds

  // Alt + F2 brings up the open application dialog on Ubuntu Unity
  SendKey(display, XK_F2, XK_Alt_L);

  // Give it time to open
  sleep(1);

  // Delete any old entries
  SendKey(display, XK_BackSpace, 0);

  // Enter google-chrome
  SendKey(display, XK_G, 0);
  SendKey(display, XK_O, 0);
  SendKey(display, XK_O, 0);
  SendKey(display, XK_G, 0);
  SendKey(display, XK_L, 0);
  SendKey(display, XK_E, 0);
  SendKey(display, XK_minus, 0);
  SendKey(display, XK_C, 0);
  SendKey(display, XK_H, 0);
  SendKey(display, XK_R, 0);
  SendKey(display, XK_O, 0);
  SendKey(display, XK_M, 0);
  SendKey(display, XK_E, 0);

  // Press Enter
  SendKey(display, XK_Return, 0);

  // Wait for Chrome to open
  sleep(1);

  // Alt + Tab to switch to the new window
  SendKey(display, XK_Tab, XK_Alt_L);
}

void OpenNewTab(Display* display) {
  // Ctrl + T
  SendKey(display, XK_T, XK_Control_L);
}

void OpenCnn(Display* display) {
  // Type www.cnn.com
  SendKey(display, XK_W, 0);
  SendKey(display, XK_W, 0);
  SendKey(display, XK_W, 0);
  SendKey(display, XK_period, 0);
  SendKey(display, XK_C, 0);
  SendKey(display, XK_N, 0);
  SendKey(display, XK_N, 0);
  SendKey(display, XK_period, 0);
  SendKey(display, XK_C, 0);
  SendKey(display, XK_O, 0);
  SendKey(display, XK_M, 0);

  // Press Enter
  SendKey(display, XK_Return, 0);
}

void OpenGoogle(Display* display) {
  // Type www.google.com
  SendKey(display, XK_W, 0);
  SendKey(display, XK_W, 0);
  SendKey(display, XK_W, 0);
  SendKey(display, XK_period, 0);
  SendKey(display, XK_G, 0);
  SendKey(display, XK_O, 0);
  SendKey(display, XK_O, 0);
  SendKey(display, XK_G, 0);
  SendKey(display, XK_L, 0);
  SendKey(display, XK_E, 0);
  SendKey(display, XK_period, 0);
  SendKey(display, XK_C, 0);
  SendKey(display, XK_O, 0);
  SendKey(display, XK_M, 0);

  // Press Enter
  SendKey(display, XK_Return, 0);
}

void CloseTab(Display* display) {
  // Ctrl + W
  SendKey(display, XK_W, XK_Control_L);
}

void ZoomIn(Display* display) {
  // Ctrl + Plus
  SendKey(display, XK_plus, XK_Control_L);
}

void ZoomOut(Display* display) {
  // Ctrl + Minus
  SendKey(display, XK_minus, XK_Control_L);
}

void RefreshTab(Display* display) {
  // F5
  SendKey(display, XK_F5, 0);
}

void RegisterGestures() {
  SendGestureSubscriptionMessage(
      "Open Chrome",
      "_n._s._s._s._s._x._r._l._d._x._e._k._q._q._p._p._p._p._p._p.");
  SendGestureSubscriptionMessage(
      "New tab",
      "_p._p._p._p._p._p._p._p._p._p._p._p._s._s._s._s._s._s._e._e._e._e._e._e._e._e._e.");
  SendGestureSubscriptionMessage(
      "Open CNN",
      "_s._s._x._x._x._e._k._k._q._q._p._p._p._p._a._a._a._a._a._a._o._q._q._k._e._k._k._e._e._k._a._a._a._a._a._a._a._p._q._q._q._e._q._e._e._e._u._a._a._a._a._a.");
  SendGestureSubscriptionMessage(
      "Open Google",
      "_z._s._s._x._r._x._x._e._q._q._q._p._p._p._o._a._t._s._s._s.");
  SendGestureSubscriptionMessage(
      "Close tab",
      "_r._r._r._r._r._r._r._r._r._r._r._r._r._r._a._a._a._a._a._a._a._a._a._a._q._q._q._q._q._q._q._q._q._q._q._q._q.");
  SendGestureSubscriptionMessage(
      "Zoom in",
      "sp.sp.sp.sp.sp.sp.sp.sp.");
  SendGestureSubscriptionMessage(
      "Zoom out",
      "ps.ps.ps.ps.ps.ps.ps.ps.");
  SendGestureSubscriptionMessage(
      "Refresh tab",
      "_t._a._u._o._o._o._p._p._q._k._k._e._x._x._x._s._s._t._t._n._a.");
}

int main() {
  Display* display = XOpenDisplay(NULL);
  string gesture_name;

  RegisterGestures();

  while (true) {
    gesture_name = GetDetectedGestureMessage().get_gesture_name();
    cout << "Injector: Received message for gesture \"" << gesture_name << "\""
        << endl;

    if (gesture_name.compare("Open Chrome") == 0) {
      OpenChrome(display);
    } else if (gesture_name.compare("New tab") == 0) {
      OpenNewTab(display);
    } else if (gesture_name.compare("Open CNN") == 0) {
      OpenCnn(display);
    } else if (gesture_name.compare("Open Google") == 0) {
      OpenGoogle(display);
    } else if (gesture_name.compare("Close tab") == 0) {
      CloseTab(display);
    } else if (gesture_name.compare("Zoom in") == 0) {
      ZoomIn(display);
    } else if (gesture_name.compare("Zoom out") == 0) {
      ZoomOut(display);
    } else if (gesture_name.compare("Refresh tab") == 0) {
      RefreshTab(display);
    } else {
      cout << "Error: Received unexpected gesture: \"" << gesture_name << "\""
          << endl;
    }
  }

  return 0;
}
