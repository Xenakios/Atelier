//#include "AudibleInstruments.hpp"
#include "plugin.hpp"

#include <sys/stat.h>

#pragma GCC diagnostic push
#ifndef __clang__
#pragma GCC diagnostic ignored "-Wsuggest-override"
#endif
#include "plaits/dsp/voice.h"
#pragma GCC diagnostic pop

#define MAX_PALETTE_VOICES 16

class NVGRestorer
{
public:
	NVGRestorer() = delete;
	NVGRestorer(NVGcontext* ctx) : mCtx(ctx) 
	{
		if (mCtx) nvgSave(mCtx);
	}
	~NVGRestorer()
	{
		if (mCtx) nvgRestore(mCtx);
	} 
private:
	NVGcontext* mCtx = nullptr;
};

struct Palette : Module {
	enum ParamIds {
		MODEL1_PARAM,
		MODEL2_PARAM,
		FREQ_PARAM,
		HARMONICS_PARAM,
		TIMBRE_PARAM,
		MORPH_PARAM,
		TIMBRE_CV_PARAM,
		FREQ_CV_PARAM,
		MORPH_CV_PARAM,
		TIMBRE_LPG_PARAM,
		FREQ_LPG_PARAM,
		MORPH_LPG_PARAM,
		LPG_COLOR_PARAM,
		LPG_DECAY_PARAM,
		OUTMIX_PARAM,
		HARMONICS_CV_PARAM,
		HARMONICS_LPG_PARAM,
		UNISONOMODE_PARAM,
		UNISONOSPREAD_PARAM,
		OUTMIX_CV_PARAM,
		OUTMIX_LPG_PARAM,
		DECAY_CV_PARAM,
		LPG_COLOR_CV_PARAM,
		ENGINE_CV_PARAM,
		UNISONOSPREAD_CV_PARAM,
		SECONDARY_FREQ_PARAM,
		WAVETABLE_AUX_MODE,
		NUM_PARAMS
	};
	enum InputIds {
		ENGINE_INPUT,
		TIMBRE_INPUT,
		FREQ_INPUT,
		MORPH_INPUT,
		HARMONICS_INPUT,
		TRIGGER_INPUT,
		LEVEL_INPUT,
		NOTE_INPUT,
		LPG_COLOR_INPUT,
		LPG_DECAY_INPUT,
		SPREAD_INPUT,
		OUTMIX_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		OUT_OUTPUT,
		AUX_OUTPUT,
		AUX2_OUTPUT,
		PITCH_SPREAD_OUTPUT,
		NUM_OUTPUTS
	};
	plaits::Voice voice[MAX_PALETTE_VOICES];
	plaits::Patch patch[MAX_PALETTE_VOICES] = {};
	plaits::Modulations modulations[MAX_PALETTE_VOICES] = {};
	char shared_buffer[MAX_PALETTE_VOICES][32768] = {};
	
	int lpg_mode = 0;
	dsp::SampleRateConverter<2> outputSrc[MAX_PALETTE_VOICES];
	dsp::DoubleRingBuffer<dsp::Frame<2>, 256> outputBuffer[MAX_PALETTE_VOICES];
	bool lowCpu = false;
	bool freeTune = false;
	bool showModulations = true;
	bool lfoMode = false;
	dsp::SchmittTrigger model1Trigger;
	dsp::SchmittTrigger model2Trigger;

	float currentOutmix[16];
	float currentPitch = 0.0f;

	int curNumVoices = 0;
	int curResamplingQ = 4;
	Palette() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS);
		configParam(MODEL1_PARAM, 0.0, 1.0, 0.0, "Model selection 1");
		configParam(MODEL2_PARAM, 0.0, 1.0, 0.0, "Model selection 2");
		configParam(FREQ_PARAM, -4.0, 4.0, 0.0, "Coarse frequency adjustment");
		configParam(HARMONICS_PARAM, 0.0, 1.0, 0.5, "Harmonics");
		configParam(TIMBRE_PARAM, 0.0, 1.0, 0.5, "Timbre");
		configParam(MORPH_PARAM, 0.0, 1.0, 0.5, "Morph");
		configParam(TIMBRE_CV_PARAM, -1.0, 1.0, 0.0, "Timbre CV");
		configParam(FREQ_CV_PARAM, -1.0, 1.0, 0.0, "Frequency CV");
		configParam(MORPH_CV_PARAM, -1.0, 1.0, 0.0, "Morph CV");
		configParam(TIMBRE_LPG_PARAM, -1.0, 1.0, 0.0, "Internal AD envelope to Timbre amount");
		// doubles as Speech speed
		struct CustomQuantity1 : ParamQuantity {
			std::string getLabel() override
			{
				Palette* module = reinterpret_cast<Palette*>(this->module);
				if (module->voice[0].active_engine()==7)
				{
					return "Speech speed";
				}
				return "Internal AD envelope to Morph amount";
			}
			
		};
		configParam<CustomQuantity1>(MORPH_LPG_PARAM, -1.0, 1.0, 0.0, "Internal AD envelope to Morph amount");
		struct CustomQuantity2 : ParamQuantity {
			std::string getLabel() override
			{
				Palette* module = reinterpret_cast<Palette*>(this->module);
				if (module->voice[0].active_engine()==7)
				{
					return "Speech intonation";
				}
				return "Internal AD envelope to Frequency amount";
			}
			
		};
		// doubles as Speech intonation amount
		configParam<CustomQuantity2>(FREQ_LPG_PARAM, -1.0, 1.0, 0.0, "Internal AD envelope to Frequency amount");
		configParam(LPG_COLOR_PARAM, 0.0, 1.0, 0.5, "LPG Colour");
		configParam(LPG_DECAY_PARAM, 0.0, 1.0, 0.5, "LPG/Internal envelope Decay");
		configParam(OUTMIX_PARAM, 0.0, 1.0, 0.5, "Output mix");
		configParam(HARMONICS_CV_PARAM, -1.0, 1.0, 0.0, "Harmonics CV");
		configParam(HARMONICS_LPG_PARAM, -1.0, 1.0, 0.0, "Internal AD envelope to Harmonics amount");
		configParam(UNISONOMODE_PARAM, 1.0, 16.0, 1.0, "Unisono/Spread num voices");
		paramQuantities[UNISONOMODE_PARAM]->snapEnabled = true;
		configParam(UNISONOSPREAD_PARAM, 0.0, 1.0, 0.05, "Unisono/Spread");
		configParam(OUTMIX_CV_PARAM, -1.0, 1.0, 0.0, "Output mix CV");
		configParam(OUTMIX_LPG_PARAM, -1.0, 1.0, 0.0, "Internal AD envelope to Output mix amount");
		configParam(DECAY_CV_PARAM, -1.0, 1.0, 0.0, "Decay CV");
		configParam(LPG_COLOR_CV_PARAM, -1.0, 1.0, 0.0, "LPG Colour CV");
		configParam(ENGINE_CV_PARAM, -1.0, 1.0, 0.0, "Engine choice CV");
		configParam(UNISONOSPREAD_CV_PARAM, -1.0, 1.0, 0.0, "Unisono/Spread CV");
		configParam(SECONDARY_FREQ_PARAM, -7.0, 7.0, 0.0, "Tuning");
		configParam(WAVETABLE_AUX_MODE, 0.0, 7.0, 0.0, "Wavetable Aux output mode");
		configOutput(AUX2_OUTPUT, "Mixed audio");
		configOutput(OUT_OUTPUT,"Primary audio");
		configOutput(AUX_OUTPUT,"Secondary audio");
		
		configInput(ENGINE_INPUT,"Engine choice CV");
		configInput(TIMBRE_INPUT,"Timbre CV");
		configInput(MORPH_INPUT,"Morph CV");
		configInput(HARMONICS_INPUT,"Harmonics CV");
		configInput(OUTMIX_INPUT,"Output Mix CV");
		configInput(SPREAD_INPUT,"Spread CV");
		configInput(TRIGGER_INPUT,"LPG/Internal envelope trigger");
		configInput(NOTE_INPUT,"Pitch (1V/Oct)");
		configInput(FREQ_INPUT,"FM");
		configInput(LEVEL_INPUT,"Level CV");
		configInput(ENGINE_INPUT,"Engine CV");
		configInput(LPG_DECAY_INPUT,"LPG/Internal envelope decay CV");
		configInput(LPG_COLOR_INPUT,"LPG Color CV");

		for (int i=0;i<MAX_PALETTE_VOICES;++i)
		{
			memset(shared_buffer[i],0,sizeof(shared_buffer[i]));
			stmlib::BufferAllocator allocator(shared_buffer[i], sizeof(shared_buffer[i]));
			voice[i].Init(&allocator);
			outputSrc[i].setQuality(curResamplingQ);
			currentOutmix[i]=0.0f;
		}
		onReset();
	}
	~Palette()
	{
		
	}
	void onReset() override {
		for (int i=0;i<MAX_PALETTE_VOICES;++i)
		{
			patch[i].engine = 0;
			patch[i].lpg_colour = 0.5f;
			patch[i].decay = 0.5f;
		}
		lfoMode = false;
		lpg_mode = 0;
		freeTune = false;
	}
	int getResamplingQuality()
	{
		if (lowCpu)
			return -1;
		return curResamplingQ;
	}
	void setResamplingQuality(int qu)
	{
		if (qu == -1)
		{
			lowCpu = true;
			return;
		}
		lowCpu = false;
		curResamplingQ = qu;
	}
	void onRandomize() override {
		int engineindex = random::u32() % 16;
		for (int i=0;i<MAX_PALETTE_VOICES;++i)
			patch[i].engine = engineindex;
	}

	json_t *dataToJson() override {
		json_t *rootJ = json_object();

		json_object_set_new(rootJ, "rsqual", json_integer(getResamplingQuality()));
		json_object_set_new(rootJ, "model", json_integer(patch[0].engine));
		json_object_set_new(rootJ, "lpgColor", json_real(patch[0].lpg_colour));
		json_object_set_new(rootJ, "decay", json_real(patch[0].decay));
		json_object_set_new(rootJ, "freetune", json_boolean(freeTune));
		json_object_set_new(rootJ, "lfoMode", json_boolean(lfoMode));
		json_object_set_new(rootJ, "showmods", json_boolean(showModulations));
		json_object_set_new(rootJ, "lpgMode", json_integer(lpg_mode));
		return rootJ;
	}

	void dataFromJson(json_t *rootJ) override {
		json_t *lowCpuJ = json_object_get(rootJ, "lowCpu");
		if (lowCpuJ)
		{
			bool lowCpuTemp = json_boolean_value(lowCpuJ);
			if (lowCpuTemp == true)
			{
				setResamplingQuality(-1);
			}
		}
		json_t *rsQualJ = json_object_get(rootJ,"rsqual");
		if (rsQualJ)
			setResamplingQuality(json_integer_value(rsQualJ));
		json_t *freetuneJ = json_object_get(rootJ, "freetune");
		if (freetuneJ)
			freeTune = json_boolean_value(freetuneJ);
		json_t *lfoModeJ = json_object_get(rootJ, "lfoMode");
		if (lfoModeJ)
			lfoMode = json_boolean_value(lfoModeJ);
		json_t *showmodJ = json_object_get(rootJ, "showmods");
		if (showmodJ)
			showModulations = json_boolean_value(showmodJ);

		json_t *modelJ = json_object_get(rootJ, "model");
		if (modelJ)
			for (int i=0;i<MAX_PALETTE_VOICES;++i)
				patch[i].engine = json_integer_value(modelJ);

		json_t *lpgColorJ = json_object_get(rootJ, "lpgColor");
		if (lpgColorJ)
			for (int i=0;i<MAX_PALETTE_VOICES;++i)
				patch[i].lpg_colour = json_number_value(lpgColorJ);

		json_t *decayJ = json_object_get(rootJ, "decay");
		if (decayJ)
			for (int i=0;i<MAX_PALETTE_VOICES;++i)
				patch[i].decay = json_number_value(decayJ);
		
		json_t *lpgModeJ = json_object_get(rootJ, "lpgMode");
		if (lpgModeJ)
			lpg_mode = json_integer_value(lpgModeJ);
		
	}
	float getModulatedParamNormalized(int paramid, int whichvoice=0)
	{
		if (whichvoice<curNumVoices)
		{
			if (paramid==FREQ_PARAM)
				return rescale(voice[whichvoice].epars.note,12.0f,108.0f,0.0f,1.0f);
			if (paramid==HARMONICS_PARAM)
				return voice[whichvoice].epars.harmonics;
			if (paramid==MORPH_PARAM)
				return voice[whichvoice].epars.morph;
			if (paramid==TIMBRE_PARAM)
				return voice[whichvoice].epars.timbre;
			if (paramid==OUTMIX_PARAM)
				return currentOutmix[whichvoice];
		}
		return -1.0f;
	}
	inline float getUniSpreadAmount(int numchans, int chan, float spreadpar)
	{
		// spread slowly to ± 0.5 semitones
		if (spreadpar<0.5f)
		{
			float spreadnorm = spreadpar*2.0f;
			float spreadmt = spreadnorm*0.5;
			return rescale(chan,0,numchans-1,-spreadmt,spreadmt);
		// spread faster to ± 1 octaves
		} else if (spreadpar>=0.5f && spreadpar<0.9f)
		{
			float spreadnorm = (spreadpar-0.5f)*2.0f;
			float spreadmt = rescale(spreadpar,0.5f,0.9f,0.5f,12.0f);
			return rescale(chan,0,numchans-1,-spreadmt,spreadmt);
		// finally morph towards -1, 0, +1 octave for all voices
		} else
		{
			float interpos = rescale(spreadpar,0.9f,1.0f,0.0f,1.0f);
			interpos = 1.0-(std::pow(1.0-interpos,3.0f));
			float y0 = rescale(chan,0,numchans-1,-12.0f,12.0f);
			const float pitches[3] = {-12.0f,0.0f,12.0f};
			int index = chan % 3;
			float y1 = pitches[index];
			return y0+(y1-y0)*interpos;
		}
		return 0.0f;
	}
	void process(const ProcessArgs &args) override {
		float spreadamt = params[UNISONOSPREAD_PARAM].getValue();
		if (inputs[SPREAD_INPUT].isConnected())
		{
			spreadamt += 
				rescale(inputs[SPREAD_INPUT].getVoltage()*params[UNISONOSPREAD_CV_PARAM].getValue(),
				-5.0f,5.0f,-0.5f,0.5f);
			spreadamt = clamp(spreadamt,0.0f,1.0f);
		}
		int numpolychs = std::max(inputs[NOTE_INPUT].getChannels(),1);
		int unispreadchans = params[UNISONOMODE_PARAM].getValue();
		if (unispreadchans>=2)
			numpolychs = unispreadchans;
		curNumVoices = numpolychs;
		if (outputBuffer[0].empty()) {
			const int blockSize = 12;
			if (curResamplingQ!=outputSrc[0].quality)
			{
				for (int i=0;i<numpolychs;++i)
				{
					outputSrc[i].setQuality(curResamplingQ);
				}
			}
			float pitchAdjust = params[SECONDARY_FREQ_PARAM].getValue();
			// Model buttons
			if (model1Trigger.process(params[MODEL1_PARAM].getValue())) {
				for (int i=0;i<MAX_PALETTE_VOICES;++i)
				{
					if (patch[i].engine >= 8) {
						patch[i].engine -= 8;
					}
					else {
						patch[i].engine = (patch[i].engine + 1) % 8;
					}
				}
			}
			if (model2Trigger.process(params[MODEL2_PARAM].getValue())) {
				for (int i=0;i<MAX_PALETTE_VOICES;++i)
				{
					if (patch[i].engine < 8) {
						patch[i].engine += 8;
					}
					else {
						patch[i].engine = (patch[i].engine + 1) % 8 + 8;
					}
				}
			}

			int activeEngine = voice[0].active_engine();
			
			float pitch;
			if (!freeTune)
				pitch = std::round(params[FREQ_PARAM].getValue());
			else pitch = params[FREQ_PARAM].getValue();
			currentPitch = pitch;
			// Calculate pitch for lowCpu mode if needed
			if (lowCpu)
				pitch += log2f(48000.f * args.sampleTime);
			// Update patch
			bool fm_input_connected = inputs[FREQ_INPUT].isConnected();
			for (int i=0;i<numpolychs;++i)
			{
				voice[i].wsAuxMode = params[WAVETABLE_AUX_MODE].getValue();
				voice[i].lpg_behavior = (plaits::Voice::LPGBehavior)lpg_mode;
				if (!lfoMode)
					patch[i].note = 60.f + pitch * 12.f + pitchAdjust;
				else
					patch[i].note = -48.37f + pitch * 12.f + pitchAdjust;
				if (unispreadchans>1)
					patch[i].note+=getUniSpreadAmount(unispreadchans,i,spreadamt);
				patch[i].harmonics = params[HARMONICS_PARAM].getValue();
				
				patch[i].timbre = params[TIMBRE_PARAM].getValue();
				patch[i].morph = params[MORPH_PARAM].getValue();
				
				float lpg_colour = params[LPG_COLOR_PARAM].getValue();
				if (inputs[LPG_COLOR_INPUT].getChannels() < 2)
					lpg_colour += inputs[LPG_COLOR_INPUT].getVoltage()/10.0f*params[LPG_COLOR_CV_PARAM].getValue();
				else lpg_colour += inputs[LPG_COLOR_INPUT].getVoltage(i)/10.0f*params[LPG_COLOR_CV_PARAM].getValue();
				patch[i].lpg_colour = clamp(lpg_colour,0.0f,1.0f);
				float decay = params[LPG_DECAY_PARAM].getValue();
				if (inputs[LPG_DECAY_INPUT].getChannels() < 2)
					decay += inputs[LPG_DECAY_INPUT].getVoltage()/10.0f*params[DECAY_CV_PARAM].getValue();
				else decay += inputs[LPG_DECAY_INPUT].getVoltage(i)/10.0f*params[DECAY_CV_PARAM].getValue();
				patch[i].decay = clamp(decay,0.0f,1.0);
				
				if (fm_input_connected)
					patch[i].frequency_cv_amount = params[FREQ_CV_PARAM].getValue();
				else patch[i].frequency_cv_amount = 0.0f;
				patch[i].timbre_cv_amount = params[TIMBRE_CV_PARAM].getValue();
				patch[i].morph_cv_amount = params[MORPH_CV_PARAM].getValue();
				patch[i].harmonics_cv_amount = params[HARMONICS_CV_PARAM].getValue();
				patch[i].frequency_lpg_amount = params[FREQ_LPG_PARAM].getValue();
				patch[i].timbre_lpg_amount = params[TIMBRE_LPG_PARAM].getValue();
				patch[i].morph_lpg_amount = params[MORPH_LPG_PARAM].getValue();
				patch[i].harmonics_lpg_amount = params[HARMONICS_LPG_PARAM].getValue();
				// Update modulations
				if (inputs[ENGINE_INPUT].getChannels() < 2)
					modulations[i].engine = inputs[ENGINE_INPUT].getVoltage() / 5.f * params[ENGINE_CV_PARAM].getValue();
				else
					modulations[i].engine = inputs[ENGINE_INPUT].getVoltage(i) / 5.f * params[ENGINE_CV_PARAM].getValue();
				if (unispreadchans<2)
					modulations[i].note = inputs[NOTE_INPUT].getVoltage(i) * 12.f;
				else
					modulations[i].note = inputs[NOTE_INPUT].getVoltage() * 12.f;
				if (inputs[FREQ_INPUT].getChannels() < 2)
					modulations[i].frequency = inputs[FREQ_INPUT].getVoltage() * 6.f;
				else 
					modulations[i].frequency = inputs[FREQ_INPUT].getVoltage(i) * 6.f;
				if (inputs[HARMONICS_INPUT].getChannels() < 2)
					modulations[i].harmonics = inputs[HARMONICS_INPUT].getVoltage() / 10.f;
				else
					modulations[i].harmonics = inputs[HARMONICS_INPUT].getVoltage(i) / 10.f;
				if (inputs[TIMBRE_INPUT].getChannels() < 2)
					modulations[i].timbre = inputs[TIMBRE_INPUT].getVoltage() / 10.f;
				else
					modulations[i].timbre = inputs[TIMBRE_INPUT].getVoltage(i) / 10.f;
				if (inputs[MORPH_INPUT].getChannels() < 2)
					modulations[i].morph = inputs[MORPH_INPUT].getVoltage() / 10.f;
				else
					modulations[i].morph = inputs[MORPH_INPUT].getVoltage(i) / 10.f;
				// Triggers at around 0.7 V
				if (inputs[TRIGGER_INPUT].getChannels() < 2)
				{
					modulations[i].trigger = inputs[TRIGGER_INPUT].getVoltage() / 3.f;
				}
				else
				{
					if (unispreadchans>1) // discard triggers in unispread mode above channel 1
						modulations[i].trigger = inputs[TRIGGER_INPUT].getVoltage() / 3.f;
					else
						modulations[i].trigger = inputs[TRIGGER_INPUT].getVoltage(i) / 3.f;
				}
				if (inputs[LEVEL_INPUT].getChannels() < 2)
				{
					modulations[i].level = inputs[LEVEL_INPUT].getVoltage() / 8.f;
				}
				else
				{
					if (unispreadchans>1)
						modulations[i].level = inputs[LEVEL_INPUT].getVoltage() / 8.f;
					else
						modulations[i].level = inputs[LEVEL_INPUT].getVoltage(i) / 8.f;
				}

				modulations[i].frequency_patched = inputs[FREQ_INPUT].isConnected();
				modulations[i].timbre_patched = inputs[TIMBRE_INPUT].isConnected();
				modulations[i].morph_patched = inputs[MORPH_INPUT].isConnected();
				modulations[i].harmonics_patched = inputs[HARMONICS_INPUT].isConnected();
				modulations[i].trigger_patched = inputs[TRIGGER_INPUT].isConnected();
				modulations[i].level_patched = inputs[LEVEL_INPUT].isConnected();
			}
			

			// Render frames
			for (int polych=0;polych<numpolychs;++polych)
			{
				plaits::Voice::Frame output[blockSize];
				voice[polych].Render(patch[polych], modulations[polych], output, blockSize);

				// Convert output to frames
				dsp::Frame<2> outputFrames[blockSize];
				for (int i = 0; i < blockSize; i++) {
					outputFrames[i].samples[0] = output[i].out / 32768.f;
					outputFrames[i].samples[1] = output[i].aux / 32768.f;
				}

				// Convert output
				if (lowCpu) {
					int len = std::min((int) outputBuffer[polych].capacity(), blockSize);
					memcpy(outputBuffer[polych].endData(), outputFrames, len * sizeof(dsp::Frame<2>));
					outputBuffer[polych].endIncr(len);
				}
				else {
					outputSrc[polych].setRates(48000, args.sampleRate);
					int inLen = blockSize;
					int outLen = outputBuffer[polych].capacity();
					outputSrc[polych].process(outputFrames, &inLen, outputBuffer[polych].endData(), &outLen);
					outputBuffer[polych].endIncr(outLen);
				}
			}
		}
		outputs[OUT_OUTPUT].setChannels(numpolychs);
		outputs[AUX_OUTPUT].setChannels(numpolychs);
		outputs[AUX2_OUTPUT].setChannels(numpolychs);
		
		// Set output
		float outmixbase = params[OUTMIX_PARAM].getValue();
		for (int i=0;i<numpolychs;++i)
		{
			float outmix = outmixbase;
			outmix += rescale(inputs[OUTMIX_INPUT].getVoltage(i)*params[OUTMIX_CV_PARAM].getValue(),
			-5.0f,5.0f,-0.5f,0.5f);
			outmix += voice[i].getDecayEnvelopeValue()*params[OUTMIX_LPG_PARAM].getValue();
			outmix = clamp(outmix,0.0f,1.0f);
			currentOutmix[i] = outmix;
			if (!outputBuffer[i].empty()) {
				dsp::Frame<2> outputFrame = outputBuffer[i].shift();
				// Inverting op-amp on outputs
				float out1 = -outputFrame.samples[0] * 5.f;
				float out2 = -outputFrame.samples[1] * 5.f;
				outputs[OUT_OUTPUT].setVoltage(out1,i);
				outputs[AUX_OUTPUT].setVoltage(out2,i);
				float out3 = outmix*out2 + (1.0f-outmix)*out1;
				outputs[AUX2_OUTPUT].setVoltage(out3,i);
			}
		}
	}
};


static const std::string modelLabels[16] = {
	"Pair of classic waveforms",
	"Waveshaping oscillator",
	"Two operator FM",
	"Granular formant oscillator",
	"Harmonic oscillator",
	"Wavetable oscillator",
	"Chords",
	"Vowel and speech synthesis",
	"Granular cloud",
	"Filtered noise",
	"Particle noise",
	"Inharmonic string modeling",
	"Modal resonator",
	"Analog bass drum",
	"Analog snare drum",
	"Analog hi-hat",
};

struct PaletteKnobLarge : app::SvgKnob {
	PaletteKnobLarge() {
		minAngle = -0.78 * M_PI;
		maxAngle = 0.78 * M_PI;
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/palette/palette_knobL.svg")));
	}
	void drawLayer(const DrawArgs& args, int layer) override
	{
		if (this->getParamQuantity()==nullptr)
		{
			Widget::drawLayer(args,layer);
			return;
		}
		auto modul = dynamic_cast<Palette*>(this->getParamQuantity()->module);
		if (modul && layer == 1)
		{
			if (getParamQuantity()->paramId == Palette::FREQ_PARAM)
				getParamQuantity()->snapEnabled = !modul->freeTune;
			if (modul->showModulations==false)
				return;
			static const NVGcolor colors[5]=
			{
				nvgRGBA(0xEA, 0xEC, 0xEF, 0xff),
				nvgRGBA(0x00, 0xee, 0x00, 0xff),
				nvgRGBA(0xee, 0x00, 0x00, 0xff),
				nvgRGBA(0x00, 0x00, 0xee, 0xff),
				nvgRGBA(0x00, 0xee, 0xee, 0xff),
			};
			NVGRestorer rs(args.vg);
			for (int i=0;i<16;++i)
			{
				float modulated = modul->getModulatedParamNormalized(this->getParamQuantity()->paramId,i);
				if (modulated>=0.0f)
				{
					float angle = rescale(modulated,0.0f,1.0f,this->minAngle,this->maxAngle)-1.5708;
					float xpos = args.clipBox.pos.x;
					float ypos = args.clipBox.pos.y;
					float w = args.clipBox.size.x;
					float h = args.clipBox.size.y;
					float xcor0 = xpos + (w / 2.0f) + 13.0f * std::cos(angle);
					float ycor0 = ypos + (w / 2.0f) + 13.0f * std::sin(angle);
					float xcor1 = xpos + (w / 2.0f) + 8.0f * std::cos(angle);
					float ycor1 = ypos + (w / 2.0f) + 8.0f * std::sin(angle);
					nvgBeginPath(args.vg);
					nvgStrokeColor(args.vg,colors[0]);
					//nvgStrokeWidth(args.vg,5.0f);
					nvgMoveTo(args.vg,xcor0,ycor0);
					nvgLineTo(args.vg,xcor1,ycor1);
					nvgStroke(args.vg);
					nvgRotate(args.vg,0.0f);
				}
				
			}
		}
		Widget::drawLayer(args,layer);
	}
	void draw(const DrawArgs& args) override
    {
        SvgKnob::draw(args);
    }
};

struct PaletteKnobSmall : app::SvgKnob {
	PaletteKnobSmall() {
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/palette/palette_knobS.svg")));
	}
	void draw(const DrawArgs& args) override
	{
		nvgSave(args.vg);
		auto module = dynamic_cast<Palette*>(this->module);
		auto pq = getParamQuantity();
		if (pq && module)
		{
			if (module->voice[0].active_engine()==7 
				&& (pq->paramId == Palette::MORPH_LPG_PARAM 
				|| pq->paramId == Palette::FREQ_LPG_PARAM))
			{
				nvgTint(args.vg,nvgRGBA(0,255,0,255));
			}
			
		}
		
		SvgKnob::draw(args);
		nvgRestore(args.vg);
	}
};

struct MyPort1 : app::SvgPort {
	MyPort1() {
		setSvg(APP->window->loadSvg(asset::plugin(pluginInstance,"res/palette/palette_jack.svg")));
	}
};

struct PaletteButton : app::SvgSwitch {
	PaletteButton() {
		momentary = true;
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance,"res/palette/palette_push.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance,"res/palette/palette_pushed.svg")));
		
	}
};

struct Model_LEDWidget : public TransparentWidget
{
	Model_LEDWidget(Palette* m)
	{
		mPalette = m;
	}
	void drawLayer(const DrawArgs& args, int layer) override
	{
		if (layer == 1 && mPalette)
		{
			NVGRestorer nr(args.vg);
			static const NVGcolor inactive = nvgRGBA(0x00,0x00,0x00,0xff);
			static const NVGcolor active = nvgRGBA(0x84,0x84,0x84,0xff);
			static const NVGcolor modulatedCols[2] = {nvgRGBA(0x00,0xB5,0x91,0xff),
				nvgRGBA(0xEA,0x55,0x4E,0xff)};
			int baseEngineIndex = mPalette->patch[0].engine;
			int baseEngineBank = baseEngineIndex / 8;
			int numVoices = mPalette->curNumVoices;
			for (int i=0;i<8;++i)
			{
				nvgBeginPath(args.vg);
				if ((baseEngineIndex % 8) == i)
					nvgFillColor(args.vg,active);
				else nvgFillColor(args.vg,inactive);
				nvgEllipse(args.vg,positions[i][0],positions[i][1],3.5f,3.5f);
				nvgFill(args.vg);
			}
			//if (mPalette->inputs[Palette::ENGINE_INPUT].isConnected())
			{
				for (int i=0;i<numVoices;++i)
				{
					int modelIndex = mPalette->voice[i].active_engine();
					//if (modelIndex == baseEngineIndex)
					//	continue;
					int bank = modelIndex / 8;
					nvgBeginPath(args.vg);
					nvgFillColor(args.vg,modulatedCols[bank]);
					
					modelIndex = modelIndex % 8;
					nvgEllipse(args.vg,positions[modelIndex][0],positions[modelIndex][1],1.5f,1.5f);
					
					nvgFill(args.vg);
				}
			}
		}
		Widget::drawLayer(args,layer);
	}
	void draw(const DrawArgs& args) override
	{
		if (mPalette==nullptr)
			return;
		
	}
	void onButton(const event::Button& e) override
	{
		if (e.action!=GLFW_PRESS)
			return;
		if (e.button != GLFW_MOUSE_BUTTON_LEFT)
			return;
		int engineBank = mPalette->patch[0].engine / 8;
		for (int i=0;i<8;++i)
		{
			Rect r{positions[i][0]-3.5f,positions[i][1]-3.5f,7.0f,7.0f};
			if (r.contains(e.pos))
			{
				for (int j=0;j<MAX_PALETTE_VOICES;++j)
					mPalette->patch[j].engine = (engineBank*8)+i;
				break;
			}
		}
	}
	Palette* mPalette = nullptr;
	const float positions[8][2]=
		{
			{96.5,102.5},
			{103.5,	90.5},
			{113.5,	81.5},
			{127.5,	75.5},
			{142.5,	75.5},
			{156.5,	81.5},
			{166.5,	90.5},
			{173.5,	102.5}
		};
};

struct PaletteWidget : ModuleWidget {
	rack::FramebufferWidget* fbWidget = nullptr;
	
	void step() override
	{
		ModuleWidget::step();
		auto plaits = dynamic_cast<Palette*>(module);
		if (plaits)
		{
			if (plaits->patch->engine!=curSubPanel)
			{
				curSubPanel = plaits->patch->engine;
				if (subPanels[curSubPanel]!=nullptr)
				{
					swgWidget->show();	
					swgWidget->setSvg(subPanels[curSubPanel]);
					if (fbWidget!=nullptr)
						fbWidget->dirty = true;
				}
				else 
				{
					swgWidget->hide();
				}
			}
		}
		
	}
	PaletteWidget(Palette *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/palette/paletteBG.svg")));
		subPanels.resize(16);
		char resName[128];
		for (int i=0;i<16;++i)
		{
			sprintf(resName,"res/palette/palette_%d.svg",i+1);
			subPanels[i] = APP->window->loadSvg(asset::plugin(pluginInstance, resName));	
		}
#define FBFORSVG
#ifdef FBFORSVG
		fbWidget = new FramebufferWidget;
		fbWidget->box = {{0,0},{box.size}};
		addChild(fbWidget);
		swgWidget = new SvgWidget;
		swgWidget->setSvg(subPanels[0]);
		fbWidget->addChild(swgWidget);
		swgWidget->box = {{0,0},{box.size}};
		fbWidget->dirty = true;
#else
		swgWidget = new SvgWidget;
		swgWidget->setSvg(subPanels[0]);
		addChild(swgWidget);
		swgWidget->box = {{0,0},{box.size}};
#endif
		addParam(createParamCentered<PaletteKnobLarge>(Vec(71, 235.5), module, Palette::FREQ_PARAM));
		addParam(createParamCentered<PaletteKnobLarge>(Vec(199,235.5), module, Palette::SECONDARY_FREQ_PARAM));
		addParam(createParamCentered<PaletteKnobLarge>(Vec(135,198.5), module, Palette::HARMONICS_PARAM));
		addParam(createParamCentered<PaletteKnobLarge>(Vec(83.5,145.5), module, Palette::TIMBRE_PARAM));
		addParam(createParamCentered<PaletteKnobLarge>(Vec(186.5,145.5), module, Palette::MORPH_PARAM));
		addParam(createParamCentered<PaletteKnobLarge>(Vec(135,283.5), module, Palette::OUTMIX_PARAM));
		addParam(createParamCentered<PaletteKnobLarge>(Vec(135,283.5), module, Palette::OUTMIX_PARAM));
		
		addOutput(createOutputCentered<MyPort1>(Vec(135, 351), module, Palette::AUX2_OUTPUT));
		addOutput(createOutputCentered<MyPort1>(Vec(71,351), module, Palette::OUT_OUTPUT));
		addOutput(createOutputCentered<MyPort1>(Vec(199.5,351), module, Palette::AUX_OUTPUT));

		addInput(createInputCentered<MyPort1>(Vec(135, 44), module, Palette::TRIGGER_INPUT));
		addInput(createInputCentered<MyPort1>(Vec(88, 44), module, Palette::NOTE_INPUT));

		addInput(createInputCentered<MyPort1>(Vec(252,255), module, Palette::FREQ_INPUT));
		addInput(createInputCentered<MyPort1>(Vec(183,44), module, Palette::LEVEL_INPUT));

		addInput(createInputCentered<MyPort1>(Vec(18,255), module, Palette::HARMONICS_INPUT));
		addParam(createParamCentered<PaletteKnobSmall>(Vec(18,233), module, Palette::HARMONICS_CV_PARAM));
		addInput(createInputCentered<MyPort1>(Vec(252,148), module, Palette::MORPH_INPUT));
		addParam(createParamCentered<PaletteKnobSmall>(Vec(252,126), module, Palette::MORPH_CV_PARAM));
		addInput(createInputCentered<MyPort1>(Vec(18,148), module, Palette::TIMBRE_INPUT));
		addParam(createParamCentered<PaletteKnobSmall>(Vec(18,126), module, Palette::TIMBRE_CV_PARAM));

		addParam(createParamCentered<PaletteKnobSmall>(Vec(252,233), module, Palette::FREQ_CV_PARAM));
		addParam(createParamCentered<PaletteKnobSmall>(Vec(71,300), module, Palette::LPG_COLOR_PARAM));
		addParam(createParamCentered<PaletteKnobSmall>(Vec(199.5,300), module, Palette::LPG_DECAY_PARAM));

		auto uvoicesknob = createParamCentered<PaletteKnobSmall>(Vec(40,44), module, Palette::UNISONOMODE_PARAM);
		addParam(uvoicesknob);
		addParam(createParamCentered<PaletteKnobSmall>(Vec(230,44), module, Palette::UNISONOSPREAD_PARAM));
		
		addParam(createParamCentered<PaletteKnobSmall>(Vec(18,206), module, Palette::HARMONICS_LPG_PARAM));
		addParam(createParamCentered<PaletteKnobSmall>(Vec(18,175), module, Palette::TIMBRE_LPG_PARAM));
		addParam(createParamCentered<PaletteKnobSmall>(Vec(252,175), module, Palette::MORPH_LPG_PARAM));

		addParam(createParamCentered<PaletteKnobSmall>(Vec(252,333), module, Palette::DECAY_CV_PARAM));
		addInput(createInputCentered<MyPort1>(Vec(252,355), module, Palette::LPG_DECAY_INPUT));
		addParam(createParamCentered<PaletteKnobSmall>(Vec(18,333), module, Palette::LPG_COLOR_CV_PARAM));
		addInput(createInputCentered<MyPort1>(Vec(18,355), module, Palette::LPG_COLOR_INPUT));

		addParam(createParamCentered<PaletteButton>(Vec(77.5, 98.5), module, Palette::MODEL1_PARAM));
		addParam(createParamCentered<PaletteButton>(Vec(192.5, 98.5), module, Palette::MODEL2_PARAM));

		addParam(createParamCentered<PaletteKnobSmall>(Vec(18,76), module, Palette::ENGINE_CV_PARAM));
		addInput(createInputCentered<MyPort1>(Vec(18,98), module, Palette::ENGINE_INPUT));

		addParam(createParamCentered<PaletteKnobSmall>(Vec(252,283), module, Palette::OUTMIX_CV_PARAM));
		addInput(createInputCentered<MyPort1>(Vec(252,305), module, Palette::OUTMIX_INPUT));

		addParam(createParamCentered<PaletteKnobSmall>(Vec(18,283), module, Palette::OUTMIX_LPG_PARAM));

		addParam(createParamCentered<PaletteKnobSmall>(Vec(252,206), module, Palette::FREQ_LPG_PARAM));

		addParam(createParamCentered<PaletteKnobSmall>(Vec(252,76), module, Palette::UNISONOSPREAD_CV_PARAM));
		addInput(createInputCentered<MyPort1>(Vec(252,98), module, Palette::SPREAD_INPUT));
		
		Model_LEDWidget* ledwid = new Model_LEDWidget(module);
		ledwid->box = {{0,0},{box.size}};
		addChild(ledwid);
	}

	void appendContextMenu(Menu *menu) override {
		Palette *module = dynamic_cast<Palette*>(this->module);
		
		struct LPGMenuItem : MenuItem
		{
			Palette* module = nullptr;
			int lpg_mode = 0;
			void onAction(const event::Action &e) override
			{
				module->lpg_mode = lpg_mode;
			}
		};
		struct LPGMenuItems : MenuItem
		{
			Palette* module = nullptr;
			Menu *createChildMenu() override 
			{
				Menu *submenu = new Menu();
				std::string menutexts[3] = {"Classic (Low pass and VCA)","Low pass","Bypassed"};
				for (int i=0;i<3;++i)
				{
					auto menuItem = createMenuItem<LPGMenuItem>(menutexts[i], CHECKMARK(module->lpg_mode==i));
					menuItem->module = module;
					menuItem->lpg_mode = i;
					submenu->addChild(menuItem);
				}
				
				
				return submenu;
			}
		};
		
		struct ResamplerQItem : MenuItem
		{
			Palette* module = nullptr;
			int rsq = 0;
			void onAction(const event::Action &e) override {
				module->setResamplingQuality(rsq);
			}
		};

		struct ResamplerQMenu : MenuItem
		{
			Palette* module = nullptr;
			Menu *createChildMenu() override 
			{
				Menu *submenu = new Menu();
				std::string modetexts[12] =
				{
					"Resampling disabled (Classic Low CPU mode)",
					"0 (Lowest quality)",
					"1",
					"2",
					"3",
					"4 (Default quality)",
					"5",
					"6",
					"7",
					"8",
					"9",
					"10 (Highest quality, most CPU use)"
				};
				for (int i=0;i<12;++i)
				{
					bool checked = (i-1) == module->getResamplingQuality();
					
					auto menuItem = createMenuItem<ResamplerQItem>(modetexts[i], CHECKMARK(checked));
					menuItem->module = module;
					menuItem->rsq = i-1;
					submenu->addChild(menuItem);
				}
				
				
				return submenu;
			}
		};


		struct WaveShaperAuxModeItem : MenuItem {
			Palette *module = nullptr;
			int newMode = 0;
			void onAction(const event::Action &e) override {
				module->params[Palette::WAVETABLE_AUX_MODE].setValue(newMode);
			}
		};
		
		struct WaveShaperSubMenu : MenuItem
		{
			Palette* module = nullptr;
			Menu *createChildMenu() override 
			{
				Menu *submenu = new Menu();
				std::string menutexts[8] = {
				"Classic (5 bit output)",
				"Sine subosc at -12.1 semitones and 10% gain XOR'ed with main output",
				"Sine subosc at -12.1 semitones and 50% gain XOR'ed with main output",
				"Sine subosc at -0.1 semitones and 10% gain XOR'ed with main output",
				"Sine subosc at -0.1 semitones and 50% gain XOR'ed with main output",
				"Sine subosc at +12.1 semitones and 10% gain XOR'ed with main output",
				"Sine subosc at +12.1 semitones and 50% gain XOR'ed with main output",
				"Random value XOR'ed with main output",
				};
				for (int i=0;i<8;++i)
				{
					auto menuItem = createMenuItem<WaveShaperAuxModeItem>(menutexts[i], 
						CHECKMARK((int)module->params[Palette::WAVETABLE_AUX_MODE].getValue()==i));
					menuItem->module = module;
					menuItem->newMode = i;
					submenu->addChild(menuItem);
				}
				
				
				return submenu;
			}
		};

		struct PlaitsShowModulationsItem : MenuItem {
			Palette *module;
			void onAction(const event::Action &e) override {
				module->showModulations ^= true;
			}
		};
		
		struct PlaitsFreeTuneItem : MenuItem {
			Palette *module;
			void onAction(const event::Action &e) override {
				module->freeTune ^= true;
			}
		};

		struct PlaitsLFOModeItem : MenuItem {
			Palette *module;
			void onAction(const event::Action &e) override {
				module->lfoMode ^= true;
			}
		};

		struct PlaitsModelItem : MenuItem {
			Palette *module;
			int model;
			void onAction(const event::Action &e) override {
				for (int i=0;i<MAX_PALETTE_VOICES;++i)
					module->patch[i].engine = model;
			}
		};

		menu->addChild(new MenuEntry);
		
		if (!isNear(APP->engine->getSampleRate(),48000.0f,1.0f))
		{
			ResamplerQMenu *rqMode = createMenuItem<ResamplerQMenu>("Resampling quality", RIGHT_ARROW);
			rqMode->module = module;
			menu->addChild(rqMode);
		}
		
		
		
		PlaitsFreeTuneItem *freeTuneItem = createMenuItem<PlaitsFreeTuneItem>("Octave knob free tune", CHECKMARK(module->freeTune));
		freeTuneItem->module = module;
		menu->addChild(freeTuneItem);

		PlaitsLFOModeItem *lfoModeItem 
			= createMenuItem<PlaitsLFOModeItem>("LFO Mode", CHECKMARK(module->lfoMode));
		lfoModeItem->module = module;
		menu->addChild(lfoModeItem);

		PlaitsShowModulationsItem *showModsItem 
			= createMenuItem<PlaitsShowModulationsItem>("Show modulation amounts on knobs", CHECKMARK(module->showModulations));
		showModsItem->module = module;
		menu->addChild(showModsItem);

		WaveShaperSubMenu *auxMode 
		= createMenuItem<WaveShaperSubMenu>("Aux output mode for Wave Table engine", 
			RIGHT_ARROW);
		auxMode->module = module;
		menu->addChild(auxMode);

		LPGMenuItems* lpg_items = createMenuItem<LPGMenuItems>("LPG mode",RIGHT_ARROW);
		lpg_items->module = module;
		menu->addChild(lpg_items);

		menu->addChild(new MenuEntry());
		menu->addChild(createMenuLabel("Models"));
		for (int i = 0; i < 16; i++) {
			PlaitsModelItem *modelItem = createMenuItem<PlaitsModelItem>(modelLabels[i], CHECKMARK(module->patch[0].engine == i));
			modelItem->module = module;
			modelItem->model = i;
			menu->addChild(modelItem);
		}
	}
	
	std::vector<std::shared_ptr<SVG>> subPanels;
	int curSubPanel = 0;
	SvgWidget* swgWidget = nullptr;
};
#ifdef PALETTE_ZEROMEM_TEST
template <class TModule, class TModuleWidget>
plugin::Model* createModelTest(const std::string& slug) {
	struct TModel : plugin::Model {
		engine::Module* createModule() override {
			unsigned char* membuf = (unsigned char*)malloc(sizeof(TModule));
			memset(membuf,0,sizeof(TModule));
			new(membuf) TModule;
			//engine::Module* m = new TModule;
			engine::Module* m = (engine::Module*)membuf;
			m->model = this;
			return m;
		}
		app::ModuleWidget* createModuleWidget() override {
			TModule* m = new TModule;
			m->engine::Module::model = this;
			app::ModuleWidget* mw = new TModuleWidget(m);
			mw->model = this;
			return mw;
		}
		app::ModuleWidget* createModuleWidgetNull() override {
			app::ModuleWidget* mw = new TModuleWidget(NULL);
			mw->model = this;
			return mw;
		}
	};

	plugin::Model* o = new TModel;
	o->slug = slug;
	return o;
}
#endif

Model *modelPalette = createModel<Palette, PaletteWidget>("AtelierPalette");
