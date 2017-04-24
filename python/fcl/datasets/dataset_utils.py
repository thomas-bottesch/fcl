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

def load_and_extract_dataset_from_github(repo_name, path_to_save_to, dataset_filename):
  import bz2
  import shutil
  dataset_path = os.path.join(path_to_save_to, dataset_filename)
  dataset_extract_path = dataset_path[:-4]
    
  if not dataset_filename.endswith(".bz2"):
    raise Exception("only .bz2 files are downloadable with this function!")
  
  if os.path.isfile(dataset_extract_path):
    return dataset_extract_path
  
  try:
    if not os.path.isfile(dataset_path):   
      print("Downloading compressed dataset to:", dataset_path)
      urlretrieve("https://github.com/thomas-bottesch/%s/blob/master/%s?raw=true"%(repo_name, dataset_filename), dataset_path)
    else:
      print("Detect existing: " + dataset_path)
    
    with open(dataset_path, 'r') as f:
      is_index_file = f.read(len(dataset_filename)) == dataset_filename
    
    if is_index_file:
      parts = []
      with open(dataset_path, 'r') as f:
        for line in f:
          parts.append(line.rstrip())
        
      print("Multipart dataset detected with %d parts" % len(parts))
      
      part_paths = []
      for part in parts:
        part_path = os.path.join(path_to_save_to, part)
        part_paths.append(part_path)
        if not os.path.isfile(part_path):
          print("Downloading part to:", part_path)
          urlretrieve("https://github.com/thomas-bottesch/%s/blob/master/%s?raw=true"%(repo_name, part), part_path)
      
      print("Joining multiple parts together")
      dataset_path_tmp = dataset_path + "_tmp"
      with open(dataset_path_tmp, 'wb') as f_out:
        for part_path in part_paths:
          with open(part_path, 'rb') as f_in:
            f_out.write(f_in.read())
      
      shutil.move(dataset_path_tmp, dataset_path)
      
      print("Successful. Removing parts")
      for part_path in part_paths:
        os.remove(part_path)
    
    print("Extracting dataset to:", dataset_extract_path)
    
    with open(dataset_extract_path, 'wb') as f_out:
      
      f = bz2.BZ2File(dataset_path, 'rb')
      try:
        for line in f:
          f_out.write(line)
      finally:
          f.close()
        
    print("Removing compressed dataset:", dataset_path)
    os.remove(dataset_path)
  except:
    print("Error while downloading dataset: " + dataset_filename)
    print(traceback.format_exc())

  return dataset_extract_path

def load_sector_dataset(path_to_save_to):
  return load_dataset_from_github(path_to_save_to, 'sector.scaled')

def load_usps_dataset(path_to_save_to):
  return load_dataset_from_github(path_to_save_to, 'usps.scaled')
  
def load_example_dataset(path_to_save_to):
  return load_dataset_from_github(path_to_save_to, 'example.scaled')