/*++

Module Name:

    public.h

Abstract:

    This module contains the common declarations shared by driver
    and user applications.

Environment:

    user and kernel

--*/

#pragma once

//
// Define an Interface Guid so that apps can find the device and talk to it.
//
DEFINE_GUID (GUID_DEVINTERFACE_zzvirtual,
    0x37af2b6d,0xe77c,0x4233,0x8e,0x39,0x2f,0xea,0xdb,0x6e,0x49,0xdf);
// {37af2b6d-e77c-4233-8e39-2feadb6e49df}


#define REPORT_ID_MOUSE_INPUT       0x01
#define REPORT_ID_MOUSE_OUTPUT      0x02
#define REPORT_ID_KEYBOARD_INPUT    0x03
#define REPORT_ID_KEYBOARD_OUTPUT   0x04

#define VENDOR_ID             0x1234
#define PRODUCT_ID            0x4321
#define VERSION_NUMBER        0x12d3



#pragma pack(1)
typedef struct _HID_MOUSE_INPUT_REPORT {
    BYTE                reportId;
    BYTE                buttons;
    CHAR                x;
    CHAR                y;
} HID_MOUSE_INPUT_REPORT, * PHID_MOUSE_INPUT_REPORT;

typedef struct _HID_MOUSE_OUTPUT_REPORT {
    BYTE                reportId;
    BYTE                buttons;
    CHAR                x;
    CHAR                y;
} HID_MOUSE_OUTPUT_REPORT, * PHID_MOUSE_OUTPUT_REPORT;

typedef struct _HID_KEYBOARD_INPUT_REPORT {
    BYTE                reportId;
    BYTE                modifiers;
    BYTE                _reserved;
    BYTE                keyCodes[6];
} HID_KEYBOARD_INPUT_REPORT, * PHID_KEYBOARD_INPUT_REPORT;

typedef struct _HID_KEYBOARD_OUTPUT_REPORT {
    BYTE                reportId;
    BYTE                modifiers;
    BYTE                _reserved;
    BYTE                keyCodes[6];
} HID_KEYBOARD_OUTPUT_REPORT, * PHID_KEYBOARD_OUTPUT_REPORT;
#pragma pack()