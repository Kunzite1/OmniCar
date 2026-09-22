#include "car_control/can_tx_queue.hpp"

#include <iostream>

#define CHECK(condition)                                                      \
    do                                                                        \
    {                                                                         \
        if (!(condition))                                                     \
        {                                                                     \
            std::cerr << "CHECK failed at " << __FILE__ << ':' << __LINE__   \
                      << ": " << #condition << '\n';                         \
            return 1;                                                         \
        }                                                                     \
    } while (0)

int main()
{
    car_control::BoundedQueue<int> queue(2U);
    int value = 0;

    CHECK(queue.size() == 0U);
    CHECK(queue.try_push(10));
    CHECK(queue.try_push(20));
    CHECK(!queue.try_push(30));
    CHECK(queue.size() == 2U);
    CHECK(queue.try_pop(value) && value == 10);
    CHECK(queue.try_pop(value) && value == 20);
    CHECK(!queue.try_pop(value));
    CHECK(queue.size() == 0U);

    std::cout << "can_tx_queue tests passed\n";
    return 0;
}
