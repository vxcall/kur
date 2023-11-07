#include "kur.h"

auto main() -> int
{
  kur_t kur = kur_t(L"EchoDrv", L"\\Device\\EchoDrv");
  auto status = kur.init();

  if (!status)
    return 1;
  const HANDLE h_device = kur.query_device_handle();
  printf("device_handle: %p\n", h_device);

  std::cin.get();
  kur.cleanup();
  return 0;
}

// the function that's supposed to be called by the user first
auto kur_t::init() -> BOOL
{
  if (!vul_driver.install())
  {
    std::cerr << "Deploying driver failed." << std::endl;
    return FALSE;
  }

  auto status = vul_driver.setup_reg_key();
  if (!status)
  {
    std::cerr << "Setting up registry key to load driver is failed." << std::endl;
    vul_driver.uninstall();
    return FALSE;
  }

  status = vul_driver.load();
  if (!status)
  {
    std::cerr << "Loading the vulnerable driver failed." << std::endl;
    vul_driver.uninstall();
    vul_driver.delete_reg_key();
    return FALSE;
  }

  status = vul_driver.get_device_handle();
  if (!status)
  {
    std::cerr << "Getting device handle failed" << std::endl;
    vul_driver.uninstall();
    vul_driver.delete_reg_key();
    vul_driver.unload();
    return FALSE;
  }

  status = vul_driver.initialize_driver();
  if (!status)
  {
    std::cerr << "Getting device handle failed" << std::endl;
    vul_driver.uninstall();
    vul_driver.delete_reg_key();
    vul_driver.unload();
    CloseHandle(vul_driver.h_device);
    return FALSE;
  }

  return TRUE;
}

// might return nullptr
auto kur_t::query_device_handle() -> HANDLE
{
  return vul_driver.h_device;
}

auto kur_t::cleanup() -> BOOL
{
  // first check if the key exists
  const auto status = utils::open_reg_key(HKEY_LOCAL_MACHINE, (SERVICE_PATH_COMMON).c_str());
  if (status != ERROR_SUCCESS)
  {
    if (status == ERROR_FILE_NOT_FOUND)
    {
      return TRUE;
    }
    std::cerr << "cleanup: key cannot be opened correctly" << std::endl;
    return FALSE;
  }

  if (vul_driver.h_device != nullptr)
    CloseHandle(vul_driver.h_device);
  // unload the driver
  BOOL st = vul_driver.unload();
  if (!st)
  {
    std::cerr << "cleanup: unloading driver failed" << std::endl;
    return FALSE;
  }

  // delete the key
  st = vul_driver.delete_reg_key();
  if (!st)
  {
    std::cerr << "cleanup: deleting key failed" << std::endl;
    return FALSE;
  }

  // delete the driver file
  st = vul_driver.uninstall();
  if (!st)
  {
    std::cerr << "cleanup: uninstalling driver file failed" << std::endl;
    return FALSE;
  }

  return TRUE;
}
