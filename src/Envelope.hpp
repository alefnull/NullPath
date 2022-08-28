#include "plugin.hpp"

#define ENV_MAX_VOLTAGE 10.f

struct Envelope {
    enum Stage {
        IDLE,
        RISING,
        FALLING
    };

    Stage stage = IDLE;
    float env = 0.0f;
    float rise_time = 0.01f;
    float fall_time = 0.01f;
    bool loop = false;
    bool eoc = false;
    
    Envelope() {}

    void set_rise(float& rise_time) {
        this->rise_time = clamp(rise_time, 0.01f, 10.f);
    }
    void set_fall(float& fall_time) {
        this->fall_time = clamp(fall_time, 0.01f, 10.f);
    }
    void set_loop(bool& loop) {
        this->loop = loop;
    }
    void trigger() {
        stage = RISING;
        env = 0.0f;
    }
    void retrigger() {
        stage = RISING;
    }
    void process(float st) {
        switch (stage) {
            case IDLE:
                eoc = false;
                break;
            case RISING:
                eoc = false;
                env += st * ENV_MAX_VOLTAGE / rise_time;
                if (env >= 10.0f) {
                    env = 10.0f;
                    stage = FALLING;
                }
                break;
            case FALLING:
                env -= st * ENV_MAX_VOLTAGE / fall_time;
                if (env <= 0.0f) {
                    env = 0.0f;
                    eoc = true;
                    if (loop) {
                        stage = RISING;
                    } else {
                        stage = IDLE;
                    }
                }
                break;
        }
    }
};