#!/usr/bin/env python3

#@title Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

#@title MIT License
#
# Copyright (c) 2017 François Chollet
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.

# Machine-learning based LaGeR recognizer using TensorFlow DNN classifier
# Partially based on:
#  https://github.com/tensorflow/models/blob/master/samples/core/tutorials/keras/basic_classification.ipynb
#  https://colab.research.google.com/notebooks/mlcc/multi-class_classification_of_handwritten_digits.ipynb

# TensorFlow and tf.keras
import tensorflow as tf
from tensorflow import keras

# Helper libraries
import numpy as np
import pandas as pd
from skimage.transform import resize
import sys
import time

# Custom libraries
from lager_ml_common import _GESTURE_LIST, _NUM_CLASSES, _NUM_FEATURES, _MAX_FEATURE_VALUE, convert_lager_to_numbers, expand_gesture_num_to_target

class_names = _GESTURE_LIST

model = keras.models.load_model('/tmp/lager_model.h5')

# Call classifier with dummy predicition to speed up subsequent calls
dummy_sample = np.array([np.zeros(_NUM_FEATURES)],dtype=np.float32)
predictions_single = model.predict(dummy_sample)

def main(input_gesture = ""):
	while(True):
		single_gesture = 0

		if (len(input_gesture) > 0):
			single_gesture = 1
			if (not input_gesture[0].isdigit()):
				input_gesture = convert_lager_to_numbers(input_gesture)
		elif (len(sys.argv) == 2):
			single_gesture = 1
			input_gesture = sys.argv[1]
			if (not input_gesture[0].isdigit()):
				input_gesture = convert_lager_to_numbers(input_gesture)
		else:
			print("Enter gesture values:")
			input_gesture = input()

		gesture_values = [int(e) for e in input_gesture.strip().split(',')]
		gesture_values = np.array([gesture_values],dtype=np.uint8)

		gesture_values = gesture_values / _MAX_FEATURE_VALUE
		gesture_values.shape = (1, gesture_values.size)

		new_samples = resize(gesture_values, (1, _NUM_FEATURES), anti_aliasing=False, order=0, mode='edge')

		before_time = time.clock()
		prediction = model.predict(new_samples)
		after_time = time.clock()

		print("")
		print("Probabilities")
		print("-------------")
		class_label = 0
		for number in prediction[0]:
			print(" ", _GESTURE_LIST[class_label], ":", round(number * 100, 2),  "%")
			class_label += 1

		print("")

		class_label = np.argmax(prediction[0])
		print("Classified gesture: ", _GESTURE_LIST[class_label])
		print("Elapsed time: ", int(round((after_time-before_time)*1000)), "ms")

		if single_gesture:
			return class_label

if __name__ == "__main__":
    main("")
