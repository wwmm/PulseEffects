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

#pragma once

#include <adwaita.h>
#include <glib/gi18n.h>
#include "resources_tags.hpp"
#include "tags_multiband_compressor.hpp"
#include "ui_helpers.hpp"

namespace ui::multiband_compressor_band_box {

G_BEGIN_DECLS

#define EE_TYPE_MULTIBAND_COMPRESSOR_BAND_BOX (multiband_compressor_band_box_get_type())

G_DECLARE_FINAL_TYPE(MultibandCompressorBandBox,
                     multiband_compressor_band_box,
                     EE,
                     MULTIBAND_COMPRESSOR_BAND_BOX,
                     GtkBox)

G_END_DECLS

auto create() -> MultibandCompressorBandBox*;

void setup(MultibandCompressorBandBox* self, GSettings* settings, int index);

void set_end_label(MultibandCompressorBandBox* self, const float& value);

void set_envelope_label(MultibandCompressorBandBox* self, const float& value);

void set_curve_label(MultibandCompressorBandBox* self, const float& value);

void set_gain_label(MultibandCompressorBandBox* self, const float& value);

}  // namespace ui::multiband_compressor_band_box
