#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")
#endif

#include "apu/apu.hpp"
#include <algorithm>
#include <cmath>

#if !defined(_WIN32) && !defined(JESTER_NO_AUDIO)
#include <pulse/error.h>
#include <pulse/simple.h>
#endif

namespace jester {

// waveform shapes n shit
constexpr u8 APU::DUTY_TABLE[4][8];

#ifdef _WIN32
// windows audio shit (double buffered and whatever)
struct WindowsAudioContext {
  HWAVEOUT hWaveOut = nullptr;
  static constexpr int BUFFER_COUNT = 3;
  WAVEHDR headers[BUFFER_COUNT];
  std::vector<s16> buffers[BUFFER_COUNT];
  int currentBuffer = 0;
  int bufferPos = 0;

  WindowsAudioContext(int bufferSize) {
    for (int i = 0; i < BUFFER_COUNT; i++) {
      buffers[i].resize(bufferSize);
      memset(&headers[i], 0, sizeof(WAVEHDR));
      headers[i].lpData = (LPSTR)buffers[i].data();
      headers[i].dwBufferLength = bufferSize * sizeof(s16);
    }
  }
};
#endif

APU::APU() { sampleBuffer.resize(BUFFER_SIZE); }

APU::~APU() { cleanup(); }

bool APU::init() {
  // waking up the apu state
  ch4.lfsr = 0x7FFF;
  for (int i = 0; i < 16; i++) {
    ch3.waveRam[i] = (i & 1) ? 0x00 : 0xFF;
  }
  nr52 = 0x80;
  nr51 = 0xFF;
  nr50 = 0x77;

#ifdef _WIN32
  // windows waveout bullshit
  auto ctx = new WindowsAudioContext(BUFFER_SIZE);
  audioContext = ctx;

  WAVEFORMATEX wfx = {};
  wfx.wFormatTag = WAVE_FORMAT_PCM;
  wfx.nChannels = 1;
  wfx.nSamplesPerSec = SAMPLE_RATE;
  wfx.wBitsPerSample = 16;
  wfx.nBlockAlign = (wfx.nChannels * wfx.wBitsPerSample) / 8;
  wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

  MMRESULT result =
      waveOutOpen(&ctx->hWaveOut, WAVE_MAPPER, &wfx, 0, 0, CALLBACK_NULL);
  if (result != MMSYSERR_NOERROR) {
    delete ctx;
    audioContext = nullptr;
    return false;
  }

  // headers will be prepared when first buffer is ready to send
  running = true;
  return true;

#else
// pulse audio for linux nerds
#ifdef JESTER_NO_AUDIO
  running = false;
  return true;
#else
  pa_sample_spec spec;
  spec.format = PA_SAMPLE_S16LE;
  spec.rate = SAMPLE_RATE;
  spec.channels = 1;

  pa_buffer_attr bufattr;
  bufattr.maxlength = BUFFER_SIZE * 4;
  bufattr.tlength = BUFFER_SIZE * 2;
  bufattr.prebuf = (u32)-1;
  bufattr.minreq = (u32)-1;
  bufattr.fragsize = (u32)-1;

  int error;
  audioContext =
      pa_simple_new(nullptr, "jester-gb", PA_STREAM_PLAYBACK, nullptr,
                    "Game Boy Audio", &spec, nullptr, &bufattr, &error);

  if (!audioContext) {
    return false;
  }

  running = true;
  return true;
#endif
#endif
}

void APU::cleanup() {
  running = false;

#ifdef _WIN32
  if (audioContext) {
    auto ctx = static_cast<WindowsAudioContext *>(audioContext);
    waveOutReset(ctx->hWaveOut);
    for (int i = 0; i < WindowsAudioContext::BUFFER_COUNT; i++) {
      if (ctx->headers[i].dwFlags & WHDR_PREPARED) {
        waveOutUnprepareHeader(ctx->hWaveOut, &ctx->headers[i], sizeof(WAVEHDR));
      }
    }
    waveOutClose(ctx->hWaveOut);
    delete ctx;
    audioContext = nullptr;
  }
#else
#ifndef JESTER_NO_AUDIO
  if (audioContext) {
    pa_simple_drain(static_cast<pa_simple *>(audioContext), nullptr);
    pa_simple_free(static_cast<pa_simple *>(audioContext));
    audioContext = nullptr;
  }
#endif
#endif
}

void APU::step(u32 cycles) {
  if (!audioEnabled || !(nr52 & 0x80)) // audio off? idc bail out
    return;

  static u32 cycleAccum = 0;
  cycleAccum += cycles;

  frameSequencerCycles += cycles;
  while (frameSequencerCycles >= 8192) {
    frameSequencerCycles -= 8192;
    stepFrameSequencer();
  }

  static const u32 CYCLES_PER_SAMPLE =
      CPU_CLOCK_HZ / SAMPLE_RATE; // how many cycles per noise bit

  while (cycleAccum >= CYCLES_PER_SAMPLE) {
    cycleAccum -= CYCLES_PER_SAMPLE;

    for (u32 i = 0; i < CYCLES_PER_SAMPLE; i++) {
      if (ch1.enabled && (i & 3) == 0) {
        if (ch1.timer > 0)
          ch1.timer--;
        if (ch1.timer == 0) {
          ch1.timer = (2048 - ch1.frequency);
          ch1.dutyPos = (ch1.dutyPos + 1) & 7;
        }
      }

      if (ch2.enabled && (i & 3) == 0) {
        if (ch2.timer > 0)
          ch2.timer--;
        if (ch2.timer == 0) {
          ch2.timer = (2048 - ch2.frequency);
          ch2.dutyPos = (ch2.dutyPos + 1) & 7;
        }
      }

      if (ch3.enabled && (i & 1) == 0) {
        if (ch3.timer > 0)
          ch3.timer--;
        if (ch3.timer == 0) {
          ch3.timer = (2048 - ch3.frequency);
          ch3.wavePos = (ch3.wavePos + 1) & 31;
        }
      }

      if (ch4.enabled) {
        if (ch4.timer > 0)
          ch4.timer--;
        if (ch4.timer == 0) {
          u8 divisor = ch4.polynomial & 0x07;
          u8 shift = (ch4.polynomial >> 4) & 0x0F;
          ch4.timer = (divisor == 0 ? 8 : divisor * 16) << shift;

          u8 xorBit = ((ch4.lfsr & 1) ^ ((ch4.lfsr >> 1) & 1));
          ch4.lfsr = (ch4.lfsr >> 1) | (xorBit << 14);
          if (ch4.polynomial & 0x08) {
            ch4.lfsr &= ~(1 << 6);
            ch4.lfsr |= (xorBit << 6);
          }
        }
      }
    }

    s16 sample = mixChannels();
    outputSample(sample); // spit it out
  }
}

void APU::stepFrameSequencer() {
  switch (frameSequencerStep) {
  case 0:
    stepLength();
    break;
  case 2:
    stepLength();
    stepSweep();
    break;
  case 4:
    stepLength();
    break;
  case 6:
    stepLength();
    stepSweep();
    break;
  case 7:
    stepEnvelope();
    break;
  }
  frameSequencerStep = (frameSequencerStep + 1) & 7;
}

void APU::stepLength() {
  if ((ch1.freqHi & 0x40) && ch1.lengthCounter > 0) {
    ch1.lengthCounter--;
    if (ch1.lengthCounter == 0)
      ch1.enabled = false;
  }
  if ((ch2.freqHi & 0x40) && ch2.lengthCounter > 0) {
    ch2.lengthCounter--;
    if (ch2.lengthCounter == 0)
      ch2.enabled = false;
  }
  if ((ch3.freqHi & 0x40) && ch3.lengthCounter > 0) {
    ch3.lengthCounter--;
    if (ch3.lengthCounter == 0)
      ch3.enabled = false;
  }
  if ((ch4.control & 0x40) && ch4.lengthCounter > 0) {
    ch4.lengthCounter--;
    if (ch4.lengthCounter == 0)
      ch4.enabled = false;
  }
}

void APU::stepEnvelope() {
  u8 period = ch1.envelope & 0x07;
  if (period > 0) {
    if (ch1.envelopeTimer > 0)
      ch1.envelopeTimer--;
    if (ch1.envelopeTimer == 0) {
      ch1.envelopeTimer = period;
      if (ch1.envelope & 0x08) {
        if (ch1.volume < 15)
          ch1.volume++;
      } else {
        if (ch1.volume > 0)
          ch1.volume--;
      }
    }
  }

  period = ch2.envelope & 0x07;
  if (period > 0) {
    if (ch2.envelopeTimer > 0)
      ch2.envelopeTimer--;
    if (ch2.envelopeTimer == 0) {
      ch2.envelopeTimer = period;
      if (ch2.envelope & 0x08) {
        if (ch2.volume < 15)
          ch2.volume++;
      } else {
        if (ch2.volume > 0)
          ch2.volume--;
      }
    }
  }

  period = ch4.envelope & 0x07;
  if (period > 0) {
    if (ch4.envelopeTimer > 0)
      ch4.envelopeTimer--;
    if (ch4.envelopeTimer == 0) {
      ch4.envelopeTimer = period;
      if (ch4.envelope & 0x08) {
        if (ch4.volume < 15)
          ch4.volume++;
      } else {
        if (ch4.volume > 0)
          ch4.volume--;
      }
    }
  }
}

void APU::stepSweep() {
  u8 period = (ch1.sweep >> 4) & 0x07;
  u8 shift = ch1.sweep & 0x07;

  if (ch1.sweepTimer > 0)
    ch1.sweepTimer--;
  if (ch1.sweepTimer == 0) {
    ch1.sweepTimer = period > 0 ? period : 8;
    if (ch1.sweepEnabled && period > 0) {
      u16 newFreq = calculateSweepFreq();
      if (newFreq <= 2047 && shift > 0) {
        ch1.frequency = newFreq;
        ch1.shadowFreq = newFreq;
        calculateSweepFreq();
      }
    }
  }
}

u16 APU::calculateSweepFreq() {
  u8 shift = ch1.sweep & 0x07;
  u16 delta = ch1.shadowFreq >> shift;
  u16 newFreq =
      (ch1.sweep & 0x08) ? ch1.shadowFreq - delta : ch1.shadowFreq + delta;
  if (newFreq > 2047)
    ch1.enabled = false;
  return newFreq;
}

s16 APU::mixChannels() {
  s32 output = 0;
  int activeChannels = 0;

  if (ch1.enabled && (ch1.envelope & 0xF8)) {
    u8 duty = (ch1.duty >> 6) & 0x03;
    s32 sample = DUTY_TABLE[duty][ch1.dutyPos] ? ch1.volume : -ch1.volume;
    output += sample;
    activeChannels++;
  }

  if (ch2.enabled && (ch2.envelope & 0xF8)) {
    u8 duty = (ch2.duty >> 6) & 0x03;
    s32 sample = DUTY_TABLE[duty][ch2.dutyPos] ? ch2.volume : -ch2.volume;
    output += sample;
    activeChannels++;
  }

  if (ch3.enabled && (ch3.dacEnable & 0x80)) {
    u8 pos = ch3.wavePos >> 1;
    u8 sample = ch3.waveRam[pos];
    sample = (ch3.wavePos & 1) ? (sample & 0x0F) : (sample >> 4);

    u8 volShift = (ch3.volume >> 5) & 0x03;
    if (volShift == 0)
      sample = 0;
    else
      sample >>= (volShift - 1);

    output += (s32)sample - 8;
    activeChannels++;
  }

  if (ch4.enabled && (ch4.envelope & 0xF8)) {
    s32 sample = (ch4.lfsr & 1) ? -ch4.volume : ch4.volume;
    output += sample;
    activeChannels++;
  }

  if (activeChannels > 0) {
    output = (output * 1500) / activeChannels;
  }

  output = (output * masterVolume) / 100; // master volume multiplier shit

  return static_cast<s16>(std::clamp(output, (s32)-32767, (s32)32767));
}

void APU::outputSample(s16 sample) {
#ifdef _WIN32
  if (!audioContext)
    return;
  auto ctx = static_cast<WindowsAudioContext *>(audioContext);

  // fill up the buffer
  ctx->buffers[ctx->currentBuffer][ctx->bufferPos++] = sample;

  if (ctx->bufferPos >= BUFFER_SIZE) {
    // buffer is full, send it to the speakers
    WAVEHDR *hdr = &ctx->headers[ctx->currentBuffer];

    // only send if buffer is not still in use (non-blocking)
    if (!(hdr->dwFlags & WHDR_INQUEUE)) {
      // unprepare if it was prepared before
      if (hdr->dwFlags & WHDR_PREPARED) {
        waveOutUnprepareHeader(ctx->hWaveOut, hdr, sizeof(WAVEHDR));
      }

      // set the data pointer fresh (in case vector reallocated)
      hdr->lpData = (LPSTR)ctx->buffers[ctx->currentBuffer].data();
      hdr->dwBufferLength = BUFFER_SIZE * sizeof(s16);
      waveOutPrepareHeader(ctx->hWaveOut, hdr, sizeof(WAVEHDR));
      waveOutWrite(ctx->hWaveOut, hdr, sizeof(WAVEHDR));
    }

    // move to the next one regardless
    ctx->bufferPos = 0;
    ctx->currentBuffer =
        (ctx->currentBuffer + 1) % WindowsAudioContext::BUFFER_COUNT;
  }
#else
#ifndef JESTER_NO_AUDIO
  if (!audioContext)
    return;

  sampleBuffer[sampleIndex++] = sample;
  if (sampleIndex >= BUFFER_SIZE) {
    sampleIndex = 0;
    if (running) {
      pa_simple_write(static_cast<pa_simple *>(audioContext),
                      sampleBuffer.data(), BUFFER_SIZE * sizeof(s16), nullptr);
    }
  }
#endif
#endif
}

void APU::triggerChannel1() {
  ch1.enabled = true;
  if (ch1.lengthCounter == 0)
    ch1.lengthCounter = 64;
  ch1.timer = (2048 - ch1.frequency);
  ch1.envelopeTimer = ch1.envelope & 0x07;
  ch1.volume = (ch1.envelope >> 4) & 0x0F;
  ch1.shadowFreq = ch1.frequency;
  u8 sweepPeriod = (ch1.sweep >> 4) & 0x07;
  u8 sweepShift = ch1.sweep & 0x07;
  ch1.sweepTimer = sweepPeriod > 0 ? sweepPeriod : 8;
  ch1.sweepEnabled = (sweepPeriod > 0) || (sweepShift > 0);
  if (sweepShift > 0)
    calculateSweepFreq();
  if (!(ch1.envelope & 0xF8))
    ch1.enabled = false;
}

void APU::triggerChannel2() {
  ch2.enabled = true;
  if (ch2.lengthCounter == 0)
    ch2.lengthCounter = 64;
  ch2.timer = (2048 - ch2.frequency);
  ch2.envelopeTimer = ch2.envelope & 0x07;
  ch2.volume = (ch2.envelope >> 4) & 0x0F;
  if (!(ch2.envelope & 0xF8))
    ch2.enabled = false;
}

void APU::triggerChannel3() {
  ch3.enabled = true;
  if (ch3.lengthCounter == 0)
    ch3.lengthCounter = 256;
  ch3.timer = (2048 - ch3.frequency);
  ch3.wavePos = 0;
  if (!(ch3.dacEnable & 0x80))
    ch3.enabled = false;
}

void APU::triggerChannel4() {
  ch4.enabled = true;
  if (ch4.lengthCounter == 0)
    ch4.lengthCounter = 64;
  ch4.envelopeTimer = ch4.envelope & 0x07;
  ch4.volume = (ch4.envelope >> 4) & 0x0F;
  ch4.lfsr = 0x7FFF;
  if (!(ch4.envelope & 0xF8))
    ch4.enabled = false;
}

u8 APU::readRegister(u16 addr) const {
  switch (addr) {
  case 0xFF10:
    return ch1.sweep | 0x80;
  case 0xFF11:
    return ch1.duty | 0x3F;
  case 0xFF12:
    return ch1.envelope;
  case 0xFF14:
    return ch1.freqHi | 0xBF;
  case 0xFF16:
    return ch2.duty | 0x3F;
  case 0xFF17:
    return ch2.envelope;
  case 0xFF19:
    return ch2.freqHi | 0xBF;
  case 0xFF1A:
    return ch3.dacEnable | 0x7F;
  case 0xFF1C:
    return ch3.volume | 0x9F;
  case 0xFF1E:
    return ch3.freqHi | 0xBF;
  case 0xFF21:
    return ch4.envelope;
  case 0xFF22:
    return ch4.polynomial;
  case 0xFF23:
    return ch4.control | 0xBF;
  case 0xFF24:
    return nr50;
  case 0xFF25:
    return nr51;
  case 0xFF26: {
    u8 status = nr52 & 0x80;
    if (ch1.enabled)
      status |= 0x01;
    if (ch2.enabled)
      status |= 0x02;
    if (ch3.enabled)
      status |= 0x04;
    if (ch4.enabled)
      status |= 0x08;
    return status | 0x70; // bits 4-6 always 1 mfs
  }
  default:
    if (addr >= 0xFF30 && addr <= 0xFF3F) {
      return ch3.waveRam[addr - 0xFF30];
    }
    return 0xFF;
  }
}

void APU::writeRegister(u16 addr, u8 val) {
  if (!(nr52 & 0x80)) {
    if (addr == 0xFF26) {
      nr52 = val & 0x80;
      if (nr52 & 0x80)
        frameSequencerStep = 0;
    } else if (addr >= 0xFF30 && addr <= 0xFF3F) {
      ch3.waveRam[addr - 0xFF30] = val;
    }
    return;
  }

  switch (addr) {
  case 0xFF10:
    ch1.sweep = val;
    break;
  case 0xFF11:
    ch1.duty = val;
    ch1.lengthCounter = 64 - (val & 0x3F);
    break;
  case 0xFF12:
    ch1.envelope = val;
    break;
  case 0xFF13:
    ch1.freqLo = val;
    ch1.frequency = (ch1.frequency & 0x700) | val;
    break;
  case 0xFF14:
    ch1.freqHi = val;
    ch1.frequency = (ch1.frequency & 0xFF) | ((val & 0x07) << 8);
    if (val & 0x80)
      triggerChannel1();
    break;

  case 0xFF16:
    ch2.duty = val;
    ch2.lengthCounter = 64 - (val & 0x3F);
    break;
  case 0xFF17:
    ch2.envelope = val;
    break;
  case 0xFF18:
    ch2.freqLo = val;
    ch2.frequency = (ch2.frequency & 0x700) | val;
    break;
  case 0xFF19:
    ch2.freqHi = val;
    ch2.frequency = (ch2.frequency & 0xFF) | ((val & 0x07) << 8);
    if (val & 0x80)
      triggerChannel2();
    break;

  case 0xFF1A:
    ch3.dacEnable = val;
    break;
  case 0xFF1B:
    ch3.length = val;
    ch3.lengthCounter = 256 - val;
    break;
  case 0xFF1C:
    ch3.volume = val;
    break;
  case 0xFF1D:
    ch3.freqLo = val;
    ch3.frequency = (ch3.frequency & 0x700) | val;
    break;
  case 0xFF1E:
    ch3.freqHi = val;
    ch3.frequency = (ch3.frequency & 0xFF) | ((val & 0x07) << 8);
    if (val & 0x80)
      triggerChannel3();
    break;

  case 0xFF20:
    ch4.length = val;
    ch4.lengthCounter = 64 - (val & 0x3F);
    break;
  case 0xFF21:
    ch4.envelope = val;
    break;
  case 0xFF22:
    ch4.polynomial = val;
    break;
  case 0xFF23:
    ch4.control = val;
    if (val & 0x80)
      triggerChannel4();
    break;

  case 0xFF24:
    nr50 = val;
    break;
  case 0xFF25:
    nr51 = val;
    break;
  case 0xFF26:
    nr52 = val & 0x80;
    if (!(nr52 & 0x80)) {
      ch1 = {};
      ch2 = {};
      ch3.enabled = false;
      ch4 = {};
    }
    break;

  default:
    if (addr >= 0xFF30 && addr <= 0xFF3F) {
      ch3.waveRam[addr - 0xFF30] = val;
    }
    break;
  }
}

} // namespace jester
