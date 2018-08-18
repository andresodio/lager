#!/usr/bin/python3

# Expands LaGeR strings for use with machine learning classifiers

import sys
from io import StringIO
from lager_ml_common import num_features, convert_lager_to_numbers, convert_numbers_to_lager, expand_gesture_num_to_target

if (len(sys.argv) < 3):
	print("lager_expander [GESTURE_FILE] [TARGET_LENGTH]")
	exit()

orig_gesture_file = open(sys.argv[1], "r");

expanded_filename = sys.argv[1][:-4] + "_expanded.dat"
expanded_file = open(expanded_filename, "w")

target_length = int(sys.argv[2])

for gesture in orig_gesture_file:
	gesture_num = convert_lager_to_numbers(gesture)
	expanded_num = expand_gesture_num_to_target(gesture_num, num_features, ',')
	expanded_file.write(convert_numbers_to_lager(expanded_num) + "\n")

expanded_file.close()
orig_gesture_file.close()
