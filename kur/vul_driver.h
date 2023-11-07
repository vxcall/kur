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
#define SERVICE_PATH_COMMON std::wstring(L"SYSTEM\\CurrentControlSet\\Services\\")

class vul_driver
{
  std::wstring driver_name;
  std::wstring device_name;

public:
  auto get_full_driver_path() const -> std::wstring;
  auto get_device_handle() -> BOOL;
  auto unload() const -> BOOL;

  HANDLE h_device = nullptr;

  vul_driver(std::wstring name, std::wstring device_name)
    : driver_name(std::move(name)), device_name(std::move(device_name))
  {
  }

  auto load() const -> BOOL;
  auto install() const -> BOOL;
  auto setup_reg_key() const -> BOOL;

  // cleanup functions
  auto uninstall() const -> BOOL;
  auto delete_reg_key() const -> BOOL;
  auto cleanup() const -> BOOL;
};
