# Utility functions to prepare LaGeR strings for use with machine learning classifiers

from io import StringIO

def convert_lager_to_numbers(gesture):
	new_str = StringIO()
	gesture_lst = list(gesture)
	
	for i, char in enumerate(gesture_lst):
		if char == '0':
			new_str.write('0,')
			continue

		if not char.isalpha():
			continue

		new_str.write(str(ord(char) - ord('a') + 1))
		new_str.write(',')

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
