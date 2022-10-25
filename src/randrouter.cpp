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
	bool mono = true;

	int random_index() {
		if (mono) {
			return random::u32() % 9;
		}
		else {
			return (random::u32() % 5) * 2;
		}
	}

	int random_index(const int indices[]) {
		int len = sizeof(indices) / sizeof(indices[0]);
		return indices[random::u32() % len];
	}

	void process_basic(int entropy) {
		if (entropy == 0) { // Negative Entropy - Unwind
			int r;
			int attempts = 0;
			do {
				r = random_index();
				attempts++;
			} while (output_map[r] == r && attempts <= 10);
			int temp = output_map[r];
			output_map[r] = output_map[temp];
			output_map[temp] = temp;
		}
		else if (entropy == 1) { // Low Entropy - Swap
			int r1, r2;
			int attempts = 0;
			do {
				r1 = random_index();
				r2 = random_index();
				attempts++;
			} while (output_map[r1] == output_map[r2] && attempts <= 10);
			int index_1 = output_map[r1];
			int index_2 = output_map[r2];
			output_map[r1] = index_2;
			output_map[r2] = index_1;
		}
		else if (entropy == 2) { // High Entropy - Randomize
			for (int i = 0; i < 9; i++) {
				output_map[i] = i;
			}
			for (int i = 0; i < 9; i++) {
				int r = random_index();
				int temp = output_map[i];
				output_map[i] = output_map[r];
				output_map[r] = temp;
			}
		}
	}

	void process_up(int entropy) {
		if (entropy == 0) { // Negative Entropy - Sort Up
			int index_1 = 0;
			int index_2 = 0;
			for (int i = 8; i > 0; i--) {
				if (output_map[i] < output_map[i - 1]) {
					index_1 = i;
					index_2 = i - 1;
					break;
				}
			}
			int temp = output_map[index_1];
			output_map[index_1] = output_map[index_2];
			output_map[index_2] = temp;
		}
		else if (entropy == 1) { // Low Entropy - Shunt Up
			int max_index = random_index();
			for (int i = 0; i < max_index; i++) {
				output_map[i] = (output_map[i] - 1 + 9) % 9;
			}
		}
		else if (entropy == 2) { // High Entropy - Rotate Up
			for (int i = 0; i < 8; i++) {
				output_map[i] = (output_map[i] - 1 + 9) % 9;
			}
		}
	}

	void process_down(int entropy) {
		if (entropy == 0) { // Negative Entropy - Sort Down
			int index_1 = 0;
			int index_2 = 0;
			for (int i = 0; i < 8; i++) {
				if (output_map[i] > output_map[i + 1]) {
					index_1 = i;
					index_2 = i + 1;
					break;
				}
			}
			int temp = output_map[index_1];
			output_map[index_1] = output_map[index_2];
			output_map[index_2] = temp;
		}
		else if (entropy == 1) { // Low Entropy - Shunt Down
			int max_index = random_index();
			for (int i = 0; i < max_index; i++) {
				output_map[i] = (output_map[i] + 1) % 9;
			}
		}
		else if (entropy == 2) { // High Entropy - Rotate Down
			for (int i = 0; i < 8; i++) {
				output_map[i] = (output_map[i] + 1) % 9;
			}
		}
	}

	void process_broadcast(int entropy) {
		if (entropy == 0) { // Negative Entropy - Split
			int out_index;
			int in_index;
			int attempts = 0;
			do {
				out_index = random_index();
				in_index = output_map[out_index];
				attempts++;
			} while (output_map[out_index] != in_index && attempts <= 10);
			output_map[out_index] = out_index;
		}
		else if (entropy == 1) { // Low Entropy - Double
			int out_1 = random_index();
			int out_2 = random_index();
			int in = random_index();
			output_map[out_1] = in;
			output_map[out_2] = in;
		}
		else if (entropy == 2) { // High Entropy - Blast
			int r = random_index();
			for (int i = 0; i < 9; i++) {
				output_map[i] = r;
			}
		}
	}

	void process_pairs(int entropy) {
		if (entropy == 0) { // Negative Entropy - Unwind-2
			if (mono) {
				const int choices_mono[5] = { 0, 2, 4, 6, 8 };
				int index = random_index(choices_mono);
				output_map[index] = index;
				if (index + 1 < 9) {
					output_map[index + 1] = index + 1;
				}
			}
			else {
				const int choices_stereo[3] = { 0, 4, 8 };
				int index = random_index(choices_stereo);
				output_map[index] = index;
				if (index + 2 < 9) {
					output_map[index + 2] = index + 2;
				}
			}
		}
		else if (entropy == 1) { // Low Entropy - Swap-2
			if (mono) {
				const int choices_mono[4] = { 0, 2, 4, 6 };
				int index = random_index(choices_mono);
				int prev_input = output_map[index];
				int prev_input_2 = output_map[index + 1];
				output_map[index] = prev_input_2;
				output_map[index + 1] = prev_input;
			}
			else {
				const int choices_stereo[2] = { 0, 4 };
				int index = random_index(choices_stereo);
				int prev_input = output_map[index];
				int prev_input_2 = output_map[index + 2];
				output_map[index] = prev_input_2;
				output_map[index + 2] = prev_input;
			}
		}
		else if (entropy == 2) { // High Entropy - Randomize-2
			if (mono) {
				for (int i = 0; i <= 6; i += 2) {
					bool flipped = random::uniform() < 0.5;
					if (flipped) {
						int prev_input = output_map[i];
						int prev_input_2 = output_map[i + 1];
						output_map[i] = prev_input_2;
						output_map[i + 1] = prev_input;
					}
				}
			}
			else {
				for (int i = 0; i <= 4; i += 4) {
					bool flipped = random::uniform() < 0.5;
					if (flipped) {
						int prev_input = output_map[i];
						int prev_input_2 = output_map[i + 2];
						output_map[i] = prev_input_2;
						output_map[i + 2] = prev_input;
					}
				}
			}
		}
	}

	void process_triplets(int entropy) {
		if (entropy == 0) { // Negative Entropy - Unwind-3
			if (mono) {
				const int choices_mono[3] = { 0, 3, 6 };
				int index = random_index(choices_mono);
				output_map[index] = index;
				output_map[index + 1] = index + 1;
				output_map[index + 2] = index + 2;
			}
			else {
				float odds = random::uniform();
				if (odds < 0.66) {
					for (int i = 0; i < 6; i++) {
						output_map[i] = i;
					}
				}
				else {
					for (int i = 6; i < 9; i++) {
						output_map[i] = i;
					}
				}
			}
		}
		else if (entropy == 1) { // Low Entropy - Swap-3
			if (mono) {
				const int choices_mono[3] = { 0, 3, 6 };
				int index = random_index(choices_mono);
				int r = random::uniform() * 3;
				int* row = triplet_swap[r];
				int prev_input_0 = output_map[index + row[0]];
				int prev_input_1 = output_map[index + row[1]];
				int prev_input_2 = output_map[index + row[2]];
				output_map[index + 0] = prev_input_0;
				output_map[index + 1] = prev_input_1;
				output_map[index + 2] = prev_input_2;
			}
			else {
				float odds = random::uniform();
				if (odds < 0.66) {
					int r = random::uniform() * 3;
					int* row = triplet_swap[r];
					int prev_input_0 = output_map[2 * row[0]];
					int prev_input_1 = output_map[2 * row[1]];
					int prev_input_2 = output_map[2 * row[2]];
					output_map[0] = prev_input_0;
					output_map[2] = prev_input_1;
					output_map[4] = prev_input_2;
				}
				else {
					int r = random::uniform() * 3;
					int* row = triplet_swap[r];
					int prev_input_0 = output_map[6 + row[0]];
					int prev_input_1 = output_map[6 + row[1]];
					int prev_input_2 = output_map[6 + row[2]];
					output_map[6] = prev_input_0;
					output_map[7] = prev_input_1;
					output_map[8] = prev_input_2;
				}
			}
		}
		else if (entropy == 2) { // High Entropy - Randomize-3
			if (mono) {
				for (int i = 0; i <= 6; i += 3) {
					int r = random::uniform() * 6;
					int* row = triplet_randomize[r];
					int prev_input_0 = output_map[i + row[0]];
					int prev_input_1 = output_map[i + row[1]];
					int prev_input_2 = output_map[i + row[2]];
					output_map[i + 0] = prev_input_0;
					output_map[i + 1] = prev_input_1;
					output_map[i + 2] = prev_input_2;
				}
			}
			else {
				for (int i = 0; i < 6; i++) {
					int r = random::uniform() * 6;
					int* row = triplet_randomize[r];
					int prev_input_0 = output_map[2 * row[0]];
					int prev_input_1 = output_map[2 * row[1]];
					int prev_input_2 = output_map[2 * row[2]];
					output_map[0] = prev_input_0;
					output_map[2] = prev_input_1;
					output_map[4] = prev_input_2;
				}
				for (int i = 6; i < 9; i++) {
					int r = random::uniform() * 6;
					int* row = triplet_randomize[r];
					int prev_input_0 = output_map[6 + row[0]];
					int prev_input_1 = output_map[6 + row[1]];
					int prev_input_2 = output_map[6 + row[2]];
					output_map[6] = prev_input_0;
					output_map[7] = prev_input_1;
					output_map[8] = prev_input_2;
				}
			}
		}
	}

	void process(const ProcessArgs& args) override {
		mono = params[CHANNELS_PARAM].getValue() == 0;
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
					process_basic(entropy);
					break;
				case 1: // Up
					process_up(entropy);
					break;
				case 2: // Down
					process_down(entropy);
					break;
				case 3: // Broadcast
					process_broadcast(entropy);
					break;
				case 4: // Pairs
					process_pairs(entropy);
					break;
				case 5: // Triplets
					process_triplets(entropy);
					break;
			}
		}

		if (mono) {
			for (int i = 0; i < 9; i++) {
				outputs[SIGNAL_OUTPUT + i].setVoltage(inputs[SIGNAL_INPUT + output_map[i]].getVoltage());
			}
		}
		else {
			for (int i = 0; i < 9; i += 2) {
				int index = 2 * std::floor(output_map[i] / 2);
				bool i_is_8 = i == 8;
				bool index_is_8 = index == 8;
				if (i_is_8) {
					if (index_is_8) {
						outputs[SIGNAL_OUTPUT + i].setVoltage(inputs[SIGNAL_INPUT + index].getVoltage());
					}
					else {
						bool con_1 = inputs[SIGNAL_INPUT + index + 0].isConnected();
						bool con_2 = inputs[SIGNAL_INPUT + index + 1].isConnected();
						if (con_1 && con_2) {
							float avg = (inputs[SIGNAL_INPUT + index + 0].getVoltage() + inputs[SIGNAL_INPUT + index + 1].getVoltage()) / 2;
							outputs[SIGNAL_OUTPUT + i].setVoltage(avg);
						}
						else if (con_1 && !con_2) {
							outputs[SIGNAL_OUTPUT + i].setVoltage(inputs[SIGNAL_INPUT + index + 0].getVoltage());
						}
						else if (!con_1 && con_2) {
							outputs[SIGNAL_OUTPUT + i].setVoltage(inputs[SIGNAL_INPUT + index + 1].getVoltage());
						}
						else {
							outputs[SIGNAL_OUTPUT + i].setVoltage(0);
						}
					}
				}
				else {
					if (index_is_8) {
						outputs[SIGNAL_OUTPUT + i + 0].setVoltage(inputs[SIGNAL_INPUT + index].getVoltage());
						outputs[SIGNAL_OUTPUT + i + 1].setVoltage(inputs[SIGNAL_INPUT + index].getVoltage());
					}
					else {
						outputs[SIGNAL_OUTPUT + i + 0].setVoltage(inputs[SIGNAL_INPUT + index + 0].getVoltage());
						outputs[SIGNAL_OUTPUT + i + 1].setVoltage(inputs[SIGNAL_INPUT + index + 1].getVoltage());
					}
				}
			}
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