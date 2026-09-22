#ifndef CAR_CONTROL_CAN_PROTOCOL_H_
#define CAR_CONTROL_CAN_PROTOCOL_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CAR_CAN_MAX_DATA_LENGTH          8U
#define DM_USB_CAN_TX_FRAME_SIZE         30U
#define DM_USB_CAN_REPORT_SIZE           16U
#define DM_USB_CAN_BITRATE_COMMAND_SIZE  5U
#define DM_USB_CAN_BITRATE_500K_INDEX    3U

typedef struct {
    uint32_t id;
    uint8_t dlc;
    bool is_extended;
    bool is_remote;
    uint8_t data[CAR_CAN_MAX_DATA_LENGTH];
} car_can_frame_t;

typedef enum {
    DM_USB_CAN_REPORT_HEARTBEAT = 0x00,
    DM_USB_CAN_REPORT_RX_FAILED = 0x01,
    DM_USB_CAN_REPORT_TX_FAILED = 0x02,
    DM_USB_CAN_REPORT_RX_SUCCESS = 0x11,
    DM_USB_CAN_REPORT_TX_SUCCESS = 0x12,
    DM_USB_CAN_REPORT_UNKNOWN = 0xFF,
} dm_usb_can_report_type_t;

typedef struct {
    dm_usb_can_report_type_t type;
    car_can_frame_t frame;
} dm_usb_can_report_t;

bool car_can_frame_is_valid(const car_can_frame_t *frame);

void dm_usb_can_make_bitrate_command(
    uint8_t bitrate_index,
    uint8_t output[DM_USB_CAN_BITRATE_COMMAND_SIZE]);

bool dm_usb_can_pack_tx_frame(
    const car_can_frame_t *frame,
    uint8_t output[DM_USB_CAN_TX_FRAME_SIZE]);

bool dm_usb_can_parse_report(
    const uint8_t input[DM_USB_CAN_REPORT_SIZE],
    dm_usb_can_report_t *report);

#ifdef __cplusplus
}
#endif

#endif  /* CAR_CONTROL_CAN_PROTOCOL_H_ */
