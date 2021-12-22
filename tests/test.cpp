// *****************************************************
//  Copyright 2021 Videonetics Technology Pvt Ltd
// *****************************************************

#include <catch2/catch.hpp>
#include <cctype>
#include <iomanip>
#include <iostream>
#include <map>
#include <string>

#include "Demuxer.h"
#include "FFmpegException.h"
#include <Poco/Process.h>
#include <Poco/URI.h>
/*
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
*/

std::string url_encode(const std::string& value)
{
  std::ostringstream escaped;
  // escaped.fill('0');
  // escaped << std::hex;

  for (std::string::const_iterator i = value.begin(), n = value.end(); i != n; ++i) {
    std::string::value_type c = (*i);

    // Keep alphanumeric and other accepted characters intact
    //._+:@%/-
    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~' || c == '@' || c == ':' || c == '+' || c == '%' ||
        c == '/') {
      escaped << c;
      continue;
    }

    // Any other characters are percent-encoded
    // escaped << std::uppercase;
    // escaped << '\\' << std::setw(2) << int((unsigned char)c);
    // escaped << std::nouppercase;
    escaped << '\\' << c;
  }

  return escaped.str();
}
TEST_CASE("utility test", "[utility]")
{
  std::string input_name; //= "rtmp://192.168.1.122//rtsp//000";
  REQUIRE((input_name.rfind("rtsp", 0) == 0));
}

TEST_CASE("uri encode", "[utility]")
{
  std::string input_name =
      "rtsp://poc:aDmin@4213@10.0.31.34:554/cam/realmonitor?channel=1&subtype=1&unicast=true&proto=Onvif";
  // Poco::URI uri(input_name);
  // std::string out_str;

  // Poco::URI::encode(input_name, "&", out_str); // uri(input_name);
  std::string out_str2 = url_encode(input_name);
  std::cout << "Input:   " << input_name << std::endl;
  // std::cout << "Output1: " << out_str << std::endl;
  std::cout << "Output2: " << out_str2 << std::endl;
  Poco::Process::Args arg;
  arg.push_back("-i");
  arg.push_back(input_name);
  // rtsp://poc:aDmin@4213@10.0.31.34:554/cam/realmonitor?channel%3D1%26subtype%3D1%26unicast%3Dtrue%26proto%3DOnvif

  REQUIRE(input_name == "a");
}
