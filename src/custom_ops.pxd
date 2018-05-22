from fst cimport FstClass
from fst cimport MutableFstClass

cdef extern from "wildcardcomposescript.h" \
    namespace "fst::script" nogil:

    void WildcardCompose(const FstClass &, const FstClass &,
                         MutableFstClass *, const int wildcard)
