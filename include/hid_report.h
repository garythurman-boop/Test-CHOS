#ifndef CHOS_HID_REPORT_H
#define CHOS_HID_REPORT_H
#include "input.h"
#define HID_FIELDS 96
#define HID_REPORTS 16
struct hid_field {uint16_t bit,page,usage,count;uint8_t size,id,flags;int32_t minimum,maximum;};
struct hid_layout {struct hid_field field[HID_FIELDS];uint16_t bits[HID_REPORTS];uint8_t ids[HID_REPORTS],reports,count;bool numbered,keyboard,pointer,absolute;};
struct hid_report_state {uint8_t keys[HID_REPORTS][32],buttons[HID_REPORTS];};
/* 1201 malformed items, 1202 limits, 1203 unsupported state/items,
 * 1204 no keyboard/pointer inputs, 1205 invalid logical range/field. */
unsigned hid_compile(const uint8_t *,unsigned,struct hid_layout *);
bool hid_decode(const struct hid_layout *,struct hid_report_state *,const uint8_t *,unsigned,struct input_event *,unsigned *);
bool hid_release(struct hid_report_state *,struct input_event *,unsigned *);
uint16_t usb_usage_code(unsigned);
#endif
