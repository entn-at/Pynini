#pragma once

#include <fst/script/arg-packs.h>
#include <fst/script/fst-class.h>
#include "wildcardcompose.h"

namespace fst {
namespace script {

using WildcardComposeArgs = std::tuple<const FstClass &, const FstClass &,
                                    MutableFstClass *, const int>;

template <class Arc>
void WildcardCompose(WildcardComposeArgs *args) {
  const Fst<Arc> &fst1 = *(std::get<0>(*args).GetFst<Arc>());
  const Fst<Arc> &fst2 = *(std::get<1>(*args).GetFst<Arc>());
  MutableFst<Arc> *ofst = std::get<2>(*args)->GetMutableFst<Arc>();
  const int wildcard = std::get<3>(*args);

  WildcardCompose(fst1, fst2, ofst, wildcard);
}

void WildcardCompose(const FstClass &fst1, const FstClass &fst2,
                     MutableFstClass *ofst, const int wildcard);

}
}
