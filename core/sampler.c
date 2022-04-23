#include "engine.h"

void
sampler_init(struct sampler *sampler, struct wav *wav)
{
	/* Playback */
	sampler->wav      = wav;
	sampler->state    = STOP;
	sampler->pb_start = 0;
	sampler->pb_end   = wav->extras.nb_samples;
	sampler->pb_head  = 0;
	/* Loop */
	sampler->loop_on  = 1;
	sampler->loop_start = 0;
	sampler->loop_end  = sampler->pb_end;

	sampler->trig_on = 1;
	sampler->vol = 1;
}

float step_sampler(struct sampler *sampler)
{
	int16_t *samples = (int16_t *) sampler->wav->audio_data;
	float vol = sampler->vol;
	int trig_on = sampler->trig_on;
	size_t offset = sampler->pb_head;
	int pb_fini = (sampler->pb_head >= sampler->wav->extras.nb_samples);
	float ret = 0;

	if (sampler->state == PLAY && trig_on == 1) {
		sampler->pb_head = sampler->pb_start;
		sampler->trig_on = 0;
	} else if (sampler->state == STOP && trig_on == 1) {
		sampler->state = PLAY;
		sampler->trig_on = 0;
	}

	if(pb_fini) {
		if (sampler->loop_on) {
			sampler->pb_head = sampler->loop_start;
		} else {
			sampler->state = STOP;
			sampler->pb_head = sampler->pb_start;
		}
	}

	switch(sampler->state){
	case STOP:
		ret = 0;
		break;
	case PLAY:
		offset = sampler->pb_head++;
		ret = vol * (float) (samples[offset] / (float) INT16_MAX);
		break;
	}
	return ret;
}
