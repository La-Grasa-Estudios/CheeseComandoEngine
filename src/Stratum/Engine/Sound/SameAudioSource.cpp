#include "SameAudioSource.h"

#include "VFS/ZVFS.h"

#include "MP3AudioSource.h"
#include "WaveAudioSource.h"
#include "Core/Logger.h"

using namespace ENGINE_NAMESPACE;

struct audio_context
{
	double_t time = 0;
	double_t delta = 0;
	uint32_t hz = 44100;
	uint32_t freq_pos = 0;
	float afsk_phase = 0.0f;
	RawAudioBuffer* buffer;
	bool reset = false;
};

static void audio_buffer_write(RawAudioBuffer* buffer, float sample)
{
	buffer->Queue(&sample, 1);
}

static constexpr double_t to_seconds(double_t milliseconds)
{
	return milliseconds / 1000.0;
}

static constexpr double_t to_millis_to_seconds(double_t bauds)
{
	return (1000.0 / bauds) / 1000.0; // Bits per second to seconds
}

static void audio_context_init(audio_context* context)
{
	context->delta = 1.0 / context->hz;
}

static double base_frequency(audio_context* context, float hz)
{
	return sin(context->time * 3.14159 * hz * 2);
}

static double base_frequency(audio_context* context, float* phase, float hz)
{
	*phase += (3.14159 * hz * 2) / context->hz;
	if (*phase > 3.14159 * 2)
		*phase -= 3.14159 * 2;
	return sin(*phase);
}

// Returns true when the header as finished encoding
static bool same_encode(const char* data, bool marks, audio_context* context)
{
	static double_t baud_timer = 0;
	static double_t header_timer = 0;
	static uint8_t bit_position = 0;
	static uint8_t char_position = 0;
	static bool bit_state = false;
	static const char* text = NULL;
	constexpr double_t baud = to_millis_to_seconds(520.83); // ~1.92ms per bit
	static bool write_header = true;
	double_t headerDuration = 1;
	bool finished = false;
	size_t len = strnlen(data, 100) + 16;
	if (!marks)
		len -= 16;
	static char buffer[256]{};
	if (text != data)
	{
		// Simulate SAGE encoders with 16 0xAB marks
		memcpy(buffer, "лллллллллллллллл", 16);
		if (marks)
		{
			memcpy(buffer + 16, data, len - 16);
		}
		else
		{
			// In case we don't have marks just overwrite them
			memcpy(buffer, data, len);
		}
		text = data;
		write_header = true;
		bit_state = false;
		char_position = 0;
		bit_position = 0;
		header_timer = 0;
		baud_timer = 0;
	}
	if (write_header)
	{
		// Wait for the timer to expire so we read a single bit from the data buffer[char_position]
		baud_timer -= context->delta;
		if (baud_timer <= 0.0)
		{
			baud_timer = baud;
			uint8_t c = buffer[char_position];
			bool bit = (c >> bit_position) & 0x01; // Mask it
			bit_position++;
			bit_state = bit;
			// Advance to next character
			if (bit_position > 7)
			{
				bit_position = 0;
				char_position++;
			}
			// We have finished the data burst return true and start a 1 second timer for the next burst
			if (char_position >= len)
			{
				write_header = false;
				header_timer = headerDuration;
				char_position = 0;
				bit_position = 0;
				finished = true;
			}
		}

		// Pulled from the SAME Header specification
		const double markFreq = 2083.3f;
		const double spaceFreq = 1562.5f;

		float freq = bit_state ? markFreq : spaceFreq;

		// The mark (logic 1) and space (logic 0) frequencies get selected based on bit state
		// But we need to generate a continous sine wave by just changing the phase
		// That's why we provide our own phase accumulator for the base_frequency function
		audio_buffer_write(context->buffer, base_frequency(context, &context->afsk_phase, freq));
	}
	else
	{
		header_timer -= context->delta;
		if (header_timer <= 0.0)
		{
			write_header = true;
		}
		audio_buffer_write(context->buffer, 0.0f);
	}
	return finished;
}

// Return true while there is still something to encode
static bool audio_context_update(audio_context* context)
{
	/*
	"An EAS Participant has issued a Required Weekly Test for the following counties/areas:
	Hillsborough FL, Manatee FL, Pasco FL, Pinellas FL, and Sarasota FL
	at 12:15 am EDT on October 5 effective until 12:45 am EDT. Message from WTSP-TV."
	*/
	const char* header = "ZCZC-EAS-RWT-012057-012081-012101-012103-012115+0030-2780415-WTSP/TV-";
	const char* eom = "NNNNNNNNNNNNNNNN";
	static double_t attention_timer = 1; // 1 second silent
	static double_t eom_timer = 1; // 1 second silent

	static uint8_t header_count = 0;
	static uint8_t eom_count = 0;
	static uint8_t stage = 0;

	if (context->reset)
	{
		context->reset = false;
		attention_timer = 1;
		eom_timer = 1;
		header_count = 0;
		eom_count = 0;
		stage = 0;
	}

	// Iconic start of an EAS message
	if (stage == 0)
	{
		if (same_encode(header, true, context))
		{
			header_count++;
		}
		if (header_count >= 3)
		{
			header_count = 0;
			stage++;
		}
	}
	// EOM
	else if (stage == 2)
	{
		eom_timer -= context->delta;
		if (eom_timer < 0.0)
		{
			if (same_encode(eom, false, context))
				eom_count++;
		}
		else
		{
			audio_buffer_write(context->buffer, 0.0f);
		}
		if (eom_count >= 3)
		{
			eom_count = 0;
			eom_timer = 1;
			stage++;
		}
	}
	// EBS attention tone (960hz + 853hz sine wave)
	else if (stage == 1)
	{
		attention_timer -= context->delta;
		if (attention_timer < 0.0 && attention_timer > -8.0f)
		{
			float f1 = base_frequency(context, 960);
			float f2 = base_frequency(context, 853);
			float f = (f1 + f2) * 0.5f;
			audio_buffer_write(context->buffer, f);
		}
		else
		{
			audio_buffer_write(context->buffer, 0.0f);
		}
		if (attention_timer <= -8.0f)
		{
			attention_timer = 1;
			stage++;
		}
	}
	/*
	float f = context->buffer.data[context->buffer._data_index - 1];
	f *= 0.5f;
	static float noise = 0.0f;
	if (context->buffer._data_index % 4 == 0)
		noise = (rand() / (float)RAND_MAX) * 0.04f - 0.02f;
	f += noise * 2.0f;
	f *= (rand() / (float)RAND_MAX) * 0.1f + 0.9f;
	context->buffer.data[context->buffer._data_index - 1] = f;
	*/
	return stage != 3;
}

static bool audio_context_advance(audio_context* context)
{
	bool written = false;
	while (written = audio_context_update(context))
	{
		if (!context->buffer->CanWrite(1))
		{
			break;
		}
		context->time = context->freq_pos / (float)context->hz;
		context->freq_pos += 1;
	}
	return written;
}

SameAudioSource::SameAudioSource(ma_engine* pEngine)
{
	memset(audio_ctx, 0, sizeof(audio_context));

	m_AudioBuffer = new RawAudioBuffer(44100, 1, ma_format_f32, 44100 / 4);
    ma_sound_init_from_data_source(pEngine, m_AudioBuffer->GetDataSource(), 0, NULL, &p_Sound);
    ma_node_attach_output_bus(&p_Sound, 0, pEngine, 0);
    p_Params.IsReady = true;
	this->pEngine = pEngine;

	audio_context* ctx = (audio_context*)audio_ctx;
	ctx->buffer = m_AudioBuffer;
	ctx->hz = 44100;
	audio_context_init(ctx);
}

SameAudioSource::~SameAudioSource()
{
    if (m_AudioBuffer)
    {
        delete m_AudioBuffer;
    }
}

void SameAudioSource::Rewind()
{
	p_Params.DoRewind = true;
	audio_context* ctx = (audio_context*)audio_ctx;
	ctx->reset = true;
}

void SameAudioSource::UpdateSource()
{
	audio_context* ctx = (audio_context*)audio_ctx;

	p_Params.IsPlaying = audio_context_advance(ctx);

	if (!p_Params.IsPlaying && p_Params.IsLooping)
	{
		ctx->reset = true;
		p_Params.IsPlaying = true;
	}

    if (!ma_sound_is_playing(&p_Sound) || ma_sound_at_end(&p_Sound))
    {
        ma_sound_start(&p_Sound);
    }
}

void SameAudioSource::Seek(uint32_t samplePosition)
{
	audio_context* ctx = (audio_context*)audio_ctx;
	m_Position = samplePosition;
	ctx->freq_pos = 0;
}

uint32_t SameAudioSource::Position()
{
    return m_Position;
}

float SameAudioSource::PositionF()
{
    return m_Position / 44100.0f;
}
