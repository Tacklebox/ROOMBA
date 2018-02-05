enum command {
	FIRE,
	TEST
};

typedef struct packet {
    enum command cmd;
    int param;  
}__attribute__((packed)) packet_t ;
