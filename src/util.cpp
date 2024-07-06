/*
 *  Copyright © 2017-2024 Wellington Wallace
 *
 *  This file is part of Easy Effects.
 *
 *  Easy Effects is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Easy Effects is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Easy Effects. If not, see <https://www.gnu.org/licenses/>.
 */

#include "util.hpp"
#include <qdebug.h>
#include <qlogging.h>
#include <sys/types.h>
#include <array>
#include <cmath>
#include <exception>
#include <filesystem>
#include <iostream>
#include <numbers>
#include <ostream>
#include <regex>
#include <string>
#include <thread>
#include <vector>

namespace util {

auto prepare_debug_message(const std::string& message, source_location location) -> std::string {
  auto file_path = std::filesystem::path{location.file_name()};

  std::string msg = file_path.filename().string() + ":" + to_string(location.line()) + "\t" + message;

  return msg;
}

void debug(const std::string& s, source_location location) {
  qDebug().noquote() << prepare_debug_message(s, location);
}

void fatal(const std::string& s, source_location location) {
  qFatal().noquote() << prepare_debug_message(s, location);
}

void critical(const std::string& s, source_location location) {
  qCritical().noquote() << prepare_debug_message(s, location);
}

void warning(const std::string& s, source_location location) {
  qWarning().noquote() << prepare_debug_message(s, location);
}

void info(const std::string& s, source_location location) {
  qInfo().noquote() << prepare_debug_message(s, location);
}

void print_thread_id() {
  std::cout << "thread id: " << std::this_thread::get_id() << '\n';
}

auto normalize(const double& x, const double& max, const double& min) -> double {
  // Mainly used for gating level bar in gate effects
  return (x - min) / (max - min);
}

auto linear_to_db(const float& amp) -> float {
  if (amp >= minimum_linear_level) {
    return 20.0F * std::log10(amp);
  }

  return minimum_db_level;
}

auto linear_to_db(const double& amp) -> double {
  if (amp >= minimum_linear_d_level) {
    return 20.0 * std::log10(amp);
  }

  return minimum_db_d_level;
}

auto db_to_linear(const float& db) -> float {
  return std::exp((db / 20.0F) * std::numbers::ln10_v<float>);
}

auto db_to_linear(const double& db) -> double {
  return std::exp((db / 20.0) * std::numbers::ln10);
}

auto remove_filename_extension(const std::string& basename) -> std::string {
  return basename.substr(0U, basename.find_last_of('.'));
}

auto get_files_name(const std::filesystem::path& dir_path, const std::string& ext) -> std::vector<std::string> {
  std::vector<std::string> names;

  for (std::filesystem::directory_iterator it{dir_path}; it != std::filesystem::directory_iterator{}; ++it) {
    if (std::filesystem::is_regular_file(it->status())) {
      if (it->path().extension() == ext) {
        names.push_back(it->path().stem().string());
      }
    }
  }

  return names;
}

auto str_contains(const std::string& haystack, const std::string& needle) -> bool {
  // This helper indicates if the needle is contained in the haystack string,
  // but the empty needle will NOT return true.

  // Instead .find method of C++ string class returns a size_type different
  // than std::string::npos when the needle is empty indicating that an empty
  // string IS CONTAINED in the haystack. That's pointless, so here is this helper.

  if (needle.empty()) {
    return false;
  }

  return (haystack.find(needle) != std::string::npos);
}

void str_trim_start(std::string& str) {
  // This util removes whitespaces such as simple space " ", new line "\n",
  // carriage return "\r", tab "\t", vertical tab "\v" and form feed "\f"
  // at the start of the given string.
  // No copy involved, the input string is just modified if needed.

  str.erase(0U, str.find_first_not_of(" \n\r\t\v\f"));
}

void str_trim_end(std::string& str) {
  // Same as above, but at the end of the given string.
  str.erase(str.find_last_not_of(" \n\r\t\v\f") + 1U);
}
void str_trim(std::string& str) {
  // Trim both sides of the given string. See above.
  str_trim_end(str);
  str_trim_start(str);
}

auto search_filename(const std::filesystem::path& path,
                     const std::string& filename,
                     std::string& full_path_result,
                     const uint& top_scan_level) -> bool {
  // Recursive util to search a filename from an origin full path directory.
  // The search is performed in subdirectories and it's stopped at a specified
  // sublevel (top_scan_level = 1 searches only in the path).

  if (!std::filesystem::exists(path) || !std::filesystem::is_directory(path)) {
    return false;
  }

  const auto fn = path / filename;

  // Get the file in this directory, if exists.
  if (std::filesystem::exists(fn) && std::filesystem::is_regular_file(fn)) {
    // File found, abort the search.
    full_path_result = fn.c_str();

    return true;
  }

  // The file is not in this directory, search in subdirectories.
  const auto scan_level = top_scan_level - 1U;

  if (scan_level == 0U) {
    return false;
  }

  auto it = std::filesystem::directory_iterator{path};

  try {
    while (it != std::filesystem::directory_iterator{}) {
      if (std::filesystem::is_directory(it->status())) {
        if (const auto p = it->path(); !p.empty()) {
          // Continue the search in the subfolder.
          const auto found = search_filename(p, filename, full_path_result, scan_level);

          if (found) {
            return true;
          }
        }
      }

      ++it;
    }
  } catch (const std::exception& e) {
    util::warning(e.what());

    return false;
  }

  return false;
}

auto compare_versions(const std::string& v0, const std::string& v1) -> int {
  /* This is an util to compare two strings as semver, mainly used to compare
     two Pipewire versions.
     The format should adhere to what is defined at `https://semver.org/`.
     The additional extension label, if present, is ignored and fortunately
     we don't need to look at it since Pipewire does not use it.

     Given two version strings v0 and v1, this util returns an integer:
     - 0 if the versions are equal;
     - 1 if v0 is higher than v1;
     - -1 if v0 is lower than v1;
     - Whichever other number if the comparison fails (i.e. giving one or
       both strings not respecting the semver format).
  */

  struct SemVer {
    int major = -1;
    int minor = -1;
    int patch = -1;
  };

  static const auto re_semver = std::regex(R"(^(\d+)\.?(\d+)?\.?(\d+)?)");

  std::array<SemVer, 2> sv{};
  std::array<std::string, 2> v{v0, v1};

  // SemVer struct initialization. Loop the given strings.
  for (int v_idx = 0; v_idx < 2; v_idx++) {
    // For both strings, execute the regular expression search,
    // then loop through the submatches.
    std::smatch match;

    if (!std::regex_search(v[v_idx], match, re_semver)) {
      // The given string is not a semver: the comparison failed.
      return 9;
    }

    // Submatches lookup
    for (int sub_idx = 0, size = match.size(); sub_idx < size; sub_idx++) {
      // Fill the structure converting the string to an integer.
      switch (sub_idx) {
        case 1:  // major
          str_to_num(match[sub_idx].str(), sv[v_idx].major);
          break;

        case 2:  // minor
          str_to_num(match[sub_idx].str(), sv[v_idx].minor);
          break;

        case 3:  // patch
          str_to_num(match[sub_idx].str(), sv[v_idx].patch);
          break;

        default:
          // sub_idx = 0: the first group which is the entire match we don't need.
          break;
      }
    }
  }

  // Now that we are sure to have two valid semver, let's compare each part.
  if (sv[0].major < sv[1].major) {
    return -1;
  } else if (sv[0].major > sv[1].major) {
    return 1;
  }

  if (sv[0].minor < sv[1].minor) {
    return -1;
  } else if (sv[0].minor > sv[1].minor) {
    return 1;
  }

  if (sv[0].patch < sv[1].patch) {
    return -1;
  } else if (sv[0].patch > sv[1].patch) {
    return 1;
  }

  return 0;
}

}  // namespace util
