#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/watchdog.h"
#include "ws2812.pio.h"

#define IS_RGBW true  // Will use RGBW format
#define NUM_PIXELS 1  // There is 1 WS2812 device in the chain
#define WS2812_PIN 28 // The GPIO pin that the WS2812 connected to

// -------------------------------------- WS2812 RGB LED --------------------------------------

/**
 * @brief Wrapper function used to call the underlying PIO
 *        function that pushes the 32-bit RGB colour value
 *        out to the LED serially using the PIO0 block. The
 *        function does not return until all of the data has
 *        been written out.
 *
 * @param pixel_grb The 32-bit colour value generated by urgb_u32()
 */
static inline void put_pixel(uint32_t pixel_grb)
{
    pio_sm_put_blocking(pio0, 0, pixel_grb << 8u);
}

/**
 * @brief Function to generate an unsigned 32-bit composit GRB
 *        value by combining the individual 8-bit paramaters for
 *        red, green and blue together in the right order.
 *
 * @param r     The 8-bit intensity value for the red component
 * @param g     The 8-bit intensity value for the green component
 * @param b     The 8-bit intensity value for the blue component
 * @return uint32_t Returns the resulting composit 32-bit RGB value
 */
static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b)
{
    return ((uint32_t)(r) << 8) |
           ((uint32_t)(g) << 16) |
           (uint32_t)(b);
}

// ------------------------------ Declare Main Assembly Entry Before use ------------------------------

void main_asm();

// -------------------------------------- GPIO Pin Initialisation --------------------------------------

// Initialise GPIO pin
void asm_gpio_init(uint pin)
{
    gpio_init(pin);
}

// Set direction of a GPIO pin
void asm_gpio_set_dir(uint pin, bool out)
{
    gpio_set_dir(pin, out);
}

// Get the value of a GPIO pin
bool asm_gpio_get(uint pin)
{
    return gpio_get(pin);
}

// Set the value of a GPIO pin
void asm_gpio_put(uint pin, bool value)
{
    gpio_put(pin, value);
}

// Set the IRQ of a GPIO pin
void asm_gpio_set_irq(uint pin)
{
    gpio_set_irq_enabled(pin, GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(pin, GPIO_IRQ_EDGE_RISE, true);
}

// -------------------------------------- Button Timer --------------------------------------

// Get timestamp in milliseconds
int get_time_in_ms()
{
    absolute_time_t time = get_absolute_time();
    return to_ms_since_boot(time);
}

// Find time difference in milliseconds
int get_time_difference(int end, int start)
{
    return (end - start);
}

// -------------------------------------- Global Variables --------------------------------------

int currentLevel = 0;
int highestLevel = 0;
int selectLevel = 0;

int lives = 3;

int wins = 0;
int totalCorrectAnswers = 0;
int rightInput = 0;
int wrongInput = 0;
int remaining = 5;

char currentInput[20];
int i = 0; // i = length of input sequence
int tmpIndex = 0;
int inputComplete = 0;

// -------------------------------------- Letter Struct --------------------------------------

#define TABLE_SIZE 36

typedef struct morse
{
    char letter;
    char *code;
} morse;
morse table[TABLE_SIZE];

void morse_init()
{

    table[0].letter = 'A';
    table[1].letter = 'B';
    table[2].letter = 'C';
    table[3].letter = 'D';
    table[4].letter = 'E';
    table[5].letter = 'F';
    table[6].letter = 'G';
    table[7].letter = 'H';
    table[8].letter = 'I';
    table[9].letter = 'J';
    table[10].letter = 'K';
    table[11].letter = 'L';
    table[12].letter = 'M';
    table[13].letter = 'N';
    table[14].letter = 'O';
    table[15].letter = 'P';
    table[16].letter = 'Q';
    table[17].letter = 'R';
    table[18].letter = 'S';
    table[19].letter = 'T';
    table[20].letter = 'U';
    table[21].letter = 'V';
    table[22].letter = 'W';
    table[23].letter = 'X';
    table[24].letter = 'Y';
    table[25].letter = 'Z';
    table[26].letter = '0';
    table[27].letter = '1';
    table[28].letter = '2';
    table[29].letter = '3';
    table[30].letter = '4';
    table[31].letter = '5';
    table[32].letter = '6';
    table[33].letter = '7';
    table[34].letter = '8';
    table[35].letter = '9';

    table[0].code = ".-";
    table[1].code = "-...";
    table[2].code = "-.-.";
    table[3].code = "-..";
    table[4].code = ".";
    table[5].code = "..-.";
    table[6].code = "--.";
    table[7].code = "....";
    table[8].code = "..";
    table[9].code = ".---";
    table[10].code = "-.-";
    table[11].code = ".-..";
    table[12].code = "--";
    table[13].code = "-.";
    table[14].code = "---";
    table[15].code = ".--.";
    table[16].code = "--.-";
    table[17].code = ".-.";
    table[18].code = "...";
    table[19].code = "-";
    table[20].code = "..-";
    table[21].code = "...-";
    table[22].code = ".--";
    table[23].code = "-..-";
    table[24].code = "-.--";
    table[25].code = "--..";
    table[26].code = "-----";
    table[27].code = ".----";
    table[28].code = "..---";
    table[29].code = "...--";
    table[30].code = "....-";
    table[31].code = ".....";
    table[32].code = "-....";
    table[33].code = "--...";
    table[34].code = "---..";
    table[35].code = "----.";
}

// -------------------------------------- Start Game --------------------------------------
void start_game()
{
    put_pixel(urgb_u32(0x00, 0x3F, 0x00)); // Set the RGB LED color to green
    rightInput = 0;
    lives = 3;
    remaining = 5;
    wrongInput = 0;
}

// -------------------------------------- Select Level --------------------------------------

void level_select_true()
{
    selectLevel = 1;
}

void level_select_false()
{
    selectLevel = 0;
}

void select_difficulty()
{
    while (true)
    {
        if (strcmp(currentInput, ".----") == 0)
        {
            currentLevel = 1;
            return;
        }
        else if (strcmp(currentInput, "..---") == 0)
        {
            currentLevel = 2;
            return;
        }
        else if (strcmp(currentInput, "...--") == 0)
        {
            currentLevel = 3;
            return;
        }
        else if (strcmp(currentInput, "...--") == 0)
        {
            currentLevel = 4;
            return;
        }
        else
        {
            printf("Error: Invalid input.");
            return;
        }
    }
}

int select_random(int low, int high)
{
    if (low > high)
        return high;
    return low + (rand() % (high - low + 1));
}

// Levels

// -------------------------------------- Inputs --------------------------------------

void add_input(int input_type)
{
    // ToDo
}

// -------------------------------------- Display Message --------------------------------------

void welcome()
{
    printf("__        _______ _     ____ ___  __  __ _____ \n");
    printf("\\ \\      / / ____| |   / ___/ _ \\|  \\/  | ____| \n");
    printf(" \\ \\ /\\ / /|  _| | |  | |  | | | | |\\/| |  _|  \n");
    printf("  \\ V  V / | |___| |__| |__| |_| | |  | | |___  \n");
    printf("   \\_/\\_/  |_____|_____\\____\\___/|_|  |_|_____| \n");
    printf("  ____ ____   ___  _   _ ____    _________  \n");
    printf(" / ___|  _ \\ / _ \\| | | |  _ \\  |___ /___ \\ \n");
    printf("| |  _| |_) | | | | | | | |_) |   |_ \\ __) | \n");
    printf("| |_| |  _ <| |_| | |_| |  __/   ___) / __/ \n");
    printf(" \\____|_|\\_  \\___/ \\___/|_|     |____/_____| \n");

    printf("\n       WELCOME TO OUR MORSE CODE GAME!        \n");
    printf("       PRESS THE GPIO PIN 21 TO CONTINUE        \n");
}

void instructions()
{
    printf("\n                 HOW TO PLAY\n");
    printf("You must enter the correct morse code sequence \n");
    printf("There are 4 levels in total - each level is 5 rounds!\n");
    printf("You have 3 lives before the game is over. \n");
    printf("\n");
    printf("1. For a dot (.), Hold down GPIO PIN 21 <0.25s \n");
    printf("2. For a dash (-), Hold down GPIO PIN 21 for >0.25s \n");
    printf("3. For a space, Leave the button unpressed for 1s \n");
    printf("4. To submit, Leave the button unpressed for 2s \n");
    printf("\n");
}

void difficulty_level_inputs()
{
    printf("\n\n\t*****************************\n");
    printf("\t*                           *\n");
    printf("\t* Enter .---- for Level 1   *\n");
    printf("\t* Enter ..--- for Level 2   *\n");
    printf("\t* Enter ...-- for Level 3   *\n");
    printf("\t* Enter ....- for Level 4   *\n");
    printf("\t*****************************\n");
}

// -------------------------------------- LED --------------------------------------

void set_corrrect_led()
{
    if (currentLevel != 0)
    {
        switch (lives)
        {
        case 3:
            // Set Green
            put_pixel(urgb_u32(0x0, 0x3F, 0x0));
            break;
        case 2:
            // Set Yellow
            put_pixel(urgb_u32(0x3F, 0x3F, 0x0));
            break;
        case 1:
            // Set Red
            put_pixel(urgb_u32(0x3F, 0x0, 0x0));
            break;
        default:
            break;
        }
    }
    else
    {
        // Set Blue
        put_pixel(urgb_u32(0x0, 0x0, 0x3F));
    }
}

void set_red_on()
{
    put_pixel(urgb_u32(0x3F, 0x0, 0x0));
}

// -------------------------------------- Game Logic --------------------------------------

/**
 * @brief A function called upon in start the game function that
 * calculates your overall accuracy throughout the game by
 * summing your total attempts over the total correct attempts.
 */
void calculate_stats(int reset)
{
    printf("\n\n\t\t***************STATS***************\n\n");
    printf("\n\t\t*Attempts: \t\t\t\t%d*", rightInput + wrongInput);
    printf("\n\t\t*Correct: \t\t\t\t%d*", rightInput);
    printf("\n\t\t*Incorrect: \t\t\t\t%d*", wrongInput);
    printf("\n\t\t*Accuracy: \t\t\t\t%.2f%%*", (float)rightInput / (rightInput + wrongInput) * 100);
    printf("\n\t\t*Win Streak: \t\t\t\t%d*", wins);
    printf("\n\t\t*Lives Left: \t\t\t\t%d*", lives);
    if (rightInput != 0 || wrongInput != 0)
    {
        float stat = rightInput / (rightInput + wrongInput) * 100;
        if (reset)
        {
            rightInput = 0;
            wrongInput = 0;
            printf("\t\t*Correct %% for this level: \t%.2f%%*\n", stat);
        }
        else
        {
            printf("\t\t*Correct Percent :\t\t\t%.2f%%*\n", stat);
        }
    }
    printf("\n\t\t**********************************\n\n");
}

void game_finished()
{
    calculate_stats(1);
    if (lives == 0)
        set_red_on();
    printf("\n\n\n\n\n\n\t\t*****************************\n");
    printf("\t\t*                           *\n");
    printf("\t\t* Enter .---- to play again *\n");
    printf("\t\t* Enter ..--- to exit       *\n");
    printf("\t\t*****************************\n");
    main_asm();
    if (strcmp(currentInput, ".----") == 0)
    {
        start_game();
    }
}

/**
 * @brief EXAMPLE - WS2812_RGB
 *        Simple example to initialise the NeoPixel RGB LED on
 *        the MAKER-PI-PICO and then flash it in alternating
 *        colours between red, green and blue forever using
 *        one of the RP2040 built-in PIO controllers.
 *
 * @return int  Application return code (zero for success).
 */
int main()
{
    // Initialise all STDIO as we will be using the GPIOs
    stdio_init_all();
    morse_init();

    // Initialise the PIO interface with the WS2812 code
    PIO pio = pio0;
    uint offset = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, 0, offset, WS2812_PIN, 800000, IS_RGBW);
    watchdog_enable(0x7fffff, 1); // Watchdog Enables to Max Timeout

    welcome();
    instructions();
    difficulty_level_inputs();
    main_asm();
    printf("\n\n\n");

    return (0);
}