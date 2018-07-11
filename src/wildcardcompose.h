#pragma once

#include <fst/arc.h>
#include <fst/arc-map.h>
#include <fst/compose.h>
#include <fst/fst.h>
#include <fst/map.h>
#include <fst/matcher.h>
#include <fst/prune.h>
#include <fst/fstlib.h>

namespace fst {

template<class Arc> void WildcardCompose(
  const Fst<Arc> &fst1,
  const Fst<Arc> &fst2,
  MutableFst<Arc> *ofst,
  const int wildcard,
  const float prune_threshold = 0.0f
);

template<class Arc> void WildcardComposeNonPruned(
  const Fst<Arc> &fst1, const Fst<Arc> &fst2, MutableFst<Arc> *ofst, const int wildcard
) {
  using WildcardMatcher = SigmaMatcher<SortedMatcher<Fst<Arc>>>;

  using M = typename DefaultLookAhead<Arc, MATCH_BOTH>::FstMatcher;
  using F = typename DefaultLookAhead<Arc, MATCH_BOTH>::ComposeFilter;

  using LookAhead = LookAheadMatcher<Fst<Arc>>;
  ComposeFstOptions<Arc, M, F> opts;
  opts.filter = new F(
    fst1,
    fst2,
    new M(new WildcardMatcher(fst1, MATCH_NONE)),
    new M(new WildcardMatcher(fst2, MATCH_INPUT, wildcard))
  );
  /*
  ComposeFstOptions<Arc, WildcardMatcher> opts;
  opts.matcher1 = new WildcardMatcher(fst1, MATCH_NONE);
  opts.matcher2 = new WildcardMatcher(fst2, MATCH_INPUT, wildcard);
  */
  *ofst = ComposeFst<Arc>(fst1, fst2, opts);
}


///
/// Typedefs for pruned composition types
///

using PrunedWildcardWeight = LexicographicWeight<TropicalWeight, TropicalWeight>;
using PrunedWildcardArc = ArcTpl<PrunedWildcardWeight>;
using PrunedWildcardFst = Fst<PrunedWildcardArc>;

struct LatticeConverter {
  PrunedWildcardWeight operator()(const TropicalWeight &weight_in) const {
    return {weight_in == TropicalWeight::Zero() ? TropicalWeight::Zero() : TropicalWeight::One(), weight_in};
  }
};

using LatticeArcMapper = WeightConvertMapper<StdArc, PrunedWildcardArc, LatticeConverter>;

struct IntentsConverter {
  PrunedWildcardWeight operator()(const TropicalWeight &weight_in) const {
    return {weight_in, weight_in == TropicalWeight::Zero() ? TropicalWeight::Zero() : TropicalWeight::One()};
  }
};

using IntentsArcMapper = WeightConvertMapper<StdArc, PrunedWildcardArc, IntentsConverter>;

struct WildcardToStdConverter {
  TropicalWeight operator()(const PrunedWildcardWeight &weight_in) const {
    if (weight_in.Value1() == TropicalWeight::Zero() || weight_in.Value2() == TropicalWeight::Zero()) {
      return TropicalWeight::Zero();
    }
    return weight_in.Value2();
  }
};

using WildcardToStdArcMapper = WeightConvertMapper<PrunedWildcardArc, StdArc, WildcardToStdConverter>;

template<class Arc> void WildcardComposePruned(
  const Fst<Arc> &fst1, const Fst<Arc> &fst2, MutableFst<Arc> *ofst, const int wildcard, const float prune_threshold
) {
  ArcMapFst<StdArc, PrunedWildcardArc, LatticeArcMapper> mapped_lattice{fst1, LatticeArcMapper{}};
  ArcMapFst<StdArc, PrunedWildcardArc, IntentsArcMapper> mapped_intents{fst2, IntentsArcMapper{}};

  using M = typename DefaultLookAhead<PrunedWildcardArc, MATCH_BOTH>::FstMatcher;
  using F = typename DefaultLookAhead<PrunedWildcardArc, MATCH_BOTH>::ComposeFilter;

  using LookAhead = LookAheadMatcher<PrunedWildcardFst>;
  using PrunedWildcardMatcher = SigmaMatcher<SortedMatcher<PrunedWildcardFst>>;
  ComposeFstOptions<PrunedWildcardArc, M, F> pruned_wildcard_compose_opts;
  pruned_wildcard_compose_opts.filter = new F(
    mapped_lattice,
    mapped_intents,
    new M(new PrunedWildcardMatcher(mapped_lattice, MATCH_NONE)),
    new M(new PrunedWildcardMatcher(mapped_intents, MATCH_INPUT, wildcard))
    );
  /*ComposeFstOptions<PrunedWildcardArc, PrunedWildcardMatcher> pruned_wildcard_compose_opts;
  pruned_wildcard_compose_opts.matcher1 = new PrunedWildcardMatcher(mapped_lattice, MATCH_NONE);
  pruned_wildcard_compose_opts.matcher2 = new PrunedWildcardMatcher(mapped_intents, MATCH_INPUT, wildcard);*/

  ComposeFst<PrunedWildcardArc> composed{mapped_lattice, mapped_intents, pruned_wildcard_compose_opts};

  VectorFst<PrunedWildcardArc> pruned;
  const PrunedWildcardWeight weight_threshold{prune_threshold, std::numeric_limits<float>::max()};
  Prune(composed, &pruned, weight_threshold);

  ArcMap(pruned, ofst, WildcardToStdArcMapper{});
}

template<class Arc> void WildcardCompose(
  const Fst<Arc> &fst1, const Fst<Arc> &fst2, MutableFst<Arc> *ofst, const int wildcard, const float prune_threshold
) {
  if (prune_threshold == 0.0f) {
    WildcardComposeNonPruned(fst1, fst2, ofst, wildcard);
  } else {
    WildcardComposePruned(fst1, fst2, ofst, wildcard, prune_threshold);
  }
}
}
