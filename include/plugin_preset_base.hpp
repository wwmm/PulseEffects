/*
 *  Copyright © 2017-2022 Wellington Wallace
 *
 *  This file is part of EasyEffects.
 *
 *  EasyEffects is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  EasyEffects is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with EasyEffects.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef PLUGIN_PRESET_BASE_HPP
#define PLUGIN_PRESET_BASE_HPP

#include <giomm.h>
#include <iostream>
#include <nlohmann/json.hpp>
#include "preset_type.hpp"
#include "util.hpp"

class PluginPresetBase {
 public:
  PluginPresetBase() = default;
  PluginPresetBase(const PluginPresetBase&) = delete;
  auto operator=(const PluginPresetBase&) -> PluginPresetBase& = delete;
  PluginPresetBase(const PluginPresetBase&&) = delete;
  auto operator=(const PluginPresetBase&&) -> PluginPresetBase& = delete;
  virtual ~PluginPresetBase() = default;

  void write(PresetType preset_type, nlohmann::json& json) {
    try {
      switch (preset_type) {
        case PresetType::output:
          save(json, "output", output_settings);
          break;
        case PresetType::input:
          save(json, "input", input_settings);
          break;
      }
    } catch (const nlohmann::json::exception& e) {
      util::warning(e.what());
    }
  }

  void read(PresetType preset_type, const nlohmann::json& json) {
    try {
      switch (preset_type) {
        case PresetType::output:
          load(json, "output", output_settings);
          break;
        case PresetType::input:
          load(json, "input", input_settings);
          break;
      }
    } catch (const nlohmann::json::exception& e) {
      util::warning(e.what());
    }
  }

 protected:
  Glib::RefPtr<Gio::Settings> input_settings, output_settings;

  virtual void save(nlohmann::json& json, const std::string& section, const Glib::RefPtr<Gio::Settings>& settings) = 0;

  virtual void load(const nlohmann::json& json,
                    const std::string& section,
                    const Glib::RefPtr<Gio::Settings>& settings) = 0;

  template <typename T>
  auto get_default(const Glib::RefPtr<Gio::Settings>& settings, const std::string& key) -> T {
    Glib::Variant<T> value;

    settings->get_default_value(key, value);

    return value.get();
  }

  template <typename T>
  void update_key(const nlohmann::json& json,
                  const Glib::RefPtr<Gio::Settings>& settings,
                  const std::string& key,
                  const std::string& json_key) {
    Glib::Variant<T> aux;

    settings->get_value(key, aux);

    T current_value = aux.get();

    T new_value = json.value(json_key, get_default<T>(settings, key));

    if (is_different(current_value, new_value)) {
      auto v = Glib::Variant<T>::create(new_value);

      settings->set_value(key, v);
    }
  }

  void update_string_key(const nlohmann::json& json,
                         const Glib::RefPtr<Gio::Settings>& settings,
                         const std::string& key,
                         const std::string& json_key) {
    std::string current_value = settings->get_string(key);

    std::string new_value = json.value(json_key, get_default<std::string>(settings, key));

    if (current_value != new_value) {
      settings->set_string(key, new_value);
    }
  }

 private:
  /*
    Very naive test for equal values...
  */

  template <typename T>
  auto is_different(const T& a, const T& b) -> bool {
    return a != b;
  }
};

#endif
