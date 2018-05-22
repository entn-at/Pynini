#pragma once

#include <fst/fst.h>


namespace fst {

template <class Arc>
void WildcardCompose(
  const Fst<Arc> &fst1,
  const Fst<Arc> &fst2,
  MutableFst<Arc> *ofst,
  const int wildcard) {

  using WildcardMatcher = RhoMatcher<SortedMatcher<Arc>>;

  ComposeFstOptions<Arc, WildcardMatcher> opts;
  opts.matcher1 = new WildcardMatcher(*fst1, MATCH_NONE);
  opts.matcher2 = new WildcardMatcher(*fst2, MATCH_INPUT, wildcard);

  *ofst = ComposeFst<Arc>(*fst1, *fst2, opts);
}

}