
#include "Reverb.h"

Reverb::Reverb()
{
}

void Reverb::init()
{
  for(int i = 0; i < numLines; i++)
  {
    arm_fill_f32(0.0, delayLines[i], delayLineSize);
    arm_fill_f32(0.0, outputBuffers[i], blockSize);
    modLinePhases[i] = (i * 2 * M_PI) / numLines;
  }

  arm_fill_f32(0.0, lowPassSamples, numLines);

  decay = decayParam;
  mod = modParam;
  damp = dampParam;

  delayLineIndex = 0;

  initializeMatrices();

  calculateDelays();

  for(int i = 0; i < numLines; i++)
    delays[i] = targetDelays[i];

  updateFeedbackMatrix();
}

void Reverb::setDamp(float val)
{
  dampParam = val;
}

void Reverb::setDecay(float val)
{
  decayParam = val;
}

void Reverb::setModulation(float val)
{
  modParam = val;
}

void Reverb::process(AudioHandle::OutputBuffer buffer, size_t size)
{
  for(int line = 0; line < numLines; line++)
    for(int sample = 0; sample < (int) size; sample++)
      delayLines[line][(delayLineIndex + sample) % delayLineSize] = 0.2 * buffer[0][sample]; 

  const int sr0 = sampleRate / 1000;

  arm_status status;
  float interpolationSamples[4];
  for(int sample = 0; sample < (int) size; sample++)
  {
    for(int line = 0; line < numLines; line++)
    {
      float modulation = delays[line] + (modMax * sr0 + mod * modDepth * arm_sin_f32(modLinePhases[line] + modPhase));

      int interpolationIndex = (delayLineSize + delayLineIndex + sample - (int) modulation - 2) % delayLineSize;

      for(int i = 0; i < 4; i++)
        interpolationSamples[i] = delayLines[line][interpolationIndex];
      
      float gamma = (modulation - floor(modulation));
      float modSample = interpolationSamples[2];
      //float modSample = gamma * interpolationSamples[1] + (1 - gamma) * interpolationSamples[2];
      //float modSample = cubicLagrangeInterpolation(interpolationSamples, 1.0 - gamma); 

      float l0 = 1 -  (1 - damp) * (1 - damp);
      lowPassSamples[line] = l0 * lowPassSamples[line] + (1 - l0) * modSample;

      if(lowPassSamples[line] >  1) lowPassSamples[line] =  1;
      if(lowPassSamples[line] < -1) lowPassSamples[line] = -1;

      feedbackSamples.pData[line] = lowPassSamples[line];
      outputBuffers[line][sample] = lowPassSamples[line];
    }

    modPhase += modPhaseIncrement;
    modPhase *= (modPhase < 2 * M_PI);

    damp = smoothing * damp + (1 - smoothing) * dampParam;
    mod = smoothing * mod + (1 - smoothing) * modParam;
    decay = smoothing * decay + (1 - smoothing) * decayParam;
    updateFeedbackMatrix();


    status = arm_mat_mult_f32(&feedbackSamples,
                     &diffusionMatrix,
                     &feedbackOutput);

    arm_copy_f32(feedbackOutput.pData, feedbackSamples.pData, numLines);

    status = arm_mat_mult_f32(&feedbackSamples,
                     &feedbackMatrix,
                     &feedbackOutput);

    for(int line = 0; line < numLines; line++)
      delayLines[line][(delayLineIndex + sample) % delayLineSize] += feedbackOutput.pData[line];

  }

  /*
  for(int channel = 0; channel < 2; channel++)
    for(int sample = 0; sample < size; sample++)
      for(int line = 0; line < numLines; line += 2)
        buffer[channel][sample] = outputBuffers[line + channel][sample];
  */
  for(int sample = 0; sample < size; sample++)
  {
    buffer[0][sample] = outputBuffers[0][sample] - outputBuffers[2][sample] + outputBuffers[4][sample] - outputBuffers[6][sample];
    buffer[1][sample] = -outputBuffers[1][sample] + outputBuffers[3][sample] - outputBuffers[5][sample] + outputBuffers[7][sample];
  }

  delayLineIndex += size;
  delayLineIndex %= delayLineSize;
}

void Reverb::initializeMatrices()
{
  float alpha = M_PI / 2;
  float c = cos(alpha);
  float s = sin(alpha);

  float rotationMatrixData[] = { c,  0,  0,  0,  0,  s,  0,  0,
                                 0,  c,  0,  s,  0,  0,  0,  0,
                                 0,  0,  c,  0,  0,  0,  0,  s,
                                 0, -s,  0,  c,  0,  0,  0,  0,
                                 0,  0,  0,  0,  c,  0,  s,  0,
                                -s,  0,  0,  0,  0,  c,  0,  0,
                                 0,  0,  0,  0, -s,  0,  c,  0,
                                 0,  0, -s,  0,  0,  0,  0,  c};

  arm_matrix_instance_f32 rotationMatrix;
  arm_mat_init_f32(&rotationMatrix, 
                   numLines, 
                   numLines, 
                   rotationMatrixData);

  float randomMatrixData[] = {-0.38345834,  0.3108451 , -0.00322765, -0.51178442, -0.49213217,  0.02397016, -0.33355148,  0.37466796,
                    -0.02654298, -0.11984763,  0.21959055, -0.43532666, -0.28268424,  0.17871056,  0.76567461, -0.221579,
                    -0.17535621, -0.34617128, -0.62212802,  0.09841386, -0.29456891,  0.50158979, -0.14068938, -0.30745798,
                     0.49050726, -0.57646064, -0.0410285 , -0.08911063, -0.43275748, -0.41343856, -0.11300052,  0.21562031,
                     0.57648317,  0.2934478 , -0.43958422, -0.53213858,  0.30055032,  0.11379304, -0.03392339, -0.02682627,
                    -0.2272003 , -0.43272297,  0.28228561, -0.48185514,  0.36534965, -0.07127675, -0.39330791, -0.39498034,
                     0.24054673,  0.39863141,  0.18506033,  0.10496524, -0.41894284, -0.16821703, -0.23804182, -0.69100743,
                     0.372951  , -0.067843  ,  0.50584205,  0.06758603, -0.06724367,  0.70614304, -0.2396192 ,  0.1878397};

  arm_matrix_instance_f32 randomMatrix;
  arm_mat_init_f32(&randomMatrix,
                   numLines,
                   numLines,
                   randomMatrixData);

  float baseMatrixData[] = {0,  0,  0,  0,  0,  1,  1,  0,
                            0,  0,  0,  1,  0,  0,  0, -1,
                            0,  0,  0, -1,  0,  0,  0, -1,
                            0,  1,  1,  0,  0,  0,  0,  0,
                            0,  0,  0,  0,  0, -1,  1,  0,
                            1,  0,  0,  0, -1,  0,  0,  0,
                           -1,  0,  0,  0, -1,  0,  0,  0,
                            0, -1,  1,  0,  0,  0,  0,  0};

  for(int i = 0; i < numLines * numLines; i++)
    baseMatrixData[i] *= 1.0 / sqrt(2);

  arm_matrix_instance_f32 baseMatrix;
  arm_mat_init_f32(&baseMatrix,
                   numLines,
                   numLines,
                   baseMatrixData);

  arm_mat_init_f32(&diffusionMatrix,
                   numLines,
                   numLines,
                   diffusionMatrixData);

  float tmpData[numLines * numLines];
  arm_matrix_instance_f32 tmpMatrix;
  arm_mat_init_f32(&tmpMatrix,
                   numLines,
                   numLines,
                   tmpData);

  arm_mat_mult_f32(&baseMatrix,
                   &baseMatrix,
                   &tmpMatrix);

  arm_mat_mult_f32(&tmpMatrix,
                   &randomMatrix,
                   &diffusionMatrix);

  arm_copy_f32(diffusionMatrix.pData, tmpMatrix.pData, numLines * numLines);

  arm_mat_mult_f32(&tmpMatrix,
                   &rotationMatrix,
                   &diffusionMatrix);

  arm_fill_f32(0.0, feedbackMatrixData, numLines * numLines);
  arm_mat_init_f32(&feedbackMatrix,
                   numLines,
                   numLines,
                   feedbackMatrixData);

  arm_fill_f32(0.0, feedbackSampleData, numLines);
  arm_mat_init_f32(&feedbackSamples,
                   1,
                   numLines,
                   feedbackSampleData);

  arm_fill_f32(0.0, feedbackOutputData, numLines);
  arm_mat_init_f32(&feedbackOutput,
                   1,
                   numLines,
                   feedbackOutputData);
}

void Reverb::updateFeedbackMatrix()
{
  float r = minR + ((maxR - minR) * pow(decay, 0.3));
  float* feedbackMatrixData = feedbackMatrix.pData;
  for(int i = 0; i < numLines; i++)
    feedbackMatrixData[i + i * numLines] = pow(r, delays[i]);
}

void Reverb::calculateDelays()
{
  const float delayScale = pow((float) maxDelayMs / minDelayMs, 1.0 / numLines);
  targetDelays[0] = ((float) minDelayMs * sampleRate) / 1000;
  for(int i = 1; i < numLines; i++)
    targetDelays[i] = targetDelays[i - 1] * delayScale;
}


float Reverb::cubicLagrangeInterpolation(float y[4], float x)
{
  float basisPolynomial[4] = {1, 1, 1, 1};
  for(int i = 0; i < 4; i++)
  {
    for(int m = 0; m < i; m++)
      basisPolynomial[i] *= (x - (m - 1)) / ((i - 1) - (m - 1));

    for(int m = i + 1; m < 4; m++)
      basisPolynomial[i] *= (x - (m - 1)) / ((i - 1) - (m - 1));
  }
    
  float tmp = 0;
    
  for(int i = 0; i < 4; i++)
      tmp += y[i] * basisPolynomial[i];
    
  return tmp;
}
