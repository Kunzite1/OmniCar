#include "car_control/can_communication_node.hpp"

#include <algorithm>
#include <array>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <fcntl.h>
#include <functional>
#include <iomanip>
#include <iterator>
#include <poll.h>
#include <sstream>
#include <stdexcept>
#include <termios.h>
#include <unistd.h>

namespace car_control
{

using namespace std::chrono_literals;

CanCommunicationNode::CanCommunicationNode(const rclcpp::NodeOptions & options)
: Node("can_communication_node", options)
{
    device_ = declare_parameter<std::string>("device", "/dev/ttyACM0");
    serial_baud_ = declare_parameter<int>("serial_baud", 921600);
    can_bitrate_index_ = declare_parameter<int>(
        "can_bitrate_index", DM_USB_CAN_BITRATE_500K_INDEX);
    tx_topic_ = declare_parameter<std::string>("tx_topic", "/car_control/can/tx");
    rx_topic_ = declare_parameter<std::string>("rx_topic", "/car_control/can/rx");
    const int queue_capacity = declare_parameter<int>("tx_queue_capacity", 128);
    io_poll_timeout_ms_ = declare_parameter<int>("io_poll_timeout_ms", 5);
    max_tx_per_cycle_ = declare_parameter<int>("max_tx_per_cycle", 32);
    log_frames_ = declare_parameter<bool>("log_frames", true);

    if ((queue_capacity <= 0) || (io_poll_timeout_ms_ < 0) ||
        (max_tx_per_cycle_ <= 0) || (can_bitrate_index_ < 0) ||
        (can_bitrate_index_ > 14))
    {
        throw std::invalid_argument("invalid CAN communication parameter");
    }

    tx_queue_ = std::make_unique<BoundedQueue<car_can_frame_t>>(
        static_cast<std::size_t>(queue_capacity));

    rx_publisher_ = create_publisher<CanFrameMessage>(rx_topic_, rclcpp::QoS(100));
    tx_subscription_ = create_subscription<CanFrameMessage>(
        tx_topic_, rclcpp::QoS(100),
        std::bind(&CanCommunicationNode::on_tx_message, this, std::placeholders::_1));

    if (!open_serial())
    {
        throw std::runtime_error("failed to open USB-CAN serial device");
    }
    if (!configure_adapter_bitrate())
    {
        close_serial();
        throw std::runtime_error("failed to configure USB-CAN bitrate");
    }

    status_timer_ = create_wall_timer(5s, std::bind(&CanCommunicationNode::log_status, this));
    running_.store(true);
    io_thread_ = std::thread(&CanCommunicationNode::io_loop, this);

    RCLCPP_INFO(
        get_logger(),
        "CAN communication ready: device=%s serial=%d bitrate_index=%d tx=%s rx=%s queue=%d",
        device_.c_str(), serial_baud_, can_bitrate_index_, tx_topic_.c_str(),
        rx_topic_.c_str(), queue_capacity);
}

CanCommunicationNode::~CanCommunicationNode()
{
    running_.store(false);
    if (io_thread_.joinable())
    {
        io_thread_.join();
    }
    close_serial();
}

bool CanCommunicationNode::enqueue_tx(const car_can_frame_t & frame)
{
    if (!car_can_frame_is_valid(&frame))
    {
        tx_invalid_.fetch_add(1U);
        return false;
    }
    if (!tx_queue_->try_push(frame))
    {
        tx_queue_dropped_.fetch_add(1U);
        return false;
    }
    tx_enqueued_.fetch_add(1U);
    return true;
}

void CanCommunicationNode::on_tx_message(const CanFrameMessage::SharedPtr message)
{
    car_can_frame_t frame{};

    frame.id = message->id;
    frame.dlc = message->dlc;
    frame.is_extended = message->is_extended;
    frame.is_remote = message->is_remote;
    std::copy(message->data.begin(), message->data.end(), frame.data);

    if (!enqueue_tx(frame))
    {
        RCLCPP_WARN_THROTTLE(
            get_logger(), *get_clock(), 2000,
            "Rejected CAN TX request: id=0x%X dlc=%u (invalid frame or full queue)",
            frame.id, static_cast<unsigned int>(frame.dlc));
    }
}

bool CanCommunicationNode::open_serial()
{
    speed_t speed;
    termios tty{};

    switch (serial_baud_)
    {
        case 115200: speed = B115200; break;
        case 460800: speed = B460800; break;
        case 921600: speed = B921600; break;
        default:
            RCLCPP_ERROR(get_logger(), "Unsupported serial baud: %d", serial_baud_);
            return false;
    }

    serial_fd_ = ::open(device_.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK | O_CLOEXEC);
    if (serial_fd_ < 0)
    {
        RCLCPP_ERROR(
            get_logger(), "open(%s) failed: %s", device_.c_str(), std::strerror(errno));
        return false;
    }

    if (tcgetattr(serial_fd_, &tty) != 0)
    {
        RCLCPP_ERROR(get_logger(), "tcgetattr failed: %s", std::strerror(errno));
        close_serial();
        return false;
    }

    cfmakeraw(&tty);
    tty.c_cflag &= static_cast<tcflag_t>(~(PARENB | CSTOPB | CSIZE | CRTSCTS));
    tty.c_cflag |= static_cast<tcflag_t>(CS8 | CLOCAL | CREAD);
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 0;
    cfsetispeed(&tty, speed);
    cfsetospeed(&tty, speed);

    if (tcsetattr(serial_fd_, TCSANOW, &tty) != 0)
    {
        RCLCPP_ERROR(get_logger(), "tcsetattr failed: %s", std::strerror(errno));
        close_serial();
        return false;
    }
    tcflush(serial_fd_, TCIOFLUSH);
    return true;
}

void CanCommunicationNode::close_serial()
{
    if (serial_fd_ >= 0)
    {
        ::close(serial_fd_);
        serial_fd_ = -1;
    }
}

bool CanCommunicationNode::configure_adapter_bitrate()
{
    std::array<uint8_t, DM_USB_CAN_BITRATE_COMMAND_SIZE> command{};
    dm_usb_can_make_bitrate_command(
        static_cast<uint8_t>(can_bitrate_index_), command.data());

    if (!write_all(command.data(), command.size()))
    {
        return false;
    }
    if (tcdrain(serial_fd_) != 0)
    {
        RCLCPP_ERROR(get_logger(), "tcdrain failed: %s", std::strerror(errno));
        return false;
    }
    std::this_thread::sleep_for(50ms);
    RCLCPP_INFO(
        get_logger(), "USB-CAN bitrate command sent: 55 05 %02X AA 55",
        static_cast<unsigned int>(can_bitrate_index_));
    return true;
}

bool CanCommunicationNode::write_all(const uint8_t * data, std::size_t length)
{
    std::size_t offset = 0U;
    const auto deadline = std::chrono::steady_clock::now() + 250ms;

    while (offset < length)
    {
        const ssize_t written = ::write(serial_fd_, data + offset, length - offset);
        if (written > 0)
        {
            offset += static_cast<std::size_t>(written);
            continue;
        }
        if ((written < 0) && (errno == EINTR))
        {
            continue;
        }
        if ((written < 0) && (errno != EAGAIN) && (errno != EWOULDBLOCK))
        {
            RCLCPP_ERROR(get_logger(), "serial write failed: %s", std::strerror(errno));
            return false;
        }

        const auto now = std::chrono::steady_clock::now();
        if (now >= deadline)
        {
            RCLCPP_ERROR(get_logger(), "serial write timed out");
            return false;
        }
        const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
            deadline - now);
        pollfd descriptor{serial_fd_, POLLOUT, 0};
        const int result = ::poll(&descriptor, 1, static_cast<int>(remaining.count()));
        if (result <= 0)
        {
            RCLCPP_ERROR(
                get_logger(), "serial write poll failed: %s",
                (result == 0) ? "timeout" : std::strerror(errno));
            return false;
        }
    }
    return true;
}

void CanCommunicationNode::io_loop()
{
    while (running_.load())
    {
        pollfd descriptor{serial_fd_, POLLIN, 0};
        const int result = ::poll(&descriptor, 1, io_poll_timeout_ms_);

        if (result > 0)
        {
            if ((descriptor.revents & POLLIN) != 0)
            {
                read_available();
            }
            if ((descriptor.revents & (POLLERR | POLLHUP | POLLNVAL)) != 0)
            {
                RCLCPP_ERROR_THROTTLE(
                    get_logger(), *get_clock(), 2000,
                    "USB-CAN device poll error: revents=0x%X",
                    static_cast<unsigned int>(descriptor.revents));
            }
        }
        else if ((result < 0) && (errno != EINTR))
        {
            RCLCPP_ERROR_THROTTLE(
                get_logger(), *get_clock(), 2000,
                "USB-CAN poll failed: %s", std::strerror(errno));
        }

        parse_reports();
        drain_tx_queue();
    }
}

void CanCommunicationNode::read_available()
{
    std::array<uint8_t, 256> buffer{};

    for (;;)
    {
        const ssize_t count = ::read(serial_fd_, buffer.data(), buffer.size());
        if (count > 0)
        {
            rx_accumulator_.insert(
                rx_accumulator_.end(), buffer.begin(),
                buffer.begin() + static_cast<std::ptrdiff_t>(count));
            if (rx_accumulator_.size() > 8192U)
            {
                rx_accumulator_.erase(
                    rx_accumulator_.begin(), rx_accumulator_.end() - DM_USB_CAN_REPORT_SIZE);
                rx_frame_errors_.fetch_add(1U);
            }
            continue;
        }
        if ((count < 0) && (errno != EAGAIN) && (errno != EWOULDBLOCK) &&
            (errno != EINTR))
        {
            RCLCPP_ERROR_THROTTLE(
                get_logger(), *get_clock(), 2000,
                "USB-CAN read failed: %s", std::strerror(errno));
            rx_frame_errors_.fetch_add(1U);
        }
        break;
    }
}

void CanCommunicationNode::parse_reports()
{
    while (rx_accumulator_.size() >= DM_USB_CAN_REPORT_SIZE)
    {
        const auto header = std::find(rx_accumulator_.begin(), rx_accumulator_.end(), 0xAAU);
        if (header == rx_accumulator_.end())
        {
            rx_frame_errors_.fetch_add(rx_accumulator_.size());
            rx_accumulator_.clear();
            return;
        }
        if (header != rx_accumulator_.begin())
        {
            const auto discarded = static_cast<uint64_t>(
                std::distance(rx_accumulator_.begin(), header));
            rx_frame_errors_.fetch_add(discarded);
            rx_accumulator_.erase(rx_accumulator_.begin(), header);
        }
        if (rx_accumulator_.size() < DM_USB_CAN_REPORT_SIZE)
        {
            return;
        }
        if (rx_accumulator_[DM_USB_CAN_REPORT_SIZE - 1U] != 0x55U)
        {
            rx_accumulator_.erase(rx_accumulator_.begin());
            rx_frame_errors_.fetch_add(1U);
            continue;
        }

        std::array<uint8_t, DM_USB_CAN_REPORT_SIZE> raw{};
        std::copy_n(rx_accumulator_.begin(), raw.size(), raw.begin());
        rx_accumulator_.erase(
            rx_accumulator_.begin(), rx_accumulator_.begin() + raw.size());

        dm_usb_can_report_t report{};
        if (!dm_usb_can_parse_report(raw.data(), &report))
        {
            rx_frame_errors_.fetch_add(1U);
            continue;
        }
        handle_report(report);
    }
}

void CanCommunicationNode::handle_report(const dm_usb_can_report_t & report)
{
    switch (report.type)
    {
        case DM_USB_CAN_REPORT_RX_SUCCESS:
        {
            CanFrameMessage message;
            message.id = report.frame.id;
            message.dlc = report.frame.dlc;
            message.is_extended = report.frame.is_extended;
            message.is_remote = report.frame.is_remote;
            std::copy(std::begin(report.frame.data), std::end(report.frame.data), message.data.begin());
            rx_publisher_->publish(message);
            rx_frames_.fetch_add(1U);

            if (report.frame.id == 0x101U)
            {
                heartbeat_frames_.fetch_add(1U);
                RCLCPP_INFO(get_logger(), "STM32 heartbeat %s", format_frame(report.frame).c_str());
            }
            else if (report.frame.id == 0x2FEU)
            {
                echo_frames_.fetch_add(1U);
                RCLCPP_INFO(get_logger(), "STM32 echo response %s", format_frame(report.frame).c_str());
            }
            else if (log_frames_)
            {
                RCLCPP_INFO(get_logger(), "CAN RX %s", format_frame(report.frame).c_str());
            }
            break;
        }
        case DM_USB_CAN_REPORT_TX_SUCCESS:
            adapter_tx_success_.fetch_add(1U);
            RCLCPP_DEBUG(get_logger(), "USB-CAN accepted TX %s", format_frame(report.frame).c_str());
            break;
        case DM_USB_CAN_REPORT_RX_FAILED:
            adapter_error_reports_.fetch_add(1U);
            RCLCPP_WARN(get_logger(), "USB-CAN reported CAN receive failure");
            break;
        case DM_USB_CAN_REPORT_TX_FAILED:
            adapter_error_reports_.fetch_add(1U);
            RCLCPP_WARN(get_logger(), "USB-CAN reported CAN transmit failure");
            break;
        case DM_USB_CAN_REPORT_HEARTBEAT:
            RCLCPP_DEBUG(get_logger(), "USB-CAN adapter heartbeat");
            break;
        default:
            rx_frame_errors_.fetch_add(1U);
            RCLCPP_WARN(get_logger(), "Unknown USB-CAN report type");
            break;
    }
}

void CanCommunicationNode::drain_tx_queue()
{
    for (int index = 0; index < max_tx_per_cycle_; ++index)
    {
        car_can_frame_t frame{};
        if (!tx_queue_->try_pop(frame))
        {
            return;
        }

        std::array<uint8_t, DM_USB_CAN_TX_FRAME_SIZE> raw{};
        if (!dm_usb_can_pack_tx_frame(&frame, raw.data()) ||
            !write_all(raw.data(), raw.size()))
        {
            tx_write_errors_.fetch_add(1U);
            RCLCPP_ERROR(get_logger(), "CAN TX failed %s", format_frame(frame).c_str());
            continue;
        }

        tx_sent_.fetch_add(1U);
        if (log_frames_)
        {
            RCLCPP_INFO(get_logger(), "CAN TX %s", format_frame(frame).c_str());
        }
    }
}

void CanCommunicationNode::log_status()
{
    RCLCPP_INFO(
        get_logger(),
        "CAN status queue=%zu enqueued=%llu sent=%llu queue_drop=%llu invalid=%llu "
        "write_err=%llu rx=%llu parse_err=%llu adapter_tx_ok=%llu adapter_err=%llu "
        "heartbeat=%llu echo=%llu",
        tx_queue_->size(),
        static_cast<unsigned long long>(tx_enqueued_.load()),
        static_cast<unsigned long long>(tx_sent_.load()),
        static_cast<unsigned long long>(tx_queue_dropped_.load()),
        static_cast<unsigned long long>(tx_invalid_.load()),
        static_cast<unsigned long long>(tx_write_errors_.load()),
        static_cast<unsigned long long>(rx_frames_.load()),
        static_cast<unsigned long long>(rx_frame_errors_.load()),
        static_cast<unsigned long long>(adapter_tx_success_.load()),
        static_cast<unsigned long long>(adapter_error_reports_.load()),
        static_cast<unsigned long long>(heartbeat_frames_.load()),
        static_cast<unsigned long long>(echo_frames_.load()));
}

std::string CanCommunicationNode::format_frame(const car_can_frame_t & frame) const
{
    std::ostringstream stream;
    stream << "id=0x" << std::uppercase << std::hex << std::setfill('0')
           << std::setw(frame.is_extended ? 8 : 3) << frame.id
           << " dlc=" << std::dec << static_cast<unsigned int>(frame.dlc)
           << " data=";
    for (uint8_t index = 0U; index < frame.dlc; ++index)
    {
        if (index != 0U)
        {
            stream << ' ';
        }
        stream << std::uppercase << std::hex << std::setw(2)
               << static_cast<unsigned int>(frame.data[index]);
    }
    return stream.str();
}

}  // namespace car_control
