#include "plugin.hpp"


struct Entangle : Module {
	enum ParamId {
		MODE_PARAM,
		ENTROPY_PARAM,
		CHANNELS_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		SIGNAL_1_INPUT,
		SIGNAL_2_INPUT,
		SIGNAL_3_INPUT,
		SIGNAL_4_INPUT,
		SIGNAL_5_INPUT,
		SIGNAL_6_INPUT,
		SIGNAL_7_INPUT,
		SIGNAL_8_INPUT,
		SIGNAL_9_INPUT,
		CLOCK_INPUT,
		RESET_INPUT,
		MODE_INPUT,
		ENTROPY_INPUT,
		CHANNELS_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		SIGNAL_1_OUTPUT,
		SIGNAL_2_OUTPUT,
		SIGNAL_3_OUTPUT,
		SIGNAL_4_OUTPUT,
		SIGNAL_5_OUTPUT,
		SIGNAL_6_OUTPUT,
		SIGNAL_7_OUTPUT,
		SIGNAL_8_OUTPUT,
		SIGNAL_9_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	Entangle() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(MODE_PARAM, 0.f, 1.f, 0.f, "");
		configParam(ENTROPY_PARAM, 0.f, 1.f, 0.f, "");
		configParam(CHANNELS_PARAM, 0.f, 1.f, 0.f, "");
		configInput(SIGNAL_1_INPUT, "");
		configInput(SIGNAL_2_INPUT, "");
		configInput(SIGNAL_3_INPUT, "");
		configInput(SIGNAL_4_INPUT, "");
		configInput(SIGNAL_5_INPUT, "");
		configInput(SIGNAL_6_INPUT, "");
		configInput(SIGNAL_7_INPUT, "");
		configInput(SIGNAL_8_INPUT, "");
		configInput(SIGNAL_9_INPUT, "");
		configInput(CLOCK_INPUT, "");
		configInput(RESET_INPUT, "");
		configInput(MODE_INPUT, "");
		configInput(ENTROPY_INPUT, "");
		configInput(CHANNELS_INPUT, "");
		configOutput(SIGNAL_1_OUTPUT, "");
		configOutput(SIGNAL_2_OUTPUT, "");
		configOutput(SIGNAL_3_OUTPUT, "");
		configOutput(SIGNAL_4_OUTPUT, "");
		configOutput(SIGNAL_5_OUTPUT, "");
		configOutput(SIGNAL_6_OUTPUT, "");
		configOutput(SIGNAL_7_OUTPUT, "");
		configOutput(SIGNAL_8_OUTPUT, "");
		configOutput(SIGNAL_9_OUTPUT, "");
	}

	void process(const ProcessArgs& args) override {
	}
};


struct EntangleWidget : ModuleWidget {
	EntangleWidget(Entangle* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/Entangle.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(63.404, 28.433)), module, Entangle::MODE_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(63.404, 66.81)), module, Entangle::ENTROPY_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(63.404, 99.503)), module, Entangle::CHANNELS_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(21.534, 17.146)), module, Entangle::SIGNAL_1_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(21.557, 28.794)), module, Entangle::SIGNAL_2_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(21.557, 40.353)), module, Entangle::SIGNAL_3_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(21.557, 51.869)), module, Entangle::SIGNAL_4_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(21.534, 63.448)), module, Entangle::SIGNAL_5_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(21.534, 75.023)), module, Entangle::SIGNAL_6_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(21.557, 86.671)), module, Entangle::SIGNAL_7_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(21.557, 98.247)), module, Entangle::SIGNAL_8_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(21.557, 109.746)), module, Entangle::SIGNAL_9_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.129, 58.382)), module, Entangle::CLOCK_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.151, 76.546)), module, Entangle::RESET_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(63.404, 44.207)), module, Entangle::MODE_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(63.381, 78.229)), module, Entangle::ENTROPY_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(63.404, 109.741)), module, Entangle::CHANNELS_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(49.567, 17.286)), module, Entangle::SIGNAL_1_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(49.563, 28.87)), module, Entangle::SIGNAL_2_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(49.567, 40.437)), module, Entangle::SIGNAL_3_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(49.563, 52.021)), module, Entangle::SIGNAL_4_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(49.563, 63.596)), module, Entangle::SIGNAL_5_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(49.563, 75.172)), module, Entangle::SIGNAL_6_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(49.563, 86.747)), module, Entangle::SIGNAL_7_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(49.563, 98.323)), module, Entangle::SIGNAL_8_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(49.567, 109.89)), module, Entangle::SIGNAL_9_OUTPUT));

		// mm2px(Vec(28.006, 92.756))
		addChild(createWidget<Widget>(mm2px(Vec(21.557, 17.142))));
	}
};


Model* modelEntangle = createModel<Entangle, EntangleWidget>("Entangle");