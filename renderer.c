/*
 * This File is Part Of : 
 *      ___                       ___           ___           ___           ___           ___                 
 *     /  /\        ___          /__/\         /  /\         /__/\         /  /\         /  /\          ___   
 *    /  /::\      /  /\         \  \:\       /  /:/         \  \:\       /  /:/_       /  /::\        /  /\  
 *   /  /:/\:\    /  /:/          \  \:\     /  /:/           \__\:\     /  /:/ /\     /  /:/\:\      /  /:/  
 *  /  /:/~/:/   /__/::\      _____\__\:\   /  /:/  ___   ___ /  /::\   /  /:/ /:/_   /  /:/~/::\    /  /:/   
 * /__/:/ /:/___ \__\/\:\__  /__/::::::::\ /__/:/  /  /\ /__/\  /:/\:\ /__/:/ /:/ /\ /__/:/ /:/\:\  /  /::\   
 * \  \:\/:::::/    \  \:\/\ \  \:\~~\~~\/ \  \:\ /  /:/ \  \:\/:/__\/ \  \:\/:/ /:/ \  \:\/:/__\/ /__/:/\:\  
 *  \  \::/~~~~      \__\::/  \  \:\  ~~~   \  \:\  /:/   \  \::/       \  \::/ /:/   \  \::/      \__\/  \:\ 
 *   \  \:\          /__/:/    \  \:\        \  \:\/:/     \  \:\        \  \:\/:/     \  \:\           \  \:\
 *    \  \:\         \__\/      \  \:\        \  \::/       \  \:\        \  \::/       \  \:\           \__\/
 *     \__\/                     \__\/         \__\/         \__\/         \__\/         \__\/                
 *
 * Copyright (c) Rinnegatamante <rinnegatamante@gmail.com>
 *
 */

#include <psp2/types.h>
#include <psp2/display.h>
#include <libk/stdio.h>
#include <libk/stdarg.h>
#include <libk/string.h>
#include "font.h"

unsigned int* vram32;
int bufferwidth;
uint32_t color;

void updateFramebuf(const SceDisplayFrameBuf *param) {
	vram32 = param->base;
	bufferwidth = param->pitch;
}

void setTextColor(uint32_t clr) {
	color = clr;
}

void drawCharacter(int character, int x, int y) {
    for (int yy = 0; yy < 10; yy++) {
        uint32_t *screenPos = vram32 + x + (y + yy) * bufferwidth;

        uint8_t charPos = font[character * 10 + yy];
        for (int xx = 7; xx >= 2; xx--) {
			if ((charPos >> xx) & 1) {
				*screenPos = color;
				*(screenPos + 1) = 0;
				*(screenPos + bufferwidth + 1) = 0;
				*(screenPos + bufferwidth) = 0;
			}
			screenPos++;
        }
    }
}

void drawBigCharacter(int character, int x, int y){
    for (int yy = 0; yy < 10; yy++) {
        int xDisplacement = x;
        int yDisplacement = (y + (yy<<1)) * bufferwidth;
        uint32_t* screenPos = vram32 + xDisplacement + yDisplacement;

        uint8_t charPos = font[character * 10 + yy];
        for (int xx = 7; xx >= 2; xx--) {
			if ((charPos >> xx) & 1) {
				*(screenPos) = color;
				*(screenPos + 1) = color;
				*(screenPos + 2) = 0;
				*(screenPos + bufferwidth) = color;
				*(screenPos + bufferwidth + 1) = color;
				*(screenPos + bufferwidth + 2) = 0;
				*(screenPos + bufferwidth * 2) = 0;
				*(screenPos + 1+ bufferwidth * 2) = 0;
			}
			screenPos += 2;
        }
    }
}

void drawString(int x, int y, const char *str) {
    for (size_t i = 0; i < strlen(str); i++)
        drawCharacter(str[i], x + i * 6, y);
}

void drawBigString(int x, int y, const char *str) {
    for (size_t i = 0; i < strlen(str); i++)
        drawBigCharacter(str[i], x + i * 12, y);
}

void drawStringF(int x, int y, const char *format, ...) {
	char str[512] = { 0 };
	va_list va;

	va_start(va, format);
	sceClibVsnprintf(str, 512, format, va);
	va_end(va);

	drawString(x, y, str);
}

void drawBigStringF(int x, int y, const char *format, ...) {
	char str[512] = { 0 };
	va_list va;

	va_start(va, format);
	sceClibVsnprintf(str, 512, format, va);
	va_end(va);

	drawBigString(x, y, str);
}

void drawLine(float x, float y, float x2, float y2, uint32_t color) {
	int inverted = 0;
	if (y > y2) {
		float tmp = y;
		y = y2;
		y2 = tmp;
		inverted = 1;
	}
	float x_diff = x2 - x;
	float y_diff = y2 - y;
	float x_incr, y_incr;
	if (y_diff > x_diff) {
		x_incr = x_diff / y_diff;
		y_incr = 1;
	} else {
		y_incr = y_diff / x_diff;
		x_incr = 1;
	}
	while (x < x2 || y < y2) {
		if (inverted) {
			uint32_t *screenPos = vram32 + (int)x2 + (int)y * bufferwidth;
			*screenPos = color;
			x2 -= x_incr;
			y += y_incr;
		} else {
			uint32_t *screenPos = vram32 + (int)x + (int)y * bufferwidth;
			*screenPos = color;
			x += x_incr;
			y += y_incr;
		}
	}
}
