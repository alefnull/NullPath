#include "plugin.hpp"
#include "widgets.hpp"

#define SIGNAL_COUNT 9
#define MODE_COUNT 6
#define ENTROPY_COUNT 3

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
		ENUMS(SIGNAL_INPUT, SIGNAL_COUNT),
		CHANNELS_CV_INPUT,
		MODE_CV_INPUT,
		ENTROPY_CV_INPUT,
		NEG_ENT_CLOCK_INPUT,
		LOW_ENT_CLOCK_INPUT,
		HIGH_ENT_CLOCK_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		ENUMS(SIGNAL_OUTPUT, SIGNAL_COUNT),
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	enum Mode{
		Basic,
		Up,
		Down,
		Broadcast,
		Pairs,
		Triplets,
	};

	enum Entropy{
		Negative,
		Low,
		High,
	};

	Randrouter() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configSwitch(CHANNELS_PARAM, 0, 1, 0, "Channels", { "Mono", "Stereo" });
		configSwitch(MODE_PARAM, 0, 5, 0, "Mode", { "Basic", "Up", "Down", "Broadcast", "Pairs", "Triplets" });
		configSwitch(ENTROPY_PARAM, 0, 2, 1, "Entropy", { "Negative", "Low", "High" });
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
		configInput(CHANNELS_CV_INPUT, "Channels CV");
		getInputInfo(CHANNELS_CV_INPUT)->description = "Expects a 0-10V signal";
		configInput(MODE_CV_INPUT, "Mode CV");
		getInputInfo(MODE_CV_INPUT)->description = "Expects a 0-10V signal";
		configInput(ENTROPY_CV_INPUT, "Entropy CV");
		getInputInfo(ENTROPY_CV_INPUT)->description = "Expects a 0-10V signal";
		configInput(NEG_ENT_CLOCK_INPUT, "Negative Entropy Clock");
		configInput(LOW_ENT_CLOCK_INPUT, "Low Entropy Clock");
		configInput(HIGH_ENT_CLOCK_INPUT, "High Entropy Clock");
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
	dsp::SchmittTrigger neg_entropy_clock_trigger;
	dsp::SchmittTrigger low_entropy_clock_trigger;
	dsp::SchmittTrigger high_entropy_clock_trigger;
	dsp::SchmittTrigger reset_trigger;
	dsp::SchmittTrigger mode_trigger;
	dsp::SchmittTrigger entropy_trigger;
	dsp::SchmittTrigger channels_trigger;
	// output map, used to determine which input is routed to which output
	// index is the input, value is the output
	int output_map[SIGNAL_COUNT] = { 0, 1, 2, 3, 4, 5, 6, 7, 8 };
	int triplet_swap[3][3] = { { 1, 0, 2 }, { 2, 1, 0 }, { 0, 2, 1 } };
	int triplet_randomize[5][3] = { { 0, 2, 1 }, { 1, 0, 2 }, { 1, 2, 0 }, { 2, 0, 1 }, { 2, 1, 0 } };
	// a 2-dimensional array, 9x9, representing the 'volumes' of every input-output combination
	float volumes[SIGNAL_COUNT][SIGNAL_COUNT] = { 0 };
	// an array of 9 floats, representing the last output value for each output
	float last_values[SIGNAL_COUNT] = { 0 };
	// a float to store the current fade duration
    float fade_duration = 0.005f;
	bool mono = true;
	bool crossfade = false;
	bool hold_last_value = false;
	bool trigger_mode = false;
	bool sequential_mode = true;

	void onReset() override {
		fade_duration = 0.005f;
		mono = true;
		params[CHANNELS_PARAM].setValue(0);
		crossfade = false;
		hold_last_value = false;
		trigger_mode = false;
		sequential_mode = true;
		for (int i = 0; i < SIGNAL_COUNT; i++) {
			output_map[i] = i;
			for (int j = 0; j < SIGNAL_COUNT; j++) {
				volumes[i][j] = 0;
			}
		}
	}

	json_t *dataToJson() override {
		json_t *rootJ = json_object();
		json_t *mapJ = json_array();
		for (int i = 0; i < SIGNAL_COUNT; i++) {
			json_array_insert_new(mapJ, i, json_integer(output_map[i]));
		}
		json_object_set_new(rootJ, "output_map", mapJ);
		json_object_set_new(rootJ, "trigger_mode", json_boolean(trigger_mode));
		json_object_set_new(rootJ, "sequential_mode", json_boolean(sequential_mode));
		json_object_set_new(rootJ, "hold_last_value", json_boolean(hold_last_value));
		json_object_set_new(rootJ, "crossfade", json_boolean(crossfade));
		json_object_set_new(rootJ, "fade_duration", json_real(fade_duration));
		return rootJ;
	}

	void dataFromJson(json_t *rootJ) override {
		json_t *mapJ = json_object_get(rootJ, "output_map");
		if (mapJ) {
			for (int i = 0; i < SIGNAL_COUNT; i++) {
				output_map[i] = json_integer_value(json_array_get(mapJ, i));
			}
		}
		json_t *trigger_modeJ = json_object_get(rootJ, "trigger_mode");
		if (trigger_modeJ) {
			trigger_mode = json_boolean_value(trigger_modeJ);
		}
		json_t *sequential_modeJ = json_object_get(rootJ, "sequential_mode");
		if (sequential_modeJ) {
			sequential_mode = json_boolean_value(sequential_modeJ);
		}
		json_t *hold_last_valueJ = json_object_get(rootJ, "hold_last_value");
		if (hold_last_valueJ) {
			hold_last_value = json_boolean_value(hold_last_valueJ);
		}
		json_t *crossfadeJ = json_object_get(rootJ, "crossfade");
		if (crossfadeJ) {
			crossfade = json_boolean_value(crossfadeJ);
		}
		json_t *fade_durationJ = json_object_get(rootJ, "fade_duration");
		if (fade_durationJ) {
			fade_duration = json_real_value(fade_durationJ);
		}
	}

	int random_index() {
		if (mono) {
			return random::u32() % SIGNAL_COUNT;
		}
		else {
			return (random::u32() % 5) * 2;
		}
	}

	template <int N>
	int random_index(const int indices[N]) {
		return indices[random::u32() % N];
	}

	void process_basic(int entropy) {
		if (entropy == Entropy::Negative) { // Negative Entropy - Unwind
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
		else if (entropy == Entropy::Low) { // Low Entropy - Swap
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
		else if (entropy == Entropy::High) { // High Entropy - Randomize
			int di = mono ? 1 : 2;
			for (int i = 0; i < SIGNAL_COUNT; i+=di) {
				output_map[i] = i;
			}
			for (int i = 0; i < SIGNAL_COUNT; i+=di) {
				int r = random_index();
				int temp = output_map[i];
				output_map[i] = output_map[r];
				output_map[r] = temp;
			}
		}
	}

	void process_up(int entropy) {
		if (entropy == Entropy::Negative) { // Negative Entropy - Sort Up
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
		else if (entropy == Entropy::Low) { // Low Entropy - Shunt Up
			int new_out_map[SIGNAL_COUNT]; //Temp store for new values so they do mess up the mapping
			int max_index = 1 + random_index(); //add 1 to include max_index
			for (int i = 0; i < max_index; i++) {
				new_out_map[i] = output_map[(i - 1 + max_index) % max_index];
			}
			memcpy(output_map,new_out_map,sizeof(int) * max_index);
		}
		else if (entropy == Entropy::High) { // High Entropy - Rotate Up
			int new_out_map[SIGNAL_COUNT]; //Temp store for new values so they do mess up the mapping
			for (int i = 0; i < SIGNAL_COUNT; i++) {
				new_out_map[i] = output_map[(i - 1 + SIGNAL_COUNT) % SIGNAL_COUNT];
			}
			memcpy(output_map,new_out_map,sizeof(int) * SIGNAL_COUNT);
		}
	}

	void process_down(int entropy) {
		if (entropy == Entropy::Negative) { // Negative Entropy - Sort Down
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
		else if (entropy == Entropy::Low) { // Low Entropy - Shunt Down
			int new_out_map[SIGNAL_COUNT]; //Temp store for new values so they do mess up the mapping
			int max_index = 1 + random_index(); //add 1 to include max_index
			for (int i = 0; i < max_index; i++) {
				new_out_map[i] = output_map[(i + 1) % max_index];
			}
			memcpy(output_map,new_out_map,sizeof(int) * max_index);
		}
		else if (entropy == Entropy::High) { // High Entropy - Rotate Down
			int new_out_map[SIGNAL_COUNT]; //Temp store for new values so they do mess up the mapping
			for (int i = 0; i < SIGNAL_COUNT; i++) {
				new_out_map[i] = output_map[(i + 1) % SIGNAL_COUNT];
			}
			memcpy(output_map,new_out_map,sizeof(int) * SIGNAL_COUNT);
		}
	}

	void process_broadcast(int entropy) {
		if (entropy == Entropy::Negative) { // Negative Entropy - Split
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
		else if (entropy == Entropy::Low) { // Low Entropy - Double
			int out_1 = random_index();
			int out_2 = random_index();
			int in = random_index();
			output_map[out_1] = in;
			output_map[out_2] = in;
		}
		else if (entropy == Entropy::High) { // High Entropy - Blast
			int r = random_index();
			for (int i = 0; i < SIGNAL_COUNT; i++) {
				output_map[i] = r;
			}
		}
	}

	void process_pairs(int entropy) {
		if (entropy == Entropy::Negative) { // Negative Entropy - Unwind-2
			if (mono) {
				const int choices_mono[5] = { 0, 2, 4, 6, 8 };
				int index = random_index<5>(choices_mono);
				output_map[index] = index;
				if (index + 1 < SIGNAL_COUNT) {
					output_map[index + 1] = index + 1;
				}
			}
			else {
				const int choices_stereo[3] = { 0, 4, 8 };
				int index = random_index<3>(choices_stereo);
				output_map[index] = index;
				if (index + 2 < SIGNAL_COUNT) {
					output_map[index + 2] = index + 2;
				}
			}
		}
		else if (entropy == Entropy::Low) { // Low Entropy - Swap-2
			if (mono) {
				const int choices_mono[4] = { 0, 2, 4, 6 };
				int index = random_index<4>(choices_mono);
				int prev_input = output_map[index];
				int prev_input_2 = output_map[index + 1];
				output_map[index] = prev_input_2;
				output_map[index + 1] = prev_input;
			}
			else {
				const int choices_stereo[2] = { 0, 4 };
				int index = random_index<2>(choices_stereo);
				int prev_input = output_map[index];
				int prev_input_2 = output_map[index + 2];
				output_map[index] = prev_input_2;
				output_map[index + 2] = prev_input;
			}
		}
		else if (entropy == Entropy::High) { // High Entropy - Randomize-2
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
		if (entropy == Entropy::Negative) { // Negative Entropy - Unwind-3
			if (mono) {
				const int choices_mono[3] = { 0, 3, 6 };
				int index = random_index<3>(choices_mono);
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
					for (int i = 6; i < SIGNAL_COUNT; i++) {
						output_map[i] = i;
					}
				}
			}
		}
		else if (entropy == Entropy::Low) { // Low Entropy - Swap-3
			if (mono) {
				const int choices_mono[3] = { 0, 3, 6 };
				int index = random_index<3>(choices_mono);
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
					bool flipped = random::uniform() < 0.5;
					if (flipped) {
						int prev_input = output_map[6];
						int prev_input_2 = output_map[8];
						output_map[6] = prev_input_2;
						output_map[8] = prev_input;
					}
				}
			}
		}
		else if (entropy == Entropy::High) { // High Entropy - Randomize-3
			if (mono) {
				for (int i = 0; i <= 6; i += 3) {
					int r = random::uniform() * 5;
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
				{
					int r = random::uniform() * 5;
					int* row = triplet_randomize[r];
					int prev_input_0 = output_map[2 * row[0]];
					int prev_input_1 = output_map[2 * row[1]];
					int prev_input_2 = output_map[2 * row[2]];
					output_map[0] = prev_input_0;
					output_map[2] = prev_input_1;
					output_map[4] = prev_input_2;
				}
				{					
					bool flipped = random::uniform() < 0.5;
					if (flipped) {
						int prev_input = output_map[6];
						int prev_input_2 = output_map[8];
						output_map[6] = prev_input_2;
						output_map[8] = prev_input;
					}
				}
			}
		}
	}

	void process(const ProcessArgs& args) override {
		mono = params[CHANNELS_PARAM].getValue() == 0;
		int mode = params[MODE_PARAM].getValue();
		int entropy = params[ENTROPY_PARAM].getValue();
		bool clock = clock_trigger.process(inputs[CLOCK_INPUT].getVoltage());
		bool neg_entropy_clock = neg_entropy_clock_trigger.process(inputs[NEG_ENT_CLOCK_INPUT].getVoltage());
		bool low_entropy_clock = low_entropy_clock_trigger.process(inputs[LOW_ENT_CLOCK_INPUT].getVoltage());
		bool high_entropy_clock = high_entropy_clock_trigger.process(inputs[HIGH_ENT_CLOCK_INPUT].getVoltage());
		bool reset = reset_trigger.process(inputs[RESET_INPUT].getVoltage());
		bool clock_neg = false;
		bool clock_low = false;
		bool clock_high = false;

		// a 2-dimensional array, 9x9, representing the 'target volumes' of every input-output combination
		float target_volumes[SIGNAL_COUNT][SIGNAL_COUNT] = { 0 };

		float fade_factor = args.sampleTime * (1.f / fade_duration);

		if (trigger_mode) {
			if (sequential_mode) {
				if (mode_trigger.process(inputs[MODE_CV_INPUT].getVoltage())) {
					mode = (mode + 1) % MODE_COUNT;
					params[MODE_PARAM].setValue(mode);
				}
				if (entropy_trigger.process(inputs[ENTROPY_CV_INPUT].getVoltage())) {
					entropy = (entropy + 1) % ENTROPY_COUNT;
					params[ENTROPY_PARAM].setValue(entropy);
				}
				if (channels_trigger.process(inputs[CHANNELS_CV_INPUT].getVoltage())) {
					mono = !mono;
					params[CHANNELS_PARAM].setValue(mono ? 0 : 1);
				}
			}
			else {
				if (mode_trigger.process(inputs[MODE_CV_INPUT].getVoltage())) {
					mode = random::u32() % MODE_COUNT;
					params[MODE_PARAM].setValue(mode);
				}
				if (entropy_trigger.process(inputs[ENTROPY_CV_INPUT].getVoltage())) {
					entropy = random::u32() % ENTROPY_COUNT;
					params[ENTROPY_PARAM].setValue(entropy);
				}
				if (channels_trigger.process(inputs[CHANNELS_CV_INPUT].getVoltage())) {
					mono = random::uniform() < 0.5;
					params[CHANNELS_PARAM].setValue(mono ? 0 : 1);
				}
			}
		}
		else {
			// channels has two possible states, mono or stereo,
			// so take a 0-10V input and map it to 0 or 1 (0-5V = mono, 5-10V = stereo)
			if (inputs[CHANNELS_CV_INPUT].isConnected()) {
				mono = inputs[CHANNELS_CV_INPUT].getVoltage() < 5.0;
				params[CHANNELS_PARAM].setValue(mono ? 0 : 1);
			}

			// mode has six possible states, basic, up, down, broadcast, pairs, triplets,
			// so take a 0-10V input and map it to 0-5 (0-1.66V = basic, 1.66-3.33V = up, etc.)
			if (inputs[MODE_CV_INPUT].isConnected()) {
				mode = clamp((int)std::round(inputs[MODE_CV_INPUT].getVoltage() * 0.6), 0, 5);
				params[MODE_PARAM].setValue(mode);
			}

			// entropy has three possible states, negative, low, high,
			// so take a 0-10V input and map it to 0-2 (0-3.33V = negative, 3.33-6.66V = low, etc.)
			if (inputs[ENTROPY_CV_INPUT].isConnected()) {
				entropy = clamp((int)std::round(inputs[ENTROPY_CV_INPUT].getVoltage() * 0.3), 0, 2);
				params[ENTROPY_PARAM].setValue(entropy);
			}
		}

		if (clock) {
			switch (entropy) {
				case 0: // negative
					clock_neg = true;
					break;
				case 1: // low
					clock_low = true;
					break;
				case 2: // high
					clock_high = true;
					break;
			}
		}

		if (high_entropy_clock) {
			clock_high = true;
		}

		if (low_entropy_clock) {
			clock_low = true;
		}

		if (neg_entropy_clock) {
			clock_neg = true;
		}

		if (clock_high) {
			switch (mode) {
				case Mode::Basic: // Basic
					process_basic(2);
					break;
				case Mode::Up: // Up
					process_up(2);
					break;
				case Mode::Down: // Down
					process_down(2);
					break;
				case Mode::Broadcast: // Broadcast
					process_broadcast(2);
					break;
				case Mode::Pairs: // Pairs
					process_pairs(2);
					break;
				case Mode::Triplets: // Triplets
					process_triplets(2);
					break;
			}
		}

		if (clock_low) {
			switch (mode) {
				case Mode::Basic: // Basic
					process_basic(1);
					break;
				case Mode::Up: // Up
					process_up(1);
					break;
				case Mode::Down: // Down
					process_down(1);
					break;
				case Mode::Broadcast: // Broadcast
					process_broadcast(1);
					break;
				case Mode::Pairs: // Pairs
					process_pairs(1);
					break;
				case Mode::Triplets: // Triplets
					process_triplets(1);
					break;
			}
		}

		if (clock_neg) {
			switch (mode) {
				case Mode::Basic: // Basic
					process_basic(0);
					break;
				case Mode::Up: // Up
					process_up(0);
					break;
				case Mode::Down: // Down
					process_down(0);
					break;
				case Mode::Broadcast: // Broadcast
					process_broadcast(0);
					break;
				case Mode::Pairs: // Pairs
					process_pairs(0);
					break;
				case Mode::Triplets: // Triplets
					process_triplets(0);
					break;
			}
		}

		if (reset) {
			for (int i = 0; i < SIGNAL_COUNT; i++) {
				output_map[i] = i;
				for (int j = 0; j < SIGNAL_COUNT; j++) {
					volumes[i][j] = 0.f;
					target_volumes[i][j] = 0.f;
				}
			}
		}

		if (mono) {
			for (int i = 0; i < SIGNAL_COUNT; i++) {
				// set target volumes
				for (int j = 0; j < SIGNAL_COUNT; j++) {
					target_volumes[i][j] = (j == output_map[i]) ? 1.f : 0.f;
				}
			}
		}
		else {
			for (int ii = 0; ii < SIGNAL_COUNT; ii += 2) {
				int oi = 2 * std::floor(output_map[ii] / 2);
				bool ii_is_8 = ii == 7;
				bool oi_is_8 = oi == 7;
				if (ii_is_8) {
					if (oi_is_8) {
						//Both input and output are mono so connect them
						target_volumes[ii][oi] = 1.f;
					}
					else {
						//Input is Mono and Ouptut is Stereo so connect the input to both outputs at full strenth
						target_volumes[ii + 0][oi] = 1.f;
						target_volumes[ii + 1][oi] = 1.f;
					}
				}
				else {
					if (oi_is_8) {
						//Input is Stereo and Ouptut is Mono
						//So check to see if both inputs are connected
						bool con_1 = inputs[SIGNAL_INPUT + ii + 0].isConnected();
						bool con_2 = inputs[SIGNAL_INPUT + ii + 1].isConnected();
						if (con_1 && con_2) {
							//If both are connected, blend them at 50% volume
							target_volumes[ii + 0][oi] = 0.5f;
							target_volumes[ii + 1][oi] = 0.5f;
						}
						else if (con_1 && !con_2) {
							//Only one is connected
							target_volumes[ii + 0][oi] = 1.f;
						}
						else if (!con_1 && con_2) {
							//Only one is connected
							target_volumes[ii + 1][oi] = 1.f;
						}
						else {
							//Neither are connected, set volume to 0
							target_volumes[ii + 0][oi] = 0.f;
							target_volumes[ii + 1][oi] = 0.f;
						}
					}
					else {
						//Neither input nor output is mono so connect L to L and R to R
						target_volumes[ii + 0][oi + 0] = 1.f;
						target_volumes[ii + 1][oi + 1] = 1.f;
					}
				}
			}
		}

		for (int i = 0; i < SIGNAL_COUNT; i++) {
			for (int j = 0; j < SIGNAL_COUNT; j++) {
				if (crossfade) {
					if (volumes[i][j] < target_volumes[i][j]) {
						volumes[i][j] = std::min(volumes[i][j] + fade_factor, target_volumes[i][j]);
					}
					else if (volumes[i][j] > target_volumes[i][j]) {
						volumes[i][j] = std::max(volumes[i][j] - fade_factor, target_volumes[i][j]);
					}
				}
				else {
					volumes[i][j] = target_volumes[i][j];
				}
			}
		}

		for (int i = 0; i < SIGNAL_COUNT; i++) {
			float out = 0;
			if (hold_last_value && !inputs[SIGNAL_INPUT + output_map[i]].isConnected()) {
				out = last_values[i];
			}
			else {
				for (int j = 0; j < SIGNAL_COUNT; j++) {
					out += inputs[SIGNAL_INPUT + j].getVoltage() * volumes[i][j];
				}
			}
			last_values[i] = out;
			outputs[SIGNAL_OUTPUT + i].setVoltage(out);
		}
	}
};


struct RandrouterWidget : ModuleWidget {

	struct RRModeSwitch : app::SvgSwitch {
		RRModeSwitch(){
			addFrame(Svg::load(asset::plugin(pluginInstance,"res/entangle_mode_switch_basic.svg")));
			addFrame(Svg::load(asset::plugin(pluginInstance,"res/entangle_mode_switch_up.svg")));
			addFrame(Svg::load(asset::plugin(pluginInstance,"res/entangle_mode_switch_down.svg")));
			addFrame(Svg::load(asset::plugin(pluginInstance,"res/entangle_mode_switch_broadcast.svg")));
			addFrame(Svg::load(asset::plugin(pluginInstance,"res/entangle_mode_switch_pairs.svg")));
			addFrame(Svg::load(asset::plugin(pluginInstance,"res/entangle_mode_switch_triplets.svg")));
			shadow->blurRadius = 0;
		}
	};

	struct RoutingWidget : widget::Widget{
		Randrouter* module;

		Vec ins[SIGNAL_COUNT];
		Vec outs[SIGNAL_COUNT];

		void initalize(){
			Rect b = box.zeroPos();
			float div = SIGNAL_COUNT - 1;
			for(int si = 0; si < SIGNAL_COUNT; si ++){
				ins[si] = b.interpolate(Vec(0,si/div));
				outs[si] = b.interpolate(Vec(1,si/div));
			}
		}

		void draw(const DrawArgs& args) override {
			drawLayer(args,0);
		}

		void drawLayer(const DrawArgs& args, int layer) override {

			if (layer == 0){

				const int DEFAULT_OUTPUT_MAP[SIGNAL_COUNT] = { 5, 6, 7, 4, 0, 8, 2, 3, 1 };
				const NVGcolor LINE_COLOR = nvgRGBA(0x97, 0xf7, 0xf9, 0x80);
				const float CONST_RND[SIGNAL_COUNT][SIGNAL_COUNT][4] = {{{0.799f,0.199f,0.007f,0.023f},{0.158f,0.268f,0.887f,0.505f},{0.829f,0.654f,0.756f,0.280f},{0.833f,0.826f,0.479f,0.340f},{0.624f,0.468f,0.544f,0.841f},{0.328f,0.930f,0.496f,0.966f},{0.258f,0.022f,0.003f,0.742f},{0.257f,0.516f,0.592f,0.339f},{0.676f,0.857f,0.465f,0.965f}},{{0.889f,0.320f,0.343f,0.212f},{0.361f,0.072f,0.290f,0.173f},{0.520f,0.152f,0.802f,0.263f},{0.076f,0.900f,0.643f,0.085f},{0.662f,0.072f,0.010f,0.087f},{0.345f,0.046f,0.558f,0.579f},{0.716f,0.336f,0.997f,0.374f},{0.489f,0.861f,0.283f,0.961f},{0.424f,0.954f,0.526f,0.695f}},{{0.552f,0.922f,0.571f,0.014f},{0.910f,0.770f,0.263f,0.045f},{0.763f,0.509f,0.397f,0.620f},{0.914f,0.642f,0.129f,0.849f},{0.534f,0.806f,0.559f,0.353f},{0.657f,0.587f,0.647f,0.085f},{0.348f,0.417f,0.297f,0.695f},{0.652f,0.898f,0.245f,0.241f},{0.368f,0.207f,0.884f,0.593f}},{{0.662f,0.684f,0.716f,0.360f},{0.431f,0.149f,0.519f,0.267f},{0.563f,0.271f,0.630f,0.576f},{0.547f,0.195f,0.629f,0.383f},{0.113f,0.156f,0.088f,0.653f},{0.525f,0.489f,0.227f,0.357f},{0.058f,0.963f,0.232f,0.563f},{0.165f,0.934f,0.515f,0.145f},{0.635f,0.224f,0.684f,0.383f}},{{0.447f,0.363f,0.661f,0.121f},{0.569f,0.888f,0.141f,0.582f},{0.408f,0.779f,0.384f,0.569f},{0.339f,0.832f,0.716f,0.360f},{0.240f,0.173f,0.669f,0.867f},{0.545f,0.315f,0.938f,0.286f},{0.567f,0.911f,0.774f,0.250f},{0.153f,0.179f,0.319f,0.237f},{0.616f,0.110f,0.682f,0.270f}},{{0.559f,0.642f,0.873f,0.179f},{0.844f,0.514f,0.780f,0.954f},{0.461f,0.076f,0.289f,0.694f},{0.924f,0.382f,0.763f,0.522f},{0.479f,0.053f,0.534f,0.423f},{0.281f,0.580f,0.489f,0.320f},{0.110f,0.895f,0.625f,0.666f},{0.467f,0.428f,0.513f,0.714f},{0.641f,0.788f,0.128f,0.733f}},{{0.024f,0.782f,0.744f,0.810f},{0.998f,0.316f,0.414f,0.147f},{0.259f,0.920f,0.687f,0.446f},{0.563f,0.716f,0.039f,0.268f},{0.941f,0.254f,0.918f,0.375f},{0.787f,0.783f,0.158f,0.166f},{0.925f,0.750f,0.355f,0.509f},{0.039f,0.778f,0.073f,0.806f},{0.369f,0.754f,0.652f,0.596f}},{{0.767f,0.822f,0.813f,0.561f},{0.530f,0.280f,0.308f,0.498f},{0.357f,0.570f,0.693f,0.373f},{0.367f,0.433f,0.238f,0.067f},{0.537f,0.888f,0.638f,0.080f},{0.553f,0.825f,0.200f,0.405f},{0.019f,0.037f,0.568f,0.700f},{0.405f,0.365f,0.437f,0.348f},{0.950f,0.463f,0.474f,0.147f}},{{0.110f,0.736f,0.628f,0.519f},{0.534f,0.808f,0.994f,0.545f},{0.842f,0.059f,0.038f,0.157f},{0.740f,0.470f,0.208f,0.379f},{0.785f,0.450f,0.909f,0.500f},{0.810f,0.161f,0.066f,0.958f},{0.926f,0.403f,0.393f,0.354f},{0.338f,0.901f,0.998f,0.533f},{0.111f,0.584f,0.872f,0.080f}}};

				nvgStrokeColor(args.vg, LINE_COLOR);
				nvgLineCap(args.vg, NVG_ROUND);
				nvgMiterLimit(args.vg, 2.f);
				nvgStrokeWidth(args.vg, 2.f);

				bool stereo = false;
				if(module != NULL){
					stereo = !module->mono;
				}

				//Draw Stereo Connectors
				if(stereo){
					for(int si = 0; si < 8; si += 2){
						drawConnector(args,ins[si],ins[si+1]);
						drawConnector(args,outs[si],outs[si+1]);
					}
				}

			
				for(int oi = 0; oi < SIGNAL_COUNT; oi += (stereo ? 2 : 1)){
					for(int _ii = 0; _ii < SIGNAL_COUNT; _ii += (stereo ? 2 : 1)){
						int ii = _ii;
						if(module == NULL){
							if(ii != DEFAULT_OUTPUT_MAP[oi]) continue;
						}else{
							float vol = module->volumes[oi][ii];
							// DEBUG("%i to %i vol = %f",ii,oi,vol);
							if(vol == 0) continue;
							nvgGlobalAlpha(args.vg, vol);	
						}		

						if(stereo){
							ii = 2 * std::floor(ii / 2);
						}

						Vec in = ins[ii];
						Vec out = outs[oi];
						// DEBUG("drawing %i to %i",ii,oi);
						if(stereo){
							if(ii+1 < SIGNAL_COUNT) in = in.plus(ins[ii+1]).mult(0.5f);
							if(oi+1 < SIGNAL_COUNT) out = out.plus(outs[oi+1]).mult(0.5f);
						}

						int di = std::abs(ii - oi);

						if(di <= 1){
							//Draw Straight Line
							nvgBeginPath(args.vg);
							nvgMoveTo(args.vg, VEC_ARGS(in));
							nvgLineTo(args.vg, VEC_ARGS(out));
							nvgStroke(args.vg);
						}else{
							Vec m1;
							Vec m2;

							Vec d = out.minus(in);

							float h1 = 0.33f + 0.2f * (CONST_RND[oi][ii][0] * 2.f - 1);
							float h2 = 0.66f + 0.2f * (CONST_RND[oi][ii][1] * 2.f - 1);

							m1 = in.plus(d.mult(h1));
							m2 = in.plus(d.mult(h2));

							float v1 = 0.5f + 0.5f * (CONST_RND[oi][ii][2] * 2.f - 1);
							float v2 = 0.5f + 0.5f * (CONST_RND[oi][ii][3] * 2.f - 1);

							m1.y = m1.y * (1-v1) + v1 * in.y;
							m2.y = m2.y * (1-v2) + v2 * out.y;

							nvgBeginPath(args.vg);
							nvgMoveTo(args.vg, VEC_ARGS(in));
							nvgLineTo(args.vg, VEC_ARGS(m1));
							nvgLineTo(args.vg, VEC_ARGS(m2));
							nvgLineTo(args.vg, VEC_ARGS(out));
							nvgStroke(args.vg);
						}
					}
				}
			}	
		}

		void drawConnector(const DrawArgs& args, Vec a, Vec b){
			nvgBeginPath(args.vg);
			nvgMoveTo(args.vg, VEC_ARGS(a));
			nvgLineTo(args.vg, VEC_ARGS(b));
			nvgStroke(args.vg);
		}

	};

	RandrouterWidget(Randrouter* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/Entangle.svg")));

		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<RRModeSwitch>(mm2px(Vec(63.404, 29.491)), module, Randrouter::MODE_PARAM));
		addParam(createParamCentered<NP::EntropySwitch>(mm2px(Vec(63.404, 67.338)), module, Randrouter::ENTROPY_PARAM));
		addParam(createParamCentered<NP::ChannelsSwitch>(mm2px(Vec(63.404, 99.503)), module, Randrouter::CHANNELS_PARAM));

		addInput(createInputCentered<NP::InPort>(mm2px(Vec(21.534, 17.146)), module, Randrouter::SIGNAL_INPUT + 0));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(21.557, 28.794)), module, Randrouter::SIGNAL_INPUT + 1));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(21.557, 40.353)), module, Randrouter::SIGNAL_INPUT + 2));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(21.557, 51.869)), module, Randrouter::SIGNAL_INPUT + 3));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(21.534, 63.448)), module, Randrouter::SIGNAL_INPUT + 4));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(21.534, 75.023)), module, Randrouter::SIGNAL_INPUT + 5));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(21.557, 86.671)), module, Randrouter::SIGNAL_INPUT + 6));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(21.557, 98.247)), module, Randrouter::SIGNAL_INPUT + 7));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(21.557, 109.746)), module, Randrouter::SIGNAL_INPUT + 8));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(8.129, 58.382)), module, Randrouter::CLOCK_INPUT));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(8.151, 76.546)), module, Randrouter::RESET_INPUT));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(63.404, 46.323)), module, Randrouter::MODE_CV_INPUT));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(63.381, 79.287)), module, Randrouter::ENTROPY_CV_INPUT));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(63.404, 109.741)), module, Randrouter::CHANNELS_CV_INPUT));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(8.151, 88.046)), module, Randrouter::NEG_ENT_CLOCK_INPUT));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(8.151, 99.546)), module, Randrouter::LOW_ENT_CLOCK_INPUT));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(8.151, 111.046)), module, Randrouter::HIGH_ENT_CLOCK_INPUT));

		addOutput(createOutputCentered<NP::OutPort>(mm2px(Vec(49.567, 17.286)), module, Randrouter::SIGNAL_OUTPUT + 0));
		addOutput(createOutputCentered<NP::OutPort>(mm2px(Vec(49.563, 28.87)), module, Randrouter::SIGNAL_OUTPUT + 1));
		addOutput(createOutputCentered<NP::OutPort>(mm2px(Vec(49.567, 40.437)), module, Randrouter::SIGNAL_OUTPUT + 2));
		addOutput(createOutputCentered<NP::OutPort>(mm2px(Vec(49.563, 52.021)), module, Randrouter::SIGNAL_OUTPUT + 3));
		addOutput(createOutputCentered<NP::OutPort>(mm2px(Vec(49.563, 63.596)), module, Randrouter::SIGNAL_OUTPUT + 4));
		addOutput(createOutputCentered<NP::OutPort>(mm2px(Vec(49.563, 75.172)), module, Randrouter::SIGNAL_OUTPUT + 5));
		addOutput(createOutputCentered<NP::OutPort>(mm2px(Vec(49.563, 86.747)), module, Randrouter::SIGNAL_OUTPUT + 6));
		addOutput(createOutputCentered<NP::OutPort>(mm2px(Vec(49.563, 98.323)), module, Randrouter::SIGNAL_OUTPUT + 7));
		addOutput(createOutputCentered<NP::OutPort>(mm2px(Vec(49.567, 109.89)), module, Randrouter::SIGNAL_OUTPUT + 8));

		addChild(createRoutingWidget(module,mm2px(Vec(21.557, 17.142)),mm2px(Vec(28.006, 92.756))));
	}

	struct FadeDurationQuantity : Quantity {
		float* fade_duration;

		FadeDurationQuantity(float* fs) {
			fade_duration = fs;
		}

		void setValue(float value) override {
			value = (int)(value * 1000);
			value = (float)value / 1000;
			*fade_duration = clamp(value, 0.005f, 10.f);
		}

		float getValue() override {
			return *fade_duration;
		}
		
		float getMinValue() override {return 0.005f;}
		float getMaxValue() override {return 10.f;}
		float getDefaultValue() override {return 0.005f;}
		float getDisplayValue() override {return *fade_duration;}

		std::string getUnit() override {
			return "s";
		}
	};

	struct FadeDurationSlider : ui::Slider {
		FadeDurationSlider(float* fade_duration) {
			quantity = new FadeDurationQuantity(fade_duration);
		}
		~FadeDurationSlider() {
			delete quantity;
		}
	};

	void appendContextMenu(Menu* menu) override {
		Randrouter* module = dynamic_cast<Randrouter*>(this->module);
		assert(module);

		menu->addChild(new MenuSeparator());
		menu->addChild(createSubmenuItem("Trigger mode", "", [=](Menu* menu) {
			Menu* modeMenu = new Menu();
			modeMenu->addChild(createMenuItem("Enabled", CHECKMARK(module->trigger_mode), [module]() { module->trigger_mode = !module->trigger_mode; }));
			modeMenu->addChild(new MenuSeparator());
			modeMenu->addChild(createMenuItem("Random mode", CHECKMARK(!module->sequential_mode), [module]() { module->sequential_mode = !module->sequential_mode; }));
			menu->addChild(modeMenu);
		}));
		menu->addChild(createMenuItem("Hold last value", CHECKMARK(module->hold_last_value), [module]() { module->hold_last_value = !module->hold_last_value; }));
		menu->addChild(createMenuItem("Fade while switching", CHECKMARK(module->crossfade), [module]() { module->crossfade = !module->crossfade; }));
		FadeDurationSlider *fade_slider = new FadeDurationSlider(&(module->fade_duration));
		fade_slider->box.size.x = 200.f;
		menu->addChild(fade_slider);
	}

	RoutingWidget* createRoutingWidget(Randrouter* module, Vec pos, Vec size){
		RoutingWidget* display = createWidget<RoutingWidget>(pos);
		display->module = module;
		display->box.size = size;
		display->initalize();
		return display;
	}
};


Model* modelRandrouter = createModel<Randrouter, RandrouterWidget>("randrouter");