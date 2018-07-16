#pragma once

#include <fst/script/fst-class.h>
#include "wildcardcompose.h"

namespace fst {
namespace script {
  void ToConstFst(const FstClass &ifst_class, FstClass *ofst_class);
}
}