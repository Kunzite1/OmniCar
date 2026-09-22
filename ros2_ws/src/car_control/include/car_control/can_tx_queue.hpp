#ifndef CAR_CONTROL_CAN_TX_QUEUE_HPP_
#define CAR_CONTROL_CAN_TX_QUEUE_HPP_

#include <cstddef>
#include <deque>
#include <mutex>

namespace car_control
{

template<typename T>
class BoundedQueue
{
public:
    explicit BoundedQueue(std::size_t capacity)
    : capacity_(capacity)
    {
    }

    bool try_push(const T & item)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if ((capacity_ == 0U) || (queue_.size() >= capacity_))
        {
            return false;
        }
        queue_.push_back(item);
        return true;
    }

    bool try_pop(T & item)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (queue_.empty())
        {
            return false;
        }
        item = queue_.front();
        queue_.pop_front();
        return true;
    }

    std::size_t size() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

private:
    const std::size_t capacity_;
    mutable std::mutex mutex_;
    std::deque<T> queue_;
};

}  // namespace car_control

#endif  // CAR_CONTROL_CAN_TX_QUEUE_HPP_
