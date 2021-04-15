
struct Struct
{
    uint8_t index;
    uint16_t some_value;
};

msg Message
{
    uint8_t flags;
    Struct list[];
    Struct fixed_list[3];
};
