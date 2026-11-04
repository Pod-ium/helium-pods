// PodABC - plays the voice clips (and the jingle) on the terminal's audio.
//
// Non-blocking: play() / then() queue clips and pump(), called often from the
// main loop (and while drawing), keeps the output fed. The clips are 16 kHz;
// the output runs at twice that, so the PWM carrier is out of earshot.
#pragma once
#include <cstdint>

#include "assets.h"
#include "pd/hal/audio.h"

namespace abc {

class Sound {
public:
    void init(pd::hal::AudioOut* out, int volume);
    void set_volume(int percent);

    // Stops what is playing and plays `name`.
    void play(const char* name);
    // Plays `name` after what is queued, `gap_ms` of silence first.
    void then(const char* name, uint32_t gap_ms = 0);
    void stop();
    // True until the last queued clip has been heard.
    bool busy();
    void pump();

private:
    struct Item {
        Clip clip;
        uint32_t gap = 0;   // silent output frames before the clip
    };
    static constexpr int kQueue = 4;

    bool next();

    pd::hal::AudioOut* out_ = nullptr;
    Item q_[kQueue];
    int qn_ = 0;
    Item cur_;
    bool active_ = false;
    uint32_t pos_ = 0;
    int16_t prev_ = 0;
    uint32_t end_ms_ = 0;   // no audio output: when the clips would have ended
};

}  // namespace abc
