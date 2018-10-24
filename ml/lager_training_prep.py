#!/usr/bin/env python3

# This program prepares LaGeR strings for use with a machine learning training
# algorithm.
#
# It expands the string or set of strings to specific length (number of
# features), then generates variants for each of those. Finally, it converts
# the variants into numbers and adds the result to a dataset file.

import sys
from subprocess import call

if (len(sys.argv) < 5):
	print("lager_training_prep [GESTURE_NAME] [GESTURE_LABEL] [NUM_FEATURES] [NUM_VARIANTS]")
	exit()

gesture_name = sys.argv[1]
gesture_label = sys.argv[2]
num_features = sys.argv[3]
num_variants = sys.argv[4]

print("Gesture name: ", gesture_name)
print("Gesture label: ", gesture_label)
print("Number of features: ", num_features)
print("Number of variants: ", num_variants)

orig_gesture_filename = gesture_name + ".dat"
gesture_expanded_filename = gesture_name + "_expanded.dat"
gesture_variants_filename = gesture_name + "_expanded_variants.dat"
gesture_numbers_filename = gesture_name + "_expanded_variants_numbers.csv"

call(['./lager_expander.py', 'gestures/' + orig_gesture_filename, num_features])
call(['../lager_generator/lager_generator.py', 'gestures/' + gesture_expanded_filename, num_variants])
call(['./lager_file_to_numbers.py', "gestures/" + gesture_variants_filename, gesture_label])
call('cat ' + 'gestures/' + gesture_numbers_filename + ' >>'+ ' gestures/dataset.csv', shell=True)
