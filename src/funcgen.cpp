#include "plugin.hpp"
#include "Envelope.hpp"


#define CHANNEL_COUNT 4
#define ENV_MAX_VOLTAGE 10.f

struct Funcgen : Module {
	enum ParamId {
		ENUMS(RISE_PARAM, CHANNEL_COUNT),
		ENUMS(LOOP_PARAM, CHANNEL_COUNT),
		ENUMS(FALL_PARAM, CHANNEL_COUNT),
		ENUMS(PUSH_PARAM, CHANNEL_COUNT),
		MODE_PARAM,
		CASCADE_TRIGGER_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		ENUMS(TRIGGER_INPUT, CHANNEL_COUNT),
		ENUMS(RISE_CV_INPUT, CHANNEL_COUNT),
		ENUMS(FALL_CV_INPUT, CHANNEL_COUNT),
		ENUMS(TAH_GATE_INPUT, CHANNEL_COUNT),
		CASCADE_TRIGGER_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		ENUMS(FUNCTION_OUTPUT, CHANNEL_COUNT),
		ENUMS(EOC_OUTPUT, CHANNEL_COUNT),
		ENUMS(TAH_OUTPUT, CHANNEL_COUNT),
		CASCADE_OUTPUT,
		CASCADE_EOC_OUTPUT,
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
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};
	enum Stage {
		IDLE,
		RISING,
		FALLING
	};
	enum Mode {
		NORMAL,
		CASCADE,
		CHAOTIC_CASCADE,
	};

	Stage stage = IDLE;
	Mode mode = NORMAL;
	
	Envelope envelope[CHANNEL_COUNT];

	float tah_value[CHANNEL_COUNT];

	dsp::SchmittTrigger trigger[CHANNEL_COUNT];
	dsp::SchmittTrigger push[CHANNEL_COUNT];
	dsp::SchmittTrigger cascade_trigger;
	dsp::SchmittTrigger cascade_push;
	dsp::SchmittTrigger tah_trigger[CHANNEL_COUNT];
	dsp::BooleanTrigger normal_mode_trigger;
	dsp::BooleanTrigger eoc_trigger[CHANNEL_COUNT];
	dsp::PulseGenerator eoc_pulse[CHANNEL_COUNT];
	dsp::PulseGenerator cascade_eoc_pulse;

	bool normal_mode = true;
	int chaos_index = 0;

	Funcgen() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configSwitch(MODE_PARAM, 0.f, 2.f, 0.f, "Mode", {"Normal", "Cascade", "Chaotic Cascade"});
		configOutput(CASCADE_EOC_OUTPUT, "Cascade EOC");
		configOutput(CASCADE_OUTPUT, "Cascade");
		for (int i = 0; i < CHANNEL_COUNT; i++) {
			configParam(RISE_PARAM + i, 0.01f, 10.f, 0.01f, "Rise time", " s");
			configParam(FALL_PARAM + i, 0.01f, 10.f, 0.01f, "Fall time", " s");
			configSwitch(LOOP_PARAM + i, 0.f, 1.f, 0.f, "Loop");
			configParam(CASCADE_TRIGGER_PARAM, 0.f, 1.f, 0.f, "Cascade push");
			configParam(PUSH_PARAM + i, 0.f, 1.f, 0.f, "Push");
			configInput(TRIGGER_INPUT + i, "Trigger");
			configInput(RISE_CV_INPUT + i, "Rise CV");
			configInput(FALL_CV_INPUT + i, "Fall CV");
			configInput(CASCADE_TRIGGER_INPUT, "Cascade trigger");
			configOutput(FUNCTION_OUTPUT + i, "Function");
			configOutput(EOC_OUTPUT + i, "EOC");
			configInput(TAH_GATE_INPUT + i, "Track & Hold gate");
			configOutput(TAH_OUTPUT + i, "Track & Hold");
		}
		configOutput(MIN_OUTPUT, "Minimum");
		configOutput(MAX_OUTPUT, "Maximum");
		configOutput(AVG_OUTPUT, "Average");
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
		if (params[MODE_PARAM].getValue() == 0.f) {
			normal_mode = true;
			mode = NORMAL;
		}
		else if (params[MODE_PARAM].getValue() == 1.f) {
			normal_mode = false;
			mode = CASCADE;
		}
		else if (params[MODE_PARAM].getValue() == 2.f) {
			normal_mode = false;
			mode = CHAOTIC_CASCADE;
		}

		for (int i = 0; i < CHANNEL_COUNT; i++) {
			float rise_time = params[RISE_PARAM + i].getValue();
			float fall_time = params[FALL_PARAM + i].getValue();
			envelope[i].set_rise(rise_time);
			envelope[i].set_fall(fall_time);

			if (inputs[RISE_CV_INPUT].isConnected()) {
				rise_time = clamp(rise_time * inputs[RISE_CV_INPUT + i].getVoltage() / 10.f, 0.01f, 10.f);
				envelope[i].set_rise(rise_time);
			}

			if (inputs[FALL_CV_INPUT].isConnected()) {
				fall_time = clamp(fall_time * inputs[FALL_CV_INPUT + i].getVoltage() / 10.f, 0.01f, 10.f);
				envelope[i].set_fall(fall_time);
			}

			bool loop = params[LOOP_PARAM + i].getValue() > 0.5f;
			envelope[i].set_loop(loop && mode == NORMAL);

			if (mode == NORMAL) {
				if (trigger[i].process(inputs[TRIGGER_INPUT + i].getVoltage()) || push[i].process(params[PUSH_PARAM + i].getValue())) {
					envelope[i].retrigger();		
				}
			}


			if (tah_trigger[i].process(inputs[TAH_GATE_INPUT + i].getVoltage())) {
				tah_value[i] = envelope[i].env;
			}

			if (inputs[TAH_GATE_INPUT + i].getVoltage() > 0.f) {
				outputs[TAH_OUTPUT + i].setVoltage(tah_value[i]);
			}
			else {
				outputs[TAH_OUTPUT + i].setVoltage(envelope[i].env);
			}

			envelope[i].process(st);

			if (eoc_trigger[i].process(envelope[i].eoc)) {
				eoc_pulse[i].trigger(1e-3f);
			}

			outputs[FUNCTION_OUTPUT + i].setVoltage(envelope[i].env);


			bool eoc = eoc_pulse[i].process(st);
			outputs[EOC_OUTPUT + i].setVoltage(eoc ? 10.f : 0.f);
			if (eoc && mode == CASCADE) {
				envelope[(i + 1) % 4].retrigger();
			}
			else if (eoc && mode == CHAOTIC_CASCADE) {
				envelope[chaos_index].retrigger();
			}
		}

		float cascade_output = 0.f;
		if (mode == CASCADE) {
			cascade_output = std::max(envelope[0].env, envelope[1].env);
			cascade_output = std::max(cascade_output, envelope[2].env);
			cascade_output = std::max(cascade_output, envelope[3].env);
		}
		else if (mode == CHAOTIC_CASCADE) {
			cascade_output = envelope[chaos_index].env;
			if (eoc_pulse[chaos_index].process(st)) {
				int last_index = chaos_index;
				while (last_index == chaos_index) {
					chaos_index = random::u32() % CHANNEL_COUNT;
				}
				for (int i = 0; i < CHANNEL_COUNT; i++) {
					if (i != chaos_index) {
						envelope[i].reset();
					}
				}
			}
		}
		outputs[CASCADE_OUTPUT].setVoltage(cascade_output);

		if (mode == CASCADE) {
			outputs[CASCADE_EOC_OUTPUT].setVoltage(outputs[EOC_OUTPUT + 3].getVoltage());
		}
		else if (mode == CHAOTIC_CASCADE) {
			outputs[CASCADE_EOC_OUTPUT].setVoltage(outputs[EOC_OUTPUT + chaos_index].getVoltage());
		}
		else {
			outputs[CASCADE_EOC_OUTPUT].setVoltage(0.f);
		}

		if (normal_mode_trigger.process(normal_mode)) {
			for (int i = 0; i < CHANNEL_COUNT; i++) {
				envelope[i].reset();
			}
		}

		if (cascade_trigger.process(inputs[CASCADE_TRIGGER_INPUT].getVoltage() || cascade_push.process(params[CASCADE_TRIGGER_PARAM].getValue()))) {
			if (mode == CASCADE) {
				envelope[0].retrigger();
				envelope[1].reset();
				envelope[2].reset();
				envelope[3].reset();
			}
			else if (mode == CHAOTIC_CASCADE) {
				envelope[chaos_index].retrigger();
				for (int i = 0; i < CHANNEL_COUNT; i++) {
					if (i != chaos_index) {
						envelope[i].reset();
					}
				}
			}
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
	}
};


struct FuncgenWidget : ModuleWidget {
	FuncgenWidget(Funcgen* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/funcgen.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		float dx = RACK_GRID_WIDTH * 2;
		float dy = RACK_GRID_WIDTH * 2;

		float y_start = RACK_GRID_WIDTH;
		float x_start = RACK_GRID_WIDTH;

		float x = x_start;
		float y = y_start;

		for (int i = 0; i < CHANNEL_COUNT; i++) {
			x = x_start + 4 * dx * (i / 2) + dx;
			y = y_start + 5 * dy * (i % 2) + RACK_GRID_WIDTH;
			addParam(createParamCentered<TL1105>(Vec(x, y), module, Funcgen::PUSH_PARAM + i));
			x += dx;
			addInput(createInputCentered<PJ301MPort>(Vec(x, y), module, Funcgen::TRIGGER_INPUT + i));
			y += dy;
			x -= dx * 2;
			addParam(createParamCentered<RoundSmallBlackKnob>(Vec(x, y), module, Funcgen::RISE_PARAM + i));
			x += dx;
			addParam(createParamCentered<CKSS>(Vec(x, y), module, Funcgen::LOOP_PARAM + i));
			x += dx;
			addParam(createParamCentered<RoundSmallBlackKnob>(Vec(x, y), module, Funcgen::FALL_PARAM + i));
			y += dy;
			x -= dx * 2;
			addInput(createInputCentered<PJ301MPort>(Vec(x, y), module, Funcgen::RISE_CV_INPUT + i));
			x += dx;
			addOutput(createOutputCentered<PJ301MPort>(Vec(x, y), module, Funcgen::FUNCTION_OUTPUT + i));
			x += dx;
			addInput(createInputCentered<PJ301MPort>(Vec(x, y), module, Funcgen::FALL_CV_INPUT + i));
			y += dy;
			x -= dx * 2;
			addOutput(createOutputCentered<PJ301MPort>(Vec(x, y), module, Funcgen::EOC_OUTPUT + i));
			x += dx;
			addInput(createInputCentered<PJ301MPort>(Vec(x, y), module, Funcgen::TAH_GATE_INPUT + i));
			x += dx;
			addOutput(createOutputCentered<PJ301MPort>(Vec(x, y), module, Funcgen::TAH_OUTPUT + i));
		}
		x = x_start;
		y = box.size.y - (RACK_GRID_WIDTH * 2);
		addParam(createParamCentered<TL1105>(Vec(x, y), module, Funcgen::CASCADE_TRIGGER_PARAM));
		x += dx;
		addParam(createParamCentered<CKSSThree>(Vec(x, y), module, Funcgen::MODE_PARAM));
		x += dx;
		addInput(createInputCentered<PJ301MPort>(Vec(x, y), module, Funcgen::CASCADE_TRIGGER_INPUT));
		x += dx;
		addOutput(createOutputCentered<PJ301MPort>(Vec(x, y), module, Funcgen::CASCADE_OUTPUT));
		x += dx;
		addOutput(createOutputCentered<PJ301MPort>(Vec(x, y), module, Funcgen::CASCADE_EOC_OUTPUT));
		x = box.size.x - (RACK_GRID_WIDTH * 6);
		y = y_start + dy;
		addOutput(createOutputCentered<PJ301MPort>(Vec(x, y), module, Funcgen::AGTB_OUTPUT));
		x += dx;
		addOutput(createOutputCentered<PJ301MPort>(Vec(x, y), module, Funcgen::AGTC_OUTPUT));
		x += dx;
		addOutput(createOutputCentered<PJ301MPort>(Vec(x, y), module, Funcgen::AGTD_OUTPUT));
		x -= dx * 2;
		y += dy;
		addOutput(createOutputCentered<PJ301MPort>(Vec(x, y), module, Funcgen::BGTA_OUTPUT));
		x += dx;
		addOutput(createOutputCentered<PJ301MPort>(Vec(x, y), module, Funcgen::BGTC_OUTPUT));
		x += dx;
		addOutput(createOutputCentered<PJ301MPort>(Vec(x, y), module, Funcgen::BGTD_OUTPUT));
		x -= dx * 2;
		y += dy;
		addOutput(createOutputCentered<PJ301MPort>(Vec(x, y), module, Funcgen::CGTA_OUTPUT));
		x += dx;
		addOutput(createOutputCentered<PJ301MPort>(Vec(x, y), module, Funcgen::CGTB_OUTPUT));
		x += dx;
		addOutput(createOutputCentered<PJ301MPort>(Vec(x, y), module, Funcgen::CGTD_OUTPUT));
		x -= dx * 2;
		y += dy;
		addOutput(createOutputCentered<PJ301MPort>(Vec(x, y), module, Funcgen::DGTA_OUTPUT));
		x += dx;
		addOutput(createOutputCentered<PJ301MPort>(Vec(x, y), module, Funcgen::DGTB_OUTPUT));
		x += dx;
		addOutput(createOutputCentered<PJ301MPort>(Vec(x, y), module, Funcgen::DGTC_OUTPUT));
		x -= dx * 2;
		y += dy;
		addOutput(createOutputCentered<PJ301MPort>(Vec(x, y), module, Funcgen::MIN_OUTPUT));
		x += dx;
		addOutput(createOutputCentered<PJ301MPort>(Vec(x, y), module, Funcgen::MAX_OUTPUT));
		x += dx;
		addOutput(createOutputCentered<PJ301MPort>(Vec(x, y), module, Funcgen::AVG_OUTPUT));
		x -= dx * 1.5f;
		y += dy;
		addOutput(createOutputCentered<PJ301MPort>(Vec(x, y), module, Funcgen::TOPAVG_OUTPUT));
		x += dx;
		addOutput(createOutputCentered<PJ301MPort>(Vec(x, y), module, Funcgen::BOTAVG_OUTPUT));
		x -= dx * 1.5f;
		y += dy;
		addOutput(createOutputCentered<PJ301MPort>(Vec(x, y), module, Funcgen::ABSAB_OUTPUT));
		x += dx;
		addOutput(createOutputCentered<PJ301MPort>(Vec(x, y), module, Funcgen::ABSAC_OUTPUT));
		x += dx;
		addOutput(createOutputCentered<PJ301MPort>(Vec(x, y), module, Funcgen::ABSAD_OUTPUT));
		x -= dx * 2;
		y += dy;
		addOutput(createOutputCentered<PJ301MPort>(Vec(x, y), module, Funcgen::ABSBC_OUTPUT));
		x += dx;
		addOutput(createOutputCentered<PJ301MPort>(Vec(x, y), module, Funcgen::ABSBD_OUTPUT));
		x += dx;
		addOutput(createOutputCentered<PJ301MPort>(Vec(x, y), module, Funcgen::ABSCD_OUTPUT));
		x -= dx * 2;
		y += dy;
		addOutput(createOutputCentered<PJ301MPort>(Vec(x, y), module, Funcgen::ABSBA_OUTPUT));
		x += dx;
		addOutput(createOutputCentered<PJ301MPort>(Vec(x, y), module, Funcgen::ABSCA_OUTPUT));
		x += dx;
		addOutput(createOutputCentered<PJ301MPort>(Vec(x, y), module, Funcgen::ABSCB_OUTPUT));
		x -= dx * 2;
		y += dy;
		addOutput(createOutputCentered<PJ301MPort>(Vec(x, y), module, Funcgen::ABSDA_OUTPUT));
		x += dx;
		addOutput(createOutputCentered<PJ301MPort>(Vec(x, y), module, Funcgen::ABSDB_OUTPUT));
		x += dx;
		addOutput(createOutputCentered<PJ301MPort>(Vec(x, y), module, Funcgen::ABSDC_OUTPUT));
	}
};


Model* modelFuncgen = createModel<Funcgen, FuncgenWidget>("funcgen");