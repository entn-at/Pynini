#pragma once

#include <fst/arc.h>
#include <fst/arc-map.h>
#include <fst/compose.h>
#include <fst/fst.h>
#include <fst/map.h>
#include <fst/matcher.h>
#include <fst/prune.h>
#include <fst/fstlib.h>
#include <fst/script/print-impl.h>

namespace fst {

template<typename Arc> using NonTerminal = std::pair<typename Arc::Label, const Fst<Arc> *>;

using StdNonTerminal = NonTerminal<StdArc>;

template<class Arc> void WildcardCompose(
  const Fst<Arc> &fst1,
  const Fst<Arc> &fst2,
  MutableFst<Arc> *ofst,
  const int wildcard,
  const float prune_threshold = 0.0f,
  const std::vector<NonTerminal<Arc>> &replacements = {}
);

template<class Arc> void WildcardComposeNonPruned(
  const Fst<Arc> &fst1, const Fst<Arc> &fst2, MutableFst<Arc> *ofst, const int wildcard
) {
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
    return {weight_in == TropicalWeight::Zero() ? TropicalWeight::Zero() : TropicalWeight::One(), weight_in};
  }
};

using LatticeArcMapper = WeightConvertMapper<StdArc, PrunedWildcardArc, LatticeConverter>;

struct AnnotationsConverter {
  PrunedWildcardWeight operator()(const TropicalWeight &weight_in) const {
    return {weight_in, weight_in == TropicalWeight::Zero() ? TropicalWeight::Zero() : TropicalWeight::One()};
  }
};

using AnnotationsArcMapper = WeightConvertMapper<StdArc, PrunedWildcardArc, AnnotationsConverter>;

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
  ArcMapFst<StdArc, PrunedWildcardArc, LatticeArcMapper> mapped_fst1{fst1, LatticeArcMapper{}};
  ArcMapFst<StdArc, PrunedWildcardArc, AnnotationsArcMapper> mapped_fst2{fst2, AnnotationsArcMapper{}};

  using PrunedWildcardMatcher = SigmaMatcher<SortedMatcher<PrunedWildcardFst>>;
  ComposeFstOptions<PrunedWildcardArc, PrunedWildcardMatcher> pruned_wildcard_compose_opts;
  pruned_wildcard_compose_opts.matcher1 = new PrunedWildcardMatcher(mapped_fst1, MATCH_NONE);
  pruned_wildcard_compose_opts.matcher2 = new PrunedWildcardMatcher(mapped_fst2, MATCH_INPUT, wildcard);

  ComposeFst<PrunedWildcardArc> composed{mapped_fst1, mapped_fst2, pruned_wildcard_compose_opts};

  VectorFst<PrunedWildcardArc> pruned;
  const PrunedWildcardWeight weight_threshold{prune_threshold, std::numeric_limits<float>::max()};
  Prune(composed, &pruned, weight_threshold);

  ArcMap(pruned, ofst, WildcardToStdArcMapper{});
}

template<class Arc> void WildcardCompose(
  const Fst<Arc> &fst1,
  const Fst<Arc> &fst2,
  MutableFst<Arc> *ofst,
  const int wildcard,
  const float prune_threshold,
  const std::vector<NonTerminal<Arc>> &replacements
) {
  const auto *rhs_fst = &fst2;

  std::unique_ptr<ReplaceFst<Arc>> expanded_fst2;
  if (!replacements.empty()) {
    std::vector<NonTerminal<Arc>> to_expand{{kNoLabel, &fst2}};
    to_expand.insert(to_expand.end(), replacements.begin(), replacements.end());
    ReplaceFstOptions<Arc> opts;
    opts.call_label_type = REPLACE_LABEL_NEITHER;
    expanded_fst2.reset(new ReplaceFst<Arc>{to_expand, opts});
    rhs_fst = expanded_fst2.get();
  }

  if (prune_threshold == 0.0f) {
    WildcardComposeNonPruned(fst1, *rhs_fst, ofst, wildcard);
  } else {
    WildcardComposePruned(fst1, *rhs_fst, ofst, wildcard, prune_threshold);
  }
}
}
