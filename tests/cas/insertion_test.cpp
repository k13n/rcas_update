#include "cas/cas.hpp"
#include "cas/csv_importer.hpp"
#include "test/catch.hpp"
#include "cas/interleaver.hpp"
#include "comparator.hpp"

static int counter = 0;

 void TraverseNodesRecursively(cas::Node *node, cas::InterleavedKey* ikeys) {
   if (node == nullptr) {
     return;
   }
   node->ForEachChild([&](uint8_t, cas::Node& child) -> bool {
    TraverseNodesRecursively(&child, ikeys);
    return true;
   });

   cas::BinaryKey bkey;
   bkey.path_ =  std::vector<uint8_t>(&node->prefix_[0], &node->prefix_[node->separator_pos_]);
   bkey.value_ = std::vector<uint8_t>(&node->prefix_[node->separator_pos_], &node->prefix_[node->prefix_.size()]);
   bkey.did_ = counter;

   ikeys[counter] = cas::Interleaver::PathValue(bkey);
   counter++;
}

TEST_CASE("Test Insertion interleaving", "[cas::Interleaver]") {

  SECTION("StrictSlow insertion") {

    counter = 0;
    cas::InterleavedKey ikeys[13];

    cas::Cas<cas::vint64_t> index(cas::IndexType::TwoDimensional, {});
    std::string dataset = "../../datasets/bom_strictslow.csv";

    cas::CsvImporter<cas::vint64_t> importer(index, ';');
    importer.BulkLoad(dataset);

    dataset = "../../datasets/bom_insert_strictslow.csv";
    importer.Load(dataset, cas::InsertType::StrictSlow, cas::InsertType::StrictSlow);

    TraverseNodesRecursively(index.root_, ikeys);

    std::vector<cas::InterleavedByte> expected_keys [13];

    std::vector<cas::InterleavedByte> expected_key0_1 = {
    };
    expected_keys[0] = expected_key0_1;
    expected_keys[1] = expected_key0_1;

    std::vector<cas::InterleavedByte> expected_key2 = {
      { .byte_ = 0x72, .type_ = cas::Path },
      { .byte_ = 0xFF, .type_ = cas::Path },
      { .byte_ = 0x62, .type_ = cas::Path },
      { .byte_ = 0x61, .type_ = cas::Path },
      { .byte_ = 0x74, .type_ = cas::Path },
      { .byte_ = 0x74, .type_ = cas::Path },
      { .byte_ = 0x65, .type_ = cas::Path },
      { .byte_ = 0x72, .type_ = cas::Path },
      { .byte_ = 0x79, .type_ = cas::Path },
      { .byte_ = 0x00, .type_ = cas::Path },
      { .byte_ = 0xD3, .type_ = cas::Value }
    };
    expected_keys[2] = expected_key2;

    std::vector<cas::InterleavedByte> expected_key3 = {
      { .byte_ = 0x6E, .type_ = cas::Path },
      { .byte_ = 0x6F, .type_ = cas::Path },
      { .byte_ = 0x65, .type_ = cas::Path },
      { .byte_ = 0x00, .type_ = cas::Path },
      { .byte_ = 0x0E, .type_ = cas::Value },
      { .byte_ = 0x50, .type_ = cas::Value }
    };
    expected_keys[3] = expected_key3;

    std::vector<cas::InterleavedByte> expected_key4 = {
      { .byte_ = 0xFF, .type_ = cas::Path },
      { .byte_ = 0x62, .type_ = cas::Path },
      { .byte_ = 0x65, .type_ = cas::Path },
      { .byte_ = 0x6C, .type_ = cas::Path },
      { .byte_ = 0x74, .type_ = cas::Path },
      { .byte_ = 0x00, .type_ = cas::Path },
      { .byte_ = 0x50, .type_ = cas::Value }
    };
    expected_keys[4] = expected_key4;

    std::vector<cas::InterleavedByte> expected_key5 = {
      { .byte_ = 0xFF, .type_ = cas::Path },
      { .byte_ = 0x62, .type_ = cas::Path },
      { .byte_ = 0x72, .type_ = cas::Path },
      { .byte_ = 0x61, .type_ = cas::Path },
      { .byte_ = 0x6B, .type_ = cas::Path },
      { .byte_ = 0x65, .type_ = cas::Path },
      { .byte_ = 0x00, .type_ = cas::Path },
      { .byte_ = 0xC2, .type_ = cas::Value }
    };
    expected_keys[5] = expected_key5;

    std::vector<cas::InterleavedByte> expected_key6 = {
      { .byte_ = 0xFF, .type_ = cas::Path },
      { .byte_ = 0x62, .type_ = cas::Path },
      { .byte_ = 0x65, .type_ = cas::Path },
      { .byte_ = 0x6C, .type_ = cas::Path },
      { .byte_ = 0x74, .type_ = cas::Path },
      { .byte_ = 0x00, .type_ = cas::Path },
      { .byte_ = 0x4A, .type_ = cas::Value }
    };
    expected_keys[6] = expected_key6;

    std::vector<cas::InterleavedByte> expected_key7 = {
      { .byte_ = 0xFF, .type_ = cas::Path },
      { .byte_ = 0x62, .type_ = cas::Path },
      { .byte_ = 0x75, .type_ = cas::Path },
      { .byte_ = 0x6D, .type_ = cas::Path },
      { .byte_ = 0x70, .type_ = cas::Path },
      { .byte_ = 0x65, .type_ = cas::Path },
      { .byte_ = 0x72, .type_ = cas::Path },
      { .byte_ = 0x00, .type_ = cas::Path },
      { .byte_ = 0x8C, .type_ = cas::Value }
    };
    expected_keys[7] = expected_key7;

    std::vector<cas::InterleavedByte> expected_key8 = {
      { .byte_ = 0x61, .type_ = cas::Path },
      { .byte_ = 0x62, .type_ = cas::Path },
      { .byte_ = 0x69, .type_ = cas::Path },
      { .byte_ = 0x6E, .type_ = cas::Path },
      { .byte_ = 0x65, .type_ = cas::Path },
      { .byte_ = 0x72, .type_ = cas::Path },
      { .byte_ = 0x00, .type_ = cas::Path },
      { .byte_ = 0xF1, .type_ = cas::Value }
    };
    expected_keys[8] = expected_key8;

    std::vector<cas::InterleavedByte> expected_key9 = {
    };
    expected_keys[9] = expected_key9;

    std::vector<cas::InterleavedByte> expected_key10 = {
      { .byte_ = 0x69, .type_ = cas::Path },
      { .byte_ = 0x73, .type_ = cas::Path },
      { .byte_ = 0x74, .type_ = cas::Path },
      { .byte_ = 0x65, .type_ = cas::Path },
      { .byte_ = 0x72, .type_ = cas::Path },
      { .byte_ = 0x00, .type_ = cas::Path },
      { .byte_ = 0xAB, .type_ = cas::Value },
      { .byte_ = 0xCD, .type_ = cas::Value }
    };
    expected_keys[10] = expected_key10;

    std::vector<cas::InterleavedByte> expected_key11 = {
    };
    expected_keys[11] = expected_key11;

    std::vector<cas::InterleavedByte> expected_key12 = {
      { .byte_ = 0xFF, .type_ = cas::Path },
      { .byte_ = 0x62, .type_ = cas::Path },
      { .byte_ = 0x6F, .type_ = cas::Path },
      { .byte_ = 0x6D, .type_ = cas::Path },
      { .byte_ = 0xFF, .type_ = cas::Path },
      { .byte_ = 0x69, .type_ = cas::Path },
      { .byte_ = 0x74, .type_ = cas::Path },
      { .byte_ = 0x65, .type_ = cas::Path },
      { .byte_ = 0x6D, .type_ = cas::Path },
      { .byte_ = 0xFF, .type_ = cas::Path },
      { .byte_ = 0x63, .type_ = cas::Path },
      { .byte_ = 0x61, .type_ = cas::Path },
      { .byte_ = 0x80, .type_ = cas::Value },
      { .byte_ = 0x00, .type_ = cas::Value },
      { .byte_ = 0x00, .type_ = cas::Value },
      { .byte_ = 0x00, .type_ = cas::Value },
      { .byte_ = 0x00, .type_ = cas::Value }
    };
    expected_keys[12] = expected_key12;

    for(int i=0; i<13; i++){
        REQUIRE(Comparator::Equals(ikeys[i].bytes_, expected_keys[i]));
    }
  }

    SECTION("LazyFast insertion") {

    counter = 0;
    cas::InterleavedKey ikeys[14];

    cas::Cas<cas::vint64_t> index(cas::IndexType::TwoDimensional, {});
    std::string dataset = "../../datasets/bom_lazyfast.csv";

    cas::CsvImporter<cas::vint64_t> importer(index, ';');
    importer.BulkLoad(dataset);

    dataset = "../../datasets/bom_insert_lazyfast.csv";
    importer.Load(dataset, cas::InsertType::LazyFast, cas::InsertType::LazyFast);

    TraverseNodesRecursively(index.root_, ikeys);

    std::vector<cas::InterleavedByte> expected_keys [14];

    std::vector<cas::InterleavedByte> expected_key0_1 = {
    };
    expected_keys[0] = expected_key0_1;
    expected_keys[1] = expected_key0_1;

    std::vector<cas::InterleavedByte> expected_key2 = {
      { .byte_ = 0x72, .type_ = cas::Path },
      { .byte_ = 0xFF, .type_ = cas::Path },
      { .byte_ = 0x62, .type_ = cas::Path },
      { .byte_ = 0x61, .type_ = cas::Path },
      { .byte_ = 0x74, .type_ = cas::Path },
      { .byte_ = 0x74, .type_ = cas::Path },
      { .byte_ = 0x65, .type_ = cas::Path },
      { .byte_ = 0x72, .type_ = cas::Path },
      { .byte_ = 0x79, .type_ = cas::Path },
      { .byte_ = 0x00, .type_ = cas::Path },
      { .byte_ = 0xD3, .type_ = cas::Value }
    };
    expected_keys[2] = expected_key2;

    std::vector<cas::InterleavedByte> expected_key3 = {
      { .byte_ = 0x6E, .type_ = cas::Path },
      { .byte_ = 0x6F, .type_ = cas::Path },
      { .byte_ = 0x65, .type_ = cas::Path },
      { .byte_ = 0x00, .type_ = cas::Path },
      { .byte_ = 0x0E, .type_ = cas::Value },
      { .byte_ = 0x50, .type_ = cas::Value }
    };
    expected_keys[3] = expected_key3;

    std::vector<cas::InterleavedByte> expected_key4 = {
      { .byte_ = 0x65, .type_ = cas::Path },
      { .byte_ = 0x6C, .type_ = cas::Path },
      { .byte_ = 0x74, .type_ = cas::Path },
      { .byte_ = 0x00, .type_ = cas::Path },
      { .byte_ = 0x50, .type_ = cas::Value }
    };
    expected_keys[4] = expected_key4;

    std::vector<cas::InterleavedByte> expected_key5 = {
      { .byte_ = 0x72, .type_ = cas::Path },
      { .byte_ = 0x61, .type_ = cas::Path },
      { .byte_ = 0x6B, .type_ = cas::Path },
      { .byte_ = 0x65, .type_ = cas::Path },
      { .byte_ = 0x00, .type_ = cas::Path },
      { .byte_ = 0xC2, .type_ = cas::Value }
    };
    expected_keys[5] = expected_key5;

    std::vector<cas::InterleavedByte> expected_key6 = {
      { .byte_ = 0x65, .type_ = cas::Path },
      { .byte_ = 0x6C, .type_ = cas::Path },
      { .byte_ = 0x74, .type_ = cas::Path },
      { .byte_ = 0x00, .type_ = cas::Path },
      { .byte_ = 0x4A, .type_ = cas::Value }
    };
    expected_keys[6] = expected_key6;

    std::vector<cas::InterleavedByte> expected_key7 = {
      { .byte_ = 0x75, .type_ = cas::Path },
      { .byte_ = 0x6D, .type_ = cas::Path },
      { .byte_ = 0x70, .type_ = cas::Path },
      { .byte_ = 0x65, .type_ = cas::Path },
      { .byte_ = 0x72, .type_ = cas::Path },
      { .byte_ = 0x00, .type_ = cas::Path },
      { .byte_ = 0x8C, .type_ = cas::Value }
    };
    expected_keys[7] = expected_key7;

    std::vector<cas::InterleavedByte> expected_key8 = {
      { .byte_ = 0x62, .type_ = cas::Path },
    };
    expected_keys[8] = expected_key8;

    std::vector<cas::InterleavedByte> expected_key9 = {
      { .byte_ = 0x62, .type_ = cas::Path },
      { .byte_ = 0x69, .type_ = cas::Path },
      { .byte_ = 0x6E, .type_ = cas::Path },
      { .byte_ = 0x65, .type_ = cas::Path },
      { .byte_ = 0x72, .type_ = cas::Path },
      { .byte_ = 0x00, .type_ = cas::Path },
      { .byte_ = 0x00, .type_ = cas::Value },
      { .byte_ = 0xF1, .type_ = cas::Value }
    };
    expected_keys[9] = expected_key9;

    std::vector<cas::InterleavedByte> expected_key10 = {
    };
    expected_keys[10] = expected_key10;

    std::vector<cas::InterleavedByte> expected_key11 = {
      { .byte_ = 0x69, .type_ = cas::Path },
      { .byte_ = 0x73, .type_ = cas::Path },
      { .byte_ = 0x74, .type_ = cas::Path },
      { .byte_ = 0x65, .type_ = cas::Path },
      { .byte_ = 0x72, .type_ = cas::Path },
      { .byte_ = 0x00, .type_ = cas::Path },
      { .byte_ = 0xAB, .type_ = cas::Value },
      { .byte_ = 0xCD, .type_ = cas::Value }
    };
    expected_keys[11] = expected_key11;

    std::vector<cas::InterleavedByte> expected_key12 = {
    };
    expected_keys[12] = expected_key12;

    std::vector<cas::InterleavedByte> expected_key13 = {
      { .byte_ = 0xFF, .type_ = cas::Path },
      { .byte_ = 0x62, .type_ = cas::Path },
      { .byte_ = 0x6F, .type_ = cas::Path },
      { .byte_ = 0x6D, .type_ = cas::Path },
      { .byte_ = 0xFF, .type_ = cas::Path },
      { .byte_ = 0x69, .type_ = cas::Path },
      { .byte_ = 0x74, .type_ = cas::Path },
      { .byte_ = 0x65, .type_ = cas::Path },
      { .byte_ = 0x6D, .type_ = cas::Path },
      { .byte_ = 0xFF, .type_ = cas::Path },
      { .byte_ = 0x63, .type_ = cas::Path },
      { .byte_ = 0x61, .type_ = cas::Path },
      { .byte_ = 0x80, .type_ = cas::Value },
      { .byte_ = 0x00, .type_ = cas::Value },
      { .byte_ = 0x00, .type_ = cas::Value },
      { .byte_ = 0x00, .type_ = cas::Value },
      { .byte_ = 0x00, .type_ = cas::Value }
    };
    expected_keys[13] = expected_key13;

    for(int i=0; i<14; i++){
        REQUIRE(Comparator::Equals(ikeys[i].bytes_, expected_keys[i]));
    }
  }




}