#include "plugin.hpp"


struct Randrouter : Module {
	enum ParamId {
		CHANNELS_PARAM,
		MODE_PARAM,
		ENTROPY_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		CLOCK_INPUT,
		RESET_INPUT,
		ENUMS(SIGNAL_INPUT, 9),
		INPUTS_LEN
	};
	enum OutputId {
		ENUMS(SIGNAL_OUTPUT, 9),
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	Randrouter() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configSwitch(CHANNELS_PARAM, 0, 1, 0, "Channels", { "Mono", "Stereo" });
		configSwitch(MODE_PARAM, 0, 5, 0, "Mode", { "Basic", "Up", "Down", "Broadcast", "Pairs", "Triplets" });
		configSwitch(ENTROPY_PARAM, 0, 2, 0, "Entropy", { "Negative", "Low", "High" });
		configInput(CLOCK_INPUT, "Clock");
		configInput(RESET_INPUT, "Reset");
		configInput(SIGNAL_INPUT + 0, "Signal 1");
		configInput(SIGNAL_INPUT + 1, "Signal 2");
		configInput(SIGNAL_INPUT + 2, "Signal 3");
		configInput(SIGNAL_INPUT + 3, "Signal 4");
		configInput(SIGNAL_INPUT + 4, "Signal 5");
		configInput(SIGNAL_INPUT + 5, "Signal 6");
		configInput(SIGNAL_INPUT + 6, "Signal 7");
		configInput(SIGNAL_INPUT + 7, "Signal 8");
		configInput(SIGNAL_INPUT + 8, "Signal 9");
		configOutput(SIGNAL_OUTPUT + 0, "Signal 1");
		configOutput(SIGNAL_OUTPUT + 1, "Signal 2");
		configOutput(SIGNAL_OUTPUT + 2, "Signal 3");
		configOutput(SIGNAL_OUTPUT + 3, "Signal 4");
		configOutput(SIGNAL_OUTPUT + 4, "Signal 5");
		configOutput(SIGNAL_OUTPUT + 5, "Signal 6");
		configOutput(SIGNAL_OUTPUT + 6, "Signal 7");
		configOutput(SIGNAL_OUTPUT + 7, "Signal 8");
		configOutput(SIGNAL_OUTPUT + 8, "Signal 9");
	}

	dsp::SchmittTrigger clock_trigger;
	dsp::SchmittTrigger reset_trigger;
	int output_map[9] = { 0, 1, 2, 3, 4, 5, 6, 7, 8 };
	int triplet_swap[3][3] = { { 1, 0, 2 }, { 2, 1, 0 }, { 0, 2, 1 } };
	int triplet_randomize[5][3] = { { 0, 2, 1 }, { 1, 0, 2 }, { 1, 2, 0 }, { 2, 0, 1 }, { 2, 1, 0 } };

	void process_basic(int entropy, int channels) {
		if (entropy == 0) { // Negative
			if (channels == 1) { // Mono
				// Unwind
				int r = random::u32() % 9;
				int attempts = 0;
				while (output_map[r] == r && attempts < 10) {
					r = random::u32() % 9;
					attempts++;
				}
				int temp = output_map[r];
				output_map[r] = r;
				output_map[temp] = temp;
			}
			else if (channels == 2) { // Stereo
				// TODO
			}
		}
		else if (entropy == 1) { // Low
			if (channels == 1) { // Mono
				// Swap
				int r1 = random::u32() % 9;
				int r2 = random::u32() % 9;
				int attempts = 0;
				while (output_map[r1] == output_map[r2] && attempts < 10) {
					r1 = random::u32() % 9;
					r2 = random::u32() % 9;
					attempts++;
				}
				int index_1 = output_map[r1];
				int index_2 = output_map[r2];
				output_map[r1] = index_2;
				output_map[r2] = index_1;
			}
			else if (channels == 2) { // Stereo
				// TODO
			}
		}
		else if (entropy == 2) { // High
			if (channels == 1) { // Mono
			 	// Randomize
				for (int i = 0; i < 9; i++) {
					int r = random::u32() % 9;
					int temp = output_map[i];
					output_map[i] = output_map[r];
					output_map[r] = temp;
				}
			}
			else if (channels == 2) { // Stereo
				// TODO
			}
		}
	}

	void process_up(int entropy, int channels) {
		if (entropy == 0) { // Negative
			if (channels == 1) { // Mono
				// Sort Up
				// TODO
			}
			else if (channels == 2) { // Stereo
				// TODO
			}
		}
		else if (entropy == 1) { // Low
			if (channels == 1) { // Mono
				// Shunt Up
				// TODO
			}
			else if (channels == 2) { // Stereo
				// TODO
			}
		}
		else if (entropy == 2) { // High
			if (channels == 1) { // Mono
				// Rotate Up
				// TODO
			}
			else if (channels == 2) { // Stereo
				// TODO
			}
		}
	}

	void process_down(int entropy, int channels) {
		if (entropy == 0) { // Negative
			if (channels == 1) { // Mono
				// Sort Down
				// TODO
			}
			else if (channels == 2) { // Stereo
				// TODO
			}
		}
		else if (entropy == 1) { // Low
			if (channels == 1) { // Mono
				// Shunt Down
				// TODO
			}
			else if (channels == 2) { // Stereo
				// TODO
			}
		}
		else if (entropy == 2) { // High
			if (channels == 1) { // Mono
				// Rotate Down
				// TODO
			}
			else if (channels == 2) { // Stereo
				// TODO
			}
		}
	}

	void process_broadcast(int entropy, int channels) {
		if (entropy == 0) { // Negative
			if (channels == 1) { // Mono
				// Split
				// TODO
			}
			else if (channels == 2) { // Stereo
				// TODO
			}
		}
		else if (entropy == 1) { // Low
			if (channels == 1) { // Mono
				// Double
				// TODO
			}
			else if (channels == 2) { // Stereo
				// TODO
			}
		}
		else if (entropy == 2) { // High
			if (channels == 1) { // Mono
				// Blast
				// TODO
			}
			else if (channels == 2) { // Stereo
				// TODO
			}
		}
	}

	void process_pairs(int entropy, int channels) {
		if (entropy == 0) { // Negative
			if (channels == 1) { // Mono
				// Unwind-2
				// TODO
			}
			else if (channels == 2) { // Stereo
				// TODO
			}
		}
		else if (entropy == 1) { // Low
			if (channels == 1) { // Mono
				// Swap-2
				// TODO
			}
			else if (channels == 2) { // Stereo
				// TODO
			}
		}
		else if (entropy == 2) { // High
			if (channels == 1) { // Mono
				// Randomize-2
				// TODO
			}
			else if (channels == 2) { // Stereo
				// TODO
			}
		}
	}

	void process_triplets(int entropy, int channels) {
		if (entropy == 0) { // Negative
			if (channels == 1) { // Mono
				// Unwind-3
				// TODO
			}
			else if (channels == 2) { // Stereo
				// TODO
			}
		}
		else if (entropy == 1) { // Low
			if (channels == 1) { // Mono
				// Swap-3
				// TODO
			}
			else if (channels == 2) { // Stereo
				// TODO
			}
		}
		else if (entropy == 2) { // High
			if (channels == 1) { // Mono
				// Randomize-3
				// TODO
			}
			else if (channels == 2) { // Stereo
				// TODO
			}
		}
	}

	void process(const ProcessArgs& args) override {
		int channels = params[CHANNELS_PARAM].getValue() + 1;
		int mode = params[MODE_PARAM].getValue();
		int entropy = params[ENTROPY_PARAM].getValue();
		bool clock = clock_trigger.process(inputs[CLOCK_INPUT].getVoltage());
		bool reset = reset_trigger.process(inputs[RESET_INPUT].getVoltage());

		if (reset) {
			for (int i = 0; i < 9; i++) {
				output_map[i] = i;
			}
		}

		if (clock) {
			switch (mode) {
				case 0: // Basic
					process_basic(entropy, channels);
					break;
				case 1: // Up
					process_up(entropy, channels);
					break;
				case 2: // Down
					process_down(entropy, channels);
					break;
				case 3: // Broadcast
					process_broadcast(entropy, channels);
					break;
				case 4: // Pairs
					process_pairs(entropy, channels);
					break;
				case 5: // Triplets
					process_triplets(entropy, channels);
					break;
			}
		}

		for (int i = 0; i < 9; i++) {
			outputs[SIGNAL_OUTPUT + i].setVoltage(inputs[SIGNAL_INPUT + output_map[i]].getVoltage());
		}
	}
};


struct RandrouterWidget : ModuleWidget {
	RandrouterWidget(Randrouter* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/randrouter.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		float x_start = RACK_GRID_WIDTH * 2;
		float y_start = RACK_GRID_WIDTH * 4;
		float dx = RACK_GRID_WIDTH * 2;
		float dy = RACK_GRID_WIDTH * 2;

		float x = x_start;
		float y = y_start;

		addInput(createInputCentered<PJ301MPort>(Vec(x, y), module, Randrouter::CLOCK_INPUT));
		y += dy;
		addInput(createInputCentered<PJ301MPort>(Vec(x, y), module, Randrouter::RESET_INPUT));
		y += dy;
		addParam(createParamCentered<CKSS>(Vec(x, y), module, Randrouter::CHANNELS_PARAM));
		y += dy;
		addParam(createParamCentered<CKSSThree>(Vec(x, y), module, Randrouter::MODE_PARAM));
		y += dy;
		addParam(createParamCentered<CKSSThree>(Vec(x, y), module, Randrouter::ENTROPY_PARAM));
		y -= dy * 3;
		x += dx;
		addInput(createInputCentered<PJ301MPort>(Vec(x, y), module, Randrouter::SIGNAL_INPUT + 0));
		y += dy;
		addInput(createInputCentered<PJ301MPort>(Vec(x, y), module, Randrouter::SIGNAL_INPUT + 1));
		y += dy;
		addInput(createInputCentered<PJ301MPort>(Vec(x, y), module, Randrouter::SIGNAL_INPUT + 2));
		y += dy;
		addInput(createInputCentered<PJ301MPort>(Vec(x, y), module, Randrouter::SIGNAL_INPUT + 3));
		y += dy;
		addInput(createInputCentered<PJ301MPort>(Vec(x, y), module, Randrouter::SIGNAL_INPUT + 4));
		y += dy;
		addInput(createInputCentered<PJ301MPort>(Vec(x, y), module, Randrouter::SIGNAL_INPUT + 5));
		y += dy;
		addInput(createInputCentered<PJ301MPort>(Vec(x, y), module, Randrouter::SIGNAL_INPUT + 6));
		y += dy;
		addInput(createInputCentered<PJ301MPort>(Vec(x, y), module, Randrouter::SIGNAL_INPUT + 7));
		y += dy;
		addInput(createInputCentered<PJ301MPort>(Vec(x, y), module, Randrouter::SIGNAL_INPUT + 8));
		y -= dy * 8;
		x += dx;
		addOutput(createOutputCentered<PJ301MPort>(Vec(x, y), module, Randrouter::SIGNAL_OUTPUT + 0));
		y += dy;
		addOutput(createOutputCentered<PJ301MPort>(Vec(x, y), module, Randrouter::SIGNAL_OUTPUT + 1));
		y += dy;
		addOutput(createOutputCentered<PJ301MPort>(Vec(x, y), module, Randrouter::SIGNAL_OUTPUT + 2));
		y += dy;
		addOutput(createOutputCentered<PJ301MPort>(Vec(x, y), module, Randrouter::SIGNAL_OUTPUT + 3));
		y += dy;
		addOutput(createOutputCentered<PJ301MPort>(Vec(x, y), module, Randrouter::SIGNAL_OUTPUT + 4));
		y += dy;
		addOutput(createOutputCentered<PJ301MPort>(Vec(x, y), module, Randrouter::SIGNAL_OUTPUT + 5));
		y += dy;
		addOutput(createOutputCentered<PJ301MPort>(Vec(x, y), module, Randrouter::SIGNAL_OUTPUT + 6));
		y += dy;
		addOutput(createOutputCentered<PJ301MPort>(Vec(x, y), module, Randrouter::SIGNAL_OUTPUT + 7));
		y += dy;
		addOutput(createOutputCentered<PJ301MPort>(Vec(x, y), module, Randrouter::SIGNAL_OUTPUT + 8));
	}
};


Model* modelRandrouter = createModel<Randrouter, RandrouterWidget>("randrouter");