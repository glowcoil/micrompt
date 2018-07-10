/*
* IPCWindow.cpp
* -------------
* Purpose: Hidden window to receive file open commands from another OpenMPT instance
* Notes  : (currently none)
* Authors: OpenMPT Devs
* The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
*/


#include "stdafx.h"
#include "IPCWindow.h"

#include "../common/version.h"
#include "Mptrack.h"


OPENMPT_NAMESPACE_BEGIN

namespace IPCWindow
{

	static const TCHAR ClassName[] = _T("OpenMPT_IPC_Wnd");
	static HWND ipcWindow = nullptr;

	static LRESULT CALLBACK IPCWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		if(uMsg == WM_COPYDATA)
		{
			const auto &copyData = *reinterpret_cast<const COPYDATASTRUCT *>(lParam);
			LRESULT result = 0;
			switch(copyData.dwData)
			{
			case static_cast<ULONG>(Function::OpenMultipleAndSetWindowForeground):
				{
					size_t remain = copyData.cbData / sizeof(WCHAR);
					const auto *str = static_cast<const WCHAR *>(copyData.lpData);
					while(remain > 0)
					{
						size_t length = ::wcsnlen(str, remain);
						const std::wstring name(str, length);
						theApp.OpenDocumentFile(mpt::PathString::FromWide(name).AsNative().c_str());
						auto mainWnd = theApp.GetMainWnd();
						if(mainWnd)
						{
							if(mainWnd->IsIconic()) mainWnd->ShowWindow(SW_RESTORE);
							mainWnd->SetForegroundWindow();
						}
						// Skip null terminator between strings
						if(length < remain)
							length++;
						str += length;
						remain -= length;
					}
					result = 1;
				}
				break;
			case static_cast<ULONG>(Function::Open):
				{
					std::size_t count = copyData.cbData / sizeof(WCHAR);
					const WCHAR* data = static_cast<const WCHAR *>(copyData.lpData);
					const std::wstring name = std::wstring(data, data + count);
					result = theApp.OpenDocumentFile(mpt::PathString::FromWide(name).AsNative().c_str()) ? 1 : 0;
				}
				break;
			case static_cast<ULONG>(Function::SetWindowForeground):
				{
					auto mainWnd = theApp.GetMainWnd();
					if(mainWnd)
					{
						if(mainWnd->IsIconic())
						{
							mainWnd->ShowWindow(SW_RESTORE);
						}
						mainWnd->SetForegroundWindow();
						result = 1;
					} else
					{
						result = 0;
					}
				}
				break;
			case static_cast<ULONG>(Function::GetVersion):
				{
					result = Version::Current().GetRawVersion();
				}
				break;
			case static_cast<ULONG>(Function::GetArchitecture):
				{
					#if MPT_OS_WINDOWS
						result = static_cast<int32>(mpt::Windows::GetProcessArchitecture());
					#else
						result = -1;
					#endif
				}
				break;
			default:
				result = 0;
				break;
			}
			return result;
		}
		return ::DefWindowProc(hwnd, uMsg, wParam, lParam);
	}

	void Open(HINSTANCE hInstance)
	{
		WNDCLASS ipcWindowClass =
		{
			0,
			IPCWindowProc,
			0,
			0,
			hInstance,
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			ClassName
		};
		auto ipcAtom = RegisterClass(&ipcWindowClass);
		ipcWindow = CreateWindow(MAKEINTATOM(ipcAtom), _T("OpenMPT IPC Window"), 0, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, 0);
	}

	void Close()
	{
		::DestroyWindow(ipcWindow);
		ipcWindow = nullptr;
	}

	LRESULT SendIPC(HWND ipcWnd, Function function, mpt::const_byte_span data)
	{
		if(!ipcWnd)
		{
			return 0;
		}
		if(!Util::TypeCanHoldValue<DWORD>(data.size()))
		{
			return 0;
		}
		COPYDATASTRUCT copyData{};
		copyData.dwData = static_cast<ULONG>(function);
		copyData.cbData = mpt::saturate_cast<DWORD>(data.size());
		copyData.lpData = const_cast<void*>(mpt::void_cast<const void*>(data.data()));
		return ::SendMessage(ipcWnd, WM_COPYDATA, 0, reinterpret_cast<LPARAM>(&copyData));
	}

	HWND FindIPCWindow()
	{
		return ::FindWindow(ClassName, nullptr);
	}


	bool SendToIPC(const std::vector<mpt::PathString> &filenames)
	{
		HWND ipcWnd = FindIPCWindow();
		if(!ipcWnd)
		{
			return false;
		}
		bool result = true;
		for(const auto &filename : filenames)
		{
			if(SendIPC(ipcWnd, Function::Open, mpt::as_span(filename.ToWide())) == 0)
			{
				result = false;
			}
		}
		SendIPC(ipcWnd, Function::SetWindowForeground);
		return result;
	}

}

OPENMPT_NAMESPACE_END