#include <exception>
#include <memory>

#include "car_control/can_communication_node.hpp"
#include "rclcpp/rclcpp.hpp"

int main(int argc, char * argv[])
{
    rclcpp::init(argc, argv);
    try
    {
        rclcpp::spin(std::make_shared<car_control::CanCommunicationNode>());
    }
    catch (const std::exception & exception)
    {
        RCLCPP_FATAL(rclcpp::get_logger("can_communication_node"), "%s", exception.what());
        rclcpp::shutdown();
        return 1;
    }
    rclcpp::shutdown();
    return 0;
}
