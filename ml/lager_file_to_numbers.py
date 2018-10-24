#!/usr/bin/python3

# Converts all LaGeR strings in a file to number format for use with machine learning classifiers.
# Prefixes each string with a class label.
# Currently keeps only one character per movement (equivalent to single-sensor gestures) 

import sys
from lager_ml_common import convert_lager_to_numbers

if (len(sys.argv) < 3):
	print("lager_file_to_numbers [GESTURE_FILE] [GESTURE_LABEL]")
	exit()

orig_gesture_file = open(sys.argv[1], "r");

numbers_filename = sys.argv[1][:-4] + "_numbers.csv"
numbers_file = open(numbers_filename, "w")

for gesture in orig_gesture_file:
	numbers_file.write(sys.argv[2] + "," + convert_lager_to_numbers(gesture) + "\n")

numbers_file.close()
orig_gesture_file.close()
