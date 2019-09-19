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
#include "logic.h"

#define BUFFER_SIZE (256 * 1024)
#define HEADER_MAGIC "Minotereplay"
#define HEADER_VERSION "0001"

void
pushReplayHeader(struct replay *replay, struct game *game, int keyframeFreq)
{
	if (!replay->frames)
		replay->frames = createVqueue();
	copyArray(replay->header.magic, HEADER_MAGIC);
	copyArray(replay->header.version, HEADER_VERSION);
	memcpy(&replay->header.initialRng, &game->rngState,
	       sizeof(replay->header.initialRng));
	replay->header.keyframeFreq = keyframeFreq;
}

void pushReplayFrame(struct replay *replay, struct game *frame)
{
	if (replay->header.totalFrames % replay->header.keyframeFreq == 0) {
		struct replayKeyframe *keyframe =
			produceVqueueItem(replay->frames,
			                  sizeof(struct replayKeyframe));
		memcpy(&keyframe->rngState, &frame->rngState,
		       sizeof(keyframe->rngState));
		// Can't use copyArray, different element size
		for (int y = 0; y < PLAYFIELD_H; y++)
			for (int x = 0; x < PLAYFIELD_W; x++)
				keyframe->playfield[y][x] =
					frame->playfield[y][x];
		for (int y = 0; y < PLAYFIELD_H; y++)
			keyframe->clearedLines[y] = frame->clearedLines[y];
		keyframe->player.state = frame->player.state;
		keyframe->player.x = frame->player.x;
		keyframe->player.y = frame->player.y;
		keyframe->player.ySub = frame->player.ySub;
		keyframe->player.type = frame->player.type;
		keyframe->player.preview = frame->player.preview;
		for (int i = 0; i < HISTORY_SIZE; i++)
			keyframe->player.history[i] = frame->player.history[i];
		keyframe->player.rotation = frame->player.rotation;
		keyframe->player.dasDirection = frame->player.dasDirection;
		keyframe->player.dasCharge = frame->player.dasCharge;
		keyframe->player.dasDelay = frame->player.dasDelay;
		keyframe->player.lockDelay = frame->player.lockDelay;
		keyframe->player.clearDelay = frame->player.clearDelay;
		keyframe->player.spawnDelay = frame->player.spawnDelay;
		keyframe->player.dropBonus = frame->player.dropBonus;
		keyframe->level = frame->level;
		keyframe->nextLevelstop = frame->nextLevelstop;
		keyframe->score = frame->score;
		keyframe->combo = frame->combo;
		keyframe->grade = frame->grade;
		// This is fine because char is guaranteed to be one byte
		copyArray(keyframe->gradeString, frame->gradeString);
		keyframe->eligible = frame->eligible;
		for (int i = 0; i < GameCmdSize; i++)
			keyframe->cmdHeld[i] = frame->cmdHeld[i];
		for (int i = 0; i < GameCmdSize; i++)
			keyframe->cmdPrev[i] = frame->cmdPrev[i];
		keyframe->frame = frame->frame;
		keyframe->time = frame->time;
	}

	struct replayInputframe *inputframe = produceVqueueItem(replay->frames,
	                                                        sizeof(struct replayInputframe));
	for (int i = 0; i < GameCmdSize; i++)
		inputframe->inputs[i] = frame->cmdRaw[i];
	inputframe->lastDirection = frame->lastDirection;

	replay->header.totalFrames += 1;
}

void saveReplay(struct replay *replay)
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

	lzma.avail_in = sizeof(replay->header) + replay->frames->size;
	uint8_t *inBuffer = allocate(lzma.avail_in);
	lzma.next_in = inBuffer;
	size_t offset = 0;
	memcpy(inBuffer, &replay->header, sizeof(replay->header));
	offset += sizeof(replay->header);
	memcpy(inBuffer + offset, replay->frames->buffer, replay->frames->size);

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

void loadReplay(struct replay *replay)
{
	if (!replay->frames)
		replay->frames = createVqueue();

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

	size_t offset = 0;
	memcpy(&replay->header, outBuffer, sizeof(replay->header));
	offset += sizeof(replay->header);
	if (memcmp(replay->header.magic, HEADER_MAGIC,
	           sizeof(replay->header.magic)) != 0) {
		logError("Invalid replay file");
		free(outBuffer);
		return;
	}
	if (memcmp(replay->header.version, HEADER_VERSION,
	           sizeof(replay->header.version)) != 0) {
		logError("Invalid replay version");
		free(outBuffer);
		return;
	}

	replay->frames->size = outBufferSize - offset;
	replay->frames->allocated = replay->frames->size;
	replay->frames->buffer = reallocate(replay->frames->buffer,
	                                    replay->frames->allocated);
	memcpy(replay->frames->buffer, outBuffer + offset,
	       replay->frames->allocated);
	free(outBuffer);
}

static struct replayKeyframe *getKeyframe(struct replay *replay, int frame)
{
	int keyframe = frame / replay->header.keyframeFreq;
	size_t offset = keyframe * (sizeof(struct replayKeyframe) +
	                            replay->header.keyframeFreq
	                            * sizeof(struct replayInputframe));
	return (struct replayKeyframe *)(replay->frames->buffer + offset);
}

static struct replayInputframe *getInputs(struct replay *replay, int frame)
{
	size_t offset = frame * sizeof(struct replayInputframe);
	offset += sizeof(struct replayKeyframe)
	          * (frame / replay->header.keyframeFreq + 1);
	return (struct replayInputframe *)(replay->frames->buffer + offset);
}

void applyReplayInitial(struct game *game, struct replay *replay)
{
	memcpy(&game->rngState, &replay->header.initialRng,
	       sizeof(game->rngState));
}

void applyReplayKeyframe(struct game *game, struct replay *replay, int frame)
{
	if (frame % replay->header.keyframeFreq != 0) {
		logDebug("Failed to apply keyframe: %d is not a keyframe",
		         frame);
		return;
	}

	struct replayKeyframe *keyframe = getKeyframe(replay, frame);
	memcpy(&game->rngState, &keyframe->rngState,
	       sizeof(game->rngState));
	// Can't use copyArray, different element size
	for (int y = 0; y < PLAYFIELD_H; y++)
		for (int x = 0; x < PLAYFIELD_W; x++)
			game->playfield[y][x] =
				keyframe->playfield[y][x];
	for (int y = 0; y < PLAYFIELD_H; y++)
		game->clearedLines[y] = keyframe->clearedLines[y];
	game->player.state = keyframe->player.state;
	game->player.x = keyframe->player.x;
	game->player.y = keyframe->player.y;
	game->player.ySub = keyframe->player.ySub;
	game->player.type = keyframe->player.type;
	game->player.preview = keyframe->player.preview;
	for (int i = 0; i < HISTORY_SIZE; i++)
		game->player.history[i] = keyframe->player.history[i];
	game->player.rotation = keyframe->player.rotation;
	game->player.dasDirection = keyframe->player.dasDirection;
	game->player.dasCharge = keyframe->player.dasCharge;
	game->player.dasDelay = keyframe->player.dasDelay;
	game->player.lockDelay = keyframe->player.lockDelay;
	game->player.clearDelay = keyframe->player.clearDelay;
	game->player.spawnDelay = keyframe->player.spawnDelay;
	game->player.dropBonus = keyframe->player.dropBonus;
	game->level = keyframe->level;
	game->nextLevelstop = keyframe->nextLevelstop;
	game->score = keyframe->score;
	game->combo = keyframe->combo;
	game->grade = keyframe->grade;
	// This is fine because char is guaranteed to be one byte
	copyArray(game->gradeString, keyframe->gradeString);
	game->eligible = keyframe->eligible;
	for (int i = 0; i < GameCmdSize; i++)
		game->cmdHeld[i] = keyframe->cmdHeld[i];
	for (int i = 0; i < GameCmdSize; i++)
		game->cmdPrev[i] = keyframe->cmdPrev[i];
	game->frame = keyframe->frame;
	game->time = keyframe->time;
}

void applyReplayInputs(struct game *game, struct replay *replay, int frame)
{
	struct replayInputframe *inputframe = getInputs(replay, frame);
	for (int i = 0; i < GameCmdSize; i++)
		game->cmdRaw[i] = inputframe->inputs[i];
	game->lastDirection = inputframe->lastDirection;
}