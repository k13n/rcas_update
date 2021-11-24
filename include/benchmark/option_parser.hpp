#pragma once

#include <iostream>
#include <getopt.h>
#include "cas/update_type.hpp"


namespace benchmark {


struct Config {
  std::string input_filename_ = "";
  char dataset_delim_ = ';';
  double percent_query_ = 0.6;
  double percent_bulkload_ = 1.0;
  cas::InsertMethod insert_method_ = cas::MainLF;
  cas::MergeMethod merge_method_ = cas::MergeMethod::Fast;
  std::string perf_datafile_ = "perf.data";
};


namespace option_parser {


void ParseInt(
      const char* optarg,
      int& target,
      const char* option_name)
{
  if (sscanf(optarg, "%d", &target) != 1) {
    std::cerr << "Could not parse option --"
      << std::string{option_name}
      << "=" << std::string{optarg} << " (integer expected)\n";
    exit(-1);
  }
}


void ParseDouble(
      const char* optarg,
      double& target,
      const char* option_name)
{
  if (sscanf(optarg, "%lf", &target) != 1) {
    std::cerr << "Could not parse option --"
      << std::string{option_name}
      << "=" << std::string{optarg} << " (double expected)\n";
    exit(-1);
  }
}


void Parse(int argc, char** argv, benchmark::Config& config) {
  const int OPT_INPUT_FILENAME = 1;
  const int OPT_BULKLOAD_PERCENT = 2;
  const int OPT_QUERY_PERCENT = 3;
  const int OPT_INSERT_METHOD = 4;
  const int OPT_MERGE_METHOD = 5;
  const int OPT_PERF_DATAFILE = 6;
  static struct option long_options[] = {
    {"input_filename",    required_argument, nullptr, OPT_INPUT_FILENAME},
    {"bulkload_percent",  required_argument, nullptr, OPT_BULKLOAD_PERCENT},
    {"query_percent",     required_argument, nullptr, OPT_QUERY_PERCENT},
    {"insert_method",     required_argument, nullptr, OPT_INSERT_METHOD},
    {"merge_method",      required_argument, nullptr, OPT_MERGE_METHOD},
    {"perf_datafile",     required_argument, nullptr, OPT_PERF_DATAFILE},
    {0, 0, 0, 0}
  };

  while (true) {
    int option_index;
    int c = getopt_long(argc, argv, "", long_options, &option_index);
    if (c == -1) {
      break;
    }
    std::string optvalue{optarg};
    switch (c) {
      case OPT_INPUT_FILENAME:
        config.input_filename_ = optvalue;
        break;
      case OPT_BULKLOAD_PERCENT:
        ParseDouble(optarg, config.percent_bulkload_ , long_options[option_index].name);
        if (!(0 <= config.percent_bulkload_ && config.percent_bulkload_ <= 1)) {
          std::cerr << "--bulkload_percent must be between 0 and 1";
          exit(-1);
        }
        break;
      case OPT_QUERY_PERCENT:
        ParseDouble(optarg, config.percent_query_ , long_options[option_index].name);
        if (!(0 <= config.percent_query_ && config.percent_query_ <= 1)) {
          std::cerr << "--bulkload_percent must be between 0 and 1";
          exit(-1);
        }
        break;
      case OPT_INSERT_METHOD:
        int insert_method;
        ParseInt(optarg, insert_method, long_options[option_index].name);
        config.insert_method_ = cas::InsertMethodFromInt(insert_method);
        break;
      case OPT_MERGE_METHOD:
        int merge_method;
        ParseInt(optarg, merge_method, long_options[option_index].name);
        config.merge_method_ = cas::MergeMethodFromInt(merge_method);
        break;
      case OPT_PERF_DATAFILE:
        config.perf_datafile_ = optvalue;
        break;
    }
  }
}


benchmark::Config Parse(int argc, char** argv) {
  benchmark::Config context;
  Parse(argc, argv, context);
  return context;
}


}; // namespace option_parser
}; // namespace benchmark
