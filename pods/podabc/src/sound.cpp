// PodABC - clip player.
#include "sound.h"

#include <algorithm>

#include "pd/sys.h"

namespace abc {

namespace {

int16_t ulaw(uint8_t b) {
    b = uint8_t(~b);
    const int sign = b & 0x80, exp = (b >> 4) & 7, mant = b & 0x0F;
    const int mag = (((mant << 3) + 132) << exp) - 132;
    return int16_t(sign ? -mag : mag);
}

struct Table {
    int16_t v[256];
    Table() {
        for (int i = 0; i < 256; ++i) v[i] = ulaw(uint8_t(i));
    }
};
const Table& table() {
    static const Table t;
    return t;
}

}  // namespace

void Sound::init(pd::hal::AudioOut* out, int volume) {
    out_ = out;
    if (out_ && !out_->start(clip_rate() * 2)) out_ = nullptr;
    set_volume(volume);
}

void Sound::set_volume(int percent) {
    if (out_) out_->set_volume(percent);
}

void Sound::stop() {
    qn_ = 0;
    active_ = false;
    end_ms_ = 0;
    if (out_) out_->silence();
}

void Sound::play(const char* name) {
    stop();
    then(name);
}

void Sound::then(const char* name, uint32_t gap_ms) {
    Item it;
    if (!clip(name, it.clip) || qn_ == kQueue) return;
    it.gap = gap_ms * clip_rate() * 2 / 1000;
    if (!out_) {   // keep the timing, so the game waits as if it heard it
        const uint32_t ms = gap_ms + it.clip.length * 1000 / clip_rate();
        const uint32_t now = pd::sys::now_ms();
        end_ms_ = (end_ms_ && !pd::sys::reached(end_ms_) ? end_ms_ : now) + ms;
        return;
    }
    q_[qn_++] = it;
    pump();
}

bool Sound::next() {
    if (!qn_) return false;
    cur_ = q_[0];
    std::copy(q_ + 1, q_ + qn_, q_);
    --qn_;
    pos_ = 0;
    prev_ = 0;
    active_ = true;
    return true;
}

bool Sound::busy() {
    if (!out_) return end_ms_ && !pd::sys::reached(end_ms_);
    pump();
    return active_ || qn_ || out_->buffered() > 0;
}

void Sound::pump() {
    if (!out_) return;
    // ~40 ms ahead: enough to ride over a screen redraw step.
    const int target = std::min(out_->capacity(), 1280);
    int16_t buf[2 * 128];
    while (out_->buffered() < target - 128) {
        if (!active_ && !next()) return;
        int n = 0;
        while (active_) {
            if (cur_.gap) {   // silence before the clip
                if (n == 128) break;
                buf[2 * n] = buf[2 * n + 1] = 0;
                --cur_.gap;
                ++n;
                continue;
            }
            if (pos_ >= cur_.clip.length) {
                active_ = false;
                break;
            }
            if (n + 2 > 128) break;
            // 2x upsampling: the midpoint, then the sample.
            const int16_t s = table().v[cur_.clip.data[pos_++]];
            const int16_t mid = int16_t((int(prev_) + s) / 2);
            buf[2 * n] = buf[2 * n + 1] = mid;
            buf[2 * n + 2] = buf[2 * n + 3] = s;
            n += 2;
            prev_ = s;
        }
        if (n) out_->push(buf, n);
    }
}

}  // namespace abc
