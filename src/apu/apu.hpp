#pragma once

#include "types.hpp"
#include <array>
#include <atomic>
#include <vector>

namespace jester {

class APU {
public:
  APU();
  ~APU();

  bool init();
  void cleanup();
  void step(u32 cycles);

  u8 readRegister(u16 addr) const;
  void writeRegister(u16 addr, u8 val);

  void setEnabled(bool enabled) { audioEnabled = enabled; }
  bool isEnabled() const { return audioEnabled; }
  void setVolume(int vol) { masterVolume = vol; }

private:
  void *audioContext = nullptr; // audio handle (pulse or windows shit)
  std::atomic<bool> running{false};
  std::atomic<bool> audioEnabled{true};

  static constexpr int SAMPLE_RATE = 44100;
  static constexpr int BUFFER_SIZE = 1024;
  std::vector<s16> sampleBuffer;
  int sampleIndex = 0;

  u32 frameSequencerCycles = 0;
  u8 frameSequencerStep = 0;

  struct Channel1 {
    u8 sweep = 0, duty = 0, envelope = 0, freqLo = 0, freqHi = 0;
    bool enabled = false;
    u16 frequency = 0;
    u32 timer = 0;
    u8 dutyPos = 0, volume = 0, envelopeTimer = 0;
    u16 sweepTimer = 0, shadowFreq = 0, lengthCounter = 0;
    bool sweepEnabled = false;
  } ch1;

  struct Channel2 {
    u8 duty = 0, envelope = 0, freqLo = 0, freqHi = 0;
    bool enabled = false;
    u16 frequency = 0;
    u32 timer = 0;
    u8 dutyPos = 0, volume = 0, envelopeTimer = 0;
    u16 lengthCounter = 0;
  } ch2;

  struct Channel3 {
    u8 dacEnable = 0, length = 0, volume = 0, freqLo = 0, freqHi = 0;
    bool enabled = false;
    u16 frequency = 0;
    u32 timer = 0;
    u8 wavePos = 0;
    u16 lengthCounter = 0;
    std::array<u8, 16> waveRam = {};
  } ch3;

  struct Channel4 {
    u8 length = 0, envelope = 0, polynomial = 0, control = 0;
    bool enabled = false;
    u32 timer = 0;
    u8 volume = 0, envelopeTimer = 0;
    u16 lfsr = 0x7FFF, lengthCounter = 0;
  } ch4;

  u8 nr50 = 0, nr51 = 0, nr52 = 0;
  int masterVolume = 50;

  static constexpr u8 DUTY_TABLE[4][8] = {{0, 0, 0, 0, 0, 0, 0, 1},
                                          {1, 0, 0, 0, 0, 0, 0, 1},
                                          {1, 0, 0, 0, 0, 1, 1, 1},
                                          {0, 1, 1, 1, 1, 1, 1, 0}};

  void stepFrameSequencer();
  void stepLength();
  void stepEnvelope();
  void stepSweep();
  s16 mixChannels();
  void outputSample(s16 sample);
  void triggerChannel1();
  void triggerChannel2();
  void triggerChannel3();
  void triggerChannel4();
  u16 calculateSweepFreq();
};

} // namespace jester
