#pragma once

#include "sloppycomposefilter.h"

namespace fst {

template <class Arc>
void WildcardCompose(
  const Fst<Arc> &fst1,
  const Fst<Arc> &fst2,
  MutableFst<Arc> *ofst,
  const int wildcard, SlopMap slop_map, const int end_of_annotation
) {
  using WildcardMatcher = SigmaMatcher<SortedMatcher<Fst<Arc>>>;
  using SloppyFilter = SloppyComposeFilter<WildcardMatcher, WildcardMatcher>;

  ComposeFstOptions<Arc, WildcardMatcher, SloppyFilter> opts;
  opts.filter = new SloppyFilter(
    fst1,
    fst2,
    new WildcardMatcher(fst1, MATCH_NONE), new WildcardMatcher(fst2, MATCH_INPUT, wildcard), slop_map, end_of_annotation
  );

  *ofst = ComposeFst<Arc>(fst1, fst2, opts);
}

}
