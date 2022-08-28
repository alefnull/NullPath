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
		RR_TRIGGER_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		ENUMS(TRIGGER_INPUT, CHANNEL_COUNT),
		ENUMS(RISE_CV_INPUT, CHANNEL_COUNT),
		ENUMS(FALL_CV_INPUT, CHANNEL_COUNT),
		RR_TRIGGER_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		ENUMS(FUNCTION_OUTPUT, CHANNEL_COUNT),
		ENUMS(EOC_OUTPUT, CHANNEL_COUNT),
		ROUND_ROBIN_OUTPUT,
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

	Stage stage = IDLE;
	
	Envelope envelope[CHANNEL_COUNT];

	dsp::SchmittTrigger trigger[CHANNEL_COUNT];
	dsp::SchmittTrigger push[CHANNEL_COUNT];
	dsp::SchmittTrigger rr_trigger;
	dsp::SchmittTrigger rr_push;
	dsp::BooleanTrigger eoc_trigger[CHANNEL_COUNT];
	dsp::PulseGenerator eoc_pulse[CHANNEL_COUNT];

	bool round_robin = false;

	Funcgen() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configSwitch(MODE_PARAM, 0.f, 1.f, 0.f, "Mode", {"Normal", "Round-robin"});
		for (int i = 0; i < CHANNEL_COUNT; i++) {
			configParam(RISE_PARAM + i, 0.01f, 10.f, 0.01f, "Rise time", " s");
			configParam(FALL_PARAM + i, 0.01f, 10.f, 0.01f, "Fall time", " s");
			configSwitch(LOOP_PARAM + i, 0.f, 1.f, 0.f, "Loop");
			configParam(RR_TRIGGER_PARAM, 0.f, 1.f, 0.f, "Round-robin push");
			configParam(PUSH_PARAM + i, 0.f, 1.f, 0.f, "Push");
			configInput(TRIGGER_INPUT + i, "Trigger");
			configInput(RISE_CV_INPUT + i, "Rise CV");
			configInput(FALL_CV_INPUT + i, "Fall CV");
			configInput(RR_TRIGGER_INPUT, "Round-robin trigger");
			configOutput(FUNCTION_OUTPUT + i, "Function");
			configOutput(EOC_OUTPUT + i, "EOC");
			configOutput(ROUND_ROBIN_OUTPUT, "Round-robin");
		}
	}

	void process(const ProcessArgs& args) override {
		float st = args.sampleTime;
		round_robin = params[MODE_PARAM].getValue() > 0.5f;
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
			envelope[i].set_loop(loop && !round_robin);

			if (trigger[i].process(inputs[TRIGGER_INPUT + i].getVoltage()) || push[i].process(params[PUSH_PARAM + i].getValue())) {
				envelope[i].retrigger();		
			}
			envelope[i].process(st);
			if (eoc_trigger[i].process(envelope[i].eoc)) {
				eoc_pulse[i].trigger(1e-3f);
			}
			outputs[FUNCTION_OUTPUT + i].setVoltage(envelope[i].env);
			bool eoc = eoc_pulse[i].process(st);
			outputs[EOC_OUTPUT + i].setVoltage(eoc ? 10.f : 0.f);
			if (eoc && round_robin) {
				DEBUG("EOC & RR");
				envelope[(i + 1) % 4].retrigger();
			}
			float rro = std::max(envelope[0].env, envelope[1].env);
			rro = std::max(rro, envelope[2].env);
			rro = std::max(rro, envelope[3].env);
			outputs[ROUND_ROBIN_OUTPUT].setVoltage(rro);
		}
		if (round_robin && (rr_trigger.process(inputs[RR_TRIGGER_INPUT].getVoltage()) || rr_push.process(params[RR_TRIGGER_PARAM].getValue()))) {
			envelope[0].retrigger();
		}
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

		for (int i = 0; i < CHANNEL_COUNT; i++) {
			float x = x_start + 5 * dx * (i / 2) + dx;
			float y = y_start + 4 * dy * (i % 2) + dy;
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
			x -= dx;
			addOutput(createOutputCentered<PJ301MPort>(Vec(x, y), module, Funcgen::EOC_OUTPUT + i));
			if (i == 0) {
				addParam(createParamCentered<TL1105>(Vec(box.size.x / 2 - dx, y + 5 * dy), module, Funcgen::RR_TRIGGER_PARAM));
				addParam(createParamCentered<CKSS>(Vec(box.size.x / 2, y + 5 * dy + dy), module, Funcgen::MODE_PARAM));
				addInput(createInputCentered<PJ301MPort>(Vec(box.size.x / 2 + dx, y + 5 * dy), module, Funcgen::RR_TRIGGER_INPUT));
				addOutput(createOutputCentered<PJ301MPort>(Vec(box.size.x / 2, y + 5 * dy), module, Funcgen::ROUND_ROBIN_OUTPUT));
			}
		}
	}
};


Model* modelFuncgen = createModel<Funcgen, FuncgenWidget>("funcgen");