
struct ServoCommand
{
	uint8_t id;
	uint16_t command;
};

struct ServoPacket
{
	ServoCommand cmds[];
	uint8_t flags;
};
