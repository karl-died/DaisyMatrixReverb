#include "daisy_patch.h"
#include "daisysp.h"
#include "Reverb.h"

using namespace daisy;
using namespace daisysp;

const int sampleRate = 48000;
const int blockSize = 64;

DaisyPatch hw;

TimerHandle timerHandle;

Parameter decayParam, dampParam, modParam, mixParam;
Reverb reverb;

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
  uint32_t t0 = timerHandle.GetUs();

	hw.ProcessAllControls();
	for (size_t i = 0; i < size; i++)
	{
		out[0][i] = in[0][i];
		out[1][i] = in[1][i];
		out[2][i] = in[2][i];
		out[3][i] = in[3][i];
	}

  reverb.setDecay(decayParam.Process());
  reverb.setDamp(dampParam.Process());
  reverb.setModulation(modParam.Process());
  reverb.process(out, size);

  for(size_t i = 0; i < size; i++)
  {
    float mix = mixParam.Process();
    float dry = 1 - mix * mix;
    float wet = 1 - (1 - mix) * (1 - mix);
    out[0][i] = wet * out[0][i] + dry * in[0][i];
    out[1][i] = wet * out[1][i] + dry * in[0][i];
  }

  uint32_t t1 = timerHandle.GetUs();
  uint32_t delta = t1 - t0;
  const uint32_t blockDuration = (blockSize * 1000) / (sampleRate / 1000);
  float32_t blockBudget = (float) delta / blockDuration;
  uint32_t up = ((float) blockSize) * blockBudget;
  if(up > blockSize) up = blockSize;

  for(uint32_t i = 0; i < up; i++)
    out[3][i] = 1.0;
  for(uint32_t i = up; i < blockSize; i++)
    out[3][i] = 0.0;

}

int main(void)
{
	hw.Init();
	hw.SetAudioBlockSize(blockSize); // number of samples handled per callback
	hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);

  reverb.init();

  decayParam.Init(hw.controls[0], 0.0f, 1.0f, Parameter::LINEAR);
  dampParam.Init(hw.controls[1], 0.0f, 1.0f, Parameter::LINEAR);
  modParam.Init(hw.controls[2], 0.0f, 1.0f, Parameter::LINEAR);
  mixParam.Init(hw.controls[3], 0.0f, 1.0f, Parameter::LINEAR);

  TimerHandle::Config config;
  timerHandle.Init(config);

	hw.StartAdc();
	hw.StartAudio(AudioCallback);
	while(1) {}
}
