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

#include "compressor.hpp"
#include <sys/types.h>
#include <algorithm>
#include <memory>
#include <span>
#include <string>
#include "db_manager.hpp"
#include "easyeffects_db_compressor.h"
#include "lv2_macros.hpp"
#include "lv2_wrapper.hpp"
#include "plugin_base.hpp"
#include "pw_manager.hpp"
#include "pw_objects.hpp"
#include "tags_plugin_name.hpp"
#include "util.hpp"

Compressor::Compressor(const std::string& tag, pw::Manager* pipe_manager, PipelineType pipe_type, QString instance_id)
    : PluginBase(tag,
                 tags::plugin_name::BaseName::compressor,
                 tags::plugin_package::Package::lsp,
                 instance_id,
                 pipe_manager,
                 pipe_type,
                 true),
      settings(db::Manager::self().get_plugin_db<db::Compressor>(
          pipe_type,
          tags::plugin_name::BaseName::compressor + "#" + instance_id)) {
  lv2_wrapper = std::make_unique<lv2::Lv2Wrapper>("http://lsp-plug.in/plugins/lv2/sc_compressor_stereo");

  package_installed = lv2_wrapper->found_plugin;

  if (!package_installed) {
    util::debug(log_tag + "http://lsp-plug.in/plugins/lv2/sc_compressor_stereo is not installed");
  }

  init_common_controls<db::Compressor>(settings);

  // specific plugin controls

  connect(settings, &db::Compressor::sidechainTypeChanged, [&]() { update_sidechain_links(); });
  connect(settings, &db::Compressor::sidechainInputDeviceChanged, [&]() { update_sidechain_links(); });

  BIND_LV2_PORT("cm", mode, setMode, db::Compressor::modeChanged);
  BIND_LV2_PORT("sct", sidechainType, setSidechainType, db::Compressor::sidechainTypeChanged);
  BIND_LV2_PORT("scm", sidechainMode, setSidechainMode, db::Compressor::sidechainModeChanged);
  BIND_LV2_PORT("scl", sidechainListen, setSidechainListen, db::Compressor::sidechainListenChanged);
  BIND_LV2_PORT("scs", sidechainSource, setSidechainSource, db::Compressor::sidechainSourceChanged);
  BIND_LV2_PORT("sscs", stereoSplitSource, setStereoSplitSource, db::Compressor::stereoSplitSourceChanged);
  BIND_LV2_PORT("ssplit", stereoSplit, setStereoSplit, db::Compressor::stereoSplitChanged);

  // lv2_wrapper->bind_key_enum<"shpm", "hpf-mode">(settings);

  // lv2_wrapper->bind_key_enum<"slpm", "lpf-mode">(settings);

  // lv2_wrapper->bind_key_double<"at", "attack">(settings);

  // lv2_wrapper->bind_key_double<"rt", "release">(settings);

  // lv2_wrapper->bind_key_double<"cr", "ratio">(settings);

  // lv2_wrapper->bind_key_double<"scr", "sidechain-reactivity">(settings);

  // lv2_wrapper->bind_key_double<"sla", "sidechain-lookahead">(settings);

  // lv2_wrapper->bind_key_double<"shpf", "hpf-frequency">(settings);

  // lv2_wrapper->bind_key_double<"slpf", "lpf-frequency">(settings);

  // lv2_wrapper->bind_key_double_db<"bth", "boost-threshold">(settings);

  // lv2_wrapper->bind_key_double_db<"bsa", "boost-amount">(settings);

  // lv2_wrapper->bind_key_double_db<"al", "threshold">(settings);

  // lv2_wrapper->bind_key_double_db<"kn", "knee">(settings);

  // lv2_wrapper->bind_key_double_db<"mk", "makeup">(settings);

  // lv2_wrapper->bind_key_double_db<"scp", "sidechain-preamp">(settings);

  // The following controls can assume -inf
  // lv2_wrapper->bind_key_double_db<"rrl", "release-threshold", false>(settings);

  // lv2_wrapper->bind_key_double_db<"cdr", "dry", false>(settings);

  // lv2_wrapper->bind_key_double_db<"cwt", "wet", false>(settings);
}

Compressor::~Compressor() {
  if (connected_to_pw) {
    disconnect_from_pw();
  }

  util::debug(log_tag + name.toStdString() + " destroyed");
}

void Compressor::reset() {
  settings->setDefaults();
}

void Compressor::setup() {
  if (!lv2_wrapper->found_plugin) {
    return;
  }

  lv2_wrapper->set_n_samples(n_samples);

  if (lv2_wrapper->get_rate() != rate) {
    lv2_wrapper->create_instance(rate);
  }
}

void Compressor::process([[maybe_unused]] std::span<float>& left_in,
                         [[maybe_unused]] std::span<float>& right_in,
                         [[maybe_unused]] std::span<float>& left_out,
                         [[maybe_unused]] std::span<float>& right_out) {}

void Compressor::process(std::span<float>& left_in,
                         std::span<float>& right_in,
                         std::span<float>& left_out,
                         std::span<float>& right_out,
                         std::span<float>& probe_left,
                         std::span<float>& probe_right) {
  if (!lv2_wrapper->found_plugin || !lv2_wrapper->has_instance() || bypass) {
    std::copy(left_in.begin(), left_in.end(), left_out.begin());
    std::copy(right_in.begin(), right_in.end(), right_out.begin());

    return;
  }

  if (input_gain != 1.0F) {
    apply_gain(left_in, right_in, input_gain);
  }

  lv2_wrapper->connect_data_ports(left_in, right_in, left_out, right_out, probe_left, probe_right);
  lv2_wrapper->run();

  if (output_gain != 1.0F) {
    apply_gain(left_out, right_out, output_gain);
  }

  /*
   This plugin gives the latency in number of samples
 */

  const auto lv = static_cast<uint>(lv2_wrapper->get_control_port_value("out_latency"));

  if (latency_n_frames != lv) {
    latency_n_frames = lv;

    latency_value = static_cast<float>(latency_n_frames) / static_cast<float>(rate);

    util::debug(log_tag + name.toStdString() + " latency: " + util::to_string(latency_value, "") + " s");

    update_filter_params();
  }

  get_peaks(left_in, right_in, left_out, right_out);

  reduction_left = util::linear_to_db(lv2_wrapper->get_control_port_value("rlm_l"));
  reduction_right = util::linear_to_db(lv2_wrapper->get_control_port_value("rlm_r"));

  sidechain_left = util::linear_to_db(lv2_wrapper->get_control_port_value("slm_l"));
  sidechain_right = util::linear_to_db(lv2_wrapper->get_control_port_value("slm_r"));

  curve_left = util::linear_to_db(lv2_wrapper->get_control_port_value("clm_l"));
  curve_right = util::linear_to_db(lv2_wrapper->get_control_port_value("clm_r"));

  envelope_left = util::linear_to_db(lv2_wrapper->get_control_port_value("elm_l"));
  envelope_right = util::linear_to_db(lv2_wrapper->get_control_port_value("elm_r"));
}

void Compressor::update_sidechain_links() {
  if (settings->sidechainType() != db::Compressor::EnumSidechainType::type::external) {
    pm->destroy_links(list_proxies);

    list_proxies.clear();

    return;
  }

  const auto device_name = settings->sidechainInputDevice();

  auto input_device = pm->ee_source_node;

  for (const auto& [serial, node] : pm->node_map) {
    if (node.name == device_name) {
      input_device = node;

      break;
    }
  }

  pm->destroy_links(list_proxies);

  list_proxies.clear();

  for (const auto& link : pm->link_nodes(input_device.id, get_node_id(), true)) {
    list_proxies.push_back(link);
  }
}

void Compressor::update_probe_links() {
  update_sidechain_links();
}

auto Compressor::get_latency_seconds() -> float {
  return this->latency_value;
}

float Compressor::getReductionLevelLeft() const {
  return reduction_left;
}

float Compressor::getReductionLevelRight() const {
  return reduction_right;
}

float Compressor::getSideChainLevelLeft() const {
  return sidechain_left;
}

float Compressor::getSideChainLevelRight() const {
  return sidechain_right;
}

float Compressor::getCurveLevelLeft() const {
  return curve_left;
}

float Compressor::getCurveLevelRight() const {
  return curve_right;
}

float Compressor::getEnvelopeLevelLeft() const {
  return envelope_left;
}

float Compressor::getEnvelopeLevelRight() const {
  return envelope_right;
}