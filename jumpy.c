#include <gb/gb.h>
#include <stdio.h>
#include "eggy.c"
#include "floortiles.c"
#include "floormap.c"
#include "floormap_full_segments.c"

BYTE isjumping = 0;
BYTE reached_end = 0;
UINT8 gravity = 1;
INT8 jumpmomentum = 0;
INT8 max_falling_speed = -15;
UINT8 jumpy_x = 25;
UINT8 jumpy_y = 120;
UINT8 new_jumpy_y = 0;
UINT8 jumpstrength = 10;
UINT8 idle_loop_frame = 0;
UINT8 floor_y = 120;
UINT8 bkg_position_offset = 0;
UINT16 bkg_columns_scrolled = 32;
UINT16 bkg_colscroll_counter = 0;
UINT16 stage_width = 0;
UINT16 next_vram_location = 0;
const char groundtile[1] = {0x01};

void jump() {
	isjumping = 1;
	// Play jump sound
	NR11_REG = 0x1F;
	NR12_REG = 0xF1;     
	NR13_REG = 0x30;
	NR14_REG = 0xC0;

	set_sprite_tile(0, 0);
	jumpmomentum = jumpstrength;
}

void performantdelay(UINT8 delay) {
	UINT8 i;
	for (i = 0; i < delay; i++)
	{
		wait_vbl_done();
	}
	
}

UBYTE hashitground(UINT8 new_x, UINT8 new_y, UINT8 y_offset) {
    UINT16 indexTLx, indexTLy, tileindexTL;
	
	// convert x and y of jumpy into the index of a background tile it'll overlap.
	indexTLx = (new_x - 4 + bkg_position_offset) / 8;
	indexTLy = ((new_y - 8) / 8) - y_offset; // If we are dropping faster than 8 tiles per frame, needs the offset
	tileindexTL = floormapWidth * indexTLy + indexTLx;


	// Check to see if it's a tile we'd land on
	if (groundtile[0] == floormap[tileindexTL]) {
		// If yes then snap to the top of that tile
		new_jumpy_y = (indexTLy * 8) + 8;
		//printf("%u", (UINT16)(tileindexTL));
		return 1;
	}
	
	return 0;
}

void animatejump() {
	new_jumpy_y = jumpy_y - jumpmomentum;
	if (jumpmomentum > max_falling_speed) {
		jumpmomentum = jumpmomentum - gravity;
	}
	// Check if we're moving down and if we hit a "ground" tile
	if (jumpmomentum < 0 && hashitground(jumpy_x, new_jumpy_y, 0)) {
		isjumping = 0;
	} else if (jumpmomentum < -7 && hashitground(jumpy_x, new_jumpy_y, 1)) {
		// If we're jumping more than 8 tiles, check the intermediate row
		isjumping = 0;
	} else {
		// Affect sprite based on height
		if (jumpmomentum >= 2) {
			set_sprite_tile(0, 2);
		} else if (jumpmomentum >= -2) {
			set_sprite_tile(0, 1);
		} else {
			set_sprite_tile(0, 0);
		}
	}
	jumpy_y = new_jumpy_y;
	move_sprite(0, jumpy_x, jumpy_y);
}

void animateidle(){
	idle_loop_frame = idle_loop_frame + 1;
	if (idle_loop_frame == 4) {
		idle_loop_frame = 0;
	}
	set_sprite_tile(0, idle_loop_frame);
}


void main() {
	// Enable sounds
	NR52_REG = 0x80; // Turns on sound
	NR50_REG = 0x77; // Turn up volume on both left & right
	NR51_REG = 0xFF; // Turn on all 8 channels

	set_bkg_data(0, 3, floortiles);		// Load 3 tiles into background memory

	set_bkg_tiles(0, 0, 32, 18, floormap);
	stage_width = (UINT16)(sizeof(floormap_full_segments) / sizeof(floormap_full_segments[0]));
	set_sprite_data(0, 4, eggy);
	set_sprite_tile(0, 0);
	move_sprite(0, jumpy_x, jumpy_y);
	SHOW_SPRITES;
	SHOW_BKG;

	while(1){
		UBYTE joypad_state = joypad();
		UINT16 i = 0;

		if (joypad_state) {
			if(joypad_state & J_A) {
				if (isjumping == 0){
					jump();
				}
			}
			if(joypad_state & J_LEFT && isjumping) {  
				if (jumpy_x > 8) {
					jumpy_x -= 2;
					move_sprite(0, jumpy_x, jumpy_y);
				}
			}

			if(joypad_state & J_RIGHT && isjumping) {
				if (jumpy_x < 80 || (reached_end && jumpy_x < 160)) {
					jumpy_x += 2;
					move_sprite(0, jumpy_x, jumpy_y);
				} else if (!reached_end) {
					bkg_position_offset += 2;
					bkg_colscroll_counter += 2;
					scroll_bkg(2, 0);
					if (bkg_colscroll_counter == 8) {
						bkg_colscroll_counter = 0;
						if (bkg_columns_scrolled < stage_width) {
							// Only continue to render stage if there is stage left to render.
							set_bkg_tiles(next_vram_location, 0, 1, 18, floormap_full_segments[bkg_columns_scrolled]);
							for (i = 0; i < 18; i = i + 1)
							{
								floormap[(i * 32) + next_vram_location] = floormap_full_segments[bkg_columns_scrolled][i];
							}
						}
						bkg_columns_scrolled += 1;
						if (bkg_columns_scrolled - 12 == stage_width) {
							// We've displayed all the stage there is! No more scrolling.
							// NOTE: the -12 refers to the original viewport (32 VRAM width minus initial 20 viewport = 12)
							reached_end = 1;
						}
						next_vram_location += 1;
						if (next_vram_location == 32) {
							next_vram_location = 0;
						}
					}
				}
			}

		}
		if (isjumping == 1) {
			animatejump();
		} else {
			animateidle();
		}
		wait_vbl_done();
		performantdelay(2);
	}
}