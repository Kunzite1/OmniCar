#include "car_control/can_protocol.h"

#include <stdio.h>
#include <string.h>

#define CHECK(condition)                                                      \
    do                                                                        \
    {                                                                         \
        if (!(condition))                                                     \
        {                                                                     \
            fprintf(stderr, "CHECK failed at %s:%d: %s\n",                  \
                    __FILE__, __LINE__, #condition);                           \
            return 1;                                                         \
        }                                                                     \
    } while (0)

static int test_bitrate_command(void)
{
    uint8_t command[DM_USB_CAN_BITRATE_COMMAND_SIZE] = {0};
    const uint8_t expected[] = {0x55U, 0x05U, 0x03U, 0xAAU, 0x55U};

    dm_usb_can_make_bitrate_command(DM_USB_CAN_BITRATE_500K_INDEX, command);
    CHECK(memcmp(command, expected, sizeof(expected)) == 0);
    return 0;
}

static int test_tx_frame(void)
{
    car_can_frame_t frame = {
        .id = 0x2FFU,
        .dlc = 8U,
        .is_extended = false,
        .is_remote = false,
        .data = {0x42U, 0x4BU, 0x31U, 0x53U, 0x54U, 0x4DU, 0x33U, 0x32U},
    };
    uint8_t output[DM_USB_CAN_TX_FRAME_SIZE];

    CHECK(dm_usb_can_pack_tx_frame(&frame, output));
    CHECK(output[0] == 0x55U && output[1] == 0xAAU);
    CHECK(output[2] == 0x1EU && output[3] == 0x01U);
    CHECK(output[4] == 0x01U && output[5] == 0U && output[6] == 0U && output[7] == 0U);
    CHECK(output[12] == 0U);
    CHECK(output[13] == 0xFFU && output[14] == 0x02U);
    CHECK(output[17] == 0U && output[18] == 8U);
    CHECK(memcmp(&output[21], frame.data, sizeof(frame.data)) == 0);
    CHECK(output[29] == 0U);

    frame.id = 0x800U;
    CHECK(!car_can_frame_is_valid(&frame));
    frame.is_extended = true;
    CHECK(car_can_frame_is_valid(&frame));
    frame.dlc = 9U;
    CHECK(!car_can_frame_is_valid(&frame));
    return 0;
}

static int test_rx_report(void)
{
    const uint8_t input[DM_USB_CAN_REPORT_SIZE] = {
        0xAAU, 0x11U, 0x08U, 0xFEU, 0x02U, 0x00U, 0x00U,
        0x42U, 0x4BU, 0x31U, 0x53U, 0x54U, 0x4DU, 0x33U, 0x32U, 0x55U,
    };
    dm_usb_can_report_t report;
    uint8_t invalid[DM_USB_CAN_REPORT_SIZE];

    CHECK(dm_usb_can_parse_report(input, &report));
    CHECK(report.type == DM_USB_CAN_REPORT_RX_SUCCESS);
    CHECK(report.frame.id == 0x2FEU);
    CHECK(report.frame.dlc == 8U);
    CHECK(!report.frame.is_extended && !report.frame.is_remote);
    CHECK(memcmp(report.frame.data, &input[7], sizeof(report.frame.data)) == 0);

    memcpy(invalid, input, sizeof(invalid));
    invalid[15] = 0U;
    CHECK(!dm_usb_can_parse_report(invalid, &report));
    memcpy(invalid, input, sizeof(invalid));
    invalid[2] = 9U;
    CHECK(!dm_usb_can_parse_report(invalid, &report));
    return 0;
}

int main(void)
{
    CHECK(test_bitrate_command() == 0);
    CHECK(test_tx_frame() == 0);
    CHECK(test_rx_report() == 0);
    puts("can_protocol tests passed");
    return 0;
}
