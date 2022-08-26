#include "plugin.hpp"
#include "widgets.hpp"
#include "SwitchBase.hpp"

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
		configSwitch(MODE_PARAM, 0.f, 3.f, 0.f, "mode", { "select chance", "skip chance", "repeat weight", "fixed pattern" });
		getParamQuantity(MODE_PARAM)->snapEnabled = true;
		getParamQuantity(MODE_PARAM)->randomizeEnabled = false;
		configParam(RANDOMIZE_STEPS_PARAM, 0.f, 1.f, 0.f, "randomize steps");
		configParam(RANDOMIZE_MODE_PARAM, 0.f, 1.f, 0.f, "randomize mode");
		configParam(STEP_1_PARAM, 0.f, 1.f, 1.f, "step 1");
		configParam(STEP_2_PARAM, 0.f, 1.f, 1.f, "step 2");
		configParam(STEP_3_PARAM, 0.f, 1.f, 1.f, "step 3");
		configParam(STEP_4_PARAM, 0.f, 1.f, 1.f, "step 4");
		configParam(STEP_5_PARAM, 0.f, 1.f, 1.f, "step 5");
		configParam(STEP_6_PARAM, 0.f, 1.f, 1.f, "step 6");
		configParam(STEP_7_PARAM, 0.f, 1.f, 1.f, "step 7");
		configParam(STEP_8_PARAM, 0.f, 1.f, 1.f, "step 8");
		configParam(STEP_9_PARAM, 0.f, 1.f, 1.f, "step 9");
		configInput(TRIGGER_INPUT, "trigger");
		configInput(RESET_INPUT, "reset");
		configInput(RANDOMIZE_STEPS_INPUT, "randomize steps");
		configInput(RANDOMIZE_MODE_INPUT, "randomize mode");
		configInput(STEP_CV_1_INPUT, "step 1 CV");
		configInput(STEP_CV_2_INPUT, "step 2 CV");
		configInput(STEP_CV_3_INPUT, "step 3 CV");
		configInput(STEP_CV_4_INPUT, "step 4 CV");
		configInput(STEP_CV_5_INPUT, "step 5 CV");
		configInput(STEP_CV_6_INPUT, "step 6 CV");
		configInput(STEP_CV_7_INPUT, "step 7 CV");
		configInput(STEP_CV_8_INPUT, "step 8 CV");
		configInput(STEP_CV_9_INPUT, "step 9 CV");
		configInput(STEP_1_INPUT, "step 1");
		configInput(STEP_2_INPUT, "step 2");
		configInput(STEP_3_INPUT, "step 3");
		configInput(STEP_4_INPUT, "step 4");
		configInput(STEP_5_INPUT, "step 5");
		configInput(STEP_6_INPUT, "step 6");
		configInput(STEP_7_INPUT, "step 7");
		configInput(STEP_8_INPUT, "step 8");
		configInput(STEP_9_INPUT, "step 9");
		configOutput(SIGNAL_OUTPUT, "signal");
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
			if (inputs[STEP_1_INPUT + i].isConnected()) {
				if (!inputs[STEP_CV_1_INPUT + i].isConnected()) {
					weights[i] = params[STEP_1_PARAM + i].getValue();
				}
				else {
					weights[i] = inputs[STEP_CV_1_INPUT + i].getVoltage() / 5.f;
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
		float output = 0.f;
		mode = (int)params[MODE_PARAM].getValue();

		compute_weights();

		if (trigger.process(inputs[TRIGGER_INPUT].getVoltage())) {
			advance_steps();
		}

		if (fade_while_switching) {
			for (int v = 0; v < STEP_COUNT; v++) {
				if (v == current_step) {
					volumes[v] = clamp(volumes[v] + args.sampleTime * (1.f / fade_speed), 0.f, 1.f);
				}
				else {
					volumes[v] = clamp(volumes[v] - args.sampleTime * (1.f / fade_speed), 0.f, 1.f);
				}
				output += inputs[STEP_1_INPUT + v].getVoltage() * volumes[v];
			}
		}
		else {
			output = inputs[STEP_1_INPUT + current_step].getVoltage();
		}

		outputs[SIGNAL_OUTPUT].setVoltage(output);

		for (int i = 0; i < STEP_COUNT; i++) {
			lights[STEP_1_LIGHT + i].setBrightness(i == current_step ? 1.f : 0.f);
		}
	}

    json_t* dataToJson() override {
        json_t* rootJ = json_object();
        json_object_set_new(rootJ, "fade_while_switching", json_boolean(fade_while_switching));
        json_object_set_new(rootJ, "fade_speed", json_real(fade_speed));
        return rootJ;
    }

    void dataFromJson(json_t* rootJ) override {
        json_t* fadeJ = json_object_get(rootJ, "fade_while_switching");
        if (fadeJ)
            fade_while_switching = json_boolean_value(fadeJ);
        json_t* fade_speedJ = json_object_get(rootJ, "fade_speed");
        if (fade_speedJ)
            fade_speed = json_real_value(fade_speedJ);
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

		addParam(createParamCentered<NP::Knob>(mm2px(Vec(23.303, 16.401)), module, Switch81::STEP_1_PARAM));
		addParam(createParamCentered<NP::Knob>(mm2px(Vec(19.171, 28.368)), module, Switch81::STEP_2_PARAM));
		addParam(createParamCentered<NP::Knob>(mm2px(Vec(20.896, 42.491)), module, Switch81::STEP_3_PARAM));
		addParam(createParamCentered<NP::Knob>(mm2px(Vec(24.638, 53.697)), module, Switch81::STEP_4_PARAM));
		addParam(createParamCentered<NP::Knob>(mm2px(Vec(19.963, 64.122)), module, Switch81::STEP_5_PARAM));
		addParam(createParamCentered<NP::Knob>(mm2px(Vec(24.411, 76.411)), module, Switch81::STEP_6_PARAM));
		addParam(createParamCentered<NP::Knob>(mm2px(Vec(27.728, 88.871)), module, Switch81::STEP_7_PARAM));
		addParam(createParamCentered<NP::Knob>(mm2px(Vec(28.743, 101.797)), module, Switch81::STEP_8_PARAM));
		addParam(createParamCentered<NP::Knob>(mm2px(Vec(35.896, 112.878)), module, Switch81::STEP_9_PARAM));
		addParam(createParamCentered<NP::Button>(mm2px(Vec(56.547, 77.941)), module, Switch81::RANDOMIZE_MODE_PARAM));
		addParam(createParamCentered<SwitchModeSwitch>(mm2px(Vec(52.389, 91.291)), module, Switch81::MODE_PARAM));
		addParam(createParamCentered<NP::Button>(mm2px(Vec(56.396, 114.105)), module, Switch81::RANDOMIZE_STEPS_PARAM));

		addInput(createInputCentered<NP::InPort>(mm2px(Vec(5.742, 20.17)), module, Switch81::STEP_1_INPUT));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(4.888, 30.285)), module, Switch81::STEP_2_INPUT));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(5.6, 40.827)), module, Switch81::STEP_3_INPUT));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(5.173, 53.507)), module, Switch81::STEP_4_INPUT));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(6.17, 65.901)), module, Switch81::STEP_5_INPUT));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(9.481, 77.614)), module, Switch81::STEP_6_INPUT));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(13.56, 90.215)), module, Switch81::STEP_7_INPUT));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(9.117, 99.966)), module, Switch81::STEP_8_INPUT));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(7.786, 112.763)), module, Switch81::STEP_9_INPUT));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(33.446, 15.684)), module, Switch81::STEP_CV_1_INPUT));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(29.719, 30.996)), module, Switch81::STEP_CV_2_INPUT));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(31.029, 41.271)), module, Switch81::STEP_CV_3_INPUT));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(34.727, 51.552)), module, Switch81::STEP_CV_4_INPUT));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(31.293, 63.825)), module, Switch81::STEP_CV_5_INPUT));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(34.219, 72.616)), module, Switch81::STEP_CV_6_INPUT));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(36.213, 82.677)), module, Switch81::STEP_CV_7_INPUT));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(36.925, 95.357)), module, Switch81::STEP_CV_8_INPUT));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(42.267, 104.474)), module, Switch81::STEP_CV_9_INPUT));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(47.92, 114.105)), module, Switch81::RANDOMIZE_STEPS_INPUT));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(50.996, 19.826)), module, Switch81::TRIGGER_INPUT));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(50.996, 32.866)), module, Switch81::RESET_INPUT));
		addInput(createInputCentered<NP::InPort>(mm2px(Vec(48.071, 77.941)), module, Switch81::RANDOMIZE_MODE_INPUT));

		addOutput(createOutputCentered<NP::OutPort>(mm2px(Vec(53.725, 54.162)), module, Switch81::SIGNAL_OUTPUT));

		addChild(createLightCentered<MediumLight<NP::TealLight>>(mm2px(Vec(13.423, 18.191)), module, Switch81::STEP_1_LIGHT));
		addChild(createLightCentered<MediumLight<NP::TealLight>>(mm2px(Vec(11.018, 28.774)), module, Switch81::STEP_2_LIGHT));
		addChild(createLightCentered<MediumLight<NP::TealLight>>(mm2px(Vec(12.428, 41.366)), module, Switch81::STEP_3_LIGHT));
		addChild(createLightCentered<MediumLight<NP::TealLight>>(mm2px(Vec(13.838, 53.958)), module, Switch81::STEP_4_LIGHT));
		addChild(createLightCentered<MediumLight<NP::TealLight>>(mm2px(Vec(12.025, 63.629)), module, Switch81::STEP_5_LIGHT));
		addChild(createLightCentered<MediumLight<NP::TealLight>>(mm2px(Vec(16.133, 76.868)), module, Switch81::STEP_6_LIGHT));
		addChild(createLightCentered<MediumLight<NP::TealLight>>(mm2px(Vec(19.623, 91.755)), module, Switch81::STEP_7_LIGHT));
		addChild(createLightCentered<MediumLight<NP::TealLight>>(mm2px(Vec(17.914, 99.876)), module, Switch81::STEP_8_LIGHT));
		addChild(createLightCentered<MediumLight<NP::TealLight>>(mm2px(Vec(20.122, 107.996)), module, Switch81::STEP_9_LIGHT));
	}

	struct FadeSpeedQuantity : Quantity {
		float* fade_speed;

		FadeSpeedQuantity(float* fs) {
			fade_speed = fs;
		}

		void setValue(float value) override {
			*fade_speed = clamp(value, 0.005f, 10.f);
		}

		float getValue() override {
			return *fade_speed;
		}
		
		float getMinValue() override {return 0.005f;}
		float getMaxValue() override {return 10.f;}
		float getDefaultValue() override {return 0.005f;}
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
		Switch81* module = dynamic_cast<Switch81*>(this->module);
		assert(module);

		menu->addChild(new MenuSeparator());
		menu->addChild(createMenuItem("Fade while switching", CHECKMARK(module->fade_while_switching), [module]() { module->fade_while_switching = !module->fade_while_switching; }));
		FadeSpeedSlider *fade_slider = new FadeSpeedSlider(&(module->fade_speed));
		fade_slider->box.size.x = 200.f;
		menu->addChild(fade_slider);
	}
};


Model* modelSwitch81 = createModel<Switch81, Switch81Widget>("switch81");