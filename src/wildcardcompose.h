#pragma once

#include <fst/arc.h>
#include <fst/arc-map.h>
#include <fst/compose.h>
#include <fst/fst.h>
#include <fst/map.h>
#include <fst/matcher.h>
#include <fst/prune.h>

namespace fst {

// Special slop value reserved for the filter state blocking the composition.
constexpr int kNoFilterState = -1;

constexpr int kEpsilonLabel = 0;

class SloppyFilterState {
public:
  SloppyFilterState() = default;

  explicit SloppyFilterState(int accumulated_slop, int current_olabel = kNoLabel)
    : accumulated_slop{accumulated_slop},
      current_olabel{current_olabel}
  {}

  static const SloppyFilterState NoState() {
    return {};
  }

  size_t Hash() const {
    return static_cast<size_t>(accumulated_slop) << 32 + static_cast<size_t>(current_olabel);
  }

  bool operator==(const SloppyFilterState &fs) const {
    return accumulated_slop != kNoFilterState && fs.accumulated_slop != kNoFilterState;
  }

  bool operator!=(const SloppyFilterState &fs) const {
    return !(*this == fs);
  }

  bool AnnotationActive() const {
    return current_olabel != kNoLabel;
  }

  int AccumulatedSlop() const {
    return accumulated_slop;
  }

  int CurrentOlabel() const {
    return current_olabel;
  }

private:
  int accumulated_slop = kNoFilterState;
  int current_olabel = kNoLabel;
};


//template<typename Arc>
ostream &operator<<(ostream &os, const StdArc &a) {
  os << "Arc(ilabel=" << a.ilabel << ", olabel=" << a.olabel << ")";
}


template <class M1, class M2 /* = M1 */>
class SloppyComposeFilter {
public:
  using Matcher1 = M1;
  using Matcher2 = M2;
  using FST1 = typename M1::FST;
  using FST2 = typename M2::FST;
  using FilterState = SloppyFilterState;

  using Arc = typename FST1::Arc;
  using Label = typename Arc::Label;
  using StateId = typename Arc::StateId;
  using Weight = typename Arc::Weight;

  SloppyComposeFilter(const FST1 &fst1, const FST2 &fst2,
                       Matcher1 *matcher1 = nullptr,
                       Matcher2 *matcher2 = nullptr,
                       unordered_map<Label, int> slop_per_olabel = {},
                       int end_of_annotation_label = kNoLabel)
    : slop_per_olabel_{move(slop_per_olabel)},
      matcher1_{matcher1 ? matcher1 : new Matcher1(fst1, MATCH_OUTPUT)},
      matcher2_{matcher2 ? matcher2 : new Matcher2(fst2, MATCH_INPUT)},
      fst1_{matcher1_->GetFst()},
      fst2_{matcher2_->GetFst()},
      end_of_annotation_label_{end_of_annotation_label}
      {}

  SloppyComposeFilter(const SloppyComposeFilter<Matcher1, Matcher2> &filter,
                       bool safe = false)
    : slop_per_olabel_{filter.slop_per_olabel_},
      matcher1_{filter.matcher1_->Copy(safe)},
      matcher2_{filter.matcher2_->Copy(safe)},
      fst1_{matcher1_->GetFst()},
      fst2_{matcher2_->GetFst()},
      end_of_annotation_label_{filter.end_of_annotation_label_}
      {}

  FilterState Start() const {
    return FilterState{0};
  }

  void SetState(StateId s1, StateId s2, const FilterState &filter_state) {
    current_filter_state_ = filter_state;
  }

  FilterState FilterArc(Arc *a1, Arc *a2) const {

    if (!current_filter_state_.AnnotationActive()) {
      return current_filter_state_;
    }

    if (a2->olabel == kEpsilonLabel) {
      const auto slop = current_filter_state_.AccumulatedSlop() + 1;
      const auto olabel = current_filter_state_.CurrentOlabel();
      if (slop < slop_per_olabel_[olabel]) {
        return SloppyFilterState{slop, olabel};
      }
      else {
        return SloppyFilterState::NoState();
      }
    }
    else if (a2->olabel == end_of_annotation_label_) {
      return FilterState{0};
    }
    else {
      return SloppyFilterState{0, a2->olabel};
    }
  }

  void FilterFinal(Weight *, Weight *) const {}

  Matcher1 *GetMatcher1() { return matcher1_.get(); }

  Matcher2 *GetMatcher2() { return matcher2_.get(); }

  uint64 Properties(uint64 props) const { return props; }

private:
  mutable unordered_map<Label, int> slop_per_olabel_;
  std::unique_ptr<Matcher1> matcher1_;
  std::unique_ptr<Matcher2> matcher2_;
  const FST1 &fst1_;
  const FST2 &fst2_;

  FilterState current_filter_state_;
  int end_of_annotation_label_;
};

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
  using WildcardMatcher = SigmaMatcher<SortedMatcher<Fst<Arc>>>;
  using SloppyFilter = SloppyComposeFilter<WildcardMatcher, WildcardMatcher>;

  ComposeFstOptions<Arc, WildcardMatcher, SloppyFilter> opts;
  opts.filter = new SloppyFilter(
    fst1,
    fst2,
    new WildcardMatcher(fst1, MATCH_NONE),
    new WildcardMatcher(fst2, MATCH_INPUT, wildcard),
    {}
  );

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
