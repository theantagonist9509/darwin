#include <arpa/inet.h>
#include <errno.h>
#include <poll.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "raymath.h"

#include "inputs.h"
#include "ioutils.h"
#include "gamestate.h"
#include "tank.h" // SHOULD TANK_H BE COMMON?

#define NUM_FDS (1 + MAX_PLAYERS)
nfds_t accept_and_add(int accepting_sock_fd, struct pollfd *pfds)
{
	struct sockaddr_in client_addr;
	socklen_t client_addr_size = sizeof client_addr;
	int con_sock_fd = accept(accepting_sock_fd, (struct sockaddr*)&client_addr, &client_addr_size);
	if (con_sock_fd == -1) {
		return 0; // change if keeping accepting socket separate from player sockets
	}
	for (nfds_t i = 1; i < NUM_FDS; i++) {
		if (pfds[i].fd == -1) {
			pfds[i].fd = con_sock_fd;
			pfds[i].events = POLLIN | POLLOUT;
			return i;
		}
	}
	close(con_sock_fd);
	return 0; // change if keeping accepting socket separate from player sockets
}

int main(int argc, char* argv[])
{
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <port_num>\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	uint16_t port_num = atoi(argv[1]);
	int accepting_sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (accepting_sock_fd == -1) {
		fprintf(stderr, "Failed to create socket: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	struct sockaddr_in server_addr;
	bzero(&server_addr, sizeof server_addr);
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port_num);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(accepting_sock_fd, (struct sockaddr*)&server_addr, sizeof server_addr) == -1) {
		fprintf(stderr, "Failed to bind socket to port %d\n", port_num);
		close(accepting_sock_fd);
		exit(EXIT_FAILURE);
	}
	printf("BindDone: %d\n", port_num);
	if (listen(accepting_sock_fd, 3) == -1) {
		fprintf(stderr, "Failed to listen on socket\n");
		close(accepting_sock_fd);
		exit(EXIT_FAILURE);
	}
	printf("ListenDone: %d\n", port_num);

	struct pollfd pfds[NUM_FDS];
	pfds[0].fd = accepting_sock_fd;
	pfds[0].events = POLLIN;
	pfds[0].revents = 0; // set to what?
	for (nfds_t i = 1; i < NUM_FDS; ++i) { // uint8_t? check everywhere
		pfds[i].fd = -1;
	}

	GameState game_state;
	game_state.num_tanks = 0;
	uint8_t speed = 5; // make a macro?

	while (poll(pfds, NUM_FDS, 0) >= 0) {
		if (pfds[0].revents & POLLIN) {
			nfds_t i = accept_and_add(accepting_sock_fd, pfds);
			if (i == 0) {
				fprintf(stderr, "Failed to accept connection: %s\n", strerror(errno));
			} else {
				game_state.tanks[i].pos.x = 0;
				game_state.tanks[i].pos.y = 0;
				game_state.num_tanks++;
				printf("Client connected\nPlayers: %u\n", game_state.num_tanks);
			}
		}
		for (nfds_t i = 1; i < NUM_FDS; i++) {
			if (pfds[i].fd > 0 && (pfds[i].revents & (POLLIN | POLLOUT))) { // > 0 instead of >= 0 since fd = 0 is reserved for stdin
				Inputs inputs;
				size_t bytes_read = readFileToBuffer(pfds[i].fd, &inputs, sizeof inputs);
				if (bytes_read == 0) {
					close(pfds[i].fd);
					pfds[i].fd = -1;
					game_state.num_tanks--;
					printf("Client disconnected\nPlayers: %u\n", game_state.num_tanks);
					continue;
				}

				Vector2 inputs_mouse_pos;
				inputs_mouse_pos.x = inputs.mouse_x;
				inputs_mouse_pos.y = inputs.mouse_y;
				game_state.tanks[i].dir = Vector2Normalize(inputs_mouse_pos);
printf("[%lu]: (%f, %f)\n", i, game_state.tanks[i].dir.x, game_state.tanks[i].dir.y);
				if (inputs.w) { // branchless?
					game_state.tanks[i].pos.y -= speed;
				}
				if (inputs.a) {
					game_state.tanks[i].pos.x -= speed;
				}
				if (inputs.s) {
					game_state.tanks[i].pos.y += speed;
				}
				if (inputs.d) {
					game_state.tanks[i].pos.x += speed;
				}

				GameState buf; // buffer? currently just putting all tanks into buffer; fix this to make it local-only
				buf.num_tanks = 0;
				for (uint8_t j = 1; j < NUM_FDS;  j++) {
					if (pfds[j].fd != -1 && j != i) {
						buf.tanks[buf.num_tanks].pos = Vector2Subtract(game_state.tanks[j].pos, game_state.tanks[i].pos);
						buf.tanks[buf.num_tanks].dir = game_state.tanks[j].dir;
						buf.num_tanks++;
					}
				}
				writeBufferToFile(pfds[i].fd, &buf.num_tanks, sizeof buf.num_tanks);
				writeBufferToFile(pfds[i].fd, &buf.tanks, buf.num_tanks * sizeof (Tank));
			}
		}
	}
	return EXIT_SUCCESS;
}
