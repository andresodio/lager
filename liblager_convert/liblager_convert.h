/*
 * liblager_convert.h
 *
 *  Created on: Mar 2, 2015
 *      Author: andres
 */

#ifndef LIBLAGER_CONVERT_H_
#define LIBLAGER_CONVERT_H_

#include <string>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <boost/thread.hpp>

#include <osvr/ClientKit/Context.h>
#include <osvr/ClientKit/Interface.h>
#include <osvr/ClientKit/Context_decl.h>

using std::string;
using std::stringstream;
using std::chrono::system_clock;
using std::chrono::time_point;

#define DISTANCE_INTERVAL_SQUARED 0.0004f //0.4 * (1 cm)^2

/**
 * Converts the movement of input sensors into LaGeR strings.
 */
class LagerConverter {
 public:
  /**
   * Returns a pointer to an instance of the class.
   * Instantiates the class if needed.
   */
  static LagerConverter* Instance();

  /**
   * Sets the print_updates_ member variable.
   *
   * If true, LaGeR string updates are printed out as they occur.
   */
  void SetPrintUpdates(bool print_updates) {
    print_updates_ = print_updates;
  }

  /**
   * Sets the use_buttons_ member variable.
   *
   * If true, sensor movements are only converted to LaGeR while a button is
   * being pressed.
   */
  void SetUseButtons(bool use_buttons) {
    use_buttons_ = use_buttons;

    // If using buttons, will only draw when button is pressed
    draw_gestures_1_ = !use_buttons;
    draw_gestures_2_ = !use_buttons;
  }

  /**
   * Destructor for this class.
   * Takes care of joining threads and deleting dynamically allocated variables.
   */
  ~LagerConverter() {
    processing_thread_.interrupt();
    processing_thread_.join();
  }

  /**
   * Initializes the movement tracking callbacks and starts processing sensor
   * events.
   */
  void Start() {
    DetectTrackerIndexes();
    InitializeTrackers();
    processing_thread_ = boost::thread(&LagerConverter::ProcessSensorEvents, this);
  }

  /**
   * Stops processing sensor events and deletes the movement tracking objects.
   */
  void Stop() {
    processing_thread_.interrupt();
    processing_thread_.join();
  }

  /**
   * Returns the LaGeR string that corresponds to the latest sensor gesture.
   * If a gesture is underway, this function blocks until the gesture has
   * finished.
   */
  string BlockingGetLagerString();

 private:
  /**
   * Constructor for this class.
   * Initializes the OSVR context.
   */
  LagerConverter() : context_("LagerConverter") {
  }
  ;

  /**
   * Constructor for this class, with a parameter taking a reference to
   * a class instance.
   * Initializes the OSVR context.
   */
  LagerConverter(LagerConverter const&) : context_("LagerConverter") {
  }
  ;

  /**
   * Empty implementation of the operator= for this class.
   */
  LagerConverter& operator=(LagerConverter const&) {
  }
  ;

  /// OSVR context
  osvr::clientkit::ClientContext context_;

  /// Pointer to an instance of this class
  static LagerConverter* instance_;

  /// Mutex variable use to protect access to other variables
  std::mutex mutex_;
  /// Condition variable used to synchronize threads reading and writing
  /// the LaGeR string for the current gesture
  std::condition_variable lager_rw_condition_;

  /// Indicates that a thread has finished reading a LaGeR string
  bool lager_read_complete_ = false;
  /// Indicates that a thread has finished writing a LaGeR string
  bool lager_write_complete_ = false;

  /// The thread that is used for processing sensor events
  boost::thread processing_thread_;
  /// Indicates whether LaGeR string updates are printed out as they occur.
  bool print_updates_ = false;
  /// Indicates whether sensor buttons are used to determine when to convert
  /// movements to LaGeR.
  bool use_buttons_ = false;

  /// Pointer to the current tracker structures
  osvr::clientkit::Interface left_tracker_;
  osvr::clientkit::Interface right_tracker_;

  /// Pointer to the current button structures
  osvr::clientkit::Interface left_button_;
  osvr::clientkit::Interface right_button_;

  /// Pointer to the current pinch structures
  osvr::clientkit::Interface left_pinch_;
  osvr::clientkit::Interface right_pinch_;

  /// Last report for sensor 0
  OSVR_PositionReport last_report_0_;
  /// Last report for sensor 1
  OSVR_PositionReport last_report_1_;

  /// Whether to convert sensor movements to LaGeR or not.
  /// Each variable corresponds to one of the sensors.
  /// We draw whenever at least one of them is activated.
  bool draw_gestures_1_ = false;
  bool draw_gestures_2_ = false;

  /// The LaGeR string for the current sensor movements
  std::stringstream lager_string_;

  /// The last time there was any movement
  time_point<system_clock> global_last_movement_time_ = system_clock::now();

  /// The last time sensor 0 moved
  time_point<system_clock> sensor_0_last_movement_time_ = system_clock::now();
  /// The last time the movement of sensor 0 was grouped with sensor 1
  time_point<system_clock> sensor_0_last_grouped_movement_time_ =
      system_clock::now();

  /// The last time sensor 1 moved
  time_point<system_clock> sensor_1_last_movement_time_ = system_clock::now();
  /// The last time the movement of sensor 1 was grouped with sensor 0
  time_point<system_clock> sensor_1_last_grouped_movement_time_ =
      system_clock::now();

  /// The LaGeR symbol corresponding to the most recent sensor 0 movemennt
  char last_sensor_0_letter_ = '_';
  /// The LaGeR symbol corresponding to the most recent sensor 1 movement
  char last_sensor_1_letter_ = '_';

  /**
   * Detects the OSVR sensor indexes for each tracker.
   */
  void DetectTrackerIndexes();

  /**
   * Registers and initializes the OSVR sensor handlers.
   */
  void InitializeTrackers();

  /**
   * Main loop for processing sensor movement and button events.
   */
  void ProcessSensorEvents();

  /**
   * Callback that handles changes to the sensor positions.
   */
  static void HandleTrackerChange(void * /*userdata*/,
                          const OSVR_TimeValue * time_value,
                          const OSVR_PositionReport *cur_report);

  /**
   * Callback that handles changes to the sensor button states.
   */
  static void HandleButtonChange(void *user_data, const OSVR_TimeValue *time_value,
                        const OSVR_ButtonReport *cur_report);

  /**
   * Callback that handles changes to the sensor pinch states.
   */
  static void HandlePinchChange(void *user_data, const OSVR_TimeValue *time_value,
                        const OSVR_AnalogReport *cur_report);


  /**
   * Dummy callback that handles the first few sensor position changes during
   * initialization.
   */
  static void DummyHandleTrackerChange(void * user_data,
                          const OSVR_TimeValue * time_value,
                          const OSVR_PositionReport *cur_report);

  /**
   * Takes the previous and current sensor data and returns the change in X
   * axis position.
   */
  double GetDeltaX(const OSVR_PositionReport* last_report,
                         const OSVR_PositionReport* cur_report);
  /**
   * Takes the previous and current sensor data and returns the change in Y
   * axis position.
   */
  double GetDeltaY(const OSVR_PositionReport* last_report,
                         const OSVR_PositionReport* cur_report);
  /**
   * Takes the previous and current sensor data and returns the change in Z
   * axis position.
   */
  double GetDeltaZ(const OSVR_PositionReport* last_report,
                         const OSVR_PositionReport* cur_report);

  /**
   * Takes the change in X, Y, and Z Cartesian coordinates and returns the
   * equivalent theta change in spherical coordinates.
   */
  double GetMovementThetaInDegrees(double delta_x, double delta_y,
                                   double delta_z);
  /**
   * Takes the change in X, Y, and Z Cartesian coordinates and returns the
   * equivalent phi change in spherical coordinates.
   */
  double GetMovementPhiInDegrees(double delta_x, double delta_y,
                                 double theta);

  /**
   * Takes an angle and returns that closest angle in 45 degree increments.
   */
  int SnapAngle(double aAngle);

  /**
   * Takes the previous and current sensor data and returns the sum of the
   * square of each X, Y, and Z distance component in Cartesian coordinates.
   */
  double GetDistanceSquared(const OSVR_PositionReport* last_report,
                                  const OSVR_PositionReport* cur_report);

  /**
   * Takes the previous and current sensor data and prints it.
   */
  void PrintSensorCoordinates(const OSVR_PositionReport* last_report,
                              const OSVR_PositionReport* cur_report);

  /**
   * Takes a time and returns the numberof milliseconds elapsed until now.
   */
  int GetMillisecondsUntilNow(const time_point<system_clock> &last_time);
  /**
   * Takes sensor data and a time, and returns the number of milliseconds
   * elapsed between the sensor's last movement and that time.
   */
  int GetMillisecondsSinceTrackerTime(
      const OSVR_TimeValue* time_value, const time_point<system_clock> &last_time);
  /**
   * Takes two time variables and assigns one to the other.
   */
  void UpdateTimePoint(time_point<system_clock>& time_to_update,
                       time_point<system_clock> new_time);
  /**
   * Takes sensor data and returns its movement time.
   */
  time_point<system_clock> GetCurrentMovementTime(
        const OSVR_TimeValue* time_value);

  /**
   * Returns whether the current gesture has paused or not.
   */
  bool GesturePaused();

  /**
   * Clears the global LaGeR gesture stringstream.
   */
  void ResetLagerString();

  /**
   * Takes the current and previous sensor data and calculates the position
   * change in X, Y, and Z Cartesian coordinates.
   */
  void CalculateMovementDeltas(const OSVR_PositionReport* cur_report,
                               const OSVR_PositionReport* last_report,
                               double& delta_x, double& delta_y,
                               double& delta_z);
  /**
   * Takes the current and previous sensor data and calculates the movement
   * angles in theta and phi spherical coordinates.
   */
  void CalculateMovementAngles(double& theta, double& phi, int& snap_theta,
                               int& snap_phi, double delta_x,
                               double delta_y, double delta_z);

  /**
   * Moves the global LaGeR stringstream head back to the beginning of a
   * letter pair.
   */
  void MoveHeadToBeginningOfLetterPair();

  /**
   * Takes theta and phi spherical coordinates and returns the corresponding
   * LaGeR symbol.
   */
  char GetCurrentLetter(int snap_theta, int snap_phi);

  /**
   * Takes the current sensor data and the theta and phi spherical coordinates
   * corresponding to its movement, and update the global LaGeR string with
   * the corresponding symbol.
   */
  void UpdateLagerString(const OSVR_PositionReport* cur_report,
                         const OSVR_TimeValue* time_value, const int snap_theta,
                         const int snap_phi);

  /**
   * Takes the current sensor data structure and updates the global movement
   * timers.
   */
  void UpdateTimers(const OSVR_PositionReport* cur_report, const OSVR_TimeValue* time_value);

};

#endif /* LIBLAGER_CONVERT_H_ */
