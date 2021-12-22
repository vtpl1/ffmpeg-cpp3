#include <Poco/ErrorHandler.h>
#include <Poco/Net/HTTPServer.h>
#include <Poco/Util/HelpFormatter.h>
#include <Poco/Util/ServerApplication.h>
#include <atomic>
#include <chrono>
#include <csignal>
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

class EntryPoint : public Poco::Util::Application
{
private:
  std::string _session_dir{};
  ServerErrorHandler _serverErrorHandler;

public:
  static std::atomic_bool do_shutdown;
  EntryPoint() = default;
  void initialize(Application& self) override
  {
    loadConfiguration(); // load default configuration files, if present
    Poco::ErrorHandler::set(&_serverErrorHandler);
    Application::initialize(self);
  }

  void uninitialize() override { Application::uninitialize(); }

  void defineOptions(Poco::Util::OptionSet& options) override
  {
    Application::defineOptions(options);
    options.addOption(Poco::Util::Option("help", "h", "display help information on command line arguments")
                          .required(false)
                          .repeatable(false));
  }

  void handleOption(const std::string& name, const std::string& value) override
  {
    Application::handleOption(name, value);
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

  static void sigHandlerAppClose(int l_signo)
  {
    if (l_signo == SIGINT) {
      RAY_LOG(WARNING) << "Signal: " << l_signo << ":SIGINT(Interactive attention signal) received.\n";
      do_shutdown = true;
    }
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
    if (signal(SIGINT, EntryPoint::sigHandlerAppClose) == SIG_ERR) {
      RAY_LOG(FATAL) << "Can't attach sigHandlerAppClose signal\n";
      return ExitCode::EXIT_OSERR;
    }
    Poco::Net::initializeNetwork();
    std::unique_ptr<ffpp::InputToMessageQueue> input_to_message_queue(
        new ffpp::InputToMessageQueue("rtsp://admin:AdmiN1234@192.168.0.58/h264/ch1/main")
        // new ffpp::InputToMessageQueue("rtmp://0.0.0.0:9005")
    );
    std::unique_ptr<ffpp::OutputFromMessageQueue> output_from_message_queue(
        new ffpp::OutputFromMessageQueue("rtmp://192.168.1.116:9101", input_to_message_queue->GetMessageQueue()));
    input_to_message_queue->Start();
    output_from_message_queue->Start();
    while (!do_shutdown) {
      std::this_thread::sleep_for(std::chrono::microseconds(100));
      if (input_to_message_queue->IsDone())
        break;
      if (output_from_message_queue->IsDone())
        break;
    }
    input_to_message_queue->SignalToStop();
    output_from_message_queue->SignalToStop();
    output_from_message_queue->Stop();
    output_from_message_queue = nullptr;
    input_to_message_queue->Stop();
    input_to_message_queue = nullptr;

    RAY_LOG(INFO) << "main Stopped: " << name_of_app;
    Poco::Net::uninitializeNetwork();
    return Application::EXIT_OK;
  }
};

std::atomic_bool EntryPoint::do_shutdown{false};

POCO_APP_MAIN(EntryPoint);
