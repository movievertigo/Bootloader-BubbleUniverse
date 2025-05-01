__asm__("jmp _start");

#define CURVECOUNT 256
#define CURVESTEP 4
#define ITERATIONS 256

#define SCREENWIDTH 320
#define SCREENHEIGHT 200
#define SCREENBUFFER 0xA0000
#define SCREENBUFFERSIZE (SCREENWIDTH*SCREENHEIGHT)

#define INITSIZE (SCREENHEIGHT * 0.25)
#define INITANIMSPEED 0.001
#define SCALESPEED 1.004
#define MOVESPEED 1.0
#define ANIMSPEEDCHANGE 0.002

#define PI 3.1415926535897932384626433832795
#define SINTABLEPOWER 14
#define SINTABLEENTRIES (1<<SINTABLEPOWER)
#define ANG1INC (CURVESTEP*2*PI/235)
#define ANG2INC CURVESTEP

#define KEY_ESCAPE      0x01
#define KEY_MINUS       0x0C
#define KEY_EQUAL       0x0D
#define KEY_BACKSPACE   0x0E
#define KEY_RETURN      0x1C
#define KEY_SLASH       0x35
#define KEY_SPACE       0x39
#define KEY_HOME        0x47
#define KEY_LEFT        0x4B
#define KEY_RIGHT       0x4D
#define KEY_UP          0x48
#define KEY_DOWN        0x50

typedef unsigned char uint8_t;      typedef char int8_t;
typedef unsigned short uint16_t;    typedef short int16_t;
typedef unsigned int uint32_t;      typedef int int32_t;


void outb(uint16_t port, uint8_t value)
{
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

uint8_t inb(uint16_t port) {
    uint8_t result;
    __asm__ volatile ("inb %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

void vsync(void) { while (!(inb(0x3DA) & 0x08)) {} }

void clearBuffer(uint8_t* buffer, uint32_t value, uint32_t dwordCount) { __asm__ volatile ("rep stosl" : "+D"(buffer), "+c"(dwordCount) : "a"(value) : "memory"); }
void copyBuffer(uint8_t* src, uint8_t* dst, uint32_t dwordCount) { __asm__ volatile ("rep movsl" : "+D"(dst), "+S"(src),"+c"(dwordCount) : : "memory"); }
void setPaletteEntry(uint8_t i, uint8_t r, uint8_t g, uint8_t b) { outb(0x3C8, i); outb(0x3C9, r); outb(0x3C9, g); outb(0x3C9, b); }

double sin(double x) { __asm__ volatile("fsin" : "+t"(x)); return x; }
float sinf(float x) { __asm__ volatile("fsin" : "+t"(x)); return x; }
double cos(double x) { __asm__ volatile("fcos" : "+t"(x)); return x; }
float cosf(float x) { __asm__ volatile("fcos" : "+t"(x)); return x; }

void makeSinTable(double* table)
{
    for (int i = 0; i < SINTABLEENTRIES; ++i)
    {
        table[i] = sin((double)i * 2*PI/SINTABLEENTRIES);
    }
}

void setPalette()
{
    for (int i = 0; i < 16; ++i)
    {
        const int red = (i*63)/15;
        for (int j = 0; j < 16; ++j)
        {
            const int green = (j*63)/15;
            const int blue = (126-(red+green))>>1;
            setPaletteEntry(i*16+j, red, green, blue);
        }
    }
    setPaletteEntry(0, 0, 0, 0);
}

void pollKeyboard(uint8_t* keyState)
{
    if (inb(0x64) & 0x01)
    {
        uint8_t scanCode = inb(0x60);
        keyState[scanCode & 0x7F] = !(scanCode&0x80);
    }
}

#include "irq.h"

void _start()
{
    enableInterrupts();

    uint8_t keyState[128] = {0};
    double sinTable[SINTABLEENTRIES];
    uint8_t backBuffer[SCREENBUFFERSIZE];
    uint8_t* screenBuffer = (uint8_t*)SCREENBUFFER;
    setPalette();
    makeSinTable(sinTable);

    double size = INITSIZE;
    double animSpeed = INITANIMSPEED;
    double oldAnimSpeed = 0.0;
    double animTime = 0.0;
    double xOffset = 0.0;
    double yOffset = 0.0;
    uint8_t debug = 0;
    while (1)
    {
        clearBuffer(backBuffer, 0, SCREENBUFFERSIZE/4);
        if (debug) setPaletteEntry(0, 0, 0, 63);

        double ang1Start = animTime;
        double ang2Start = animTime;

        for (int i = 0; i < CURVECOUNT; i += CURVESTEP)
        {
            double x = 0, y = 0;
            for (int j = 0; j < ITERATIONS; ++j)
            {
                int ang1Offset = (int)((ang1Start + x)*(SINTABLEENTRIES/2/PI));
                int ang2Offset = (int)((ang2Start + y)*(SINTABLEENTRIES/2/PI));
                x = sinTable[ang1Offset & (SINTABLEENTRIES-1)] + sinTable[ang2Offset & (SINTABLEENTRIES-1)];
                y = sinTable[(ang1Offset+SINTABLEENTRIES/4) & (SINTABLEENTRIES-1)] + sinTable[(ang2Offset+SINTABLEENTRIES/4) & (SINTABLEENTRIES-1)];
                double pX = SCREENWIDTH/2 + xOffset + x*size;
                double pY = SCREENHEIGHT/2 + yOffset + y*size;
                if (pX >= 0 && pX < SCREENWIDTH && pY >= 0 && pY < SCREENHEIGHT)
                {
                    backBuffer[(int)pY*SCREENWIDTH + (int)pX] = ((i*16)/CURVECOUNT)*16 + (j*16)/ITERATIONS;
                }
            }

            ang1Start += ANG1INC;
            ang2Start += ANG2INC;
        }

        animTime += animSpeed;

        uint8_t oldSpaceState = keyState[KEY_SPACE];
        uint8_t oldSlashState = keyState[KEY_SLASH];
        pollKeyboard(keyState);
        if (keyState[KEY_HOME] || keyState[KEY_ESCAPE]) { size = INITSIZE; animSpeed = INITANIMSPEED; xOffset = 0.0; yOffset = 0.0; }
        if (keyState[KEY_MINUS]) animSpeed -= ANIMSPEEDCHANGE / size;
        if (keyState[KEY_EQUAL]) animSpeed += ANIMSPEEDCHANGE / size;
        if (keyState[KEY_RETURN]) { size *= SCALESPEED; xOffset *= SCALESPEED; yOffset *= SCALESPEED; }
        if (keyState[KEY_BACKSPACE]) { size /= SCALESPEED; xOffset /= SCALESPEED; yOffset /= SCALESPEED; }
        if (keyState[KEY_SPACE] && !oldSpaceState) { if (animSpeed == 0.0) { animSpeed = oldAnimSpeed; } else { oldAnimSpeed = animSpeed; animSpeed = 0.0; } }
        if (keyState[KEY_SLASH] && !oldSlashState) { debug = !debug; setPaletteEntry(0, 0, 0, 0); }
        if (keyState[KEY_LEFT]) xOffset += MOVESPEED;
        if (keyState[KEY_RIGHT]) xOffset -= MOVESPEED;
        if (keyState[KEY_UP]) yOffset += MOVESPEED;
        if (keyState[KEY_DOWN]) yOffset -= MOVESPEED;

        if (debug) setPaletteEntry(0, 0, 0, 0);
        vsync();
        if (debug) setPaletteEntry(0, 63, 0, 0);
        copyBuffer(backBuffer, screenBuffer, SCREENBUFFERSIZE/4);
        if (debug) setPaletteEntry(0, 0, 63, 0);
    }
}
