#include "check.hpp"
#include "fastsm/sound/playback_policy.hpp"

using fastsm::sound::PlaybackPolicy;
using namespace std::chrono_literals;

void test_sound_wake_recovery() {
	PlaybackPolicy policy;
	const auto start = PlaybackPolicy::Clock::time_point{} + 100s;
	CHECK(policy.allows(false, false, start));
	CHECK(policy.allows(true, false, start));

	// Sleep blocks already-queued callbacks before the core handles teardown.
	policy.begin_power_transition();
	CHECK(!policy.allows(false, false, start));
	CHECK(!policy.allows(true, false, start));
	policy.suspend();
	policy.end_power_transition();
	CHECK(!policy.allows(false, false, start + 1h));
	CHECK(!policy.allows(true, false, start + 1h));

	const auto wake = start + 1h;
	policy.begin_power_transition();
	policy.resume(wake);
	CHECK(!policy.allows(false, false, wake));
	policy.end_power_transition();
	// Actions work immediately; catching-up timelines stay quiet for five seconds.
	CHECK(policy.allows(false, false, wake));
	for (int i = 0; i < 500; ++i) {
		CHECK(!policy.allows(true, false, wake + i * 10ms));
	}
	CHECK(policy.allows(true, false, wake + 5s));

	// A second pending sleep/wake command must not reopen the gate early.
	policy.begin_power_transition();
	policy.begin_power_transition();
	policy.resume(wake + 10s);
	policy.end_power_transition();
	CHECK(!policy.allows(false, false, wake + 10s));
	policy.suspend();
	policy.end_power_transition();
	CHECK(!policy.allows(false, false, wake + 20s));
	policy.begin_power_transition();
	policy.resume(wake + 20s);
	policy.end_power_transition();
	CHECK(policy.allows(false, false, wake + 20s));
	CHECK(!policy.allows(true, false, wake + 24s));
	CHECK(policy.allows(true, false, wake + 25s));
}

void test_sound_background_burst() {
	PlaybackPolicy policy;
	const auto start = PlaybackPolicy::Clock::time_point{} + 100s;
	CHECK(policy.allows(true, false, start));
	policy.background_started(start);
	// Even very short chimes cannot turn a batch of arrivals into a sound burst.
	for (int i = 0; i < 100; ++i) {
		CHECK(!policy.allows(true, false, start + i * 10ms));
	}
	CHECK(policy.allows(true, false, start + 1s));
	// Long custom soundpack clips cannot overlap after the cooldown expires.
	CHECK(!policy.allows(true, true, start + 30s));
	CHECK(policy.allows(false, true, start + 30s));
	CHECK(policy.allows(true, false, start + 30s));
	policy.background_started(start + 30s);
	CHECK(!policy.allows(true, false, start + 30500ms));
	CHECK(policy.allows(true, false, start + 31s));
}
