#pragma once

#include <fst/script/arg-packs.h>
#include <fst/script/fst-class.h>
#include "wildcardcompose.h"

namespace fst {
namespace script {

using WildcardComposeArgs = std::tuple<const FstClass &, const FstClass &,
                                    MutableFstClass *, const int, const float>;

template <class Arc>
void WildcardCompose(WildcardComposeArgs *args) {
  const auto &fst1 = *(std::get<0>(*args).GetFst<Arc>());
  const auto &fst2 = *(std::get<1>(*args).GetFst<Arc>());
  auto *ofst = std::get<2>(*args)->GetMutableFst<Arc>();
  const auto wildcard = std::get<3>(*args);
  const auto prune_threshold = std::get<4>(*args);

  WildcardCompose(fst1, fst2, ofst, wildcard, prune_threshold);
}

void WildcardCompose(const FstClass &fst1, const FstClass &fst2,
                     MutableFstClass *ofst, const int wildcard, const float prune_threshold);

}
}
