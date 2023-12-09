#ifndef INPUTSET_H
#define INPUTSET_H

typedef struct {
	int16_t mouse_x;
	int16_t mouse_y;
	uint8_t mouse_left : 1;
	uint8_t w : 1;
	uint8_t a : 1;
	uint8_t s : 1;
	uint8_t d : 1;
} InputSet;

#endif /* INPUTSET_H */
