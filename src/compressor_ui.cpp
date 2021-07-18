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

#include "compressor_ui.hpp"

namespace {

auto mode_enum_to_int(GValue* value, GVariant* variant, gpointer user_data) -> gboolean {
  const auto* v = g_variant_get_string(variant, nullptr);

  if (std::strcmp(v, "Downward") == 0) {
    g_value_set_int(value, 0);
  } else if (std::strcmp(v, "Upward") == 0) {
    g_value_set_int(value, 1);
  } else if (std::strcmp(v, "Boosting") == 0) {
    g_value_set_int(value, 2);
  }

  return 1;
}

auto int_to_mode_enum(const GValue* value, const GVariantType* expected_type, gpointer user_data) -> GVariant* {
  const auto v = g_value_get_int(value);

  switch (v) {
    case 0:
      return g_variant_new_string("Downward");

    case 1:
      return g_variant_new_string("Upward");

    case 2:
      return g_variant_new_string("Boosting");

    default:
      return g_variant_new_string("Downward");
  }
}

auto sidechain_type_enum_to_int(GValue* value, GVariant* variant, gpointer user_data) -> gboolean {
  const auto* v = g_variant_get_string(variant, nullptr);

  if (std::strcmp(v, "Feed-forward") == 0) {
    g_value_set_int(value, 0);
  } else if (std::strcmp(v, "Feed-back") == 0) {
    g_value_set_int(value, 1);
  } else if (std::strcmp(v, "External") == 0) {
    g_value_set_int(value, 2);
  }

  return 1;
}

auto int_to_sidechain_type_enum(const GValue* value, const GVariantType* expected_type, gpointer user_data)
    -> GVariant* {
  const auto v = g_value_get_int(value);

  switch (v) {
    case 0:
      return g_variant_new_string("Feed-forward");

    case 1:
      return g_variant_new_string("Feed-back");

    case 2:
      return g_variant_new_string("External");

    default:
      return g_variant_new_string("Feed-forward");
  }
}

auto sidechain_mode_enum_to_int(GValue* value, GVariant* variant, gpointer user_data) -> gboolean {
  const auto* v = g_variant_get_string(variant, nullptr);

  if (std::strcmp(v, "Peak") == 0) {
    g_value_set_int(value, 0);
  } else if (std::strcmp(v, "RMS") == 0) {
    g_value_set_int(value, 1);
  } else if (std::strcmp(v, "Low-Pass") == 0) {
    g_value_set_int(value, 2);
  } else if (std::strcmp(v, "Uniform") == 0) {
    g_value_set_int(value, 3);
  }

  return 1;
}

auto int_to_sidechain_mode_enum(const GValue* value, const GVariantType* expected_type, gpointer user_data)
    -> GVariant* {
  const auto v = g_value_get_int(value);

  switch (v) {
    case 0:
      return g_variant_new_string("Peak");

    case 1:
      return g_variant_new_string("RMS");

    case 2:
      return g_variant_new_string("Low-Pass");

    case 3:
      return g_variant_new_string("Uniform");

    default:
      return g_variant_new_string("RMS");
  }
}

auto sidechain_source_enum_to_int(GValue* value, GVariant* variant, gpointer user_data) -> gboolean {
  const auto* v = g_variant_get_string(variant, nullptr);

  if (std::strcmp(v, "Middle") == 0) {
    g_value_set_int(value, 0);
  } else if (std::strcmp(v, "Side") == 0) {
    g_value_set_int(value, 1);
  } else if (std::strcmp(v, "Left") == 0) {
    g_value_set_int(value, 2);
  } else if (std::strcmp(v, "Right") == 0) {
    g_value_set_int(value, 3);
  }

  return 1;
}

auto int_to_sidechain_source_enum(const GValue* value, const GVariantType* expected_type, gpointer user_data)
    -> GVariant* {
  const auto v = g_value_get_int(value);

  switch (v) {
    case 0:
      return g_variant_new_string("Middle");

    case 1:
      return g_variant_new_string("Side");

    case 2:
      return g_variant_new_string("Left");

    case 3:
      return g_variant_new_string("Right");

    default:
      return g_variant_new_string("Middle");
  }
}

auto filter_mode_enum_to_int(GValue* value, GVariant* variant, gpointer user_data) -> gboolean {
  const auto* v = g_variant_get_string(variant, nullptr);

  if (std::strcmp(v, "off") == 0) {
    g_value_set_int(value, 0);
  } else if (std::strcmp(v, "12 dB/oct") == 0) {
    g_value_set_int(value, 1);
  } else if (std::strcmp(v, "24 dB/oct") == 0) {
    g_value_set_int(value, 2);
  } else if (std::strcmp(v, "36 dB/oct") == 0) {
    g_value_set_int(value, 3);
  }

  return 1;
}

auto int_to_filter_mode_enum(const GValue* value, const GVariantType* expected_type, gpointer user_data) -> GVariant* {
  const auto v = g_value_get_int(value);

  switch (v) {
    case 0:
      return g_variant_new_string("off");

    case 1:
      return g_variant_new_string("12 dB/oct");

    case 2:
      return g_variant_new_string("24 dB/oct");

    case 3:
      return g_variant_new_string("36 dB/oct");

    default:
      return g_variant_new_string("off");
  }
}

}  // namespace

CompressorUi::CompressorUi(BaseObjectType* cobject,
                           const Glib::RefPtr<Gtk::Builder>& builder,
                           const std::string& schema,
                           const std::string& schema_path)
    : Gtk::Box(cobject),
      PluginUiBase(builder, schema, schema_path),
      input_devices_model(Gio::ListStore<NodeInfoHolder>::create()) {
  name = plugin_name::compressor;

  // loading builder widgets

  input_gain = builder->get_widget<Gtk::Scale>("input_gain");
  output_gain = builder->get_widget<Gtk::Scale>("output_gain");

  attack = builder->get_widget<Gtk::SpinButton>("attack");
  knee = builder->get_widget<Gtk::SpinButton>("knee");
  makeup = builder->get_widget<Gtk::SpinButton>("makeup");
  ratio = builder->get_widget<Gtk::SpinButton>("ratio");
  release = builder->get_widget<Gtk::SpinButton>("release");
  threshold = builder->get_widget<Gtk::SpinButton>("threshold");
  preamp = builder->get_widget<Gtk::SpinButton>("preamp");
  reactivity = builder->get_widget<Gtk::SpinButton>("reactivity");
  lookahead = builder->get_widget<Gtk::SpinButton>("lookahead");
  release_threshold = builder->get_widget<Gtk::SpinButton>("release_threshold");
  boost_threshold = builder->get_widget<Gtk::SpinButton>("boost_threshold");
  boost_amount = builder->get_widget<Gtk::SpinButton>("boost_amount");
  hpf_freq = builder->get_widget<Gtk::SpinButton>("hpf_freq");
  lpf_freq = builder->get_widget<Gtk::SpinButton>("lpf_freq");

  compression_mode = builder->get_widget<Gtk::ComboBoxText>("compression_mode");
  sidechain_type = builder->get_widget<Gtk::ComboBoxText>("sidechain_type");
  sidechain_mode = builder->get_widget<Gtk::ComboBoxText>("sidechain_mode");
  sidechain_source = builder->get_widget<Gtk::ComboBoxText>("sidechain_source");
  hpf_mode = builder->get_widget<Gtk::ComboBoxText>("hpf_mode");
  lpf_mode = builder->get_widget<Gtk::ComboBoxText>("lpf_mode");

  reduction = builder->get_widget<Gtk::LevelBar>("reduction");
  sidechain = builder->get_widget<Gtk::LevelBar>("sidechain");
  curve = builder->get_widget<Gtk::LevelBar>("curve");

  listen = builder->get_widget<Gtk::ToggleButton>("listen");

  reduction_label = builder->get_widget<Gtk::Label>("reduction_label");
  sidechain_label = builder->get_widget<Gtk::Label>("sidechain_label");
  curve_label = builder->get_widget<Gtk::Label>("curve_label");

  dropdown_input_devices = builder->get_widget<Gtk::DropDown>("dropdown_input_devices");

  dropdown_input_devices->property_selected_item().signal_changed().connect([=, this]() {
    if (dropdown_input_devices->get_selected_item() == nullptr) {
      return;
    }

    auto holder = std::dynamic_pointer_cast<NodeInfoHolder>(dropdown_input_devices->get_selected_item());

    settings->set_string("sidechain-input-device", holder->info.name);
  });

  setup_dropdown_input_devices();

  // gsettings bindings

  settings->bind("attack", attack->get_adjustment().get(), "value");
  settings->bind("knee", knee->get_adjustment().get(), "value");
  settings->bind("makeup", makeup->get_adjustment().get(), "value");
  settings->bind("ratio", ratio->get_adjustment().get(), "value");
  settings->bind("release", release->get_adjustment().get(), "value");
  settings->bind("threshold", threshold->get_adjustment().get(), "value");
  settings->bind("sidechain-listen", listen, "active");
  settings->bind("sidechain-preamp", preamp->get_adjustment().get(), "value");
  settings->bind("sidechain-reactivity", reactivity->get_adjustment().get(), "value");
  settings->bind("sidechain-lookahead", lookahead->get_adjustment().get(), "value");
  settings->bind("input-gain", input_gain->get_adjustment().get(), "value");
  settings->bind("output-gain", output_gain->get_adjustment().get(), "value");
  settings->bind("release-threshold", release_threshold->get_adjustment().get(), "value");
  settings->bind("boost-threshold", boost_threshold->get_adjustment().get(), "value");
  settings->bind("boost-amount", boost_amount->get_adjustment().get(), "value");
  settings->bind("hpf-frequency", hpf_freq->get_adjustment().get(), "value");
  settings->bind("lpf-frequency", lpf_freq->get_adjustment().get(), "value");

  g_settings_bind_with_mapping(settings->gobj(), "mode", compression_mode->gobj(), "active", G_SETTINGS_BIND_DEFAULT,
                               mode_enum_to_int, int_to_mode_enum, nullptr, nullptr);

  g_settings_bind_with_mapping(settings->gobj(), "sidechain-type", sidechain_type->gobj(), "active",
                               G_SETTINGS_BIND_DEFAULT, sidechain_type_enum_to_int, int_to_sidechain_type_enum, nullptr,
                               nullptr);

  g_settings_bind_with_mapping(settings->gobj(), "sidechain-mode", sidechain_mode->gobj(), "active",
                               G_SETTINGS_BIND_DEFAULT, sidechain_mode_enum_to_int, int_to_sidechain_mode_enum, nullptr,
                               nullptr);

  g_settings_bind_with_mapping(settings->gobj(), "sidechain-source", sidechain_source->gobj(), "active",
                               G_SETTINGS_BIND_DEFAULT, sidechain_source_enum_to_int, int_to_sidechain_source_enum,
                               nullptr, nullptr);

  g_settings_bind_with_mapping(settings->gobj(), "hpf-mode", hpf_mode->gobj(), "active", G_SETTINGS_BIND_DEFAULT,
                               filter_mode_enum_to_int, int_to_filter_mode_enum, nullptr, nullptr);

  g_settings_bind_with_mapping(settings->gobj(), "lpf-mode", lpf_mode->gobj(), "active", G_SETTINGS_BIND_DEFAULT,
                               filter_mode_enum_to_int, int_to_filter_mode_enum, nullptr, nullptr);

  connections.emplace_back(settings->signal_changed("sidechain-type").connect([=, this](const auto& key) {
    if (settings->get_string(key) != "External") {
      dropdown_input_devices->set_sensitive(false);
    } else {
      dropdown_input_devices->set_sensitive(true);
    }
  }));

  prepare_scale(input_gain, "");
  prepare_scale(output_gain, "");

  prepare_spinbutton(threshold, "dB");
  prepare_spinbutton(attack, "ms");

  prepare_spinbutton(release_threshold, "dB");
  prepare_spinbutton(release, "ms");

  prepare_spinbutton(boost_threshold, "dB");
  prepare_spinbutton(boost_amount, "dB");
  prepare_spinbutton(knee, "dB");
  prepare_spinbutton(makeup, "dB");

  prepare_spinbutton(preamp, "dB");
  prepare_spinbutton(lookahead, "ms");
  prepare_spinbutton(reactivity, "ms");

  prepare_spinbutton(hpf_freq, "Hz");
  prepare_spinbutton(lpf_freq, "Hz");

  prepare_spinbutton(ratio, "");

  if (settings->get_string("sidechain-type") != "External") {
    dropdown_input_devices->set_sensitive(false);
  } else {
    dropdown_input_devices->set_sensitive(true);
  }
}

CompressorUi::~CompressorUi() {
  util::debug(name + " ui destroyed");
}

auto CompressorUi::add_to_stack(Gtk::Stack* stack, const std::string& schema_path) -> CompressorUi* {
  auto builder = Gtk::Builder::create_from_resource("/com/github/wwmm/easyeffects/ui/compressor.ui");

  auto* ui = Gtk::Builder::get_widget_derived<CompressorUi>(
      builder, "top_box", "com.github.wwmm.easyeffects.compressor", schema_path + "compressor/");

  auto stack_page = stack->add(*ui, plugin_name::compressor);

  return ui;
}

void CompressorUi::reset() {
  bypass->set_active(false);

  settings->reset("input-gain");

  settings->reset("output-gain");

  settings->reset("mode");

  settings->reset("attack");

  settings->reset("release");

  settings->reset("release-threshold");

  settings->reset("threshold");

  settings->reset("ratio");

  settings->reset("knee");

  settings->reset("makeup");

  settings->reset("boost-threshold");

  settings->reset("sidechain-listen");

  settings->reset("sidechain-type");

  settings->reset("sidechain-mode");

  settings->reset("sidechain-source");

  settings->reset("sidechain-preamp");

  settings->reset("sidechain-reactivity");

  settings->reset("sidechain-lookahead");

  settings->reset("hpf-mode");

  settings->reset("hpf-frequency");

  settings->reset("lpf-mode");

  settings->reset("lpf-frequency");
}

void CompressorUi::on_new_reduction(double value) {
  reduction->set_value(value);

  reduction_label->set_text(level_to_localized_string(util::linear_to_db(value), 0));
}

void CompressorUi::on_new_sidechain(double value) {
  sidechain->set_value(value);

  sidechain_label->set_text(level_to_localized_string(util::linear_to_db(value), 0));
}

void CompressorUi::on_new_curve(double value) {
  curve->set_value(value);

  curve_label->set_text(level_to_localized_string(util::linear_to_db(value), 0));
}

void CompressorUi::setup_dropdown_input_devices() {
  // setting the dropdown model and factory

  auto selection_model = Gtk::SingleSelection::create(input_devices_model);

  dropdown_input_devices->set_model(selection_model);

  auto factory = Gtk::SignalListItemFactory::create();

  dropdown_input_devices->set_factory(factory);

  // setting the factory callbacks

  factory->signal_setup().connect([=](const Glib::RefPtr<Gtk::ListItem>& list_item) {
    auto* box = Gtk::make_managed<Gtk::Box>();
    auto* label = Gtk::make_managed<Gtk::Label>();
    auto* icon = Gtk::make_managed<Gtk::Image>();

    label->set_hexpand(true);
    label->set_halign(Gtk::Align::START);

    icon->set_from_icon_name("audio-input-microphone-symbolic");

    box->set_spacing(6);
    box->append(*icon);
    box->append(*label);

    // setting list_item data

    list_item->set_data("name", label);

    list_item->set_child(*box);
  });

  factory->signal_bind().connect([=](const Glib::RefPtr<Gtk::ListItem>& list_item) {
    auto* label = static_cast<Gtk::Label*>(list_item->get_data("name"));

    auto holder = std::dynamic_pointer_cast<NodeInfoHolder>(list_item->get_item());

    auto name = holder->info.name;

    label->set_name(name);
    label->set_text(name);
  });
}

void CompressorUi::set_pipe_manager_ptr(PipeManager* pipe_manager) {
  pm = pipe_manager;

  input_devices_model->append(NodeInfoHolder::create(pm->pe_source_node));

  for (const auto& node : pm->list_nodes) {
    if (node.media_class == "Audio/Source") {
      input_devices_model->append(NodeInfoHolder::create(node));
    }
  }

  connections.emplace_back(pm->source_added.connect([=, this](const NodeInfo& info) {
    for (guint n = 0; n < input_devices_model->get_n_items(); n++) {
      auto item = input_devices_model->get_item(n);

      if (item->info.id == info.id) {
        return;
      }
    }

    input_devices_model->append(NodeInfoHolder::create(info));
  }));

  connections.emplace_back(pm->source_removed.connect([=, this](const NodeInfo& info) {
    for (guint n = 0; n < input_devices_model->get_n_items(); n++) {
      auto item = input_devices_model->get_item(n);

      if (item->info.id == info.id) {
        input_devices_model->remove(n);

        return;
      }
    }
  }));
}