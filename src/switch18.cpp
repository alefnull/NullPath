#include "plugin.hpp"
#include "widgets.hpp"
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
		STEP_9_PARAM,
		INVERT_WEIGHTS_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		SIGNAL_INPUT,
		TRIGGER_INPUT,
		RESET_INPUT,
		RANDOMIZE_STEPS_INPUT,
		RANDOMIZE_MODE_INPUT,
		STEP_CV_1_INPUT,
		STEP_CV_2_INPUT,
		STEP_CV_3_INPUT,
		STEP_CV_4_INPUT,
		STEP_CV_5_INPUT,
		STEP_CV_6_INPUT,
		STEP_CV_7_INPUT,
		STEP_CV_8_INPUT,
		STEP_CV_9_INPUT,
		INVERT_TRIGGER_INPUT,
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
		STEP_9_OUTPUT,
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
		STEP_9_LIGHT,
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
		configParam(STEP_9_PARAM, 0.f, 1.f, 1.f, "step 9 probability");
		configSwitch(INVERT_WEIGHTS_PARAM, 0.f, 1.f, 0.f, "invert weights", {"invert on low","invert on high"});
		configInput(SIGNAL_INPUT, "signal");
		configInput(TRIGGER_INPUT, "trigger");
		configInput(RESET_INPUT, "reset");
		configInput(RANDOMIZE_STEPS_INPUT, "randomize steps");
		configInput(RANDOMIZE_MODE_INPUT, "randomize mode");
		configInput(STEP_CV_1_INPUT, "step 1 cv");
		configInput(STEP_CV_2_INPUT, "step 2 cv");
		configInput(STEP_CV_3_INPUT, "step 3 cv");
		configInput(STEP_CV_4_INPUT, "step 4 cv");
		configInput(STEP_CV_5_INPUT, "step 5 cv");
		configInput(STEP_CV_6_INPUT, "step 6 cv");
		configInput(STEP_CV_7_INPUT, "step 7 cv");
		configInput(STEP_CV_8_INPUT, "step 8 cv");
		configInput(STEP_CV_9_INPUT, "step 9 cv");
		configInput(INVERT_TRIGGER_INPUT, "invert trigger");
		configOutput(STEP_1_OUTPUT, "step 1");
		configOutput(STEP_2_OUTPUT, "step 2");
		configOutput(STEP_3_OUTPUT, "step 3");
		configOutput(STEP_4_OUTPUT, "step 4");
		configOutput(STEP_5_OUTPUT, "step 5");
		configOutput(STEP_6_OUTPUT, "step 6");
		configOutput(STEP_7_OUTPUT, "step 7");
		configOutput(STEP_8_OUTPUT, "step 8");
		configOutput(STEP_9_OUTPUT, "step 9");
	}

	void onReset() override {
		current_step = 0;
	}

	void randomize_steps() {
		for (int i = 0; i < STEP_COUNT; i++) {
			params[STEP_1_PARAM + i].setValue(random::uniform());
		}
	}

	void randomize_mode() {
		params[MODE_PARAM].setValue((int)(random::uniform() * 4));
	}

	void compute_weights() {
		for (int i = 0; i < STEP_COUNT; i++) {
			if (outputs[STEP_1_OUTPUT + i].isConnected()) {
				if (!inputs[STEP_CV_1_INPUT + i].isConnected()) {
					weights[i] = params[STEP_1_PARAM + i].getValue();
				}
				else {
					weights[i] = inputs[STEP_CV_1_INPUT + i].getVoltage() / 5.f;
					weights[i] = clamp(weights[i] + params[STEP_1_PARAM].getValue(), 0.f, 1.f);
				}
				if(invert_weights){
					weights[i] = 1 - weights[i];
				}
			}
			else {
				weights[i] = 0.f;
			}
		}
	}

	void process(const ProcessArgs& args) override {
		int channels = inputs[SIGNAL_INPUT].getChannels();
		mode = (int)params[MODE_PARAM].getValue();

		invert_trigger.process(inputs[INVERT_TRIGGER_INPUT].getVoltage());
		invert_button.process(params[INVERT_WEIGHTS_PARAM].getValue() > 0.f);
		invert_weights = invert_trigger.isHigh() != invert_button.state;

		compute_weights();

		if (trigger.process(inputs[TRIGGER_INPUT].getVoltage())) {
			advance_steps();
		}
		
		for (int c = 0; c < channels; c++) {
			float signal = inputs[SIGNAL_INPUT].getPolyVoltage(c);
			for (int i = 0; i < OUTPUTS_LEN; i++) {
				if (crossfade) {
					if (i == current_step) {
						volumes[i] = clamp(volumes[i] + args.sampleTime * (1.f / fade_duration), 0.f, 1.f);
						outputs[STEP_1_OUTPUT + i].setChannels(channels);
						outputs[STEP_1_OUTPUT + i].setVoltage(signal * volumes[i], c);
					}
					else {
						volumes[i] = clamp(volumes[i] - args.sampleTime * (1.f / fade_duration), 0.f, 1.f);
						outputs[STEP_1_OUTPUT + i].setChannels(1);
						outputs[STEP_1_OUTPUT + i].setVoltage(signal * volumes[i]);
					}
				}
				else {
					if (i == current_step) {
						outputs[STEP_1_OUTPUT + i].setChannels(channels);
						outputs[STEP_1_OUTPUT + i].setVoltage(signal, c);
					}
					else {
						outputs[STEP_1_OUTPUT + i].setChannels(1);
						outputs[STEP_1_OUTPUT +i].setVoltage(0.f);
					}
				}
				lights[STEP_1_LIGHT + i].setBrightness(volumes[i]);
			}
		}

		if (reset.process(inputs[RESET_INPUT].getVoltage())) {
			onReset();
		}
		if (rand_steps_input.process(inputs[RANDOMIZE_STEPS_INPUT].getVoltage()) || rand_steps_button.process(params[RANDOMIZE_STEPS_PARAM].getValue() > 0.f)) {
			randomize_steps();
		}
		if (rand_mode_input.process(inputs[RANDOMIZE_MODE_INPUT].getVoltage()) || rand_mode_button.process(params[RANDOMIZE_MODE_PARAM].getValue() > 0.f)) {
			randomize_mode();
		}
	}

    json_t* dataToJson() override {
        json_t* rootJ = json_object();
        json_object_set_new(rootJ, "crossfade", json_boolean(crossfade));
        json_object_set_new(rootJ, "fade_duration", json_real(fade_duration));
        return rootJ;
    }

    void dataFromJson(json_t* rootJ) override {
        json_t* fadeJ = json_object_get(rootJ, "crossfade");
        if (fadeJ)
            crossfade = json_boolean_value(fadeJ);
        json_t* fade_durationJ = json_object_get(rootJ, "fade_duration");
        if (fade_durationJ)
            fade_duration = json_real_value(fade_durationJ);
    }
};


struct Switch18Widget : ModuleWidget {
	Switch18Widget(Switch18* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/Expand.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<NP::Knob>(mm2px(Vec(39.646, 17.345)), module, Switch18::STEP_1_PARAM));
		addParam(createParamCentered<NP::Knob>(mm2px(Vec(39.646, 28.92)), module, Switch18::STEP_2_PARAM));
		addParam(createParamCentered<NP::Knob>(mm2px(Vec(39.646, 40.496)), module, Switch18::STEP_3_PARAM));
		addParam(createParamCentered<NP::Knob>(mm2px(Vec(39.646, 52.071)), module, Switch18::STEP_4_PARAM));
		addParam(createParamCentered<NP::Knob>(mm2px(Vec(39.646, 63.647)), module, Switch18::STEP_5_PARAM));
		addParam(createParamCentered<NP::Knob>(mm2px(Vec(39.646, 75.222)), module, Switch18::STEP_6_PARAM));
		addParam(createParamCentered<NP::Knob>(mm2px(Vec(39.646, 86.798)), module, Switch18::STEP_7_PARAM));
		addParam(createParamCentered<NP::Knob>(mm2px(Vec(39.646, 98.373)), module, Switch18::STEP_8_PARAM));
		addParam(createParamCentered<NP::Knob>(mm2px(Vec(39.646, 109.949)), module, Switch18::STEP_9_PARAM));
		addParam(createParamCentered<NP::Button>(mm2px(Vec(14.637, 68.285)), module, Switch18::RANDOMIZE_MODE_PARAM));
		addParam(createParamCentered<SwitchModeSwitch>(mm2px(Vec(10.554, 81.643)), module, Switch18::MODE_PARAM));
		addParam(createParamCentered<NP::Button>(mm2px(Vec(14.801, 100.753)), module, Switch18::RANDOMIZE_STEPS_PARAM));
		addParam(createParamCentered<NP::Switch>(mm2px(Vec(14.801, 115.804)), module, Switch18::INVERT_WEIGHTS_PARAM));

		addInput(createInputCentered<NP::InPort>(mm2px(Vec(8.854, 50.053)), module, Switch18::SIGNAL_INPUT));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(29.098, 17.142)), module, Switch18::STEP_CV_1_INPUT));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(29.098, 28.718)), module, Switch18::STEP_CV_2_INPUT));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(29.098, 40.293)), module, Switch18::STEP_CV_3_INPUT));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(29.098, 51.869)), module, Switch18::STEP_CV_4_INPUT));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(29.098, 63.444)), module, Switch18::STEP_CV_5_INPUT));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(29.098, 75.02)), module, Switch18::STEP_CV_6_INPUT));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(29.098, 86.595)), module, Switch18::STEP_CV_7_INPUT));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(29.098, 98.171)), module, Switch18::STEP_CV_8_INPUT));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(29.098, 109.746)), module, Switch18::STEP_CV_9_INPUT));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(6.303, 100.681)), module, Switch18::RANDOMIZE_STEPS_INPUT));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(11.721, 19.628)), module, Switch18::TRIGGER_INPUT));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(11.699, 32.671)), module, Switch18::RESET_INPUT));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(6.152, 68.221)), module, Switch18::RANDOMIZE_MODE_INPUT));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(6.303, 115.657)), module, Switch18::INVERT_TRIGGER_INPUT));

		addOutput(createOutputCentered<NP::OutPort>(mm2px(Vec(54.988, 17.294)), module, Switch18::STEP_1_OUTPUT));
		addOutput(createOutputCentered<NP::OutPort>(mm2px(Vec(54.988, 28.87)), module, Switch18::STEP_2_OUTPUT));
		addOutput(createOutputCentered<NP::OutPort>(mm2px(Vec(54.988, 40.445)), module, Switch18::STEP_3_OUTPUT));
		addOutput(createOutputCentered<NP::OutPort>(mm2px(Vec(54.988, 52.021)), module, Switch18::STEP_4_OUTPUT));
		addOutput(createOutputCentered<NP::OutPort>(mm2px(Vec(54.988, 63.596)), module, Switch18::STEP_5_OUTPUT));
		addOutput(createOutputCentered<NP::OutPort>(mm2px(Vec(54.988, 75.172)), module, Switch18::STEP_6_OUTPUT));
		addOutput(createOutputCentered<NP::OutPort>(mm2px(Vec(54.988, 86.747)), module, Switch18::STEP_7_OUTPUT));
		addOutput(createOutputCentered<NP::OutPort>(mm2px(Vec(54.988, 98.323)), module, Switch18::STEP_8_OUTPUT));
		addOutput(createOutputCentered<NP::OutPort>(mm2px(Vec(54.988, 109.898)), module, Switch18::STEP_9_OUTPUT));

		addChild(createLightCentered<MediumLight<NP::TealLight>>(mm2px(Vec(47.799, 17.218)), module, Switch18::STEP_1_LIGHT));
		addChild(createLightCentered<MediumLight<NP::TealLight>>(mm2px(Vec(47.799, 28.794)), module, Switch18::STEP_2_LIGHT));
		addChild(createLightCentered<MediumLight<NP::TealLight>>(mm2px(Vec(47.799, 40.369)), module, Switch18::STEP_3_LIGHT));
		addChild(createLightCentered<MediumLight<NP::TealLight>>(mm2px(Vec(47.799, 51.945)), module, Switch18::STEP_4_LIGHT));
		addChild(createLightCentered<MediumLight<NP::TealLight>>(mm2px(Vec(47.799, 63.52)), module, Switch18::STEP_5_LIGHT));
		addChild(createLightCentered<MediumLight<NP::TealLight>>(mm2px(Vec(47.799, 75.096)), module, Switch18::STEP_6_LIGHT));
		addChild(createLightCentered<MediumLight<NP::TealLight>>(mm2px(Vec(47.799, 86.671)), module, Switch18::STEP_7_LIGHT));
		addChild(createLightCentered<MediumLight<NP::TealLight>>(mm2px(Vec(47.799, 98.247)), module, Switch18::STEP_8_LIGHT));
		addChild(createLightCentered<MediumLight<NP::TealLight>>(mm2px(Vec(47.799, 109.822)), module, Switch18::STEP_9_LIGHT));
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
		Switch18* module = dynamic_cast<Switch18*>(this->module);
		assert(module);

		menu->addChild(new MenuSeparator());
		menu->addChild(createMenuItem("Fade while switching", CHECKMARK(module->crossfade), [module]() { module->crossfade = !module->crossfade; }));
		FadeDurationSlider *fade_slider = new FadeDurationSlider(&(module->fade_duration));
		fade_slider->box.size.x = 200.f;
		menu->addChild(fade_slider);
	}
};


Model* modelSwitch18 = createModel<Switch18, Switch18Widget>("switch18");