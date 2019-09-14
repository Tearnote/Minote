// Minote - replay.c

#include "replay.h"

#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include <lzma.h>

#include "readall/readall.h"

#include "gameplay.h"
#include "queue.h"
#include "log.h"
#include "input.h"
#include "state.h"
#include "util.h"

#define BUFFER_SIZE (256 * 1024)

enum replayCmd {
	ReplCmdNone,
	ReplCmdPlay,
	ReplCmdFwd, ReplCmdBack,
	ReplCmdSkipFwd, ReplCmdSkipBack,
	ReplCmdFaster, ReplCmdSlower,
	ReplCmdSize
};

static queue *replayBuffer = NULL;

static struct replay *replay = NULL;

void initReplayQueue(void)
{
	replayBuffer = createQueue(sizeof(struct replayFrame));
}

void cleanupReplayQueue(void)
{
	destroyQueue(replayBuffer);
	replayBuffer = NULL;
}

void saveReplay(void)
{
	lzma_stream lzma = LZMA_STREAM_INIT;
	lzma_ret lzmaRet =
		lzma_easy_encoder(&lzma, LZMA_PRESET_DEFAULT, LZMA_CHECK_CRC64);
	if (lzmaRet != LZMA_OK) {
		const char *msg;
		switch (lzmaRet) {
		case LZMA_MEM_ERROR:
			msg = "Memory allocation failed";
			break;
		case LZMA_OPTIONS_ERROR:
			msg = "Specified preset is not supported";
			break;
		case LZMA_UNSUPPORTED_CHECK:
			msg = "Specified integrity check is not supported";
			break;
		default:
			msg = "Unknown error";
			break;
		}
		logError("Failed to initialize compression stream: %s", msg);
		return;
	}

	FILE *replayFile = fopen("replay.mre", "wb");
	if (!replayFile) {
		logError("Failed to open the replay file for writing: %s",
		         strerror(errno));
		return;
	}

	lzma.next_in = replayBuffer->buffer;
	lzma.avail_in = replayBuffer->itemSize * replayBuffer->count;
	uint8_t outBuffer[BUFFER_SIZE];
	lzma.next_out = outBuffer;
	lzma.avail_out = sizeof(outBuffer);
	while (true) {
		lzmaRet = lzma_code(&lzma, LZMA_FINISH);

		if (lzma.avail_out == 0 || lzmaRet == LZMA_STREAM_END) {
			size_t outBytes = sizeof(outBuffer) - lzma.avail_out;
			if (fwrite(outBuffer, 1, outBytes, replayFile)
			    != outBytes) {
				logError("Failed to write the replay: %s\n",
				         strerror(errno));
				return;
			}

			lzma.next_out = outBuffer;
			lzma.avail_out = sizeof(outBuffer);
		}

		if (lzmaRet != LZMA_OK) {
			if (lzmaRet == LZMA_STREAM_END)
				break;

			const char *msg;
			switch (lzmaRet) {
			case LZMA_MEM_ERROR:
				msg = "Memory allocation failed";
				break;
			case LZMA_DATA_ERROR:
				msg = "File size limits exceeded";
				break;
			default:
				msg = "Unknown error";
				break;
			}
			logError("Failed to compress the replay: %s", msg);
			return;
		}
	}

	fclose(replayFile);
	lzma_end(&lzma);
}

void pushReplayFrame(struct game *frame)
{
	struct replayFrame *replayFrame = produceQueueItem(replayBuffer);
	// Can't use copyArray, different element size
	for (int y = 0; y < PLAYFIELD_H; y++)
		for (int x = 0; x < PLAYFIELD_W; x++)
			replayFrame->playfield[y][x] = frame->playfield[y][x];
	replayFrame->player.state = frame->player.state;
	replayFrame->player.x = frame->player.x;
	replayFrame->player.y = frame->player.y;
	replayFrame->player.type = frame->player.type;
	replayFrame->player.preview = frame->player.preview;
	replayFrame->player.rotation = frame->player.rotation;
	replayFrame->level = frame->level;
	replayFrame->nextLevelstop = frame->nextLevelstop;
	replayFrame->score = frame->score;
	// This is fine because char is guaranteed to by one byte
	copyArray(replayFrame->gradeString, frame->gradeString);
	replayFrame->eligible = frame->eligible;
	copyArray(replayFrame->cmdRaw, frame->cmdRaw);
}

static void loadReplay(void)
{
	lzma_stream lzma = LZMA_STREAM_INIT;
	lzma_ret lzmaRet =
		lzma_stream_decoder(&lzma, UINT64_MAX, LZMA_CONCATENATED);
	if (lzmaRet != LZMA_OK) {
		const char *msg;
		switch (lzmaRet) {
		case LZMA_MEM_ERROR:
			msg = "Memory allocation failed";
			break;
		case LZMA_OPTIONS_ERROR:
			msg = "Specified preset is not supported";
			break;
		default:
			msg = "Unknown error";
			break;
		}
		logError("Failed to initialize decompression stream: %s", msg);
		return;
	}

	FILE *replayFile = fopen("replay.mre", "rb");
	if (!replayFile) {
		logError("Failed to open the replay file for reading: %s",
		         strerror(errno));
		return;
	}
	char *compressed = NULL;
	size_t compressedSize = 0;
	if (readall(replayFile, &compressed, &compressedSize) != READALL_OK) {
		logError("Could not read contents of replay file");
		fclose(replayFile);
		return;
	}
	fclose(replayFile);

	lzma.next_in = (uint8_t *)compressed;
	lzma.avail_in = compressedSize;
	char *outBuffer = allocate(BUFFER_SIZE);
	size_t outBufferSize = 0;
	lzma.next_out = (uint8_t *)outBuffer;
	lzma.avail_out = BUFFER_SIZE;
	while (true) {
		lzmaRet = lzma_code(&lzma, LZMA_FINISH);

		if (lzma.avail_out == 0 || lzmaRet == LZMA_STREAM_END) {
			size_t outBytes = BUFFER_SIZE - lzma.avail_out;
			outBufferSize += outBytes;
			outBuffer = reallocate(outBuffer,
			                       outBufferSize + BUFFER_SIZE);
			lzma.next_out = (uint8_t *)(outBuffer + outBufferSize);
			lzma.avail_out = BUFFER_SIZE;
		}

		if (lzmaRet != LZMA_OK) {
			if (lzmaRet == LZMA_STREAM_END)
				break;

			const char *msg;
			switch (lzmaRet) {
			case LZMA_MEM_ERROR:
				msg = "Memory allocation failed";
				break;
			case LZMA_DATA_ERROR:
				msg = "File size limits exceeded";
				break;
			default:
				msg = "Unknown error";
				break;
			}
			logError("Failed to decompress the replay: %s", msg);
			return;
		}
	}
	lzma_end(&lzma);
	free(compressed);

	free(replayBuffer->buffer);
	// We ignore the remainders in this buffer handoff because they
	// don't matter, it won't go out of bounds and will be corrected
	// by the next realloc anyway
	replayBuffer->buffer = outBuffer;
	replayBuffer->count = outBufferSize / replayBuffer->itemSize;
	replayBuffer->allocated =
		(outBufferSize + BUFFER_SIZE) / replayBuffer->itemSize;
	replay->frames = (struct replayFrame *)outBuffer;
}

void initReplay(void)
{
	replay = allocate(sizeof(*replay));
	app->replay = replay;
	replay->playback = false;
	replay->frame = 0.0;
	replay->totalFrames = 0;
	replay->speed = 1.0f;

	initReplayQueue();
	loadReplay();
	replay->totalFrames = replayBuffer->count;

	lockMutex(&gameMutex);
	app->game = allocate(sizeof(*app->game));
	memset(app->game, 0, sizeof(*app->game));
	unlockMutex(&gameMutex);
}

void cleanupReplay(void)
{
	lockMutex(&gameMutex);
	free(app->game);
	app->game = NULL;
	unlockMutex(&gameMutex);

	cleanupReplayQueue();
}

static void clampFrame(void)
{
	if (replay->frame < 0)
		replay->frame = 0;
	if (replay->frame >= replay->totalFrames)
		replay->frame = replay->totalFrames - 1;
}

static enum replayCmd inputToCmd(enum inputType i)
{
	switch (i) {
	case InputButton1:
		return ReplCmdPlay;
	case InputLeft:
		return ReplCmdBack;
	case InputRight:
		return ReplCmdFwd;
	case InputDown:
		return ReplCmdSkipBack;
	case InputUp:
		return ReplCmdSkipFwd;
	case InputButton2:
		return ReplCmdSlower;
	case InputButton3:
		return ReplCmdFaster;
	default:
		return ReplCmdNone;
	}
}

static void processInput(struct input *i)
{
	enum replayCmd cmd = inputToCmd(i->type);
	switch (i->action) {
	case ActionPressed:
		if (i->type == InputQuit) {
			logInfo("User exited");
			setState(AppShutdown);
			break;
		}

		switch (cmd) {
		case ReplCmdPlay:
			replay->playback = !replay->playback;
			break;
		case ReplCmdFwd:
			replay->frame += 1;
			break;
		case ReplCmdBack:
			replay->frame -= 1;
			break;
		case ReplCmdSkipFwd:
			replay->frame += 60 * 5;
			break;
		case ReplCmdSkipBack:
			replay->frame -= 60 * 5;
			break;
		case ReplCmdSlower:
			replay->speed /= 2.0f;
			break;
		case ReplCmdFaster:
			replay->speed *= 2.0f;
			break;
		default:
			break;
		}
		clampFrame();
	default:
		break;
	}
}

static void processInputs(void)
{
	// Receive all pending inputs
	struct input *in = NULL;
	while ((in = dequeueInput())) {
		processInput(in);
		free(in);
	}
}

void updateReplay(void)
{
	processInputs();

	struct replayFrame *frame = &replay->frames[(int)replay->frame];
	lockMutex(&gameMutex);
	for (int y = 0; y < PLAYFIELD_H; y++)
		for (int x = 0; x < PLAYFIELD_W; x++)
			app->game->playfield[y][x] = frame->playfield[y][x];
	app->game->player.state = frame->player.state;
	app->game->player.x = frame->player.x;
	app->game->player.y = frame->player.y;
	app->game->player.type = frame->player.type;
	app->game->player.preview = frame->player.preview;
	app->game->player.rotation = frame->player.rotation;
	app->game->level = frame->level;
	app->game->nextLevelstop = frame->nextLevelstop;
	app->game->score = frame->score;
	copyArray(app->game->gradeString, frame->gradeString);
	app->game->eligible = frame->eligible;
	copyArray(app->game->cmdRaw, frame->cmdRaw);
	app->game->time = (nsec)replay->frame * GAMEPLAY_FRAME_LENGTH;
	unlockMutex(&gameMutex);

	if (replay->playback) {
		replay->frame += replay->speed;
		clampFrame();
		if ((int)replay->frame + 1 >= replay->totalFrames)
			replay->playback = false;
	}
}