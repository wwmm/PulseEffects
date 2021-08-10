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

#include "multiband_gate.hpp"

MultibandGate::MultibandGate(const std::string& tag,
                             const std::string& schema,
                             const std::string& schema_path,
                             PipeManager* pipe_manager)
    : PluginBase(tag, plugin_name::multiband_gate, schema, schema_path, pipe_manager),
      lv2_wrapper(std::make_unique<lv2::Lv2Wrapper>("http://calf.sourceforge.net/plugins/MultibandGate")) {
  if (!lv2_wrapper->found_plugin) {
    util::warning(log_tag + "http://calf.sourceforge.net/plugins/MultibandGate is not installed");
  }

  input_gain = static_cast<float>(util::db_to_linear(settings->get_double("input-gain")));
  output_gain = static_cast<float>(util::db_to_linear(settings->get_double("output-gain")));

  settings->signal_changed("input-gain").connect([=, this](auto key) {
    input_gain = util::db_to_linear(settings->get_double(key));
  });

  settings->signal_changed("output-gain").connect([=, this](auto key) {
    output_gain = util::db_to_linear(settings->get_double(key));
  });

  lv2_wrapper->bind_key_enum(settings, "mode", "mode");

  lv2_wrapper->bind_key_double(settings, "freq0", "freq0");

  lv2_wrapper->bind_key_double(settings, "freq1", "freq1");

  lv2_wrapper->bind_key_double(settings, "freq2", "freq2");

  // sub band

  lv2_wrapper->bind_key_double_db(settings, "range0", "range0");

  lv2_wrapper->bind_key_double_db(settings, "threshold0", "threshold0");

  lv2_wrapper->bind_key_double_db(settings, "makeup0", "makeup0");

  lv2_wrapper->bind_key_double_db(settings, "knee0", "knee0");

  lv2_wrapper->bind_key_double(settings, "ratio0", "ratio0");

  lv2_wrapper->bind_key_double(settings, "attack0", "attack0");

  lv2_wrapper->bind_key_double(settings, "release0", "release0");

  lv2_wrapper->bind_key_enum(settings, "detection0", "detection0");

  lv2_wrapper->bind_key_bool(settings, "bypass0", "bypass0");

  lv2_wrapper->bind_key_bool(settings, "solo0", "solo0");

  // low band

  lv2_wrapper->bind_key_double_db(settings, "range1", "range1");

  lv2_wrapper->bind_key_double_db(settings, "threshold1", "threshold1");

  lv2_wrapper->bind_key_double_db(settings, "makeup1", "makeup1");

  lv2_wrapper->bind_key_double_db(settings, "knee1", "knee1");

  lv2_wrapper->bind_key_double(settings, "ratio1", "ratio1");

  lv2_wrapper->bind_key_double(settings, "attack1", "attack1");

  lv2_wrapper->bind_key_double(settings, "release1", "release1");

  lv2_wrapper->bind_key_enum(settings, "detection1", "detection1");

  lv2_wrapper->bind_key_bool(settings, "bypass1", "bypass1");

  lv2_wrapper->bind_key_bool(settings, "solo1", "solo1");

  // mid band

  lv2_wrapper->bind_key_double_db(settings, "range2", "range2");

  lv2_wrapper->bind_key_double_db(settings, "threshold2", "threshold2");

  lv2_wrapper->bind_key_double_db(settings, "makeup2", "makeup2");

  lv2_wrapper->bind_key_double_db(settings, "knee2", "knee2");

  lv2_wrapper->bind_key_double(settings, "ratio2", "ratio2");

  lv2_wrapper->bind_key_double(settings, "attack2", "attack2");

  lv2_wrapper->bind_key_double(settings, "release2", "release2");

  lv2_wrapper->bind_key_enum(settings, "detection2", "detection2");

  lv2_wrapper->bind_key_bool(settings, "bypass2", "bypass2");

  lv2_wrapper->bind_key_bool(settings, "solo2", "solo2");

  // high band

  lv2_wrapper->bind_key_double_db(settings, "range3", "range3");

  lv2_wrapper->bind_key_double_db(settings, "threshold3", "threshold3");

  lv2_wrapper->bind_key_double_db(settings, "makeup3", "makeup3");

  lv2_wrapper->bind_key_double_db(settings, "knee3", "knee3");

  lv2_wrapper->bind_key_double(settings, "ratio3", "ratio3");

  lv2_wrapper->bind_key_double(settings, "attack3", "attack3");

  lv2_wrapper->bind_key_double(settings, "release3", "release3");

  lv2_wrapper->bind_key_enum(settings, "detection3", "detection3");

  lv2_wrapper->bind_key_bool(settings, "bypass3", "bypass3");

  lv2_wrapper->bind_key_bool(settings, "solo3", "solo3");
}

MultibandGate::~MultibandGate() {
  if (connected_to_pw) {
    disconnect_from_pw();
  }

  util::debug(log_tag + name + " destroyed");
}

void MultibandGate::setup() {
  if (!lv2_wrapper->found_plugin) {
    return;
  }

  lv2_wrapper->set_n_samples(n_samples);
  lv2_wrapper->create_instance(rate);
}

void MultibandGate::process(std::span<float>& left_in,
                            std::span<float>& right_in,
                            std::span<float>& left_out,
                            std::span<float>& right_out) {
  if (!lv2_wrapper->found_plugin || !lv2_wrapper->has_instance() || bypass) {
    std::copy(left_in.begin(), left_in.end(), left_out.begin());
    std::copy(right_in.begin(), right_in.end(), right_out.begin());

    return;
  }

  apply_gain(left_in, right_in, input_gain);

  lv2_wrapper->connect_data_ports(left_in, right_in, left_out, right_out);
  lv2_wrapper->run();

  apply_gain(left_out, right_out, output_gain);

  if (post_messages) {
    get_peaks(left_in, right_in, left_out, right_out);

    notification_dt += sample_duration;

    if (notification_dt >= notification_time_window) {
      float output0_value = lv2_wrapper->get_control_port_value("output0");
      float output1_value = lv2_wrapper->get_control_port_value("output1");
      float output2_value = lv2_wrapper->get_control_port_value("output2");
      float output3_value = lv2_wrapper->get_control_port_value("output3");

      float gating0_value = lv2_wrapper->get_control_port_value("gating0");
      float gating1_value = lv2_wrapper->get_control_port_value("gating1");
      float gating2_value = lv2_wrapper->get_control_port_value("gating2");
      float gating3_value = lv2_wrapper->get_control_port_value("gating3");

      Glib::signal_idle().connect_once([=, this] {
        output0.emit(output0_value);
        output1.emit(output1_value);
        output2.emit(output2_value);
        output3.emit(output3_value);

        gating0.emit(gating0_value);
        gating1.emit(gating1_value);
        gating2.emit(gating2_value);
        gating3.emit(gating3_value);
      });

      notify();

      notification_dt = 0.0F;
    }
  }
}
