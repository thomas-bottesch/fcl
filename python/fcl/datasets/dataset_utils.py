from __future__ import print_function
import os

try:
  # Python 2
  from urllib import urlretrieve
except:
  # Python 3
  from urllib.request import urlretrieve
  
import traceback

def load_dataset_from_github(path_to_save_to, dataset_filename):
  dataset_path = os.path.join(path_to_save_to, dataset_filename)
  
  if os.path.isfile(dataset_path):
    return dataset_path
  
  try:
    print("Downloading dataset to:", dataset_path)
    urlretrieve("https://github.com/thomas-bottesch/fcl_datasets/blob/master/%s?raw=true"%dataset_filename, dataset_path)
  except:
    print("Error while downloading dataset: " + dataset_filename)
    print(traceback.format_exc())
  
  return dataset_path

def load_sector_dataset(path_to_save_to):
  return load_dataset_from_github(path_to_save_to, 'sector.scaled')

def load_usps_dataset(path_to_save_to):
  return load_dataset_from_github(path_to_save_to, 'usps.scaled')
  
def load_example_dataset(path_to_save_to):
  return load_dataset_from_github(path_to_save_to, 'example.scaled')