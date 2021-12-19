#include <Demuxer.h>
#include <catch2/catch.hpp>
#include <iostream>
#include <map>
#include <string>

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

  std::unique_ptr<ffpp::InputSource> demuxer(new ffpp::Demuxer("rtmp://0.0.0.0:9105", "flv", options));
  REQUIRE(demuxer.get() != nullptr);
}