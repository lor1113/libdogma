/* libdogma
 * Copyright (C) 2012, 2013 Romain "Artefact2" Dalmaso <artefact2@gmail.com>
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include <assert.h>
#include <stdbool.h>
#include "dogma.h"
#include "dogma_internal.h"
#include "tables.h"
#include "eval.h"
#include "attribute.h"

#define DOGMA_MIN_SKILL_LEVEL 0
#define DOGMA_MAX_SKILL_LEVEL 5

#define CAT_Skill 16

int dogma_init(void) {
	static bool initialized = false;
	if(initialized) return DOGMA_OK;
	initialized = true;

	dogma_init_tables();

	assert(DOGMA_MIN_SKILL_LEVEL <= DOGMA_MAX_SKILL_LEVEL);

	return DOGMA_OK;
}

int dogma_init_context(dogma_context_t** ctx) {
	dogma_context_t* new_ctx = malloc(sizeof(dogma_context_t));
	dogma_env_t** value;
	key_t index = 0;
	const dogma_type_t** type;

	new_ctx->character = malloc(sizeof(dogma_env_t));
	new_ctx->ship = malloc(sizeof(dogma_env_t));
	new_ctx->target = malloc(sizeof(dogma_env_t));
	new_ctx->area = malloc(sizeof(dogma_env_t));

	DOGMA_INIT_ENV(new_ctx->character, 0, NULL, 0, new_ctx->character);
	DOGMA_INIT_ENV(new_ctx->target, 0, NULL, 0, NULL);
	DOGMA_INIT_ENV(new_ctx->area, 0, NULL, 0, NULL);

	JLI(value, new_ctx->character->children, 0);
	*value = new_ctx->ship;

	DOGMA_INIT_ENV(new_ctx->ship, 0, new_ctx->character, 0, new_ctx->character);

	new_ctx->default_skill_level = DOGMA_MAX_SKILL_LEVEL;
	new_ctx->skill_levels = (array_t)NULL;

	new_ctx->drone_map = (array_t)NULL;

	*ctx = new_ctx;

	/* Inject all skills. This is somewhat costly, maybe there is a
	 * way to do it lazily? */
	JLF(type, types_by_id, index);
	while(type != NULL) {
		if((*type)->categoryid == CAT_Skill) {
			dogma_inject_skill(*ctx, (*type)->id);
		}
		JLN(type, types_by_id, index);
	}

	return DOGMA_OK;
}

int dogma_free_context(dogma_context_t* ctx) {
	dogma_drone_context_t** value;
	key_t index = 0;
	int ret;

	dogma_free_env(ctx->character);
	dogma_free_env(ctx->target);
	dogma_free_env(ctx->area);
	dogma_reset_skill_levels(ctx);

	JLF(value, ctx->drone_map, index);
	while(value != NULL) {
		/* The drone environments were freed when char was freed */
		free(*value);
		JLN(value, ctx->drone_map, index);
	}
	JLFA(ret, ctx->drone_map);


	free(ctx);
	return DOGMA_OK;
}

int dogma_set_default_skill_level(dogma_context_t* ctx, uint8_t default_level) {
	if(default_level < DOGMA_MIN_SKILL_LEVEL) default_level = DOGMA_MIN_SKILL_LEVEL;
	if(default_level > DOGMA_MAX_SKILL_LEVEL) default_level = DOGMA_MAX_SKILL_LEVEL;

	ctx->default_skill_level = default_level;

	return DOGMA_OK;
}

int dogma_set_skill_level(dogma_context_t* ctx, typeid_t skillid, uint8_t level) {
	uint8_t* value;

	if(level < DOGMA_MIN_SKILL_LEVEL) level = DOGMA_MIN_SKILL_LEVEL;
	if(level > DOGMA_MAX_SKILL_LEVEL) level = DOGMA_MAX_SKILL_LEVEL;

	JLI(value, ctx->skill_levels, skillid);
	*value = level;

	return DOGMA_OK;
}

int dogma_reset_skill_levels(dogma_context_t* ctx) {
	int ret;
	JLFA(ret, ctx->skill_levels);

	return DOGMA_OK;
}

int dogma_set_ship(dogma_context_t* ctx, typeid_t ship_typeid) {
	if(ship_typeid == ctx->ship->id) {
		/* Be lazy */
		return DOGMA_OK;
	}

	DOGMA_ASSUME_OK(dogma_set_env_state(ctx, ctx->ship, NULL, 0));
	ctx->ship->id = ship_typeid;
	DOGMA_ASSUME_OK(dogma_set_env_state(ctx, ctx->ship, NULL, DOGMA_Online));

	return DOGMA_OK;
}

int dogma_add_module(dogma_context_t* ctx, typeid_t module_typeid, key_t* out_index) {
	dogma_env_t* module_env = malloc(sizeof(dogma_env_t));
	dogma_env_t** value;
	key_t index = DOGMA_SAFE_CHAR_INDEXES;
	int result;

	JLFE(result, ctx->ship->children, index);
	JLI(value, ctx->ship->children, index);
	*value = module_env;
	*out_index = index;

	DOGMA_INIT_ENV(module_env, module_typeid, ctx->ship, index, ctx->character);

	return DOGMA_OK;
}

int dogma_remove_module(dogma_context_t* ctx, key_t index) {
	dogma_env_t** module_env;
	int result;

	JLG(module_env, ctx->ship->children, index);
	if(module_env == NULL) return DOGMA_NOT_FOUND;

	DOGMA_ASSUME_OK(dogma_set_env_state(ctx, *module_env, NULL, 0));

	dogma_free_env(*module_env);

	JLD(result, ctx->ship->children, index);
	return DOGMA_OK;
}

int dogma_set_module_state(dogma_context_t* ctx, key_t index, state_t new_state) {
	dogma_env_t** module_env;

	JLG(module_env, ctx->ship->children, index);
	if(module_env == NULL) return DOGMA_NOT_FOUND;

	return dogma_set_env_state(ctx, *module_env, NULL, new_state);
}

int dogma_add_charge(dogma_context_t* ctx, key_t index, typeid_t chargeid) {
	dogma_env_t* charge_env = malloc(sizeof(dogma_env_t));
	dogma_env_t** module_env;
	dogma_env_t** value;
	int result;
	key_t charge_index = 0;

	JLG(module_env, ctx->ship->children, index);
	if(module_env == NULL) return DOGMA_NOT_FOUND;

	/* Maybe remove previous charge */
	dogma_remove_charge(ctx, index);

	JLFE(result, (*module_env)->children, charge_index);
	JLI(value, (*module_env)->children, charge_index);
	*value = charge_env;

	DOGMA_INIT_ENV(charge_env, chargeid, *module_env, charge_index, ctx->character);

	return dogma_set_env_state(ctx, charge_env, *module_env, DOGMA_Active);
}

int dogma_remove_charge(dogma_context_t* ctx, key_t index) {
	dogma_env_t** module_env;
	dogma_env_t** charge_env;
	int count;
	key_t charge_index = 0;

	JLG(module_env, ctx->ship->children, index);
	if(module_env == NULL) return DOGMA_NOT_FOUND;

	if((*module_env)->children == NULL) return DOGMA_OK; /* No charge */
	JLC(count, (*module_env)->children, 0, -1);
	assert(count == 1); /* If there's more than one charge in this
						 * module, something is wrong */

	JLF(charge_env, (*module_env)->children, charge_index);
	assert(charge_env != NULL);

	DOGMA_ASSUME_OK(dogma_set_env_state(ctx, *charge_env, *module_env, 0));

	dogma_free_env(*charge_env);
	JLFA(count, (*module_env)->children);

	return DOGMA_OK;
}

int dogma_add_drone(dogma_context_t* ctx, typeid_t droneid, unsigned int quantity) {
	dogma_env_t** value1;
	dogma_drone_context_t** value2;
	dogma_drone_context_t* drone_ctx;
	dogma_env_t* drone_env;
	key_t index = DOGMA_SAFE_CHAR_INDEXES;
	int ret;

	if(quantity == 0) return DOGMA_OK;

	JLG(value2, ctx->drone_map, droneid);
	if(value2 != NULL) {
		/* Already have drones of the same type, just add the quantity */
		drone_ctx = *value2;
		drone_ctx->quantity += quantity;
		return DOGMA_OK;
	}

	drone_ctx = malloc(sizeof(dogma_drone_context_t));
	drone_env = malloc(sizeof(dogma_env_t)); /* Two calls to malloc
											  * are necessary here,
											  * since the env will be
											  * freed in
											  * dogma_free_env(). */
	JLFE(ret, ctx->character->children, index);
	JLI(value1, ctx->character->children, index);
	*value1 = drone_env;

	DOGMA_INIT_ENV(drone_env, droneid, ctx->character, index, ctx->character);

	JLI(value2, ctx->drone_map, droneid);
	*value2 = drone_ctx;

	drone_ctx->drone = drone_env;
	drone_ctx->quantity = quantity;

	return dogma_set_env_state(ctx, drone_env, NULL, DOGMA_Active);
}

int dogma_remove_drone_partial(dogma_context_t* ctx, typeid_t droneid, unsigned int quantity) {
	dogma_drone_context_t** value;

	JLG(value, ctx->drone_map, droneid);
	if(value == NULL) return DOGMA_OK;

	if(quantity >= (*value)->quantity) {
		return dogma_remove_drone(ctx, droneid);
	} else {
		/* At least one drone will remain */
		(*value)->quantity -= quantity;
		return DOGMA_OK;
	}
}

int dogma_remove_drone(dogma_context_t* ctx, typeid_t droneid) {
	dogma_drone_context_t** value;
	dogma_env_t* drone_env;
	int ret;

	JLG(value, ctx->drone_map, droneid);
	if(value == NULL) return DOGMA_OK; /* Nonexistent drone */

	drone_env = (*value)->drone;
	DOGMA_ASSUME_OK(dogma_set_env_state(ctx, drone_env, NULL, 0));
	JLD(ret, drone_env->parent->children, drone_env->index);
	JLD(ret, ctx->drone_map, drone_env->id);

	dogma_free_env(drone_env);
	free(*value);

	return DOGMA_OK;
}

static inline int dogma_get_location_env(dogma_context_t* ctx, location_t location, dogma_env_t** env) {
	dogma_env_t** env1;
	dogma_env_t** env2;
	dogma_drone_context_t** drone_env1;
	key_t index = 0;

	switch(location.type) {

	case DOGMA_LOC_Char:
		*env = ctx->character;
		return DOGMA_OK;

	case DOGMA_LOC_Ship:
		*env = ctx->ship;
		return DOGMA_OK;

	case DOGMA_LOC_Module:
		JLG(env1, ctx->ship->children, location.module_index);
		if(env1 == NULL) return DOGMA_NOT_FOUND;
		*env = *env1;
		return DOGMA_OK;

	case DOGMA_LOC_Charge:
		JLG(env1, ctx->ship->children, location.module_index);
		if(env1 == NULL) return DOGMA_NOT_FOUND;
		JLF(env2, (*env1)->children, index);
		if(env2 == NULL) return DOGMA_NOT_FOUND;
		*env = *env1;
		return DOGMA_OK;

	case DOGMA_LOC_Drone:
		JLG(drone_env1, ctx->drone_map, location.drone_typeid);
		if(drone_env1 == NULL) return DOGMA_NOT_FOUND;
		*env = (*drone_env1)->drone;
		return DOGMA_OK;

	default:
		return DOGMA_NOT_FOUND;

	}
}

int dogma_get_location_attribute(dogma_context_t* ctx, location_t location,
								 attributeid_t attributeid, double* out) {
	dogma_env_t* loc_env;
	DOGMA_ASSUME_OK(dogma_get_location_env(ctx, location, &loc_env));
	return dogma_get_env_attribute(ctx, loc_env, attributeid, out);
}

int dogma_get_character_attribute(dogma_context_t* ctx, attributeid_t attributeid, double* out) {
	return dogma_get_env_attribute(ctx, ctx->character, attributeid, out);
}

int dogma_get_ship_attribute(dogma_context_t* ctx, attributeid_t attributeid, double* out) {
	return dogma_get_env_attribute(ctx, ctx->ship, attributeid, out);
}

int dogma_get_module_attribute(dogma_context_t* ctx, key_t index, attributeid_t attributeid, double* out) {
	return dogma_get_location_attribute(
		ctx,
		(location_t){ .type = DOGMA_LOC_Module, .module_index = index },
		attributeid,
		out
	);
}

int dogma_get_charge_attribute(dogma_context_t* ctx, key_t index, attributeid_t attributeid, double* out) {
	return dogma_get_location_attribute(
		ctx,
		(location_t){ .type = DOGMA_LOC_Charge, .module_index = index },
		attributeid,
		out
	);
}

int dogma_get_drone_attribute(dogma_context_t* ctx, typeid_t droneid, attributeid_t attributeid, double* out) {
	return dogma_get_location_attribute(
		ctx,
		(location_t){ .type = DOGMA_LOC_Drone, .drone_typeid = droneid },
		attributeid,
		out
	);
}
