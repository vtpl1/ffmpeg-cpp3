// *****************************************************
//  Copyright 2021 Videonetics Technology Pvt Ltd
// *****************************************************

#include <catch2/catch.hpp>
#include <iostream>
#include <map>
#include <string>

#include "Demuxer.h"
#include "FFmpegException.h"

TEST_CASE("demuxer test", "[demuxer]")
{
  std::multimap<std::string, std::string> options;
  options.emplace(std::make_pair<std::string, std::string>("listen", "1"));
  options.emplace(std::make_pair<std::string, std::string>("timeout", "10"));
  options.emplace(std::make_pair<std::string, std::string>("rtmp_buffer", "100"));
  options.emplace(std::make_pair<std::string, std::string>("rtmp_live", "live"));
  options.emplace(std::make_pair<std::string, std::string>("analyzeduration", "32"));
  options.emplace(std::make_pair<std::string, std::string>("probesize", "32"));
  options.emplace(std::make_pair<std::string, std::string>("sync", "1"));
  options.emplace(std::make_pair<std::string, std::string>("fflags", "nobuffer"));
  options.emplace(std::make_pair<std::string, std::string>("fflags", "flush_packets"));

  std::unique_ptr<ffpp::InputSource> demuxer;
  REQUIRE_THROWS_AS(demuxer.reset(new ffpp::Demuxer("rtmp://0.0.0.0:9105", "flv", options)), ffpp::FFmpegException);

  // REQUIRE(demuxer.get() != nullptr);
}