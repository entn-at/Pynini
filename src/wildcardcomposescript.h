#pragma once

#include <fst/script/arg-packs.h>
#include <fst/script/fst-class.h>
#include "wildcardcompose.h"

namespace fst {
namespace script {

using StringFstClassPair = std::pair<std::string, const FstClass *>;

using WildcardComposeArgs = std::tuple<
  const FstClass &, const FstClass &, MutableFstClass *, const int, const float, const std::vector<StringFstClassPair> &
>;

template<class Arc> void WildcardCompose(WildcardComposeArgs *args) {
  const auto &fst1 = *(std::get<0>(*args).GetFst<Arc>());
  const auto &fst2 = *(std::get<1>(*args).GetFst<Arc>());
  auto *ofst = std::get<2>(*args)->GetMutableFst<Arc>();
  const auto wildcard = std::get<3>(*args);
  const auto prune_threshold = std::get<4>(*args);

  const auto &replacements = std::get<5>(*args);
  const auto &symbol_table = *fst2.InputSymbols();
  const size_t size = replacements.size();
  std::vector<NonTerminal<Arc>> typed_pairs;
  typed_pairs.reserve(size);
  for (const auto &untyped_pair : replacements) {
    const auto label = symbol_table.Find(untyped_pair.first);
    if (label != kNoLabel) {
      typed_pairs.emplace_back(
        label, untyped_pair.second->GetFst<Arc>());
    }
    else {
      throw std::runtime_error{"No such symbol: " + untyped_pair.first + " in the ifst2 InputSymbols()"};
    }
  }

  WildcardCompose(fst1, fst2, ofst, wildcard, prune_threshold, typed_pairs);
}

void WildcardCompose(
  const FstClass &fst1,
  const FstClass &fst2,
  MutableFstClass *ofst,
  const int wildcard,
  const float prune_threshold,
  const std::vector<StringFstClassPair> &replacements
);
}
}
