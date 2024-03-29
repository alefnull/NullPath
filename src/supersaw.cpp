#include "plugin.hpp"
#include "widgets.hpp"
#include "oscillator.hpp"
#include "Envelope.hpp"

#define MAX_POLY 16
using simd::float_4;

struct Supersaw : Module {
	enum ParamId {
		OCTAVE_PARAM,
		PITCH_PARAM,
		WAVE_PARAM,
		FINE_1_PARAM,
		FINE_2_PARAM,
		FINE_3_PARAM,
		NOISE_DUR_PARAM,
		NOISE_MIX_PARAM,
		PULSE_WIDTH_PARAM,
		ATTACK_PARAM,
		DECAY_PARAM,
		SUSTAIN_PARAM,
		RELEASE_PARAM,
		ENUMS(WAVE_LEVEL_PARAM, 3),
		FINAL_LEVEL_PARAM,
		ENV_TO_DUR_PARAM,
		ENV_TO_MIX_PARAM,
		ENV_TO_PW_PARAM,
		ENV_DUR_ATT_PARAM,
		ENV_MIX_ATT_PARAM,
		ENV_PW_ATT_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		VOCT_INPUT,
		ATTACK_CV_INPUT,
		DECAY_CV_INPUT,
		SUSTAIN_CV_INPUT,
		RELEASE_CV_INPUT,
		ENUMS(WAVE_LEVEL_CV_INPUT, 3),
		FINAL_LEVEL_CV_INPUT,
		NOISE_DUR_CV_INPUT,
		PULSE_WIDTH_CV_INPUT,
		GATE_INPUT,
		NOISE_MIX_CV_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		SIGNAL_OUTPUT,
		ENUMS(WAVE_OUTPUT, 3),
		NOISE_OUTPUT,
		ENV_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	Oscillator osc1[MAX_POLY];
	Oscillator osc2[MAX_POLY];
	Oscillator osc3[MAX_POLY];
	Oscillator osc4[MAX_POLY];
	ADSREnvelope envelope;
	dsp::SchmittTrigger trigger;
	float noise_dur = 0.f;
	float noise_mix = 0.f;
	float last_noise = 0.f;
	float noise_time = 0.f;
	bool last_gate = false;
	bool linear = false;

	Supersaw() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(OCTAVE_PARAM, -2.0, 2.0, 0.0, "Octave");
		getParamQuantity(OCTAVE_PARAM)->snapEnabled = true;
		configParam(PITCH_PARAM, -1.0, 1.0, 0.0, "Pitch");
		configSwitch(WAVE_PARAM, 0.0, 1.0, 0.0, "Middle Wave", {"Saw / Triangle", "Pulse"});
		configParam(FINE_1_PARAM, -0.02, 0.02, 0.0, "Fine 1");
		configParam(FINE_2_PARAM, -0.02, 0.02, 0.0, "Fine 2");
		configParam(FINE_3_PARAM, -0.02, 0.02, 0.0, "Fine 3");
		configParam(NOISE_DUR_PARAM, 0.0, 0.001, 0.0, "Noise duration");
		configInput(NOISE_DUR_CV_INPUT, "Noise duration CV");
		getInputInfo(NOISE_DUR_CV_INPUT)->description = "Expects a 0-10V signal";
		configParam(NOISE_MIX_PARAM, 0.0, 0.5, 0.0, "Noise mix", "%", 0.0, 100.0);
		configInput(NOISE_MIX_CV_INPUT, "Noise mix CV");
		getInputInfo(NOISE_MIX_CV_INPUT)->description = "Expects a 0-10V signal";
		configParam(PULSE_WIDTH_PARAM, 0.0, 1.0, 0.0, "B width");
		configInput(PULSE_WIDTH_CV_INPUT, "B width CV");
		getInputInfo(PULSE_WIDTH_CV_INPUT)->description = "Expects a 0-10V signal";
		configParam(ATTACK_PARAM, 0.001, 5.0, 0.001, "Attack time", " s");
		configParam(DECAY_PARAM, 0.01, 5.0, 0.01, "Decay time", " s");
		configParam(SUSTAIN_PARAM, 0.0, 1.0, 1.0, "Sustain level");
		configParam(RELEASE_PARAM, 0.01, 5.0, 0.01, "Release time", " s");
		configParam(WAVE_LEVEL_PARAM + 0, 0.0, 1.0, 1.0, "Wave 1 level");
		configInput(WAVE_LEVEL_CV_INPUT + 0, "Wave 1 level CV");
		getInputInfo(WAVE_LEVEL_CV_INPUT + 0)->description = "Expects a 0-10V signal";
		configParam(WAVE_LEVEL_PARAM + 1, 0.0, 1.0, 1.0, "Wave 2 level");
		configInput(WAVE_LEVEL_CV_INPUT + 1, "Wave 2 level CV");
		getInputInfo(WAVE_LEVEL_CV_INPUT + 1)->description = "Expects a 0-10V signal";
		configParam(WAVE_LEVEL_PARAM + 2, 0.0, 1.0, 1.0, "Wave 3 level");
		configInput(WAVE_LEVEL_CV_INPUT + 2, "Wave 3 level CV");
		getInputInfo(WAVE_LEVEL_CV_INPUT + 2)->description = "Expects a 0-10V signal";
		configParam(FINAL_LEVEL_PARAM, 0.0, 1.0, 1.0, "Final level");
		configInput(FINAL_LEVEL_CV_INPUT, "Final level CV");
		getInputInfo(FINAL_LEVEL_CV_INPUT)->description = "Expects a 0-10V signal";
		configSwitch(ENV_TO_DUR_PARAM, 0.0, 1.0, 0.0, "Env -> Noise duration", {"Off", "On"});
		configSwitch(ENV_TO_MIX_PARAM, 0.0, 1.0, 0.0, "Env -> Noise mix", {"Off", "On"});
		configSwitch(ENV_TO_PW_PARAM, 0.0, 1.0, 0.0, "Env -> B width", {"Off", "On"});
		configParam(ENV_DUR_ATT_PARAM, -1.0, 1.0, 0.0, "Env -> Noise duration amount", "%", 0.0, 100.0);
		configParam(ENV_MIX_ATT_PARAM, 0.0, 1.0, 0.0, "Env -> Noise mix amount", "%", 0.0, 100.0);
		configParam(ENV_PW_ATT_PARAM, -1.0, 1.0, 0.0, "Env -> B width amount", "%", 0.0, 100.0);
		configInput(VOCT_INPUT, "1 V/Oct");
		configInput(ATTACK_CV_INPUT, "Attack CV");
		getInputInfo(ATTACK_CV_INPUT)->description = "Expects a 0-10V signal";
		configInput(DECAY_CV_INPUT, "Decay CV");
		getInputInfo(DECAY_CV_INPUT)->description = "Expects a 0-10V signal";
		configInput(SUSTAIN_CV_INPUT, "Sustain CV");
		getInputInfo(SUSTAIN_CV_INPUT)->description = "Expects a 0-10V signal";
		configInput(RELEASE_CV_INPUT, "Release CV");
		getInputInfo(RELEASE_CV_INPUT)->description = "Expects a 0-10V signal";
		configOutput(SIGNAL_OUTPUT, "Signal");
		for (int i = 0; i < 3; i++) {
			configOutput(WAVE_OUTPUT + i, "Wave " + std::to_string(i + 1));
		}
		configOutput(NOISE_OUTPUT, "Noise");
		configInput(GATE_INPUT, "Gate");
		configOutput(ENV_OUTPUT, "Envelope");
	}

	void onReset() override {
		linear = false;
		// reset the envelope
		envelope.reset();
		// reset the oscillators
		for (int i = 0; i < MAX_POLY; i++) {
			osc1[i].reset_phase();
			osc2[i].reset_phase();
			osc3[i].reset_phase();
		}
	}

	void process(const ProcessArgs& args) override {
		int channels = std::max(1, inputs[VOCT_INPUT].getChannels());
		outputs[SIGNAL_OUTPUT].setChannels(channels);
		for (int i = 0; i < 3; i++) {
			outputs[WAVE_OUTPUT + i].setChannels(channels);
		}

		float pitch = params[PITCH_PARAM].getValue();
		int wave = params[WAVE_PARAM].getValue();
		float octave = params[OCTAVE_PARAM].getValue();
		float fine1 = params[FINE_1_PARAM].getValue();
		float fine2 = params[FINE_2_PARAM].getValue();
		float fine3 = params[FINE_3_PARAM].getValue();

		float noise_dur = 0.f;
		// if cv input is connected, use param as attenuator
		if (inputs[NOISE_DUR_CV_INPUT].isConnected()) {
			noise_dur = params[NOISE_DUR_PARAM].getValue() * inputs[NOISE_DUR_CV_INPUT].getVoltage() / 10.f;
		}
		else {
			noise_dur = params[NOISE_DUR_PARAM].getValue();
		}

		float pulse_width = params[PULSE_WIDTH_PARAM].getValue();
		float pulse_width_cv = inputs[PULSE_WIDTH_CV_INPUT].getVoltage() / 10.f;
		pulse_width = clamp(pulse_width + pulse_width_cv, 0.1f, 0.9f);

		float noise_mix = 0.f;
		// if cv input is connected, use param as attenuator
		if (inputs[NOISE_MIX_CV_INPUT].isConnected()) {
			noise_mix = params[NOISE_MIX_PARAM].getValue() * inputs[NOISE_MIX_CV_INPUT].getVoltage() / 10.f;
		}
		else {
			noise_mix = params[NOISE_MIX_PARAM].getValue();
		}

		float attack_time = params[ATTACK_PARAM].getValue();
		float decay_time = params[DECAY_PARAM].getValue();
		float sustain_level = params[SUSTAIN_PARAM].getValue();
		float release_time = params[RELEASE_PARAM].getValue();
		if (inputs[ATTACK_CV_INPUT].isConnected()) {
			attack_time *= clamp(inputs[ATTACK_CV_INPUT].getVoltage() / 10.f, 0.001f, 1.f);
		}
		if (inputs[DECAY_CV_INPUT].isConnected()) {
			decay_time *= clamp(inputs[DECAY_CV_INPUT].getVoltage() / 10.f, 0.001f, 1.f);
		}
		if (inputs[SUSTAIN_CV_INPUT].isConnected()) {
			sustain_level *= clamp(inputs[SUSTAIN_CV_INPUT].getVoltage() / 10.f, 0.f, 1.f);
		}
		if (inputs[RELEASE_CV_INPUT].isConnected()) {
			release_time *= clamp(inputs[RELEASE_CV_INPUT].getVoltage() / 10.f, 0.001f, 1.f);
		}

		bool env_to_dur = params[ENV_TO_DUR_PARAM].getValue() > 0.5;
		bool env_to_mix = params[ENV_TO_MIX_PARAM].getValue() > 0.5;
		bool env_to_pw = params[ENV_TO_PW_PARAM].getValue() > 0.5;
		float env_dur_att = params[ENV_DUR_ATT_PARAM].getValue();
		float env_mix_att = params[ENV_MIX_ATT_PARAM].getValue();
		float env_pw_att = params[ENV_PW_ATT_PARAM].getValue();

		envelope.set_attack(attack_time);
		envelope.set_decay(decay_time);
		envelope.set_sustain(sustain_level);
		envelope.set_release(release_time);

		if (!linear) {
			envelope.set_attack_shape(-0.5f);
			envelope.set_decay_shape(-0.5f);
			envelope.set_release_shape(-0.5f);
		}
		else {
			envelope.set_attack_shape(0.f);
			envelope.set_decay_shape(0.f);
			envelope.set_release_shape(0.f);
		}

		if (env_to_dur) {
			noise_dur += envelope.env * env_dur_att * 0.001;
			noise_dur = clamp(noise_dur, 0.f, 0.001f);
		}

		if (env_to_mix) {
			noise_mix += envelope.env * env_mix_att * 0.5;
			noise_mix = clamp(noise_mix, 0.f, 0.5f);
		}

		if (env_to_pw) {
			float pw_min = 0.1;
			float pw_max = 0.9;
			pulse_width += envelope.env * env_pw_att * (pw_max - pw_min);
			pulse_width = clamp(pulse_width, pw_min, pw_max);
		}

		for (int c = 0; c < channels; c += 4) {
			float_4 voct = inputs[VOCT_INPUT].isConnected() ? inputs[VOCT_INPUT].getVoltageSimd<float_4>(c) : 0.f;

			float_4 wave_levels[3];
			for (int i = 0; i < 3; i++) {
				wave_levels[i] = params[WAVE_LEVEL_PARAM + i].getValue();
				if (inputs[WAVE_LEVEL_CV_INPUT + i].isConnected()) {
					wave_levels[i] *= clamp(inputs[WAVE_LEVEL_CV_INPUT + i].getVoltageSimd<float_4>(c) / 10.f, 0.f, 1.f);
				}
			}
			float_4 final_level = params[FINAL_LEVEL_PARAM].getValue();
			if (inputs[FINAL_LEVEL_CV_INPUT].isConnected()) {
				final_level *= clamp(inputs[FINAL_LEVEL_CV_INPUT].getVoltageSimd<float_4>(c) / 10.f, 0.f, 1.f);
			}

			if (noise_time > noise_dur) {
				last_noise = osc4[c].noise();
				noise_time = 0.f;
			}
			noise_time += args.sampleTime;
			float noise = last_noise * noise_mix;

			float_4 out = 0.f;

			osc1[c].set_pitch(pitch + octave + fine1 + voct);
			osc2[c].set_pitch(pitch + octave + fine2 + voct);
			osc3[c].set_pitch(pitch + octave + fine3 + voct);

			out += osc1[c].saw(args.sampleTime) * 0.33f * wave_levels[0];
			outputs[WAVE_OUTPUT].setVoltageSimd<float_4>(osc1[c].saw(args.sampleTime) * 5.f * wave_levels[0], c);
			switch (wave) {
				case 0:
				  {
				 	out += osc2[c].triangle(args.sampleTime, pulse_width) * 0.33f * wave_levels[1];
					outputs[WAVE_OUTPUT + 1].setVoltageSimd<float_4>(osc2[c].triangle(args.sampleTime, pulse_width * 0.5) * 5.f * wave_levels[1], c);
					break;
				  }
				case 1:
					out += osc2[c].pulse(args.sampleTime, pulse_width) * 0.33f * wave_levels[1];
					outputs[WAVE_OUTPUT + 1].setVoltageSimd<float_4>(osc2[c].pulse(args.sampleTime, pulse_width) * 5.f * wave_levels[1], c);
					break;
			}
			out += osc3[c].saw(args.sampleTime) * 0.33f * wave_levels[2];
			outputs[WAVE_OUTPUT + 2].setVoltageSimd<float_4>(osc3[c].saw(args.sampleTime) * 5.f * wave_levels[2], c);

			out += noise;

			if (inputs[GATE_INPUT].isConnected()) {
				if (envelope.idle) {
					outputs[SIGNAL_OUTPUT].setVoltageSimd<float_4>(0.f, c);
				}
				else {
					outputs[SIGNAL_OUTPUT].setVoltageSimd<float_4>(out * 5.f * envelope.env * final_level, c);
				}
			}
			else {
				if (inputs[FINAL_LEVEL_CV_INPUT].isConnected()) {
					final_level *= clamp(inputs[FINAL_LEVEL_CV_INPUT].getVoltageSimd<float_4>(c) / 10.f, 0.f, 1.f);
				}
				outputs[SIGNAL_OUTPUT].setVoltageSimd<float_4>(out * 5.f * final_level, c);
			}
		}

		if (trigger.process(inputs[GATE_INPUT].getVoltage())) {
			envelope.retrigger();
		}

		if (last_gate && !(inputs[GATE_INPUT].getVoltage() > 0.5)) {
			envelope.release();
		}

		envelope.process(args.sampleTime);

		last_gate = inputs[GATE_INPUT].getVoltage();

		outputs[ENV_OUTPUT].setVoltage(envelope.env * 10.f);
		outputs[NOISE_OUTPUT].setVoltage(last_noise * 5.f);
	}
};

struct WaveSwitch : app::SvgSwitch {
	WaveSwitch(){
		addFrame(Svg::load(asset::plugin(pluginInstance,"res/wave_switch_0.svg")));
		addFrame(Svg::load(asset::plugin(pluginInstance,"res/wave_switch_1.svg")));
		shadow->blurRadius = 0;
	}
};


struct SupersawWidget : ModuleWidget {
	SupersawWidget(Supersaw* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/Turbulence.svg")));

		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<NP::Knob>(mm2px(Vec(9.263, 33.709)), module, Supersaw::OCTAVE_PARAM));
		addParam(createParamCentered<NP::Knob>(mm2px(Vec(9.265, 52.724)), module, Supersaw::PITCH_PARAM));
		addParam(createParamCentered<NP::Knob>(mm2px(Vec(9.138, 91.183)), module, Supersaw::PULSE_WIDTH_PARAM));
		addParam(createParamCentered<NP::Knob>(mm2px(Vec(28.114, 34.946)), module, Supersaw::NOISE_DUR_PARAM));
		addParam(createParamCentered<NP::Knob>(mm2px(Vec(61.942, 34.955)), module, Supersaw::NOISE_MIX_PARAM));
		addParam(createParamCentered<NP::Knob>(mm2px(Vec(75.752, 33.71)), module, Supersaw::ATTACK_PARAM));
		addParam(createParamCentered<NP::Knob>(mm2px(Vec(75.752, 52.861)), module, Supersaw::DECAY_PARAM));
		addParam(createParamCentered<NP::Knob>(mm2px(Vec(75.752, 72.022)), module, Supersaw::SUSTAIN_PARAM));
		addParam(createParamCentered<NP::Knob>(mm2px(Vec(75.752, 91.183)), module, Supersaw::RELEASE_PARAM));
		addParam(createParamCentered<WaveSwitch>(mm2px(Vec(8.887, 71.886)), module, Supersaw::WAVE_PARAM));
		addParam(createParamCentered<NP::SmallKnob>(mm2px(Vec(29.741, 70.031)), module, Supersaw::FINE_1_PARAM));
		addParam(createParamCentered<NP::SmallKnob>(mm2px(Vec(44.986, 70.028)), module, Supersaw::FINE_2_PARAM));
		addParam(createParamCentered<NP::SmallKnob>(mm2px(Vec(60.228, 70.034)), module, Supersaw::FINE_3_PARAM));
		addParam(createParamCentered<NP::SmallKnob>(mm2px(Vec(30.714, 46.306)), module, Supersaw::ENV_DUR_ATT_PARAM));
		addParam(createParamCentered<NP::SmallKnob>(mm2px(Vec(54.503, 46.104)), module, Supersaw::ENV_MIX_ATT_PARAM));
		addParam(createParamCentered<NP::SmallKnob>(mm2px(Vec(14.052, 114.442)), module, Supersaw::ENV_PW_ATT_PARAM));
		addParam(createParamCentered<NP::OrangeSwitch>(mm2px(Vec(20.694, 114.751)), module, Supersaw::ENV_TO_PW_PARAM));
		addParam(createParamCentered<NP::OrangeSwitch>(mm2px(Vec(37.885, 46.614)), module, Supersaw::ENV_TO_DUR_PARAM));
		addParam(createParamCentered<NP::OrangeSwitch>(mm2px(Vec(47.332, 46.413)), module, Supersaw::ENV_TO_MIX_PARAM));
		addParam(createParamCentered<NP::SmallKnob>(mm2px(Vec(29.742, 89.615)), module, Supersaw::WAVE_LEVEL_PARAM+0));
		addParam(createParamCentered<NP::SmallKnob>(mm2px(Vec(44.987, 89.615)), module, Supersaw::WAVE_LEVEL_PARAM+1));
		addParam(createParamCentered<NP::SmallKnob>(mm2px(Vec(60.229, 89.621)), module, Supersaw::WAVE_LEVEL_PARAM+2));
		addParam(createParamCentered<NP::SmallKnob>(mm2px(Vec(34.75, 106.736)), module, Supersaw::FINAL_LEVEL_PARAM));

		addInput(createInputCentered<NP::InPort>(mm2px(Vec(10.713, 15.141)), module, Supersaw::VOCT_INPUT));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(81.321, 14.855)), module, Supersaw::GATE_INPUT));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(38.935, 34.747)), module, Supersaw::NOISE_DUR_CV_INPUT));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(51.014, 34.743)), module, Supersaw::NOISE_MIX_CV_INPUT));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(9.118, 103.205)), module, Supersaw::PULSE_WIDTH_CV_INPUT));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(29.743, 97.653)), module, Supersaw::WAVE_LEVEL_CV_INPUT+0));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(44.98, 97.581)), module, Supersaw::WAVE_LEVEL_CV_INPUT+1));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(60.215, 97.581)), module, Supersaw::WAVE_LEVEL_CV_INPUT+2));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(34.729, 115.231)), module, Supersaw::FINAL_LEVEL_CV_INPUT));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(85.816, 33.589)), module, Supersaw::ATTACK_CV_INPUT));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(85.816, 52.653)), module, Supersaw::DECAY_CV_INPUT));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(85.838, 71.81)), module, Supersaw::SUSTAIN_CV_INPUT));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(85.838, 90.971)), module, Supersaw::RELEASE_CV_INPUT));

		addOutput(createOutputCentered<NP::OutPort>(mm2px(Vec(44.986, 18.19)), module, Supersaw::NOISE_OUTPUT));
		addOutput(createOutputCentered<NP::OutPort>(mm2px(Vec(29.742, 61.968)), module, Supersaw::WAVE_OUTPUT+0));
		addOutput(createOutputCentered<NP::OutPort>(mm2px(Vec(44.986, 61.968)), module, Supersaw::WAVE_OUTPUT+1));
		addOutput(createOutputCentered<NP::OutPort>(mm2px(Vec(60.233, 61.96)), module, Supersaw::WAVE_OUTPUT+2));
		addOutput(createOutputCentered<NP::OutPort>(mm2px(Vec(45.861, 113.093)), module, Supersaw::SIGNAL_OUTPUT));
		addOutput(createOutputCentered<NP::OutPort>(mm2px(Vec(79.597, 109.967)), module, Supersaw::ENV_OUTPUT));
	}

	void appendContextMenu(Menu *menu) override {
		Supersaw *module = dynamic_cast<Supersaw*>(this->module);
		assert(module);

		menu->addChild(new MenuSeparator());

		// add context menu item to toggle linear variable
		menu->addChild(createMenuItem("Linear envelope", CHECKMARK(module->linear), [module]() {
			module->linear = !module->linear;
		}));
	}
};


Model* modelSupersaw = createModel<Supersaw, SupersawWidget>("supersaw");