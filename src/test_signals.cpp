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

#include "test_signals.hpp"
#include <pipewire/filter.h>
#include <pipewire/keys.h>
#include <pipewire/port.h>
#include <pipewire/properties.h>
#include <spa/node/io.h>
#include <spa/utils/hook.h>
#include <sys/types.h>
#include <chrono>
#include <cmath>
#include <numbers>
#include <span>
#include <thread>
#include "pw_manager.hpp"
#include "tags_app.hpp"
#include "util.hpp"

namespace {

constexpr auto pi_x_2 = 2.0F * std::numbers::pi_v<float>;

void on_process(void* userdata, spa_io_position* position) {
  auto* d = static_cast<TestSignals::data*>(userdata);

  const auto n_samples = position->clock.duration;
  const auto rate = position->clock.rate.denom;

  if (n_samples == 0 || rate == 0) {
    return;
  }

  if (rate != d->ts->rate || n_samples != d->ts->n_samples) {
    d->ts->rate = rate;
    d->ts->n_samples = n_samples;

    d->ts->sine_phase = 0.0F;
  }

  // util::warning("processing: " + util::to_string(n_samples));

  auto* out_left = static_cast<float*>(pw_filter_get_dsp_buffer(d->out_left, n_samples));
  auto* out_right = static_cast<float*>(pw_filter_get_dsp_buffer(d->out_right, n_samples));

  if (out_left == nullptr || out_right == nullptr) {
    return;
  }

  std::span left_out(out_left, n_samples);
  std::span right_out(out_right, n_samples);

  const auto phase_delta = pi_x_2 * d->ts->sine_frequency / static_cast<float>(rate);

  for (uint n = 0U; n < n_samples; n++) {
    float signal = 0.0F;
    left_out[n] = 0.0F;
    right_out[n] = 0.0F;

    switch (d->ts->signal_type) {
      case TestSignalType::sine_wave: {
        d->ts->sine_phase += phase_delta;

        if (d->ts->sine_phase >= pi_x_2) {
          d->ts->sine_phase -= pi_x_2;
        }

        signal = 0.5F * std::sin(d->ts->sine_phase);

        break;
      }
      case TestSignalType::gaussian: {
        signal = d->ts->white_noise();

        break;
      }
      case TestSignalType::pink: {
        break;
      }
    }

    if (d->ts->create_left_channel) {
      left_out[n] = signal;
    }

    if (d->ts->create_right_channel) {
      right_out[n] = signal;
    }
  }
}

void on_filter_state_changed(void* userdata,
                             [[maybe_unused]] pw_filter_state old,
                             pw_filter_state state,
                             [[maybe_unused]] const char* error) {
  auto* d = static_cast<TestSignals::data*>(userdata);

  d->ts->state = state;

  switch (state) {
    case PW_FILTER_STATE_ERROR:
      d->ts->can_get_node_id = false;
      break;
    case PW_FILTER_STATE_UNCONNECTED:
      d->ts->can_get_node_id = false;
      break;
    case PW_FILTER_STATE_CONNECTING:
      d->ts->can_get_node_id = false;
      break;
    case PW_FILTER_STATE_STREAMING:
      d->ts->can_get_node_id = true;
      break;
    case PW_FILTER_STATE_PAUSED:
      d->ts->can_get_node_id = true;
      break;
    default:
      break;
  }
}

const struct pw_filter_events filter_events = {.state_changed = on_filter_state_changed, .process = on_process};

}  // namespace

TestSignals::TestSignals(pw::Manager* pipe_manager) : pm(pipe_manager), random_generator(rd()) {
  pf_data.ts = this;

  const auto* filter_name = "ee_test_signals";

  pm->lock();

  auto* props_filter = pw_properties_new(nullptr, nullptr);

  pw_properties_set(props_filter, PW_KEY_APP_ID, tags::app::id);
  pw_properties_set(props_filter, PW_KEY_NODE_NAME, filter_name);
  pw_properties_set(props_filter, PW_KEY_NODE_DESCRIPTION, "Easy Effects Filter");
  pw_properties_set(props_filter, PW_KEY_NODE_DRIVER, "true");
  pw_properties_set(props_filter, PW_KEY_MEDIA_TYPE, "Audio");
  pw_properties_set(props_filter, PW_KEY_MEDIA_CATEGORY, "Source");
  pw_properties_set(props_filter, PW_KEY_MEDIA_ROLE, "DSP");
  // pw_properties_set(props_filter, PW_KEY_MEDIA_CLASS, tags::pipewire::media_class::output_stream);

  filter = pw_filter_new(pm->core, filter_name, props_filter);

  // left channel output

  auto* props_out_left = pw_properties_new(nullptr, nullptr);

  pw_properties_set(props_out_left, PW_KEY_FORMAT_DSP, "32 bit float mono audio");
  pw_properties_set(props_out_left, PW_KEY_PORT_NAME, "output_FL");
  pw_properties_set(props_out_left, "audio.channel", "FL");

  pf_data.out_left = static_cast<port*>(pw_filter_add_port(filter, PW_DIRECTION_OUTPUT, PW_FILTER_PORT_FLAG_MAP_BUFFERS,
                                                           sizeof(port), props_out_left, nullptr, 0));

  // right channel output

  auto* props_out_right = pw_properties_new(nullptr, nullptr);

  pw_properties_set(props_out_right, PW_KEY_FORMAT_DSP, "32 bit float mono audio");
  pw_properties_set(props_out_right, PW_KEY_PORT_NAME, "output_FR");
  pw_properties_set(props_out_right, "audio.channel", "FR");

  pf_data.out_right = static_cast<port*>(pw_filter_add_port(
      filter, PW_DIRECTION_OUTPUT, PW_FILTER_PORT_FLAG_MAP_BUFFERS, sizeof(port), props_out_right, nullptr, 0));

  if (pw_filter_connect(filter, PW_FILTER_FLAG_RT_PROCESS, nullptr, 0) != 0) {
    using namespace std::string_literals;

    pm->unlock();

    util::warning(filter_name + " cannot connect the filter to PipeWire!"s);

    return;
  }

  pw_filter_add_listener(filter, &listener, &filter_events, &pf_data);

  pm->sync_wait_unlock();

  while (!can_get_node_id) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    if (state == PW_FILTER_STATE_ERROR) {
      using namespace std::string_literals;

      util::warning(filter_name + " is in an error"s);

      return;
    }
  }

  pm->lock();

  node_id = pw_filter_get_node_id(filter);

  pm->sync_wait_unlock();
}

TestSignals::~TestSignals() {
  util::debug("destroyed");

  pm->lock();

  spa_hook_remove(&listener);

  pw_filter_set_active(filter, false);

  pw_filter_disconnect(filter);

  pw_filter_destroy(filter);

  pm->sync_wait_unlock();
}

void TestSignals::set_state(const bool& state) {
  sine_phase = 0.0F;

  if (state) {
    for (const auto& link : pm->link_nodes(node_id, pm->ee_sink_node.id, false, false)) {
      list_proxies.push_back(link);
    }
  } else {
    pm->destroy_links(list_proxies);

    list_proxies.clear();
  }
}

void TestSignals::set_frequency(const float& value) {
  sine_frequency = value;

  sine_phase = 0.0F;
}

auto TestSignals::white_noise() -> float {
  const auto v = normal_distribution(random_generator);

  return (v > 1.0F) ? 1.0F : ((v < -1.0F) ? -1.0F : v);
}
