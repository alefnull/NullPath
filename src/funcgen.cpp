#include "plugin.hpp"
#include "widgets.hpp"
#include "Envelope.hpp"


#define CHANNEL_COUNT 4
#define ENV_MAX_VOLTAGE 10.f
#define MIN_TIME 0.01f
#define MAX_TIME 5.f

struct Funcgen : Module {
	enum ParamId {
		ENUMS(RISE_PARAM, CHANNEL_COUNT),
		ENUMS(LOOP_PARAM, CHANNEL_COUNT),
		ENUMS(FALL_PARAM, CHANNEL_COUNT),
		ENUMS(PUSH_PARAM, CHANNEL_COUNT),
		MODE_PARAM,
		TRIGGER_ALL_PARAM,
		CASCADE_TRIGGER_PARAM,
		ENV_FUNC_PARAM,
		ENUMS(RISE_SHAPE_PARAM, CHANNEL_COUNT),
		ENUMS(FALL_SHAPE_PARAM, CHANNEL_COUNT),
		ENUMS(SPEED_PARAM, CHANNEL_COUNT),
		CASCADE_LOOP_PARAM, //TODO IMPLEMENT
		CASCADE_SPEED_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		ENUMS(TRIGGER_INPUT, CHANNEL_COUNT),
		ENUMS(RISE_CV_INPUT, CHANNEL_COUNT),
		ENUMS(FALL_CV_INPUT, CHANNEL_COUNT),
		TRIGGER_ALL_INPUT,
		CASCADE_TRIGGER_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		ENUMS(FUNCTION_OUTPUT, CHANNEL_COUNT),
		ENUMS(EOC_OUTPUT, CHANNEL_COUNT),
		CASCADE_OUTPUT,
		MIN_OUTPUT,
		MAX_OUTPUT,
		AVG_OUTPUT,
		AGTB_OUTPUT,
		AGTC_OUTPUT,
		AGTD_OUTPUT,
		BGTA_OUTPUT,
		BGTC_OUTPUT,
		BGTD_OUTPUT,
		CGTA_OUTPUT,
		CGTB_OUTPUT,
		CGTD_OUTPUT,
		DGTA_OUTPUT,
		DGTB_OUTPUT,
		DGTC_OUTPUT,
		ABSAB_OUTPUT,
		ABSAC_OUTPUT,
		ABSAD_OUTPUT,
		ABSBC_OUTPUT,
		ABSBD_OUTPUT,
		ABSCD_OUTPUT,
		ABSBA_OUTPUT,
		ABSCA_OUTPUT,
		ABSCB_OUTPUT,
		ABSDA_OUTPUT,
		ABSDB_OUTPUT,
		ABSDC_OUTPUT,
		TOPAVG_OUTPUT,
		BOTAVG_OUTPUT,
		ENUMS(RISING_OUTPUT, CHANNEL_COUNT),
		ENUMS(FALLING_OUTPUT, CHANNEL_COUNT),
		CASCADE_RISING_OUTPUT,
		CASCADE_FALLING_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		ENUMS(OUTPUT_LIGHT, CHANNEL_COUNT),
		ENUMS(CASCADE_LIGHT, CHANNEL_COUNT),
		LIGHTS_LEN
	};
	enum Mode {
		EACH,
		SHUFFLE,
		RANDOM
	};

	int chaos_index = 0;
	int current_index = 0;
	int shuffle_list[CHANNEL_COUNT] = {0,1,2,3};
	Mode mode = EACH;
	Envelope envelope[CHANNEL_COUNT];
	Envelope cm_envelope;
	dsp::SchmittTrigger trigger[CHANNEL_COUNT];
	dsp::SchmittTrigger push[CHANNEL_COUNT];
	dsp::SchmittTrigger cascade_trigger;
	dsp::SchmittTrigger cascade_push;
	dsp::SchmittTrigger trigger_all;
	dsp::SchmittTrigger trigger_all_push;
	dsp::BooleanTrigger eoc_trigger[CHANNEL_COUNT];
	dsp::BooleanTrigger cm_eoc_trigger;
	dsp::BooleanTrigger loop_trigger[CHANNEL_COUNT];
	dsp::PulseGenerator eoc_pulse[CHANNEL_COUNT];
	dsp::PulseGenerator cm_eoc_pulse;


	Funcgen() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configSwitch(MODE_PARAM, 0.f, 2.f, 0.f, "Mode", {"Each", "Shuffle", "Random"});
		configInput(TRIGGER_ALL_INPUT, "Trigger all");
		configParam(TRIGGER_ALL_PARAM, 0.f, 1.f, 0.f, "Trigger all");
		configOutput(CASCADE_OUTPUT, "Cascade");
		configParam(CASCADE_TRIGGER_PARAM, 0.f, 1.f, 0.f, "Cascade Re-Trigger");
		configInput(CASCADE_TRIGGER_INPUT, "Cascade Re-Trigger");
		configSwitch(CASCADE_LOOP_PARAM, 0.f, 1.f, 0.f, "Cascade Loop", {"Off", "On"});
		configSwitch(CASCADE_SPEED_PARAM, 0.f, 2.f, 0.f, "Cascade Speed", {"Slow", "Normal", "Fast"});
		configOutput(CASCADE_RISING_OUTPUT, "Cascade Rising");
		configOutput(CASCADE_FALLING_OUTPUT, "Cascade Falling");
		for (int i = 0; i < CHANNEL_COUNT; i++) {
			configParam(RISE_PARAM + i, MIN_TIME, MAX_TIME, 1.f, "Rise time", " s");
			configParam(FALL_PARAM + i, MIN_TIME, MAX_TIME, 1.f, "Fall time", " s");
			configSwitch(LOOP_PARAM + i, 0.f, 1.f, 0.f, "Loop", {"Off", "On"});
			configParam(RISE_SHAPE_PARAM + i, -1.f, 1.f, 0.f, "Rise shape");
			configParam(FALL_SHAPE_PARAM + i, -1.f, 1.f, 0.f, "Fall shape");
			configSwitch(SPEED_PARAM + i, 0.f, 2.f, 1.f, "Speed", {"Slow", "Normal", "Fast"});
			configParam(PUSH_PARAM + i, 0.f, 1.f, 0.f, "Push");
			configInput(TRIGGER_INPUT + i, "Trigger");
			configInput(RISE_CV_INPUT + i, "Rise CV");
			configInput(FALL_CV_INPUT + i, "Fall CV");
			configOutput(FUNCTION_OUTPUT + i, "Function");
			configOutput(RISING_OUTPUT + i, "Rising");
			configOutput(FALLING_OUTPUT + i, "Falling");
			configOutput(EOC_OUTPUT + i, "EOC");
		}
		configOutput(MIN_OUTPUT, "Minimum");
		configOutput(MAX_OUTPUT, "Maximum");
		configOutput(AVG_OUTPUT, "Average");
		configOutput(TOPAVG_OUTPUT, "Top 2 Average");
		configOutput(BOTAVG_OUTPUT, "Bottom 2 Average");
		configOutput(AGTB_OUTPUT, "A > B");
		configOutput(AGTC_OUTPUT, "A > C");
		configOutput(AGTD_OUTPUT, "A > D");
		configOutput(BGTA_OUTPUT, "B > A");
		configOutput(BGTC_OUTPUT, "B > C");
		configOutput(BGTD_OUTPUT, "B > D");
		configOutput(CGTA_OUTPUT, "C > A");
		configOutput(CGTB_OUTPUT, "C > B");
		configOutput(CGTD_OUTPUT, "C > D");
		configOutput(DGTA_OUTPUT, "D > A");
		configOutput(DGTB_OUTPUT, "D > B");
		configOutput(DGTC_OUTPUT, "D > C");
		configOutput(ABSAB_OUTPUT, "10 - abs(A - B)");
		configOutput(ABSAC_OUTPUT, "10 - abs(A - C)");
		configOutput(ABSAD_OUTPUT, "10 - abs(A - D)");
		configOutput(ABSBC_OUTPUT, "10 - abs(B - C)");
		configOutput(ABSBD_OUTPUT, "10 - abs(B - D)");
		configOutput(ABSCD_OUTPUT, "10 - abs(C - D)");
		configOutput(ABSBA_OUTPUT, "abs(A - B)");
		configOutput(ABSCA_OUTPUT, "abs(A - C)");
		configOutput(ABSCB_OUTPUT, "abs(B - C)");
		configOutput(ABSDA_OUTPUT, "abs(A - D)");
		configOutput(ABSDB_OUTPUT, "abs(B - D)");
		configOutput(ABSDC_OUTPUT, "abs(C - D)");
	}

	void process(const ProcessArgs& args) override {
		float st = args.sampleTime;

		mode = Funcgen::Mode(params[MODE_PARAM].getValue());

		float cascade_speed = convert_param_to_multiplier(params[CASCADE_SPEED_PARAM].getValue());

		for (int i = 0; i < CHANNEL_COUNT; i++) {
			float rise_time = params[RISE_PARAM + i].getValue();
			float fall_time = params[FALL_PARAM + i].getValue();
			float speed = convert_param_to_multiplier(params[SPEED_PARAM + i].getValue());
			float rise_shape = params[RISE_SHAPE_PARAM + i].getValue();
			float fall_shape = params[FALL_SHAPE_PARAM + i].getValue();
			envelope[i].set_rise_shape(rise_shape);
			envelope[i].set_rise(rise_time * speed);
			envelope[i].set_fall_shape(fall_shape);
			envelope[i].set_fall(fall_time * speed);
			cm_envelope.set_rise_shape(rise_shape);
			cm_envelope.set_rise(rise_time * cascade_speed);
			cm_envelope.set_fall_shape(fall_shape);
			cm_envelope.set_fall(fall_time * cascade_speed);

			if (inputs[RISE_CV_INPUT + i].isConnected()) {
				rise_time = clamp(rise_time * (inputs[RISE_CV_INPUT + i].getVoltage() / 10.f), MIN_TIME, MAX_TIME);
				envelope[i].set_rise(rise_time);
				cm_envelope.set_rise(rise_time);
			}

			if (inputs[FALL_CV_INPUT + i].isConnected()) {
				fall_time = clamp(fall_time * (inputs[FALL_CV_INPUT + i].getVoltage() / 10.f), MIN_TIME, MAX_TIME);
				envelope[i].set_fall(fall_time);
				cm_envelope.set_fall(fall_time);
			}

			bool loop = params[LOOP_PARAM + i].getValue() > 0.5f;
			envelope[i].set_loop(loop);

			if (loop_trigger[i].process(params[LOOP_PARAM + i].getValue())) {
				envelope[i].retrigger();
			}

			if (trigger[i].process(inputs[TRIGGER_INPUT + i].getVoltage()) || push[i].process(params[PUSH_PARAM + i].getValue())) {
				envelope[i].retrigger();
			}

			envelope[i].process(st);
			cm_envelope.process(st);

			if (eoc_trigger[i].process(envelope[i].eoc)) {
				eoc_pulse[i].trigger(1e-3f);
			}

			outputs[FUNCTION_OUTPUT + i].setVoltage(envelope[i].env);

			outputs[RISING_OUTPUT + i].setVoltage(envelope[i].stage == Envelope::RISING ? 10.f : 0.f);
			outputs[FALLING_OUTPUT + i].setVoltage(envelope[i].stage == Envelope::FALLING ? 10.f : 0.f);

			bool eoc = eoc_pulse[i].process(st);
			outputs[EOC_OUTPUT + i].setVoltage(eoc ? 10.f : 0.f);
		}

		if (cm_eoc_trigger.process(cm_envelope.eoc)) {
			cm_eoc_pulse.trigger(1e-3f);
		}

		bool cm_eoc = cm_eoc_pulse.process(st);
		if (cm_eoc) {
			end_envelope(current_index);
		}

		if (trigger_all.process(inputs[TRIGGER_ALL_INPUT].getVoltage()) || trigger_all_push.process(params[TRIGGER_ALL_PARAM].getValue())) {
			for (int i = 0; i < CHANNEL_COUNT; i++) {
				envelope[i].retrigger();
			}
			start_cycle();
		}

		float cascade_output = 0.f;
		if (mode == EACH) {
			cascade_output = std::max(cm_envelope[0].env, cm_envelope[1].env);
			cascade_output = std::max(cascade_output, cm_envelope[2].env);
			cascade_output = std::max(cascade_output, cm_envelope[3].env);
		}
		else if (mode == SHUFFLE) {
			cascade_output = cm_envelope[chaos_index].env;
			if (cm_eoc_pulse[chaos_index].process(st)) {
				int last_index = chaos_index;
				while (last_index == chaos_index) {
					chaos_index = random::u32() % CHANNEL_COUNT;
				}
				for (int i = 0; i < CHANNEL_COUNT; i++) {
					if (i != chaos_index) {
						cm_envelope[i].reset();
					}
				}
			}
		}
		outputs[CASCADE_OUTPUT].setVoltage(cascade_output);
		outputs[CASCADE_RISING_OUTPUT].setVoltage(cm_envelope.stage == Envelope::RISING ? 10.f : 0.f);
		outputs[CASCADE_FALLING_OUTPUT].setVoltage(cm_envelope.stage == Envelope::FALLING ? 10.f : 0.f);

		if (cascade_trigger.process(inputs[CASCADE_TRIGGER_INPUT].getVoltage())) {
			start_cycle();
		}

		float a = envelope[0].env;
		float b = envelope[1].env;
		float c = envelope[2].env;
		float d = envelope[3].env;
		outputs[MIN_OUTPUT].setVoltage(std::min(a, std::min(b, std::min(c, d))));
		outputs[MAX_OUTPUT].setVoltage(std::max(a, std::max(b, std::max(c, d))));
		outputs[AVG_OUTPUT].setVoltage((a + b + c + d) / CHANNEL_COUNT);
		outputs[AGTB_OUTPUT].setVoltage(a > b ? 10.f : 0.f);
		outputs[AGTC_OUTPUT].setVoltage(a > c ? 10.f : 0.f);
		outputs[AGTD_OUTPUT].setVoltage(a > d ? 10.f : 0.f);
		outputs[BGTA_OUTPUT].setVoltage(b > a ? 10.f : 0.f);
		outputs[BGTC_OUTPUT].setVoltage(b > c ? 10.f : 0.f);
		outputs[BGTD_OUTPUT].setVoltage(b > d ? 10.f : 0.f);
		outputs[CGTA_OUTPUT].setVoltage(c > a ? 10.f : 0.f);
		outputs[CGTB_OUTPUT].setVoltage(c > b ? 10.f : 0.f);
		outputs[CGTD_OUTPUT].setVoltage(c > d ? 10.f : 0.f);
		outputs[DGTA_OUTPUT].setVoltage(d > a ? 10.f : 0.f);
		outputs[DGTB_OUTPUT].setVoltage(d > b ? 10.f : 0.f);
		outputs[DGTC_OUTPUT].setVoltage(d > c ? 10.f : 0.f);
		outputs[ABSAB_OUTPUT].setVoltage(10 - std::abs(a - b));
		outputs[ABSAC_OUTPUT].setVoltage(10 - std::abs(a - c));
		outputs[ABSAD_OUTPUT].setVoltage(10 - std::abs(a - d));
		outputs[ABSBC_OUTPUT].setVoltage(10 - std::abs(b - c));
		outputs[ABSBD_OUTPUT].setVoltage(10 - std::abs(b - d));
		outputs[ABSCD_OUTPUT].setVoltage(10 - std::abs(c - d));
		outputs[ABSBA_OUTPUT].setVoltage(std::abs(a - b));
		outputs[ABSCA_OUTPUT].setVoltage(std::abs(a - c));
		outputs[ABSCB_OUTPUT].setVoltage(std::abs(b - c));
		outputs[ABSDA_OUTPUT].setVoltage(std::abs(a - d));
		outputs[ABSDB_OUTPUT].setVoltage(std::abs(b - d));
		outputs[ABSDC_OUTPUT].setVoltage(std::abs(c - d));

		// find the two channels with the highest amplitude
		float max_a = 0.f;
		float max_b = 0.f;
		for (int i = 0; i < CHANNEL_COUNT; i++) {
			if (envelope[i].env > max_a) {
				max_b = max_a;
				max_a = envelope[i].env;
			}
			else if (envelope[i].env > max_b) {
				max_b = envelope[i].env;
			}
		}
		outputs[TOPAVG_OUTPUT].setVoltage((max_a + max_b) / 2.f);

		// find the two channels with the lowest amplitude
		float min_a = 10.f;
		float min_b = 10.f;
		for (int i = 0; i < CHANNEL_COUNT; i++) {
			if (envelope[i].env < min_a) {
				min_b = min_a;
				min_a = envelope[i].env;
			}
			else if (envelope[i].env < min_b) {
				min_b = envelope[i].env;
			}
		}
		outputs[BOTAVG_OUTPUT].setVoltage((min_a + min_b) / 2.f);

		for (int i = 0; i < CHANNEL_COUNT; i++) {
			lights[i].setBrightness(envelope[i].env / 10.f);
			// lights[CASCADE_LIGHT + i].setBrightness(cm_envelope[i].env / 10.f);
			lights[CASCADE_LIGHT].setBrightness(cm_envelope.env / 10.f);
		}
	}

	float convert_param_to_multiplier(int param) {
		float multiplier = 1.f;
		switch (param) {
			case 0:
				multiplier = 0.5f;
				break;
			case 1:
				multiplier = 1.f;
				break;
			case 2:
				multiplier = 2.f;
				break;
		}
		return multiplier;
	}

	void shuffle(int array[], int len) {
		int temp = 0;
		for (int i = 0; i < len; i++) {
			temp = array[i];
			int rand = random::u32() % len;
			array[i] = array[rand];
			array[rand] = temp;
		}
	}

	void start_cycle() {
		switch (mode) {
			case EACH:
			 	current_index = 0;
				start_envelope(current_index);
				break;
			case SHUFFLE:
				shuffle(shuffle_list, CHANNEL_COUNT);
				current_index = shuffle_list[0];
				start_envelope(current_index);
				break;
			case RANDOM:
				// start a random envelope
				start_envelope(random::u32() % CHANNEL_COUNT);
				break;
		}
	}

	void start_envelope(int index) {
		current_index = index;
		DEBUG("start_envelope %d", index);
		cm_envelope.trigger();
	}

	void end_envelope(int index) {
		bool loop = params[LOOP_PARAM].getValue() > 0.5f;
		switch (mode) {
			case EACH:
				if (index == 3) {
					start_cycle();
				}
				else {
					current_index = index + 1;
					start_envelope(current_index);
				}
				break;
			case SHUFFLE:
				if (index == shuffle_list[3]) {
					end_cycle(loop);
				}
				else {
					start_envelope(shuffle_list[index + 1]);
				}
				break;
			case RANDOM:
				end_cycle(loop);
				break;
		}
	}

	void end_cycle(bool loop) {
		if (loop) {
			start_cycle();
		}
		else {
			// reset all envelopes
			for (int i = 0; i < CHANNEL_COUNT; i++) {
				cm_envelope[i].reset();
			}
		}
	}
};


struct FuncgenWidget : ModuleWidget {
	FuncgenWidget(Funcgen* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/Cascade.svg")));

		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<NP::Knob>(mm2px(Vec(11.812, 36.657)), module, Funcgen::RISE_PARAM + 0));
		addParam(createParamCentered<NP::SmallKnob>(mm2px(Vec(20.626, 32.424)), module, Funcgen::RISE_SHAPE_PARAM + 0));
		addParam(createParamCentered<NP::SmallKnob>(mm2px(Vec(27.506, 32.424)), module, Funcgen::FALL_SHAPE_PARAM + 0));
		addParam(createParamCentered<NP::Knob>(mm2px(Vec(36.572, 36.657)), module, Funcgen::FALL_PARAM + 0));
		addParam(createParamCentered<NP::LoopSwitch>(mm2px(Vec(36.572, 15.665)), module, Funcgen::LOOP_PARAM + 0));
		addParam(createParamCentered<NP::SpeedSwitch>(mm2px(Vec(24.192, 44.891)), module, Funcgen::SPEED_PARAM + 0));

		addParam(createParamCentered<NP::Knob>(mm2px(Vec(116.706, 36.657)), module, Funcgen::RISE_PARAM + 1));
		addParam(createParamCentered<NP::SmallKnob>(mm2px(Vec(125.521, 32.424)), module, Funcgen::RISE_SHAPE_PARAM + 1));
		addParam(createParamCentered<NP::SmallKnob>(mm2px(Vec(132.4, 32.424)), module, Funcgen::FALL_SHAPE_PARAM + 1));
		addParam(createParamCentered<NP::Knob>(mm2px(Vec(141.466, 36.657)), module, Funcgen::FALL_PARAM + 1));
		addParam(createParamCentered<NP::LoopSwitch>(mm2px(Vec(141.466, 15.665)), module, Funcgen::LOOP_PARAM + 1));
		addParam(createParamCentered<NP::SpeedSwitch>(mm2px(Vec(129.086, 44.891)), module, Funcgen::SPEED_PARAM + 1));

		addParam(createParamCentered<NP::Knob>(mm2px(Vec(11.812, 103.342)), module, Funcgen::RISE_PARAM + 3));
		addParam(createParamCentered<NP::SmallKnob>(mm2px(Vec(20.626, 99.108)), module, Funcgen::RISE_SHAPE_PARAM + 3));
		addParam(createParamCentered<NP::SmallKnob>(mm2px(Vec(27.506, 99.108)), module, Funcgen::FALL_SHAPE_PARAM + 3));
		addParam(createParamCentered<NP::Knob>(mm2px(Vec(36.572, 103.342)), module, Funcgen::FALL_PARAM + 3));
		addParam(createParamCentered<NP::LoopSwitch>(mm2px(Vec(36.572, 82.349)), module, Funcgen::LOOP_PARAM + 3));
		addParam(createParamCentered<NP::SpeedSwitch>(mm2px(Vec(24.192, 111.576)), module, Funcgen::SPEED_PARAM + 3));

		addParam(createParamCentered<NP::Knob>(mm2px(Vec(116.704, 103.342)), module, Funcgen::RISE_PARAM + 2));
		addParam(createParamCentered<NP::SmallKnob>(mm2px(Vec(125.519, 99.108)), module, Funcgen::RISE_SHAPE_PARAM + 2));
		addParam(createParamCentered<NP::SmallKnob>(mm2px(Vec(132.398, 99.108)), module, Funcgen::FALL_SHAPE_PARAM + 2));
		addParam(createParamCentered<NP::Knob>(mm2px(Vec(141.464, 103.342)), module, Funcgen::FALL_PARAM + 2));
		addParam(createParamCentered<NP::LoopSwitch>(mm2px(Vec(141.464, 82.349)), module, Funcgen::LOOP_PARAM + 2));
		addParam(createParamCentered<NP::SpeedSwitch>(mm2px(Vec(129.084, 111.576)), module, Funcgen::SPEED_PARAM + 2));

		addParam(createParamCentered<NP::Button>(mm2px(Vec(81.478, 43.048)), module, Funcgen::TRIGGER_ALL_PARAM));
		addParam(createParamCentered<NP::LoopSwitch>(mm2px(Vec(89.03, 59.95)), module, Funcgen::CASCADE_LOOP_PARAM));
		addParam(createParamCentered<NP::SpeedSwitch>(mm2px(Vec(97.077, 65.028)), module, Funcgen::CASCADE_SPEED_PARAM));
		addParam(createParamCentered<CKSSThree>(mm2px(Vec(55.767, 64.957)), module, Funcgen::MODE_PARAM));

		addInput(createInputCentered<NP::InPort>(mm2px(Vec(11.812, 15.465)), module, Funcgen::TRIGGER_INPUT + 0));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(11.812, 47.572)), module, Funcgen::RISE_CV_INPUT + 0));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(36.572, 47.572)), module, Funcgen::FALL_CV_INPUT + 0));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(116.706, 15.465)), module, Funcgen::TRIGGER_INPUT + 1));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(116.706, 47.572)), module, Funcgen::RISE_CV_INPUT + 1));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(141.466, 47.572)), module, Funcgen::FALL_CV_INPUT + 1));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(11.812, 82.15)), module, Funcgen::TRIGGER_INPUT + 3));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(11.812, 114.257)), module, Funcgen::RISE_CV_INPUT + 3));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(36.572, 114.257)), module, Funcgen::FALL_CV_INPUT + 3));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(116.704, 82.15)), module, Funcgen::TRIGGER_INPUT + 2));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(116.704, 114.257)), module, Funcgen::RISE_CV_INPUT + 2));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(141.464, 114.257)), module, Funcgen::FALL_CV_INPUT + 2));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(64.247, 59.678)), module, Funcgen::CASCADE_TRIGGER_INPUT));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(72.146, 42.982)), module, Funcgen::TRIGGER_ALL_INPUT));

		addOutput(createOutputCentered<NP::OutPort>(mm2px(Vec(11.812, 24.934)), module, Funcgen::RISING_OUTPUT + 0));
		addOutput(createOutputCentered<NP::OutPort>(mm2px(Vec(24.163, 15.465)), module, Funcgen::FUNCTION_OUTPUT + 0));
		addOutput(createOutputCentered<NP::OutPort>(mm2px(Vec(36.572, 25.018)), module, Funcgen::FALLING_OUTPUT + 0));
		addOutput(createOutputCentered<NP::OutPort>(mm2px(Vec(116.706, 24.934)), module, Funcgen::RISING_OUTPUT + 1));
		addOutput(createOutputCentered<NP::OutPort>(mm2px(Vec(129.058, 15.465)), module, Funcgen::FUNCTION_OUTPUT + 1));
		addOutput(createOutputCentered<NP::OutPort>(mm2px(Vec(141.466, 25.018)), module, Funcgen::FALLING_OUTPUT + 1));
		addOutput(createOutputCentered<NP::OutPort>(mm2px(Vec(11.812, 91.619)), module, Funcgen::RISING_OUTPUT + 3));
		addOutput(createOutputCentered<NP::OutPort>(mm2px(Vec(24.163, 82.15)), module, Funcgen::FUNCTION_OUTPUT + 3));
		addOutput(createOutputCentered<NP::OutPort>(mm2px(Vec(36.572, 91.702)), module, Funcgen::FALLING_OUTPUT + 3));
		addOutput(createOutputCentered<NP::OutPort>(mm2px(Vec(116.704, 91.619)), module, Funcgen::RISING_OUTPUT + 2));
		addOutput(createOutputCentered<NP::OutPort>(mm2px(Vec(129.055, 82.15)), module, Funcgen::FUNCTION_OUTPUT + 2));
		addOutput(createOutputCentered<NP::OutPort>(mm2px(Vec(141.464, 91.702)), module, Funcgen::FALLING_OUTPUT + 2));
		addOutput(createOutputCentered<NP::OutPort>(mm2px(Vec(60.649, 23.774)), module, Funcgen::AGTB_OUTPUT));
		addOutput(createOutputCentered<NP::OutPort>(mm2px(Vec(71.232, 23.774)), module, Funcgen::ABSAB_OUTPUT));
		addOutput(createOutputCentered<NP::OutPort>(mm2px(Vec(81.816, 23.774)), module, Funcgen::ABSBA_OUTPUT));
		addOutput(createOutputCentered<NP::OutPort>(mm2px(Vec(92.399, 23.774)), module, Funcgen::BGTA_OUTPUT));
		addOutput(createOutputCentered<NP::OutPort>(mm2px(Vec(113.089, 67.27)), module, Funcgen::BGTC_OUTPUT));
		addOutput(createOutputCentered<NP::OutPort>(mm2px(Vec(123.673, 67.27)), module, Funcgen::ABSBC_OUTPUT));
		addOutput(createOutputCentered<NP::OutPort>(mm2px(Vec(134.256, 67.27)), module, Funcgen::ABSCB_OUTPUT));
		addOutput(createOutputCentered<NP::OutPort>(mm2px(Vec(144.839, 67.27)), module, Funcgen::CGTB_OUTPUT));
		addOutput(createOutputCentered<NP::OutPort>(mm2px(Vec(60.451, 110.876)), module, Funcgen::CGTD_OUTPUT));
		addOutput(createOutputCentered<NP::OutPort>(mm2px(Vec(71.034, 110.876)), module, Funcgen::ABSCD_OUTPUT));
		addOutput(createOutputCentered<NP::OutPort>(mm2px(Vec(81.618, 110.876)), module, Funcgen::ABSDC_OUTPUT));
		addOutput(createOutputCentered<NP::OutPort>(mm2px(Vec(92.201, 110.876)), module, Funcgen::DGTC_OUTPUT));
		addOutput(createOutputCentered<NP::OutPort>(mm2px(Vec(7.568, 67.27)), module, Funcgen::AGTD_OUTPUT));
		addOutput(createOutputCentered<NP::OutPort>(mm2px(Vec(18.151, 67.27)), module, Funcgen::ABSAD_OUTPUT));
		addOutput(createOutputCentered<NP::OutPort>(mm2px(Vec(28.735, 67.27)), module, Funcgen::ABSDA_OUTPUT));
		addOutput(createOutputCentered<NP::OutPort>(mm2px(Vec(39.318, 67.27)), module, Funcgen::DGTA_OUTPUT));
		addOutput(createOutputCentered<NP::OutPort>(mm2px(Vec(56.948, 90.039)), module, Funcgen::MIN_OUTPUT));
		addOutput(createOutputCentered<NP::OutPort>(mm2px(Vec(66.734, 89.254)), module, Funcgen::BOTAVG_OUTPUT));
		addOutput(createOutputCentered<NP::OutPort>(mm2px(Vec(76.524, 88.46)), module, Funcgen::AVG_OUTPUT));
		addOutput(createOutputCentered<NP::OutPort>(mm2px(Vec(86.314, 87.666)), module, Funcgen::TOPAVG_OUTPUT));
		addOutput(createOutputCentered<NP::OutPort>(mm2px(Vec(96.107, 86.864)), module, Funcgen::MAX_OUTPUT));
		addOutput(createOutputCentered<NP::OutPort>(mm2px(Vec(76.621, 59.826)), module, Funcgen::CASCADE_OUTPUT));
		addOutput(createOutputCentered<NP::OutPort>(mm2px(Vec(64.27, 69.295)), module, Funcgen::CASCADE_RISING_OUTPUT));
		addOutput(createOutputCentered<NP::OutPort>(mm2px(Vec(89.033, 69.37)), module, Funcgen::CASCADE_FALLING_OUTPUT));

		Vec pos;

		// mm2px(Vec(12.531, 7.897))
		//addChild(createWidget<Widget>(mm2px(Vec(17.846, 21.056))));		
		pos = mm2px(Vec(12.531, 7.897) * 0.5 + Vec(17.846, 21.056));
		addChild(createLightCentered<LargeLight<RedLight>>(pos, module, Funcgen::OUTPUT_LIGHT + 0));
		// mm2px(Vec(12.531, 7.897))
		//addChild(createWidget<Widget>(mm2px(Vec(122.741, 21.056))));
		pos = mm2px(Vec(12.531, 7.897) * 0.5 + Vec(122.741, 21.056));
		addChild(createLightCentered<LargeLight<GreenLight>>(pos, module, Funcgen::OUTPUT_LIGHT + 1));
		// mm2px(Vec(12.531, 7.897))
		//addChild(createWidget<Widget>(mm2px(Vec(17.846, 87.74))));
		pos = mm2px(Vec(12.531, 7.897) * 0.5 + Vec(17.846, 87.74));
		addChild(createLightCentered<LargeLight<YellowLight>>(pos, module, Funcgen::OUTPUT_LIGHT + 3));
		// mm2px(Vec(12.531, 7.897))
		//addChild(createWidget<Widget>(mm2px(Vec(122.739, 87.74))));
		pos = mm2px(Vec(12.531, 7.897) * 0.5 + Vec(122.739, 87.74));
		addChild(createLightCentered<LargeLight<BlueLight>>(pos, module, Funcgen::OUTPUT_LIGHT + 2));
		// mm2px(Vec(12.531, 7.897))
		//addChild(createWidget<Widget>(mm2px(Vec(70.304, 65.287))));
		pos = mm2px(Vec(12.531, 7.897) * 0.5 + Vec(70.304, 65.287));
		addChild(createLightCentered<LargeLight<RedLight>>(pos, module, Funcgen::CASCADE_LIGHT + 0));
		addChild(createLightCentered<LargeLight<GreenLight>>(pos, module, Funcgen::CASCADE_LIGHT + 1));
		addChild(createLightCentered<LargeLight<BlueLight>>(pos, module, Funcgen::CASCADE_LIGHT + 3));
		addChild(createLightCentered<LargeLight<YellowLight>>(pos, module, Funcgen::CASCADE_LIGHT + 2));
	}
};


Model* modelFuncgen = createModel<Funcgen, FuncgenWidget>("funcgen");