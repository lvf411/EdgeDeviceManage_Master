#ifndef __MSG_HPP
#define __MSG_HPP

#include <thread>
#include "master.hpp"
#include "file.hpp"
#include "msg_type.hpp"
#include "interact_status.hpp"

#define MSG_BUFFER_SIZE 1024

void msg_send(ClientNode *client);
void msg_recv(ClientNode *client);

#endif //__MSG_HPP