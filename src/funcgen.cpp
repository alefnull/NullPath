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

	float range[CHANNEL_COUNT] = {10.f, 10.f, 10.f, 10.f};
	float last_range[CHANNEL_COUNT] = {10.f, 10.f, 10.f, 10.f};
	float range_cascade = 10.f;
	float last_range_cascade = 10.f;
	float range_ab = 10.f;
	float last_range_ab = 10.f;
	float range_bc = 10.f;
	float last_range_bc = 10.f;
	float range_cd = 10.f;
	float last_range_cd = 10.f;
	float range_da = 10.f;
	float last_range_da = 10.f;
	float range_all = 10.f;
	float last_range_all = 10.f;
	bool range_override = false;
	bool not_range_override = true;
	bool unipolar[CHANNEL_COUNT] = {true, true, true, true};
	bool last_unipolar[CHANNEL_COUNT] = {true, true, true, true};
	bool unipolar_cascade = true;
	bool last_unipolar_cascade = true;
	bool unipolar_ab = true;
	bool last_unipolar_ab = true;
	bool unipolar_bc = true;
	bool last_unipolar_bc = true;
	bool unipolar_cd = true;
	bool last_unipolar_cd = true;
	bool unipolar_da = true;
	bool last_unipolar_da = true;
	bool unipolar_all = true;
	bool last_unipolar_all = true;

	int current_index = 0;
	int shuffle_list[CHANNEL_COUNT] = {0,1,2,3};
	int shuffle_index = 0; //when in shuffle mode, stores which shuffle index is being played next

	Mode mode = EACH;
	ADEnvelope envelope[CHANNEL_COUNT];
	ADEnvelope cm_envelope;
	dsp::SchmittTrigger trigger[CHANNEL_COUNT];
	dsp::SchmittTrigger push[CHANNEL_COUNT];
	dsp::SchmittTrigger cascade_trigger;
	dsp::SchmittTrigger cascade_push;
	dsp::SchmittTrigger trigger_all;
	dsp::SchmittTrigger trigger_all_push;
	dsp::BooleanTrigger eoc_trigger[CHANNEL_COUNT];
	dsp::BooleanTrigger cm_eoc_trigger;
	dsp::BooleanTrigger loop_trigger[CHANNEL_COUNT];
	dsp::BooleanTrigger cm_loop_trigger;
	dsp::BooleanTrigger range_override_trigger;
	dsp::BooleanTrigger not_range_override_trigger;
	dsp::PulseGenerator eoc_pulse[CHANNEL_COUNT];


	Funcgen() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configSwitch(MODE_PARAM, 0.f, 2.f, 0.f, "Mode", {"Each", "Shuffle", "Random"});
		configInput(TRIGGER_ALL_INPUT, "Trigger all");
		configParam(TRIGGER_ALL_PARAM, 0.f, 1.f, 0.f, "Trigger all");
		configOutput(CASCADE_OUTPUT, "Cascade");
		configParam(CASCADE_TRIGGER_PARAM, 0.f, 1.f, 0.f, "Cascade Re-Trigger");
		configInput(CASCADE_TRIGGER_INPUT, "Cascade Re-Trigger");
		configSwitch(CASCADE_LOOP_PARAM, 0.f, 1.f, 1.f, "Cascade Loop", {"Off", "On"});
		getParamQuantity(CASCADE_LOOP_PARAM)->randomizeEnabled = false;
		configSwitch(CASCADE_SPEED_PARAM, 0.f, 2.f, 1.f, "Cascade Speed", {"Slow", "Normal", "Fast"});
		getParamQuantity(CASCADE_SPEED_PARAM)->randomizeEnabled = false;
		configOutput(CASCADE_RISING_OUTPUT, "Cascade Rising");
		configOutput(CASCADE_FALLING_OUTPUT, "Cascade Falling");
		for (int i = 0; i < CHANNEL_COUNT; i++) {
			configParam(RISE_PARAM + i, MIN_TIME, MAX_TIME, 1.f, "Rise time", " s");
			configParam(FALL_PARAM + i, MIN_TIME, MAX_TIME, 1.f, "Fall time", " s");
			configSwitch(LOOP_PARAM + i, 0.f, 1.f, 1.f, "Loop", {"Off", "On"});
			getParamQuantity(LOOP_PARAM + i)->randomizeEnabled = false;
			configParam(RISE_SHAPE_PARAM + i, -1.f, 1.f, 0.f, "Rise shape");
			configParam(FALL_SHAPE_PARAM + i, -1.f, 1.f, 0.f, "Fall shape");
			configSwitch(SPEED_PARAM + i, 0.f, 2.f, 1.f, "Speed", {"Slow", "Normal", "Fast"});
			getParamQuantity(SPEED_PARAM + i)->randomizeEnabled = false;
			configParam(PUSH_PARAM + i, 0.f, 1.f, 0.f, "Push");
			if (i == 0) {
				configInput(TRIGGER_INPUT + i, "Trigger A");
				configOutput(FUNCTION_OUTPUT + i, "Function A");
				configInput(RISE_CV_INPUT + i, "A Rise CV");
				getInputInfo(RISE_CV_INPUT + i)->description = "Expects a 0-10V signal";
				configInput(FALL_CV_INPUT + i, "A Fall CV");
				getInputInfo(FALL_CV_INPUT + i)->description = "Expects a 0-10V signal";
				configOutput(RISING_OUTPUT + i, "A Rising");
				configOutput(FALLING_OUTPUT + i, "A Falling");
			}
			else if (i == 1) {
				configInput(TRIGGER_INPUT + i, "Trigger B");
				configOutput(FUNCTION_OUTPUT + i, "Function B");
				configInput(RISE_CV_INPUT + i, "B Rise CV");
				getInputInfo(RISE_CV_INPUT + i)->description = "Expects a 0-10V signal";
				configInput(FALL_CV_INPUT + i, "B Fall CV");
				getInputInfo(FALL_CV_INPUT + i)->description = "Expects a 0-10V signal";
				configOutput(RISING_OUTPUT + i, "B Rising");
				configOutput(FALLING_OUTPUT + i, "B Falling");
			}
			else if (i == 2) {
				configInput(TRIGGER_INPUT + i, "Trigger C");
				configOutput(FUNCTION_OUTPUT + i, "Function C");
				configInput(RISE_CV_INPUT + i, "C Rise CV");
				getInputInfo(RISE_CV_INPUT + i)->description = "Expects a 0-10V signal";
				configInput(FALL_CV_INPUT + i, "C Fall CV");
				getInputInfo(FALL_CV_INPUT + i)->description = "Expects a 0-10V signal";
				configOutput(RISING_OUTPUT + i, "C Rising");
				configOutput(FALLING_OUTPUT + i, "C Falling");
			}
			else if (i == 3) {
				configInput(TRIGGER_INPUT + i, "Trigger D");
				configOutput(FUNCTION_OUTPUT + i, "Function D");
				configInput(RISE_CV_INPUT + i, "D Rise CV");
				getInputInfo(RISE_CV_INPUT + i)->description = "Expects a 0-10V signal";
				configInput(FALL_CV_INPUT + i, "D Fall CV");
				getInputInfo(FALL_CV_INPUT + i)->description = "Expects a 0-10V signal";
				configOutput(RISING_OUTPUT + i, "D Rising");
				configOutput(FALLING_OUTPUT + i, "D Falling");
			}
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
		not_range_override = !range_override;

		mode = Funcgen::Mode(params[MODE_PARAM].getValue());

		float cascade_speed = convert_param_to_multiplier(params[CASCADE_SPEED_PARAM].getValue());

		if (range_override_trigger.process(range_override)) {
			for (int i = 0; i < CHANNEL_COUNT; i++) {
				last_range[i] = range[i];
				range[i] = range_all;
				last_unipolar[i] = unipolar[i];
				unipolar[i] = unipolar_all;
			}
			last_range_cascade = range_cascade;
			range_cascade = range_all;
			last_unipolar_cascade = unipolar_cascade;
			unipolar_cascade = unipolar_all;
			last_range_ab = range_ab;
			range_ab = range_all;
			last_unipolar_ab = unipolar_ab;
			unipolar_ab = unipolar_all;
			last_range_bc = range_bc;
			range_bc = range_all;
			last_unipolar_bc = unipolar_bc;
			unipolar_bc = unipolar_all;
			last_range_cd = range_cd;
			range_cd = range_all;
			last_unipolar_cd = unipolar_cd;
			unipolar_cd = unipolar_all;
			last_range_da = range_da;
			range_da = range_all;
			last_unipolar_da = unipolar_da;
			unipolar_da = unipolar_all;
		}

		if (not_range_override_trigger.process(not_range_override)) {
			for (int i = 0; i < CHANNEL_COUNT; i++) {
				range[i] = last_range[i];
				unipolar[i] = last_unipolar[i];
			}
			range_cascade = last_range_cascade;
			unipolar_cascade = last_unipolar_cascade;
			range_ab = last_range_ab;
			unipolar_ab = last_unipolar_ab;
			range_bc = last_range_bc;
			unipolar_bc = last_unipolar_bc;
			range_cd = last_range_cd;
			unipolar_cd = last_unipolar_cd;
			range_da = last_range_da;
			unipolar_da = last_unipolar_da;
		}

		for (int i = 0; i < CHANNEL_COUNT; i++) {
			float attack_time = params[RISE_PARAM + i].getValue();
			float decay_time = params[FALL_PARAM + i].getValue();
			float speed = convert_param_to_multiplier(params[SPEED_PARAM + i].getValue());
			float attack_shape = params[RISE_SHAPE_PARAM + i].getValue();
			float decay_shape = params[FALL_SHAPE_PARAM + i].getValue();
			envelope[i].set_attack_shape(attack_shape);
			envelope[i].set_attack(attack_time * speed);
			envelope[i].set_decay_shape(decay_shape);
			envelope[i].set_decay(decay_time * speed);
			if(i == current_index){
				cm_envelope.set_attack_shape(attack_shape);
				cm_envelope.set_attack(attack_time * cascade_speed);
				cm_envelope.set_decay_shape(decay_shape);
				cm_envelope.set_decay(decay_time * cascade_speed);
			}

			if (inputs[RISE_CV_INPUT + i].isConnected()) {
				attack_time = clamp(attack_time * (inputs[RISE_CV_INPUT + i].getVoltage() / 10.f), MIN_TIME, MAX_TIME);
				envelope[i].set_attack(attack_time);
				cm_envelope.set_attack(attack_time);
			}

			if (inputs[FALL_CV_INPUT + i].isConnected()) {
				decay_time = clamp(decay_time * (inputs[FALL_CV_INPUT + i].getVoltage() / 10.f), MIN_TIME, MAX_TIME);
				envelope[i].set_decay(decay_time);
				cm_envelope.set_decay(decay_time);
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
			if (i == current_index) {
				cm_envelope.process(st);
			}

			if (eoc_trigger[i].process(envelope[i].eoc)) {
				eoc_pulse[i].trigger(1e-3f);
			}

			if (unipolar[i]) {
				outputs[FUNCTION_OUTPUT + i].setVoltage(range[i] * envelope[i].env);
			}
			else {
				outputs[FUNCTION_OUTPUT + i].setVoltage(range[i] * 2 * (envelope[i].env - 0.5f));
			}

			outputs[RISING_OUTPUT + i].setVoltage(envelope[i].stage == ADEnvelope::ATTACK ? 10.f : 0.f);
			outputs[FALLING_OUTPUT + i].setVoltage(envelope[i].stage == ADEnvelope::DECAY ? 10.f : 0.f);
		}

		if (cm_loop_trigger.process(params[CASCADE_LOOP_PARAM].getValue())) {
			cm_envelope.retrigger();
		}

		if (cm_eoc_trigger.process(cm_envelope.eoc)) {
			end_envelope(current_index);
		}

		if (trigger_all.process(inputs[TRIGGER_ALL_INPUT].getVoltage()) || trigger_all_push.process(params[TRIGGER_ALL_PARAM].getValue())) {
			for (int i = 0; i < CHANNEL_COUNT; i++) {
				envelope[i].trigger();
			}
			start_cycle();
		}

		float cascade_output = 0.f;
		
		cascade_output = cm_envelope.env;

		if (unipolar_cascade) {
			outputs[CASCADE_OUTPUT].setVoltage(range_cascade * cascade_output);
		}
		else {
			outputs[CASCADE_OUTPUT].setVoltage(range_cascade * 2 * (cascade_output - 0.5f));
		}
		outputs[CASCADE_RISING_OUTPUT].setVoltage(cm_envelope.stage == ADEnvelope::ATTACK ? 10.f : 0.f);
		outputs[CASCADE_FALLING_OUTPUT].setVoltage(cm_envelope.stage == ADEnvelope::DECAY ? 10.f : 0.f);

		if (cascade_trigger.process(inputs[CASCADE_TRIGGER_INPUT].getVoltage())) {
			start_cycle();
		}

		float a = envelope[0].env;
		float b = envelope[1].env;
		float c = envelope[2].env;
		float d = envelope[3].env;
		outputs[MIN_OUTPUT].setVoltage(std::min(a * range[0], std::min(b * range[1], std::min(c * range[2], d * range[3]))));
		outputs[MAX_OUTPUT].setVoltage(std::max(a * range[0], std::max(b * range[1], std::max(c * range[2], d * range[3]))));
		outputs[AVG_OUTPUT].setVoltage((a * range[0] + b * range[1] + c * range[2] + d * range[3]) / CHANNEL_COUNT);
		outputs[AGTB_OUTPUT].setVoltage(a > b ? 10.f : 0.f);
		outputs[AGTD_OUTPUT].setVoltage(a > d ? 10.f : 0.f);
		outputs[BGTA_OUTPUT].setVoltage(b > a ? 10.f : 0.f);
		outputs[BGTC_OUTPUT].setVoltage(b > c ? 10.f : 0.f);
		outputs[CGTB_OUTPUT].setVoltage(c > b ? 10.f : 0.f);
		outputs[CGTD_OUTPUT].setVoltage(c > d ? 10.f : 0.f);
		outputs[DGTA_OUTPUT].setVoltage(d > a ? 10.f : 0.f);
		outputs[DGTC_OUTPUT].setVoltage(d > c ? 10.f : 0.f);

		if (unipolar_ab) {
			// max is range_ab, min is 0
			outputs[ABSAB_OUTPUT].setVoltage(range_ab - std::abs(a * range_ab - b * range_ab));
			outputs[ABSBA_OUTPUT].setVoltage(std::abs(a * range_ab - b * range_ab));
		}
		else {
			// max is range_ab, min is -range_ab
			outputs[ABSAB_OUTPUT].setVoltage((1 - 2 * std::abs(a - b)) * range_ab);
			outputs[ABSBA_OUTPUT].setVoltage((std::abs(a - b) * 2 - 1) * range_ab);
		}
		if (unipolar_bc) {
			// max is range_bc, min is 0
			outputs[ABSBC_OUTPUT].setVoltage(range_bc - std::abs(b * range_bc - c * range_bc));
			outputs[ABSCB_OUTPUT].setVoltage(std::abs(b * range_bc - c * range_bc));
		}
		else {
			// max is range_bc, min is -range_bc
			outputs[ABSBC_OUTPUT].setVoltage((1 - 2 * std::abs(b - c)) * range_bc);
			outputs[ABSCB_OUTPUT].setVoltage((std::abs(b - c) * 2 - 1) * range_bc);
		}
		if (unipolar_cd) {
			// max is range_cd, min is 0
			outputs[ABSCD_OUTPUT].setVoltage(range_cd - std::abs(c * range_cd - d * range_cd));
			outputs[ABSDC_OUTPUT].setVoltage(std::abs(c * range_cd - d * range_cd));
		}
		else {
			// max is range_cd, min is -range_cd
			outputs[ABSCD_OUTPUT].setVoltage((1 - 2 * std::abs(c - d)) * range_cd);
			outputs[ABSDC_OUTPUT].setVoltage((std::abs(c - d) * 2 - 1) * range_cd);
		}
		if (unipolar_da) {
			// max is range_da, min is 0
			outputs[ABSAD_OUTPUT].setVoltage(range_da - std::abs(d * range_da - a * range_da));
			outputs[ABSDA_OUTPUT].setVoltage(std::abs(d * range_da - a * range_da));
		}
		else {
			// max is range_da, min is -range_da
			outputs[ABSAD_OUTPUT].setVoltage((1 - 2 * std::abs(d - a)) * range_da);
			outputs[ABSDA_OUTPUT].setVoltage((std::abs(d - a) * 2 - 1) * range_da);
		}

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
		outputs[TOPAVG_OUTPUT].setVoltage((max_a * 10 + max_b * 10) / 2);

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
		outputs[BOTAVG_OUTPUT].setVoltage((min_a * 10 + min_b * 10) / 2);

		for (int i = 0; i < CHANNEL_COUNT; i++) {
			lights[OUTPUT_LIGHT + i].setBrightness(envelope[i].env);
			lights[CASCADE_LIGHT  + i].setBrightness(i == current_index ? cm_envelope.env : 0.f);
		}
	}

	float convert_param_to_multiplier(int param) {
		float multiplier = 1.f;
		switch (param) {
			case 0:
				multiplier = 5.f;
				break;
			case 1:
				multiplier = 1.f;
				break;
			case 2:
				multiplier = 0.2f;
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
				start_envelope(0);
				break;
			case SHUFFLE:
				shuffle(shuffle_list, CHANNEL_COUNT);
				shuffle_index = 0;
				start_envelope(shuffle_list[shuffle_index]);
				break;
			case RANDOM:
				// start a random envelope
				start_envelope(random::u32() % CHANNEL_COUNT);
				break;
		}
	}

	void start_envelope(int index) {
		current_index = index;
		cm_envelope.trigger();
	}

	void end_envelope(int index) {
		switch (mode) {
			case EACH:
				if (index == 3) {
					end_cycle();
				}
				else {
					start_envelope(index + 1);
				}
				break;
			case SHUFFLE:
				if(shuffle_index == 3){
					end_cycle();
				}
				else{
					shuffle_index++;
					start_envelope(shuffle_list[shuffle_index]);
				}
				break;
			case RANDOM:
				end_cycle();
				break;
		}
	}

	void end_cycle() {
		if (params[CASCADE_LOOP_PARAM].getValue() > 0.5f) {
			start_cycle();
		}
	}

	void onReset() override {
		for (int i = 0; i < CHANNEL_COUNT; i++) {
			range[i] = 10;
			unipolar[i] = true;
		}
		range_cascade = 10;
		unipolar_cascade = true;
		
		for (int i = 0; i < CHANNEL_COUNT; i++) {
			envelope[i].reset();
		}
		cm_envelope.reset();
	}

	json_t *dataToJson() override {
		json_t *rootJ = json_object();
		json_object_set_new(rootJ, "range_a", json_integer(range[0]));
		json_object_set_new(rootJ, "unipolar_a", json_boolean(unipolar[0]));
		json_object_set_new(rootJ, "range_b", json_integer(range[1]));
		json_object_set_new(rootJ, "unipolar_b", json_boolean(unipolar[1]));
		json_object_set_new(rootJ, "range_c", json_integer(range[2]));
		json_object_set_new(rootJ, "unipolar_c", json_boolean(unipolar[2]));
		json_object_set_new(rootJ, "range_d", json_integer(range[3]));
		json_object_set_new(rootJ, "unipolar_d", json_boolean(unipolar[3]));
		json_object_set_new(rootJ, "range_cascade", json_integer(range_cascade));
		json_object_set_new(rootJ, "unipolar_cascade", json_boolean(unipolar_cascade));
		json_object_set_new(rootJ, "range_ab", json_integer(range_ab));
		json_object_set_new(rootJ, "unipolar_ab", json_boolean(unipolar_ab));
		json_object_set_new(rootJ, "range_bc", json_integer(range_bc));
		json_object_set_new(rootJ, "unipolar_bc", json_boolean(unipolar_bc));
		json_object_set_new(rootJ, "range_cd", json_integer(range_cd));
		json_object_set_new(rootJ, "unipolar_cd", json_boolean(unipolar_cd));
		json_object_set_new(rootJ, "range_da", json_integer(range_da));
		json_object_set_new(rootJ, "unipolar_da", json_boolean(unipolar_da));
		json_object_set_new(rootJ, "range_all", json_integer(range_all));
		json_object_set_new(rootJ, "unipolar_all", json_boolean(unipolar_all));
		json_object_set_new(rootJ, "range_override", json_boolean(range_override));
		return rootJ;
	}

	void dataFromJson(json_t *rootJ) override {
		json_t *range_aJ = json_object_get(rootJ, "range_a");
		if (range_aJ)
			range[0] = json_integer_value(range_aJ);
		json_t *unipolar_aJ = json_object_get(rootJ, "unipolar_a");
		if (unipolar_aJ)
			unipolar[0] = json_boolean_value(unipolar_aJ);
		json_t *range_bJ = json_object_get(rootJ, "range_b");
		if (range_bJ)
			range[1] = json_integer_value(range_bJ);
		json_t *unipolar_bJ = json_object_get(rootJ, "unipolar_b");
		if (unipolar_bJ)
			unipolar[1] = json_boolean_value(unipolar_bJ);
		json_t *range_cJ = json_object_get(rootJ, "range_c");
		if (range_cJ)
			range[2] = json_integer_value(range_cJ);
		json_t *unipolar_cJ = json_object_get(rootJ, "unipolar_c");
		if (unipolar_cJ)
			unipolar[2] = json_boolean_value(unipolar_cJ);
		json_t *range_dJ = json_object_get(rootJ, "range_d");
		if (range_dJ)
			range[3] = json_integer_value(range_dJ);
		json_t *unipolar_dJ = json_object_get(rootJ, "unipolar_d");
		if (unipolar_dJ)
			unipolar[3] = json_boolean_value(unipolar_dJ);
		json_t *range_cascadeJ = json_object_get(rootJ, "range_cascade");
		if (range_cascadeJ)
			range_cascade = json_integer_value(range_cascadeJ);
		json_t *unipolar_cascadeJ = json_object_get(rootJ, "unipolar_cascade");
		if (unipolar_cascadeJ)
			unipolar_cascade = json_boolean_value(unipolar_cascadeJ);
		json_t *range_abJ = json_object_get(rootJ, "range_ab");
		if (range_abJ)
			range_ab = json_integer_value(range_abJ);
		json_t *unipolar_abJ = json_object_get(rootJ, "unipolar_ab");
		if (unipolar_abJ)
			unipolar_ab = json_boolean_value(unipolar_abJ);
		json_t *range_bcJ = json_object_get(rootJ, "range_bc");
		if (range_bcJ)
			range_bc = json_integer_value(range_bcJ);
		json_t *unipolar_bcJ = json_object_get(rootJ, "unipolar_bc");
		if (unipolar_bcJ)
			unipolar_bc = json_boolean_value(unipolar_bcJ);
		json_t *range_cdJ = json_object_get(rootJ, "range_cd");
		if (range_cdJ)
			range_cd = json_integer_value(range_cdJ);
		json_t *unipolar_cdJ = json_object_get(rootJ, "unipolar_cd");
		if (unipolar_cdJ)
			unipolar_cd = json_boolean_value(unipolar_cdJ);
		json_t *range_daJ = json_object_get(rootJ, "range_da");
		if (range_daJ)
			range_da = json_integer_value(range_daJ);
		json_t *unipolar_daJ = json_object_get(rootJ, "unipolar_da");
		if (unipolar_daJ)
			unipolar_da = json_boolean_value(unipolar_daJ);
		json_t *range_allJ = json_object_get(rootJ, "range_all");
		if (range_allJ)
			range_all = json_integer_value(range_allJ);
		json_t *unipolar_allJ = json_object_get(rootJ, "unipolar_all");
		if (unipolar_allJ)
			unipolar_all = json_boolean_value(unipolar_allJ);
		json_t *range_overrideJ = json_object_get(rootJ, "range_override");
		if (range_overrideJ)
			range_override = json_boolean_value(range_overrideJ);

		for (int i = 0; i < CHANNEL_COUNT; i++) {
			if (params[LOOP_PARAM + i].getValue() > 0.5f) {
				envelope[i].trigger();
			}
		}
		if (params[CASCADE_LOOP_PARAM].getValue() > 0.5f) {
			start_cycle();
		}
	}
};

static const NVGcolor CC_COLOR_1 = nvgRGB(0xa0, 0xf9, 0xce);
static const NVGcolor CC_COLOR_2 = nvgRGB(0xf7, 0xf9, 0xa0);
static const NVGcolor CC_COLOR_3 = nvgRGB(0xf9, 0xce, 0xa0);
static const NVGcolor CC_COLOR_4 = nvgRGB(0xa0, 0xf9, 0xf9);

struct CCColor1 : GrayModuleLightWidget {
	CCColor1() {
		addBaseColor(CC_COLOR_1);
	}
};

struct CCColor2 : GrayModuleLightWidget {
	CCColor2() {
		addBaseColor(CC_COLOR_2);
	}
};

struct CCColor3 : GrayModuleLightWidget {
	CCColor3() {
		addBaseColor(CC_COLOR_3);
	}
};

struct CCColor4 : GrayModuleLightWidget {
	CCColor4() {
		addBaseColor(CC_COLOR_4);
	}
};

struct CCColors : GrayModuleLightWidget {
	CCColors() {
		addBaseColor(CC_COLOR_1);
		addBaseColor(CC_COLOR_2);
		addBaseColor(CC_COLOR_4);
		addBaseColor(CC_COLOR_3);
	}
};

struct CascadeModeSwitch : app::SvgSwitch {
	CascadeModeSwitch(){
		addFrame(Svg::load(asset::plugin(pluginInstance,"res/cascade_mode_switch_0.svg")));
		addFrame(Svg::load(asset::plugin(pluginInstance,"res/cascade_mode_switch_1.svg")));
		addFrame(Svg::load(asset::plugin(pluginInstance,"res/cascade_mode_switch_2.svg")));
		shadow->blurRadius = 0;
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
		addParam(createParamCentered<CascadeModeSwitch>(mm2px(Vec(55.767, 64.957)), module, Funcgen::MODE_PARAM));

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
		addChild(createLightCentered<LargeLight<CCColor1>>(pos, module, Funcgen::OUTPUT_LIGHT + 0));
		// mm2px(Vec(12.531, 7.897))
		//addChild(createWidget<Widget>(mm2px(Vec(122.741, 21.056))));
		pos = mm2px(Vec(12.531, 7.897) * 0.5 + Vec(122.741, 21.056));
		addChild(createLightCentered<LargeLight<CCColor2>>(pos, module, Funcgen::OUTPUT_LIGHT + 1));
		// mm2px(Vec(12.531, 7.897))
		//addChild(createWidget<Widget>(mm2px(Vec(17.846, 87.74))));
		pos = mm2px(Vec(12.531, 7.897) * 0.5 + Vec(17.846, 87.74));
		addChild(createLightCentered<LargeLight<CCColor3>>(pos, module, Funcgen::OUTPUT_LIGHT + 3));
		// mm2px(Vec(12.531, 7.897))
		//addChild(createWidget<Widget>(mm2px(Vec(122.739, 87.74))));
		pos = mm2px(Vec(12.531, 7.897) * 0.5 + Vec(122.739, 87.74));
		addChild(createLightCentered<LargeLight<CCColor4>>(pos, module, Funcgen::OUTPUT_LIGHT + 2));
		// mm2px(Vec(12.531, 7.897))
		//addChild(createWidget<Widget>(mm2px(Vec(70.304, 65.287))));
		pos = mm2px(Vec(12.531, 7.897) * 0.5 + Vec(70.304, 65.287));
		addChild(createLightCentered<LargeLight<CCColors>>(pos, module, Funcgen::CASCADE_LIGHT));
	}

	// override context menu to add range options
	void appendContextMenu(Menu *menu) override {
		Funcgen *module = dynamic_cast<Funcgen*>(this->module);
		assert(module);
		menu->addChild(new MenuSeparator());
		menu->addChild(createSubmenuItem("All Ranges", "", [=](Menu* menu) {
			Menu* rangeMenu = new Menu();
			rangeMenu->addChild(createMenuItem("Override individual range settings", CHECKMARK(module->range_override), [module]() { module->range_override = !module->range_override; }));
			rangeMenu->addChild(new MenuSeparator());
			rangeMenu->addChild(createMenuItem("+/- 1v", CHECKMARK(module->range_all == 1), [module]() { module->range_all = 1; }));
			rangeMenu->addChild(createMenuItem("+/- 2v", CHECKMARK(module->range_all == 2), [module]() { module->range_all = 2; }));
			rangeMenu->addChild(createMenuItem("+/- 3v", CHECKMARK(module->range_all == 3), [module]() { module->range_all = 3; }));
			rangeMenu->addChild(createMenuItem("+/- 5v", CHECKMARK(module->range_all == 5), [module]() { module->range_all = 5; }));
			rangeMenu->addChild(createMenuItem("+/- 10v", CHECKMARK(module->range_all == 10), [module]() { module->range_all = 10; }));
			rangeMenu->addChild(new MenuSeparator());
			rangeMenu->addChild(createMenuItem("Unipolar", CHECKMARK(module->unipolar_all), [module]() { module->unipolar_all = !module->unipolar_all; }));
			menu->addChild(rangeMenu);
		}));
		menu->addChild(new MenuSeparator());
		menu->addChild(createSubmenuItem("Channel A Range", "", [=](Menu* menu) {
			Menu* rangeMenu = new Menu();
			rangeMenu->addChild(createMenuItem("+/- 1v", CHECKMARK(module->range[0] == 1), [module]() { module->range[0] = 1; }));
			rangeMenu->addChild(createMenuItem("+/- 2v", CHECKMARK(module->range[0] == 2), [module]() { module->range[0] = 2; }));
			rangeMenu->addChild(createMenuItem("+/- 3v", CHECKMARK(module->range[0] == 3), [module]() { module->range[0] = 3; }));
			rangeMenu->addChild(createMenuItem("+/- 5v", CHECKMARK(module->range[0] == 5), [module]() { module->range[0] = 5; }));
			rangeMenu->addChild(createMenuItem("+/- 10v", CHECKMARK(module->range[0] == 10), [module]() { module->range[0] = 10; }));
			rangeMenu->addChild(new MenuSeparator());
			rangeMenu->addChild(createMenuItem("Unipolar", CHECKMARK(module->unipolar[0]), [module]() { module->unipolar[0] = !module->unipolar[0]; }));
			menu->addChild(rangeMenu);
		}));
		menu->addChild(createSubmenuItem("A <-> B Range", "", [=](Menu* menu) {
			Menu* rangeMenu = new Menu();
			rangeMenu->addChild(createMenuItem("+/- 1v", CHECKMARK(module->range_ab == 1), [module]() { module->range_ab = 1; }));
			rangeMenu->addChild(createMenuItem("+/- 2v", CHECKMARK(module->range_ab == 2), [module]() { module->range_ab = 2; }));
			rangeMenu->addChild(createMenuItem("+/- 3v", CHECKMARK(module->range_ab == 3), [module]() { module->range_ab = 3; }));
			rangeMenu->addChild(createMenuItem("+/- 5v", CHECKMARK(module->range_ab == 5), [module]() { module->range_ab = 5; }));
			rangeMenu->addChild(createMenuItem("+/- 10v", CHECKMARK(module->range_ab == 10), [module]() { module->range_ab = 10; }));
			rangeMenu->addChild(new MenuSeparator());
			rangeMenu->addChild(createMenuItem("Unipolar", CHECKMARK(module->unipolar_ab), [module]() { module->unipolar_ab = !module->unipolar_ab; }));
			menu->addChild(rangeMenu);
		}));
		menu->addChild(createSubmenuItem("Channel B Range", "", [=](Menu* menu) {
			Menu* rangeMenu = new Menu();
			rangeMenu->addChild(createMenuItem("+/- 1v", CHECKMARK(module->range[1] == 1), [module]() { module->range[1] = 1; }));
			rangeMenu->addChild(createMenuItem("+/- 2v", CHECKMARK(module->range[1] == 2), [module]() { module->range[1] = 2; }));
			rangeMenu->addChild(createMenuItem("+/- 3v", CHECKMARK(module->range[1] == 3), [module]() { module->range[1] = 3; }));
			rangeMenu->addChild(createMenuItem("+/- 5v", CHECKMARK(module->range[1] == 5), [module]() { module->range[1] = 5; }));
			rangeMenu->addChild(createMenuItem("+/- 10v", CHECKMARK(module->range[1] == 10), [module]() { module->range[1] = 10; }));
			rangeMenu->addChild(new MenuSeparator());
			rangeMenu->addChild(createMenuItem("Unipolar", CHECKMARK(module->unipolar[1]), [module]() { module->unipolar[1] = !module->unipolar[1]; }));
			menu->addChild(rangeMenu);
		}));
		menu->addChild(createSubmenuItem("B <-> C Range", "", [=](Menu* menu) {
			Menu* rangeMenu = new Menu();
			rangeMenu->addChild(createMenuItem("+/- 1v", CHECKMARK(module->range_bc == 1), [module]() { module->range_bc = 1; }));
			rangeMenu->addChild(createMenuItem("+/- 2v", CHECKMARK(module->range_bc == 2), [module]() { module->range_bc = 2; }));
			rangeMenu->addChild(createMenuItem("+/- 3v", CHECKMARK(module->range_bc == 3), [module]() { module->range_bc = 3; }));
			rangeMenu->addChild(createMenuItem("+/- 5v", CHECKMARK(module->range_bc == 5), [module]() { module->range_bc = 5; }));
			rangeMenu->addChild(createMenuItem("+/- 10v", CHECKMARK(module->range_bc == 10), [module]() { module->range_bc = 10; }));
			rangeMenu->addChild(new MenuSeparator());
			rangeMenu->addChild(createMenuItem("Unipolar", CHECKMARK(module->unipolar_bc), [module]() { module->unipolar_bc = !module->unipolar_bc; }));
			menu->addChild(rangeMenu);
		}));
		menu->addChild(createSubmenuItem("Channel C Range", "", [=](Menu* menu) {
			Menu* rangeMenu = new Menu();
			rangeMenu->addChild(createMenuItem("+/- 1v", CHECKMARK(module->range[2] == 1), [module]() { module->range[2] = 1; }));
			rangeMenu->addChild(createMenuItem("+/- 2v", CHECKMARK(module->range[2] == 2), [module]() { module->range[2] = 2; }));
			rangeMenu->addChild(createMenuItem("+/- 3v", CHECKMARK(module->range[2] == 3), [module]() { module->range[2] = 3; }));
			rangeMenu->addChild(createMenuItem("+/- 5v", CHECKMARK(module->range[2] == 5), [module]() { module->range[2] = 5; }));
			rangeMenu->addChild(createMenuItem("+/- 10v", CHECKMARK(module->range[2] == 10), [module]() { module->range[2] = 10; }));
			rangeMenu->addChild(new MenuSeparator());
			rangeMenu->addChild(createMenuItem("Unipolar", CHECKMARK(module->unipolar[2]), [module]() { module->unipolar[2] = !module->unipolar[2]; }));
			menu->addChild(rangeMenu);
		}));
		menu->addChild(createSubmenuItem("C <-> D Range", "", [=](Menu* menu) {
			Menu* rangeMenu = new Menu();
			rangeMenu->addChild(createMenuItem("+/- 1v", CHECKMARK(module->range_cd == 1), [module]() { module->range_cd = 1; }));
			rangeMenu->addChild(createMenuItem("+/- 2v", CHECKMARK(module->range_cd == 2), [module]() { module->range_cd = 2; }));
			rangeMenu->addChild(createMenuItem("+/- 3v", CHECKMARK(module->range_cd == 3), [module]() { module->range_cd = 3; }));
			rangeMenu->addChild(createMenuItem("+/- 5v", CHECKMARK(module->range_cd == 5), [module]() { module->range_cd = 5; }));
			rangeMenu->addChild(createMenuItem("+/- 10v", CHECKMARK(module->range_cd == 10), [module]() { module->range_cd = 10; }));
			rangeMenu->addChild(new MenuSeparator());
			rangeMenu->addChild(createMenuItem("Unipolar", CHECKMARK(module->unipolar_cd), [module]() { module->unipolar_cd = !module->unipolar_cd; }));
			menu->addChild(rangeMenu);
		}));
		menu->addChild(createSubmenuItem("Channel D Range", "", [=](Menu* menu) {
			Menu* rangeMenu = new Menu();
			rangeMenu->addChild(createMenuItem("+/- 1v", CHECKMARK(module->range[3] == 1), [module]() { module->range[3] = 1; }));
			rangeMenu->addChild(createMenuItem("+/- 2v", CHECKMARK(module->range[3] == 2), [module]() { module->range[3] = 2; }));
			rangeMenu->addChild(createMenuItem("+/- 3v", CHECKMARK(module->range[3] == 3), [module]() { module->range[3] = 3; }));
			rangeMenu->addChild(createMenuItem("+/- 5v", CHECKMARK(module->range[3] == 5), [module]() { module->range[3] = 5; }));
			rangeMenu->addChild(createMenuItem("+/- 10v", CHECKMARK(module->range[3] == 10), [module]() { module->range[3] = 10; }));
			rangeMenu->addChild(new MenuSeparator());
			rangeMenu->addChild(createMenuItem("Unipolar", CHECKMARK(module->unipolar[3]), [module]() { module->unipolar[3] = !module->unipolar[3]; }));
			menu->addChild(rangeMenu);
		}));
		menu->addChild(createSubmenuItem("D <-> A Range", "", [=](Menu* menu) {
			Menu* rangeMenu = new Menu();
			rangeMenu->addChild(createMenuItem("+/- 1v", CHECKMARK(module->range_da == 1), [module]() { module->range_da = 1; }));
			rangeMenu->addChild(createMenuItem("+/- 2v", CHECKMARK(module->range_da == 2), [module]() { module->range_da = 2; }));
			rangeMenu->addChild(createMenuItem("+/- 3v", CHECKMARK(module->range_da == 3), [module]() { module->range_da = 3; }));
			rangeMenu->addChild(createMenuItem("+/- 5v", CHECKMARK(module->range_da == 5), [module]() { module->range_da = 5; }));
			rangeMenu->addChild(createMenuItem("+/- 10v", CHECKMARK(module->range_da == 10), [module]() { module->range_da = 10; }));
			rangeMenu->addChild(new MenuSeparator());
			rangeMenu->addChild(createMenuItem("Unipolar", CHECKMARK(module->unipolar_da), [module]() { module->unipolar_da = !module->unipolar_da; }));
			menu->addChild(rangeMenu);
		}));
		menu->addChild(createSubmenuItem("Cascade Range", "", [=](Menu* menu) {
			Menu* rangeMenu = new Menu();
			rangeMenu->addChild(createMenuItem("+/- 1v", CHECKMARK(module->range_cascade == 1), [module]() { module->range_cascade = 1; }));
			rangeMenu->addChild(createMenuItem("+/- 2v", CHECKMARK(module->range_cascade == 2), [module]() { module->range_cascade = 2; }));
			rangeMenu->addChild(createMenuItem("+/- 3v", CHECKMARK(module->range_cascade == 3), [module]() { module->range_cascade = 3; }));
			rangeMenu->addChild(createMenuItem("+/- 5v", CHECKMARK(module->range_cascade == 5), [module]() { module->range_cascade = 5; }));
			rangeMenu->addChild(createMenuItem("+/- 10v", CHECKMARK(module->range_cascade == 10), [module]() { module->range_cascade = 10; }));
			rangeMenu->addChild(new MenuSeparator());
			rangeMenu->addChild(createMenuItem("Unipolar", CHECKMARK(module->unipolar_cascade), [module]() { module->unipolar_cascade = !module->unipolar_cascade; }));
			menu->addChild(rangeMenu);
		}));
	}
};


Model* modelFuncgen = createModel<Funcgen, FuncgenWidget>("funcgen");