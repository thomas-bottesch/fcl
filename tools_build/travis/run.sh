#!/bin/bash

# configure the shell
set -e                         # Exit immediately if a command exits with a non-zero status. 
set -x                         # Print commands and their arguments as they are executed.

if [ "x$INSTALL_OCTAVE" = "xyes" ]; then
    # make sure the octave examples work
    # mex files are compiled automatically when executing the first example
    for BASE_FOLDER in examples/matlab/kmeans/basic/ examples/matlab/kmeans/plotting/
    do
        FILES=${BASE_FOLDER}*.m
        for f in $FILES
        do
          echo octave -p $BASE_FOLDER --eval `basename "${f%.*}"`
          echo
          
          octave -p $BASE_FOLDER --eval `basename "${f%.*}"`
          echo "------------------------------------------------"
        done
    done
fi

if [ "x$INSTALL_PYTHON_REQUIREMENTS" = "xyes" ]; then
    python python/setup.py develop
    
    # make sure the python examples work
    for BASE_FOLDER in examples/python/kmeans/basic/ examples/python/kmeans/plotting/
    do
        FILES=${BASE_FOLDER}*.py
        for f in $FILES
        do
          echo python $f
          echo
          
          python $f
          echo "------------------------------------------------"
        done
    done
fi