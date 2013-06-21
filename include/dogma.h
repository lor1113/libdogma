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

#pragma once
#ifndef _DOGMA_H
#define _DOGMA_H 1

#include <stdint.h>
#include <Judy.h>

/* -------- Disclaimer -------- */

/* Most of the functions in libdogma that operate with types do no
 * checking at all. For example, dogma_set_ship() will not complain if
 * you pass a typeid that is not a ship. If you really want, you could
 * fit a titan skillbook to a piece of tritanium, and then use a
 * planet as a charge, then launch some corpses as drones. If you do,
 * things may blow up in your face!
 *
 * libdogma does NOT check the category of typeids it is given (is it
 * a ship? a module?), nor does it check any overflow (is there enough
 * CPU? powergrid? is there a launcher hardpoint available? etc.). If
 * you care about overflows and such, you will have to do it yourself.
 *
 * To keep the API simple, there are no methods to query what is
 * fitted to a context. You are expected to keep track of what you add
 * and what you remove on your own.
 *
 * A final note, libdogma is licensed under the GNU General Public
 * License version 3 (or later). You are free to use it for your
 * program, but the GPLv3 has a strong copyleft clause; make sure you
 * understand what it means for you and your program!
 */





/* -------- Return codes -------- */

/* Used to indicate success. */
#define DOGMA_OK 0

/* Used when the requested object (type, expression, etc.) does not
 * exist. */
#define DOGMA_NOT_FOUND 1





/* -------- Data types -------- */

typedef Pvoid_t array_t;
typedef Word_t key_t;
typedef uint32_t typeid_t;
typedef uint16_t attributeid_t;



enum location_type_e {
	DOGMA_LOC_Char,
	DOGMA_LOC_Implant,
	DOGMA_LOC_Ship,
	DOGMA_LOC_Module,
	DOGMA_LOC_Charge,
	DOGMA_LOC_Drone,
};
typedef enum location_type_e location_type_t;

struct location_s {
	location_type_t type;

	union {
		key_t implant_index; /* The index generated by
		                      * dogma_add_implant(). */
		key_t module_index; /* The index generated by
		                     * dogma_add_module(). */
		typeid_t drone_typeid; /* The typeid of the drone */
	};
};
typedef struct location_s location_t;



enum state_s {
	/* These values are actually bitmasks: if bit of index i is set,
	 * it means effects with category i should be evaluated. */
	DOGMA_Offline = 1,     /* 0b00000001 */
	DOGMA_Online = 17,     /* 0b00010001 */
	DOGMA_Active = 31,     /* 0b00011111 */
	DOGMA_Overloaded = 63, /* 0b00111111 */
};
typedef enum state_s state_t;



/* Opaque structure, to help maintain ABI compatibility */
struct dogma_context_s;
typedef struct dogma_context_s dogma_context_t;





/* -------- General functions -------- */

/* Initialize the static state needed by the library. Must only be
 * called exactly once before any other dogma_* function. */
int dogma_init(void);





/* -------- Context manipulation functions -------- */

/* Create a dogma context. A dogma context consists of a character and
 * his skills, a ship, its fitted modules, etc. By default the
 * character has all skills to level V. */
int dogma_init_context(dogma_context_t**);

/* Free a dogma context previously created by dogma_init_context(). */
int dogma_free_context(dogma_context_t*);



/* Add an implant or a booster to this context. */
int dogma_add_implant(dogma_context_t*, typeid_t, key_t*);

/* Remove an implant (or a booster). Use the index generated by
 * dogma_add_implant(). */
int dogma_remove_implant(dogma_context_t*, key_t);



/* Set the default skill level of a context. */
int dogma_set_default_skill_level(dogma_context_t*, uint8_t);

/* Override the skill level of a particular skill. */
int dogma_set_skill_level(dogma_context_t*, typeid_t, uint8_t);

/* Forgets all overriden skill levels. This does not reset the default
 * skill level that was set with dogma_set_default_skill_level(). */
int dogma_reset_skill_levels(dogma_context_t*);



/* Set the ship of a dogma context. Use ID 0 to remove the current
 * ship (but not its fitted modules or anything else). */
int dogma_set_ship(dogma_context_t*, typeid_t);



/* Add a module to a ship. By default the module will not even be
 * offline, it will have state 0 (which means that none of its effects
 * are evaluated). */
int dogma_add_module(dogma_context_t*, typeid_t, key_t*);

/* Remove a module. Use the index generated by dogma_add_module(). */
int dogma_remove_module(dogma_context_t*, key_t);

/* Set the state of a module. Use the index generated by
 * dogma_add_module(). */
int dogma_set_module_state(dogma_context_t*, key_t, state_t);


/* Add a charge to a module. Use the index generated by
 * dogma_add_module(). Added charges automatically have the online
 * state and there is no way to change it. If there is already a
 * charge in the specified module, it will be overwritten. */
int dogma_add_charge(dogma_context_t*, key_t, typeid_t);

/* Remove a charge. Use the index generated by dogma_add_module(). No
 * effect if there is no charge in the module. */
int dogma_remove_charge(dogma_context_t*, key_t);



/* Add a given number of drones to a loadout context. The drones are
 * assumed to be "launched" (ie not in the drone bay doing
 * nothing). */
int dogma_add_drone(dogma_context_t*, typeid_t, unsigned int);

/* Remove a given number of drones from a loadout context. */
int dogma_remove_drone_partial(dogma_context_t*, typeid_t, unsigned int);

/* Remove all drones (of a certain type) from the loadout, regardless
 * of how many were added. If trying to remove a nonexistent drone,
 * nothing is done and no error is returned. */
int dogma_remove_drone(dogma_context_t*, typeid_t);





/* -------- Attribute getters -------- */

/* Get an attribute of something. All the fuctions below are just
 * shorthands of this version. */
int dogma_get_location_attribute(dogma_context_t*, location_t, attributeid_t, double*);

/* Get an attribute of the character. */
int dogma_get_character_attribute(dogma_context_t*, attributeid_t, double*);

/* Get an attribute of an implant (or booster). Use the index
 * generated by dogma_add_implant(). */
int dogma_get_implant_attribute(dogma_context_t*, key_t, attributeid_t, double*);

/* Get an attribute of the ship. */
int dogma_get_ship_attribute(dogma_context_t*, attributeid_t, double*);

/* Get an attribute of a module. Use the index generated by
 * dogma_add_module(). */
int dogma_get_module_attribute(dogma_context_t*, key_t, attributeid_t, double*);

/* Get an attribute of a charge. Use the index generated by
 * dogma_add_module(). */
int dogma_get_charge_attribute(dogma_context_t*, key_t, attributeid_t, double*);

/* Get an attribute of a drone. */
int dogma_get_drone_attribute(dogma_context_t*, typeid_t, attributeid_t, double*);





#endif
