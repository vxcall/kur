#pragma once

#include "vul_driver.h"

class kur_t
{
  vul_driver vul_driver;

public:
  auto init() -> BOOL;
  auto cleanup() -> BOOL;
  auto query_device_handle() -> HANDLE;

  kur_t(std::wstring name, std::wstring device_name)
    : vul_driver(std::move(name), std::move(device_name))
  {
  }
};
