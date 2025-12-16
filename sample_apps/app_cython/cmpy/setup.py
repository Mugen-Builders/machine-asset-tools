from setuptools import Extension, setup
from Cython.Build import cythonize

setup(
    ext_modules = cythonize([
        Extension("cmpy", ["cmpy.pyx"],
            libraries=["cmt","cma"]
        )
    ])
)
