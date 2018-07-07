from libcpp.unordered_map cimport unordered_map

from fst cimport FstClass
from fst cimport MutableFstClass

cdef extern from "wildcardcomposescript.h" \
    namespace "fst::script" nogil:

    void WildcardCompose(
        const FstClass &,
        const FstClass &,
        MutableFstClass *,
        const int wildcard,
        unordered_map[int, int] slop_map,
        const int end_of_annotation
    )
