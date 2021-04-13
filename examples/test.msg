
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

struct ServoStatus
{
    uint16_t position;
};

msg ServoStatusPacket
{
    ServoStatus servos[];
};