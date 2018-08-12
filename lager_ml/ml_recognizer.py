#!/usr/bin/env python3

# Licensed under the Apache License, Version 2.0 (the "License");
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

# Machine-learning based LaGeR recognizer using TensorFlow linear and DNN classifiers
# Partially based on the following: https://colab.research.google.com/notebooks/mlcc/multi-class_classification_of_handwritten_digits.ipynb

import numpy as np
import tensorflow as tf
from tensorflow.python.data import Dataset
import sys
import time
from lager_ml_utils import convert_lager_to_numbers, expand_gesture_to_target

num_classes = 3
num_features = 96
max_feature_value = 26

def construct_feature_columns():
  """Construct the TensorFlow Feature Columns.

  Returns:
    A set of feature columns
  """ 
  
  # There are <num_features> movements in each image.
  return set([tf.feature_column.numeric_column('movements', shape=num_features)])

def create_linear_classifier():
	# Create a LinearClassifier object.
	learning_rate = 0.03
	my_optimizer = tf.train.AdagradOptimizer(learning_rate=learning_rate)
	my_optimizer = tf.contrib.estimator.clip_gradients_by_norm(my_optimizer, 5.0)
	
	print("Loading classifier")
	classifier = tf.estimator.LinearClassifier(
		feature_columns=construct_feature_columns(),
		n_classes=num_classes,
		optimizer=my_optimizer,
		config=tf.estimator.RunConfig(keep_checkpoint_max=1),
		model_dir="/tmp/lager_model"
	)

	return classifier

def create_nn_classifier():
	# Create a DNNClassifier object.
	learning_rate = 0.05
	my_optimizer = tf.train.AdagradOptimizer(learning_rate=learning_rate)
	my_optimizer = tf.contrib.estimator.clip_gradients_by_norm(my_optimizer, 5.0)

	classifier = tf.estimator.DNNClassifier(
   		feature_columns=construct_feature_columns(),
    	n_classes=num_classes,
    	hidden_units=[100, 100],
    	optimizer=my_optimizer,
    	config=tf.contrib.learn.RunConfig(keep_checkpoint_max=1),
    	model_dir="/tmp/lager_model"
  	)

	return classifier

# Do fake first pass of classifier for subsequent speed-up
classifier = create_nn_classifier()

predict_input_fn = tf.estimator.inputs.numpy_input_fn(
		  x={"movements": np.array([np.zeros(num_features)],dtype=np.float32)},
		  num_epochs=1,
		  shuffle=False)

predictions = list(classifier.predict(input_fn=predict_input_fn))

while(True):
	single_gesture = 0

	if (len(sys.argv) == 2):
		single_gesture = 1
		input_gesture = sys.argv[1]
		if (not input_gesture[0].isdigit()):
			input_gesture = convert_lager_to_numbers(input_gesture)
			print("input as numbers: ", input_gesture)
			input_gesture = expand_gesture_to_target(input_gesture, num_features, ',')
	else:
		print("Enter gesture values:")
		input_gesture = input()
	
	print("Using input gesture: ", input_gesture)

	gesture_values = [int(e) for e in input_gesture.strip().split(',')]

	new_samples = np.array([gesture_values],dtype=np.float32)
	new_samples = new_samples / max_feature_value

	predict_input_fn = tf.estimator.inputs.numpy_input_fn(
		  x={"movements": new_samples},
		  num_epochs=1,
		  shuffle=False)
	
	before_time = time.clock()
	predictions = list(classifier.predict(input_fn=predict_input_fn))
	after_time = time.clock()

	predicted_classes = [p["classes"] for p in predictions]
	for p in predicted_classes:
	  print("Classified gesture: ", p)

	print("Elapsed time: ", int(round((after_time-before_time)*1000)), "ms")

	if single_gesture:
		break
