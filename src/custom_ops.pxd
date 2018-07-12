from libcpp.vector cimport vector
from libcpp.string cimport string
from libcpp.utility cimport pair

from fst cimport FstClass
from fst cimport MutableFstClass

ctypedef pair[string, const FstClass *] StringFstClassPair

cdef extern from "wildcardcomposescript.h" \
    namespace "fst::script" nogil:

    void WildcardCompose(const FstClass &, const FstClass &,
                         MutableFstClass *, const int wildcard, float prune_threshold, const vector[StringFstClassPair] &replacements)
