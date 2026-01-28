#pragma once
#include "Message.h"
#include <queue>
#include <vector>
#include <functional>
#include <unordered_map>
#include <memory>
#include <mutex>

using MessageCallback = std::function<void(const Message&)>;

class MessageQueue
{
public:
    static MessageQueue& Instance()
    {
        static MessageQueue instance;
        return instance;
    }

    // Post a message to the queue
    template<typename T>
    void Post(std::shared_ptr<T> message)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push(std::static_pointer_cast<Message>(message));
    }

    // Subscribe to a specific message type
    void Subscribe(MessageType type, MessageCallback callback)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_subscribers[type].push_back(callback);
    }

    // Process all messages in the queue (call once per frame)
    void ProcessMessages()
    {
        std::queue<std::shared_ptr<Message>> localQueue;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            std::swap(localQueue, m_queue);
        }

        while (!localQueue.empty())
        {
            auto msg = localQueue.front();
            localQueue.pop();

            // Dispatch to subscribers
            auto it = m_subscribers.find(msg->GetType());
            if (it != m_subscribers.end())
            {
                for (auto& callback : it->second)
                {
                    callback(*msg);
                }
            }
        }
    }

    // Clear all messages
    void Clear()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        while (!m_queue.empty()) m_queue.pop();
    }

    // Get queue size
    size_t Size() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.size();
    }

private:
    MessageQueue() = default;
    ~MessageQueue() = default;
    MessageQueue(const MessageQueue&) = delete;
    MessageQueue& operator=(const MessageQueue&) = delete;

    std::queue<std::shared_ptr<Message>> m_queue;
    std::unordered_map<MessageType, std::vector<MessageCallback>> m_subscribers;
    mutable std::mutex m_mutex;
};
