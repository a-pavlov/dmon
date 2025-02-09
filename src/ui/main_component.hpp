#ifndef UI_MAIN_COMPONENT_HPP
#define UI_MAIN_COMPONENT_HPP

#include <ftxui/component/component.hpp>
#include <ftxui/component/component.hpp>
#include "ftxui/component/component_base.hpp"
#include "ftxui/component/component_options.hpp"  // for InputOption
#include <ftxui/component/task.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <map>

#include "ui/log_displayer.hpp"
#include "ui/info_component.hpp"

#include "data/session.h"
#include "spdlog/spdlog.h"
#include "clip.h"

using namespace ftxui;


class Animator {
 private:
  std::condition_variable m_cv;
  std::mutex m_m;
  ScreenInteractive& m_screen;
  bool m_stop{false};
  std::thread m_thread;
  std::vector<std::string> m_signals;
 public:
  Animator(ScreenInteractive& screen) : m_screen(screen) {
    m_thread = std::thread([this]() {
      while(true) {
        {
          std::unique_lock<std::mutex> lk(m_m);
          m_cv.wait(lk, [this] { return !m_signals.empty() || m_stop; });
          spdlog::info("Post event, stop {}", m_stop);
          if (m_stop) break;
          std::for_each(m_signals.begin(), m_signals.end(), [this](const auto& sig){
            m_screen.PostEvent(Event::Special(sig));
          });
        }
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(0.2s);
      }

      spdlog::info("Thread finished");
    });
  }

  ~Animator() {
    {
      std::lock_guard<std::mutex> lk(m_m);
      m_stop = true;
    }
    m_cv.notify_one();
    m_thread.join();
  }

  void start(const std::string& sig) {
    {
      std::lock_guard<std::mutex> lk(m_m);
      if (std::none_of(m_signals.begin(), m_signals.end(), [&sig](const auto& s) { return s == sig;})) {
        m_signals.push_back(sig);
      }
    }
    m_cv.notify_one();
  }

  void stop(const std::string& sig) {
    {
      std::lock_guard<std::mutex> lk(m_m);
      m_signals.erase(std::remove(m_signals.begin(), m_signals.end(), sig), m_signals.end());
    }
    m_cv.notify_one();
  }

  void close() {
    {
      std::lock_guard<std::mutex> lk(m_m);
      m_stop = true;
    }

    m_cv.notify_one();
  }
};


class MainComponent : public ComponentBase {
 public:
  static std::string test_data;
  MainComponent(Session& session, Closure&& screenExit);
  Element Render() override;
  bool OnEvent(Event) override;

  void onFetchCompleted(const std::string& errorMessage, std::vector<Topic>&& topics) {
    m_topics = std::move(topics);
    m_fetch_error_message = errorMessage;
  }

  void onSubscribeCompleted(const std::string& errorMessage, std::vector<Topic>&& topics) {
    std::copy(topics.begin(), topics.end(), std::back_inserter(m_subscribe_topics));
    m_subscribe_error_message = errorMessage;
  }

 private:
  Closure m_screen_exit_;
  std::string m_current_payload;
  std::string m_search_selector;
  std::string m_subscribe_payload;

  int tab_selected_ = 0;
  std::vector<std::wstring> tab_entries_ = {
      L"Search",
      L"Subscription",
      L"Info",
  };

  std::vector<Topic> m_topics;
  std::vector<Topic> m_subscribe_topics;
  std::string m_fetch_error_message;
  std::string m_subscribe_error_message;

  Component toggle_ = Toggle(&tab_entries_, &tab_selected_);
  Component container_level_filter_ = Container::Vertical({});
  Component container_thread_filter_ = Container::Horizontal({});
  std::shared_ptr<LogDisplayer> log_displayer_1_;
  std::shared_ptr<LogDisplayer> log_displayer_2_;
  Component info_component_ = Make<InfoComponent>();
  Component container_search_selector_ = Input(&m_search_selector, "", InputOption{.multiline=false, .on_change=[&](){
  }, .on_enter = [&](){
    if (!m_search_selector.empty() && m_session.fetch(m_search_selector)) {
      log_displayer_1_->clearSelected();
      m_spinner_indx = 0;
    }
  }});
  Component m_btn_search_ = Button("Search", [&]{
        if (!m_search_selector.empty() && m_session.fetch(m_search_selector)) {
          log_displayer_1_->clearSelected();
          m_spinner_indx = 0;
        }
      }, ButtonOption::Ascii());
  Component m_payload_text_box_ = Input(&m_current_payload, InputOption{.multiline = true});
  Component m_btn_exit_ = Button("Exit", m_screen_exit_, ButtonOption::Ascii());
  Component m_btn_copy_ = Button("Copy", [&](){
        spdlog::debug("Copy to clipboard: {}", m_current_payload);
        if (!clip::set_text(m_current_payload)) {
          spdlog::debug("Copy to clipboard failed");
        }else {
          spdlog::debug("Copied!!!");
        }
      }, ButtonOption::Ascii());
  Component m_btn_clear_ = Button("Clear", [&](){
        m_search_selector.clear();
      }, ButtonOption::Ascii());

  Component m_error_report = Renderer([&] {
    return window(text("Fetching status"),
                                             hbox(
                                                 text(m_session.isFetchInProgress()?std::string("In progress"):m_fetch_error_message)| color(m_fetch_error_message.empty()?Color::Yellow:Color::Red),
                                                 separator(),
                                                 spinner(18, m_spinner_indx)));
  }) | Maybe([&] { return m_session.isFetchInProgress() || !m_fetch_error_message.empty(); });



  std::string m_subscribe_selector;

  Component m_subsribe_error_report = Renderer([&] {
                               return window(text("Subscribe status"),
                                             hbox(
                                                 text(m_session.isSubscribtionInProgress()?std::string("In progress"):m_subscribe_error_message)| color(m_subscribe_error_message.empty()?Color::Yellow:Color::Red),
                                                 separator(),
                                                 spinner(18, m_subscribtion_spinner_indx)));
                             }) | Maybe([&] { return m_session.isSubscribtionInProgress() || !m_subscribe_error_message.empty(); });

  Component m_subscribe_selector_ = Input(&m_subscribe_selector, "", InputOption{.multiline=false, .on_change=[&](){
                                                                                   }, .on_enter = [&](){
                                                                                     if (!m_subscribe_selector.empty() && m_session.subscribe(m_subscribe_selector)) {
                                                                                       log_displayer_2_->clearSelected();
                                                                                       m_subscribtion_spinner_indx = 0;
                                                                                     }
                                                                                   }});

  Component m_btn_subscribe_ = Button("Subscribe", [&]{
        if (!m_subscribe_selector.empty() && m_session.subscribe(m_subscribe_selector)) {
          log_displayer_2_->clearSelected();
          m_subscribtion_spinner_indx = 0;
        }
      }, ButtonOption::Ascii());

  Component m_subscribe_payload_text_box_ = Input(&m_subscribe_payload, InputOption{.multiline = true});

  Session& m_session;
  size_t m_spinner_indx{0};
  size_t m_subscribtion_spinner_indx{0};
};

#endif /* end of include guard: UI_MAIN_COMPONENT_HPP */
