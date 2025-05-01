struct IDTEntry
{
    uint16_t offset_low;
    uint16_t selector;
    uint8_t zero;
    uint8_t type_attr;
    uint16_t offset_high;
} __attribute__((packed));

struct IDTPointer
{
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

#define IDT_SIZE 256
struct IDTEntry idt[IDT_SIZE];
struct IDTPointer idtPtr;

__attribute__((naked)) void defaultISR() { __asm__ volatile ("iret"); }

void initIDT()
{
    uint32_t handler = (uint32_t)defaultISR;
    for (int i = 0; i < IDT_SIZE; ++i)
    {
        idt[i].offset_high = (handler >> 16) & 0xFFFF;
        idt[i].offset_low = handler & 0xFFFF;
        idt[i].selector = 0x08;
        idt[i].zero = 0;
        idt[i].type_attr = 0x8E;
    }

    idtPtr.limit = sizeof(idt) - 1;
    idtPtr.base = (uint32_t)&idt;

    __asm__ volatile ("lidtl (%0)" : : "r" (&idtPtr));
}

void remapPIC()
{
    outb(0x20, 0x11); outb(0xA0, 0x11);
    outb(0x21, 0x20); outb(0xA1, 0x28);
    outb(0x21, 0x04); outb(0xA1, 0x02);
    outb(0x21, 0x01); outb(0xA1, 0x01);
    outb(0x21, 0x00); outb(0xA1, 0x00);
}

void enableInterrupts()
{
    initIDT();
    remapPIC();
    __asm__ volatile ("sti");
}
