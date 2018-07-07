#pragma once

#include <unordered_map>

#include <fst/arc.h>
#include <fst/arc-map.h>
#include <fst/compose.h>
#include <fst/fst.h>
#include <fst/map.h>
#include <fst/matcher.h>

namespace fst {

using SlopMap = unordered_map<int, int>;

// Special slop value reserved for the filter state blocking the composition.
constexpr int kNoFilterState = -1;

constexpr int kEpsilonLabel = 0;

class SloppyFilterState {
public:
  SloppyFilterState() = default;

  explicit SloppyFilterState(int accumulated_slop, int current_olabel = kNoLabel)
    : accumulated_slop{accumulated_slop}, current_olabel{current_olabel} {
  }

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

template<class M1, class M2 /* = M1 */> class SloppyComposeFilter {
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

  SloppyComposeFilter(
    const FST1 &fst1,
    const FST2 &fst2,
    Matcher1 *matcher1 = nullptr,
    Matcher2 *matcher2 = nullptr,
    SlopMap slop_per_olabel = {},
    int end_of_annotation_label = kNoLabel
  )
    : slop_per_olabel_{move(slop_per_olabel)},
      matcher1_{matcher1 ? matcher1 : new Matcher1(fst1, MATCH_OUTPUT)},
      matcher2_{matcher2 ? matcher2 : new Matcher2(fst2, MATCH_INPUT)},
      fst1_{matcher1_->GetFst()},
      fst2_{matcher2_->GetFst()},
      end_of_annotation_label_{end_of_annotation_label} {
  }

  SloppyComposeFilter(
    const SloppyComposeFilter<Matcher1, Matcher2> &filter, bool safe = false
  )
    : slop_per_olabel_{filter.slop_per_olabel_},
      matcher1_{filter.matcher1_->Copy(safe)},
      matcher2_{filter.matcher2_->Copy(safe)},
      fst1_{matcher1_->GetFst()},
      fst2_{matcher2_->GetFst()},
      end_of_annotation_label_{filter.end_of_annotation_label_} {
  }

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
        return FilterState{slop, olabel};
      } else {
        return FilterState::NoState();
      }
    } else if (a2->olabel == end_of_annotation_label_) {
      return FilterState{0};
    } else {
      return FilterState{0, a2->olabel};
    }
  }

  void FilterFinal(Weight *, Weight *) const {
  }

  Matcher1 *GetMatcher1() {
    return matcher1_.get();
  }

  Matcher2 *GetMatcher2() {
    return matcher2_.get();
  }

  uint64 Properties(uint64 props) const {
    return props;
  }

private:
  mutable SlopMap slop_per_olabel_;
  std::unique_ptr<Matcher1> matcher1_;
  std::unique_ptr<Matcher2> matcher2_;
  const FST1 &fst1_;
  const FST2 &fst2_;

  FilterState current_filter_state_;
  int end_of_annotation_label_;
};
}