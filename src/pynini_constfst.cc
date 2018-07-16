#include "pynini_constfst.h"

namespace fst {
namespace script {
void ToConstFst(const FstClass &ifst_class, FstClass *ofst_class) {

  const auto *const std_ifst = ifst_class.GetFst<StdArc>();
  if (std_ifst != nullptr) {
    *ofst_class = FstClass{ConstFst<StdArc, uint32_t>{*std_ifst}};
  }

  const auto *const pruned_wildcard_ifst = ifst_class.GetFst<PrunedWildcardArc>();
  if (pruned_wildcard_ifst != nullptr) {
    *ofst_class = FstClass{ConstFst<PrunedWildcardArc, uint32_t>{*pruned_wildcard_ifst}};
  }
}
}
}