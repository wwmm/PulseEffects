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

#include "crystalizer.hpp"

Crystalizer::Crystalizer(const std::string& tag,
                         const std::string& schema,
                         const std::string& schema_path,
                         PipeManager* pipe_manager)
    : PluginBase(tag, plugin_name::crystalizer, schema, schema_path, pipe_manager) {
  for (uint n = 0U; n < nbands; n++) {
    auto nstr = std::to_string(n);

    if (n == 0U) {
      filters.at(n) = std::make_unique<FirFilterLowpass>(log_tag + name + " band" + nstr);
    } else if (n == nbands - 1U) {
      filters.at(n) = std::make_unique<FirFilterHighpass>(log_tag + name + " band" + nstr);
    } else {
      filters.at(n) = std::make_unique<FirFilterBandpass>(log_tag + name + " band" + nstr);
    }
  }

  std::ranges::fill(band_mute, false);
  std::ranges::fill(band_bypass, false);
  std::ranges::fill(band_intensity, 1.0F);
  std::ranges::fill(band_last_L, 0.0F);
  std::ranges::fill(band_last_R, 0.0F);

  frequencies[0] = 500.0F;
  frequencies[1] = 1000.0F;
  frequencies[2] = 2000.0F;
  frequencies[3] = 3000.0F;
  frequencies[4] = 4000.0F;
  frequencies[5] = 5000.0F;
  frequencies[6] = 6000.0F;
  frequencies[7] = 7000.0F;
  frequencies[8] = 8000.0F;
  frequencies[9] = 9000.0F;
  frequencies[10] = 10000.0F;
  frequencies[11] = 15000.0F;

  input_gain = static_cast<float>(util::db_to_linear(settings->get_double("input-gain")));
  output_gain = static_cast<float>(util::db_to_linear(settings->get_double("output-gain")));

  settings->signal_changed("input-gain").connect([=, this](auto key) {
    input_gain = util::db_to_linear(settings->get_double(key));
  });

  settings->signal_changed("output-gain").connect([=, this](auto key) {
    output_gain = util::db_to_linear(settings->get_double(key));
  });

  for (uint n = 0U; n < nbands; n++) {
    bind_band(static_cast<int>(n));
  }
}

Crystalizer::~Crystalizer() {
  if (connected_to_pw) {
    disconnect_from_pw();
  }

  data_mutex.lock();

  filters_are_ready = false;

  data_mutex.unlock();

  util::debug(log_tag + name + " destroyed");
}

void Crystalizer::setup() {
  filters_are_ready = false;

  /*
    As zita uses fftw we have to be careful when reinitializing it. The thread that creates the fftw plan has to be the
    same that destroys it. Otherwise segmentation faults can happen. As we do not want to do this initializing in the
    plugin realtime thread we send it to the main thread through Glib::signal_idle().connect_once
  */

  Glib::signal_idle().connect_once([&, this] {
    blocksize = n_samples;

    n_samples_is_power_of_2 = (n_samples & (n_samples - 1)) == 0 && n_samples != 0;

    if (!n_samples_is_power_of_2) {
      while ((blocksize & (blocksize - 1)) != 0 && blocksize > 2) {
        blocksize--;
      }
    }

    notify_latency = true;
    do_first_rotation = true;

    latency_n_frames = 1U;  // the second derivative forces us to delay at least one sample

    deque_out_L.resize(0);
    deque_out_R.resize(0);

    data_L.resize(0);
    data_R.resize(0);

    for (uint n = 0U; n < nbands; n++) {
      band_data_L.at(n).resize(blocksize);
      band_data_R.at(n).resize(blocksize);

      band_second_derivative_L.at(n).resize(blocksize);
      band_second_derivative_R.at(n).resize(blocksize);
    }

    /*
      Bandpass transition band has to be twice the value used for lowpass and
      highpass. This way all filters will have the same delay.
    */

    float transition_band = 100.0F;  // Hz

    for (uint n = 0U; n < nbands; n++) {
      filters.at(n)->set_n_samples(blocksize);
      filters.at(n)->set_rate(rate);

      if (n == 0U) {
        filters.at(n)->set_max_frequency(frequencies[0]);
        filters.at(n)->set_transition_band(transition_band);
      } else if (n == nbands - 1U) {
        filters.at(n)->set_min_frequency(frequencies.at(n - 1U));  // frequencies array size = nbands - 1
        filters.at(n)->set_transition_band(transition_band);
      } else {
        filters.at(n)->set_min_frequency(frequencies.at(n - 1U));
        filters.at(n)->set_max_frequency(frequencies.at(n));
        filters.at(n)->set_transition_band(2.0F * transition_band);
      }

      filters.at(n)->setup();
    }

    std::scoped_lock<std::mutex> lock(data_mutex);

    filters_are_ready = true;
  });
}

void Crystalizer::process(std::span<float>& left_in,
                          std::span<float>& right_in,
                          std::span<float>& left_out,
                          std::span<float>& right_out) {
  std::scoped_lock<std::mutex> lock(data_mutex);

  if (bypass || !filters_are_ready) {
    std::copy(left_in.begin(), left_in.end(), left_out.begin());
    std::copy(right_in.begin(), right_in.end(), right_out.begin());

    return;
  }

  apply_gain(left_in, right_in, input_gain);

  if (n_samples_is_power_of_2) {
    std::copy(left_in.begin(), left_in.end(), left_out.begin());
    std::copy(right_in.begin(), right_in.end(), right_out.begin());

    enhance_peaks(left_out, right_out);
  } else {
    for (size_t j = 0; j < left_in.size(); j++) {
      data_L.emplace_back(left_in[j]);
      data_R.emplace_back(right_in[j]);

      if (data_L.size() == blocksize) {
        enhance_peaks(data_L, data_R);

        for (const auto& v : data_L) {
          deque_out_L.emplace_back(v);
        }

        for (const auto& v : data_R) {
          deque_out_R.emplace_back(v);
        }

        data_L.resize(0);
        data_R.resize(0);
      }
    }

    // copying the processed samples to the output buffers

    if (deque_out_L.size() >= left_out.size()) {
      for (float& v : left_out) {
        v = deque_out_L.front();

        deque_out_L.pop_front();
      }

      for (float& v : right_out) {
        v = deque_out_R.front();

        deque_out_R.pop_front();
      }
    } else {
      uint offset = 2 * (left_out.size() - deque_out_L.size());

      if (offset != latency_n_frames) {
        latency_n_frames = offset + 1U;  // the second derivative forces us to delay at least one sample

        notify_latency = true;
      }

      for (uint n = 0U; !deque_out_L.empty() && n < left_out.size(); n++) {
        if (n < offset) {
          left_out[n] = 0.0F;
          right_out[n] = 0.0F;
        } else {
          left_out[n] = deque_out_L.front();
          right_out[n] = deque_out_R.front();

          deque_out_R.pop_front();
          deque_out_L.pop_front();
        }
      }
    }
  }

  apply_gain(left_out, right_out, output_gain);

  if (notify_latency) {
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

    notify_latency = false;
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

void Crystalizer::bind_band(const int& n) {
  auto nstr = std::to_string(n);

  band_intensity.at(n) = static_cast<float>(util::db_to_linear(settings->get_double("intensity-band" + nstr)));
  band_mute.at(n) = settings->get_boolean("mute-band" + nstr);
  band_bypass.at(n) = settings->get_boolean("bypass-band" + nstr);

  settings->signal_changed("intensity-band" + nstr).connect([=, this](auto key) {
    band_intensity.at(n) = util::db_to_linear(settings->get_double(key));
  });

  settings->signal_changed("mute-band" + nstr).connect([=, this](auto key) {
    band_mute.at(n) = settings->get_boolean(key);
  });

  settings->signal_changed("bypass-band" + nstr).connect([=, this](auto key) {
    band_bypass.at(n) = settings->get_boolean(key);
  });
}
