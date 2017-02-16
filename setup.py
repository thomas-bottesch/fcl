from setuptools import setup, find_packages
from distutils.extension import Extension
from Cython.Build import cythonize
import platform
from distutils import sysconfig
import shutil
import os
from distutils.command.clean import clean as BaseClean
import sys


if platform.system() == 'Linux':
    ldshared = sysconfig.get_config_var('LDSHARED')
    sysconfig._config_vars['LDSHARED'] = ldshared.replace(' -g ', ' ')

def walk_up_folder(path, depth=1):
    _cur_depth = 1        
    while _cur_depth < depth:
        path = os.path.dirname(path)
        _cur_depth += 1
    return path   

base_library_path = walk_up_folder(os.path.realpath(__file__), 2)
base_path = os.path.join(base_library_path, 'python')
print base_path
sys.path.insert(0, base_path)

import fcl

class CleanClass(BaseClean):
    def run(self):
        BaseClean.run(self)
        is_not_sdist = not os.path.exists(os.path.join(os.path.abspath(os.path.dirname(__file__)), 'PKG-INFO'))
        for path, dirs, names in os.walk(os.path.join(base_library_path, 'fcl')):
            for fname in names:
                to_delete = [".pyc", ".pyd", ".dll", ".so"]
                
                for sfx in to_delete:
                  if fname.endswith(sfx):
                    os.remove(os.path.join(path, fname))
                    continue

                sfx = os.path.splitext(fname)[1]
                if is_not_sdist and sfx in ['.c', '.cpp']:
                    cython_file = str.replace(fname, sfx, '.pyx')
                    if os.path.exists(os.path.join(path, cython_file)):
                        os.remove(os.path.join(path, fname))
            for dname in dirs:
                if dname == '__pycache__':
                  shutil.rmtree(os.path.join(path, dname))
                  
        if os.path.exists('build'):
            shutil.rmtree('build')

algorithms_path = os.path.join(base_library_path, 'algorithms')
utils_path = os.path.join(base_library_path, 'utils')

csr_matrix_folder = os.path.join(utils_path, 'matrix', 'csr_matrix')
vector_list_folder = os.path.join(utils_path, 'matrix', 'vector_list')
common_vector_folder = os.path.join(utils_path, 'vector', 'common')
sparse_vector_folder = os.path.join(utils_path, 'vector', 'sparse')

algorithm_c_files = {}

for algo in os.listdir(algorithms_path):
    algorithms_path_specific = os.path.join(algorithms_path, algo)
    if os.path.isdir(algorithms_path_specific):
      algorithm_c_files_specific = []
      for f in os.listdir(algorithms_path_specific):
          if f.endswith(".c"):
              algorithm_c_files_specific.append(os.path.join(algorithms_path_specific, f))
      algorithm_c_files[algo] = algorithm_c_files_specific

setup(
  name = 'fcl',
  version = fcl.__version__,
  ext_modules=cythonize([
    Extension("fcl.kmeans._kmeans", [os.path.join(base_path, "fcl", "kmeans", "_kmeans.pyx")
                                    , os.path.join(utils_path,"clogging.c")
                                    , os.path.join(utils_path,"fcl_logging.c")
                                    , os.path.join(utils_path,"cdict.c")
                                    , os.path.join(utils_path,"fcl_file.c")
                                    , os.path.join(utils_path,"fcl_random.c")
                                    , os.path.join(utils_path,"fcl_string.c")
                                    , os.path.join(utils_path,"fcl_time.c")
                                    , os.path.join(csr_matrix_folder,"csr_assign.c")
                                    , os.path.join(csr_matrix_folder,"csr_load_matrix.c")
                                    , os.path.join(csr_matrix_folder,"csr_math.c")
                                    , os.path.join(csr_matrix_folder,"csr_matrix.c")
                                    , os.path.join(csr_matrix_folder,"csr_store_matrix.c")
                                    , os.path.join(csr_matrix_folder,"csr_to_vector_list.c")
                                    , os.path.join(vector_list_folder,"vector_list_math.c")
                                    , os.path.join(vector_list_folder,"vector_list_to_csr.c")
                                    , os.path.join(vector_list_folder,"vector_list.c")
                                    , os.path.join(common_vector_folder,"common_vector_math.c")
                                    , os.path.join(sparse_vector_folder,"sparse_vector_math.c")] + algorithm_c_files['kmeans']

                  , extra_compile_args=['-fopenmp', '-DEXTENSION']
                  , include_dirs=['.', base_library_path]
                  , extra_link_args=['-fopenmp']),
                         
    Extension("fcl.matrix.csr_matrix", [os.path.join(base_path, "fcl", "matrix", "csr_matrix.pyx")
                                        , os.path.join(utils_path,"clogging.c")
                                        , os.path.join(utils_path,"fcl_logging.c")
                                        , os.path.join(utils_path,"fcl_file.c")
                                        , os.path.join(csr_matrix_folder,"csr_load_matrix.c")
                                        , os.path.join(csr_matrix_folder,"csr_matrix.c")
                                        , os.path.join(csr_matrix_folder,"csr_store_matrix.c")]

                  , extra_compile_args=['-fopenmp', '-DEXTENSION']
                  , include_dirs=['.', base_library_path]
                  , extra_link_args=['-fopenmp']),
    Extension("fcl.cython.utils.matrix.csr_matrix.csr_assign", [os.path.join(base_path, "fcl", "cython", "utils", "matrix", "csr_matrix", "csr_assign.pyx")
                                        , os.path.join(utils_path,"fcl_file.c")
                                        , os.path.join(utils_path,"fcl_logging.c")
                                        , os.path.join(utils_path,"clogging.c")
                                        , os.path.join(csr_matrix_folder,"csr_load_matrix.c")
                                        , os.path.join(csr_matrix_folder,"csr_math.c")
                                        , os.path.join(csr_matrix_folder,"csr_matrix.c")
                                        , os.path.join(csr_matrix_folder,"csr_assign.c")
                                        , os.path.join(csr_matrix_folder,"csr_store_matrix.c")
                                        , os.path.join(common_vector_folder,"common_vector_math.c")
                                        , os.path.join(sparse_vector_folder,"sparse_vector_math.c")]

                  , extra_compile_args=['-fopenmp', '-DEXTENSION']
                  , include_dirs=['.', base_library_path]
                  , extra_link_args=['-fopenmp']),
    Extension("fcl.cython.utils.cdict", [os.path.join(base_path, "fcl", "cython", "utils", "cdict.pyx")
                                        , os.path.join(utils_path,"clogging.c")
                                        , os.path.join(utils_path,"cdict.c")
                                        , os.path.join(utils_path,"fcl_logging.c")]

                  , extra_compile_args=['-fopenmp', '-DEXTENSION']
                  , include_dirs=['.', base_library_path]
                  , extra_link_args=['-fopenmp'])
    ]),
  packages=["fcl"],
  package_dir={'':'python'},
  cmdclass={'clean': CleanClass},
)
