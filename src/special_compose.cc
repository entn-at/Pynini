#include <iostream>
#include <fst/fst.h>
#include <fst/script/print-impl.h>

#include "wildcardcompose.h"

using namespace std;
using namespace fst;

template<typename FstType> void print(const FstType *const fst, ostream &out = cout) {
  FstPrinter<typename FstType::Arc> printer(*fst, nullptr, nullptr, nullptr, false, false, " ");
  printer.Print(&out, "");
}

int main(const int argc, const char *const *const argv) {

  if (argc != 4) {
    cout << "Usage: fst_find_intents <lattice-fst> <annotation-fst> <word-symbol-table>\n";
    exit(0);
  }

  const auto lattice_path = argv[1];
  const auto annotation_path = argv[2];
  const auto word_symbol_table_path = argv[3];

  // Read and prepare lattice
  auto lattice = dynamic_cast<MutableFst<StdArc> *>(StdFst::Read(lattice_path));
  ArcSort(lattice, OLabelCompare<StdArc>{});

  // Read and prepare intents
  auto annotator = dynamic_cast<MutableFst<StdArc> *>(StdFst::Read(annotation_path));
  ArcSort(annotator, ILabelCompare<StdArc>{});

  // Read and prepare special symbols
  const auto word_symbol_table = SymbolTable::ReadText(word_symbol_table_path);

  const auto wildcard = static_cast<ArcTpl<StdArc>::Label>(word_symbol_table->Find("<wildcard>"));

  SlopMap permittable_slop;
  const int end_of_annotation = 1;

  // Prepare special matchers for composition
  StdVectorFst composed;
  WildcardCompose(*lattice, *annotator, &composed, wildcard, std::move(permittable_slop), end_of_annotation);

  // Compose
  cout << "wildcard composed:\n";
  print(&composed);

  ofstream wildcard_out_file{"wildcard_composed.txt"};
  print(&composed, wildcard_out_file);

  composed.Write("wildcard_composed.fst");

  return 0;
}