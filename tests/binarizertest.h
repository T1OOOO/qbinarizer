#ifndef BINARIZERTEST_H
#define BINARIZERTEST_H

#include <gtest/gtest.h>
#include <qbinarizer/StructDecoder>
#include <qbinarizer/StructEncoder>

// The fixture for testing class Foo.
class BinarizerTest : public ::testing::Test {
protected:
  // You can remove any or all of the following functions if their bodies would
  // be empty.

  BinarizerTest() {
    // You can do set-up work for each test here.
  }

  ~BinarizerTest() override {
    // You can do clean-up work that doesn't throw exceptions here.
  }

  // If the constructor and destructor are not enough for setting up
  // and cleaning up each test, you can define the following methods:

  void SetUp() override {
    // encoder.clear();
    // decoder.clear();
    //  Code here will be called immediately after the constructor (right
    //  before each test).
  }

  void TearDown() override {
    // encoder.clear();
    // decoder.clear();
    //  Code here will be called immediately after each test (right
    //  before the destructor).
  }

  // Class members declared here can be used by all tests in the test suite
  // for Foo.
  qbinarizer::StructEncoder encoder;
  qbinarizer::StructDecoder decoder;
};

#endif // BINARIZERTEST_H
