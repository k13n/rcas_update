#include "benchmark/insertion_experiment2.hpp"
#include "benchmark/runtime_experiment.hpp"
#include "benchmark/perf_wrapper.hpp"
#include "cas/cas.hpp"
#include "cas/cas_seq.hpp"
#include "cas/key.hpp"
#include "cas/key_encoder.hpp"
#include "cas/search_key.hpp"
#include "cas/csv_importer.hpp"
#include <iostream>
#include <chrono>
#include <fstream>
#include <cmath>

void Run() {
  using VType = cas::vint64_t;
  std::cout << "Insertion experiment: " << std::endl;
  std::cout << std::endl;

  cas::Cas<VType> index{cas::IndexType::TwoDimensional, {}};
  cas::CsvImporter<VType> importer(index, ';');

  std::deque<cas::Key<VType>> keys_all;
  std::deque<cas::Key<VType>> keys_all_cpy;

  std::string dataset_filename_ = "../datasets/bom.csv";
  std::ifstream infile(dataset_filename_);
  std::string line;
  while (std::getline(infile, line)) {
    keys_all.push_back(importer.ProcessLine(line));
  }

  keys_all_cpy = keys_all;
  index.BulkLoad(keys_all);

  std::cout << "index before deletion:\n";
  index.DumpConcise();

  cas::KeyEncoder<VType> encoder;

  auto bkey = encoder.Encode(keys_all_cpy[1]);
  std::cout << "Key to delete:\n";
  bkey.Dump();
  std::cout << "\n";
  index.Delete(bkey);
  std::cout << "index after deletion:\n";
  index.DumpConcise();

  bkey = encoder.Encode(keys_all_cpy[3]);
  std::cout << "Key to delete:\n";
  bkey.Dump();
  std::cout << "\n";
  index.Delete(bkey);
  std::cout << "index after deletion:\n";
  index.DumpConcise();

  bkey = encoder.Encode(keys_all_cpy[0]);
  std::cout << "Key to delete:\n";
  bkey.Dump();
  std::cout << "\n";
  index.Delete(bkey);
  std::cout << "index after deletion:\n";
  index.DumpConcise();

  bkey = encoder.Encode(keys_all_cpy[2]);
  std::cout << "Key to delete:\n";
  bkey.Dump();
  std::cout << "\n";
  index.Delete(bkey);
  std::cout << "index after deletion:\n";
  index.DumpConcise();

  bkey = encoder.Encode(keys_all_cpy[4]);
  std::cout << "Key to delete:\n";
  bkey.Dump();
  std::cout << "\n";
  index.Delete(bkey);
  std::cout << "index after deletion:\n";
  index.DumpConcise();

  bkey = encoder.Encode(keys_all_cpy[5]);
  std::cout << "Key to delete:\n";
  bkey.Dump();
  std::cout << "\n";
  index.Delete(bkey);
  std::cout << "index after deletion:\n";
  index.DumpConcise();

  bkey = encoder.Encode(keys_all_cpy[6]);
  std::cout << "Key to delete:\n";
  bkey.Dump();
  std::cout << "\n";
  index.Delete(bkey);
  std::cout << "index after deletion:\n";
  index.DumpConcise();
}



int main(int argc, char** argv) {
  Run();
  /* benchmark::Config config = benchmark::option_parser::Parse(argc, argv); */
  /* Benchmark(config); */
  return 0;
}
