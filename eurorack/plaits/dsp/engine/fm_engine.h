// Copyright 2016 Emilie Gillet.
//
// Author: Emilie Gillet (emilie.o.gillet@gmail.com)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
// 
// See http://creativecommons.org/licenses/MIT/ for more information.
//
// -----------------------------------------------------------------------------
//
// Classic 2-op FM, as found in Braids, Rings and Elements.

#ifndef PLAITS_DSP_ENGINE_FM_ENGINE_H_
#define PLAITS_DSP_ENGINE_FM_ENGINE_H_

#include "plaits/dsp/engine/engine.h"

namespace plaits {
  
class FMEngine : public Engine {
 public:
  FMEngine() { }
  ~FMEngine() { }
  
  virtual void Init(stmlib::BufferAllocator* allocator);
  virtual void Reset();
  virtual void Render(const EngineParameters& parameters,
      float* out,
      float* aux,
      size_t size,
      bool* already_enveloped);
  
 private:
  inline float SinePM(uint32_t phase, float fm) const;
  
  uint32_t carrier_phase_  = 0;
  uint32_t modulator_phase_ = 0;
  uint32_t sub_phase_ = 0;
  
  float previous_carrier_frequency_ = 0.0f;
  float previous_modulator_frequency_ = 0.0f;
  float previous_amount_ = 0.0f;
  float previous_feedback_ = 0.0f;
  float previous_sample_ = 0.0f;
  
  float sub_fir_ = 0.0f;
  float carrier_fir_ = 0.0f;
  
  DISALLOW_COPY_AND_ASSIGN(FMEngine);
};

}  // namespace plaits

#endif  // PLAITS_DSP_ENGINE_FM_ENGINE_H_