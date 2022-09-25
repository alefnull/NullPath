#include "plugin.hpp"

#define M_PI 3.14159265358979323846

struct Oscillator {
    double phase = 0.f;
    double freq = 0.f;

    Oscillator() {
        phase = 0.f;
        freq = dsp::FREQ_C4;
    }

    void set_freq(float freq) {
        this->freq = freq;
    }
    void set_phase(float phase) {
        this->phase = phase;
    }
    void set_pitch(float pitch) {
        this->freq = dsp::FREQ_C4 * std::pow(2.f, pitch);
    }
    void update_phase(float delta) {
        phase += freq * delta;
        if (phase >= 1.f) {
            phase -= 1.f;
        }
    }

    float sine(float freq, float sample_time) {
        update_phase(sample_time);
        return clamp(std::sin(2.f * M_PI * phase), -1.f, 1.f);
    }

    float pulse(float freq, float sample_time, float width) {
        update_phase(sample_time);
        return clamp(phase < width ? 1.f : -1.f, -1.f, 1.f);
    }

    float saw(float freq, float sample_time) {
        update_phase(sample_time);
        return clamp(phase * 2.f - 1.f, -1.f, 1.f);
    }

    float triangle(float freq, float sample_time, float width) {
        update_phase(sample_time);
        return clamp(phase < width ? phase * 2.f / width - 1.f : 1.f - (phase - width) * 2.f / (1.f - width), -1.f, 1.f);
    }

    float noise() {
        return clamp(random::uniform() * 2.f - 1.f, -1.f, 1.f);
    }
};