#pragma once

#include <atomic>
#include <chrono>

namespace fastsm::sound {

// Admission only: rejected sounds are discarded, never queued for later.
// Time is supplied by the caller so sleep recovery can be tested without audio.
class PlaybackPolicy {
public:
	using Clock = std::chrono::steady_clock;

	// The command dispatcher may call this from another thread. Block sounds
	// immediately, even when timeline callbacks precede the power command.
	void begin_power_transition() { pending_transitions_.fetch_add(1); }
	void end_power_transition() { pending_transitions_.fetch_sub(1); }

	// All remaining methods run on the core loop.
	void suspend() { suspended_ = true; }
	void resume(Clock::time_point now) {
		suspended_ = false;
		background_after_ = now + std::chrono::seconds(5);
	}

	bool allows(bool background, bool background_playing, Clock::time_point now) const {
		if (suspended_ || pending_transitions_.load() != 0) {
			return false;
		}
		return !background || (!background_playing && now >= background_after_);
	}

	void background_started(Clock::time_point now) {
		background_after_ = now + std::chrono::seconds(1);
	}

private:
	std::atomic<unsigned> pending_transitions_{0};
	bool suspended_ = false;
	Clock::time_point background_after_{};
};

} // namespace fastsm::sound
