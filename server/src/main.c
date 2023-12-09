#include <errno.h>
#include <arpa/inet.h>
#include <poll.h>
#include <sys/socket.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "raymath.h"

#include "gamestate.h"
#include "inputset.h"
#include "ioutils.h"
#include "tank.h" // SHOULD TANK_H BE COMMON?

int8_t acceptAndAdd(int accepting_sock_fd, struct pollfd *client_pfds)
{
	struct sockaddr_in client_addr;
	socklen_t client_addr_size = sizeof client_addr;
	int con_sock_fd = accept(accepting_sock_fd, (struct sockaddr*)&client_addr, &client_addr_size);
	if (con_sock_fd < 0) {
		return -1;
	}
	for (uint8_t i = 0; i < MAX_PLAYERS; i++) {
		if (client_pfds[i].fd < 0) {
			client_pfds[i].fd = con_sock_fd;
			client_pfds[i].events = POLLIN | POLLOUT;
			return i;
		}
	}
	close(con_sock_fd);
	return -2;
}

uint16_t milliDiffTimeSpec(struct timespec const *ts1, struct timespec const *ts2)
{
	return (ts1->tv_sec - ts2->tv_sec) * 1000 + (ts1->tv_nsec - ts2->tv_nsec) / 1000000;
}

#define TARGET_TPS 30 // 1 causes nanosleep() to fail
#define BASE_TANK_SPEED 0.2

int main(int argc, char **argv)
{
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <port_num>\n", argv[0]);
		return EXIT_FAILURE;
	}
	uint16_t port_num = atoi(argv[1]);
	int accepting_sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (accepting_sock_fd < 0) {
		fprintf(stderr, "Failed to create socket: %s\n", strerror(errno));
		return EXIT_FAILURE;
	}

	struct sockaddr_in server_addr;
	bzero(&server_addr, sizeof server_addr);
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port_num);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(accepting_sock_fd, (struct sockaddr*)&server_addr, sizeof server_addr) == -1) {
		fprintf(stderr, "Failed to bind socket to port: %s\n", strerror(errno));
		return EXIT_FAILURE;
	}

	if (listen(accepting_sock_fd, MAX_PLAYERS) < 0) {
		fprintf(stderr, "Failed to listen on socket: %s\n", strerror(errno));
		return EXIT_FAILURE;
	}
	printf("Listening...\n");

	struct pollfd accepting_pfd;
	accepting_pfd.fd = accepting_sock_fd;
	accepting_pfd.events = POLLIN;

	struct pollfd client_pfds[MAX_PLAYERS];
	for (uint8_t i = 0; i < MAX_PLAYERS; i++) {
		client_pfds[i].fd = -1;
	}

	GameState game_state;
	game_state.num_tanks = 0;

	struct timespec prev;
	clock_gettime(CLOCK_MONOTONIC, &prev);
	while (true) {
		// accept client connection
		int poll_ret = poll(&accepting_pfd, 1, 0);
		if (poll_ret < 0) {
			fprintf(stderr, "Failed to poll accepting socket: %s\n", strerror(errno));
		} else if (poll_ret > 0) {
			if (accepting_pfd.revents & POLLIN) {
				int8_t idx = acceptAndAdd(accepting_sock_fd, client_pfds);
				if (idx == -1) {
					fprintf(stderr, "Failed to accept client connection: %s\n", strerror(errno));
				} else if (idx == -2) {
					printf("Refused client connection request\n");
				} else {
					game_state.tanks[idx].pos.x = 0; // spawning code goes here
					game_state.tanks[idx].pos.y = 0;
					game_state.num_tanks++;
					printf("Client connected\nPlayers: %u\n", game_state.num_tanks);
				}
			}
		}

		// receive client inputs
		InputSet input_sets[MAX_PLAYERS];
		memset(&input_sets, 0, sizeof input_sets); // overrides mouse inputs :(
		poll_ret = poll(client_pfds, MAX_PLAYERS, 0);
		if (poll_ret < 0) {
			fprintf(stderr, "Failed to poll client sockets: %s\n", strerror(errno));
		} else if (poll_ret > 0) {
			for (uint8_t i = 0; i < MAX_PLAYERS; i++) {
				if (client_pfds[i].revents & POLLIN) { // no need to check fd = -1 as poll overrides revents
					ssize_t bytes_read = recvAll(client_pfds[i].fd, &input_sets[i], sizeof input_sets[i]);
					if (bytes_read < 0 && errno != ECONNRESET) {
						fprintf(stderr, "Failed to receive data from client: %s\n", strerror(errno));
					} else if (bytes_read <= 0) {
						close(client_pfds[i].fd);
						client_pfds[i].fd = -1;
						game_state.num_tanks--;
						printf("Client disconnected\n"
						       "Players: %u\n", game_state.num_tanks);
					}
				}
			}
		}

		// where should i put the tps capping code??
		// cap tps
		struct timespec current;
		clock_gettime(CLOCK_MONOTONIC, &current);
		uint16_t delta_time = milliDiffTimeSpec(&current, &prev);
		if (delta_time < 1000 / (float)TARGET_TPS) {
			struct timespec rem;
			rem.tv_sec = 0;
			rem.tv_nsec = (1000 / (float)TARGET_TPS - delta_time) * 1000000;
			nanosleep(&rem, NULL); // fails for TARGET_TPS = 1
			clock_gettime(CLOCK_MONOTONIC, &current);
			delta_time = milliDiffTimeSpec(&current, &prev);
		}
		prev = current;

		// update game-state
		for (uint8_t i = 0; i < MAX_PLAYERS; i++) {
			if (client_pfds[i].fd > 0) {
				Vector2 mouse_pos;
				mouse_pos.x = input_sets[i].mouse_x;
				mouse_pos.y = input_sets[i].mouse_y;
				game_state.tanks[i].dir = Vector2Normalize(mouse_pos);
				if (input_sets[i].w) {
					game_state.tanks[i].pos.y -= delta_time * BASE_TANK_SPEED;
				}
				if (input_sets[i].a) {
					game_state.tanks[i].pos.x -= delta_time * BASE_TANK_SPEED;
				}
				if (input_sets[i].s) {
					game_state.tanks[i].pos.y += delta_time * BASE_TANK_SPEED;
				}
				if (input_sets[i].d) {
					game_state.tanks[i].pos.x += delta_time * BASE_TANK_SPEED;
				}
printf("[%u]: (%f, %f)\n", i, game_state.tanks[i].pos.x, game_state.tanks[i].pos.y);
			}
		}

		// send client-local game-state
		if (poll_ret > 0) { // already checked poll_ret < 0 above
			for (uint8_t i = 0; i < MAX_PLAYERS; i++) {
				if (client_pfds[i].fd > 0 && client_pfds[i].revents & POLLOUT) { // need to check fd = -1 as fd could be set to -1 after call to poll()
					GameState buf; // currently just putting all tanks into buffer; fix this to make it local-only
					buf.num_tanks = 0;
					for (uint8_t j = 0; j < MAX_PLAYERS;  j++) {
						if (client_pfds[j].fd > 0 && j != i) { // memcpy?
							buf.tanks[buf.num_tanks].pos = Vector2Subtract(game_state.tanks[j].pos, game_state.tanks[i].pos);
							buf.tanks[buf.num_tanks].dir = game_state.tanks[j].dir;
							buf.num_tanks++;
						}
					}

					ssize_t send_ret = sendAll(client_pfds[i].fd, &buf.num_tanks, sizeof buf.num_tanks);
					if (send_ret < 0) {
						goto send_error;
					}

					send_ret = sendAll(client_pfds[i].fd, &buf.tanks, buf.num_tanks * sizeof (Tank));
					if (send_ret < 0) {
					send_error:
						if (errno != ECONNRESET &&
						    errno != ENOTCONN &&
						    errno != EPIPE) {
							fprintf(stderr, "Failed to send data to client: %s\n", strerror(errno));
						}
						continue;
					}
				}
			}
		}
	}
	return EXIT_SUCCESS;
}
