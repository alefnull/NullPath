#include "plugin.hpp"
#include "SwitchBase.hpp"


struct Switch18 : Module, SwitchBase {
	enum ParamId {
		MODE_PARAM,
		STEP_1_PARAM,
		STEP_2_PARAM,
		STEP_3_PARAM,
		STEP_4_PARAM,
		STEP_5_PARAM,
		STEP_6_PARAM,
		STEP_7_PARAM,
		STEP_8_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		SIGNAL_INPUT,
		TRIGGER_INPUT,
		STEP_1_CV_INPUT,
		STEP_2_CV_INPUT,
		STEP_3_CV_INPUT,
		STEP_4_CV_INPUT,
		STEP_5_CV_INPUT,
		STEP_6_CV_INPUT,
		STEP_7_CV_INPUT,
		STEP_8_CV_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		STEP_1_OUTPUT,
		STEP_2_OUTPUT,
		STEP_3_OUTPUT,
		STEP_4_OUTPUT,
		STEP_5_OUTPUT,
		STEP_6_OUTPUT,
		STEP_7_OUTPUT,
		STEP_8_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		STEP_1_LIGHT,
		STEP_2_LIGHT,
		STEP_3_LIGHT,
		STEP_4_LIGHT,
		STEP_5_LIGHT,
		STEP_6_LIGHT,
		STEP_7_LIGHT,
		STEP_8_LIGHT,
		LIGHTS_LEN
	};

	Switch18() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configSwitch(MODE_PARAM, 0.f, 3.f, 0.f, "mode", { "select chance", "skip chance", "repeat weight", "fixed pattern" });
		getParamQuantity(MODE_PARAM)->snapEnabled = true;
		configParam(STEP_1_PARAM, 0.f, 1.f, 1.f, "step 1 probability");
		configParam(STEP_2_PARAM, 0.f, 1.f, 1.f, "step 2 probability");
		configParam(STEP_3_PARAM, 0.f, 1.f, 1.f, "step 3 probability");
		configParam(STEP_4_PARAM, 0.f, 1.f, 1.f, "step 4 probability");
		configParam(STEP_5_PARAM, 0.f, 1.f, 1.f, "step 5 probability");
		configParam(STEP_6_PARAM, 0.f, 1.f, 1.f, "step 6 probability");
		configParam(STEP_7_PARAM, 0.f, 1.f, 1.f, "step 7 probability");
		configParam(STEP_8_PARAM, 0.f, 1.f, 1.f, "step 8 probability");
		configInput(SIGNAL_INPUT, "signal");
		configInput(TRIGGER_INPUT, "trigger");
		configInput(STEP_1_CV_INPUT, "step 1 cv");
		configInput(STEP_2_CV_INPUT, "step 2 cv");
		configInput(STEP_3_CV_INPUT, "step 3 cv");
		configInput(STEP_4_CV_INPUT, "step 4 cv");
		configInput(STEP_5_CV_INPUT, "step 5 cv");
		configInput(STEP_6_CV_INPUT, "step 6 cv");
		configInput(STEP_7_CV_INPUT, "step 7 cv");
		configInput(STEP_8_CV_INPUT, "step 8 cv");
		configOutput(STEP_1_OUTPUT, "step 1");
		configOutput(STEP_2_OUTPUT, "step 2");
		configOutput(STEP_3_OUTPUT, "step 3");
		configOutput(STEP_4_OUTPUT, "step 4");
		configOutput(STEP_5_OUTPUT, "step 5");
		configOutput(STEP_6_OUTPUT, "step 6");
		configOutput(STEP_7_OUTPUT, "step 7");
		configOutput(STEP_8_OUTPUT, "step 8");
	}

	void compute_weights() {
		for (int i = 0; i < 8; i++) {
			if (outputs[STEP_1_OUTPUT + i].isConnected()) {
				if (!inputs[STEP_1_CV_INPUT + i].isConnected()) {
					weights[i] = params[STEP_1_PARAM + i].getValue();
				}
				else {
					weights[i] = inputs[STEP_1_CV_INPUT + i].getVoltage();
					weights[i] *= params[STEP_1_PARAM].getValue();
				}
			}
			else {
				weights[i] = 0.f;
			}
		}
	}

	void process(const ProcessArgs& args) override {
		float signal = inputs[SIGNAL_INPUT].getVoltage();
		mode = (int)params[MODE_PARAM].getValue();

		compute_weights();

		if (trigger.process(inputs[TRIGGER_INPUT].getVoltage())) {
			do_it_all();
		}
		
		for (int i = 0; i < OUTPUTS_LEN; i++) {
			outputs[STEP_1_OUTPUT + i].setVoltage(i == current_step ? signal : 0.f);
			lights[STEP_1_LIGHT + i].setBrightness(i == current_step ? 1.f : 0.f);
		}
	}
};


struct Switch18Widget : ModuleWidget {
	Switch18Widget(Switch18* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/switch18.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(8.085, 52.362)), module, Switch18::MODE_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(32.341, 25.796)), module, Switch18::STEP_1_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(32.341, 35.806)), module, Switch18::STEP_2_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(32.341, 46.202)), module, Switch18::STEP_3_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(32.341, 56.597)), module, Switch18::STEP_4_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(32.341, 67.955)), module, Switch18::STEP_5_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(32.341, 78.928)), module, Switch18::STEP_6_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(32.341, 89.708)), module, Switch18::STEP_7_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(32.341, 100.104)), module, Switch18::STEP_8_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.085, 25.796)), module, Switch18::SIGNAL_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.085, 36.769)), module, Switch18::TRIGGER_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(21.561, 25.796)), module, Switch18::STEP_1_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(21.561, 35.806)), module, Switch18::STEP_2_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(21.561, 46.202)), module, Switch18::STEP_3_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(21.561, 56.597)), module, Switch18::STEP_4_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(21.561, 67.955)), module, Switch18::STEP_5_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(21.561, 78.928)), module, Switch18::STEP_6_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(21.561, 89.708)), module, Switch18::STEP_7_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(21.561, 100.104)), module, Switch18::STEP_8_CV_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(43.892, 25.796)), module, Switch18::STEP_1_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(43.892, 35.806)), module, Switch18::STEP_2_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(43.892, 46.202)), module, Switch18::STEP_3_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(43.892, 56.597)), module, Switch18::STEP_4_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(43.892, 67.955)), module, Switch18::STEP_5_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(43.892, 78.928)), module, Switch18::STEP_6_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(43.892, 89.708)), module, Switch18::STEP_7_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(43.892, 100.104)), module, Switch18::STEP_8_OUTPUT));

		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(52.169, 25.796)), module, Switch18::STEP_1_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(52.169, 35.806)), module, Switch18::STEP_2_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(52.169, 46.202)), module, Switch18::STEP_3_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(52.169, 56.597)), module, Switch18::STEP_4_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(52.169, 67.955)), module, Switch18::STEP_5_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(52.169, 78.928)), module, Switch18::STEP_6_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(52.169, 89.708)), module, Switch18::STEP_7_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(52.169, 100.104)), module, Switch18::STEP_8_LIGHT));
	}
};


Model* modelSwitch18 = createModel<Switch18, Switch18Widget>("switch18");