#pragma once

#include "vul_driver.h"

namespace kur
{
	auto init() -> BOOL;
	auto cleanup(HANDLE h_device) -> BOOL;
}
