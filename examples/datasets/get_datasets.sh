#!/bin/sh

download_dataset_from_git () {
   DESTINATION_PATH=$(dirname "$0")/${1}
   LOG_PATH=$(dirname "$0")/get_dataset.log
   GITHUB_URL=https://github.com/thomas-bottesch/fcl_datasets/blob/master/${1}?raw=true
   
   if [ -f "$DESTINATION_PATH" ];
   then
      echo "Dataset file $DESTINATION_PATH already exists. Skipping download."
   else
      printf "Downloading dataset to ${DESTINATION_PATH} ..."
      wget --output-document=${DESTINATION_PATH} $GITHUB_URL >> $LOG_PATH 2>&1
      
      if [ $? -eq 0 ]; then
          printf " successful.\n"
      else
          printf " failed. (look at ${LOG_PATH})\n"
      fi      
   fi
   
}

download_dataset_from_git 'sector.scaled'
download_dataset_from_git 'usps.scaled'
download_dataset_from_git 'example.scaled'
