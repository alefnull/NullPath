#pragma once

#include "rack.hpp"

using namespace rack;

extern Plugin *pluginInstance;

namespace NP {

static const NVGcolor TEAL_COLOR = nvgRGB(0xa7, 0xd4, 0xd4);

//Knobs
struct Knob : RoundKnob {
	Knob(){
		setSvg(Svg::load(asset::plugin(pluginInstance,"res/knob_dial.svg")));
		bg->setSvg(Svg::load(asset::plugin(pluginInstance,"res/knob_bg.svg")));
		shadow->blurRadius = 0.0;
	}
};

struct SmallKnob : RoundKnob {
	SmallKnob(){
		setSvg(Svg::load(asset::plugin(pluginInstance,"res/small_knob_dial.svg")));
		bg->setSvg(Svg::load(asset::plugin(pluginInstance,"res/small_knob_bg.svg")));
		shadow->blurRadius = 0.0;
	}
};


//Ports
struct InPort : SvgPort {
	InPort(){
		setSvg(Svg::load(asset::plugin(pluginInstance,"res/port_in.svg")));
		shadow->opacity = 0.0;
	}
};

struct OutPort : SvgPort {
	OutPort(){
		setSvg(Svg::load(asset::plugin(pluginInstance,"res/port_out.svg")));
		shadow->opacity = 0.0;
	}
};

//Lights

struct TealLight : GrayModuleLightWidget {
	TealLight() {
		addBaseColor(TEAL_COLOR);
	}
};

//Params

struct Button : app::SvgSwitch {
	Button(){
		momentary = true;
		addFrame(Svg::load(asset::plugin(pluginInstance,"res/button_0.svg")));
		addFrame(Svg::load(asset::plugin(pluginInstance,"res/button_1.svg")));
		shadow->blurRadius = 0.0;
	}
};


struct Switch : app::SvgSwitch {
	Switch(){
		addFrame(Svg::load(asset::plugin(pluginInstance,"res/switch_0.svg")));
		addFrame(Svg::load(asset::plugin(pluginInstance,"res/switch_1.svg")));
		shadow->blurRadius = 0.0;
	}
};

struct LoopSwitch : app::SvgSwitch {
	LoopSwitch(){
		addFrame(Svg::load(asset::plugin(pluginInstance,"res/loop_switch_0.svg")));
		addFrame(Svg::load(asset::plugin(pluginInstance,"res/loop_switch_1.svg")));
		shadow->blurRadius = 0;
	}
};

struct SpeedSwitch : app::SvgSwitch {
	SpeedSwitch(){
		addFrame(Svg::load(asset::plugin(pluginInstance,"res/speed_switch_0.svg")));
		addFrame(Svg::load(asset::plugin(pluginInstance,"res/speed_switch_1.svg")));
		addFrame(Svg::load(asset::plugin(pluginInstance,"res/speed_switch_2.svg")));
		shadow->blurRadius = 0;
	}
};

}