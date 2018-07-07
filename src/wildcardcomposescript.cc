#include "wildcardcomposescript.h"
#include <fst/script/fst-class.h>
#include <fst/script/script-impl.h>

namespace fst {
namespace script {

void WildcardCompose(
  const FstClass &fst1,
  const FstClass &fst2,
  MutableFstClass *ofst,
  const int wildcard,
  SlopMap slop_map,
  const int end_of_annotation
) {

  if (!internal::ArcTypesMatch(fst1, fst2, "WildcardCompose")
    || !internal::ArcTypesMatch(fst2, *ofst, "WildcardCompose")) {
    ofst->SetProperties(kError, kError);
    return;
  }
  WildcardComposeArgs args(fst1, fst2, ofst, wildcard, std::move(slop_map), end_of_annotation);
  Apply<Operation<WildcardComposeArgs>>("WildcardCompose", ofst->ArcType(), &args);
}

REGISTER_FST_OPERATION(WildcardCompose, StdArc, WildcardComposeArgs);
//REGISTER_FST_OPERATION(WildcardCompose, LogArc, WildcardComposeArgs);
//REGISTER_FST_OPERATION(WildcardCompose, Log64Arc, WildcardComposeArgs);

}
}
