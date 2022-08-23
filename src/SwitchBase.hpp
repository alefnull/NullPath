#include "plugin.hpp"

struct SwitchBase {
	enum Mode {
		SELECT_CHANCE,
		SKIP_CHANCE,
		REPEAT_WEIGHT,
		FIXED_PATTERN
	};
	int mode = 0;
	int current_step = 0;
	float weights[8] = { 0.f };
	float pattern[8] = { 0.f };
    float volumes[8] = { 0.f };
	float repeat_value = 0.f;
	dsp::SchmittTrigger trigger;
    dsp::SchmittTrigger reset;
    dsp::SchmittTrigger rand_steps_input;
    dsp::SchmittTrigger rand_mode_input;
    dsp::BooleanTrigger rand_steps_button;
    dsp::BooleanTrigger rand_mode_button;

	float calculate_sum(float w[8]) {
		float sum = 0.f;
		for (int i = 0; i < 8; i++) {
			sum += w[i];
		}
		return sum;
	}

	void skip_steps(int depth) {
		int d = depth;
		if (d < 1) {
			return;
		}
		if (depth > 8) {
			d = 8;
		}
		DEBUG("incrementing current_step");
		current_step++;
		if (current_step > 7) {
			current_step = 0;
		}
		DEBUG("current_step is now %d", current_step);
		DEBUG("rolling dice for random current_step");
		float r = random::uniform();
		float w = weights[current_step];
		DEBUG("dice roll is %f", r);
		DEBUG("current_step probability is %f", w);
		if (r > w) {
			DEBUG("dice roll > current_step probability, calling skip_steps() again");
			skip_steps(d - 1);
		}
	}

	void advance_steps() {
        switch (mode) {
        case SELECT_CHANCE:
            {
                float sum = calculate_sum(weights);
                if (sum == 0.f) {
                    break;
                }
                else {
                    float r = random::uniform() * sum;

                    for (int i = 0; i < 8; i++) {
                        r -= weights[i];
                        if (r <= 0.f) {
                            if (weights[i] > 0.f) {
                                current_step = i;
                            }
                            else {
                                continue;
                            }
                            break;
                        }
                    }
                    break;
                }
                break;
            }
        case SKIP_CHANCE:
            {
                bool all_zero = true;
                for (int i = 0; i < 8; i++) {
                    if (weights[i] > 0.f) {
                        all_zero = false;
                        DEBUG("non-zero weight of %f found at index %d", weights[i], i);
                        break;
                    }
                }
                DEBUG("all_zero is %d", all_zero);
                if (!all_zero) {
                    skip_steps(8);
                }
                else {
                    break;
                }
                break;
            }
        case REPEAT_WEIGHT:
            {
                // if all weights are zero, just pick a random step
                bool all_zero = true;
                for (int i = 0; i < 8; i++) {
                    if (weights[i] > 0.f) {
                        all_zero = false;
                        break;
                    }
                }
                if (!all_zero) {
                    // find the min param value that isn't 0
                    float min = 1.f;
                    for (int i = 0; i < 8; i++) {
                        if (weights[i] < min && weights[i] > 0.f) {
                            min = weights[i];
                        }
                    }
                    DEBUG("min is %f", min);
                    // decrement the repeat value by the min value
                    repeat_value -= min;
                    DEBUG("repeat_value after decrement is %f", repeat_value);
                    // if repeat value is greater than 0, keep the same step,
                    // otherwise, advance to next step with a weight greater than 0
                    if (repeat_value > 0.f) {
                        break;
                    }
                    else {
                        DEBUG("repeat_value is 0, advancing to next step");
                        for (int i = 0; i < 8; i++) {
                            if (weights[(current_step + i + 1) % 8] > 0.f) {
                                current_step = (current_step + i + 1) % 8;
                                break;
                            }
                        }
                        repeat_value = weights[current_step];
                        DEBUG("current_step is now %d", current_step);
                        DEBUG("repeat_value is now %f", repeat_value);
                    }
                }
                break;
            }
        case FIXED_PATTERN:
            {
                float sum = calculate_sum(weights);
                if (sum == 0.f) {
                    break;
                }
                // increment stored weights by their param values
                for (int i = 0; i < 8; i++) {
                    pattern[i] += weights[i];
                }
                // select the port that has the highest stored value
                float max = 0.f;
                int max_index = 0;
                for (int i = 0; i < 8; i++) {
                    if (pattern[i] > max) {
                        max = pattern[i];
                        max_index = i;
                    }
                }
                current_step = max_index;
                // decrement the selected port's value by the sum of all the weights
                pattern[current_step] -= sum;
                break;
            }
		}
	}
};