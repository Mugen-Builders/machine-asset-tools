from setuptools import Extension, setup
from Cython.Build import cythonize

setup(
    ext_modules = cythonize([
        Extension("cmpy",
            sources=["cmpy.pyx"],
            libraries=["cmt","cma"],
            language="c++",
        )
    ])
)
