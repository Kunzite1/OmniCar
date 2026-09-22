#ifndef CAR_CONTROL_CAN_COMMUNICATION_NODE_HPP_
#define CAR_CONTROL_CAN_COMMUNICATION_NODE_HPP_

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "car_control/can_protocol.h"
#include "car_control/can_tx_queue.hpp"
#include "car_control/msg/can_frame.hpp"
#include "rclcpp/rclcpp.hpp"

namespace car_control
{

class CanCommunicationNode : public rclcpp::Node
{
public:
    explicit CanCommunicationNode(
        const rclcpp::NodeOptions & options = rclcpp::NodeOptions());
    ~CanCommunicationNode() override;

    /**
     * @brief Queue a CAN frame for the communication worker.
     *
     * Future in-process business components and the ROS TX subscription use the
     * same bounded queue. Only the communication worker writes the USB device.
     */
    bool enqueue_tx(const car_can_frame_t & frame);

private:
    using CanFrameMessage = car_control::msg::CanFrame;

    void on_tx_message(const CanFrameMessage::SharedPtr message);
    bool open_serial();
    void close_serial();
    bool configure_adapter_bitrate();
    bool write_all(const uint8_t * data, std::size_t length);
    void io_loop();
    void read_available();
    void parse_reports();
    void handle_report(const dm_usb_can_report_t & report);
    void drain_tx_queue();
    void log_status();
    std::string format_frame(const car_can_frame_t & frame) const;

    std::string device_;
    std::string tx_topic_;
    std::string rx_topic_;
    int serial_baud_{921600};
    int can_bitrate_index_{DM_USB_CAN_BITRATE_500K_INDEX};
    int io_poll_timeout_ms_{5};
    int max_tx_per_cycle_{32};
    bool log_frames_{true};
    int serial_fd_{-1};

    std::unique_ptr<BoundedQueue<car_can_frame_t>> tx_queue_;
    std::vector<uint8_t> rx_accumulator_;
    std::atomic<bool> running_{false};
    std::thread io_thread_;

    rclcpp::Publisher<CanFrameMessage>::SharedPtr rx_publisher_;
    rclcpp::Subscription<CanFrameMessage>::SharedPtr tx_subscription_;
    rclcpp::TimerBase::SharedPtr status_timer_;

    std::atomic<uint64_t> tx_enqueued_{0U};
    std::atomic<uint64_t> tx_sent_{0U};
    std::atomic<uint64_t> tx_queue_dropped_{0U};
    std::atomic<uint64_t> tx_invalid_{0U};
    std::atomic<uint64_t> tx_write_errors_{0U};
    std::atomic<uint64_t> rx_frames_{0U};
    std::atomic<uint64_t> rx_frame_errors_{0U};
    std::atomic<uint64_t> adapter_tx_success_{0U};
    std::atomic<uint64_t> adapter_error_reports_{0U};
    std::atomic<uint64_t> heartbeat_frames_{0U};
    std::atomic<uint64_t> echo_frames_{0U};
};

}  // namespace car_control

#endif  // CAR_CONTROL_CAN_COMMUNICATION_NODE_HPP_
