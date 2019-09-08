// Minote - replay.c

#include "replay.h"

#include <string.h>
#include <stdio.h>
#include <errno.h>

#include <lzma.h>

#include "gameplay.h"
#include "queue.h"
#include "log.h"

#define BUFFER_SIZE (256 * 1024)

queue *replay = NULL;

void initReplay(void)
{
	replay = createQueue(sizeof(struct game));
}

void cleanupReplay(void)
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

	FILE *replayFile = fopen("replay.mre", "w");
	if (!replayFile) {
		logError("Failed to open a replay file for writing: %s",
		         strerror(errno));
		return;
	}

	lzma.next_in = replay->buffer;
	lzma.avail_in = replay->itemSize * replay->count;
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
	destroyQueue(replay);
	replay = NULL;
}

void pushState(struct game *state)
{
	struct game *nextState = produceQueueItem(replay);
	memcpy(nextState, state, sizeof(*nextState));
}
