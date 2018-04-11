typedef __attribute__((__packed__)) union message_type {
	struct {
		uint16_t x_pos;
		uint16_t y_pos;
	}
	char[] bytes;
} message;

