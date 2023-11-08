#pragma once

#include "vul_driver.h"

class kur_t
{
  vul_driver vul_driver;
  bool is_initialized = false;

public:
  auto init() -> BOOL;
  auto cleanup() const -> BOOL;
  auto query_device_handle() const -> HANDLE;
  auto get_process_handle(DWORD pid, ACCESS_MASK access_mask) -> HANDLE;
  auto read(void* address, void* buffer, size_t buffer_size, HANDLE h_target_process) -> BOOL;

  kur_t(std::wstring name, std::wstring device_name)
    : vul_driver(std::move(name), std::move(device_name))
  {
  }
};
