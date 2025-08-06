#!/bin/bash

# configure the shell
set -e                         # Exit immediately if a command exits with a non-zero status. 
set -x                         # Print commands and their arguments as they are executed.

sudo apt-get update;

if [ "x$INSTALL_OCTAVE" = "xyes" ]; then
      sudo apt-add-repository -y ppa:octave/stable;         # add octave ppa
      sudo apt-get update -qq;                              # update package index to get octave packages
      sudo apt-get install -qq -y octave liboctave-dev;     # install octave and development packages
fi

if [ "x$INSTALL_PYTHON_REQUIREMENTS" = "xyes" ]; then
    # sudo apt-get install libblas-dev liblapack-dev libatlas-base-dev gfortran  # install blas libraries 
    sudo apt-get install -qq -y libpng-dev libfreetype6-dev libxft-dev         # needed to get matplotlib to work
    dpkg -l                                                                    # list all installed debian packages
    pip install --upgrade pip                                                  # upgrade pip (makes it possible to install .whl)
    pip uninstall numpy -y
    pip freeze                                                                 # output the installed pip packages before req install
    cat python/requirements_examples.txt | xargs -n 1 -L 1 pip install         # install all requirements one by one
    pip freeze                                                                 # output the installed pip packages
fi

if [ "x$MAKE_CLI" = "xyes" ]; then
    make
fi

echo "Done"






