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

/**
 * Takes a key symbol and a modifier key. Sends X Server a press and release
 * for the main key. If a modifier is specified, it first sends a press for the
 * modifier and sends a release for it at the very end.
 *
 * Based on code from
 * https://bharathisubramanian.wordpress.com/2010/03/14/x11-fake-key-event-generation-using-xtest-ext/ab
 */
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

/**
 * Sends X Server a series of key presses to bring up the open application
 * dialog in Unity and start Google Chrome.
 */
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

/**
 * Sends X Server the key presses to open a new tab in Google Chrome.
 */
void OpenNewTab(Display* display) {
  // Ctrl + T
  SendKey(display, XK_T, XK_Control_L);
}

/**
 * Sends X Server the key presses to type the CNN URL and press Enter.
 */
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

/**
 * Sends X Server the key presses to type the Google URL and press Enter.
 */
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

/**
 * Sends X Server the key presses to close a tab in Google Chrome.
 */
void CloseTab(Display* display) {
  // Ctrl + W
  SendKey(display, XK_W, XK_Control_L);
}

/**
 * Sends X Server the key presses to zoom in in Google Chrome.
 */
void ZoomIn(Display* display) {
  // Ctrl + Plus
  SendKey(display, XK_plus, XK_Control_L);
}

/**
 * Sends X Server the key presses to zoom out in Google Chrome.
 */
void ZoomOut(Display* display) {
  // Ctrl + Minus
  SendKey(display, XK_minus, XK_Control_L);
}

/**
 * Sends X Server the key press to refresh a page in Google Chrome.
 */
void RefreshTab(Display* display) {
  // F5
  SendKey(display, XK_F5, 0);
}

/**
 * The main loop of the LaGeR Injector.
 */
int main() {
  Display* display = XOpenDisplay(NULL);
  string gesture_name;

  string gestures_file_name;
  SubscribeToGesturesInFile(string(getenv("HOME")) + "/.lager/injector/gestures.dat");

  while (true) {
    gesture_name = GetDetectedGestureMessage().get_gesture_name();
    cout << "Injector: Received message for gesture \"" << gesture_name << "\""
        << endl;

    if (gesture_name.compare("OpenChrome") == 0) {
      OpenChrome(display);
    } else if (gesture_name.compare("NewTab") == 0) {
      OpenNewTab(display);
    } else if (gesture_name.compare("OpenCNN") == 0) {
      OpenCnn(display);
    } else if (gesture_name.compare("OpenGoogle") == 0) {
      OpenGoogle(display);
    } else if (gesture_name.compare("CloseTab") == 0) {
      CloseTab(display);
    } else if (gesture_name.compare("ZoomIn") == 0) {
      ZoomIn(display);
    } else if (gesture_name.compare("ZoomOut") == 0) {
      ZoomOut(display);
    } else if (gesture_name.compare("RefreshTab") == 0) {
      RefreshTab(display);
    } else {
      cout << "Error: Received unexpected gesture: \"" << gesture_name << "\""
          << endl;
    }
  }

  return 0;
}
