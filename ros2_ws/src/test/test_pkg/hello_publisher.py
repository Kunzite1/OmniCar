import rclpy
from rclpy.node import Node
from std_msgs.msg import String


class HelloPublisher(Node):

    def __init__(self):
        super().__init__('hello_publisher')
        self.publisher = self.create_publisher(String, 'hello', 10)
        self.count = 0
        self.timer = self.create_timer(1.0, self.publish_hello)

    def publish_hello(self):
        message = String()
        message.data = f'hello {self.count}'
        self.publisher.publish(message)
        self.get_logger().info(f'Publishing: "{message.data}"')
        self.count += 1


def main(args=None):
    rclpy.init(args=args)
    node = HelloPublisher()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()
