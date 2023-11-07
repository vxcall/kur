#pragma once
#include <filesystem>
#include <iostream>
#include <Windows.h>
#include <winternl.h>


namespace utils
{
  using NtLoadDriver = NTSTATUS(*)(PUNICODE_STRING DriverServiceName);
  using NtUnloadDriver = NTSTATUS(*)(PUNICODE_STRING DriverServiceName);
  using RtlAdjustPrivilege = NTSTATUS(*)(ULONG Privilege, BOOLEAN Enable, BOOLEAN Client, PBOOLEAN WasEnabled);

  auto get_temp_path() -> std::wstring;
  auto get_ntdll() -> HMODULE;
  auto open_reg_key(HKEY h_key_root, LPCWSTR sub_key) -> LSTATUS;
}
