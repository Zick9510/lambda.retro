// Driver.hpp

#pragma once

#include "Common.hpp"

class Driver {
public:
  Driver() = default;

  void start(MainConfig& config);

private:

  bool _read(std::string& source);
  std::optional<std::string> readSourceFile(const std::filesystem::path& path) const;

};
