#include <unistd.h>
#include <stdlib.h>



#include <fstream>
#include <ftxui/component/receiver.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/screen/string.hpp>
#include <string>
#include <thread>

#include "args.h"
#include <iostream>

#include "ui/main_component.hpp"
#include "data/session.h"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"

#include <chrono>

// Read from the file descriptor |fd|. Produce line of log and send them to
// |sender|. The |screen| wakes up when this happens.
void LineProducer(int fd,
                  Sender<std::wstring> sender,
                  ScreenInteractive* screen) {
  std::string line;
  int buffer_size = 1 << 10;
  char buffer[buffer_size];
  while (true) {
    int c = read(fd, buffer, buffer_size);
    for (int i = 0; i < c; ++i) {
      if (buffer[i] == '\n') {
        sender->Send(to_wstring(line));
        line = "";
      } else {
        line.push_back(buffer[i]);
      }
    }

    // Refresh the screen:
    screen->PostEvent(Event::Custom);

    if (c == 0) {
      using namespace std::chrono_literals;
      std::this_thread::sleep_for(0.5s);
    }
  }
}



// selector https://docs.diffusiondata.com/docs/6.1.5/manual/html-single/diffusion_single.html#topic_selector_unified
ARG_OPTS_T arg_opts[] = {
    ARG_OPTS_HELP,
    {'u', "url", "Diffusion server URL", ARG_OPTIONAL, ARG_HAS_VALUE, "ws://localhost:8080"},
    {'p', "principal", "Principal (username) for the connection", ARG_OPTIONAL, ARG_HAS_VALUE, "client"},
    {'c', "credentials", "Credentials (password) for the connection", ARG_OPTIONAL, ARG_HAS_VALUE, "password"},
    {'d', "delay", "Delay between reconnection attempts, in ms", ARG_OPTIONAL, ARG_HAS_VALUE, "2000" },
    {'r', "retries", "Reconnection retry attempts", ARG_OPTIONAL, ARG_HAS_VALUE, "5" },
    {'t', "timeout", "Reconnection timeout for a disconnected session", ARG_OPTIONAL, ARG_HAS_VALUE, NULL },
    {'s', "sleep", "Time to sleep before disconnecting (in seconds).", ARG_OPTIONAL, ARG_HAS_VALUE, "5" },
    END_OF_ARG_OPTS
};

int main(int argc, char** argv) {

  try
  {
    auto logger = spdlog::basic_logger_mt("basic_logger", "logs/infolog.txt", true);
    logger->flush_on(spdlog::level::debug);
    spdlog::set_default_logger(logger);
  }
  catch (const spdlog::spdlog_ex &ex)
  {
    std::cerr << "Log init failed: " << ex.what() << std::endl;
    return 1;
  }

  spdlog::set_level(spdlog::level::debug);


  /*
    * Standard command-line parsing.
    */
    HASH_T *options = parse_cmdline(argc, argv, arg_opts);
    if(options == nullptr || hash_get(options, "help") != nullptr) {
        // replace show usage due to fprintf wrapped in the diffusion library
        std::cout << "Usage:\n";
        for (size_t i = 0; i < sizeof(arg_opts)/sizeof(arg_opts[0]) - 1; ++i)
        {
          std::cout << "-" << arg_opts[i].short_arg << "/-" << arg_opts[i].long_arg << "  "
                << arg_opts[i].description;

          if (arg_opts[i].default_value != nullptr){
            std::cout << " default value: " << arg_opts[i].default_value;
          }

          std::cout << "\n";
        }
        return EXIT_FAILURE;
    }

    const char *url = static_cast<const char*>(hash_get(options, "url"));
    const char *principal = static_cast<const char*>(hash_get(options, "principal"));
    const char *password = static_cast<const char*>(hash_get(options, "credentials"));

    //long retry_delay = std::atol(static_cast<const char*>(hash_get(options, "delay")));
    //long retry_count = std::atoi(static_cast<const char*>(hash_get(options, "retries")));
    long reconnect_timeout;
    if(hash_get(options, "timeout") != NULL) {
      reconnect_timeout = std::atol(static_cast<const char*>(hash_get(options, "timeout")));
    }
    else {
      reconnect_timeout = -1;
    }

    //const unsigned int sleep_time = std::atol(static_cast<const char*>(hash_get(options, "sleep")));
    //const char* selector = static_cast<const char*>(hash_get(options,"selector"));

    spdlog::info("application has started url {} principal {} password {}", url, principal, reconnect_timeout);
    Session session;
    Error e;
    if (!session.connect(url, principal, password, e)) {
      spdlog::warn("Connection error {} message {}",  error2Str(e.m_code), e.m_message);
      std::cerr << "Connection error " << error2Str(e.m_code) << ": " << e.m_message << std::endl;
      return EXIT_FAILURE;
    }

  auto screen = ScreenInteractive::Fullscreen();
  Animator animator(screen);
  auto component = std::make_shared<MainComponent>(session, screen.ExitLoopClosure());

  session.setFetchCompletedCallback([&screen, &animator, &session, &component](std::string&& selector) {
    animator.stop("fetch");
    screen.Post([&component, tpx = session.getFetchTopics(), sel = std::move(selector)]() mutable {
            component->onFetchCompleted(std::string(), std::move(tpx), std::move(sel));
          });
    screen.PostEvent(Event::Special("fetch"));
  });

  session.setFetchErrorCallback([&screen, &animator, &component](Error error) {
    animator.stop("fetch");
    screen.Post([&component, error]() mutable {
      component->onFetchCompleted(error.m_message, std::vector<Topic>(), std::string());
    });
    screen.PostEvent(Event::Special("fetch"));
  });

  session.setFetchDiscardCallback([&screen, &animator, &component](std::string&& selector) {
    animator.stop("fetch");
    screen.Post([&component, sel = std::move(selector)]() mutable {
      component->onFetchCompleted("Discard", std::vector<Topic>(), std::move(sel));
    });
    screen.PostEvent(Event::Special("fetch"));
  });

  session.setFetchStartCallback([&animator]() {
           animator.start("fetch");
  });

  session.setSubscribeCompletedCallback([&screen, &animator, &component, &session](std::string&& selector) {
    animator.stop("subscribe");
    screen.Post([&component, tpx = session.getSubscribeTopics(), sel = std::move(selector)]() mutable {
      component->onSubscribeCompleted(std::string(), std::move(tpx), std::move(sel));
    });
    screen.PostEvent(Event::Special("subscribe"));
  });

  session.setSubscribeStartCallback([&animator](){
    animator.start("subscribe");
  });

  session.setSubscribeErrorCallback([&screen, &animator, &component](Error error) {
    animator.stop("subscribe");
    screen.Post([&component, error]() mutable {
      component->onSubscribeCompleted(error.m_message, std::vector<Topic>(), std::string());
    });
    screen.PostEvent(Event::Special("subscribe"));
  });

  session.notify();
  screen.Loop(component);
  spdlog::info("finished");
  return EXIT_SUCCESS;
}
