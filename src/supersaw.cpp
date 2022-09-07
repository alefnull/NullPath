#include "plugin.hpp"
#include "oscillator.hpp"


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
		WIDTH_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		VOCT_INPUT,
		WIDTH_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		SIGNAL_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	Oscillator osc[3];
	float noise_dur = 0.f;
	float noise_mix = 0.f;
	float last_noise = 0.f;
	float noise_time = 0.f;

	Supersaw() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(OCTAVE_PARAM, -2.0, 2.0, 0.0, "Octave");
		getParamQuantity(OCTAVE_PARAM)->snapEnabled = true;
		configParam(PITCH_PARAM, -1.0, 1.0, 0.0, "Pitch");
		configSwitch(WAVE_PARAM, 0.0, 1.0, 0.0, "Wave", {"Saw", "Pulse"});
		configParam(FINE_1_PARAM, -0.1, 0.1, 0.0, "Fine 1");
		configParam(FINE_2_PARAM, -0.1, 0.1, 0.0, "Fine 2");
		configParam(FINE_3_PARAM, -0.1, 0.1, 0.0, "Fine 3");
		configParam(NOISE_DUR_PARAM, 0.0, 0.001, 0.0, "Noise duration");
		configParam(NOISE_MIX_PARAM, 0.0, 1.0, 0.0, "Noise mix");
		configParam(WIDTH_PARAM, 0.0, 1.0, 0.5, "Pulse width");
		configInput(VOCT_INPUT, "1 V/Oct");
		configInput(WIDTH_INPUT, "Pulse width CV");
		configOutput(SIGNAL_OUTPUT, "Signal");
	}

	void process(const ProcessArgs& args) override {
		float pitch = params[PITCH_PARAM].getValue();
		int wave = params[WAVE_PARAM].getValue();
		float octave = params[OCTAVE_PARAM].getValue();
		float fine1 = params[FINE_1_PARAM].getValue();
		float fine2 = params[FINE_2_PARAM].getValue();
		float fine3 = params[FINE_3_PARAM].getValue();
		float width = params[WIDTH_PARAM].getValue();
		float noise_dur = params[NOISE_DUR_PARAM].getValue();
		float noise_mix = params[NOISE_MIX_PARAM].getValue();
		float voct = inputs[VOCT_INPUT].getVoltage();
		float width_cv = inputs[WIDTH_INPUT].getVoltage() / 10.f;

		if (noise_time > noise_dur) {
			last_noise = random::uniform() * 2.f - 1.f;
			noise_time = 0.f;
		}
		noise_time += args.sampleTime;
		float noise = last_noise * noise_mix;

		float out = 0.f;

		osc[0].set_pitch(pitch + octave + fine1 + voct);
		osc[1].set_pitch(pitch + octave + fine2 + voct);
		osc[2].set_pitch(pitch + octave + fine3 + voct);

		width += width_cv;
		out += osc[0].saw(osc[0].freq, args.sampleTime) * 0.33f;
		switch (wave) {
			case 0:
				out += osc[1].saw(osc[1].freq, args.sampleTime) * 0.33f;
				break;
			case 1:
				out += osc[2].pulse(osc[2].freq, args.sampleTime, width) * 0.33f;
				break;
		}
		out += osc[2].saw(osc[2].freq, args.sampleTime) * 0.33f;

		out += noise;
		outputs[SIGNAL_OUTPUT].setVoltage(clamp(out * 5.f, -10.f, 10.f));
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
		x -= dx;
		y += dy;
		addInput(createInputCentered<PJ301MPort>(Vec(x, y), module, Supersaw::VOCT_INPUT));
		x += dx;
		addInput(createInputCentered<PJ301MPort>(Vec(x, y), module, Supersaw::WIDTH_INPUT));
		x += dx;
		addParam(createParamCentered<RoundSmallBlackKnob>(Vec(x, y), module, Supersaw::WIDTH_PARAM));
		x -= dx;
		y += dy;
		addOutput(createOutputCentered<PJ301MPort>(Vec(x, y), module, Supersaw::SIGNAL_OUTPUT));
	}
};


Model* modelSupersaw = createModel<Supersaw, SupersawWidget>("supersaw");