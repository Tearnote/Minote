// Minote - main/poll.h
// Polls input events and inserts them into the queue

#ifndef MAIN_POLL_H
#define MAIN_POLL_H

void initPoll(void);
void cleanupPoll(void);

void updatePoll(void);
void sleepPoll(void);

#endif //MAIN_POLL_H
