**Modules:**
Module prefix allways uppercase
e.g.: ADC, GPIO, UI

**Variables:**

| Scope   | Format           | Example     |
|---------|------------------|-------------|
| stack   | low_snake_case   | temp_rx_str |
| file    | camelCase        | secTickFlag |
| public  | MODULE_camelCase | RTC_upTime  |

**Functions:**

| Scope    | Format           | Example         |
|----------|------------------|-----------------|
| internal | PascalCase       | CalcSysTime()   |
| public   | MODULE_PamelCase | UART_RxString() |

**Defines:**
All upper snakecase
e.g: UART_BAUD_RATE, RAD_CONV_FACTOR

**Typedefs:**
Modul prefix, underscore, pascalcase, '_t' appended
e.g: GPIO_PinStruct_t, RAD_AlarmRate_t;

**Conditional statements:**
The following style is only allowed if bothcontrolled statements are single lines:

    if (condition) { singleLineOfCode(); }
    else { anotherSingleLine(); }

If one controlled statement does not fit in a single line, this style should be used:

    if (condition)
    {
        firstLine();
        secondLineRequired();
    }
    else
    {
        singleLine();
    }

**Order of file contents**
+ header
+ includes
+ defines
+ macros
+ internal variables
+ public variables
+ internal function declarations
+ public function definitions
+ internal function definitions
+ ISRs
