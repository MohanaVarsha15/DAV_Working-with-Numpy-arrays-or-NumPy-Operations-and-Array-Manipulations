#include <reg51.h>
#include <intrins.h>

// --- SENSORS & LED ---
sbit LDR = P1^0; 
sbit STREET_LED = P2^0; 

// --- OLED I2C PINS ---
sbit SDA = P2^1;
sbit SCL = P2^2;

// --- STATE VARIABLES ---
volatile int carCount = 0;
volatile unsigned int entry_debounce = 0;
volatile unsigned int exit_debounce = 0;

unsigned char pwm_count = 0;
unsigned char pwm_duty = 0;

// Dashboard Update & Animation Trackers
int last_carCount = -1;
unsigned char last_mode = 99; 
unsigned char anim_tick = 0;
bit blink_flag = 0;
bit last_blink_flag = 0;

// --- EXPANDED FONT ARRAY (Added ., *, - for animations) ---
code unsigned char font[][5] = {
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, // 0
    {0x00, 0x42, 0x7F, 0x40, 0x00}, // 1
    {0x42, 0x61, 0x51, 0x49, 0x46}, // 2
    {0x21, 0x41, 0x45, 0x4B, 0x31}, // 3
    {0x18, 0x14, 0x12, 0x7F, 0x10}, // 4
    {0x27, 0x45, 0x45, 0x45, 0x39}, // 5
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, // 6
    {0x01, 0x71, 0x09, 0x05, 0x03}, // 7
    {0x36, 0x49, 0x49, 0x49, 0x36}, // 8
    {0x06, 0x49, 0x49, 0x29, 0x1E}, // 9
    {0x00, 0x00, 0x00, 0x00, 0x00}, // Space (Index 10)
    {0x24, 0x24, 0x24, 0x24, 0x24}, // : (Index 11)
    {0x7E, 0x11, 0x11, 0x11, 0x7E}, // A (Index 12)
    {0x7F, 0x49, 0x49, 0x49, 0x36}, // B
    {0x3E, 0x41, 0x41, 0x41, 0x22}, // C
    {0x7F, 0x41, 0x41, 0x22, 0x1C}, // D
    {0x7F, 0x49, 0x49, 0x49, 0x41}, // E
    {0x7F, 0x09, 0x09, 0x09, 0x01}, // F
    {0x3E, 0x41, 0x49, 0x49, 0x7A}, // G
    {0x7F, 0x08, 0x08, 0x08, 0x7F}, // H
    {0x00, 0x41, 0x7F, 0x41, 0x00}, // I
    {0x20, 0x40, 0x41, 0x3F, 0x01}, // J
    {0x7F, 0x08, 0x14, 0x22, 0x41}, // K
    {0x7F, 0x40, 0x40, 0x40, 0x40}, // L
    {0x7F, 0x02, 0x0C, 0x02, 0x7F}, // M
    {0x7F, 0x04, 0x08, 0x10, 0x7F}, // N
    {0x3E, 0x41, 0x41, 0x41, 0x3E}, // O
    {0x7F, 0x09, 0x09, 0x09, 0x06}, // P
    {0x3E, 0x41, 0x51, 0x21, 0x5E}, // Q
    {0x7F, 0x09, 0x19, 0x29, 0x46}, // R
    {0x46, 0x49, 0x49, 0x49, 0x31}, // S
    {0x01, 0x01, 0x7F, 0x01, 0x01}, // T
    {0x3F, 0x40, 0x40, 0x40, 0x3F}, // U
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, // V
    {0x3F, 0x40, 0x38, 0x40, 0x3F}, // W
    {0x63, 0x14, 0x08, 0x14, 0x63}, // X
    {0x07, 0x08, 0x70, 0x08, 0x07}, // Y
    {0x61, 0x51, 0x49, 0x45, 0x43}, // Z (Index 37)
    {0x00, 0x60, 0x60, 0x00, 0x00}, // . (Index 38)
    {0x14, 0x08, 0x3E, 0x08, 0x14}, // * (Index 39)
    {0x08, 0x08, 0x08, 0x08, 0x08}  // - (Index 40)
};

// --- DELAY FUNCTION (For Boot Screen) ---
void delay_ms(unsigned int ms) {
    unsigned int i, j;
    for(i=0; i<ms; i++)
        for(j=0; j<114; j++); 
}

// --- I2C BIT-BANGING LOGIC ---
void I2C_Start() { SDA=1; SCL=1; _nop_(); SDA=0; _nop_(); SCL=0; }
void I2C_Stop()  { SDA=0; SCL=1; _nop_(); SDA=1; _nop_(); }
void I2C_Write(unsigned char dat) {
    unsigned char i;
    for(i=0; i<8; i++) {
        SDA = (dat & 0x80) ? 1 : 0;
        SCL = 1; _nop_(); SCL = 0;
        dat <<= 1;
    }
    SDA = 1; SCL = 1; _nop_(); SCL = 0;
}

void OLED_Cmd(unsigned char cmd) {
    I2C_Start(); I2C_Write(0x78); I2C_Write(0x00); I2C_Write(cmd); I2C_Stop();
}
void OLED_Data(unsigned char dat) {
    I2C_Start(); I2C_Write(0x78); I2C_Write(0x40); I2C_Write(dat); I2C_Stop();
}

void OLED_SetCursor(unsigned char page, unsigned char col) {
    OLED_Cmd(0xB0 + page);
    OLED_Cmd(0x00 + (col & 0x0F));
    OLED_Cmd(0x10 + ((col >> 4) & 0x0F));
}

void OLED_Clear() {
    unsigned char i, j;
    for(i=0; i<8; i++) {
        OLED_SetCursor(i, 0);
        for(j=0; j<128; j++) OLED_Data(0x00);
    }
}

// Custom Print Function with expanded ASCII map
void OLED_Print(unsigned char page, unsigned char col, char *str) {
    unsigned char i, idx;
    OLED_SetCursor(page, col);
    while(*str) {
        if(*str >= '0' && *str <= '9') idx = *str - '0';
        else if(*str == ' ') idx = 10;
        else if(*str == ':') idx = 11;
        else if(*str >= 'A' && *str <= 'Z') idx = *str - 'A' + 12;
        else if(*str == '.') idx = 38;
        else if(*str == '*') idx = 39;
        else if(*str == '-') idx = 40;
        else idx = 10;

        for(i=0; i<5; i++) OLED_Data(font[idx][i]);
        OLED_Data(0x00); 
        str++;
    }
}

void OLED_PrintNum(unsigned char page, unsigned char col, int num) {
    char buf[4];
    buf[0] = (num / 100) + '0';
    buf[1] = ((num / 10) % 10) + '0';
    buf[2] = (num % 10) + '0';
    buf[3] = '\0';
    
    if (buf[0] == '0') {
        buf[0] = ' ';
        if (buf[1] == '0') buf[1] = ' ';
    }
    OLED_Print(page, col, buf);
}

void OLED_Init() {
    OLED_Cmd(0xAE); 
    OLED_Cmd(0x20); OLED_Cmd(0x02); 
    OLED_Cmd(0x8D); OLED_Cmd(0x14); 
    OLED_Cmd(0xAF); 
    OLED_Clear();
}

// ---------------------------------------------------------
// HARDWARE INTERRUPTS
// ---------------------------------------------------------
void ext0_isr(void) interrupt 0 {
    if (entry_debounce == 0) {
        carCount++;
        entry_debounce = 1500; 
    }
}

void ext1_isr(void) interrupt 2 {
    if (exit_debounce == 0) {
        if (carCount > 0) carCount--;
        exit_debounce = 1500; 
    }
}

void timer0_isr(void) interrupt 1 {
    // PWM Logic
    pwm_count++;
    if (pwm_count >= 100) {
        pwm_count = 0;
        
        // Animation Logic Engine (Triggers without blocking sensors)
        anim_tick++;
        if (anim_tick >= 40) { // Controls the blinking speed
            anim_tick = 0;
            blink_flag = ~blink_flag;
        }
    }
    
    if (pwm_count < pwm_duty) STREET_LED = 0; 
    else STREET_LED = 1;                      
    
    if (entry_debounce > 0) entry_debounce--;
    if (exit_debounce > 0) exit_debounce--;
}

// ---------------------------------------------------------
// MAIN SYSTEM LOOP
// ---------------------------------------------------------
// ---------------------------------------------------------
// MAIN SYSTEM LOOP
// ---------------------------------------------------------
void main() {
    LDR = 1;
    STREET_LED = 1; 

    // Start OLED first
    OLED_Init();

    // --- THE CUSTOM BOOT SEQUENCE ---
    OLED_Print(2, 40, "WELCOME");
    OLED_Print(4, 34, "MOHITHIRA");
    delay_ms(3000); // Hold for 3 seconds
    
    OLED_Clear();
    OLED_Print(0, 0, "INITIALIZING...");
    delay_ms(800);
    OLED_Print(2, 0, "CHECKING LDR...");
    delay_ms(800);
    OLED_Print(4, 0, "CHECKING IRS...");
    delay_ms(800);
    OLED_Print(6, 0, "ROAD SECURE...");
    delay_ms(1500);
    OLED_Clear();

    // Start Timers and Interrupts AFTER boot screen finishes
    TMOD = 0x02; 
    TH0 = 0x9B;  
    TL0 = 0x9B;
    TR0 = 1;
    IT0 = 1; IT1 = 1; 
    EX0 = 1; EX1 = 1; 
    ET0 = 1; EA = 1;  

    // --- PERFECTLY SPACED STATIC UI ---
    OLED_Print(2, 10, "MODE:");         // Moved to Row 2
    OLED_Print(6, 10, "SYS: ACTIVE");   // Kept at Row 6

    while (1) {
        // --- NON-BLOCKING DASHBOARD ANIMATION ---
        if (blink_flag != last_blink_flag) {
            if (blink_flag) {
                OLED_Print(0, 10, "* SMART HIGHWAY *"); // Row 0
            } else {
                OLED_Print(0, 10, "- SMART HIGHWAY -"); // Row 0
            }
            last_blink_flag = blink_flag;
        }

        // --- SENSOR LOGIC ---
        if (LDR == 1) { 
            // DAYTIME
            pwm_duty = 0;       
            carCount = 0;  
            if (last_mode != 0) {
                OLED_Print(2, 45, "DAY  "); // Moved to Row 2
                last_mode = 0;
            }
        } else { 
            // NIGHTTIME
            if (last_mode != 1) {
                OLED_Print(2, 45, "NIGHT"); // Moved to Row 2
                last_mode = 1;
            }

            if (carCount > 0) {
                pwm_duty = 100; 
            } else { 
                pwm_duty = 20;  
            }
        }

        // Update Car Count Only When Changed
        if (carCount != last_carCount) {
            OLED_Print(4, 10, "CARS ON ROAD: "); // Moved to Row 4
            OLED_PrintNum(4, 95, carCount);      // Moved to Row 4
            last_carCount = carCount;
        }
    }
}