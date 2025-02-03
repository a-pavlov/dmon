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

using namespace ftxui;


class Animator {
 private:
  std::condition_variable m_cv;
  std::mutex m_m;
  ScreenInteractive& m_screen;
  bool m_tick{false};
  bool m_stop{false};
  std::thread m_thread;
 public:
  Animator(ScreenInteractive& screen) : m_screen(screen) {
    m_thread = std::thread([this]() {
      while(true) {
        {
          std::unique_lock<std::mutex> lk(m_m);
          m_cv.wait(lk, [this] { return m_tick || m_stop; });
          spdlog::info("Post event, stop {}", m_stop);
          if (m_stop) break;
          m_screen.PostEvent(Event::Special("animation"));
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

  void start() {
    {
      std::lock_guard<std::mutex> lk(m_m);
      m_tick = true;
    }
    m_cv.notify_one();
  }

  void stop() {
    {
      std::lock_guard<std::mutex> lk(m_m);
      m_tick = false;
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
  static std::mutex test_lock;
  static std::string test_data;
  MainComponent(Session& session, Animator&, Closure&& screenExit);
  Element Render() override;
  bool OnEvent(Event) override;

 private:
  Closure m_screen_exit_;
  std::string m_current_payload;
  std::string m_search_selector;

  std::map<int, std::pair<std::wstring, std::map<int, std::wstring>>>
      translation_;

  int tab_selected_ = 0;
  std::vector<std::wstring> tab_entries_ = {
      L"Search",
      L"Subscription",
      L"Info",
  };
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
      m_animator.start();
    }
  }});
  Component m_btn_search_ = Button("Search", [&]{
        if (!m_search_selector.empty() && m_session.fetch(m_search_selector)) {
          log_displayer_1_->clearSelected();
          m_spinner_indx = 0;
          m_animator.start();
        }
      }, ButtonOption::Ascii());
  Component m_payload_text_box_ = Input(&m_current_payload, InputOption{.multiline = true});
  Component m_btn_exit_ = Button("Exit", m_screen_exit_, ButtonOption::Ascii());
  Component m_btn_copy_ = Button("Copy", [&](){
        //m_animator.start();
      }, ButtonOption::Ascii());
  Component m_btn_clear_ = Button("Clear", [&](){
        m_search_selector.clear();
        //m_animator.stop();
      }, ButtonOption::Ascii());

  Component m_error_report = Renderer([&] {
    return window(text("Fetching status"),
                                             hbox(
                                                 text(m_session.isFetchInProgress()?std::string("In progress"):m_session.getLastFetchStatusStr())| color(m_session.getLastFetchStatus().m_code!=DIFF_ERR_SUCCESS?Color::Red:Color::Yellow),
                                                 separator(),
                                                 spinner(18, m_spinner_indx)));
  }) | Maybe([&] { return m_session.isFetchInProgress() || m_session.getLastFetchStatus().m_code != DIFF_ERR_SUCCESS; });

  Session& m_session;
  size_t m_spinner_indx{0};
  Animator& m_animator;
};

#endif /* end of include guard: UI_MAIN_COMPONENT_HPP */
