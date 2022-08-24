#include "plugin.hpp"
#include "SwitchBase.hpp"


struct Switch18 : Module, SwitchBase {
	enum ParamId {
		MODE_PARAM,
		RANDOMIZE_STEPS_PARAM,
		RANDOMIZE_MODE_PARAM,
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
		RESET_INPUT,
		RANDOMIZE_STEPS_INPUT,
		RANDOMIZE_MODE_INPUT,
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
		getParamQuantity(MODE_PARAM)->randomizeEnabled = false;
		configParam(RANDOMIZE_STEPS_PARAM, 0.f, 1.f, 0.f, "randomize steps");
		configParam(RANDOMIZE_MODE_PARAM, 0.f, 1.f, 0.f, "randomize mode");
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
		configInput(RESET_INPUT, "reset");
		configInput(RANDOMIZE_STEPS_INPUT, "randomize steps");
		configInput(RANDOMIZE_MODE_INPUT, "randomize mode");
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

	void onReset() override {
		current_step = 0;
	}

	void randomize_steps() {
		for (int i = 0; i < 8; i++) {
			params[STEP_1_PARAM + i].setValue(random::uniform());
		}
	}

	void randomize_mode() {
		params[MODE_PARAM].setValue((int)(random::uniform() * 4));
	}

	void compute_weights() {
		for (int i = 0; i < 8; i++) {
			if (outputs[STEP_1_OUTPUT + i].isConnected()) {
				if (!inputs[STEP_1_CV_INPUT + i].isConnected()) {
					weights[i] = params[STEP_1_PARAM + i].getValue();
				}
				else {
					weights[i] = inputs[STEP_1_CV_INPUT + i].getVoltage() / 5.f;
					weights[i] = clamp(weights[i] + params[STEP_1_PARAM].getValue(), 0.f, 1.f);
				}
			}
			else {
				weights[i] = 0.f;
			}
		}
	}

	void process(const ProcessArgs& args) override {
		if (reset.process(inputs[RESET_INPUT].getVoltage())) {
			onReset();
		}
		if (rand_steps_input.process(inputs[RANDOMIZE_STEPS_INPUT].getVoltage()) || rand_steps_button.process(params[RANDOMIZE_STEPS_PARAM].getValue() > 0.f)) {
			randomize_steps();
		}
		if (rand_mode_input.process(inputs[RANDOMIZE_MODE_INPUT].getVoltage()) || rand_mode_button.process(params[RANDOMIZE_MODE_PARAM].getValue() > 0.f)) {
			randomize_mode();
		}
		float signal = inputs[SIGNAL_INPUT].getVoltage();
		mode = (int)params[MODE_PARAM].getValue();

		compute_weights();

		if (trigger.process(inputs[TRIGGER_INPUT].getVoltage())) {
			advance_steps();
		}
		
		for (int i = 0; i < OUTPUTS_LEN; i++) {
			if (fade_while_switching) {
				if (i == current_step) {
					volumes[i] = clamp(volumes[i] + args.sampleTime * (1.f / fade_speed), 0.f, 1.f);
				}
				else {
					volumes[i] = clamp(volumes[i] - args.sampleTime * (1.f / fade_speed), 0.f, 1.f);
				}
				outputs[STEP_1_OUTPUT + i].setVoltage(signal * volumes[i]);
			}
			else {
				outputs[STEP_1_OUTPUT + i].setVoltage(signal);
			}
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

		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(8.085, 59.362)), module, Switch18::MODE_PARAM));
		addParam(createParamCentered<TL1105>(mm2px(Vec(8.085, 70.362)), module, Switch18::RANDOMIZE_STEPS_PARAM));
		addParam(createParamCentered<TL1105>(mm2px(Vec(8.085, 92.362)), module, Switch18::RANDOMIZE_MODE_PARAM));
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
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.085, 47.769)), module, Switch18::RESET_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.085, 81.362)), module, Switch18::RANDOMIZE_STEPS_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.085, 103.362)), module, Switch18::RANDOMIZE_MODE_INPUT));
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

	struct FadeSpeedQuantity : Quantity {
		float* fade_speed;

		FadeSpeedQuantity(float* fs) {
			fade_speed = fs;
		}

		void setValue(float value) override {
			*fade_speed = clamp(value, 0.5f, 1.5f);
		}

		float getValue() override {
			return *fade_speed;
		}
		
		float getMinValue() override {return 0.5f;}
		float getMaxValue() override {return 1.5f;}
		float getDefaultValue() override {return 0.5f;}
		float getDisplayValue() override {return *fade_speed;}

		std::string getUnit() override {
			return "s";
		}
	};

	struct FadeSpeedSlider : ui::Slider {
		FadeSpeedSlider(float* fade_speed) {
			quantity = new FadeSpeedQuantity(fade_speed);
		}
		~FadeSpeedSlider() {
			delete quantity;
		}
	};

	void appendContextMenu(Menu* menu) override {
		Switch18* module = dynamic_cast<Switch18*>(this->module);
		assert(module);

		menu->addChild(new MenuSeparator());
		menu->addChild(createMenuItem("Fade while switching", CHECKMARK(module->fade_while_switching), [module]() { module->fade_while_switching = !module->fade_while_switching; }));
		FadeSpeedSlider *fade_slider = new FadeSpeedSlider(&(module->fade_speed));
		fade_slider->box.size.x = 200.f;
		menu->addChild(fade_slider);
	}
};


Model* modelSwitch18 = createModel<Switch18, Switch18Widget>("switch18");