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

#ifndef PLUGIN_UI_BASE_HPP
#define PLUGIN_UI_BASE_HPP

#include <giomm.h>
#include <gtkmm.h>
#include "plugin_name.hpp"
#include "scale_helper.hpp"
#include "spinbutton_helper.hpp"
#include "util.hpp"

class PluginUiBase {
 public:
  PluginUiBase(const Glib::RefPtr<Gtk::Builder>& builder, const std::string& schema, const std::string& schema_path);
  PluginUiBase(const PluginUiBase&) = delete;
  auto operator=(const PluginUiBase&) -> PluginUiBase& = delete;
  PluginUiBase(const PluginUiBase&&) = delete;
  auto operator=(const PluginUiBase&&) -> PluginUiBase& = delete;
  virtual ~PluginUiBase();

  Gtk::ToggleButton* bypass = nullptr;

  void set_transient_window(Gtk::Window* transient_window);

  void on_new_input_level(const float& left, const float& right);
  void on_new_output_level(const float& left, const float& right);

 protected:
  std::string name;

  Glib::RefPtr<Gio::Settings> settings;

  Gtk::Button* reset_button = nullptr;

  Gtk::LevelBar *input_level_left = nullptr, *input_level_right = nullptr;
  Gtk::LevelBar *output_level_left = nullptr, *output_level_right = nullptr;
  Gtk::Label *input_level_left_label = nullptr, *input_level_right_label = nullptr;
  Gtk::Label *output_level_left_label = nullptr, *output_level_right_label = nullptr;

  Gtk::Window* transient_window = nullptr;

  std::vector<sigc::connection> connections;

  // reset plugin method
  virtual void reset() = 0;

  template <typename T>
  auto level_to_localized_string(const T& value, const int& places) -> std::string {
    std::ostringstream msg;

    msg.precision(places);

    msg << std::fixed << value;

    return msg.str();
  }

  static void prepare_spinbutton(Gtk::SpinButton* button, const std::string& unit);

  static auto string_to_float(const std::string& value) -> float;

 private:
  template <typename T1, typename T2, typename T3, typename T4>
  void update_level(const T1& w_left,
                    const T2& w_left_label,
                    const T3& w_right,
                    const T4& w_right_label,
                    const float& left,
                    const float& right) {
    if (left >= -99.0) {
      auto db_value = util::db_to_linear(left);

      if (db_value < 0.0) {
        db_value = 0.0;
      } else if (db_value > 1.0) {
        db_value = 1.0;
      }

      w_left->set_value(db_value);
      w_left_label->set_text(level_to_localized_string(left, 0));
    } else {
      w_left->set_value(0.0);
      w_left_label->set_text("-99");
    }

    if (right >= -99.0) {
      auto db_value = util::db_to_linear(right);

      if (db_value < 0.0) {
        db_value = 0.0;
      } else if (db_value > 1.0) {
        db_value = 1.0;
      }

      w_right->set_value(db_value);
      w_right_label->set_text(level_to_localized_string(right, 0));
    } else {
      w_right->set_value(0.0);
      w_right_label->set_text("-99");
    }
  }
};

#endif
