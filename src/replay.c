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

#define BUFFER_SIZE (256 * 1024)

static queue *replayBuffer = NULL;

static struct replay *replay = NULL;

void initReplayQueue(void)
{
	replayBuffer = createQueue(sizeof(struct game));
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

void pushReplayState(struct game *state)
{
	struct game *nextState = produceQueueItem(replayBuffer);
	memcpy(nextState, state, sizeof(*nextState));
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
			outBuffer = reallocate(outBuffer, outBufferSize + BUFFER_SIZE);
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
	replayBuffer->buffer = outBuffer;
	replayBuffer->count = outBufferSize / replayBuffer->itemSize;
	replayBuffer->allocated = (outBufferSize + BUFFER_SIZE) / replayBuffer->itemSize;
}

void initReplay(void)
{
	replay = allocate(sizeof(*replay));
	app->replay = replay;
	replay->playback = false;
	replay->frame = 0;
	replay->totalFrames = 0;

	initReplayQueue();
	loadReplay();
	replay->totalFrames = replayBuffer->count;
}

void cleanupReplay(void)
{
	cleanupReplayQueue();
}

static enum replayCmd inputToCmd(enum inputType i)
{
	switch (i) {
	case InputButton1:
		return ReplCmdPlay;
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

		if (cmd == ReplCmdPlay)
			replay->playback = !replay->playback;
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

	app->game = getQueueItem(replayBuffer, replay->frame);
	if(replay->playback) {
		if (replay->frame + 1 < replay->totalFrames)
			replay->frame += 1;
		else
			replay->playback = false;
	}
}