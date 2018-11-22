#include <boost/interprocess/ipc/message_queue.hpp>
#include <iostream>
#include <vector>
#include <boost/archive/text_iarchive.hpp>
#include <time.h>
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

struct timespec sleep_interval = { 0, 250000000 }; // 250 ms

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
 * Sends X Server the key presses to close a tab in Google Chrome.
 */
void CloseTab(Display* display) {
  // Ctrl + W
  SendKey(display, XK_W, XK_Control_L);
}

/**
 * Sends X Server the key presses to maximize a window.
 */
void MaximizeWindow(Display* display) {
  // Windows + Up
  SendKey(display, XK_Up, XK_Super_L);
}

/**
 * Sends X Server the key presses to open a new tab in Google Chrome.
 */
void NewTab(Display* display) {
  // Ctrl + T
  SendKey(display, XK_T, XK_Control_L);
}

/**
 * Sends X Server a series of key presses to start Google Chrome via
 * a system shortcut. Note that the number below corresponds to the
 * position of the Google Chrome favorite on GNOME Shell dock.
 */
void OpenChrome(Display* display) {
  // Windows + 1
  SendKey(display, XK_1, XK_Super_L);
}

/**
 * Sends X Server the key presses to go to the navigation bar,
 * type the CNN URL and press Enter.
 */
void OpenCnn(Display* display) {
  // Ctrl + L shifts the focus to the navigation bar
  SendKey(display, XK_L, XK_Control_L);

  // Sleep to give time for the focus to shift
  nanosleep(&sleep_interval, NULL);

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
 * Sends X Server the key presses to go to the navigation bar,
 * type the Google URL and press Enter.
 */
void OpenGoogle(Display* display) {
  // Ctrl + L shifts the focus to the navigation bar
  SendKey(display, XK_L, XK_Control_L);

  // Sleep to give time for the focus to shift
  nanosleep(&sleep_interval, NULL);

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
 * Sends X Server the key presses to go to the navigation bar,
 * type the Google Play Music URL and press Enter.
 */
void OpenMusic(Display* display) {
  // Ctrl + L shifts the focus to the navigation bar
  SendKey(display, XK_L, XK_Control_L);

  // Sleep to give time for the focus to shift
  nanosleep(&sleep_interval, NULL);

  // Type play.google.com/music
  SendKey(display, XK_P, 0);
  SendKey(display, XK_L, 0);
  SendKey(display, XK_A, 0);
  SendKey(display, XK_Y, 0);
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
  SendKey(display, XK_slash, 0);
  SendKey(display, XK_M, 0);
  SendKey(display, XK_U, 0);
  SendKey(display, XK_S, 0);
  SendKey(display, XK_I, 0);
  SendKey(display, XK_C, 0);

  // Press Enter
  SendKey(display, XK_Return, 0);
}

/**
 * Sends X Server the key presses to go to the navigation bar,
 * type the Netflix URL and press Enter.
 */
void OpenNetflix(Display* display) {
  // Ctrl + L shifts the focus to the navigation bar
  SendKey(display, XK_L, XK_Control_L);

  // Sleep to give time for the focus to shift
  nanosleep(&sleep_interval, NULL);

  // Type www.netflix.com
  SendKey(display, XK_W, 0);
  SendKey(display, XK_W, 0);
  SendKey(display, XK_W, 0);
  SendKey(display, XK_period, 0);
  SendKey(display, XK_N, 0);
  SendKey(display, XK_E, 0);
  SendKey(display, XK_T, 0);
  SendKey(display, XK_F, 0);
  SendKey(display, XK_L, 0);
  SendKey(display, XK_I, 0);
  SendKey(display, XK_X, 0);
  SendKey(display, XK_period, 0);
  SendKey(display, XK_C, 0);
  SendKey(display, XK_O, 0);
  SendKey(display, XK_M, 0);

  // Press Enter
  SendKey(display, XK_Return, 0);
}

/**
 * Sends X Server the key presses to go to the navigation bar,
 * type the Google URL and press Enter.
 */
void OpenWeather(Display* display) {
  // Ctrl + L shifts the focus to the navigation bar
  SendKey(display, XK_L, XK_Control_L);

  // Sleep to give time for the focus to shift
  nanosleep(&sleep_interval, NULL);

  // Type weather
  SendKey(display, XK_W, 0);
  SendKey(display, XK_E, 0);
  SendKey(display, XK_A, 0);
  SendKey(display, XK_T, 0);
  SendKey(display, XK_H, 0);
  SendKey(display, XK_E, 0);
  SendKey(display, XK_R, 0);

  // Press Enter
  SendKey(display, XK_Return, 0);
}

/**
 * Sends X Server the key presses to go to the navigation bar,
 * type the YouTube URL and press Enter.
 */
void OpenYouTube(Display* display) {
  // Ctrl + L shifts the focus to the navigation bar
  SendKey(display, XK_L, XK_Control_L);

  // Sleep to give time for the focus to shift
  nanosleep(&sleep_interval, NULL);

  // Type www.youtube.com
  SendKey(display, XK_W, 0);
  SendKey(display, XK_W, 0);
  SendKey(display, XK_W, 0);
  SendKey(display, XK_period, 0);
  SendKey(display, XK_Y, 0);
  SendKey(display, XK_O, 0);
  SendKey(display, XK_U, 0);
  SendKey(display, XK_T, 0);
  SendKey(display, XK_U, 0);
  SendKey(display, XK_B, 0);
  SendKey(display, XK_E, 0);
  SendKey(display, XK_period, 0);
  SendKey(display, XK_C, 0);
  SendKey(display, XK_O, 0);
  SendKey(display, XK_M, 0);

  // Press Enter
  SendKey(display, XK_Return, 0);
}

/**
 * Sends X Server the key press to refresh a page in Google Chrome.
 */
void RefreshTab(Display* display) {
  // F5
  SendKey(display, XK_F5, 0);
}

/**
 * Sends X Server the key presses to restore a window.
 */
void RestoreWindow(Display* display) {
  // Windows + Down
  SendKey(display, XK_Down, XK_Super_L);
}

/**
 * Sends X Server the key presses to scroll down.
 */
void ScrollDown(Display* display) {
  // PgDown
  SendKey(display, XK_KP_Page_Down, 0);
}

/**
 * Sends X Server the key presses to scroll up.
 */
void ScrollUp(Display* display) {
  // PgUp
  SendKey(display, XK_KP_Page_Up, 0);
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

    if (gesture_name.compare("CloseTab") == 0) {
      CloseTab(display);
    } else if (gesture_name.compare("MaximizeWindow") == 0) {
      MaximizeWindow(display);
    } else if (gesture_name.compare("NewTab") == 0) {
      NewTab(display);
    } else if (gesture_name.compare("OpenChrome") == 0) {
      OpenChrome(display);
    } else if (gesture_name.compare("OpenCNN") == 0) {
      OpenCnn(display);
    } else if (gesture_name.compare("OpenGoogle") == 0) {
      OpenGoogle(display);
    } else if (gesture_name.compare("OpenMusic") == 0) {
      OpenMusic(display);
    } else if (gesture_name.compare("OpenNetflix") == 0) {
      OpenNetflix(display);
    } else if (gesture_name.compare("OpenWeather") == 0) {
      OpenWeather(display);
    } else if (gesture_name.compare("OpenYouTube") == 0) {
      OpenYouTube(display);
    } else if (gesture_name.compare("RefreshTab") == 0) {
      RefreshTab(display);
    } else if (gesture_name.compare("RestoreWindow") == 0) {
      RestoreWindow(display);
    } else if (gesture_name.compare("ScrollDown") == 0) {
      ScrollDown(display);
    } else if (gesture_name.compare("ScrollUp") == 0) {
      ScrollUp(display);
    } else if (gesture_name.compare("ZoomIn") == 0) {
      ZoomIn(display);
    } else if (gesture_name.compare("ZoomOut") == 0) {
      ZoomOut(display);
    } else {
      cout << "Error: Received unexpected gesture: \"" << gesture_name << "\""
          << endl;
    }
  }

  return 0;
}
