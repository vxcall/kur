#include "pch.h"
#include "kur.h"

auto kur_t::get_process_handle(DWORD pid, ACCESS_MASK access_mask) -> std::optional<HANDLE>
{
  return vul_driver.ioctl_get_process_handle(pid, access_mask);
}

auto kur_t::read(void* address, void* buffer, size_t buffer_size, HANDLE h_target_process) -> BOOL
{
  return vul_driver.ioctl_mm_copy_virtual_memory(address, buffer, buffer_size, h_target_process);
}

auto kur_t::write(void* to_address, void* from_address, size_t size, HANDLE h_target_process) -> BOOL
{
  return vul_driver.ioctl_mm_copy_virtual_memory(from_address, to_address, size, h_target_process);
}

auto kur_t::query_device_handle() const -> HANDLE
{
  return vul_driver.h_device;
}
