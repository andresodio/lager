# Utility functions to prepare LaGeR strings for use with machine learning classifiers

from io import StringIO
import numpy as np
from skimage.transform import resize
import pprint as pp
import math

_GESTURE_LIST = ['CloseTab', 'MaximizeWindow', 'NewTab', 'OpenChrome', 'OpenCNN', 'OpenGoogle', 'OpenMusic', 'OpenNetflix', 'OpenVideos', 'OpenWeather', 'RefreshTab', 'RestoreWindow', 'ScrollUp', 'ScrollDown', 'ZoomIn', 'ZoomOut']
#_GESTURE_LIST = ['CloseTab', 'NewTab', 'OpenChrome', 'OpenCNN', 'OpenGoogle',  'RefreshTab', 'ZoomIn', 'ZoomOut']
#_GESTURE_LIST = ['Triangle', 'Square', 'Circle', 'HorizontalLineRight', 'VerticalLineDown', 'G']
_NUM_CLASSES = len(_GESTURE_LIST)
_NUM_FEATURES = 224
_MAX_FEATURE_VALUE = 26

def convert_lager_to_numbers(gesture):
	new_str = StringIO()
	gesture_lst = list(gesture)
	
	for i, char in enumerate(gesture_lst):
		if char == '0':
			new_str.write('0,')
			continue

		if char == '_':
			new_str.write('0,')
			continue

		if not char.isalpha():
			continue

		new_str.write(str(ord(char) - ord('a') + 1))
		new_str.write(',')

	return new_str.getvalue()[:-1]

def convert_numbers_to_lager(gesture):
	new_str = StringIO()

	for movement in range(len(gesture)):
		for sensor in range(len(gesture[movement])):
			number = gesture[movement][sensor]
			if (number == 0):
				letter = '_'
			else:
				letter = str(chr(ord('a') + number - 1))
			new_str.write(letter)
		new_str.write('.')

	return new_str.getvalue()[:-1]

def expand_gesture_to_target(gesture, target_length, divider):
	#Remove carriage return and extra dividers from end of gesture
	gesture_movements = gesture.strip('\n,.').split(divider)
	gesture_length = len(gesture_movements)
	expansion_factor = int(target_length/gesture_length)
	padding_count = target_length % gesture_length

	new_str = StringIO()

	# Duplicate movements to expand gesture string to nearest multiple
	for movement in gesture_movements:
		for x in range(expansion_factor):
			new_str.write(movement)
			new_str.write(divider)
	
	# Pad with zeroes at the end as needed
	for x in range(padding_count):
		if (divider == '.'):
			# Assumes two-sensor format with two characters per movement
			new_str.write('_')
		new_str.write('0')
		new_str.write(divider)

	return new_str.getvalue()[:-1]

def expand_gesture_num_to_target(gesture, target_length, divider):
	#Remove carriage return and extra dividers from end of gesture
	gesture_movements = gesture.strip('\n,.').split(divider)
	gesture_length = len(gesture_movements)

	gesture_values = np.array(gesture_movements,dtype=np.uint8)
	gesture_values = gesture_values / _MAX_FEATURE_VALUE
	gesture_values = np.reshape(gesture_values, (-1, 2))

	image_resized = resize(gesture_values, (target_length, 2), anti_aliasing=False, order=0, mode='edge')

	new_samples = image_resized * _MAX_FEATURE_VALUE
	new_samples = new_samples.round().astype(int)

	return new_samples
