#include "plugin.hpp"

#define ENV_MAX_VALUE 10.f;

#define MAX_VALUE 0.999f
#define MIN_VALUE 0.001f


struct Envelope {
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
    float func = 0.f;
    float _env = MIN_VALUE;
    float env = 0;
    float rise_time = 0.01f;
    float fall_time = 0.01f;
    bool loop = false;
    bool eoc = false;
    float sampleTime = 0;
    int processCount = 0;
    
    Envelope() {}

    void set_rise(float rise_time) {
        this->rise_time = clamp(rise_time, 0.01f, 10.f);
    }
    void set_fall(float fall_time) {
        this->fall_time = clamp(fall_time, 0.01f, 10.f);
    }
    void set_loop(bool loop) {
        this->loop = loop;
    }
    void set_function(float func){
    	this->func = func;
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
    	sampleTime+=st;
    	if(processCount < 64){
    		return;
    	}else{
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
                if (func < 0.f) {
                    func = abs(func);
                    float func1 = (1.f - _env) * 6.21461f * st / rise_time;
                    float func2 = st / rise_time;
                    float out = func1 * func + func2 * (1.f - func);
                    _env += out;
                }
                else if (func > 0.f) {
                    float func1 = _env * 6.21461f * st / rise_time;
                    float func2 = st / rise_time;
                    float out = func1 * func + func2 * (1.f - func);
                    _env += out;
                }
                else {
                    _env += st / rise_time;
                }
                // switch(func){
                // 	case EASE_OUT: _env += (1.f - _env) * 6.21461f * st / rise_time; break;
                // 	case LINEAR: _env += st / rise_time; break;
                // 	case EASE_IN: _env += (_env) * 6.21461f * st / rise_time; break;
                // }
                if (_env >= MAX_VALUE) {
                    _env = MAX_VALUE;
                    stage = FALLING;
                }
                break;
            case FALLING:
                if (func < 0.f) {
                    func = abs(func);
                    float func1 = _env * 6.21461f * st / fall_time;
                    float func2 = st / fall_time;
                    float out = func1 * func + func2 * (1.f - func);
                    _env -= out;
                }
                else if (func > 0.f) {
                    float func1 = (1.f - _env) * 6.21461f * st / fall_time;
                    float func2 = st / fall_time;
                    float out = func1 * func + func2 * (1.f - func);
                    _env -= out;
                }
                else {
                    _env -= st / fall_time;
                }
           		// switch(func){
                // 	case EASE_OUT: _env -= (_env) * 6.21461f * st / fall_time; break;
                // 	case LINEAR: _env -= st / fall_time; break;
                // 	case EASE_IN: _env -= (1.f- _env) * 6.21461f * st / fall_time; break;
                // }
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
        _env = clamp(_env,MIN_VALUE,MAX_VALUE);
        env = _env * ENV_MAX_VALUE;
    }
};