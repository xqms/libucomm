
struct ServoCommand
{
	uint8_t id;
	uint16_t command;
};

msg ServoPacket
{
	ServoCommand cmds[];
	uint8_t flags;
};
