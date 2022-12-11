#include <vitasdk.h>
#include <taihen.h>
#include <string.h>
#include "renderer.h"

#define FPS_TIMER_TICK    1000000
#define FRAMETIMES_NUM 360
#define FRAMERATES_NUM 15

#define NUM_MODES 4
enum {
	NO_UI,
	MINIMAL_UI, // FPS counter
	STANDARD_UI, // Frametime chart + FPS counter
	EXTENDED_UI // Frametime chart + FPS chart + Framecounter
};
int mode_ui = STANDARD_UI;

static SceUID hook;
static tai_hook_ref_t display_ref;
uint32_t tick = 0;
uint32_t frametime_tick = 0;
uint32_t frames = 0;
int framerates[FRAMERATES_NUM * 2] = {};
float frametimes[FRAMETIMES_NUM * 2] = {};
float max_frametime = 0.0f, min_frametime = 999.0f;
int max_fps = 0, min_fps = 9999;
int frametime_idx = FRAMETIMES_NUM;
int framerate_idx = FRAMERATES_NUM;
uint64_t framecounter = 0;
SceCtrlData pad;
uint32_t oldpad = 0;
char config_path[256];

#define FRAMETIME_CHART_Y 15
#define FRAMETIME_BIG_CHART_Y 25
int calc_y_frametime_big(float ftime) {
	return FRAMETIME_BIG_CHART_Y + (int)(100 - ftime);
}
int calc_y_frametime(float ftime) {
	return FRAMETIME_CHART_Y + (int)(50 - (ftime / 2));
}

#define FRAMERATE_BIG_CHART_Y 160
#define FRAMERATE_CHART_Y 80
int calc_y_framerate_big(int frate) {
	return FRAMERATE_BIG_CHART_Y + (int)(60 - frate);
}
int calc_y_framerate(int frate) {
	return FRAMERATE_CHART_Y + (int)(30 - (frate / 2));
}

int sceDisplaySetFrameBuf_patched(const SceDisplayFrameBuf *pParam, int sync) {
	if (!pParam)
		return TAI_CONTINUE(int, display_ref, pParam, sync);
	
	sceCtrlPeekBufferPositive(0, &pad, 1);
	if (pad.buttons & SCE_CTRL_START && (pad.buttons & SCE_CTRL_LEFT && !(oldpad & SCE_CTRL_LEFT))) {
		mode_ui = (mode_ui + 1) % NUM_MODES;
		SceUID fd = sceIoOpen(config_path, SCE_O_TRUNC | SCE_O_CREAT | SCE_O_WRONLY, 0777);
		if (fd > 0) {
			sceIoWrite(fd, &mode_ui, sizeof(int));
			sceIoClose(fd);
		}
	}
	if (pad.buttons & SCE_CTRL_START && (pad.buttons & SCE_CTRL_RIGHT && !(oldpad & SCE_CTRL_RIGHT))) {
		max_frametime = 0.0f;
		min_frametime = 999.0f;
		max_fps = 0;
		min_fps = 9999;
	}
	oldpad = pad.buttons;
	framecounter++;
	
	uint32_t new_frame_tick = sceKernelGetProcessTimeLow();
	updateFramebuf(pParam);
	if (tick == 0)
		tick = new_frame_tick;
	else {
		if ((new_frame_tick - tick) > FPS_TIMER_TICK) {
			framerate_idx = (framerate_idx + 1) % (FRAMERATES_NUM * 2);
			framerates[framerate_idx] = frames;
			if (framerates[framerate_idx] > max_fps)
				max_fps = framerates[framerate_idx];
			else if (framerates[framerate_idx] < min_fps)
				min_fps = framerates[framerate_idx];
			frames = 0;
			tick = new_frame_tick;
		}
		
		if (pParam->height > 408) { // High res
			switch (mode_ui) {
			case MINIMAL_UI:
				drawBigStringF(5, 5, "FPS: %d (Min: %d, Max: %d)", framerates[framerate_idx], min_fps, max_fps);
				break;
			case STANDARD_UI:
			case EXTENDED_UI:
				drawBigStringF(5, 140, "FPS: %d (Min: %d, Max: %d)", framerates[framerate_idx], min_fps, max_fps);
				break;
			default:
				break;
			}
		
			if (mode_ui == EXTENDED_UI) {
				int curr_idx = framerate_idx;
				int previous_idx = curr_idx - 1;
				int x = 80;
				for (int i = 0; i < FRAMERATES_NUM; i++) {
					if (previous_idx < 0)
						previous_idx = FRAMERATES_NUM * 2 - 1;
					int p_frate = framerates[previous_idx] > 60 ? 60 : framerates[previous_idx];
					int c_frate = framerates[curr_idx] > 60 ? 60 : framerates[curr_idx];
					drawLine(x - 5, calc_y_framerate_big(p_frate),x, calc_y_framerate_big(c_frate), 0x0000FF00);
					x -= 5;
					curr_idx = previous_idx;
					previous_idx--;
				}
			}
		} else { // Low res
			switch (mode_ui) {
			case MINIMAL_UI:
				drawStringF(5, 5, "FPS: %d (Min: %d, Max: %d)", framerates[framerate_idx], min_fps, max_fps);
				break;
			case STANDARD_UI:
			case EXTENDED_UI:
				drawStringF(5, 70, "FPS: %d (Min: %d, Max: %d)", framerates[framerate_idx], min_fps, max_fps);
				break;
			default:
				break;
			}
		
			if (mode_ui == EXTENDED_UI) {
				int curr_idx = framerate_idx;
				int previous_idx = curr_idx - 1;
				int x = 35;
				for (int i = 0; i < FRAMERATES_NUM; i++) {
					if (previous_idx < 0)
						previous_idx = FRAMERATES_NUM * 2 - 1;
					int p_frate = framerates[previous_idx] > 60 ? 60 : framerates[previous_idx];
					int c_frate = framerates[curr_idx] > 60 ? 60 : framerates[curr_idx];
					drawLine(x - 2, calc_y_framerate(p_frate),x, calc_y_framerate(c_frate), 0x0000FF00);
					x -= 2;
					curr_idx = previous_idx;
					previous_idx--;
				}
			}
		}
		
	}
	frames++;
	
	if (frametime_tick) {
		frametimes[frametime_idx] = (float)(new_frame_tick - frametime_tick) / 1000;
		if (frametimes[frametime_idx] < min_frametime)
			min_frametime = frametimes[frametime_idx];
		else if (frametimes[frametime_idx] > max_frametime)
			max_frametime = frametimes[frametime_idx];
		if (mode_ui >= STANDARD_UI) {
			if (pParam->height > 408) { // High res
				drawBigStringF(5, 5, "Frametime: %.2fms (Min: %.2fms, Max: %.2fms)", frametimes[frametime_idx], min_frametime, max_frametime);
				if (mode_ui == EXTENDED_UI)
					drawBigStringF(5, 245, "Frame Count: %llu", framecounter);	
			} else { // Low res
				drawStringF(5, 5, "Frametime: %.2fms (Min: %.2fms, Max: %.2fms)", frametimes[frametime_idx], min_frametime, max_frametime);
				if (mode_ui == EXTENDED_UI)
					drawStringF(5, 115, "Frame Count: %llu", framecounter);
			}
		}
	}
	frametime_tick = new_frame_tick;

	int curr_idx = frametime_idx;
	int previous_idx = curr_idx - 1;
	if (pParam->height > 408) {// High res
		if (mode_ui >= STANDARD_UI) {
			int x = 365;
			for (int i = 0; i < FRAMETIMES_NUM; i++) {
				if (previous_idx < 0)
					previous_idx = FRAMETIMES_NUM * 2 - 1;
				float p_time = frametimes[previous_idx] > 100.0f ? 100.0f : frametimes[previous_idx];
				float c_time = frametimes[curr_idx] > 100.0f ? 100.0f : frametimes[curr_idx];
				drawLine(x - 1, calc_y_frametime_big(p_time), x, calc_y_frametime_big(c_time), 0x0000FF00);
				x--;
				curr_idx = previous_idx;
				previous_idx--;
			}
		}
		frametime_idx = (frametime_idx + 1) % (FRAMETIMES_NUM * 2);
	} else { // Low res
		if (mode_ui >= STANDARD_UI) {
			int x = 185;
			for (int i = 0; i < FRAMETIMES_NUM / 2; i++) {
				if (previous_idx < 0)
					previous_idx = FRAMETIMES_NUM - 1;
				float p_time = frametimes[previous_idx] > 100.0f ? 100.0f : frametimes[previous_idx];
				float c_time = frametimes[curr_idx] > 100.0f ? 100.0f : frametimes[curr_idx];
				drawLine(x - 1, calc_y_frametime(p_time), x, calc_y_frametime(c_time), 0x0000FF00);
				x--;
				curr_idx = previous_idx;
				previous_idx--;
			}
		}
		frametime_idx = (frametime_idx + 1) % FRAMETIMES_NUM;
	}
	
	return TAI_CONTINUE(int, display_ref, pParam, sync);
}	

void _start() __attribute__ ((weak, alias ("module_start")));
int module_start(SceSize argc, const void *args) {
	// Hooking sceDisplaySetFrameBuf
	hook = taiHookFunctionImport(&display_ref,
						TAI_MAIN_MODULE,
						TAI_ANY_LIBRARY,
						0x7A410B64,
						sceDisplaySetFrameBuf_patched);
						
	setTextColor(0x00FFFF00);
	
	char titleid[16];
	sceAppMgrAppParamGetString(0, 12, titleid , 16);
	sceIoMkdir("ux0:data/benchmark", 0777);
	sprintf(config_path, "ux0:data/benchmark/%s.cfg", titleid);
	SceUID fd = sceIoOpen(config_path, SCE_O_RDONLY, 0777);
	if (fd > 0) {
		sceIoRead(fd, &mode_ui, sizeof(int));
		sceIoClose(fd);
	}
						
	return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args) {

	// Freeing hook
	taiHookRelease(hook, display_ref);

	return SCE_KERNEL_STOP_SUCCESS;
	
}