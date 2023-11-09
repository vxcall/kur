#pragma once

class kur_error : public std::runtime_error {
public:
  kur_error(const std::string& function_name, const std::string& message)
    : std::runtime_error("[-] In " + function_name + ": " + message) {}
};

#define KUR_ERROR(msg) kur_error(__func__, msg)
