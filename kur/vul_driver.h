#pragma once
#include "vul_driver.h"
#include "echo_driver_resource.h"
#include "utils.h"
#include <Windows.h>
#include <iostream>
#include <fstream>

#pragma comment(lib, "ntdll.lib")

#define SE_LOAD_DRIVER_PRIVILEGE 10L
#define SERVICE_PATH_COMMON std::wstring(L"SYSTEM\\CurrentControlSet\\Services\\")

namespace vul_driver
{
  const std::wstring driver_name = L"echo";

  auto get_full_driver_path() -> std::wstring;
  auto install_driver() -> BOOL;
  auto delete_registry_key() -> BOOL;
  auto setup_registry_key() -> BOOL;
  auto service_start() -> BOOL;
}
