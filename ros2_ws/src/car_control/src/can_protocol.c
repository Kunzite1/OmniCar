#include "car_control/can_protocol.h"

#include <string.h>

static void write_u32_le(uint8_t *output, uint32_t value)
{
    output[0] = (uint8_t)(value & 0xFFU);
    output[1] = (uint8_t)((value >> 8U) & 0xFFU);
    output[2] = (uint8_t)((value >> 16U) & 0xFFU);
    output[3] = (uint8_t)((value >> 24U) & 0xFFU);
}

static uint32_t read_u32_le(const uint8_t *input)
{
    return ((uint32_t)input[0]) |
           ((uint32_t)input[1] << 8U) |
           ((uint32_t)input[2] << 16U) |
           ((uint32_t)input[3] << 24U);
}

bool car_can_frame_is_valid(const car_can_frame_t *frame)
{
    uint32_t maximum_id;

    if ((frame == NULL) || (frame->dlc > CAR_CAN_MAX_DATA_LENGTH))
    {
        return false;
    }

    maximum_id = frame->is_extended ? 0x1FFFFFFFU : 0x7FFU;
    return frame->id <= maximum_id;
}

void dm_usb_can_make_bitrate_command(
    uint8_t bitrate_index,
    uint8_t output[DM_USB_CAN_BITRATE_COMMAND_SIZE])
{
    if (output == NULL)
    {
        return;
    }

    output[0] = 0x55U;
    output[1] = 0x05U;
    output[2] = bitrate_index;
    output[3] = 0xAAU;
    output[4] = 0x55U;
}

bool dm_usb_can_pack_tx_frame(
    const car_can_frame_t *frame,
    uint8_t output[DM_USB_CAN_TX_FRAME_SIZE])
{
    if (!car_can_frame_is_valid(frame) || (output == NULL))
    {
        return false;
    }

    memset(output, 0, DM_USB_CAN_TX_FRAME_SIZE);
    output[0] = 0x55U;
    output[1] = 0xAAU;
    output[2] = 0x1EU;
    output[3] = 0x01U;
    write_u32_le(&output[4], 1U);
    write_u32_le(&output[8], 0U);
    output[12] = frame->is_extended ? 1U : 0U;
    write_u32_le(&output[13], frame->id);
    output[17] = frame->is_remote ? 1U : 0U;
    output[18] = frame->dlc;
    output[19] = 0U;
    output[20] = 0U;
    if (!frame->is_remote && (frame->dlc > 0U))
    {
        memcpy(&output[21], frame->data, frame->dlc);
    }
    output[29] = 0U;
    return true;
}

bool dm_usb_can_parse_report(
    const uint8_t input[DM_USB_CAN_REPORT_SIZE],
    dm_usb_can_report_t *report)
{
    uint8_t format;

    if ((input == NULL) || (report == NULL) ||
        (input[0] != 0xAAU) || (input[15] != 0x55U))
    {
        return false;
    }

    memset(report, 0, sizeof(*report));
    switch (input[1])
    {
        case 0x00U: report->type = DM_USB_CAN_REPORT_HEARTBEAT; break;
        case 0x01U: report->type = DM_USB_CAN_REPORT_RX_FAILED; break;
        case 0x02U: report->type = DM_USB_CAN_REPORT_TX_FAILED; break;
        case 0x11U: report->type = DM_USB_CAN_REPORT_RX_SUCCESS; break;
        case 0x12U: report->type = DM_USB_CAN_REPORT_TX_SUCCESS; break;
        default:    report->type = DM_USB_CAN_REPORT_UNKNOWN; break;
    }

    format = input[2];
    report->frame.dlc = (uint8_t)(format & 0x3FU);
    report->frame.is_extended = (format & 0x40U) != 0U;
    report->frame.is_remote = (format & 0x80U) != 0U;
    report->frame.id = read_u32_le(&input[3]);
    memcpy(report->frame.data, &input[7], CAR_CAN_MAX_DATA_LENGTH);

    if ((report->type == DM_USB_CAN_REPORT_RX_SUCCESS) ||
        (report->type == DM_USB_CAN_REPORT_TX_SUCCESS) ||
        (report->type == DM_USB_CAN_REPORT_RX_FAILED) ||
        (report->type == DM_USB_CAN_REPORT_TX_FAILED))
    {
        return car_can_frame_is_valid(&report->frame);
    }
    return true;
}
