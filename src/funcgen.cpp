#include "plugin.hpp"


#define ENV_MAX_VOLTAGE 10.f

struct Funcgen : Module {
	enum ParamId {
		RISE_PARAM,
		LOOP_PARAM,
		FALL_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		TRIGGER_INPUT,
		RISE_CV_INPUT,
		FALL_CV_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		FUNCTION_OUTPUT,
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
	float env = 0.0f;

	bool loop = false;

	dsp::SchmittTrigger trigger;

	Funcgen() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(RISE_PARAM, 0.01f, 10.f, 0.01f, "rise", "s");
		configSwitch(LOOP_PARAM, 0.f, 1.f, 0.f, "loop", {"off", "on"});
		configParam(FALL_PARAM, 0.01f, 10.f, 0.01f, "fall", "s");
		configInput(TRIGGER_INPUT, "trigger");
		configInput(RISE_CV_INPUT, "rise cv");
		configInput(FALL_CV_INPUT, "fall cv");
		configOutput(FUNCTION_OUTPUT, "function");
	}

	void process(const ProcessArgs& args) override {
		float st = args.sampleTime;

		float rise_time = params[RISE_PARAM].getValue();
		float fall_time = params[FALL_PARAM].getValue();
		if (inputs[RISE_CV_INPUT].isConnected()) {
			rise_time = clamp(rise_time + inputs[RISE_CV_INPUT].getVoltage() / 10.f, 0.01f, 10.f);
		}
		if (inputs[FALL_CV_INPUT].isConnected()) {
			fall_time = clamp(fall_time + inputs[FALL_CV_INPUT].getVoltage() / 10.f, 0.01f, 10.f);
		}

		loop = params[LOOP_PARAM].getValue() > 0.5f;
		if (trigger.process(inputs[TRIGGER_INPUT].getVoltage())) {
			stage = RISING;
		}
		if (stage == RISING) {
			env += st * ENV_MAX_VOLTAGE / rise_time;
			if (env >= ENV_MAX_VOLTAGE) {
				env = ENV_MAX_VOLTAGE;
				stage = FALLING;
			}
		}
		if (stage == FALLING) {
			env -= st * ENV_MAX_VOLTAGE / fall_time;
			if (env <= 0.0f) {
				env = 0.0f;
				if (loop) {
					stage = RISING;
				} else {
					stage = IDLE;
				}
			}
		}
		outputs[FUNCTION_OUTPUT].setVoltage(env);
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

		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(18.463, 43.273)), module, Funcgen::RISE_PARAM));
		addParam(createParamCentered<CKSS>(mm2px(Vec(30.391, 43.273)), module, Funcgen::LOOP_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(42.497, 43.273)), module, Funcgen::FALL_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(30.391, 30.413)), module, Funcgen::TRIGGER_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(18.463, 30.413)), module, Funcgen::RISE_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(42.497, 30.413)), module, Funcgen::FALL_CV_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(30.391, 56.344)), module, Funcgen::FUNCTION_OUTPUT));
	}
};


Model* modelFuncgen = createModel<Funcgen, FuncgenWidget>("funcgen");