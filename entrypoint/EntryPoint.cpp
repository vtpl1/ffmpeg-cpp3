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

  bool _help_requested{false};
  std::string _input_url{};
  std::string _output_url{};
  int _open_or_listen_timeout_in_sec{10};
  std::string _name_of_app;

public:
  static std::atomic_bool do_shutdown;
  EntryPoint() { setUnixOptions(true); }
  void initialize(Application& self) override
  {
    loadConfiguration(); // load default configuration files, if present
    Poco::ErrorHandler::set(&_serverErrorHandler);
    Application::initialize(self);
    Poco::Net::initializeNetwork();
    std::string data_dir;
    if (data_dir.empty()) {
      data_dir = get_session_folder();
    }
    _name_of_app = config().getString("application.baseName");
    std::cout << "Session folder at: " << data_dir << std::endl;
    ::ray::RayLog::StartRayLog(_name_of_app, ::ray::RayLogLevel::INFO, data_dir);
  }

  void uninitialize() override
  {
    Poco::Net::uninitializeNetwork();
    Application::uninitialize();
  }

  void defineOptions(Poco::Util::OptionSet& options) override
  {
    options.addOption(Poco::Util::Option("help", "h", "display help information on command line arguments")
                          .required(false)
                          .repeatable(false));
    options.addOption(
        Poco::Util::Option("input", "i", "input media url").required(true).repeatable(false).argument("name=value"));
    options.addOption(
        Poco::Util::Option("output", "o", "output media url").required(true).repeatable(false).argument("name=value"));
    options.addOption(Poco::Util::Option("timeout", "t", "Open or listen time out in sec")
                          .required(false)
                          .repeatable(false)
                          .argument("name=value"));
    Application::defineOptions(options);
  }

  void handleOption(const std::string& name, const std::string& value) override
  {
    std::cout << "name : " << name << " value: " << value << std::endl;
    if (name == "help") {
      _help_requested = true;
    } else if (name == "input") {
      _input_url = value;
    } else if (name == "output") {
      _output_url = value;
    } else if (name == "timeout") {
      _open_or_listen_timeout_in_sec = std::stoi(value);
    }
    Application::handleOption(name, value);
  }

  void displayHelp()
  {
    Poco::Util::HelpFormatter helpFormatter(options());
    helpFormatter.setCommand(commandName());
    helpFormatter.setUsage("OPTIONS");
    helpFormatter.setHeader("Media handler class.");
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
    if (_help_requested) {
      displayHelp();
      return Application::EXIT_CONFIG;
    }
    //::ray::RayLog::StartRayLog(name_of_app, ::ray::RayLogLevel::DEBUG, get_session_folder());
    if (signal(SIGINT, EntryPoint::sigHandlerAppClose) == SIG_ERR) {
      RAY_LOG(FATAL) << "Can't attach sigHandlerAppClose signal\n";
      return ExitCode::EXIT_OSERR;
    }
    RAY_LOG(INFO) << "main Started: " << _name_of_app;
    RAY_LOG_INF << "Starting with input : " << _input_url << " : output : " << _output_url;
    std::unique_ptr<ffpp::InputToMessageQueue> input_to_message_queue(
        // rtsp://admin:AdmiN1234@192.168.0.58/h264/ch1/main/
        // rtsp://admin:Admin@123192.168.0.42:554/0/onvif/profile1/media.smp
        // rtsp://admin:AdmiN1234@192.168.0.91/axis-media/media.amp
        new ffpp::InputToMessageQueue(_input_url, _open_or_listen_timeout_in_sec)
        // new ffpp::InputToMessageQueue("rtmp://0.0.0.0:9005")
    );
    int ret = 2;
    if (input_to_message_queue->Start()) {
      --ret;
      std::unique_ptr<ffpp::OutputFromMessageQueue> output_from_message_queue(new ffpp::OutputFromMessageQueue(
          _output_url, input_to_message_queue->GetMessageQueue(), input_to_message_queue->GetInputAVFormatContext(),
          _open_or_listen_timeout_in_sec));

      if (output_from_message_queue->Start()) {
        --ret;
        while (!do_shutdown) {
          std::this_thread::sleep_for(std::chrono::microseconds(100));
          if (input_to_message_queue->IsDone())
            break;
          if (output_from_message_queue->IsDone())
            break;
        }
        output_from_message_queue->SignalToStop();
        output_from_message_queue->Stop();
        output_from_message_queue = nullptr;
      }
      input_to_message_queue->SignalToStop();
      input_to_message_queue->Stop();
    }
    input_to_message_queue = nullptr;

    RAY_LOG(INFO) << "main Stopped: " << _name_of_app;
    // return Application::EXIT_OK;
    return ret;
  }
};

std::atomic_bool EntryPoint::do_shutdown{false};

POCO_APP_MAIN(EntryPoint);
