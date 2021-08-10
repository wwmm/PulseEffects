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

#include "equalizer.hpp"

Equalizer::Equalizer(const std::string& tag,
                     const std::string& schema,
                     const std::string& schema_path,
                     const std::string& schema_channel,
                     const std::string& schema_channel_left_path,
                     const std::string& schema_channel_right_path,
                     PipeManager* pipe_manager)
    : PluginBase(tag, plugin_name::equalizer, schema, schema_path, pipe_manager),
      settings_left(Gio::Settings::create(schema_channel, schema_channel_left_path)),
      settings_right(Gio::Settings::create(schema_channel, schema_channel_right_path)),
      lv2_wrapper(std::make_unique<lv2::Lv2Wrapper>("http://lsp-plug.in/plugins/lv2/para_equalizer_x32_lr")) {
  if (!lv2_wrapper->found_plugin) {
    util::warning(log_tag + "http://lsp-plug.in/plugins/lv2/para_equalizer_x32_lr is not installed");
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

  for (uint n = 0U; n < max_bands; n++) {
    bind_band(n);
  }

  settings->signal_changed("num-bands").connect([=, this](auto key) {
    uint nbands = settings->get_int(key);

    for (uint n = 0U; n < max_bands; n++) {
      if (n < nbands) {
        settings_left->set_enum("band" + std::to_string(n) + "-type", 1);
        settings_right->set_enum("band" + std::to_string(n) + "-type", 1);
      }

      // turn off unused bands

      if (n >= nbands) {
        settings_left->set_enum("band" + std::to_string(n) + "-type", 0);
        settings_right->set_enum("band" + std::to_string(n) + "-type", 0);
      }
    }
  });

  settings->signal_changed("split-channels").connect([=, this](auto key) {
    if (settings->get_boolean(key) == true) {
      return;
    }

    for (uint n = 0U; n < max_bands; n++) {
      settings_right->set_enum("band" + std::to_string(n) + "-type",
                               settings_left->get_enum("band" + std::to_string(n) + "-type"));

      settings_right->set_enum("band" + std::to_string(n) + "-mode",
                               settings_left->get_enum("band" + std::to_string(n) + "-mode"));

      settings_right->set_enum("band" + std::to_string(n) + "-slope",
                               settings_left->get_enum("band" + std::to_string(n) + "-slope"));

      settings_right->set_boolean("band" + std::to_string(n) + "-solo",
                                  settings_left->get_boolean("band" + std::to_string(n) + "-solo"));

      settings_right->set_boolean("band" + std::to_string(n) + "-mute",
                                  settings_left->get_boolean("band" + std::to_string(n) + "-mute"));

      settings_right->set_double("band" + std::to_string(n) + "-frequency",
                                 settings_left->get_double("band" + std::to_string(n) + "-frequency"));

      settings_right->set_double("band" + std::to_string(n) + "-gain",
                                 settings_left->get_double("band" + std::to_string(n) + "-gain"));

      settings_right->set_double("band" + std::to_string(n) + "-q",
                                 settings_left->get_double("band" + std::to_string(n) + "-q"));
    }
  });

  initialize_listener();
}

Equalizer::~Equalizer() {
  util::debug(log_tag + name + " destroyed");

  pw_thread_loop_lock(pm->thread_loop);

  pw_filter_set_active(filter, false);

  pw_filter_disconnect(filter);

  pw_core_sync(pm->core, PW_ID_CORE, 0);

  pw_thread_loop_wait(pm->thread_loop);

  pw_thread_loop_unlock(pm->thread_loop);
}

void Equalizer::setup() {
  if (!lv2_wrapper->found_plugin) {
    return;
  }

  lv2_wrapper->set_n_samples(n_samples);
  lv2_wrapper->create_instance(rate);
}

void Equalizer::process(std::span<float>& left_in,
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

  /*
    This plugin gives the latency in number of samples
  */

  uint lv = static_cast<uint>(lv2_wrapper->get_control_port_value("out_latency"));

  if (latency_n_frames != lv) {
    latency_n_frames = lv;

    float latency_value = static_cast<float>(latency_n_frames) / static_cast<float>(rate);

    util::debug(log_tag + name + " latency: " + std::to_string(latency_value) + " s");

    Glib::signal_idle().connect_once([=, this] { latency.emit(latency_value); });

    spa_process_latency_info latency_info{};

    latency_info.ns = static_cast<uint64_t>(latency_value * 1000000000.0F);

    std::array<char, 1024> buffer{};

    spa_pod_builder b{};

    spa_pod_builder_init(&b, buffer.data(), sizeof(buffer));

    const spa_pod* param = spa_process_latency_build(&b, SPA_PARAM_ProcessLatency, &latency_info);

    pw_filter_update_params(filter, nullptr, &param, 1);
  }

  if (post_messages) {
    get_peaks(left_in, right_in, left_out, right_out);

    notification_dt += sample_duration;

    if (notification_dt >= notification_time_window) {
      notify();

      notification_dt = 0.0F;
    }
  }
}

void Equalizer::bind_band(const int& index) {
  // left channel

  lv2_wrapper->bind_key_enum(settings_left, "band" + std::to_string(index) + "-type", "ftl_" + std::to_string(index));

  lv2_wrapper->bind_key_enum(settings_left, "band" + std::to_string(index) + "-mode", "fml_" + std::to_string(index));

  lv2_wrapper->bind_key_enum(settings_left, "band" + std::to_string(index) + "-slope", "sl_" + std::to_string(index));

  lv2_wrapper->bind_key_bool(settings_left, "band" + std::to_string(index) + "-solo", "xsl_" + std::to_string(index));

  lv2_wrapper->bind_key_bool(settings_left, "band" + std::to_string(index) + "-mute", "xml_" + std::to_string(index));

  lv2_wrapper->bind_key_double(settings_left, "band" + std::to_string(index) + "-frequency",
                               "fl_" + std::to_string(index));

  lv2_wrapper->bind_key_double(settings_left, "band" + std::to_string(index) + "-q", "ql_" + std::to_string(index));

  lv2_wrapper->bind_key_double_db(settings_left, "band" + std::to_string(index) + "-gain",
                                  "gl_" + std::to_string(index));

  // right channel

  lv2_wrapper->bind_key_enum(settings_right, "band" + std::to_string(index) + "-type", "ftr_" + std::to_string(index));

  lv2_wrapper->bind_key_enum(settings_right, "band" + std::to_string(index) + "-mode", "fmr_" + std::to_string(index));

  lv2_wrapper->bind_key_enum(settings_right, "band" + std::to_string(index) + "-slope", "sr_" + std::to_string(index));

  lv2_wrapper->bind_key_bool(settings_right, "band" + std::to_string(index) + "-solo", "xsr_" + std::to_string(index));

  lv2_wrapper->bind_key_bool(settings_right, "band" + std::to_string(index) + "-mute", "xmr_" + std::to_string(index));

  lv2_wrapper->bind_key_double(settings_right, "band" + std::to_string(index) + "-frequency",
                               "fr_" + std::to_string(index));

  lv2_wrapper->bind_key_double(settings_right, "band" + std::to_string(index) + "-q", "qr_" + std::to_string(index));

  lv2_wrapper->bind_key_double_db(settings_right, "band" + std::to_string(index) + "-gain",
                                  "gr_" + std::to_string(index));
}
