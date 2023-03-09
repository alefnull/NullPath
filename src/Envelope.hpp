#include "plugin.hpp"

#define ENV_MAX_VALUE 10.f;

#define MAX_VALUE 0.999f
#define MIN_VALUE 0.001f

#define MAX_PROCESS_COUNT 32

struct Envelope {
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
        EASE_IN
    };

    Stage stage = IDLE;
    int current_index = 0;
    float _env = 0.f;
    float env = 0.f;
    float attack_time = 0.01f;
    float attack_shape = 0.f;
    float decay_time = 0.1f;
    float decay_shape = 0.f;
    float sustain_level = 0.5f;
    float release_time = 0.01f;
    float release_shape = 0.f;
    bool loop = false;
    bool eoc = false;
    bool idle = true;
    float sampleTime = 0.f;
    int processCount = 0;

    Envelope() {}

    void set_attack(float attack_time) {
        this->attack_time = clamp(attack_time, 0.001f, 10.f);
    }
    void set_attack_shape(float shape){
    	this->attack_shape = shape;
    }
    void set_decay(float decay_time) {
        this->decay_time = clamp(decay_time, 0.001f, 10.f);
    }
    void set_decay_shape(float shape){
    	this->decay_shape = shape;
    }
    void set_sustain(float sustain_level) {
        this->sustain_level = clamp(sustain_level, 0.f, 1.f);
    }
    void set_release(float release_time) {
        this->release_time = clamp(release_time, 0.001f, 10.f);
    }
    void set_release_shape(float shape){
    	this->release_shape = shape;
    }
    void set_loop(bool loop) {
        this->loop = loop;
    }
    void set_index(int index) {
        this->current_index = index;
    }
    void trigger() {
        stage = ATTACK;
        _env = MIN_VALUE;
        processCount = MAX_PROCESS_COUNT;
        sampleTime = 0;
    }
    void retrigger() {
        stage = ATTACK;
        processCount = MAX_PROCESS_COUNT;
    }
    void release(){
    	stage = RELEASE;
    	processCount = MAX_PROCESS_COUNT;
    }
    void reset() {
        stage = IDLE;
        idle = true;
        _env = MIN_VALUE;
        processCount = MAX_PROCESS_COUNT;
        sampleTime = 0;
    }
};

struct ADEnvelope : Envelope {
    
    ADEnvelope() {}

    void process(float st) {
    	processCount++;
    	sampleTime += st;
    	if (processCount < MAX_PROCESS_COUNT) {
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
            case SUSTAIN:
                break;
            case RELEASE:
                break;
        }
        _env = clamp(_env, MIN_VALUE, MAX_VALUE);
        env = _env;
    }
};


struct ADSREnvelope : Envelope {

    ADSREnvelope() {}

    void process(float st) {
        processCount++;
        sampleTime += st;
        if (processCount < MAX_PROCESS_COUNT) {
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
                idle = true;
                break;
            case ATTACK:
                eoc = false;
                idle = false;
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