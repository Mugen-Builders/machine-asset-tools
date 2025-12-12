import os
from cffi import FFI

ffi = FFI()

with open(os.path.join(os.path.dirname(__file__), "/usr/include/libcmt/ffi.h")) as f:
    with open(os.path.join(os.path.dirname(__file__), "/usr/include/libcma/ffi.h")) as f2:
        ffi.cdef(f.read() + f2.read())
ffi.set_source("pycm",
    """
    #include "libcmt/rollup.h"
    #include "libcma/parser.h"
    #include "libcma/ledger.h"
    """,
    include_dirs=["usr/include"],
    library_dirs=["usr/lib"],
    libraries=['cmt','cma']
)

if __name__ == "__main__":
    ffi.compile(verbose=True)
