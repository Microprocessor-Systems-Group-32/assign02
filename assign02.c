#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/watchdog.h"
#include "ws2812.pio.h"

#define IS_RGBW true  // Will use RGBW format
#define NUM_PIXELS 1  // There is 1 WS2812 device in the chain
#define WS2812_PIN 28 // The GPIO pin that the WS2812 connected to

// -------------------------------------- Global Variables --------------------------------------

/**
 * @file assign02.c
 * @brief This file contains the variables used in the game.
 */

int initial_round = 1; // Use this to only print instructions once on initial round
int current_level = 0; // 0 = Level select, 1 - 4 = level X
int highest_level = 0;

int quit = 0;

int lives = 3;

int wins = 0;
int total_correct_answers = 0;
int right_input = 0;
int wrong_input = 0;
int remaining = 5;

char current_input[100];
int current_input_length = 0; // length of input sequence
int char_to_solve = 0;        // The index (in the table) of the current character that the player needs to solve
int input_complete = 0;

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

// -------------------------------------- Watchdog Timer --------------------------------------

/**
 * @brief Updates the watchdog to prevent chip reset
 */
void arm_watchdog_update()
{
    watchdog_update();
}

// -------------------------------------- Letter Struct --------------------------------------

/*
 * Initializes the morse code table.
 * The table contains 36 entries, each representing a letter or a digit.
 * Each entry consists of a letter or a digit and its corresponding Morse code.
 */
#define TABLE_SIZE 36

typedef struct morse
{
    char letter;
    char *code;
} morse;
morse table[TABLE_SIZE];

void morse_init()
{
    // Initialize the table with letters A-Z and digits 0-9
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

    // Initialize the Morse code for each letter or digit
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

// -------------------------------------- Word Struct --------------------------------------

/*
 * Initializes the morse code table for words in optional levels 3 & 4.
 * The table contains 25 entries, each representing a lrandom word.
 * Each entry consists of the word and its corresponding Morse code.
 */
#define TABLE_SIZE_WORD 25

typedef struct wordMorse
{
    char *word;
    char *code;
} wordMorse;
wordMorse wTable[TABLE_SIZE_WORD];

void word_morse_init()
{
    // Initialize the table with random words for the questions
    wTable[0].word = "justin";
    wTable[1].word = "conor";
    wTable[2].word = "hannah";
    wTable[3].word = "surya";
    wTable[4].word = "brian";
    wTable[5].word = "apple";
    wTable[6].word = "banana";
    wTable[7].word = "shoe";
    wTable[8].word = "hat";
    wTable[9].word = "fish";
    wTable[10].word = "bird";
    wTable[11].word = "door";
    wTable[12].word = "table";
    wTable[13].word = "lamp";
    wTable[14].word = "spoon";
    wTable[15].word = "chair";
    wTable[16].word = "sun";
    wTable[17].word = "bicycle";
    wTable[18].word = "ocean";
    wTable[19].word = "compass";
    wTable[20].word = "umbrella";
    wTable[21].word = "volcano";
    wTable[22].word = "computer";
    wTable[23].word = "system";
    wTable[24].word = "game";

    // Initialize the Morse code for each letter or digit
    wTable[0].code = ".--- ..- ... - .. -.";
    wTable[1].code = "-.-. --- -. --- .-.";
    wTable[2].code = ".... .- -. -. .- ....";
    wTable[3].code = "... ..- .-. -.-- .-";
    wTable[4].code = "-... .-. .. .- -.";
    wTable[5].code = ".- .--. .--. .-.. .";
    wTable[6].code = "-... .- -. .- -. .-";
    wTable[7].code = "... .... --- .";
    wTable[8].code = ".... .- -";
    wTable[9].code = "..-. .. ... ....";
    wTable[10].code = "-... .. .-. -..";
    wTable[11].code = "-.. --- --- .-.";
    wTable[12].code = "- .- -... .-.. .";
    wTable[13].code = ".-.. .- -- .--.";
    wTable[14].code = "... .--. --- --- -.";
    wTable[15].code = "-.-. .... .- .. .-.";
    wTable[16].code = "... ..- -.";
    wTable[17].code = "-... .. -.-. -.-- -.-. .-.. .";
    wTable[18].code = "--- -.-. . .- -.";
    wTable[19].code = "-.-. --- -- .--. .- ... ...";
    wTable[20].code = "..- -- -... .-. . .-.. .-.. .-";
    wTable[21].code = "...- --- .-.. -.-. .- -. ---";
    wTable[22].code = "-.-. --- -- .--. ..- - . .-.";
    wTable[23].code = "... -.-- ... - . --";
    wTable[24].code = "--. .- -- .";
}

// -------------------------------------- Select Level --------------------------------------

/**
 * Generates a random integer between the given low and high values (inclusive).
 *
 * @param low The lower bound of the range.
 * @param high The upper bound of the range.
 * @return A random integer between low and high.
 */
int select_random(int low, int high)
{
    return rand() % (high - low + 1) + low;
}

/**
 * Executes level 1 of the game.
 * The player is presented with a series of Morse code challenges and must input the correct answers.
 * The level continues until the player runs out of lives or answers 5 questions correctly in a row.
 * At the end of the level, the outcome (win or lose) is displayed.
 */
void level_1()
{
    printf("-----------\n");
    printf("| Level 1 |\n");
    printf("-----------\n\n");

    while (remaining > 0 && lives > 0)
    {
        srand(time(NULL));
        char_to_solve = select_random(0, 35);
        printf("-----------------------------------------\n");
        printf("|\tEnter %c = %s in Morse Code\t|\n", table[char_to_solve].letter, table[char_to_solve].code);
        printf("-----------------------------------------\n");
        while (input_complete == 0)
        {
            // Empty while to stall the game until the timer interrupt fires
        }
        input_complete = 0;
        check_input();
    }

    // At the end of Level 1, check if we finished due to running out of lives
    // or due to getting 5 answers in a row correct
    if (lives == 0)
    {
        // Ran out of lives
        printf("YOU LOSE!!!\n");
    }
    else
    {
        // Completed 5 corret questions
        printf("YOU WIN!!!\n");
        wins++;
    }
    set_blue_led();
    game_finished();
}

/**
 * Executes level 2 of the game.
 * The player is presented with a series of Morse code challenges and must input the correct answers.
 * The level continues until the player runs out of lives or answers 5 questions correctly in a row.
 * At the end of the level, the outcome (win or lose) is displayed.
 */
void level_2()
{
    printf("-----------\n");
    printf("| Level 2 |\n");
    printf("-----------\n\n");

    while (remaining > 0 && lives > 0)
    {
        srand(time(NULL));
        char_to_solve = select_random(0, 35);
        printf("---------------------------------\n");
        printf("|\tEnter %c in Morse Code\t|\n", table[char_to_solve].letter);
        printf("---------------------------------\n");
        while (input_complete == 0)
        {
            // Empty while to stall the game until the timer interrupt fires
        }
        input_complete = 0;
        check_input();
    }

    // At the end of Level 2, check if we finished due to running out of lives
    // or due to getting 5 answers in a row correct
    if (lives == 0)
    {
        // Ran out of lives
        printf("YOU LOSE!!!\n");
    }
    else
    {
        // Completed 5 corret questions
        printf("YOU WIN!!!\n");
        wins++;
    }
    game_finished();
}

/**
 * Executes level 3 of the game.
 * The player is presented with a series of words (along woth morse values) as Morse code challenges and must input the correct answers.
 * The level continues until the player runs out of lives or answers 5 questions correctly in a row.
 * At the end of the level, the outcome (win or lose) is displayed.
 */

void level_3()
{
    printf("---------------------------------------------------------\n");
    printf("|                        Level 3                        |\n");
    printf("| Please enter a space between each letter of the word  |\n");
    printf("|    Wait for one second after inputting for a space    |\n");
    printf("---------------------------------------------------------\n\n");

    while (remaining > 0 && lives > 0)
    {
        srand(time(NULL));
        char_to_solve = select_random(0, 24);
        if(strlen(wTable[char_to_solve].word) < 4){
            printf("-------------------------------------------------\n");
            printf("|\tEnter %s = %s in Morse Code\t|\n", wTable[char_to_solve].word, wTable[char_to_solve].code);
            printf("-------------------------------------------------\n");
        }
        else if(strlen(wTable[char_to_solve].word) < 6){
            printf("---------------------------------------------------------\n");
            printf("|\tEnter %s = %s in Morse Code\t|\n", wTable[char_to_solve].word, wTable[char_to_solve].code);
            printf("---------------------------------------------------------\n");
        } 
        else if(strlen(wTable[char_to_solve].word) < 8){
            printf("-----------------------------------------------------------------\n");
            printf("|\tEnter %s = %s in Morse Code\t|\n", wTable[char_to_solve].word, wTable[char_to_solve].code);
            printf("-----------------------------------------------------------------\n");
        }
        else{
            printf("-------------------------------------------------------------------------\n");
            printf("|\tEnter %s = %s in Morse Code\t|\n", wTable[char_to_solve].word, wTable[char_to_solve].code);
            printf("-------------------------------------------------------------------------\n");
        }
        
        while (input_complete == 0)
        {
            // Empty while to stall the game until the timer interrupt fires
        }
        input_complete = 0;
        check_input();
    }

    // At the end of Level 3, check if we finished due to running out of lives
    // or due to getting 5 answers in a row correct
    if (lives == 0)
    {
        // Ran out of lives
        printf("YOU LOSE!!!\n");
    }
    else
    {
        // Completed 5 corret questions
        printf("YOU WIN!!!\n");
        wins++;
    }
    set_blue_led();
    game_finished();
}

/**
 * Executes level 4 of the game.
 * The player is presented with a series of words as Morse code challenges and must input the correct answers.
 * The level continues until the player runs out of lives or answers 5 questions correctly in a row.
 * At the end of the level, the outcome (win or lose) is displayed.
 */

void level_4()
{
    printf("---------------------------------------------------------\n");
    printf("|                        Level 4                        |\n");
    printf("| Please enter a space between each letter of the word  |\n");
    printf("|    Wait for one second after inputting for a space    |\n");
    printf("---------------------------------------------------------\n\n");

    while (remaining > 0 && lives > 0)
    {
        srand(time(NULL));
        char_to_solve = select_random(0, 24);
        if(strlen(wTable[char_to_solve].word) < 4){
            printf("---------------------------------\n");
            printf("|\tEnter %s in Morse Code\t|\n", wTable[char_to_solve].word);
            printf("---------------------------------\n");
        }
        else{
            printf("-----------------------------------------\n");
            printf("|\tEnter %s in Morse Code\t|\n", wTable[char_to_solve].word);
            printf("-----------------------------------------\n");
        }

        while (input_complete == 0)
        {
            // Empty while to stall the game until the timer interrupt fires
        }
        input_complete = 0;
        check_input();
    }

    // At the end of Level 4, check if we finished due to running out of lives
    // or due to getting 5 answers in a row correct
    if (lives == 0)
    {
        // Ran out of lives
        printf("YOU LOSE!!!\n");
    }
    else
    {
        // Completed 5 corret questions
        printf("  ____    _    __  __ _____    ____ ___  __  __ ____  _     _____ _____ _____ \n ");
        printf("/ ___|  / \\  |  \\/  | ____|  / ___/ _ \\|  \\/  |  _ \\| |   | ____|_   _| ____|\n");
        printf("| |  _  / _ \\ | |\\/| |  _|   | |  | | | | |\\/| | |_) | |   |  _|   | | |  _| \n");
        printf("| |_| |/ ___ \\| |  | | |___  | |__| |_| | |  | |  __/| |___| |___  | | | |___ \n");
        printf(" \\____/_/   \\_\\_|  |_|_____|  \\____\\___/|_|  |_|_|   |_____|_____| |_| |_____|\n\n");
        wins++;
    }
    set_blue_led();
    game_finished();
}

// -------------------------------------- Inputs --------------------------------------

/**
 * @brief Adds an input character to the current input based on the input type.
 *
 * @param input_type The type of input character to add.
 *                   0 - Dot
 *                   1 - Dash
 *                   2 - Space (only if current_level > 2)
 *                   3 - End Of Line (EOL)
 */

void add_input(int input_type)
{
    if (current_input_length == 100)
        return;

    switch (input_type)
    {
    case 0:
    {
        // Dot
        current_input[current_input_length] = '.';
        printf(".");
        current_input_length++;
        break;
    }
    case 1:
    {
        // Dash
        current_input[current_input_length] = '-';
        printf("-");
        current_input_length++;
        break;
    }
    case 2:
    {
        if (current_level > 2)
        {

            // Space
            current_input[current_input_length] = ' ';
            printf(" ");
            current_input_length++;
        }
        break;
    }
    case 3:
    {
        // End Of Line (EOL)
        if ((current_level == 3) || (current_level == 4))
        {
            current_input[current_input_length - 1] = '\0';
        }
        else
        {
            current_input[current_input_length] = '\0';
        }

        printf("\n");
        current_input_length++;
        input_complete = 1;
        break;
    }
    default:
        break;
    }
}

// -------------------------------------- Display Message --------------------------------------

/**
 * Displays a welcome message and instructions for the Morse code game.
 */
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

/**
 * Displays instructions on how to play the Morse code game.
 */
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

/**
 * Displays the difficulty level inputs for the Morse code game.
 */
void difficulty_level_inputs()
{
    printf("\n\n\t*****************************\n");
    printf("\t*                           *\n");
    printf("\t* Enter .---- for Level 1   *\n");
    printf("\t* Enter ..--- for Level 2   *\n");
    printf("\t* Enter ...-- for Level 3   *\n");
    printf("\t* Enter ....- for Level 4   *\n");
    printf("\t*                           *\n");
    printf("\t* Enter ..... to exit       *\n");
    printf("\t*                           *\n");
    printf("\t*****************************\n");
}

// -------------------------------------- LED --------------------------------------

/**
 * Sets the correct LED color based on the current level and number of lives.
 * If the current level is not 0, the LED color is determined by the number of lives:
 * - 3 lives: Green
 * - 2 lives: Yellow
 * - 1 life: Red
 * If the current level is 0, the LED color is set to Blue.
 */
void set_correct_led()
{
    if (current_level != 0)
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

/**
 * Sets the LED color to Red.
 */
void set_red_led()
{
    put_pixel(urgb_u32(0x3F, 0x0, 0x0));
}

/**
 * Sets the LED color to Blue.
 */
void set_blue_led()
{
    put_pixel(urgb_u32(0x0, 0x0, 0x3F));
}

// -------------------------------------- Game Logic --------------------------------------

/**
 * @brief Clears the current input, making it ready for a new round
 */
void clear_input()
{
    for (int i = 0; i < 100; i++)
        current_input[i] = 0;
    current_input_length = 0;
}

/**
 * @brief Handles the level select, based on the current input
 */
void select_difficulty()
{
    if (strcmp(current_input, ".----") == 0)
    {
        current_level = 1;
        // Turns Green to signify game in progress
        set_correct_led();
        // level_1();
        return;
    }
    else if (strcmp(current_input, "..---") == 0)
    {
        current_level = 2;
        set_correct_led();
        return;
    }
    else if (strcmp(current_input, "...--") == 0)
    {
        current_level = 3;
        set_correct_led();
        return;
    }
    else if (strcmp(current_input, "....-") == 0)
    {
        current_level = 4;
        set_correct_led();
        return;
    }
    else if (strcmp(current_input, ".....") == 0)
    {
        quit = 1;
        return;
    }
    else
    {
        printf("Error: Invalid input.");
        return;
    }
}

/**
 * @brief Based on the current level, check if the input is valid and progress as necessary
 */
void check_input()
{
    // Handle for level select
    if (current_level == 0)
    {
        select_difficulty();
    }
    else if (current_level == 1)
    {
        // Level 1
        if (strcmp(current_input, table[char_to_solve].code) == 0)
        {
            set_correct_led();
            remaining--;
            printf("\nCORRECT!\n\n");
            printf("Remaining: %d\n", remaining);
            printf("Lives: %d\n\n\n", lives);
            right_input++;
        }
        else
        {
            lives--;
            set_correct_led();
            printf("\nWRONG! :((\n\n");
            int present = 0;
            int i;
            for(i = 0; i<36; i++){
                if (strcmp(current_input, table[i].code) == 0)
                {
                    printf("Inputted Values is: %c\n", table[i].letter);
                    present = 1;
                }
            }
            if(present != 1){
                printf("Inputted Values is: ?\n");
            }
            
            remaining = 5;
            printf("Remaining back to: %d\n", remaining);
            printf("Lives: %d\n\n\n", lives);
            wrong_input++;
        }
    }
    else if (current_level == 2)
    {
        // Level 2
        if (strcmp(current_input, table[char_to_solve].code) == 0)
        {
            set_correct_led();
            remaining--;
            printf("\nCORRECT!\n\n");
            printf("Remaining: %d\n", remaining);
            printf("Lives: %d\n\n\n", lives);
            right_input++;
        }
        else
        {
            lives--;
            set_correct_led();
            printf("\nWRONG! :((\n\n");
            int present = 0;
            int i;
            for(i = 0; i<36; i++){
                if (strcmp(current_input, table[i].code) == 0)
                {
                    printf("Inputted Values is: %c\n", table[i].letter);
                    present = 1;
                }
            }
            if(present != 1){
                printf("Inputted Values is: ?\n");
            }
            
            printf("%c in Morse is: %s\n", table[char_to_solve].letter, table[char_to_solve].code);
            remaining = 5;
            printf("Remaining back to: %d\n", remaining);
            printf("Lives: %d\n\n\n", lives);
            wrong_input++;
        }
    }
    else if (current_level == 3)
    {
        // Level 3
        if (strcmp(current_input, wTable[char_to_solve].code) == 0)
        {
            set_correct_led();
            remaining--;
            printf("\nCORRECT!\n\n");
            printf("Remaining: %d\n", remaining);
            printf("Lives: %d\n\n\n", lives);
            right_input++;
        }
        else
        {
            lives--;
            set_correct_led();
            printf("\nWRONG! :((\n\n");
            char delm[2]=" ";
            char *token;
            token = strtok(current_input, delm);
            int wrong = 0;
            printf("Inputted Value is: ");

            while(token != NULL)
            {
                int present = 0;
                
                int i;
                for(i = 0; i<36; i++){
                    if (strcmp(token, table[i].code) == 0)
                    {
                        //strcat(tempVal, table[i].letter);
                        printf("%c", table[i].letter);
                        //printf("Inputted Values is: %c\n", table[i].letter);
                        present = 1;
                    }
                }
                if(present != 1){
                    wrong = 1;
                }

                token = strtok(NULL, delm);
            }
            
            if(wrong == 1){
                printf("?\n");
            }
            else{
                printf("\n");
            }
            
            remaining = 5;
            printf("Remaining back to: %d\n", remaining);
            printf("Lives: %d\n\n\n", lives);
            wrong_input++;
        }
    }
    else if (current_level == 4)
    {
        // Level 4
        if (strcmp(current_input, wTable[char_to_solve].code) == 0)
        {
            set_correct_led();
            remaining--;
            printf("\nCORRECT!\n\n");
            printf("Remaining: %d\n", remaining);
            printf("Lives: %d\n\n\n", lives);
            right_input++;
        }
        else
        {
            lives--;
            set_correct_led();
            printf("\nWRONG! :((\n\n");
            char delm[2]=" ";
            char *token;
            token = strtok(current_input, delm);
            int wrong = 0;
            printf("Inputted Value is: ");

            while(token != NULL)
            {
                int present = 0;
                
                int i;
                for(i = 0; i<36; i++){
                    if (strcmp(token, table[i].code) == 0)
                    {
                        //strcat(tempVal, table[i].letter);
                        printf("%c", table[i].letter);
                        //printf("Inputted Values is: %c\n", table[i].letter);
                        present = 1;
                    }
                }
                if(present != 1){
                    wrong = 1;
                }

                token = strtok(NULL, delm);
            }
            
            if(wrong == 1){
                printf("?\n");
            }
            else{
                printf("\n");
            }
            
            printf("%s in Morse is: %s\n", wTable[char_to_solve].word, wTable[char_to_solve].code);
            remaining = 5;
            printf("Remaining back to: %d\n", remaining);
            printf("Lives: %d\n\n\n", lives);
            wrong_input++;
        }
    }
    else
    {
        printf("ERROR");
    }

    clear_input();
}

/**
 * @brief Resets the game for a new round
 */
void reset_game()
{
    right_input = 0;
    lives = 3;
    remaining = 5;
    wrong_input = 0;
    current_level = 0;
    input_complete = 0;
    set_blue_led();
}

/**
 * @brief Starts playing the game and doesn't quit out of this routinen
 * until the player specifies the 'quit' character sequence
 */
void start_game()
{
    // put_pixel(urgb_u32(0x00, 0x3F, 0x00)); // Set the RGB LED color to green

    while (quit == 0)
    {
        // Dont print instructions again if it's the initial round
        if (initial_round)
            initial_round = 0;
        else
            difficulty_level_inputs();

        // Stall for level select
        while (input_complete == 0)
        {
        }

        // Check for level
        check_input();
        input_complete = 0;

        if (current_level == 1)
        {
            level_1();
        }
        else if (current_level == 2)
        {
            level_2();
        }
        else if (current_level == 3)
        {
            level_3();
        }
        else if (current_level == 4)
        {
            level_4();
        }

        reset_game();
        clear_input();
    }

    printf("GOODBYE :(\n");
}

/**
 * @brief A function called upon in start the game function that
 * calculates your overall accuracy throughout the game by
 * summing your total attempts over the total correct attempts.
 */
void calculate_stats(int reset)
{
    printf("\n\n********************* STATS *********************");
    printf("\n*\t\t\t\t\t\t*");
    printf("\n*\tAttempts: \t\t\t%d\t*", right_input + wrong_input);
    printf("\n*\tCorrect: \t\t\t%d\t*", right_input);
    printf("\n*\tIncorrect: \t\t\t%d\t*", wrong_input);
    printf("\n*\tAccuracy: \t\t\t%.2f%%\t*", (float)right_input / (right_input + wrong_input) * 100);
    printf("\n*\tWin Streak: \t\t\t%d\t*", wins);
    printf("\n*\tLives Left: \t\t\t%d\t*", lives);
    if (right_input != 0 || wrong_input != 0)
    {
        float stat = (float)right_input / (right_input + wrong_input) * 100;
        if (reset)
        {
            right_input = 0;
            wrong_input = 0;
            printf("\n*\tCorrect %% for this level: \t%.2f%%\t*", stat);
        }
        else
        {
            printf("\n*\tCorrect Percent :\t\t\t%.2f%%\t*", stat);
        }
    }
    printf("\n*\t\t\t\t\t\t*");
    printf("\n*************************************************\n\n");
}

/**
 * Function to handle game completion.
 * Calculates game statistics, displays game over message, and prompts for replay or exit.
 */
void game_finished()
{
    calculate_stats(1);
    if (lives == 0)
        set_red_led();
    printf("\n\n\n\n\n\n\t*****************************\n");
    printf("\t*                           *\n");
    printf("\t* Enter .---- to play again *\n");
    printf("\t* Enter ..--- to exit       *\n");
    printf("\t*****************************\n\n\n");
    // main_asm();
    clear_input();
    while (input_complete == 0)
    {
    }
    if (strcmp(current_input, ".----") == 0)
    {
    }
    else if (strcmp(current_input, "..---") == 0)
    {
        quit = 1;
    }
    else
    {
        printf("Error: Invalid input.");
    }
}

// -------------------------------------- Main --------------------------------------

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
    srand(time(NULL));
    // Initialise all STDIO as we will be using the GPIOs
    stdio_init_all();
    morse_init();
    word_morse_init();

    // Initialise the PIO interface with the WS2812 code
    PIO pio = pio0;
    uint offset = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, 0, offset, WS2812_PIN, 800000, IS_RGBW);
    watchdog_enable(0x7fffff, 1); // Watchdog Enables to Max Timeout

    welcome();
    instructions();
    difficulty_level_inputs();
    set_blue_led();

    main_asm();

    start_game();

    printf("\n\n\n");

    return (0);
}