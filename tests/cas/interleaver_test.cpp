#include "test/catch.hpp"
#include "cas/interleaver.hpp"
#include "comparator.hpp"


TEST_CASE("Test ByteInterleaving", "[cas::Interleaver]") {

  SECTION("Sets did_ correctly") {
    cas::BinaryKey bkey;
    bkey.did_ = 1245;
    cas::InterleavedKey ikey = cas::Interleaver::ByteByte(bkey);
    REQUIRE(ikey.did_ == bkey.did_);
  }


  SECTION("Path is longer than value") {
    cas::BinaryKey bkey;
    bkey.path_  = { 0x61, 0x62, 0x63, 0x64, 0x65, 0x00 };
    bkey.value_ = { 0x00, 0x01 };

    std::vector<cas::InterleavedByte> expected_key = {
      { .byte_ = 0x61, .type_ = cas::Path  },
      { .byte_ = 0x00, .type_ = cas::Value },
      { .byte_ = 0x62, .type_ = cas::Path  },
      { .byte_ = 0x01, .type_ = cas::Value },
      { .byte_ = 0x63, .type_ = cas::Path  },
      { .byte_ = 0x64, .type_ = cas::Path  },
      { .byte_ = 0x65, .type_ = cas::Path  },
      { .byte_ = 0x00, .type_ = cas::Path  },
    };

    cas::InterleavedKey ikey = cas::Interleaver::ByteByte(bkey);
    REQUIRE(Comparator::Equals(ikey.bytes_, expected_key));
  }


  SECTION("Value is longer than path") {
    cas::BinaryKey bkey;
    bkey.path_  = { 0x00, 0x01 };
    bkey.value_ = { 0x61, 0x62, 0x63, 0x64, 0x65, 0x00 };

    std::vector<cas::InterleavedByte> expected_key = {
      { .byte_ = 0x00, .type_ = cas::Path  },
      { .byte_ = 0x61, .type_ = cas::Value },
      { .byte_ = 0x01, .type_ = cas::Path  },
      { .byte_ = 0x62, .type_ = cas::Value },
      { .byte_ = 0x63, .type_ = cas::Value },
      { .byte_ = 0x64, .type_ = cas::Value },
      { .byte_ = 0x65, .type_ = cas::Value },
      { .byte_ = 0x00, .type_ = cas::Value },
    };

    cas::InterleavedKey ikey = cas::Interleaver::ByteByte(bkey);
    REQUIRE(Comparator::Equals(ikey.bytes_, expected_key));
  }
}


TEST_CASE("Test LevelByte interleaving", "[cas::Interleaver]") {

  SECTION("Path is longer than value") {
    cas::BinaryKey bkey;
    bkey.path_  = { 0xFF, 0x61, 0x62,
                    0xFF, 0x63, 0x64,
                    0xFF, 0x65, 0x66,
                    0xFF, 0x67, 0x00 };
    bkey.value_ = { 0x00, 0x01 };

    std::vector<cas::InterleavedByte> expected_key = {
      { .byte_ = 0xFF, .type_ = cas::Path  },
      { .byte_ = 0x61, .type_ = cas::Path  },
      { .byte_ = 0x62, .type_ = cas::Path  },
      { .byte_ = 0xFF, .type_ = cas::Path  },
      { .byte_ = 0x00, .type_ = cas::Value },
      { .byte_ = 0x63, .type_ = cas::Path  },
      { .byte_ = 0x64, .type_ = cas::Path  },
      { .byte_ = 0xFF, .type_ = cas::Path  },
      { .byte_ = 0x01, .type_ = cas::Value },
      { .byte_ = 0x65, .type_ = cas::Path  },
      { .byte_ = 0x66, .type_ = cas::Path  },
      { .byte_ = 0xFF, .type_ = cas::Path  },
      { .byte_ = 0x67, .type_ = cas::Path  },
      { .byte_ = 0x00, .type_ = cas::Path  },
    };

    cas::InterleavedKey ikey = cas::Interleaver::LevelByte(bkey);
    REQUIRE(Comparator::Equals(ikey.bytes_, expected_key));
  }


  SECTION("Value is longer than path") {
    cas::BinaryKey bkey;
    bkey.path_  = { 0xFF, 0x61, 0x62,
                    0xFF, 0x63, 0x64,
                    0xFF, 0x65, 0x66,
                    0xFF, 0x67, 0x00 };
    bkey.value_ = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05 };

    std::vector<cas::InterleavedByte> expected_key = {
      { .byte_ = 0xFF, .type_ = cas::Path  },
      { .byte_ = 0x61, .type_ = cas::Path  },
      { .byte_ = 0x62, .type_ = cas::Path  },
      { .byte_ = 0xFF, .type_ = cas::Path  },
      { .byte_ = 0x00, .type_ = cas::Value },
      { .byte_ = 0x63, .type_ = cas::Path  },
      { .byte_ = 0x64, .type_ = cas::Path  },
      { .byte_ = 0xFF, .type_ = cas::Path  },
      { .byte_ = 0x01, .type_ = cas::Value },
      { .byte_ = 0x65, .type_ = cas::Path  },
      { .byte_ = 0x66, .type_ = cas::Path  },
      { .byte_ = 0xFF, .type_ = cas::Path  },
      { .byte_ = 0x02, .type_ = cas::Value },
      { .byte_ = 0x67, .type_ = cas::Path  },
      { .byte_ = 0x00, .type_ = cas::Path  },
      { .byte_ = 0x03, .type_ = cas::Value },
      { .byte_ = 0x04, .type_ = cas::Value },
      { .byte_ = 0x05, .type_ = cas::Value },
    };

    cas::InterleavedKey ikey = cas::Interleaver::LevelByte(bkey);
    REQUIRE(Comparator::Equals(ikey.bytes_, expected_key));
  }
}


TEST_CASE("Test PathValue interleaving", "[cas::Interleaver]") {
  cas::BinaryKey bkey;
  bkey.path_  = { 0x61, 0x62, 0x63, 0x64, 0x65, 0x00 };
  bkey.value_ = { 0x00, 0x01 };

  std::vector<cas::InterleavedByte> expected_key = {
    { .byte_ = 0x61, .type_ = cas::Path  },
    { .byte_ = 0x62, .type_ = cas::Path  },
    { .byte_ = 0x63, .type_ = cas::Path  },
    { .byte_ = 0x64, .type_ = cas::Path  },
    { .byte_ = 0x65, .type_ = cas::Path  },
    { .byte_ = 0x00, .type_ = cas::Path  },
    { .byte_ = 0x00, .type_ = cas::Value },
    { .byte_ = 0x01, .type_ = cas::Value },
  };

  cas::InterleavedKey ikey = cas::Interleaver::PathValue(bkey);
  REQUIRE(ikey.did_ == bkey.did_);
  REQUIRE(Comparator::Equals(ikey.bytes_, expected_key));
}


TEST_CASE("Test ValuePath interleaving", "[cas::Interleaver]") {
  cas::BinaryKey bkey;
  bkey.path_  = { 0x61, 0x62, 0x63, 0x64, 0x65, 0x00 };
  bkey.value_ = { 0x00, 0x01 };

  std::vector<cas::InterleavedByte> expected_key = {
    { .byte_ = 0x00, .type_ = cas::Value },
    { .byte_ = 0x01, .type_ = cas::Value },
    { .byte_ = 0x61, .type_ = cas::Path  },
    { .byte_ = 0x62, .type_ = cas::Path  },
    { .byte_ = 0x63, .type_ = cas::Path  },
    { .byte_ = 0x64, .type_ = cas::Path  },
    { .byte_ = 0x65, .type_ = cas::Path  },
    { .byte_ = 0x00, .type_ = cas::Path  },
  };

  cas::InterleavedKey ikey = cas::Interleaver::ValuePath(bkey);
  REQUIRE(ikey.did_ == bkey.did_);
  REQUIRE(Comparator::Equals(ikey.bytes_, expected_key));
}
