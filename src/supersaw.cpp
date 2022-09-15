#include "plugin.hpp"
#include "oscillator.hpp"

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
		PARAMS_LEN
	};
	enum InputId {
		VOCT_INPUT,
		NOISE_DUR_CV_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		SIGNAL_OUTPUT,
		NOISE_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	Oscillator osc1[MAX_POLY];
	Oscillator osc2[MAX_POLY];
	Oscillator osc3[MAX_POLY];
	Oscillator osc4[MAX_POLY];
	float noise_dur = 0.f;
	float noise_mix = 0.f;
	float last_noise = 0.f;
	float noise_time = 0.f;

	Supersaw() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(OCTAVE_PARAM, -2.0, 2.0, 0.0, "Octave");
		getParamQuantity(OCTAVE_PARAM)->snapEnabled = true;
		configParam(PITCH_PARAM, -1.0, 1.0, 0.0, "Pitch");
		configSwitch(WAVE_PARAM, 0.0, 1.0, 0.0, "Middle Wave", {"Saw", "Triangle"});
		configParam(FINE_1_PARAM, -0.02, 0.02, 0.0, "Fine 1");
		configParam(FINE_2_PARAM, -0.02, 0.02, 0.0, "Fine 2");
		configParam(FINE_3_PARAM, -0.02, 0.02, 0.0, "Fine 3");
		configParam(NOISE_DUR_PARAM, 0.0, 0.001, 0.0, "Noise duration");
		configInput(NOISE_DUR_CV_INPUT, "Noise duration CV");
		configParam(NOISE_MIX_PARAM, 0.0, 0.5, 0.0, "Noise mix", "%", 0.0, 100.0);
		configInput(VOCT_INPUT, "1 V/Oct");
		configOutput(SIGNAL_OUTPUT, "Signal");
		configOutput(NOISE_OUTPUT, "Noise");
	}

	void process(const ProcessArgs& args) override {
		int channels = std::max(1, inputs[VOCT_INPUT].getChannels());
		outputs[SIGNAL_OUTPUT].setChannels(channels);

		float pitch = params[PITCH_PARAM].getValue();
		int wave = params[WAVE_PARAM].getValue();
		float octave = params[OCTAVE_PARAM].getValue();
		float fine1 = params[FINE_1_PARAM].getValue();
		float fine2 = params[FINE_2_PARAM].getValue();
		float fine3 = params[FINE_3_PARAM].getValue();
		float noise_dur = params[NOISE_DUR_PARAM].getValue();
		float noise_dur_cv = inputs[NOISE_DUR_CV_INPUT].getVoltage() / 10000.f;
		noise_dur = clamp(noise_dur + noise_dur_cv, 0.f, 0.001f);
		float noise_mix = params[NOISE_MIX_PARAM].getValue();

		for (int c = 0; c < channels; c++) {
			float voct = inputs[VOCT_INPUT].getVoltage(c);

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
			switch (wave) {
				case 0:
					out += osc2[c].saw(osc2[c].freq, args.sampleTime) * 0.33f;
					break;
				case 1:
					out += osc2[c].triangle(osc2[c].freq, args.sampleTime) * 0.33f;
					break;
			}
			out += osc3[c].saw(osc3[c].freq, args.sampleTime) * 0.33f;

			out += noise;
			outputs[SIGNAL_OUTPUT].setVoltage(clamp(out * 5.f, -10.f, 10.f), c);
		}
		outputs[NOISE_OUTPUT].setVoltage(last_noise * 5.f);
	}
};


struct SupersawWidget : ModuleWidget {
	SupersawWidget(Supersaw* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/supersaw.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		float x_start = RACK_GRID_WIDTH * 2;
		float y_start = RACK_GRID_WIDTH * 3;
		float dx = RACK_GRID_WIDTH * 2;
		float dy = RACK_GRID_WIDTH * 2;

		float x = x_start;
		float y = y_start;

		addParam(createParamCentered<RoundSmallBlackKnob>(Vec(x, y), module, Supersaw::OCTAVE_PARAM));
		x += dx;
		addParam(createParamCentered<RoundSmallBlackKnob>(Vec(x, y), module, Supersaw::PITCH_PARAM));
		x += dx;
		addParam(createParamCentered<CKSS>(Vec(x, y), module, Supersaw::WAVE_PARAM));
		x -= dx * 2;
		y += dy;
		addParam(createParamCentered<RoundSmallBlackKnob>(Vec(x, y), module, Supersaw::FINE_1_PARAM));
		x += dx;
		addParam(createParamCentered<RoundSmallBlackKnob>(Vec(x, y), module, Supersaw::FINE_2_PARAM));
		x += dx;
		addParam(createParamCentered<RoundSmallBlackKnob>(Vec(x, y), module, Supersaw::FINE_3_PARAM));
		x -= dx * 2;
		y += dy;
		addParam(createParamCentered<RoundSmallBlackKnob>(Vec(x, y), module, Supersaw::NOISE_DUR_PARAM));
		x += dx;
		addParam(createParamCentered<RoundSmallBlackKnob>(Vec(x, y), module, Supersaw::NOISE_MIX_PARAM));
		x += dx;
		addInput(createInputCentered<PJ301MPort>(Vec(x, y), module, Supersaw::NOISE_DUR_CV_INPUT));
		x -= dx * 2;
		y += dy;
		addInput(createInputCentered<PJ301MPort>(Vec(x, y), module, Supersaw::VOCT_INPUT));
		x += dx;
		addOutput(createOutputCentered<PJ301MPort>(Vec(x, y), module, Supersaw::SIGNAL_OUTPUT));
		x += dx;
		addOutput(createOutputCentered<PJ301MPort>(Vec(x, y), module, Supersaw::NOISE_OUTPUT));
	}
};


Model* modelSupersaw = createModel<Supersaw, SupersawWidget>("supersaw");