#include "plugin.hpp"

#define M_PI 3.14159265358979323846
#define FREQ_C3 130.81
using simd::float_4;

struct Oscillator {
    float_4 phase = 0.f;
    float_4 freq = 0.f;

    Oscillator() {
        phase = 0.f;
        freq = FREQ_C3;
    }

    void set_freq(float freq) {
        this->freq = freq;
    }
    void set_phase(float phase) {
        this->phase = phase;
    }
    void reset_phase() {
        this->phase = 0.f;
    }
    void set_pitch(float_4 pitch) {
        this->freq = FREQ_C3 * simd::pow(2.f, pitch);
    }
    void update_phase(float delta) {
        phase += freq * delta;
        phase = simd::fmod(phase, 1.f);
    }

    float_4 sine(float sample_time) {
        update_phase(sample_time);
        return simd::sin(2.f * M_PI * phase);
    }

    float_4 pulse(float sample_time, float width) {
        update_phase(sample_time);
        return simd::ifelse(phase < width, 1.f, -1.f);
    }

    float_4 saw(float sample_time) {
        update_phase(sample_time);
        return phase * 2.f - 1.f;
    }

    float_4 triangle(float sample_time, float width) {
        update_phase(sample_time);
        return simd::ifelse(phase < width, phase * 2.f / width - 1.f, 1.f - (phase - width) * 2.f / (1.f - width));
    }

    float noise() {
        return random::uniform() * 2.f - 1.f;
    }
};