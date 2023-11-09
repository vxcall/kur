#include "pch.h"
#include "kur.h"

auto main() -> int
{
  kur_t kur = kur_t(L"EchoDrv", L"\\Device\\EchoDrv");

  const HANDLE h_device = kur.query_device_handle();
  printf("device_handle: %p\n", h_device);

  std::cin.get();
  return 0;
}

auto kur_t::get_process_handle(DWORD pid, ACCESS_MASK access_mask) -> HANDLE
{
  if (!is_initialized)
  {
    std::cerr << "kur_t::get_process_handle: kur_t::init() is not yet invoked." << std::endl;
    return INVALID_HANDLE_VALUE;
  }
  return vul_driver.ioctl_get_process_handle(pid, access_mask);
}

// not tested yet
auto kur_t::read(void* address, void* buffer, size_t buffer_size, HANDLE h_target_process) -> BOOL
{
  if (!is_initialized)
  {
    std::cerr << "kur_t::read: kur_t::init() is not yet invoked." << std::endl;
    return FALSE;
  }
  return vul_driver.ioctl_mm_copy_virtual_memory(address, buffer, buffer_size, h_target_process);
}

// might return nullptr
auto kur_t::query_device_handle() const -> HANDLE
{
  if (!is_initialized)
  {
    std::cerr << "kur_t::query_device_handle: kur_t::init() is not yet invoked." << std::endl;
    return INVALID_HANDLE_VALUE;
  }
  return vul_driver.h_device;
}
