/**
 * Various table data for the purers sublayer
 * @file
 */

#ifndef MINOTE_PURERSTABLES_H
#define MINOTE_PURERSTABLES_H

#include "mino.h"
#include "time.h"

/// Defines a level at which some physics change
typedef struct PurersThreshold {
	int level;
	int gravity;
} PurersThreshold;

/// List of thresholds for purers gamemode
#define PurersThresholds ((PurersThreshold[]){ \
    { .level = 0, .gravity = 4 },          \
    { .level = 30, .gravity = 6 },         \
    { .level = 35, .gravity = 8 },         \
    { .level = 40, .gravity = 10 },        \
    { .level = 50, .gravity = 12 },        \
    { .level = 60, .gravity = 16 },        \
    { .level = 70, .gravity = 32 },        \
    { .level = 80, .gravity = 48 },        \
    { .level = 90, .gravity = 64 },        \
    { .level = 100, .gravity = 80 },       \
    { .level = 120, .gravity = 96 },       \
    { .level = 140, .gravity = 112 },      \
    { .level = 160, .gravity = 128 },      \
    { .level = 170, .gravity = 144 },      \
    { .level = 200, .gravity = 4 },        \
    { .level = 220, .gravity = 32 },       \
    { .level = 230, .gravity = 64 },       \
    { .level = 233, .gravity = 96 },       \
    { .level = 236, .gravity = 128 },      \
    { .level = 239, .gravity = 160 },      \
    { .level = 243, .gravity = 192 },      \
    { .level = 247, .gravity = 224 },      \
    { .level = 251, .gravity = 256 },      \
    { .level = 300, .gravity = 512 },      \
    { .level = 330, .gravity = 768 },      \
    { .level = 360, .gravity = 1024 },     \
    { .level = 400, .gravity = 1280 },     \
    { .level = 420, .gravity = 1024 },     \
    { .level = 450, .gravity = 768 },      \
    { .level = 500, .gravity = 5120 }      \
})

/**
 * Defines a level at which certain requirements have to be met in order
 * to quality for the highest rank.
 */
typedef struct PurersRequirement {
	int level;
	int score;
	nsec time;
} PurersRequirement;

/// List of requirements to get the top rank in purers gamemode
#define PurersRequirements ((PurersRequirement[]){                        \
    { .level = 300, .score = 12000,  .time = secToNsec(4 * 60 + 15)}, \
    { .level = 500, .score = 40000,  .time = secToNsec(7 * 60)},      \
    { .level = 999, .score = 126000, .time = secToNsec(13 * 60 + 30)} \
})

/// List of score requirements for each rank of purers gamemode
#define PurersGrades ((int[]){ \
    0,                       \
    400,                     \
    800,                     \
    1400,                    \
    2000,                    \
    3500,                    \
    5500,                    \
    8000,                    \
    12000,                   \
    16000,                   \
    22000,                   \
    30000,                   \
    40000,                   \
    52000,                   \
    66000,                   \
    82000,                   \
    100000,                  \
    120000,                  \
    126000                   \
})

/**
 * Query the rotation system for a specific piece.
 * @param type Type of the piece, between MinoNone and MinoGarbage (exclusive)
 * @param rotation Spin of the piece
 * @return Read-only piece data
 */
piece* purersGetPiece(mino type, spin rotation);

#endif //MINOTE_PURERSTABLES_H
