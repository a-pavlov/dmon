#include "ui/main_component.hpp"

#include <ftxui/dom/elements.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/screen/string.hpp>
#include "data/session.h"

using namespace ftxui;


MainComponent::MainComponent(Session& session, Closure&& screen_exit)
    : m_screen_exit_(std::move(screen_exit)),
      log_displayer_1_(Make<LogDisplayer>()),
      log_displayer_2_(Make<LogDisplayer>()),
      m_session(session)
    {
  Add(Container::Vertical({
      toggle_,
      Container::Tab(
          {
              Container::Vertical({
                  Container::Horizontal({
                      container_search_selector_,
                      m_btn_search_,
                      m_btn_clear_
                  }),
                  m_error_report,
                  log_displayer_1_,
                  m_payload_text_box_,
                  m_btn_copy_
              }),
              Container::Vertical({
                  Container::Horizontal({
                      m_subscribe_selector_,
                      m_btn_subscribe_//,
                      //m_btn_clear_
                  }),
                  m_subsribe_error_report,
                  container_level_filter_,
                  log_displayer_2_,
                  m_subscribe_payload_text_box_
                  //m_btn_copy_
              }),
              Container::Vertical({m_btn_dump_exit, m_btn_exit_})
          },
          &tab_selected_)//,
       //m_btn_exit_
  }));

}

bool MainComponent::OnEvent(Event event) {
  /*if (event == Event::Special("fetch")) {
    std::lock_guard<std::mutex> lk(test_lock);
    m_current_payload = test_data;
  }*/

  if (event == Event::Special("fetch")) {
    ++m_spinner_indx;
  }

  if (event == Event::Special("subscribe")) {
    ++m_subscribtion_spinner_indx;
  }

  return ComponentBase::OnEvent(event);
}


Element MainComponent::Render() {
  m_current_payload = log_displayer_1_->GetSelected();
  m_subscribe_payload = log_displayer_2_->GetSelected();
  auto lines_count = std::min(tab_selected_, 1) == 0 ? m_topics.size(): m_subscribe_topics.size();

  int current_line =
      (std::min(tab_selected_, 1) == 0 ? log_displayer_1_ : log_displayer_2_)
          ->selected();

  auto header = hbox({
      hbox(text(m_session.getAddress()) | color(Color::LightGreen)),
      separator(),
      hcenter(toggle_->Render()),
      separator(),
      //m_btn_exit_->Render(),
      //separator(),
      text(to_wstring(current_line)),
      text(L"/"),
      text(std::to_string(lines_count)),
      text(L"  ["),
      text(to_wstring(0)),
      text(L"]"),
      separator(),
      gauge(float(current_line) /
            float(std::max(1, (int)lines_count - 1))) |
          color(Color::GrayDark),
      //separator()//,
      //spinner(18, m_spinner_indx),
  });

  std::vector<std::string> thread_filters_order_{"111", "2222", "3333"};

  Elements thread_filter_document;
  thread_filter_document.push_back(vbox({
      text(L"Process:") | bold,
      separator(),
      text(L"Thread:") | bold,
      filler()
  }));

  for (auto key : thread_filters_order_) {
    Elements checkboxes;
    std::string c = key;
    std::string c_str;
    c_str += c;
    checkboxes.push_back(text(c_str));
    checkboxes.push_back(separator());
    //checkboxes.push_back(thread_filters_[key]->container->Render());
    thread_filter_document.push_back(separator());
    thread_filter_document.push_back(vbox(checkboxes));
  }


  Element tab_menu;
  if (tab_selected_ == 0) {
    return  //
        vbox({
            header,
            separator(),
            window(text(L"Selector"), hbox(text("Enter path:"), separator(), container_search_selector_->Render(),
                         m_btn_search_->Render(), m_btn_clear_->Render()) | notflex),
             m_error_report->Render(),

            /*hbox({
                window(text(L"Type"), container_level_filter_->Render()) |
                    notflex,
                text(L" "),
                window(text(L"Filter"), hbox(thread_filter_document)) | notflex,
                text(L" ")
                //window(text(L"Selector"), hbox(container_search_selector_->Render(), m_btn_search_->Render())) | flex,
                //filler(),
            }) | notflex,*/
            log_displayer_1_->RenderLines(m_topics) | flex_shrink,
            window(text("Content"), hbox(m_payload_text_box_->Render() | size(ftxui::HEIGHT, ftxui::EQUAL, 10) | xflex_grow, vbox(m_btn_copy_->Render())))
        });
  }

  std::vector<Topic> dummy;
  if (tab_selected_ == 1) {
    return  //
        vbox({
            header,
            separator(),
            window(text(L"Subscribe"), hbox(text("Enter path:"), separator(), m_subscribe_selector_->Render(), m_btn_subscribe_->Render()) | notflex),
            m_subsribe_error_report->Render(),
            window(text(L"Subscriptions"), container_level_filter_->Render()) | notflex,

            /*hbox({
                window(text(L"Type"), container_level_filter_->Render()) |
                    notflex,
                text(L" "),
                window(text(L"Filter"), hbox(thread_filter_document)) | notflex,
                text(L" ")
                //window(text(L"Selector"), hbox(container_search_selector_->Render(), m_btn_search_->Render())) | flex,
                //filler(),
            }) | notflex,*/
            log_displayer_2_->RenderLines(m_subscribe_topics) | flex_shrink,
            window(text("Content"), hbox(m_subscribe_payload_text_box_->Render() | size(ftxui::HEIGHT, ftxui::EQUAL, 10) | xflex_grow, vbox(m_btn_copy_->Render())))
        });
  }

  return  //
      vbox({
          header,
          separator(),
          vbox(m_btn_dump_exit->Render()| center, m_btn_exit_->Render() | center) | center,
          filler()
      });
}
