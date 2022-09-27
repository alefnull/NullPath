#include "plugin.hpp"
#include "widgets.hpp"
#include "oscillator.hpp"
#include "Envelope.hpp"

#define MAX_POLY 16

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
		configParam(NOISE_MIX_PARAM, 0.0, 0.5, 0.0, "Noise mix", "%", 0.0, 100.0);
		configInput(NOISE_MIX_CV_INPUT, "Noise mix CV");
		configParam(PULSE_WIDTH_PARAM, 0.0, 1.0, 0.0, "B width");
		configInput(PULSE_WIDTH_CV_INPUT, "B width CV");
		configParam(ATTACK_PARAM, 0.01, 5.0, 0.01, "Attack time", " s");
		configParam(DECAY_PARAM, 0.1, 5.0, 0.1, "Decay time", " s");
		configParam(SUSTAIN_PARAM, 0.0, 1.0, 1.0, "Sustain level");
		configParam(RELEASE_PARAM, 0.01, 5.0, 0.01, "Release time", " s");
		configSwitch(ENV_TO_DUR_PARAM, 0.0, 1.0, 0.0, "Env -> Noise duration", {"Off", "On"});
		configSwitch(ENV_TO_MIX_PARAM, 0.0, 1.0, 0.0, "Env -> Noise mix", {"Off", "On"});
		configSwitch(ENV_TO_PW_PARAM, 0.0, 1.0, 0.0, "Env -> B width", {"Off", "On"});
		configParam(ENV_DUR_ATT_PARAM, -1.0, 1.0, 0.0, "Env -> Noise duration amount", "%", 0.0, 100.0);
		configParam(ENV_MIX_ATT_PARAM, 0.0, 1.0, 0.0, "Env -> Noise mix amount", "%", 0.0, 100.0);
		configParam(ENV_PW_ATT_PARAM, -1.0, 1.0, 0.0, "Env -> B width amount", "%", 0.0, 100.0);
		configInput(VOCT_INPUT, "1 V/Oct");
		configOutput(SIGNAL_OUTPUT, "Signal");
		for (int i = 0; i < 3; i++) {
			configOutput(WAVE_OUTPUT + i, "Wave " + std::to_string(i + 1));
		}
		configOutput(NOISE_OUTPUT, "Noise");
		configInput(GATE_INPUT, "Gate");
		configOutput(ENV_OUTPUT, "Envelope");
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
		float noise_dur = params[NOISE_DUR_PARAM].getValue();
		float noise_dur_cv = inputs[NOISE_DUR_CV_INPUT].getVoltage() / 10000.f;
		noise_dur = clamp(noise_dur + noise_dur_cv, 0.f, 0.001f);
		float pulse_width = params[PULSE_WIDTH_PARAM].getValue();
		float pulse_width_cv = inputs[PULSE_WIDTH_CV_INPUT].getVoltage() / 10.f;
		pulse_width = clamp(pulse_width + pulse_width_cv, 0.1f, 0.9f);
		float noise_mix = params[NOISE_MIX_PARAM].getValue();
		float noise_mix_cv = inputs[NOISE_MIX_CV_INPUT].getVoltage() / 10.f;
		noise_mix = clamp(noise_mix + noise_mix_cv, 0.f, 0.5f);
		float attack_time = params[ATTACK_PARAM].getValue();
		float decay_time = params[DECAY_PARAM].getValue();
		float sustain_level = params[SUSTAIN_PARAM].getValue();
		float release_time = params[RELEASE_PARAM].getValue();
		bool env_to_dur = params[ENV_TO_DUR_PARAM].getValue() > 0.5;
		bool env_to_mix = params[ENV_TO_MIX_PARAM].getValue() > 0.5;
		bool env_to_pw = params[ENV_TO_PW_PARAM].getValue() > 0.5;
		float env_dur_att = params[ENV_DUR_ATT_PARAM].getValue();
		float env_mix_att = params[ENV_MIX_ATT_PARAM].getValue();
		float env_pw_att = params[ENV_PW_ATT_PARAM].getValue();

		envelope.set_attack(attack_time);
		envelope.set_attack_shape(-0.5);
		envelope.set_decay(decay_time);
		envelope.set_decay_shape(-0.5);
		envelope.set_sustain(sustain_level);
		envelope.set_release(release_time);
		envelope.set_release_shape(-0.5);

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

		for (int c = 0; c < channels; c++) {
			float voct = inputs[VOCT_INPUT].isConnected() ? inputs[VOCT_INPUT].getVoltage(c) : 0.f;

			if (noise_time > noise_dur) {
				last_noise = osc4[c].noise();
				noise_time = 0.f;
			}
			noise_time += args.sampleTime;
			float noise = last_noise * noise_mix;

			float out = 0.f;

			osc1[c].set_pitch(pitch + octave + fine1 + voct);
			osc2[c].set_pitch(pitch + octave + fine2 + voct);
			osc3[c].set_pitch(pitch + octave + fine3 + voct);

			out += osc1[c].saw(osc1[c].freq, args.sampleTime) * 0.33f;
			outputs[WAVE_OUTPUT].setVoltage(osc1[c].saw(osc1[c].freq, args.sampleTime) * 5.f, c);
			switch (wave) {
				case 0:
				  {
				 	out += osc2[c].triangle(osc2[c].freq, args.sampleTime, pulse_width) * 0.33f;
					outputs[WAVE_OUTPUT + 1].setVoltage(osc2[c].triangle(osc2[c].freq, args.sampleTime, pulse_width * 0.5) * 5.f, c);
					break;
				  }
				case 1:
					out += osc2[c].pulse(osc2[c].freq, args.sampleTime, pulse_width) * 0.33f;
					outputs[WAVE_OUTPUT + 1].setVoltage(osc2[c].pulse(osc2[c].freq, args.sampleTime, pulse_width) * 5.f, c);
					break;
			}
			out += osc3[c].saw(osc3[c].freq, args.sampleTime) * 0.33f;
			outputs[WAVE_OUTPUT + 2].setVoltage(osc3[c].saw(osc3[c].freq, args.sampleTime) * 5.f, c);

			out += noise;

			if (inputs[GATE_INPUT].isConnected()) {
				outputs[SIGNAL_OUTPUT].setVoltage(out * 5.f * envelope.env, c);
			}
			else {
				outputs[SIGNAL_OUTPUT].setVoltage(out * 5.f, c);
			}
		}

		if (trigger.process(inputs[GATE_INPUT].getVoltage())) {
			envelope.retrigger();
		}

		if (last_gate && !(inputs[GATE_INPUT].getVoltage() > 0.5)) {
			envelope.stage = envelope.RELEASE;
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

		addParam(createParamCentered<NP::Knob>(mm2px(Vec(9.998, 34.556)), module, Supersaw::OCTAVE_PARAM));
		addParam(createParamCentered<NP::Knob>(mm2px(Vec(10.0, 53.707)), module, Supersaw::PITCH_PARAM));
		addParam(createParamCentered<NP::Knob>(mm2px(Vec(9.875, 92.03)), module, Supersaw::PULSE_WIDTH_PARAM));
		addParam(createParamCentered<NP::Knob>(mm2px(Vec(28.759, 35.811)), module, Supersaw::NOISE_DUR_PARAM));
		addParam(createParamCentered<NP::Knob>(mm2px(Vec(62.679, 35.802)), module, Supersaw::NOISE_MIX_PARAM));
		addParam(createParamCentered<NP::Knob>(mm2px(Vec(80.722, 34.556)), module, Supersaw::ATTACK_PARAM));
		addParam(createParamCentered<NP::Knob>(mm2px(Vec(80.72, 53.718)), module, Supersaw::DECAY_PARAM));
		addParam(createParamCentered<NP::Knob>(mm2px(Vec(80.72, 72.878)), module, Supersaw::SUSTAIN_PARAM));
		addParam(createParamCentered<NP::Knob>(mm2px(Vec(80.718, 92.04)), module, Supersaw::RELEASE_PARAM));
		addParam(createParamCentered<WaveSwitch>(mm2px(Vec(9.621, 72.742)), module, Supersaw::WAVE_PARAM));
		addParam(createParamCentered<NP::SmallKnob>(mm2px(Vec(30.475, 81.47)), module, Supersaw::FINE_1_PARAM));
		addParam(createParamCentered<NP::SmallKnob>(mm2px(Vec(45.719, 81.476)), module, Supersaw::FINE_2_PARAM));
		addParam(createParamCentered<NP::SmallKnob>(mm2px(Vec(60.962, 81.476)), module, Supersaw::FINE_3_PARAM));
		addParam(createParamCentered<NP::SmallKnob>(mm2px(Vec(31.447, 47.241)), module, Supersaw::ENV_DUR_ATT_PARAM));
		addParam(createParamCentered<NP::SmallKnob>(mm2px(Vec(55.326, 47.034)), module, Supersaw::ENV_MIX_ATT_PARAM));
		addParam(createParamCentered<NP::SmallKnob>(mm2px(Vec(19.722, 113.839)), module, Supersaw::ENV_PW_ATT_PARAM));
		addParam(createParamCentered<NP::OrangeSwitch>(mm2px(Vec(26.365, 114.069)), module, Supersaw::ENV_TO_PW_PARAM));
		addParam(createParamCentered<NP::OrangeSwitch>(mm2px(Vec(38.619, 47.471)), module, Supersaw::ENV_TO_DUR_PARAM));
		addParam(createParamCentered<NP::OrangeSwitch>(mm2px(Vec(48.067, 47.269)), module, Supersaw::ENV_TO_MIX_PARAM));

		addInput(createInputCentered<NP::InPort>(mm2px(Vec(10.0, 15.997)), module, Supersaw::VOCT_INPUT));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(80.607, 15.711)), module, Supersaw::GATE_INPUT));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(39.692, 35.599)), module, Supersaw::NOISE_DUR_CV_INPUT));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(51.748, 35.599)), module, Supersaw::NOISE_MIX_CV_INPUT));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(9.964, 103.205)), module, Supersaw::PULSE_WIDTH_CV_INPUT));

		addOutput(createOutputCentered<NP::OutPort>(mm2px(Vec(45.72, 19.046)), module, Supersaw::NOISE_OUTPUT));
		addOutput(createOutputCentered<NP::OutPort>(mm2px(Vec(30.48, 73.399)), module, Supersaw::WAVE_OUTPUT + 0));
		addOutput(createOutputCentered<NP::OutPort>(mm2px(Vec(45.72, 73.408)), module, Supersaw::WAVE_OUTPUT + 1));
		addOutput(createOutputCentered<NP::OutPort>(mm2px(Vec(60.967, 73.399)), module, Supersaw::WAVE_OUTPUT + 2));
		addOutput(createOutputCentered<NP::OutPort>(mm2px(Vec(45.008, 113.949)), module, Supersaw::SIGNAL_OUTPUT));
		addOutput(createOutputCentered<NP::OutPort>(mm2px(Vec(80.331, 110.899)), module, Supersaw::ENV_OUTPUT));
	}
};


Model* modelSupersaw = createModel<Supersaw, SupersawWidget>("supersaw");