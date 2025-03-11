#pragma once
#include "arm_math.h"
#include "daisy_patch.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

class Reverb
{
public:
  Reverb();

  void init();

  void process(AudioHandle::OutputBuffer, size_t);

  void setDamp(float);
  void setDecay(float);
  void setModulation(float);

private:
  void initializeMatrices();
  void updateFeedbackMatrix();
  void calculateDelays();
  float cubicLagrangeInterpolation(float[4], float);

  static constexpr float sampleRate = 48000.0f;
  static constexpr int blockSize = 64;
  static constexpr int numLines = 8;

  const float smoothingMs = 100;
  const float smoothing = pow(0.1, 1000.0 / (smoothingMs * sampleRate));

  float decayParam = 0.5;
  float dampParam = 0.1;
  float modParam = 0.2;

  float decay = 0;
  float damp = 0;
  float mod = 0;

  static constexpr float minDelayMs = 22;
  static constexpr float maxDelayMs = 60;
  static constexpr int delayLineSize = 2 * (int) maxDelayMs * (sampleRate / 1000) + blockSize;

  const float minR = 0.9995;
  const float maxR = 0.99999;

  float targetDelays[numLines];
  float delays[numLines];
  float delayLines[numLines][delayLineSize];
  int delayLineIndex = 0;

  float lowPassSamples[numLines];
  float feedbackSampleData[numLines];
  arm_matrix_instance_f32 feedbackSamples;
  float feedbackOutputData[numLines];
  arm_matrix_instance_f32 feedbackOutput;

  const float modPhaseIncrement = 5.0 / sampleRate;
  const float modMax = 10.0;
  float modDepth = 100.0;
  float modPhase = 0.0;
  float modLinePhases[numLines];

  float outputBuffers[numLines][blockSize];

  float diffusionMatrixData[numLines * numLines];
  float feedbackMatrixData[numLines * numLines];
  arm_matrix_instance_f32 diffusionMatrix;
  arm_matrix_instance_f32 feedbackMatrix;
};
