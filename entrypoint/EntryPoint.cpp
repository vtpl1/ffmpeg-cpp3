#include <Poco/ErrorHandler.h>
#include <Poco/Net/HTTPServer.h>
#include <Poco/Util/HelpFormatter.h>
#include <Poco/Util/ServerApplication.h>
#include <atomic>
#include <chrono>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>

#include "InputToMessageQueue.h"
#include "OutputFromMessageQueue.h"
#include "logging.h"

class ServerErrorHandler : public Poco::ErrorHandler
{
public:
  void exception(const Poco::Exception& exc) override { RAY_LOG(FATAL) << "Poco::Exception " << exc.what(); }
  void exception(const std::exception& exc) override { RAY_LOG(FATAL) << "std::exception " << exc.what(); }
  void exception() override { RAY_LOG(FATAL) << "unknown exception "; }
};

class EntryPoint : public Poco::Util::ServerApplication
{
private:
  std::string _session_dir{};
  ServerErrorHandler _serverErrorHandler;

public:
  EntryPoint() = default;
  void initialize(Application& self) override
  {
    loadConfiguration(); // load default configuration files, if present
    Poco::ErrorHandler::set(&_serverErrorHandler);
    ServerApplication::initialize(self);
  }

  void uninitialize() override { ServerApplication::uninitialize(); }

  void defineOptions(Poco::Util::OptionSet& options) override
  {
    ServerApplication::defineOptions(options);
    options.addOption(Poco::Util::Option("help", "h", "display help information on command line arguments")
                          .required(false)
                          .repeatable(false));
  }

  void handleOption(const std::string& name, const std::string& value) override
  {
    ServerApplication::handleOption(name, value);
  }

  void displayHelp()
  {
    Poco::Util::HelpFormatter helpFormatter(options());
    helpFormatter.setCommand(commandName());
    helpFormatter.setUsage("OPTIONS");
    helpFormatter.setHeader("A web server that shows how to work with HTML forms.");
    helpFormatter.format(std::cout);
  }
  const std::string& get_session_folder()
  {
    if (_session_dir.empty()) {
      Poco::Path base_path;
      if (config().getBool("application.runAsService", false) || config().getBool("application.runAsDaemon", false)) {
        base_path.assign(config().getString("application.dataDir", config().getString("application.dir", "./")));
        base_path.pushDirectory(config().getString("application.baseName", "ojana"));
      } else {
        base_path.assign(config().getString("application.dir", "./"));
      }
      base_path.pushDirectory("session");
      std::cout << "Session folder is at: " << base_path.toString() << std::endl;
      _session_dir = base_path.toString();
    }
    return _session_dir;
  }

  int main(const ArgVec& args)
  {
    std::string name_of_app = config().getString("application.baseName");
    //::ray::RayLog::StartRayLog(name_of_app, ::ray::RayLogLevel::DEBUG, get_session_folder());
    std::string a;
    ::ray::RayLog::StartRayLog(name_of_app, ::ray::RayLogLevel::DEBUG, a);
    RAY_LOG(INFO) << "main Started: " << name_of_app;
    std::string data_dir("");
    if (data_dir.empty()) {
      data_dir = get_session_folder();
    }
    Poco::Net::initializeNetwork();
    {
      ffpp::InputToMessageQueue i("");
      ffpp::OutputFromMessageQueue o("");
      exit(1);
      waitForTerminationRequest();
    }
    RAY_LOG(INFO) << "main Stopped: " << name_of_app;
    Poco::Net::uninitializeNetwork();
    return Application::EXIT_OK;
  }
};

POCO_SERVER_MAIN(EntryPoint);
