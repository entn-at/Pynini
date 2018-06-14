#pragma once

#include <fst/arc.h>
#include <fst/arc-map.h>
#include <fst/compose.h>
#include <fst/fst.h>
#include <fst/map.h>
#include <fst/matcher.h>
#include <fst/prune.h>

namespace fst {

template <class Arc>
void WildcardCompose(
  const Fst<Arc> &fst1,
  const Fst<Arc> &fst2,
  MutableFst<Arc> *ofst,
  const int wildcard,
  const float prune_threshold = 0.0f);


template <class Arc>
void WildcardComposeNonPruned(
  const Fst<Arc> &fst1,
  const Fst<Arc> &fst2,
  MutableFst<Arc> *ofst,
  const int wildcard
) {
  //using WildcardMatcher = RhoMatcher<SortedMatcher<Fst<Arc>>>;
  using WildcardMatcher = SigmaMatcher<SortedMatcher<Fst<Arc>>>;

  ComposeFstOptions<Arc, WildcardMatcher> opts;
  opts.matcher1 = new WildcardMatcher(fst1, MATCH_NONE);
  opts.matcher2 = new WildcardMatcher(fst2, MATCH_INPUT, wildcard);

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
    return {TropicalWeight::One(), weight_in};
  }
};
using LatticeArcMapper = WeightConvertMapper<StdArc, PrunedWildcardArc, LatticeConverter>;

struct IntentsConverter {
  PrunedWildcardWeight operator()(const TropicalWeight &weight_in) const {
    return {weight_in, TropicalWeight::One()};
  }
};
using IntentsArcMapper = WeightConvertMapper<StdArc, PrunedWildcardArc, IntentsConverter>;

struct WildcardToStdConverter {
  TropicalWeight operator()(const PrunedWildcardWeight &weight_in) const {
    return weight_in.Value2();
  }
};
using WildcardToStdArcMapper = WeightConvertMapper<PrunedWildcardArc, StdArc, WildcardToStdConverter>;


template <class Arc>
void WildcardComposePruned(
  const Fst<Arc> &fst1,
  const Fst<Arc> &fst2,
  MutableFst<Arc> *ofst,
  const int wildcard,
  const float prune_threshold
) {

    MapFst<StdArc, PrunedWildcardArc, LatticeArcMapper> mapped_lattice{fst1, LatticeArcMapper{}};
    MapFst<StdArc, PrunedWildcardArc, IntentsArcMapper> mapped_intents{fst2, IntentsArcMapper{}};

    using PrunedWildcardMatcher = RhoMatcher<SortedMatcher<PrunedWildcardFst>>;
    ComposeFstOptions<PrunedWildcardArc, PrunedWildcardMatcher> pruned_wildcard_compose_opts;
    pruned_wildcard_compose_opts.matcher1 = new PrunedWildcardMatcher(mapped_lattice, MATCH_NONE);
    pruned_wildcard_compose_opts.matcher2 = new PrunedWildcardMatcher(mapped_intents, MATCH_INPUT, wildcard);

    ComposeFst<PrunedWildcardArc> composed{mapped_lattice, mapped_intents, pruned_wildcard_compose_opts};

    VectorFst<PrunedWildcardArc> pruned_composed;
    const auto threshold = PrunedWildcardWeight{prune_threshold, std::numeric_limits<float>::max()};
    Prune(composed, &pruned_composed, threshold);

    Map(pruned_composed, ofst, WildcardToStdArcMapper{});
}

template <class Arc>
void WildcardCompose(
  const Fst<Arc> &fst1,
  const Fst<Arc> &fst2,
  MutableFst<Arc> *ofst,
  const int wildcard,
  const float prune_threshold) {
  if (prune_threshold == 0.0f) {
    WildcardComposeNonPruned(fst1, fst2, ofst, wildcard);
  }
  else {
    WildcardComposePruned(fst1, fst2, ofst, wildcard, prune_threshold);
  }
}

}
