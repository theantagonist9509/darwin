#include <errno.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <poll.h>
#include <sys/socket.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "raylib.h"

#include "gamestate.h"
#include "inputset.h"
#include "ioutils.h"

#define WINDOW_WIDTH 720 // make window dims dynamic / custom to screen?
#define WINDOW_HEIGHT 405

void readInputSet(InputSet *input_set)
{
	input_set->mouse_x = GetMouseX() - WINDOW_WIDTH / 2;
	input_set->mouse_y = GetMouseY() - WINDOW_HEIGHT / 2;
	input_set->mouse_left = IsMouseButtonDown(MOUSE_LEFT_BUTTON);
	input_set->w = IsKeyDown(KEY_W);
	input_set->a = IsKeyDown(KEY_A);
	input_set->s = IsKeyDown(KEY_S);
	input_set->d = IsKeyDown(KEY_D);
}

void sendInputSetToServer(int con_sock_fd, InputSet const *input_set) // sendBufferToServer? return error enum?
{
	ssize_t bytes_written = sendAll(con_sock_fd, input_set, sizeof (InputSet)); // put inside if statement? (check everywhere)
	if (bytes_written != sizeof (InputSet)) {
		perror("Failed to send data to server"); // Failed to send data to server
		exit(EXIT_FAILURE); // exit (or return error code)?
	}
}

void receiveGameStateFromServer(int con_sock_fd, GameState *local_game_state) // receiveBufferFromServer? return error enum?
{
	ssize_t bytes_read = recvAll(con_sock_fd, &local_game_state->num_tanks, sizeof local_game_state->num_tanks);
	//if (bytes_read == 0) { // need this special case?
	//	perror("???");
	//	exit(EXIT_FAILURE);
	//}
	if (bytes_read != sizeof local_game_state->num_tanks) {
		perror("Failed to receive data from server"); // Failed to receive data from server?
		exit(EXIT_FAILURE);
	}
	bytes_read = recvAll(con_sock_fd, &local_game_state->tanks, local_game_state->num_tanks * sizeof (Tank));
	//if (bytes_read == 0) { // need this special case?
	//	perror("???");
	//	exit(EXIT_FAILURE);
	//}
	if (bytes_read != local_game_state->num_tanks * sizeof (Tank)) {
		perror("Failed to receive data from server"); // Failed to receive data from server?
		exit(EXIT_FAILURE);
	}
}


#include "raymath.h" // use Vector2? (it has floats)


void drawGameState(GameState const *local_game_state)
{
	Vector2 window_offset;
	window_offset.x = WINDOW_WIDTH / 2;
	window_offset.y = WINDOW_HEIGHT / 2;

	BeginDrawing();
	ClearBackground(BLACK);

	for (uint8_t i = 0; i < local_game_state->num_tanks; i++) { // move player-relative-pos calculations from server to client?
		Vector2 offset_tank_pos = Vector2Add(local_game_state->tanks[i].pos, window_offset);
		DrawLineV(offset_tank_pos, Vector2Add(offset_tank_pos, Vector2Scale(local_game_state->tanks[i].dir, 40)), WHITE);
		DrawCircleV(offset_tank_pos, 20, RED);
	}

	 // client-side prediction
	Vector2 mouse_dir;
	if (IsWindowFocused()) {
		Vector2 mouse_pos;
		mouse_pos.x = GetMouseX() - WINDOW_WIDTH / 2;
		mouse_pos.y = GetMouseY() - WINDOW_HEIGHT / 2;
		mouse_dir = Vector2Normalize(mouse_pos);
	} else {
		// ??? (figure out system design for latency, sync, and client-side-prediction)
	}

	DrawLineV(window_offset, Vector2Add(window_offset, Vector2Scale(mouse_dir, 40)), WHITE);
	DrawCircleV(window_offset, 20, BLUE);

	DrawFPS(50, 50); // draw at proper (x, y)

	EndDrawing();
}

int main(int argc, char **argv)
{
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <server_ip>:<server_port>\n", argv[0]);
		return EXIT_FAILURE;
	}

	int con_sock_fd = socket(AF_INET, SOCK_STREAM, 0); // server_sock_fd? conn instead of con? (check everywhere including server)
	if (con_sock_fd == -1) {
		perror("Failed to create connecting socket");
		return EXIT_FAILURE;
	}

	char *server_ip = strtok(argv[1], ":");
	in_port_t server_port = atoi(strtok(NULL, ":"));

	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof server_addr); // necessary?
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(server_port);
	inet_pton(AF_INET, server_ip, &server_addr.sin_addr);

	if (connect(con_sock_fd, (struct sockaddr *)&server_addr, sizeof server_addr) == -1) {
		perror("Failed to connect to server");
		return EXIT_FAILURE;
	}

	//int sock_flags = fcntl(con_sock_fd, F_GETFL);
	//if (sock_flags < 0) {
	//	perror("Failed to get server socket peer flags"); // what should i even call this sockfd???
	//}
	//sock_flags = fcntl(con_sock_fd, F_SETFL, sock_flags | O_NONBLOCK);
	//if (sock_flags < 0) {
	//	perror("Failed to set server socket peer flags");
	//}

	struct pollfd server_pfd;
	server_pfd.fd = con_sock_fd;
	server_pfd.events = POLLIN | POLLOUT;

	InputSet input_set;
	memset(&input_set, 0, sizeof input_set);
	GameState local_game_state;

	InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "darwin-client");
	SetTargetFPS(60);

	while (!WindowShouldClose()) {
		if (IsWindowFocused()) {
			readInputSet(&input_set);
		} else { // just don't send anything???
			input_set.mouse_left = 0;
			input_set.w = 0;
			input_set.a = 0;
			input_set.s = 0;
			input_set.d = 0;
		}

		//int poll_ret = poll(&server_pfd, 1, 0);
		//if (poll_ret < 0) {
		//	perror("Failed to poll server socket"); // what should i even call this sockfd???
		//} else if (poll_ret > 0) {
		//	if (server_pfd.revents & POLLOUT) {
				sendInputSetToServer(con_sock_fd, &input_set);
		//	}

		//	if (server_pfd.revents & POLLIN) {
				receiveGameStateFromServer(con_sock_fd, &local_game_state);
		//	}
		//}

		drawGameState(&local_game_state); // draw even while waiting for server response (client-side prediction)
	}

	CloseWindow();
	close(con_sock_fd);
	return EXIT_SUCCESS;
}
