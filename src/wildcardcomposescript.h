#pragma once

#include <fst/script/arg-packs.h>
#include <fst/script/fst-class.h>
#include "wildcardcompose.h"

namespace fst {
namespace script {

using WildcardComposeArgs = std::tuple<
  const FstClass &, const FstClass &, MutableFstClass *, const int, SlopMap, const int
>;

template<class Arc> void WildcardCompose(WildcardComposeArgs *const args) {
  WildcardCompose(
    /*fst1=*/*(std::get<0>(*args).GetFst<Arc>()),
    /*fst2=*/*(std::get<1>(*args).GetFst<Arc>()),
    /*ofst=*/std::get<2>(*args)->GetMutableFst<Arc>(),
    /*wildcard=*/std::get<3>(*args),
    /*slop_map=*/std::move(std::get<4>(*args)),
    /*end_of_annotation=*/std::get<5>(*args));
}

void WildcardCompose(
  const FstClass &fst1,
  const FstClass &fst2,
  MutableFstClass *ofst,
  const int wildcard,
  SlopMap slop_map,
  const int end_of_annotation
);
}
}
