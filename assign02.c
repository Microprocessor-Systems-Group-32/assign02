// Must declare the main assembly entry point before use.
void main_asm();

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
}

struct letter
{
    char letter;      // char representation of letter
    char *morse_code; // morse code representation of letter, in binary string format
};

// Creates an instance of letter.
struct letter letter_create(char letter, char *morse_code)
{
    struct letter output;
    output.letter = letter;
    output.morse_code = morse_code;
    return output;
}

struct letter letter_array[37];
void letter_array_create()
{
    letter_array[0] = letter_create('A', ".-");
    letter_array[1] = letter_create('B', "-...");
    letter_array[2] = letter_create('C', "-.-.");
    letter_array[3] = letter_create('D', "-..");
    letter_array[4] = letter_create('E', ".");
    letter_array[5] = letter_create('F', "..-.");
    letter_array[6] = letter_create('G', "--.");
    letter_array[7] = letter_create('H', "....");
    letter_array[8] = letter_create('I', "..");
    letter_array[9] = letter_create('J', ".---");
    letter_array[10] = letter_create('K', "-.-");
    letter_array[11] = letter_create('L', ".-..");
    letter_array[12] = letter_create('M', "--");
    letter_array[13] = letter_create('N', "-.");
    letter_array[14] = letter_create('O', "---");
    letter_array[15] = letter_create('P', ".--.");
    letter_array[16] = letter_create('Q', "--.-");
    letter_array[17] = letter_create('R', ".-.");
    letter_array[18] = letter_create('S', "...");
    letter_array[19] = letter_create('T', "-");
    letter_array[20] = letter_create('U', "..-");
    letter_array[21] = letter_create('V', "...-");
    letter_array[22] = letter_create('W', ".--");
    letter_array[23] = letter_create('X', "-..-");
    letter_array[24] = letter_create('Y', "-.--");
    letter_array[25] = letter_create('Z', "--..");
    letter_array[26] = letter_create('0', "-----");
    letter_array[27] = letter_create('1', ".----");
    letter_array[28] = letter_create('2', "..---");
    letter_array[29] = letter_create('3', "...--");
    letter_array[30] = letter_create('4', "....-");
    letter_array[31] = letter_create('5', ".....");
    letter_array[32] = letter_create('6', "-....");
    letter_array[33] = letter_create('7', "--...");
    letter_array[34] = letter_create('8', "---..");
    letter_array[35] = letter_create('9', "----.");
    letter_array[36] = letter_create('?', "..--..");
}

/*
 * Main entry point for the code - simply calls the main assembly function.
 */
int main()
{
    // Initialise all STDIO as we will be using the GPIOs
    stdio_init_all();

    // Initialise the array of letters
    letter_array_create();

    while (true)
    {
        // Call the main assembly function
        main_asm();
    }

    return (0);
}
