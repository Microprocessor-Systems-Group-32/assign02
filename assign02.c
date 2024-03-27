// Must declare the main assembly entry point before use.
void main_asm();

/*
 * Main entry point for the code - simply calls the main assembly function.
 */
int main()
{
    int test_var_a = 1;
    int test_var_b = 5;
    test_var_a += test_var_b;
    main_asm();
    return (0);
}
