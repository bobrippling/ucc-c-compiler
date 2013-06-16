#ifndef SNAPSHOT_H
#define SNAPSHOT_H

typedef struct snapshot snapshot;

snapshot *snapshot_take(void);
void snapshot_take_post(snapshot *);

void snapshot_blue_used(snapshot *);
void snapshot_unblue_used(snapshot *);

void snapshot_restore_used(snapshot *);

void snapshot_free(snapshot *);

#endif
