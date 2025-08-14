#!/bin/bash

# configure the shell
set -e                         # Exit immediately if a command exits with a non-zero status. 
set -x                         # Print commands and their arguments as they are executed.

if [ "x$INSTALL_PYTHON_REQUIREMENTS" = "xyes" ]; then
    pip install .
    
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