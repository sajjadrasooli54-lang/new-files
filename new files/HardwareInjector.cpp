/*
 * HardwareInjector.cpp
 * Implementation of hardware injector using HID Feature Reports.
 */

#include "HardwareInjector.h"
#include <iostream>
#include <vector>
#include <memory>

HardwareInjector::HardwareInjector()
    : m_handle(INVALID_HANDLE_VALUE)
{
}

HardwareInjector::~HardwareInjector()
{
    if (m_handle != INVALID_HANDLE_VALUE) {
        CloseHandle(m_handle);
        m_handle = INVALID_HANDLE_VALUE;
    }
}

bool HardwareInjector::Connect()
{
    if (m_handle != INVALID_HANDLE_VALUE)
        return true;

    GUID hidGuid;
    HidD_GetHidGuid(&hidGuid);

    HDEVINFO devInfo = SetupDiGetClassDevs(&hidGuid, nullptr, nullptr,
                                            DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (devInfo == INVALID_HANDLE_VALUE)
        return false;

    SP_DEVICE_INTERFACE_DATA ifData = { sizeof(SP_DEVICE_INTERFACE_DATA) };
    DWORD idx = 0;
    bool found = false;

    while (SetupDiEnumDeviceInterfaces(devInfo, nullptr, &hidGuid, idx, &ifData)) {
        DWORD needed = 0;
        SetupDiGetDeviceInterfaceDetail(devInfo, &ifData, nullptr, 0, &needed, nullptr);
        if (needed == 0) { idx++; continue; }

        std::vector<BYTE> buf(needed);
        PSP_DEVICE_INTERFACE_DETAIL_DATA det = reinterpret_cast<PSP_DEVICE_INTERFACE_DETAIL_DATA>(buf.data());
        det->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
        if (!SetupDiGetDeviceInterfaceDetail(devInfo, &ifData, det, needed, nullptr, nullptr)) {
            idx++; continue;
        }

        HANDLE h = CreateFileA(det->DevicePath,
                               GENERIC_READ | GENERIC_WRITE,
                               FILE_SHARE_READ | FILE_SHARE_WRITE,
                               nullptr, OPEN_EXISTING, 0, nullptr);
        if (h == INVALID_HANDLE_VALUE) {
            idx++; continue;
        }

        HIDD_ATTRIBUTES attrs = { sizeof(HIDD_ATTRIBUTES) };
        if (!HidD_GetAttributes(h, &attrs) ||
            attrs.VendorID != TARGET_VID ||
            attrs.ProductID != TARGET_PID) {
            CloseHandle(h);
            idx++; continue;
        }

        // Check capabilities: need OutputReportByteLength >= REPORT_SIZE
        PHIDP_PREPARSED_DATA ppd = nullptr;
        if (!HidD_GetPreparsedData(h, &ppd)) {
            CloseHandle(h);
            idx++; continue;
        }
        HIDP_CAPS caps;
        HidP_GetCaps(ppd, &caps);
        HidD_FreePreparsedData(ppd);

        // We need the vendor interface (UsagePage 0xFF00) with output report length >= 7
        if (caps.UsagePage == 0xFF00 && caps.OutputReportByteLength >= REPORT_SIZE) {
            // Test with a SET_REPORT (probe) – send a dummy report
            BYTE probe[REPORT_SIZE] = { REPORT_ID, 0x00 };
            if (HidD_SetFeature(h, probe, caps.OutputReportByteLength)) {
                m_handle = h;
                found = true;
                break;
            }
        }
        CloseHandle(h);
        idx++;
    }

    SetupDiDestroyDeviceInfoList(devInfo);
    return found;
}

bool HardwareInjector::SendReport(const BYTE* report, size_t len)
{
    if (m_handle == INVALID_HANDLE_VALUE || len > REPORT_SIZE)
        return false;

    // Use HidD_SetFeature (works with Feature reports). For Output reports we might need WriteFile, but HidD_SetFeature is simpler.
    // We'll use HidD_SetFeature; the firmware expects Feature reports.
    return HidD_SetFeature(m_handle, (PVOID)report, (ULONG)REPORT_SIZE) != FALSE;
}

void HardwareInjector::SendCommand(BYTE cmd, const BYTE* data, size_t dataLen)
{
    if (m_handle == INVALID_HANDLE_VALUE || dataLen > 5)
        return;

    BYTE report[REPORT_SIZE] = { REPORT_ID, cmd };
    memcpy(report + 2, data, dataLen);
    // Pad remaining with zeros
    for (size_t i = 2 + dataLen; i < REPORT_SIZE; i++)
        report[i] = 0;

    SendReport(report, REPORT_SIZE);
}

void HardwareInjector::SendMovement(int16_t dx, int16_t dy)
{
    BYTE data[4];
    data[0] = (BYTE)(dx & 0xFF);
    data[1] = (BYTE)((dx >> 8) & 0xFF);
    data[2] = (BYTE)(dy & 0xFF);
    data[3] = (BYTE)((dy >> 8) & 0xFF);
    SendCommand(0x01, data, 4);
}

void HardwareInjector::SendCooldown(uint16_t ms)
{
    BYTE data[2];
    data[0] = (BYTE)(ms & 0xFF);
    data[1] = (BYTE)((ms >> 8) & 0xFF);
    SendCommand(0x02, data, 2);
}

void HardwareInjector::Move(int dx, int dy)
{
    SendMovement((int16_t)dx, (int16_t)dy);
}

void HardwareInjector::Click()
{
    // Not used: firmware handles click as part of SilentAim/Flick
    // We can implement a simple click by sending zero movement and a "click" command if we add it, but not needed.
}

void HardwareInjector::SilentAim(int dx, int dy)
{
    SendMovement((int16_t)dx, (int16_t)dy);
    // The firmware will apply cooldown automatically; we just send the movement.
    // The firmware expects the PC to manage cooldown? Actually we set cooldown via SetCooldown.
    // So we just send movement; firmware will enforce cooldown between successive injections.
}

void HardwareInjector::Flick(int dx, int dy)
{
    SendMovement((int16_t)dx, (int16_t)dy);
}

void HardwareInjector::SetCooldown(int ms)
{
    SendCooldown((uint16_t)ms);
}