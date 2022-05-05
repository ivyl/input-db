/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright (c) 2022 Arkadiusz Hiler for CodeWeavers
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#define UNICODE
#include <initguid.h>
#include <windows.h>
#include <commctrl.h>
#include <dinput.h>
#include <stdio.h>
#include <assert.h>
#include <sstream>

extern "C"
{
#include <hidsdi.h>
}

static IDirectInput8W *di8;
static DIJOYSTATE joystate;

static HWND combo_box;
static HWND text_area;

static DIDEVICEINSTANCEW devices[256];
static int device_count;

#define CAT(A, B)   A##B
#define WSTRING(A)  CAT(L, #A)

#define ReturnGUID2WSTRING(value, guid) \
	if (IsEqualGUID(value, guid)) \
		return WSTRING(guid)

const WCHAR *guid_to_string(GUID guid)
{
	static WCHAR str[100];

	ReturnGUID2WSTRING(guid, GUID_Button);
	ReturnGUID2WSTRING(guid, GUID_XAxis);
	ReturnGUID2WSTRING(guid, GUID_YAxis);
	ReturnGUID2WSTRING(guid, GUID_ZAxis);
	ReturnGUID2WSTRING(guid, GUID_RxAxis);
	ReturnGUID2WSTRING(guid, GUID_RyAxis);
	ReturnGUID2WSTRING(guid, GUID_RzAxis);
	ReturnGUID2WSTRING(guid, GUID_Slider);
	ReturnGUID2WSTRING(guid, GUID_Key);
	ReturnGUID2WSTRING(guid, GUID_Unknown);
	ReturnGUID2WSTRING(guid, GUID_POV);
	ReturnGUID2WSTRING(guid, GUID_ConstantForce);
	ReturnGUID2WSTRING(guid, GUID_RampForce);
	ReturnGUID2WSTRING(guid, GUID_Square);
	ReturnGUID2WSTRING(guid, GUID_Sine);
	ReturnGUID2WSTRING(guid, GUID_Triangle);
	ReturnGUID2WSTRING(guid, GUID_SawtoothUp);
	ReturnGUID2WSTRING(guid, GUID_SawtoothDown);
	ReturnGUID2WSTRING(guid, GUID_Spring);
	ReturnGUID2WSTRING(guid, GUID_Damper);
	ReturnGUID2WSTRING(guid, GUID_Inertia);
	ReturnGUID2WSTRING(guid, GUID_Friction);
	ReturnGUID2WSTRING(guid, GUID_CustomForce);
	ReturnGUID2WSTRING(guid, GUID_SysMouse);
	ReturnGUID2WSTRING(guid, GUID_SysKeyboard);
	ReturnGUID2WSTRING(guid, GUID_Joystick);
	ReturnGUID2WSTRING(guid, GUID_SysMouseEm);
	ReturnGUID2WSTRING(guid, GUID_SysMouseEm2);
	ReturnGUID2WSTRING(guid, GUID_SysKeyboardEm);
	ReturnGUID2WSTRING(guid, GUID_SysKeyboardEm2);

	swprintf(str, L"{%08lX-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX}",
			guid.Data1, guid.Data2, guid.Data3,
			guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
			guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
	return str;
}

#undef ReturnGUID2WSTRING
#undef WSTRING
#undef CAT

BOOL CALLBACK enum_devices(LPCDIDEVICEINSTANCEW instance, LPVOID)
{
	assert(device_count < ARRAYSIZE(devices));
	memcpy(&devices[device_count++], instance, sizeof(*instance));
	return DIENUM_CONTINUE;
}

BOOL CALLBACK enum_objects(LPCDIDEVICEOBJECTINSTANCEW doi, LPVOID ref)
{
	std::wostringstream *ss = (std::wostringstream*) ref;

	*ss << L"-- OBJECT:\r\n";
	*ss << L"guid                = " << guid_to_string(doi->guidType) << L"\r\n";
	*ss << L"dwOfs               = " << doi->dwOfs << L"\r\n";
	*ss << L"dwType              = " << doi->dwType << L"\r\n";
	*ss << L"dwFlags             = " << doi->dwFlags << L"\r\n";
	*ss << L"tszName             = " << doi->tszName << L"\r\n";
	*ss << L"dwFFMaxForce        = " << doi->dwFFMaxForce << L"\r\n";
	*ss << L"dwFFForceResolution = " << doi->dwFFForceResolution << L"\r\n";
	*ss << L"wCollectionNumber   = " << doi->wCollectionNumber << L"\r\n";
	*ss << L"wDesignatorIndex    = " << doi->wDesignatorIndex << L"\r\n";
	*ss << L"wUsagePage          = " << doi->wUsagePage << L"\r\n";
	*ss << L"wUsage              = " << doi->wUsage << L"\r\n";
	*ss << L"dwDimension         = " << doi->dwDimension << L"\r\n";
	*ss << L"wExponent           = " << doi->wExponent << L"\r\n";
	*ss << L"wReportId           = " << doi->wReportId << L"\r\n";

	return DIENUM_CONTINUE;
}

BOOL enum_effects(LPCDIEFFECTINFOW effect_info, LPVOID ref)
{
	std::wostringstream *ss = (std::wostringstream*) ref;

	*ss << L"-- EFFECT:\r\n";
	*ss << L"guid            = " << guid_to_string(effect_info->guid) << L"\r\n";
	*ss << L"dwEffType       = " << effect_info->dwEffType << L"\r\n";
	*ss << L"dwStaticParams  = " << effect_info->dwStaticParams << L"\r\n";
	*ss << L"dwDynamicParams = " << effect_info->dwDynamicParams << L"\r\n";
	*ss << L"tszName         = " << effect_info->tszName << L"\r\n";

	return DIENUM_CONTINUE;
}

void dump_hid(std::wostringstream &ss, const WCHAR *path)
{
	HANDLE file;

	PHIDP_PREPARSED_DATA preparsed_data;
	HIDP_CAPS hidp_caps;
	HIDP_BUTTON_CAPS *button_caps;
	HIDP_VALUE_CAPS *value_caps;
	NTSTATUS status;

	ss << L"\r\n- HID" << L"\r\n";
	file = CreateFileW(path, GENERIC_READ|GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, 0);

	if (file == INVALID_HANDLE_VALUE)
	{
		ss << L"failed to open " << path << L" with error " << GetLastError() << L"\r\n";
		return;
	}

	if (!HidD_GetPreparsedData(file, &preparsed_data))
	{
		ss << L"failed to get preparsed data " << GetLastError() << L"\r\n";
		CloseHandle(file);
		return;
	}

	status = HidP_GetCaps(preparsed_data, &hidp_caps);
	if (status != HIDP_STATUS_SUCCESS) {
		ss << L"failed to get caps " << GetLastError() << L"\r\n";
		CloseHandle(file);
		return;
	}

	CloseHandle(file);

	ss << L"Usage                     = " << hidp_caps.Usage << L"\r\n";
	ss << L"UsagePage                 = " << hidp_caps.UsagePage << L"\r\n";
	ss << L"InputReportByteLength     = " << hidp_caps.InputReportByteLength << L"\r\n";
	ss << L"OutputReportByteLength    = " << hidp_caps.OutputReportByteLength << L"\r\n";
	ss << L"FeatureReportByteLength   = " << hidp_caps.FeatureReportByteLength << L"\r\n";
	ss << L"NumberLinkCollectionNodes = " << hidp_caps.NumberLinkCollectionNodes << L"\r\n";
	ss << L"NumberInputButtonCaps     = " << hidp_caps.NumberInputButtonCaps << L"\r\n";
	ss << L"NumberInputValueCaps      = " << hidp_caps.NumberInputValueCaps << L"\r\n";
	ss << L"NumberInputDataIndices    = " << hidp_caps.NumberInputDataIndices << L"\r\n";
	ss << L"NumberOutputButtonCaps    = " << hidp_caps.NumberOutputButtonCaps << L"\r\n";
	ss << L"NumberOutputValueCaps     = " << hidp_caps.NumberOutputValueCaps << L"\r\n";
	ss << L"NumberOutputDataIndices   = " << hidp_caps.NumberOutputDataIndices << L"\r\n";
	ss << L"NumberFeatureButtonCaps   = " << hidp_caps.NumberFeatureButtonCaps << L"\r\n";
	ss << L"NumberFeatureValueCaps    = " << hidp_caps.NumberFeatureValueCaps << L"\r\n";
	ss << L"NumberFeatureDataIndices  = " << hidp_caps.NumberFeatureDataIndices << L"\r\n";

	USHORT num_button_caps = hidp_caps.NumberInputButtonCaps;
	button_caps = new HIDP_BUTTON_CAPS[num_button_caps]{0};
	status = HidP_GetButtonCaps(HidP_Input, button_caps, &num_button_caps, preparsed_data);
	if (status != HIDP_STATUS_SUCCESS) {
		ss << L"failed to get button caps " << GetLastError() << L"\r\n";
		return;
	}

	for (int i = 0; i < num_button_caps; i++)
	{
		ss <<"-- BUTTON CAPS " <<  i << L"\r\n";
		ss << L"UsagePage                = " << button_caps[i].UsagePage << L"\r\n";
		ss << L"ReportID                 = " << button_caps[i].ReportID << L"\r\n";
		ss << L"IsAlias                  = " << button_caps[i].IsAlias << L"\r\n";
		ss << L"BitField                 = " << button_caps[i].BitField << L"\r\n";
		ss << L"LinkCollection           = " << button_caps[i].LinkCollection << L"\r\n";
		ss << L"LinkUsage                = " << button_caps[i].LinkUsage << L"\r\n";
		ss << L"LinkUsagePage            = " << button_caps[i].LinkUsagePage << L"\r\n";
		ss << L"IsRange                  = " << button_caps[i].IsRange << L"\r\n";
		ss << L"IsStringRange            = " << button_caps[i].IsStringRange << L"\r\n";
		ss << L"IsDesignatorRange        = " << button_caps[i].IsDesignatorRange << L"\r\n";
		ss << L"IsAbsolute               = " << button_caps[i].IsAbsolute << L"\r\n";

		if (button_caps[i].IsRange)
		{
			ss << L"Range.UsageMin           = " << button_caps[i].Range.UsageMin << L"\r\n";
			ss << L"Range.UsageMax           = " << button_caps[i].Range.UsageMax << L"\r\n";
			ss << L"Range.StringMin          = " << button_caps[i].Range.StringMin << L"\r\n";
			ss << L"Range.StringMax          = " << button_caps[i].Range.StringMax << L"\r\n";
			ss << L"Range.DesignatorMin      = " << button_caps[i].Range.DesignatorMin << L"\r\n";
			ss << L"Range.DesignatorMax      = " << button_caps[i].Range.DesignatorMax << L"\r\n";
			ss << L"Range.DataIndexMin       = " << button_caps[i].Range.DataIndexMin << L"\r\n";
			ss << L"Range.DataIndexMax       = " << button_caps[i].Range.DataIndexMax << L"\r\n";
		}
		else
		{
			ss << L"NotRange.Usage           = " << button_caps[i].NotRange.Usage << L"\r\n";
			ss << L"NotRange.StringIndex     = " << button_caps[i].NotRange.StringIndex << L"\r\n";
			ss << L"NotRange.DesignatorIndex = " << button_caps[i].NotRange.DesignatorIndex << L"\r\n";
			ss << L"NotRange.DataIndex       = " << button_caps[i].NotRange.DataIndex << L"\r\n";
		}
	}

	USHORT num_value_caps = hidp_caps.NumberInputValueCaps;
	value_caps = new HIDP_VALUE_CAPS[num_value_caps]{0};

	status = HidP_GetValueCaps(HidP_Input, value_caps, &num_value_caps, preparsed_data);
	if (status != HIDP_STATUS_SUCCESS) {
		ss << L"failed to get value caps " << GetLastError() << L"\r\n";
		return;
	}

	for (int i = 0; i < num_value_caps; i++)
	{
		ss <<"-- VALUE CAPS " <<  i << L"\r\n";
		ss << L"UsagePage                = " << value_caps[i].UsagePage << L"\r\n";
		ss << L"ReportID                 = " << value_caps[i].ReportID << L"\r\n";
		ss << L"IsAlias                  = " << value_caps[i].IsAlias << L"\r\n";
		ss << L"BitField                 = " << value_caps[i].BitField << L"\r\n";
		ss << L"LinkCollection           = " << value_caps[i].LinkCollection << L"\r\n";
		ss << L"LinkUsage                = " << value_caps[i].LinkUsage << L"\r\n";
		ss << L"LinkUsagePage            = " << value_caps[i].LinkUsagePage << L"\r\n";
		ss << L"IsRange                  = " << value_caps[i].IsRange << L"\r\n";
		ss << L"IsStringRange            = " << value_caps[i].IsStringRange << L"\r\n";
		ss << L"IsDesignatorRange        = " << value_caps[i].IsDesignatorRange << L"\r\n";
		ss << L"IsAbsolute               = " << value_caps[i].IsAbsolute << L"\r\n";
		ss << L"HasNull                  = " << value_caps[i].HasNull << L"\r\n";
		ss << L"Reserved                 = " << value_caps[i].Reserved << L"\r\n";
		ss << L"BitSize                  = " << value_caps[i].BitSize << L"\r\n";
		ss << L"ReportCount              = " << value_caps[i].ReportCount << L"\r\n";
		ss << L"Reserved2[5]             = " << value_caps[i].Reserved2[5] << L"\r\n";
		ss << L"UnitsExp                 = " << value_caps[i].UnitsExp << L"\r\n";
		ss << L"Units                    = " << value_caps[i].Units << L"\r\n";
		ss << L"LogicalMin               = " << value_caps[i].LogicalMin << L"\r\n";
		ss << L"LogicalMax               = " << value_caps[i].LogicalMax << L"\r\n";
		ss << L"PhysicalMin              = " << value_caps[i].PhysicalMin << L"\r\n";
		ss << L"PhysicalMax              = " << value_caps[i].PhysicalMax << L"\r\n";

	    if (value_caps[i].IsRange)
	    {
		    ss << L"Range.UsageMin           = " << value_caps[i].Range.UsageMin << L"\r\n";
		    ss << L"Range.UsageMax           = " << value_caps[i].Range.UsageMax << L"\r\n";
		    ss << L"Range.StringMin          = " << value_caps[i].Range.StringMin << L"\r\n";
		    ss << L"Range.StringMax          = " << value_caps[i].Range.StringMax << L"\r\n";
		    ss << L"Range.DesignatorMin      = " << value_caps[i].Range.DesignatorMin << L"\r\n";
		    ss << L"Range.DesignatorMax      = " << value_caps[i].Range.DesignatorMax << L"\r\n";
		    ss << L"Range.DataIndexMin       = " << value_caps[i].Range.DataIndexMin << L"\r\n";
		    ss << L"Range.DataIndexMax       = " << value_caps[i].Range.DataIndexMax << L"\r\n";
	    }
	    else
	    {
		    ss << L"NotRange.Usage           = " << value_caps[i].NotRange.Usage << L"\r\n";
		    ss << L"NotRange.StringIndex     = " << value_caps[i].NotRange.StringIndex << L"\r\n";
		    ss << L"NotRange.DesignatorIndex = " << value_caps[i].NotRange.DesignatorIndex << L"\r\n";
		    ss << L"NotRange.DataIndex       = " << value_caps[i].NotRange.DataIndex << L"\r\n";
	    }
	}
}

void dump_dinput_device_data(int idx)
{
	LPDIRECTINPUTDEVICE8W dev = nullptr;
	GUID guid;

	assert(idx <= device_count);

	if (idx == 0)
	{
		SetWindowText(text_area, L"...");
		return;
	}

	guid = devices[idx-1].guidInstance;

	assert(SUCCEEDED(di8->CreateDevice(guid, &dev, nullptr)));

	std::wostringstream ss(L"", std::ios_base::ate);
	ss.setf(std::ios::hex, std::ios::basefield);
	ss.setf(std::ios::showbase);
	ss << L"- DINPUT DEVICE:" << L"\r\n";
	ss << L"dwDevType       = " << devices[idx-1].dwDevType << L"\r\n";
	ss << L"dwSize          = " << devices[idx-1].dwSize << L"\r\n";
	ss << L"guidInstance    = " << guid_to_string(devices[idx-1].guidInstance) << L"\r\n";
	ss << L"guidProduct     = " << guid_to_string(devices[idx-1].guidProduct) << L"\r\n";
	ss << L"tszInstanceName = " << devices[idx-1].tszInstanceName << L"\r\n";
	ss << L"tszProductName  = " << devices[idx-1].tszProductName << L"\r\n";
	ss << L"guidFFDriver    = " << guid_to_string(devices[idx-1].guidFFDriver) << L"\r\n";
	ss << L"wUsagePage      = " << devices[idx-1].wUsagePage << L"\r\n";
	ss << L"wUsage          = " << devices[idx-1].wUsage << L"\r\n";

	DIPROPDWORD pd;
	pd.diph.dwSize = sizeof(pd);
	pd.diph.dwHeaderSize = sizeof(pd.diph);
	pd.diph.dwObj = DIPH_DEVICE;
	pd.diph.dwHow = 0;
	dev->GetProperty(DIPROP_VIDPID, &pd.diph);

	ss << L"VIDPID          = " << pd.dwData << L"\r\n";

	DIPROPGUIDANDPATH pgp;
	pgp.diph.dwSize = sizeof(pgp);
	pgp.diph.dwHeaderSize = sizeof(pgp.diph);
	pgp.diph.dwObj = DIPH_DEVICE;
	pgp.diph.dwHow = 0;
	dev->GetProperty(DIPROP_GUIDANDPATH, &pgp.diph);

	ss << L"PATH            = " << pgp.wszPath << L"\r\n";

	dev->EnumObjects(enum_objects, &ss, DIDFT_ALL);
	dev->EnumEffects(enum_effects, &ss, DIEFT_ALL);
	dump_hid(ss, pgp.wszPath);

	SetWindowText(text_area, ss.str().c_str());
	dev->Release();
}

void populate_combobox(HWND combo_box)
{

	SendMessage(combo_box, CB_ADDSTRING, 0, (LPARAM) L"Select device...");
	SendMessage(combo_box, CB_SETCURSEL, 0, 0);

	for (int i = 0; i < device_count; i++)
		SendMessage(combo_box, CB_ADDSTRING, 0, (LPARAM) devices[i].tszProductName);
}

LRESULT CALLBACK dumper_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	if (msg == WM_CREATE)
	{
		combo_box = CreateWindow(WC_COMBOBOX, L"",
				CBS_DROPDOWNLIST | WS_CHILD | WS_OVERLAPPED | CBS_HASSTRINGS | WS_VISIBLE,
				0, 0, 500, 500,
				hwnd, NULL, NULL, NULL);
		assert(combo_box);
		populate_combobox(combo_box);

		text_area = CreateWindow(WC_EDIT, L"...",
				WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY | ES_AUTOHSCROLL,
				0, 40, 500, 800,
				hwnd, NULL, NULL, NULL);
		assert(text_area);
		SendMessage(text_area, WM_SETFONT, (WPARAM)GetStockObject(SYSTEM_FIXED_FONT), TRUE);
	}
	else if (msg == WM_COMMAND && HIWORD(wparam) == CBN_SELCHANGE)
	{
		int idx = SendMessage((HWND) lparam, CB_GETCURSEL, 0, 0);
		dump_dinput_device_data(idx);
	}
	else if (msg == WM_SYSCOMMAND && wparam == SC_CLOSE)
	{
		exit(0);
	}

	return DefWindowProc(hwnd, msg, wparam, lparam);
}

int main(int argc, char *argv[])
{
	MSG msg;
	HINSTANCE hInstance = GetModuleHandleA(NULL);
	WNDCLASSW cls = {0};
	cls.style = CS_HREDRAW | CS_VREDRAW;
	cls.hbrBackground = (HBRUSH)(COLOR_WINDOW);
	cls.hCursor = LoadCursor(NULL, IDC_ARROW);
	cls.hInstance = hInstance;
	cls.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	cls.lpszClassName = L"DinputDumperClass";
	cls.lpfnWndProc = dumper_wndproc;
	assert(RegisterClass(&cls));

	assert(SUCCEEDED(DirectInput8Create(GetModuleHandle(nullptr), DIRECTINPUT_VERSION, IID_IDirectInput8W, (void**)&di8, nullptr)));
	assert(SUCCEEDED(di8->EnumDevices(DI8DEVCLASS_ALL, enum_devices, nullptr, DIEDFL_ATTACHEDONLY)));

	HWND hwnd = CreateWindow(L"DinputDumperClass", L"dinput-dumper",
			WS_OVERLAPPEDWINDOW | WS_VISIBLE,
			CW_USEDEFAULT, 0, 550, 900,
			NULL, NULL, hInstance, NULL);
	assert(hwnd != NULL);

	ShowWindow(hwnd, SW_SHOW);

	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}
