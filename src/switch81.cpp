#include "plugin.hpp"
#include "widgets.hpp"
#include "SwitchBase.hpp"

using simd::float_4;


struct Switch81 : Module, SwitchBase {
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
		STEP_1_INPUT,
		STEP_2_INPUT,
		STEP_3_INPUT,
		STEP_4_INPUT,
		STEP_5_INPUT,
		STEP_6_INPUT,
		STEP_7_INPUT,
		STEP_8_INPUT,
		STEP_9_INPUT,
		INVERT_TRIGGER_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		SIGNAL_OUTPUT,
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

	Switch81() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configSwitch(MODE_PARAM, 0.f, 3.f, 0.f, "Mode", { "Select Chance", "Skip Chance", "Repeat Weight", "Fixed Pattern" });
		getParamQuantity(MODE_PARAM)->snapEnabled = true;
		getParamQuantity(MODE_PARAM)->randomizeEnabled = false;
		configParam(RANDOMIZE_STEPS_PARAM, 0.f, 1.f, 0.f, "Randomize Steps");
		configParam(RANDOMIZE_MODE_PARAM, 0.f, 1.f, 0.f, "Randomize Mode");
		configParam(STEP_1_PARAM, 0.f, 1.f, 1.f, "Step 1 Weight");
		configParam(STEP_2_PARAM, 0.f, 1.f, 1.f, "Step 2 Weight");
		configParam(STEP_3_PARAM, 0.f, 1.f, 1.f, "Step 3 Weight");
		configParam(STEP_4_PARAM, 0.f, 1.f, 1.f, "Step 4 Weight");
		configParam(STEP_5_PARAM, 0.f, 1.f, 1.f, "Step 5 Weight");
		configParam(STEP_6_PARAM, 0.f, 1.f, 1.f, "Step 6 Weight");
		configParam(STEP_7_PARAM, 0.f, 1.f, 1.f, "Step 7 Weight");
		configParam(STEP_8_PARAM, 0.f, 1.f, 1.f, "Step 8 Weight");
		configParam(STEP_9_PARAM, 0.f, 1.f, 1.f, "Step 9 Weight");
		configSwitch(INVERT_WEIGHTS_PARAM, 0.f, 1.f, 0.f, "Invert Weights", {"Invert on Low","Invert on High"});
		configInput(TRIGGER_INPUT, "Trigger");
		configInput(RESET_INPUT, "Reset");
		configInput(RANDOMIZE_STEPS_INPUT, "Randomize Steps");
		configInput(RANDOMIZE_MODE_INPUT, "Randomize Mode");
		configInput(STEP_CV_1_INPUT, "Step 1 Weight CV");
		configInput(STEP_CV_2_INPUT, "Step 2 Weight CV");
		configInput(STEP_CV_3_INPUT, "Step 3 Weight CV");
		configInput(STEP_CV_4_INPUT, "Step 4 Weight CV");
		configInput(STEP_CV_5_INPUT, "Step 5 Weight CV");
		configInput(STEP_CV_6_INPUT, "Step 6 Weight CV");
		configInput(STEP_CV_7_INPUT, "Step 7 Weight CV");
		configInput(STEP_CV_8_INPUT, "Step 8 Weight CV");
		configInput(STEP_CV_9_INPUT, "Step 9 Weight CV");
		configInput(STEP_1_INPUT, "Step 1");
		configInput(STEP_2_INPUT, "Step 2");
		configInput(STEP_3_INPUT, "Step 3");
		configInput(STEP_4_INPUT, "Step 4");
		configInput(STEP_5_INPUT, "Step 5");
		configInput(STEP_6_INPUT, "Step 6");
		configInput(STEP_7_INPUT, "Step 7");
		configInput(STEP_8_INPUT, "Step 8");
		configInput(STEP_9_INPUT, "Step 9");
		configInput(INVERT_TRIGGER_INPUT, "Invert Trigger");
		configOutput(SIGNAL_OUTPUT, "Signal");
	}

	void onReset() override {
		current_step = 0;
		crossfade = false;
		fade_duration = 0.005f;
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
			if (inputs[STEP_1_INPUT + i].isConnected()) {
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
		mode = (int)params[MODE_PARAM].getValue();
		float fade_factor = args.sampleTime * (1.f / fade_duration);

		invert_trigger.process(inputs[INVERT_TRIGGER_INPUT].getVoltage());
		invert_button.process(params[INVERT_WEIGHTS_PARAM].getValue() > 0.f);
		invert_weights = invert_trigger.isHigh() != invert_button.state;

		if (trigger.process(inputs[TRIGGER_INPUT].getVoltage())) {
			compute_weights();
			advance_steps();
		}

		int channels = inputs[STEP_1_INPUT + current_step].getChannels();
		if (channels > MAX_POLY) {
			channels = MAX_POLY;
		}
		outputs[SIGNAL_OUTPUT].setChannels(channels);

		for (int c = 0; c < channels; c += 4) {
			float_4 output = 0.f;
			if (crossfade) {
				for (int v = 0; v < STEP_COUNT; v++) {
					if (v == current_step) {
						volumes[v] = clamp(volumes[v] + fade_factor, 0.f, 1.f);
					}
					else {
						volumes[v] = clamp(volumes[v] - fade_factor, 0.f, 1.f);
					}
					output += inputs[STEP_1_INPUT + v].getPolyVoltageSimd<float_4>(c) * volumes[v];
				}
			}
			else {
				output = inputs[STEP_1_INPUT + current_step].getPolyVoltageSimd<float_4>(c);
			}

			outputs[SIGNAL_OUTPUT].setVoltageSimd(output, c);
		}

		if (crossfade) {
			for (int i = 0; i < STEP_COUNT; i++) {
				lights[STEP_1_LIGHT + i].setBrightness(volumes[i]);
			}
		}
		else {
			for (int i = 0; i < STEP_COUNT; i++) {
				lights[STEP_1_LIGHT + i].setBrightness(i == current_step ? 1.f : 0.f);
			}
		}


		if (reset.process(inputs[RESET_INPUT].getVoltage())) {
			current_step = 0;
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


struct Switch81Widget : ModuleWidget {
	Switch81Widget(Switch81* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/Collapse.svg")));

		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<NP::Knob>(mm2px(Vec(23.104, 17.218)), module, Switch81::STEP_1_PARAM));
		addParam(createParamCentered<NP::Knob>(mm2px(Vec(23.104, 28.794)), module, Switch81::STEP_2_PARAM));
		addParam(createParamCentered<NP::Knob>(mm2px(Vec(23.104, 40.369)), module, Switch81::STEP_3_PARAM));
		addParam(createParamCentered<NP::Knob>(mm2px(Vec(23.104, 51.945)), module, Switch81::STEP_4_PARAM));
		addParam(createParamCentered<NP::Knob>(mm2px(Vec(23.104, 63.52)), module, Switch81::STEP_5_PARAM));
		addParam(createParamCentered<NP::Knob>(mm2px(Vec(23.104, 75.096)), module, Switch81::STEP_6_PARAM));
		addParam(createParamCentered<NP::Knob>(mm2px(Vec(23.104, 86.671)), module, Switch81::STEP_7_PARAM));
		addParam(createParamCentered<NP::Knob>(mm2px(Vec(23.104, 98.247)), module, Switch81::STEP_8_PARAM));
		addParam(createParamCentered<NP::Knob>(mm2px(Vec(23.104, 109.822)), module, Switch81::STEP_9_PARAM));
		addParam(createParamCentered<NP::Button>(mm2px(Vec(56.581, 68.293)), module, Switch81::RANDOMIZE_MODE_PARAM));
		addParam(createParamCentered<SwitchModeSwitch>(mm2px(Vec(52.195, 81.643)), module, Switch81::MODE_PARAM));
		addParam(createParamCentered<NP::Button>(mm2px(Vec(56.429, 100.753)), module, Switch81::RANDOMIZE_STEPS_PARAM));
		addParam(createParamCentered<NP::Switch>(mm2px(Vec(55.866, 115.804)), module, Switch81::INVERT_WEIGHTS_PARAM));

		addInput(createInputCentered<NP::InPort>(mm2px(Vec(7.762, 17.218)), module, Switch81::STEP_1_INPUT));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(7.762, 28.794)), module, Switch81::STEP_2_INPUT));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(7.762, 40.369)), module, Switch81::STEP_3_INPUT));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(7.762, 51.945)), module, Switch81::STEP_4_INPUT));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(7.762, 63.52)), module, Switch81::STEP_5_INPUT));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(7.762, 75.096)), module, Switch81::STEP_6_INPUT));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(7.762, 86.671)), module, Switch81::STEP_7_INPUT));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(7.762, 98.247)), module, Switch81::STEP_8_INPUT));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(7.762, 109.822)), module, Switch81::STEP_9_INPUT));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(33.652, 17.218)), module, Switch81::STEP_CV_1_INPUT));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(33.652, 28.794)), module, Switch81::STEP_CV_2_INPUT));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(33.652, 40.369)), module, Switch81::STEP_CV_3_INPUT));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(33.652, 51.945)), module, Switch81::STEP_CV_4_INPUT));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(33.652, 63.52)), module, Switch81::STEP_CV_5_INPUT));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(33.652, 75.096)), module, Switch81::STEP_CV_6_INPUT));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(33.652, 86.671)), module, Switch81::STEP_CV_7_INPUT));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(33.652, 98.247)), module, Switch81::STEP_CV_8_INPUT));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(33.652, 109.822)), module, Switch81::STEP_CV_9_INPUT));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(47.954, 100.753)), module, Switch81::RANDOMIZE_STEPS_INPUT));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(51.029, 19.704)), module, Switch81::TRIGGER_INPUT));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(51.029, 32.743)), module, Switch81::RESET_INPUT));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(48.105, 68.293)), module, Switch81::RANDOMIZE_MODE_INPUT));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(47.699, 115.729)), module, Switch81::INVERT_TRIGGER_INPUT));

		addOutput(createOutputCentered<NP::OutPort>(mm2px(Vec(53.759, 49.806)), module, Switch81::SIGNAL_OUTPUT));

		addChild(createLightCentered<MediumLight<NP::TealLight>>(mm2px(Vec(14.951, 17.218)), module, Switch81::STEP_1_LIGHT));
		addChild(createLightCentered<MediumLight<NP::TealLight>>(mm2px(Vec(14.951, 28.794)), module, Switch81::STEP_2_LIGHT));
		addChild(createLightCentered<MediumLight<NP::TealLight>>(mm2px(Vec(14.951, 40.369)), module, Switch81::STEP_3_LIGHT));
		addChild(createLightCentered<MediumLight<NP::TealLight>>(mm2px(Vec(14.951, 51.945)), module, Switch81::STEP_4_LIGHT));
		addChild(createLightCentered<MediumLight<NP::TealLight>>(mm2px(Vec(14.951, 63.52)), module, Switch81::STEP_5_LIGHT));
		addChild(createLightCentered<MediumLight<NP::TealLight>>(mm2px(Vec(14.951, 75.096)), module, Switch81::STEP_6_LIGHT));
		addChild(createLightCentered<MediumLight<NP::TealLight>>(mm2px(Vec(14.951, 86.671)), module, Switch81::STEP_7_LIGHT));
		addChild(createLightCentered<MediumLight<NP::TealLight>>(mm2px(Vec(14.951, 98.247)), module, Switch81::STEP_8_LIGHT));
		addChild(createLightCentered<MediumLight<NP::TealLight>>(mm2px(Vec(14.951, 109.822)), module, Switch81::STEP_9_LIGHT));
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
		Switch81* module = dynamic_cast<Switch81*>(this->module);
		assert(module);

		menu->addChild(new MenuSeparator());
		menu->addChild(createMenuItem("Fade while switching", CHECKMARK(module->crossfade), [module]() { module->crossfade = !module->crossfade; }));
		FadeDurationSlider *fade_slider = new FadeDurationSlider(&(module->fade_duration));
		fade_slider->box.size.x = 200.f;
		menu->addChild(fade_slider);
	}
};


Model* modelSwitch81 = createModel<Switch81, Switch81Widget>("switch81");