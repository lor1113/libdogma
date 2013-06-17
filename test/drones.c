/* libdogma
 * Copyright (C) 2013 Romain "Artefact2" Dalmaso <artefact2@gmail.com>
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

#include "test.h"

#define TYPE_Dominix 645
#define TYPE_Scorpion 640
#define TYPE_GardeII 28211
#define TYPE_HornetEC300 23707
#define TYPE_LargeSDA 25920
#define TYPE_DroneDamageAmplifierII 4405
#define TYPE_OmnidirectionalTrackingLinkII 24438
#define TYPE_SignalDistortionAmplifierII 25563
#define TYPE_ECMPhaseInverterII 2559

#define ATT_MaxRange 54
#define ATT_DamageMultiplier 64
#define ATT_TrackingSpeed 160
#define ATT_ScanLadarStrengthBonus 239
#define ATT_ArmorHP 265

int main(void) {
	dogma_context_t* ctx;
	key_t slot0, slot1, slot2;
	double v;

	dogma_init();
	dogma_init_context(&ctx);

	/* Source: Pyfa-42efa48 (Jun 13 2013) */

	dogma_set_ship(ctx, TYPE_Dominix);
	assert(dogma_get_drone_attribute(ctx, TYPE_GardeII, ATT_ArmorHP, &v) == DOGMA_NOT_FOUND);

	dogma_add_drone(ctx, TYPE_GardeII, 2);
	assert(dogma_get_drone_attribute(ctx, TYPE_GardeII, ATT_ArmorHP, &v) == DOGMA_OK);
	assertf(1800.0, v, 0.05);

	dogma_remove_drone_partial(ctx, TYPE_GardeII, 1);
	assert(dogma_get_drone_attribute(ctx, TYPE_GardeII, ATT_ArmorHP, &v) == DOGMA_OK);
	assertf(1800.0, v, 0.05);

	dogma_remove_drone_partial(ctx, TYPE_GardeII, 1);
	assert(dogma_get_drone_attribute(ctx, TYPE_GardeII, ATT_ArmorHP, &v) == DOGMA_NOT_FOUND);

	dogma_add_drone(ctx, TYPE_GardeII, 5);
	dogma_remove_drone_partial(ctx, TYPE_GardeII, 10);
	assert(dogma_get_drone_attribute(ctx, TYPE_GardeII, ATT_ArmorHP, &v) == DOGMA_NOT_FOUND);

	dogma_add_drone(ctx, TYPE_GardeII, 10);
	dogma_remove_drone(ctx, TYPE_GardeII);
	assert(dogma_get_drone_attribute(ctx, TYPE_GardeII, ATT_ArmorHP, &v) == DOGMA_NOT_FOUND);

	dogma_add_drone(ctx, TYPE_GardeII, 1);
	dogma_get_drone_attribute(ctx, TYPE_GardeII, ATT_MaxRange, &v);
	assertf(45000.0, v, 0.05);
	dogma_get_drone_attribute(ctx, TYPE_GardeII, ATT_TrackingSpeed, &v);
	assertf(0.054, v, 0.0005);

	dogma_add_module(ctx, TYPE_LargeSDA, &slot0);
	dogma_add_module(ctx, TYPE_DroneDamageAmplifierII, &slot1);
	dogma_add_module(ctx, TYPE_OmnidirectionalTrackingLinkII, &slot2);
	dogma_set_module_state(ctx, slot0, DOGMA_Online);
	dogma_set_module_state(ctx, slot1, DOGMA_Online);
	dogma_set_module_state(ctx, slot2, DOGMA_Online);
	dogma_get_drone_attribute(ctx, TYPE_GardeII, ATT_MaxRange, &v);
	assertf(56250.0, v, 0.05);
	dogma_get_drone_attribute(ctx, TYPE_GardeII, ATT_TrackingSpeed, &v);
	assertf(0.0675, v, 0.00005);
	dogma_get_drone_attribute(ctx, TYPE_GardeII, ATT_DamageMultiplier, &v);
	assertf(9.625692655, v, 0.0000000005);

	dogma_free_context(ctx);

	dogma_init_context(&ctx);
	dogma_set_ship(ctx, TYPE_Scorpion);
	dogma_add_drone(ctx, TYPE_HornetEC300, 1);
	dogma_add_module(ctx, TYPE_SignalDistortionAmplifierII, &slot0);
	dogma_add_module(ctx, TYPE_ECMPhaseInverterII, &slot1);
	dogma_set_module_state(ctx, slot0, DOGMA_Online);
	dogma_set_module_state(ctx, slot1, DOGMA_Active);

	/* Ship-related ECM bonuses should not apply to the drone */
	dogma_get_drone_attribute(ctx, TYPE_HornetEC300, ATT_ScanLadarStrengthBonus, &v);
	assertf(1.0, v, 0.05);

	/* …But they should apply to the ECM module */
	dogma_get_module_attribute(ctx, slot1, ATT_ScanLadarStrengthBonus, &v);
	assertf(8.6625, v, 0.00005);

	dogma_free_context(ctx);
	return 0;
}
