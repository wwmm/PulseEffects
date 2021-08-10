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

#ifndef STREAM_OUTPUT_EFFECTS_HPP
#define STREAM_OUTPUT_EFFECTS_HPP

#include <algorithm>
#include <ranges>
#include "effects_base.hpp"

class StreamOutputEffects : public EffectsBase {
 public:
  StreamOutputEffects(PipeManager* pipe_manager);
  StreamOutputEffects(const StreamOutputEffects&) = delete;
  auto operator=(const StreamOutputEffects&) -> StreamOutputEffects& = delete;
  StreamOutputEffects(const StreamOutputEffects&&) = delete;
  auto operator=(const StreamOutputEffects&&) -> StreamOutputEffects& = delete;
  ~StreamOutputEffects() override;

  void set_bypass(const bool& state);

 private:
  bool bypass = false;

  void connect_filters();

  void disconnect_filters();

  /*
    Do not pass nd_info by reference. Sometimes it dies before we use it and a segmentation fault happens
  */

  void on_app_added(const NodeInfo& node_info);

  void on_link_changed(const LinkInfo& link_info);
};

#endif
