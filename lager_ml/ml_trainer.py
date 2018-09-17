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
import time
from skimage.transform import resize

# Custom libraries
from lager_ml_common import _GESTURE_LIST, _NUM_CLASSES, _NUM_FEATURES, _MAX_FEATURE_VALUE

# Read dataset into dataframe
dataframe = pd.read_csv(
  "dataset_shuffled.csv",
  sep=",",
  header=None)

dataframe = dataframe.reindex(np.random.permutation(dataframe.index))
dataframe.head()

def parse_labels_and_features(dataset):
  """Extracts labels and features.
  
  This is a good place to scale or transform the features if needed.
  
  Args:
    dataset: A Pandas `Dataframe`, containing the label on the first column and
      movement values on the remaining columns, in row major order.
  Returns:
    A `tuple` `(labels, features)`:
      labels: A Pandas `Series`.
      features: A Pandas `DataFrame`.
  """
  labels = dataset[0]

  # DataFrame.loc index ranges are inclusive at both ends.
  features = dataset.loc[:,1:]
  # Scale the data to [0, 1] by dividing out the max value, <_MAX_FEATURE_VALUE>.
  features = features / _MAX_FEATURE_VALUE
  # Reshape the data to a multidimensional array with 2 dimensions (sensors) per feature (movement)
  features = features.values.reshape(-1, _NUM_FEATURES, 2)

  return labels, features

train_labels, train_features = parse_labels_and_features(dataframe[:int(len(dataframe)*3/4)])
test_labels, test_features = parse_labels_and_features(dataframe[int(len(dataframe)*3/4):len(dataframe)])

num_classes = len(_GESTURE_LIST)

model = keras.Sequential([
	keras.layers.Flatten(input_shape=(_NUM_FEATURES,2)),
    keras.layers.Dense(100, activation=tf.nn.relu),
	keras.layers.Dense(100, activation=tf.nn.relu),
    keras.layers.Dense(num_classes, activation=tf.nn.softmax)
])

model.compile(optimizer=tf.keras.optimizers.SGD(lr=0.001, decay=0.0001),
              loss='sparse_categorical_crossentropy',
              metrics=['accuracy'])

model.fit(train_features, train_labels, epochs=20, validation_data=(test_features, test_labels))

test_loss, test_acc = model.evaluate(test_features, test_labels)

print('Test accuracy:', test_acc)

model.summary()

model.save('/tmp/lager_model.h5')  # creates a HDF5 file 'my_model.h5'
