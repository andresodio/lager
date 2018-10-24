#!/usr/bin/env python3

# This program prepares a batch of LaGeR strings for use with a machine learning
# training algorithm.
#
# It does so by repeatedly calling lager_training_prep.py for each one

import sys
from subprocess import call

if (len(sys.argv) < 4):
	print("lager_training_prep_batch [GESTURES_FILENAME] [NUM_FEATURES] [NUM_VARIANTS]")
	exit()

gestures_filename = sys.argv[1]
num_features = sys.argv[2]
num_variants = sys.argv[3]

print("Gestures filename: ", gestures_filename)
print("Number of features: ", num_features)
print("Number of variants: ", num_variants)

call('rm gestures/*_*.dat', shell=True)
call('rm gestures/*_*.csv', shell=True)
call('rm gestures/dataset*', shell=True)

gestures_file = open(gestures_filename, "r");

gesture_label = 0
for line in gestures_file:
	gesture_name = line.split(' ', 1)[0]
	call(['./lager_training_prep.py', gesture_name, str(gesture_label), num_features, num_variants])
	gesture_label += 1

gestures_file.close()

call('shuf gestures/dataset.csv -o gestures/dataset_shuffled.csv', shell=True)
