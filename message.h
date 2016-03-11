#ifndef MESSAGE_H_
#define MESSAGE_H_

#include "vm.h"

#define MSG_CHECKSUM 1234
#define MSG_LEN 12 // TODO

struct message {
	struct message *prev;
	struct message *next;
	int sender_id;
	int sender_lc;
};

struct message *msg_create(int sender_id, int sender_lc);
void msg_destroy(struct message *msg);

void msg_send(int sockfd, int sender_id, int sender_lc);
struct message *msg_read(int sockfd, int *buf, int buflen);

void msg_print(struct message *msg);

#endif /* MESSAGE_H_ */