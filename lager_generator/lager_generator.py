#!/usr/bin/env python3

# This program generates new gesture strings based on an input gesture string.
#
# It creates new variants by using the Monte Carlo method. The input gesture
# string is used as the model, and a normal distribution around each literal
# in the model string is used to replace the literal by either itself or one
# of its nearest neighbors in 3D space. 

import random
import sys

if (len(sys.argv) < 3):
	print("lager_training_generator [GESTURE_FILE] [NUM_VARIANTS]")
	exit()

def generate_variants(gesture_str):

	gesture_lst = list(gesture_str)
	
	for i, char in enumerate(gesture_lst):
		if not char.isalpha():
			continue

		mean = 5
		std_dev = 1.0
		choice = random.normalvariate(mean,std_dev)
	   
		# If we are within 1 std_dev of the mean, keep the
		# original character.
		# Otherwise, pick a neighbor character at random.
		if ((mean - std_dev) < choice < (mean + std_dev)):
			continue
		else:
			if (char == 'a'):
				neighbors = ['b', 'c', 'd', 'e', 'f', 'g', 'h', 'i']
			elif (char == 'b'):
				neighbors = ['h', 'a', 'd', 'i', 'c', 'q', 'j', 'k']
			elif (char == 'c'):
				neighbors = ['a', 'b', 'd', 'j', 'k', 'l']
			elif (char == 'd'):
				neighbors = ['b', 'a', 'f', 'c', 'e', 'k', 'l', 'm']
			elif (char == 'e'):
				neighbors = ['a', 'd', 'f', 'l', 'm', 'n']
			elif (char == 'f'):
				neighbors = ['d', 'a', 'h', 'e', 'g', 'm', 'n', 'o']
			elif (char == 'g'):
				neighbors = ['a', 'f', 'h', 'n', 'o', 'p']
			elif (char == 'h'):
				neighbors = ['f', 'a', 'b', 'g', 'i', 'o', 'p', 'q']
			elif (char == 'i'):
				neighbors = ['a', 'h', 'b', 'p', 'q', 'j']
			elif (char == 'j'):
				neighbors = ['i', 'b', 'c', 'q', 'k', 'y', 'r', 's']
			elif (char == 'k'):
				neighbors = ['b', 'c', 'd', 'j', 'l', 'r', 's', 't']
			elif (char == 'l'):
				neighbors = ['c', 'd', 'e', 'k', 'm', 's', 't', 'u']
			elif (char == 'm'):
				neighbors = ['d', 'e', 'f', 'l', 'n', 't', 'u', 'v']
			elif (char == 'n'):
				neighbors = ['e', 'f', 'g', 'm', 'o', 'u', 'v', 'w']
			elif (char == 'o'):
				neighbors = ['f', 'g', 'h', 'n', 'p', 'v', 'w', 'x']
			elif (char == 'p'):
				neighbors = ['g', 'h', 'i', 'o', 'q', 'w', 'x', 'y']
			elif (char == 'q'):
				neighbors = ['h', 'i', 'b', 'p', 'j', 'x', 'y', 'r']
			elif (char == 'r'):
				neighbors = ['q', 'j', 'k', 'y', 's', 'x', 'z', 't']
			elif (char == 's'):
				neighbors = ['j', 'k', 'l', 'r', 't', 'z']
			elif (char == 't'):
				neighbors = ['k', 'l', 'm', 's', 'u', 'r', 'z', 'v']
			elif (char == 'u'):
				neighbors = ['l', 'm', 'n', 't', 'v', 'z']
			elif (char == 'v'):
				neighbors = ['m', 'n', 'o', 'u', 'w', 't', 'z', 'x']
			elif (char == 'w'):
				neighbors = ['n', 'o', 'p', 'v', 'x', 'z']
			elif (char == 'x'):
				neighbors = ['o', 'p', 'q', 'w', 'y', 'v', 'z', 'r']
			elif (char == 'y'):
				neighbors = ['p', 'q', 'j', 'x', 'r', 'z']
			elif (char == 'z'):
				neighbors = ['r', 's', 't', 'u', 'v', 'w', 'x', 'y']
			else:
				print('Error: Incorrect character found, ' + char)
				exit()

			gesture_lst[i] = random.choice(neighbors)

	gesture_str = ''.join(gesture_lst)
	variants_file.write(gesture_str)

orig_gesture_file = open(sys.argv[1], "r");
variants_filename = sys.argv[1][:-4] + "_variants.dat"
variants_file = open(variants_filename, "w")

for gesture in orig_gesture_file:
	#print("Generating " +  sys.argv[2] + " variants for gesture: " + gesture)
	for x in range (0, int(sys.argv[2])):
		generate_variants(gesture)

orig_gesture_file.close()
variants_file.close()
