#pragma once

#include "vul_driver.h"

class kur_t
{
  vul_driver vul_driver;

public:
  auto query_device_handle() const -> HANDLE;
  auto get_process_handle(DWORD pid, ACCESS_MASK access_mask) -> std::optional<HANDLE>;
  auto read(void* address, void* buffer, size_t buffer_size, HANDLE h_target_process) -> BOOL;
  auto write(void* address, void* buffer, size_t buffer_size, HANDLE h_target_process) -> BOOL;

  kur_t(std::wstring name, std::wstring device_name)
    : vul_driver(std::move(name), std::move(device_name))
  {
    try
    {
      vul_driver.install();
      vul_driver.setup_reg_key();
      vul_driver.load();
      vul_driver.get_device_handle();
      vul_driver.ioctl_initialize_driver();
    }
    catch (...)
    {
      try { vul_driver.uninstall(); }
      catch (...)
      {
      }
      try { vul_driver.delete_reg_key(); }
      catch (...)
      {
      }
      try { vul_driver.unload(); }
      catch (...)
      {
      }

      // Rethrow the original exception
      throw;
    }
  }


  ~kur_t()
  {
    // Suppress all exceptions to prevent std::terminate() from being called.
    try
    {
      const auto status = utils::open_reg_key(HKEY_LOCAL_MACHINE, SERVICE_PATH_COMMON.c_str());

      if (status == ERROR_SUCCESS || status == ERROR_FILE_NOT_FOUND)
      {
        vul_driver.unload();
        vul_driver.delete_reg_key();
        vul_driver.uninstall();
      }
      else
      {
        std::cerr << "cleanup: key cannot be opened correctly, status: " << status << std::endl;
      }
    }
    catch (...)
    {
    }
  }
};
