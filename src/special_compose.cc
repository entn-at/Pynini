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

vector<NonTerminal<StdArc>> read_replacements(string path, string label, const SymbolTable &ist) {
  vector<NonTerminal<StdArc>> replacements;
  if (path.empty()) {
    return replacements;
  }

  const auto label_id = ist.Find(label);
  replacements.push_back(make_pair(label_id, Fst<StdArc>::Read(path)));
  return replacements;
}

int main(const int argc, const char *const *const argv) {

  if (argc < 4) {
    cout << "Usage: special_compose <lattice-fst> <annotation-fst> <word-symbol-table> [<prune_threshold>] [replacement-label] [<replacement> <annotation_symbol_table>]\n";
    exit(1);
  }

  const auto lattice_path = argv[1];
  const auto annotation_path = argv[2];
  const auto word_symbol_table_path = argv[3];
  const auto prune_threshold = argc > 4 ? stof(argv[4]) : 0.f;
  const auto replacement_label = argc > 7 ? argv[5] : nullptr;
  const auto replacement_path = argc > 7 ? argv[6] : nullptr;
  const auto annotation_symbol_table_path = argc > 7 ? argv[7] : nullptr;

  // Read and prepare lattice
  auto lattice = dynamic_cast<MutableFst<StdArc> *>(StdFst::Read(lattice_path));
  ArcSort(lattice, OLabelCompare<StdArc>{});

  // Read and prepare annotations
  auto annotator = dynamic_cast<MutableFst<StdArc> *>(StdFst::Read(annotation_path));
  ArcSort(annotator, ILabelCompare<StdArc>{});

  // Read and prepare special symbols
  const auto word_symbol_table = SymbolTable::ReadText(word_symbol_table_path);

  const auto annotation_symbol_table = annotation_symbol_table_path ? SymbolTable::ReadText(annotation_symbol_table_path) : nullptr;

  const auto wildcard = static_cast<ArcTpl<StdArc>::Label>(word_symbol_table->Find("<wildcard>"));

  const auto replacements = read_replacements(replacement_path, replacement_label, *annotation_symbol_table);

  // Prepare special matchers for composition
  StdVectorFst composed;
  WildcardCompose(*lattice, *annotator, &composed, wildcard, prune_threshold, replacements);

  RmEpsilon(&composed);

  // Compose
  print(&composed);

  ofstream wildcard_out_file{"wildcard_composed.txt"};
  print(&composed, wildcard_out_file);

  composed.Write("wildcard_composed.fst");

  return 0;
}
