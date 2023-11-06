#pragma once
#include "vul_driver.h"
#include "echo_driver_resource.h"
#include "utils.h"
#include <Windows.h>
#include <filesystem>
#include <iostream>
#include <fstream>

#pragma comment(lib, "ntdll.lib")

constexpr auto SE_LOAD_DRIVER_PRIVILEGE = 10L;
#define SERVICE_PATH_COMMON std::wstring(L"SYSTEM\\CurrentControlSet\\Services\\" + driver_name)

namespace vul_driver
{
  const std::wstring driver_name = L"echo";

  auto get_full_driver_path() -> std::wstring;
  auto install_driver() -> BOOL;
  auto setup_reg_key() -> BOOL;
  auto load_driver() -> BOOL;
  auto cleanup_reg_driver(HANDLE h_device) -> BOOL;
  auto delete_reg_key() -> BOOL;
  auto uninstall_driver() -> BOOL;
  auto unload_driver() -> BOOL;
}

