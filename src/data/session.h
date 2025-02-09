//
// Created by apavlov on 25.01.25.
//

#ifndef DMON_SESSION_H
#define DMON_SESSION_H

#include <string>
#include <functional>
#include <mutex>
#include <condition_variable>

#include "diffusion.h"

std::string error2Str(ERROR_CODE_T ec);

enum SubscriptionReason : int {
  REASON_SUBSCRIBE = 100500,
  REASON_REQUESTED = 0,
  /// The unsubscription was requested either by another client
  /// or by the server.
  REASON_CONTROL = 1,
  /// The unsubscription occurred because the topic was removed.
  REASON_REMOVAL = 2,
  /// The unsubscription occurred because the session is no longer
  /// authorized to access the topic.
  REASON_AUTHORIZATION = 3,
  /**
       * The server has re-subscribed this session to the topic. Existing
       * streams are unsubscribed because the topic type and other
       * attributes may have changed.
       *
       * <p>
       * This can happen if a set of servers is configured to use session
       * replication, and a session connected to one server reconnects
       * ("fails over") to a different server.
       *
       * <p>
       * A stream that receives an unsubscription notification with this
       * reason will also receive a subscription notification with the new
       * {@link TOPIC_DETAILS_T topic details}.
   */
  REASON_SUBSCRIPTION_REFRESH = 4,
  /**
       * A fallback stream has been unsubscribed due to the addition of a
       * stream that selects the topic. (Not currently used in the C API).
   */
  REASON_STREAM_CHANGE = 5
};

SubscriptionReason transformReason(NOTIFY_UNSUBSCRIPTION_REASON_T r);

struct Error {
  ERROR_CODE_T m_code{DIFF_ERR_SUCCESS};
  std::string m_message;
};

struct Topic {
  std::string m_topic_type;
  std::string m_path;
  std::vector<char> m_buffer;
  Topic(const std::string& type, const std::string& path, const char* ptr, size_t len);
  Topic(const Topic&) = default;
  Topic() =default;
  Topic(Topic&&) = default;
};

struct SubscriptionNotification {
  unsigned int m_id;
  std::string m_path;
  SubscriptionReason m_reason;
};

class Session {
public:
  using FetchCompleted = std::function<void(std::string&&)>;
  using ErrorCallback = std::function<void(Error)>;
  using FetchStart = std::function<void()>;
  using TopicSubscriptionEvent = std::function<void()>;
  using SubscribeCompleted = std::function<void(std::string&&)>;

  Session();

  bool IsValid() const {
    return m_session != nullptr;
  }

  bool connect(const std::string& url, const std::string& principal, const std::string& password, Error&);
  bool fetch(const std::string& selector);
  void onFetchTopic(Topic&&);
  void onFetchError(Error error);
  void onFetchDiscard();
  void onSubscribeError(Error error);
  void onFetchCompleted(void*);
  void setFetchCompletedCallback(FetchCompleted&&);
  void setFetchErrorCallback(ErrorCallback&&);
  void setFetchDiscardCallback(FetchCompleted&&);
  void setFetchStartCallback(FetchStart&&);

  void setSubscribeErrorCallback(ErrorCallback&&);


  std::string getAddress() const {
    return m_principal + "@" + m_url;
  }

  const Error getLastFetchStatus() {
    std::lock_guard<std::mutex> lk(m_operationMutex);
    return m_fetchStatus;
  }

  bool isFetchInProgress() {
    std::lock_guard<std::mutex> lk(m_operationMutex);
    return m_fetch_in_progress;
  }

  bool subscribe(const std::string& selector);
  bool unsubscribe(const std::string& selector);
  bool notify();


  std::vector<Topic> getSubscribeTopics() {
    std::lock_guard<std::mutex> lk(m_operationMutex);
    return std::move(m_subscrube_topics);
  }

  std::vector<Topic> getFetchTopics() {
    std::lock_guard<std::mutex> lk(m_operationMutex);
    return std::move(m_topics);
  }

  void setSubscribeStartCallback(std::function<void()>&& ssc) {
    m_subscribe_start_callback = std::move(ssc);
  }

  void onSubscribeTopic(Topic&&);
  void onSubscribeCompleted();
  void setSubscribeCompletedCallback(SubscribeCompleted&&);

  void setOnTopicSubscriptionEvent(TopicSubscriptionEvent&&);
  void onTopicSubscriptionEvent(SubscriptionNotification&&);

  bool isSubscribtionInProgress() {
    std::lock_guard<std::mutex> lk(m_operationMutex);
    return m_subscribe_in_progress;
  }

  ~Session();
  Session(const Session&) = delete;
  Session& operator=(const Session) = delete;
 private:
  std::string m_url;
  std::string m_principal;
  std::string m_password;
  SESSION_T* m_session;
  CREDENTIALS_T *m_credentials;
  std::vector<Topic> m_topics;
  std::vector<Topic> m_subscrube_topics;
  FetchCompleted m_fetch_completed_callback;
  ErrorCallback m_fetch_error_callback;
  ErrorCallback m_subscribe_error_callback;
  FetchStart m_fetch_start_callback;
  FetchCompleted m_fetch_discard_callback;
  std::mutex m_operationMutex;
  bool m_fetch_in_progress;
  bool m_subscribe_in_progress;
  Error m_fetchStatus;
  Error m_subscribeStatus;
  std::string m_selector;
  TopicSubscriptionEvent m_topic_subscription_event;
  std::vector<SubscriptionNotification> m_topic_subscription_events;
  SubscribeCompleted m_subscribe_completed_callback;
  std::function<void()> m_subscribe_start_callback;
};

#endif //DMON_SESSION_H
