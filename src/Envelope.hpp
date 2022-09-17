#include "plugin.hpp"

#define ENV_MAX_VALUE 10.f;

#define MAX_VALUE 0.999f
#define MIN_VALUE 0.001f


struct ADEnvelope {
    enum Stage {
        IDLE,
        RISING,
        FALLING
    };
    enum Func {
    	EASE_OUT,
    	LINEAR,
    	EASE_IN,
    };

    Stage stage = IDLE;
    int current_index = 0;
    float _env = MIN_VALUE;
    float env = 0;
    float rise_time = 0.01f;
    float fall_time = 0.01f;
    float rise_shape = 0.f;
    float fall_shape = 0.f;
    bool loop = false;
    bool eoc = false;
    float sampleTime = 0;
    int processCount = 0;
    
    ADEnvelope() {}

    void set_rise(float rise_time) {
        this->rise_time = clamp(rise_time, 0.01f, 10.f);
    }
    void set_fall(float fall_time) {
        this->fall_time = clamp(fall_time, 0.01f, 10.f);
    }
    void set_loop(bool loop) {
        this->loop = loop;
    }
    void set_rise_shape(float shape){
    	this->rise_shape = shape;
    }
    void set_fall_shape(float shape){
    	this->fall_shape = shape;
    }
    void set_index(int index) {
        this->current_index = index;
    }
    void trigger() {
        stage = RISING;
        _env = MIN_VALUE;
    }
    void retrigger() {
        stage = RISING;
    }
    void reset() {
        stage = IDLE;
        _env = MIN_VALUE;
    }
    void process(float st) {
    	processCount++;
    	sampleTime += st;
    	if (processCount < 64) {
    		return;
    	}
        else {
    		processCount = 0;
    		st = sampleTime;
    		sampleTime = 0;
    	}
        switch (stage) {
            case IDLE:
                eoc = false;
                break;
            case RISING:
                eoc = false;
                if (rise_shape < 0.f) {
                    rise_shape = abs(rise_shape);
                    float func1 = (1.f - _env) * 6.21461f * st / rise_time;
                    float func2 = st / rise_time;
                    float out = func1 * rise_shape + func2 * (1.f - rise_shape);
                    _env += out;
                }
                else if (rise_shape > 0.f) {
                    float func1 = _env * 6.21461f * st / rise_time;
                    float func2 = st / rise_time;
                    float out = func1 * rise_shape + func2 * (1.f - rise_shape);
                    _env += out;
                }
                else {
                    _env += st / rise_time;
                }
                if (_env >= MAX_VALUE) {
                    _env = MAX_VALUE;
                    stage = FALLING;
                }
                break;
            case FALLING:
                if (fall_shape < 0.f) {
                    fall_shape = abs(fall_shape);
                    float func1 = _env * 6.21461f * st / fall_time;
                    float func2 = st / fall_time;
                    float out = func1 * fall_shape + func2 * (1.f - fall_shape);
                    _env -= out;
                }
                else if (fall_shape > 0.f) {
                    float func1 = (1.f - _env) * 6.21461f * st / fall_time;
                    float func2 = st / fall_time;
                    float out = func1 * fall_shape + func2 * (1.f - fall_shape);
                    _env -= out;
                }
                else {
                    _env -= st / fall_time;
                }
                if (_env <= MIN_VALUE) {
                    _env = MIN_VALUE;
                    eoc = true;
                    if (loop) {
                        stage = RISING;
                    } else {
                        stage = IDLE;
                    }
                }
                break;
        }
        _env = clamp(_env, MIN_VALUE, MAX_VALUE);
        env = _env;
    }
};


struct ADSREnvelope {
    enum Stage {
        IDLE,
        ATTACK,
        DECAY,
        SUSTAIN,
        RELEASE
    };
    enum Func {
    	EASE_OUT,
    	LINEAR,
    	EASE_IN,
    };

    Stage stage = IDLE;
    int current_index = 0;
    float _env = MIN_VALUE;
    float env = 0;
    float attack_time = 0.01f;
    float decay_time = 0.01f;
    float release_time = 0.01f;
    float sustain_level = 0.5f;
    float attack_shape = 0.f;
    float decay_shape = 0.f;
    float release_shape = 0.f;
    bool loop = false;
    bool eoc = false;
    float sampleTime = 0;
    int processCount = 0;

    ADSREnvelope() {}

    void set_attack(float attack_time) {
        this->attack_time = clamp(attack_time, 0.01f, 10.f);
    }
    void set_decay(float decay_time) {
        this->decay_time = clamp(decay_time, 0.01f, 10.f);
    }
    void set_release(float release_time) {
        this->release_time = clamp(release_time, 0.01f, 10.f);
    }
    void set_sustain(float sustain_level) {
        this->sustain_level = clamp(sustain_level, 0.f, 1.f);
    }
    void set_loop(bool loop) {
        this->loop = loop;
    }
    void set_attack_shape(float shape){
    	this->attack_shape = shape;
    }
    void set_decay_shape(float shape){
    	this->decay_shape = shape;
    }
    void set_release_shape(float shape){
    	this->release_shape = shape;
    }
    void set_index(int index) {
        this->current_index = index;
    }
    void trigger() {
        stage = ATTACK;
        _env = MIN_VALUE;
    }
    void retrigger() {
        stage = ATTACK;
    }
    void reset() {
        stage = IDLE;
        _env = MIN_VALUE;
    }
    void process(float st) {
        processCount++;
        sampleTime += st;
        if (processCount < 64) {
            return;
        }
        else {
            processCount = 0;
            st = sampleTime;
            sampleTime = 0;
        }
        switch (stage) {
            case IDLE:
                eoc = false;
                break;
            case ATTACK:
                eoc = false;
                if (attack_shape < 0.f) {
                    attack_shape = abs(attack_shape);
                    float func1 = (1.f - _env) * 6.21461f * st / attack_time;
                    float func2 = st / attack_time;
                    float out = func1 * attack_shape + func2 * (1.f - attack_shape);
                    _env += out;
                }
                else if (attack_shape > 0.f) {
                    float func1 = _env * 6.21461f * st / attack_time;
                    float func2 = st / attack_time;
                    float out = func1 * attack_shape + func2 * (1.f - attack_shape);
                    _env += out;
                }
                else {
                    _env += st / attack_time;
                }
                if (_env >= MAX_VALUE) {
                    _env = MAX_VALUE;
                    stage = DECAY;
                }
                break;
            case DECAY:
                if (decay_shape < 0.f) {
                    decay_shape = abs(decay_shape);
                    float func1 = _env * 6.21461f * st / decay_time;
                    float func2 = st / decay_time;
                    float out = func1 * decay_shape + func2 * (1.f - decay_shape);
                    _env -= out;
                }
                else if (decay_shape > 0.f) {
                    float func1 = (1.f - _env) * 6.21461f * st / decay_time;
                    float func2 = st / decay_time;
                    float out = func1 * decay_shape + func2 * (1.f - decay_shape);
                    _env -= out;
                }
                else {
                    _env -= st / decay_time;
                }
                if (_env <= sustain_level) {
                    _env = sustain_level;
                    stage = SUSTAIN;
                }
                break;
            case SUSTAIN:
                eoc = false;
                break;
            case RELEASE:
                if (release_shape < 0.f) {
                    release_shape = abs(release_shape);
                    float func1 = _env * 6.21461f * st / release_time;
                    float func2 = st / release_time;
                    float out = func1 * release_shape + func2 * (1.f - release_shape);
                    _env -= out;
                }
                else if (release_shape > 0.f) {
                    float func1 = (1.f - _env) * 6.21461f * st / release_time;
                    float func2 = st / release_time;
                    float out = func1 * release_shape + func2 * (1.f - release_shape);
                    _env -= out;
                }
                else {
                    _env -= st / release_time;
                }
                if (_env <= MIN_VALUE) {
                    _env = MIN_VALUE;
                    eoc = true;
                    if (loop) {
                        stage = ATTACK;
                    } else {
                        stage = IDLE;
                    }
                }
                break;
        }
        _env = clamp(_env, MIN_VALUE, MAX_VALUE);
        env = _env;
    }
};