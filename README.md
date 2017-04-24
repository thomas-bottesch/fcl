fcl - machine learning library
==============================

[![Build Status](https://travis-ci.org/thomas-bottesch/fcl.svg?branch=master)](https://travis-ci.org/thomas-bottesch/fcl)

[![Pypi](https://badge.fury.io/py/fcl.svg)](https://badge.fury.io/py/fcl)

fcl is a machine learning library which is open source and commercially usable - MIT license (see LICENSE file).
The machine learning core is programmed in C (C99) but fcl can be used from various languages:
    
* Matlab / Octave
* Python 2.x & 3.x (numpy and scipy matrices are supported)
* Command line interface

fcl was started as part of a phd thesis supported by:

* [Avira Operations GmbH & Co. KG](https://www.avira.com)
* [Institute of Neural Information Processing - Ulm University](https://www.uni-ulm.de/en/in/institute-of-neural-information-processing/)


# Get the code

You can get the latest sources with the command:

    git clone https://github.com/thomas-bottesch/fcl.git


# Linux

On Ubuntu/Debian it might be necessary to install build essentials in order to compile fcl

    sudo apt-get install build-essential

Compile the library    
    
    make
    
Use the library
    
    # download the example datasets (in libsvm format)
    ./examples/datasets/get_datasets.sh
    
    # now you can use the library to e.g. cluster an example dataset with k-means
    ./fcl kmeans fit ./examples/datasets/usps.scaled --file_model ./result_clusters --no_clusters 10

Have a look at the available options

    ./fcl --help
    ./fcl kmeans --help
    ./fcl kmeans fit --help
    ./fcl kmeans predict --help
    
# Python 2/3
----

On Ubuntu/Debian install build essentials and the python dev package in order to create python bindings with cython

    sudo apt-get install build-essential (also python2.7-dev / python3.4-dev or whatever python version you use)

Install via pip:

    pip install fcl

Install packages required to run all the examples e.g. numpy, scipy and matplotlib (optional)

    pip install -r python/requirements_examples.txt
    
Use the library (this example needs ./examples/datasets/get_datasets.sh to be executed first!)
    
    from fcl import kmeans
    
    km = kmeans.KMeans(no_clusters=2, verbose = True)
    idx = km.fit_predict('./examples/datasets/sector.scaled')
    
    # You can use a numpy/scipy matrix instead of a path as input to fit_predict
    
There exist various python examples in

    examples/python/
    
# Matlab/Octave
----

Easiest way to use fcl inside matlab/octave is to just compile the algorithm that you need. E.g.
    
    execute matlab/kmeans/fcl_make_kmeans (from within Matlab/Octave)
    
Then this example can be run from any folder

    % create sparse matrix
    X = sprand(1000, 1000, 1/10);
    
    IDX = fcl_kmeans(X, 10);
    
There exist various matlab/octave examples in

    examples/matlab/
    
# Citations
----

When using fcl in a scientific publication, it is appreciated if you cite the following paper (Bibtex):

    @inproceedings{bottesch2016kmeans,
      title={Speeding up k-means by approximating Euclidean distances via block vectors},
      author={Bottesch, Thomas and B{\"u}hler, Thomas and K{\"a}chele, Markus},
      booktitle={Proceedings of The 33rd International Conference on Machine Learning},
      pages={2578--2586},
      year={2016}
    }
