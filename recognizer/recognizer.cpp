// phan_client.C - simplest example: generates a flat horizontal plane

#include <stdio.h>                      // for printf, NULL
#include <math.h>						// for atan2
#include <time.h>						// for nanosleep
#include <algorithm>						// for std::min_element
#include <iostream>						// for std::cout
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <chrono>

#include <vrpn_Button.h>                // for vrpn_BUTTONCB, etc
#include <vrpn_Tracker.h>               // for vrpn_TRACKERCB, etc

#include "vrpn_Configure.h"             // for VRPN_CALLBACK
#include "vrpn_Connection.h"            // for vrpn_Connection
#include "vrpn_ForceDevice.h"           // for vrpn_ForceDevice_Remote, etc
#include "vrpn_Types.h"                 // for vrpn_float64

#include "spherical_coordinates.hpp"
#include "coordinates_letter.hpp"
#include "math_utilities.hpp"
#include "string_tokenizer.hpp"

#define RECOGNIZER_ERROR -1
#define RECOGNIZER_NO_ERROR 0

#define SENSOR_SERVER "Tracker0@localhost"

#define DISTANCE_INTERVAL_SQUARED 0.0004f //0.4 * (1 cm)^2

#define MAIN_SLEEP_INTERVAL_MICROSECONDS 1000 // 1ms
#define MAIN_SLEEP_INTERVAL_MILLISECONDS MAIN_SLEEP_INTERVAL_MICROSECONDS/1000
#define GESTURE_PAUSE_TIME_MILLISECONDS 500
#define GESTURE_GROUPING_TIME_MILLISECONDS 200
#define GESTURE_DISTANCE_THRESHOLD_PCT 25

using namespace std;

/* Forward declarations */
int snapAngle(double angle);

/* Globals */

static vrpn_TRACKERCB lastReport0; // last report for sensor 0
static vrpn_TRACKERCB lastReport1; // last report for sensor 1

stringstream gestureString;

chrono::system_clock::time_point globalLastMovementTime = chrono::system_clock::now();

chrono::system_clock::time_point sensor0LastMovementTime = chrono::system_clock::now();
chrono::system_clock::time_point sensor0LastGroupedMovementTime = chrono::system_clock::now();

chrono::system_clock::time_point sensor1LastMovementTime = chrono::system_clock::now();
chrono::system_clock::time_point sensor1LastGroupedMovementTime = chrono::system_clock::now();

char lastSensor0Letter = '_';
char lastSensor1Letter = '_';

vrpn_float64 getDeltaX(const vrpn_TRACKERCB& lastReport, const vrpn_TRACKERCB& aTracker) {
	return (lastReport.pos[1] - aTracker.pos[1]);
}

vrpn_float64 getDeltaY(const vrpn_TRACKERCB& lastReport, const vrpn_TRACKERCB& aTracker) {
	return (aTracker.pos[0] - lastReport.pos[0]);
}

vrpn_float64 getDeltaZ(const vrpn_TRACKERCB& lastReport, const vrpn_TRACKERCB& aTracker) {
	return (aTracker.pos[2] - lastReport.pos[2]);
}

double getMovementThetaInDegrees(vrpn_float64 aDeltaX, vrpn_float64 aDeltaY, vrpn_float64 aDeltaZ) {
	double movementTheta = acos(aDeltaZ/(sqrt(pow(aDeltaX, 2.0) + pow(aDeltaY, 2.0) + pow(aDeltaZ, 2.0)))) * (180.0 / M_PI);
	return fmod(movementTheta + 360.0, 360.0); // Always returns a positive angle
}

double getMovementPhiInDegrees(vrpn_float64 aDeltaX, vrpn_float64 aDeltaY, double aTheta) {
	int snapTheta = snapAngle(aTheta);
	if (snapTheta == 0 || snapTheta == 180) {
		return 0; // Phi becomes meaningless at the north and south poles, so we default it to 0.
	}

	double movementPhi = atan2(aDeltaY, aDeltaX) * (180.0 / M_PI);
	return fmod(movementPhi + 360.0, 360.0); // Always returns a positive angle
}

/* Snaps angle to 45 degree intervals */
int snapAngle(double aAngle) {
	return 45 * (round(aAngle/45.0));
}

vrpn_float64 getDistanceSquared(const vrpn_TRACKERCB& aLastReport, const vrpn_TRACKERCB& aTracker) {
	return (aLastReport.pos[0] - aTracker.pos[0]) * (aLastReport.pos[0] - aTracker.pos[0])
		 + (aLastReport.pos[1] - aTracker.pos[1]) * (aLastReport.pos[1] - aTracker.pos[1])
		 + (aLastReport.pos[2] - aTracker.pos[2]) * (aLastReport.pos[2] - aTracker.pos[2]);
}

#define abs(x) ((x)<0 ? -(x) : (x))

void printSensorCoordinates(const vrpn_TRACKERCB& aLastReport, const vrpn_TRACKERCB& aTracker) {
	printf("\nSensor %d is now at (%g,%g,%g)\n", aTracker.sensor, aTracker.pos[0], aTracker.pos[1], aTracker.pos[2]);
	printf("old/new X: %g, %g\n", aLastReport.pos[1], aTracker.pos[1]);
	printf("old/new Y: %g, %g\n", aLastReport.pos[0], aTracker.pos[0]);
	printf("old/new Z: %g, %g\n", aLastReport.pos[2], aTracker.pos[2]);
}

char getCurrentLetter(int aSnapPhi, int aSnapTheta) {
	struct sphericalCoordinates currentCoordinates;
	currentCoordinates.mPhi = aSnapPhi;
	currentCoordinates.mTheta = aSnapTheta;
	return coordinatesLetter[currentCoordinates];
}

int getMillisecondsSinceNow(const chrono::system_clock::time_point &aLastTime)
{
	std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
	return chrono::duration<double, std::milli>(now - aLastTime).count();
}

int getMillisecondsSinceTrackerTime(const vrpn_TRACKERCB& aTracker, const chrono::system_clock::time_point &aLastTime)
{
	chrono::system_clock::time_point currentMovementTime(chrono::seconds(aTracker.msg_time.tv_sec) + chrono::microseconds(aTracker.msg_time.tv_usec));
	return chrono::duration<double, std::milli>(currentMovementTime - aLastTime).count();
}

void updateTimePoint(chrono::system_clock::time_point& aTimePoint, chrono::system_clock::time_point aNewTime) {
	aTimePoint = aNewTime;
}

bool gesturePaused() {
	return (getMillisecondsSinceNow(globalLastMovementTime) > GESTURE_PAUSE_TIME_MILLISECONDS);
}

void resetGestureString() {
	gestureString.str("");
}

void calculateMovementDeltas(const vrpn_TRACKERCB& aTracker, const vrpn_TRACKERCB* aLastReport,
								vrpn_float64& aDeltaX,
								vrpn_float64& aDeltaY,
								vrpn_float64& aDeltaZ) {
	aDeltaX = getDeltaX(*aLastReport, aTracker);
	aDeltaY = getDeltaY(*aLastReport, aTracker);
	aDeltaZ = getDeltaZ(*aLastReport, aTracker);
	//printf("X delta: %g, Y delta: %g, Z delta: %g\n", deltaX, deltaY, deltaZ);
}

void calculateMovementAngles(double& aTheta, double& aPhi, int& aSnapTheta, int& aSnapPhi,
								vrpn_float64 aDeltaX,
								vrpn_float64 aDeltaY,
								vrpn_float64 aDeltaZ) {
	aTheta = getMovementThetaInDegrees(aDeltaX, aDeltaY, aDeltaZ);
	aSnapTheta = snapAngle(aTheta);
	aPhi = getMovementPhiInDegrees(aDeltaX, aDeltaY, aTheta);
	aSnapPhi = snapAngle(aPhi);
}

void moveHeadToBeginningOfLetterPair() {
	if (gestureString.str().size() >= 3) {
		gestureString.seekp(-3, gestureString.cur);
	}
}

chrono::system_clock::time_point getCurrentMovementTime(const vrpn_TRACKERCB& aTracker) {
	chrono::system_clock::time_point currentMovementTime(chrono::seconds(aTracker.msg_time.tv_sec) + chrono::microseconds(aTracker.msg_time.tv_usec));
	return currentMovementTime;
}

void updateGestureString(const vrpn_TRACKERCB& aTracker, const int aSnapTheta, const int aSnapPhi) {
	int timeSinceSensor0LastMovement = 0;
	int timeSinceSensor1LastMovement = 0;
	char currentLetter = getCurrentLetter(aSnapPhi, aSnapTheta);

	if (aTracker.sensor == 0) {
		lastSensor0Letter = currentLetter;
		timeSinceSensor1LastMovement = getMillisecondsSinceTrackerTime(aTracker, sensor1LastMovementTime);

		/*
		 * If sensor 1 reported new movements since the last time it was grouped,
		 * and if it moved not too long ago, group its letter with sensor 0's.
		 */
		if ((sensor1LastGroupedMovementTime != sensor1LastMovementTime)
				&& (timeSinceSensor1LastMovement < GESTURE_GROUPING_TIME_MILLISECONDS)) {
			//printf("S0: Grouping with previous S1. Current: %c, last: %c\n", currentLetter, lastSensor1Letter);
			sensor0LastGroupedMovementTime = getCurrentMovementTime(aTracker);
			sensor1LastGroupedMovementTime = sensor1LastMovementTime;
			moveHeadToBeginningOfLetterPair();
			gestureString << currentLetter << lastSensor1Letter;
		} else {
			//printf("S0 ELSE. GroupingT=MovementT?: %i, tSS1: %i\n", sensor1LastGroupedMovementTime == sensor1LastMovementTime, timeSinceSensor1);
			lastSensor1Letter = '_';
			gestureString << currentLetter << lastSensor1Letter;
		}
	} else {
		lastSensor1Letter = currentLetter;
		timeSinceSensor0LastMovement = getMillisecondsSinceTrackerTime(aTracker, sensor0LastMovementTime);

		/*
		 * If sensor 0 reported new movements since the last time it was grouped,
		 * and if it moved not too long ago, group its letter with sensor 1's.
		 */
		if ((sensor0LastGroupedMovementTime != sensor0LastMovementTime)
				&& (timeSinceSensor0LastMovement < GESTURE_GROUPING_TIME_MILLISECONDS)) {
			//printf("S1: Grouping with previous S0. Current: %c, last: %c\n", currentLetter, lastSensor0Letter);
			sensor0LastGroupedMovementTime = sensor0LastMovementTime;
			sensor1LastGroupedMovementTime = getCurrentMovementTime(aTracker);
			moveHeadToBeginningOfLetterPair();
			gestureString << lastSensor0Letter << currentLetter;
		} else {
			//printf("S1 ELSE. GroupingT=MovementT?: %i, tSS0: %i\n", sensor0LastGroupedMovementTime == sensor0LastMovementTime, timeSinceSensor0);
			lastSensor0Letter = '_';
			gestureString << lastSensor0Letter << currentLetter;
		}
	}
	gestureString << ".";

	cout << "Gesture: " << gestureString.str() << endl;
	cout << endl;
}

void updateTimers(const vrpn_TRACKERCB& aTracker) {
	chrono::system_clock::time_point currentMovementTime = getCurrentMovementTime(aTracker);
	updateTimePoint(globalLastMovementTime, currentMovementTime);

	if (aTracker.sensor == 0) {
		updateTimePoint(sensor0LastMovementTime, currentMovementTime);
	} else {
		updateTimePoint(sensor1LastMovementTime, currentMovementTime);
	}

	//printf("Updated. GLMT: %i, S0LMT: %i, S1LMT: %i\n", getMillisecondsSinceTrackerTime(aTracker, globalLastMovementTime), getMillisecondsSinceTrackerTime(aTracker, sensor0LastMovementTime), getMillisecondsSinceTrackerTime(aTracker, sensor1LastMovementTime));
}

#define d(i,j) dd[(i) * (m+2) + (j) ]
#define min(x,y) ((x) < (y) ? (x) : (y))
#define min3(a,b,c) ((a)< (b) ? min((a),(c)) : min((b),(c)))
#define min4(a,b,c,d) ((a)< (b) ? min3((a),(c),(d)) : min3((b),(c),(d)))

int dprint(int* dd, int n,int m) {
	int i,j;
	for (i=0; i < n+2;i++){
		for (j=0;j < m+2; j++){
			printf("%02d ",d(i,j));
		}
		printf("\n");
	}
	printf("\n");
	return 0;
}

int dldist2(const char *s, const char* t, int n, int m){
	int *dd, *DA;
	int i, j, cost, k, i1,j1,DB;
	int infinity = n + m;

	DA = (int*) malloc( 256 * sizeof(int));
	dd = (int*) malloc ((n+2)*(m+2)*sizeof(int));

	d(0,0) = infinity;
	for(i = 0; i < n+1; i++) {
		d(i+1,1) = i ;
		d(i+1,0) = infinity;
	}
	for(j = 0; j<m+1; j++) {
		d(1,j+1) = j ;
		d(0,j+1) = infinity;
	}
	//dprint(dd,n,m);
	for(k = 0; k < 256; k++) DA[k] = 0;
	for(i = 1; i< n+1; i++) {
		DB = 0;
		for(j = 1; j< m+1; j++) {
			i1 = DA[t[j-1]];
			j1 = DB;
			cost = ((s[i-1]==t[j-1])?0:1);
			if(cost==0) DB = j;
			d(i+1,j+1) =
					min4(d(i,j)+cost,
						 d(i+1,j) + 1,
						 d(i,j+1)+1,
						 d(i1,j1) + (i-i1-1) + 1 + (j-j1-1));
		}
		DA[s[i-1]] = i;
		//dprint(dd,n,m);
	}
	cost = d(n+1,m+1);
	free(dd);
	return cost;
}

/* New size must be a multiple of the original string size */
string expandString(const string& aInputString, int aNewSize)
{
	stringstream outputString;
	int lengthMultiplier = aNewSize / aInputString.length();

	vector<string> movementPairs;
	tokenizeString(aInputString, movementPairs, ".");

	for (vector<string>::iterator it = movementPairs.begin(); it < movementPairs.end(); ++it) {
		for (int i = 0; i < lengthMultiplier; i++) {
			outputString << *it << ".";
		}
	}

	return outputString.str();
}

struct gestureEntry {
	string name;
	string movements;
	int dlDistance;
	float dlDistancePct;
};

bool gestureEntryLessThan (gestureEntry i, gestureEntry j)
{
	return (i.dlDistancePct < j.dlDistancePct);
}

int readAndCompareGesturesFromFile(vector<gestureEntry>& aGestures)
{
	ifstream gestureFile;
	string currentLine;

	cout << "Current gesture" << endl << "\t" << gestureString.str() << endl << endl;;

	gestureFile.open("gestures.dat");
	if (!gestureFile.is_open()) {
		return RECOGNIZER_ERROR;
	}

	while (getline(gestureFile, currentLine)) {
		//cout << "Line: " << currentLine << endl;
		stringstream ss(currentLine);
		const string& inputGesture = gestureString.str();

		gestureEntry newGestureEntry;
		ss >> newGestureEntry.name >> newGestureEntry.movements;

		int gestureLengthLeastCommonMultiple = leastCommonMultiple(inputGesture.length(), newGestureEntry.movements.length());
		string expandedInputString = expandString(inputGesture, gestureLengthLeastCommonMultiple);
		string expandedNewGestureEntryString = expandString(newGestureEntry.movements, gestureLengthLeastCommonMultiple);

		newGestureEntry.dlDistance = dldist2(expandedInputString.c_str(), expandedNewGestureEntryString.c_str(), expandedInputString.length(), expandedNewGestureEntryString.length());
		newGestureEntry.dlDistancePct = (newGestureEntry.dlDistance * 100.0f) / expandedNewGestureEntryString.length();

		cout << newGestureEntry.name << endl << "\t" << newGestureEntry.movements << endl;
		cout << "Distance: " << newGestureEntry.dlDistancePct << "% (" << newGestureEntry.dlDistance << " D-L ops)" << endl;
		cout << endl;

		aGestures.push_back(newGestureEntry);
	}

	return RECOGNIZER_NO_ERROR;
}

void drawMatchingGestures(const gestureEntry& closestGesture) {
	stringstream viewerCommand;
	string viewerCommandPrefix =
			"cd ~/lager/viewer/src/ && ../build/viewer --gesture ";
	string hideOutputSuffix = " > /dev/null";

	cout << "Drawing input gesture..." << endl;

	viewerCommand << viewerCommandPrefix << gestureString.str()
			<< hideOutputSuffix;
	system(viewerCommand.str().c_str());

	cout << "Drawing gesture match..." << endl;

	viewerCommand.str("");
	viewerCommand << viewerCommandPrefix << closestGesture.movements
			<< hideOutputSuffix;
	system(viewerCommand.str().c_str());
}

void recognizeGesture()
{
	vector<gestureEntry> gestures;
	int lowestDistance;

	cout << " ________________________________ " << endl;
	cout << "|                                |" << endl;
	cout << "|         RECOGNIZING...         |" << endl;
	cout << "|________________________________|" << endl;
	cout << "                                  " << endl;

	if (readAndCompareGesturesFromFile(gestures) != RECOGNIZER_NO_ERROR) {
		cout << "Error reading gestures from file." << endl;
		return;
	}

	gestureEntry closestGesture = *min_element(gestures.begin(), gestures.end(), gestureEntryLessThan);
	cout << endl;
	cout << "Closest gesture:\t" << closestGesture.name << endl;
	cout << "Distance:\t\t" << closestGesture.dlDistancePct << "% (" << closestGesture.dlDistance << " D-L ops)" << endl;
	cout << "Threshold:\t\t" << GESTURE_DISTANCE_THRESHOLD_PCT << "%" << endl;

	if (closestGesture.dlDistancePct <= GESTURE_DISTANCE_THRESHOLD_PCT) {
		cout << " ________________________________ " << endl;
		cout << "|                                |" << endl;
		cout << "|          MATCH FOUND!          |" << endl;
		cout << "|________________________________|" << endl;
		cout << "                                  " << endl;
		drawMatchingGestures(closestGesture);
	} else {
		cout << "NO MATCH." << endl;
	}

	cout << endl << endl;;
}

/*****************************************************************************
 *
   Callback handler
 *
 *****************************************************************************/

void VRPN_CALLBACK handle_tracker_change(void *aUserdata, const vrpn_TRACKERCB aTracker)
{
	vrpn_TRACKERCB *lastReport; // keep track of the current sensor's last report
	vrpn_float64 deltaX, deltaY, deltaZ;
	double theta, phi;
	int snapTheta, snapPhi;
	static float dist_interval_sq = DISTANCE_INTERVAL_SQUARED;

	if (aTracker.sensor == 0) {
		lastReport = &lastReport0;
	} else {
		lastReport = &lastReport1;
	}

	if (getDistanceSquared(*lastReport, aTracker) > dist_interval_sq) {
		printf("Update for sensor: %i at time: %ld.%06ld\n", aTracker.sensor, aTracker.msg_time.tv_sec, aTracker.msg_time.tv_usec);
		//printf("GLMT: %i, S0LMT: %i, S1LMT: %i\n", getMillisecondsSinceTrackerTime(aTracker, globalLastMovementTime), getMillisecondsSinceTrackerTime(aTracker, sensor0LastMovementTime), getMillisecondsSinceTrackerTime(aTracker, sensor1LastMovementTime));
		//printSensorCoordinates(lastReport, aTracker);

		updateTimers(aTracker);

		calculateMovementDeltas(aTracker, lastReport, deltaX, deltaY, deltaZ);
		calculateMovementAngles(theta, phi, snapTheta, snapPhi, deltaX, deltaY, deltaZ);

		updateGestureString(aTracker, snapTheta, snapPhi);

		*lastReport = aTracker;
	}
}

void VRPN_CALLBACK handle_button_change(void *aUserdata, const vrpn_BUTTONCB aButton)
{
	static int count=0;
	static int buttonstate = 1;
	int done = 0;

	if (aButton.state != buttonstate) {
		printf("button #%d is in state %d\n", aButton.button, aButton.state);
		buttonstate = aButton.state;
		count++;
	}
	if (count > 4)
		done = 1;
	*(int *)aUserdata = done;
}

void VRPN_CALLBACK dummy_handle_tracker_change(void *aUserdata, const vrpn_TRACKERCB aTracker)
{
	//printf("DUMMY CALLED for sensor %i!\n", aTracker.sensor);
	if (aTracker.sensor == 0) {
		lastReport0 = aTracker;
	} else {
		lastReport1 = aTracker;
	}

	int *numReportsReceived = (int *)aUserdata;
	*numReportsReceived = *numReportsReceived + 1;
}

int main(int argc, char *argv[])
{
	printf("Generates strings for movement of tracker %s\n\n", SENSOR_SERVER);

	int numReportsReceived = 0;
	int done = 0;
	vrpn_Tracker_Remote *tracker;
	vrpn_Button_Remote *button;
	struct timespec sleepInterval = {0, MAIN_SLEEP_INTERVAL_MICROSECONDS};

	// initialize the tracker
	tracker = new vrpn_Tracker_Remote(SENSOR_SERVER);

	// initialize the button
	button = new vrpn_Button_Remote(SENSOR_SERVER);

	tracker->register_change_handler(&numReportsReceived, dummy_handle_tracker_change);
	while (numReportsReceived != 2) {
		nanosleep(&sleepInterval, NULL);
		tracker->mainloop();
	}

	tracker->unregister_change_handler(&numReportsReceived, dummy_handle_tracker_change);
	tracker->register_change_handler(NULL, handle_tracker_change);
	button->register_change_handler(&done, handle_button_change);

	cout << " ________________________________ " << endl;
	cout << "|                                |" << endl;
	cout << "|       COLLECTING DATA...       |" << endl;
	cout << "|________________________________|" << endl;
	cout << "                                  " << endl;

	// Main loop
	while (! done ) {

		// Let tracker receive position information from remote tracker
		tracker->mainloop();
		// Let button receive button status from remote button
		button->mainloop();

		// If gesture buildup pauses, attempt to recognize it
		int gestureStringLength = gestureString.str().length();
		if (gesturePaused() && gestureStringLength > 0) {
			/*
			 * Only try to recognize gestures with more than one movement pair.
			 * This reduces spurious recognitions from inadvertent movements.
			 */
			if (gestureStringLength > 3) {
				recognizeGesture();
			}

			cout << " ________________________________ " << endl;
			cout << "|                                |" << endl;
			cout << "|       COLLECTING DATA...       |" << endl;
			cout << "|________________________________|" << endl;
			cout << "                                  " << endl;

			resetGestureString();
		}

		// Sleep so we don't take up 100% of CPU
		nanosleep(&sleepInterval, NULL);
	}

}	/* main */
