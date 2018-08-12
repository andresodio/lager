#!/usr/bin/python3

# Expands LaGeR strings for use with machine learning classifiers

import sys
from io import StringIO
from lager_ml_utils import expand_gesture_to_target

if (len(sys.argv) < 3):
	print("lager_expander [GESTURE_FILE] [TARGET_LENGTH]")
	exit()

orig_gesture_file = open(sys.argv[1], "r");

expanded_filename = sys.argv[1][:-4] + "_expanded.dat"
expanded_file = open(expanded_filename, "w")

target_length = int(sys.argv[2])

for gesture in orig_gesture_file:
	expanded_file.write(expand_gesture_to_target(gesture, target_length, '.') + "\n")

expanded_file.close()
orig_gesture_file.close()
