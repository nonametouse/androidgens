#define LOG_TAG "libgens"
#include <utils/Log.h>
#include "emuengine.h"
#include "drmd_main.h"
#include "app.h"

#define SCREEN_W		320
#define SCREEN_H		240
#define SCREEN_PITCH	320

extern "C" void Blit8To16Asm(void *src, void *dst, void *pal, int width);

static EmuEngine::Callbacks *callbacks;

class GensEngine : public EmuEngine {
public:
	GensEngine();
	virtual ~GensEngine();

	virtual bool initialize(Callbacks *cbs);
	virtual void destroy();
	virtual void reset();
	virtual void power();
	virtual Game *loadRom(const char *file);
	virtual void unloadRom();
	virtual bool saveState(const char *file);
	virtual bool loadState(const char *file);
	virtual void runFrame(bool skip);
	virtual void setOption(const char *name, const char *value);
};


GensEngine::GensEngine()
{
}

GensEngine::~GensEngine()
{
	drmdCleanup();
}

bool GensEngine::initialize(EmuEngine::Callbacks *cbs)
{
	callbacks = cbs;

	return drmdInitialize();
}

void GensEngine::destroy()
{
	delete this;
}

void GensEngine::reset()
{
	drmdReset();
}

void GensEngine::power()
{
	reset();
}

GensEngine::Game *GensEngine::loadRom(const char *file)
{
	if (!drmdLoadRom(file))
		return NULL;

	extern int frame_limit;
	static Game game;
	game.soundRate = 22050;
	game.soundBits = 16;
	game.soundChannels = 2;
	game.fps = frame_limit;
	return &game;
}

void GensEngine::unloadRom()
{
	drmdUnloadRom();
}

bool GensEngine::saveState(const char *file)
{
	drmdSaveState(file);
	return true;
}

bool GensEngine::loadState(const char *file)
{
	drmdLoadState(file);
	return true;
}

static void videoUpdate()
{
	EmuEngine::Surface surface;
	if (!callbacks->lockSurface(&surface))
		return;

	extern unsigned char VBuf[];
	extern unsigned int VPalette[];

	unsigned char *d = (unsigned char *) surface.bits;
	unsigned char *s = VBuf;
	for (int h = SCREEN_H; --h >= 0; ) {
		Blit8To16Asm(s, d, VPalette, SCREEN_W);
		d += surface.bpr;
		s += SCREEN_PITCH;
	}
	callbacks->unlockSurface(&surface);
}

void GensEngine::runFrame(bool skip)
{
	drmd.pad = callbacks->getKeyStates();

	if (skip) {
		drmdRunFrame(0);
	} else {
		drmdRunFrame(1);
		videoUpdate();
	}

	extern short soundbuffer[];
	extern unsigned int sound_buffer_size;
	if (sound_on)
		callbacks->playAudio(soundbuffer, sound_buffer_size * 4);
}

void GensEngine::setOption(const char *name, const char *value)
{
	if (strcmp(name, "sixButtonPad") == 0)
		drmdSetPadType(strcmp(value, "false") != 0);
	else if (strcmp(name, "soundEnabled") == 0)
		drmdSetSound(strcmp(value, "false") != 0);
}

extern "C" __attribute__((visibility("default")))
EmuEngine *createEngine()
{
	return new GensEngine;
}
