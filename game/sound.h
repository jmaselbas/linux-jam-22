#pragma once

struct sound_entry {
	struct sampler *sampler;
	struct wav *wav;
	vec3 entity_pos;
};
void sys_sound_set_listener(struct system *sys, vec3 pos, vec3 dir, vec3 left);
void sys_sound_push(struct system *sys, struct sound_entry *entry);
void sys_sound_exec(struct system *sys, struct audio *audio);


