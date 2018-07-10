#pragma once

#include <unordered_map>

#include <fst/arc.h>
#include <fst/arc-map.h>
#include <fst/compose.h>
#include <fst/fst.h>
#include <fst/map.h>
#include <fst/matcher.h>

#ifdef NDEBUG
constexpr bool _is_debug_build = false;
#else
constexpr bool _is_debug_build = true;
#endif

#define WHEN_DEBUG(expr) \
if (_is_debug_build) { \
expr; \
}


namespace fst {

class SloppyFilterState;

ostream &operator<<(ostream &os, const SloppyFilterState &fs);

ostream &operator<<(ostream &os, const StdArc &arc) {
  os << "Arc(ilabel=" << arc.ilabel << ", olabel=" << arc.olabel << ", nextstate=" << arc.nextstate << ")";
}



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
    return SloppyFilterState{kNoFilterState, kNoLabel};
  }

  size_t Hash() const {
    return static_cast<size_t>(accumulated_slop) << 32 + static_cast<size_t>(current_olabel);
  }

  bool operator==(const SloppyFilterState &fs) const {

    WHEN_DEBUG(cout << "lhs=" << *this << " :: rhs=" << fs << '\n');

    WHEN_DEBUG(cout << ((this->accumulated_slop == fs.accumulated_slop && this->current_olabel == fs.current_olabel)
                        ? "EQUAL"
                        : "NON EQUAL") << '\n');

    return this->accumulated_slop == fs.accumulated_slop && this->current_olabel == fs.current_olabel;
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
  int accumulated_slop = 0;
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
    WHEN_DEBUG(cout << "Visiting states s1=" << s1 << ", s2=" << s2 << ", fs=" << filter_state << '\n');
    current_ifst1_state_ = s1;
    current_ifst2_state_ = s2;
    //current_filter_state_ = filter_state;
    current_filter_state_ = composition_states_to_filter_state_[make_pair(s1, s2)];
  }

  FilterState FilterArc(Arc *a1, Arc *a2) const {
    const auto filter_state = DetermineFilterState(a1, a2);

    WHEN_DEBUG(
      cout << "In state (" << a1->nextstate << ", " << a2->nextstate << ") use filter " << filter_state << '\n');

    const auto inserted_it = composition_states_to_filter_state_.insert(
      make_pair(
        make_pair(a1->nextstate, a2->nextstate), filter_state
      ));
    if (!inserted_it.second) {
      cerr << "Assumption violated - each composition (state, state) pair has unique filter state.\n";
      throw runtime_error{""};
    }
    return filter_state;
  }

  void FilterFinal(Weight *, Weight *) const {
    cout << "Found final state\n";
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
  FilterState DetermineFilterState(Arc *a1, Arc *a2) const {

    WHEN_DEBUG(cout << *a1 << " -- " << *a2 << '\n';);

    if (!current_filter_state_.AnnotationActive()) {
      if (a2->olabel != kEpsilonLabel) {
        WHEN_DEBUG(cout << "<activating annotation (ID = " << a2->olabel << ")>\n");

        return FilterState{0, a2->olabel};
      }
      WHEN_DEBUG(cout << "<annotation not active>\n");

      return current_filter_state_;
    }

    if (a2->olabel == kEpsilonLabel) {
      const auto slop = current_filter_state_.AccumulatedSlop() + 1;
      const auto olabel = current_filter_state_.CurrentOlabel();
      if (slop < slop_per_olabel_[olabel]) {
        WHEN_DEBUG(cout << "<slop += 1 (" << slop << ")>\n");

        return FilterState{slop, olabel};
      } else {
        WHEN_DEBUG(cout << "<slop exceeded - rejecting path>\n");

        return FilterState::NoState();
      }
    } else if (a2->olabel == end_of_annotation_label_) {
      WHEN_DEBUG(cout << "<end of annotation (annotation ID = " << current_filter_state_.CurrentOlabel() << ")>\n");

      return FilterState{0};
    } else {
      WHEN_DEBUG(cout << "<matching annotation olabel (ID = " << current_filter_state_.CurrentOlabel() << ")>\n");

      return FilterState{0, a2->olabel};
    }
  }

private:
  mutable SlopMap slop_per_olabel_;
  std::unique_ptr<Matcher1> matcher1_;
  std::unique_ptr<Matcher2> matcher2_;
  const FST1 &fst1_;
  const FST2 &fst2_;

  FilterState current_filter_state_;
  StateId current_ifst1_state_;
  StateId current_ifst2_state_;
  int end_of_annotation_label_;

  // need info from which state we got to the current one to propagate the filter state
  mutable map<
    pair<
      StateId,
      StateId
    >, const FilterState
  > composition_states_to_filter_state_;//  /*unordered_*/map<Label, /*unordered_*/map<Label, const FilterState>> fst_state_to_filter_state_;
};

ostream &operator<<(ostream &os, const SloppyFilterState &fs) {
  os << "SloppyFilterState(current_olabel=" << fs.CurrentOlabel() << ", slop=" << fs.AccumulatedSlop() << ')';
}

}