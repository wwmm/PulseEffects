/*
 *  Copyright © 2017-2020 Wellington Wallace
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

#ifndef PLUGIN_BASE_HPP
#define PLUGIN_BASE_HPP

#include <giomm.h>
#include <pipewire/filter.h>
#include <spa/param/latency-utils.h>
#include <mutex>
#include <ranges>
#include <span>
#include "pipe_manager.hpp"
#include "plugin_name.hpp"

class PluginBase {
 public:
  PluginBase(std::string tag,
             std::string plugin_name,
             const std::string& schema,
             const std::string& schema_path,
             PipeManager* pipe_manager,
             const bool& enable_probe = false);
  PluginBase(const PluginBase&) = delete;
  auto operator=(const PluginBase&) -> PluginBase& = delete;
  PluginBase(const PluginBase&&) = delete;
  auto operator=(const PluginBase&&) -> PluginBase& = delete;
  virtual ~PluginBase();

  struct data;

  struct port {
    struct data* data;
  };

  struct data {
    struct port* in_left = nullptr;
    struct port* in_right = nullptr;

    struct port* out_left = nullptr;
    struct port* out_right = nullptr;

    struct port* probe_left = nullptr;
    struct port* probe_right = nullptr;

    PluginBase* pb = nullptr;
  };

  std::string log_tag, name;

  pw_filter* filter = nullptr;

  bool enable_probe = false;

  uint n_samples = 0U;

  uint rate = 0U;

  float sample_duration = 0.0F;

  bool bypass = false;

  bool connected_to_pw = false;

  bool post_messages = false;

  [[nodiscard]] auto get_node_id() const -> uint;

  void set_active(const bool& state) const;

  void connect_to_pw();

  void disconnect_from_pw();

  virtual void setup();

  virtual void process(std::span<float>& left_in,
                       std::span<float>& right_in,
                       std::span<float>& left_out,
                       std::span<float>& right_out);

  virtual void process(std::span<float>& left_in,
                       std::span<float>& right_in,
                       std::span<float>& left_out,
                       std::span<float>& right_out,
                       std::span<float>& probe_left,
                       std::span<float>& probe_right);

  sigc::signal<void(float, float)> input_level;
  sigc::signal<void(float, float)> output_level;

 protected:
  std::mutex data_mutex;

  Glib::RefPtr<Gio::Settings> settings;

  PipeManager* pm = nullptr;

  spa_hook listener{};

  data pf_data = {};

  float input_gain = 1.0F;
  float output_gain = 1.0F;

  float notification_time_window = 1.0F / 20.0F;  // seconds
  float notification_dt = 0.0F;

  void initialize_listener();

  void notify();

  void get_peaks(const std::span<float>& left_in,
                 const std::span<float>& right_in,
                 std::span<float>& left_out,
                 std::span<float>& right_out);

  static void apply_gain(std::span<float>& left, std::span<float>& right, const float& gain);

 private:
  uint node_id = 0U;

  float input_peak_left = util::minimum_linear_level, input_peak_right = util::minimum_linear_level;
  float output_peak_left = util::minimum_linear_level, output_peak_right = util::minimum_linear_level;
};

#endif
