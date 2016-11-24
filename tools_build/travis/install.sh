#!/bin/bash

# configure the shell
set -e                         # Exit immediately if a command exits with a non-zero status. 
set -x                         # Print commands and their arguments as they are executed.

sudo apt-get update;

if [ "x$INSTALL_OCTAVE" = "xyes" ]; then
      #apt-get install -qq software-properties-common;      # install apt-add-repository
      sudo apt-add-repository -y ppa:octave/stable;         # add octave ppa
      sudo apt-get update -qq;                              # update package index to get octave packages
      sudo apt-get install -qq -y octave liboctave-dev;     # install octave and development packages
fi

if [ "x$INSTALL_PYTHON_REQUIREMENTS" = "xyes" ]; then
    sudo apt-get install -qq -y libpng-dev libfreetype6-dev libxft-dev   # needed to get matplotlib to work
    pip install -r python/requirements.txt
    pip install -r python/requirements_examples.txt
fi

if [ "x$MAKE_CLI" = "xyes" ]; then
    make
fi

echo "Done"






