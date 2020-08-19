#include "pch.h"

using namespace System;
using namespace System::IO;

void GetDeviceIds(String^ getDriveLetter, String^% VID, String^% PID, String^% Revision, String^% SN);

int main(array<System::String ^> ^args)
{
	String^ VID;
	String^ PID;
	String^ Revision;
	String^ SN;
	GetDeviceIds("D:\\", VID, PID, Revision, SN);
    return 0;
}

struct FileHandleDelete {
	typedef HANDLE pointer;
	void operator ()(HANDLE handle) const {
		if (handle != INVALID_HANDLE_VALUE) {
			CloseHandle(handle);
		}
	}
};

typedef std::unique_ptr<HANDLE, FileHandleDelete> FileHandleDeleter;

void GetDeviceIds(String^ getDriveLetter, String^% VID, String^% PID, String^% Revision, String^% SN)
{
	auto devicePath = String::Format("\\\\.\\{0}", Path::GetPathRoot(getDriveLetter)->TrimEnd('\\'));

	auto hDrive = CreateFileW(CStringW(devicePath), 0, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (hDrive == INVALID_HANDLE_VALUE)
	{
		const auto win32Err = GetLastError();
		throw gcnew Exception(String::Format("CreateFile Fail, Win32Err=0x{0:X}, FilePath={1}", win32Err, devicePath));
	}

	FileHandleDeleter phDrive(hDrive);
	
	//大き目の値として4096バイトを確保。ERROR_INSUFFICIENT_BUFFERへの対応は未実装
	std::vector<BYTE> Buf(4096);

	STORAGE_PROPERTY_QUERY inData;
	memset(&inData, 0, sizeof inData);
	inData.PropertyId = StorageDeviceProperty;
	inData.QueryType = PropertyStandardQuery;

	DWORD bytesReturned = 0;
	auto bResult = DeviceIoControl(hDrive, IOCTL_STORAGE_QUERY_PROPERTY, &inData, sizeof(inData),
		Buf.data(), Buf.size(), &bytesReturned, nullptr);
	if (!bResult)
	{
		const auto win32Err = GetLastError();
		throw gcnew Exception(String::Format("DeviceIoControl(IOCTL_STORAGE_QUERY_PROPERTY) Fail, Win32Err=0x{0:X}", win32Err));
	}
	
	const auto pHeader = reinterpret_cast<const STORAGE_DEVICE_DESCRIPTOR*>(Buf.data());

	if(pHeader->VendorIdOffset == 0)
	{
		VID = String::Empty;
	}
	else
	{
		VID = gcnew String(reinterpret_cast<LPCSTR>(Buf.data() + pHeader->VendorIdOffset));
	}

	if (pHeader->ProductIdOffset == 0)
	{
		devicePath = String::Empty;
	}
	else
	{
		PID = gcnew String(reinterpret_cast<LPCSTR>(Buf.data() + pHeader->ProductIdOffset));
	}


	if (pHeader->ProductRevisionOffset == 0)
	{
		Revision = String::Empty;
	}
	else
	{
		Revision = gcnew String(reinterpret_cast<LPCSTR>(Buf.data() + pHeader->ProductRevisionOffset));
	}


	if (pHeader->SerialNumberOffset == 0)
	{
		SN = String::Empty;
	}
	else
	{
		SN = gcnew String(reinterpret_cast<LPCSTR>(Buf.data() + pHeader->SerialNumberOffset));
	}
	
}
