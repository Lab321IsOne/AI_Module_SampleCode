/** InstAI Co. (Public Version)
    Description: This is the sample code for complete program guide (OD/S-Motion OD/S_MOTION_JPEG_OD) for InstAI C-series AI Module
    Modified Date: Feb 25, 2023
*/

#include "ai_module.h"

#ifdef PLATFORM_RASPI
    // define pin number of CS, RST connected to your host
    #define PIN_CS RPI_BPLUS_GPIO_J8_22    // set the CS Pin 22 (GPIO 25)
    #define PIN_RST RPI_BPLUS_GPIO_J8_18   // set the RESET Pin 18 (GPIO 24)
    // define user button connected to your host (active HIGH),
    // pull to ground when it's not pressed
    #define USER_BUTTON_PIN RPI_BPLUS_GPIO_J8_36

#elif defined PLATFORM_ARDUINO
    // define pin number of CS, RST connected to your host
    #define PIN_CS  21
    #define PIN_RST 22
    // define user button connected to your host (active HIGH),
    // pull to ground when it's not pressed
    #define USER_BUTTON_PIN 4

#else   // define your hardware platform here other than Raspberry Pi or Arduino

#endif

// define message display method on your platform
#ifdef PLATFORM_RASPI
    #define GENERAL_PRINT(x) printf(x)
#elif defined PLATFORM_ARDUINO
    #define GENERAL_PRINT(x) Serial.print(x)
#else   // define your hardware platform here other than Raspberry Pi or Arduino

#endif

struct user_setting_struct
{
    enum AI_MODULE_MODE operation_mode;

    // available values JPEG_QUALITY_LOW_VAL, JPEG_QUALITY_DEFAULT_MEDIUM_VAL, JPEG_QUALITY_HIGH_VAL
    enum JPEG_QUALITY jpeg_quality_value;
};

/* --------- (Provides by InstAI Co.) set AI module's OD event triggering threshold values of each object type --------- */
const uint8_t ai_module_od_thresholds[MAX_OD_SUPPORT_TYPES] = {50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50};

// declare global variable user_setting to store AI Module's settings
struct user_setting_struct user_setting;

/* --------- set your application requirements here --------- */
void prepare_user_setting_variable(struct user_setting_struct *setting)
{
    memset(setting, 0, sizeof(struct user_setting_struct));

    // Select one from IDLE_MODE, OD_MODE, S_MOTION_OD_MODE, OD_JPEG_MODE or S_MOTION_OD_JPEG_MODE
    setting->operation_mode = OD_MODE;

    // Select JPEG quality from JPEG_QUALITY_LOW_VAL, JPEG_QUALITY_DEFAULT_MEDIUM_VAL, JPEG_QUALITY_HIGH_VAL
    setting->jpeg_quality_value = JPEG_QUALITY_DEFAULT_MEDIUM_VAL;
}

// the function to store OD triggered pictures and results received from AI module
void Platform_JPEG_Save(uint8_t *jpeg_data, size_t jpeg_size, struct od_data_struct *od_result)
{
#ifdef PLATFORM_RASPI
    static unsigned long jpeg_num = 0;
    jpeg_num += 1;

    FILE* fp;
    char file_name[50];
    time_t now = time(0);
    struct tm *ltm = localtime(&now);

    // save JPEG file
    sprintf(file_name, "%04d%02d%02d%02d%02d%02d_%d.jpg", 1900 + ltm->tm_year, 1 + ltm->tm_mon, ltm->tm_mday, ltm->tm_hour, ltm->tm_min, ltm->tm_sec, jpeg_num);
    fp = fopen(file_name, "wb");
    fwrite(jpeg_data, 1, jpeg_size, fp);
    fclose(fp);

    // save OD result
    if(od_result != NULL)
    {
        sprintf(file_name, "%04d%02d%02d%02d%02d%02d_%d.csv", 1900 + ltm->tm_year, 1 + ltm->tm_mon, ltm->tm_mday, ltm->tm_hour, ltm->tm_min, ltm->tm_sec, jpeg_num);
        fp = fopen(file_name, "wt");
        char content_buffer[80];
        sprintf(content_buffer, "obj index,center x,center y,width,height,type,confidence level\n");
        fwrite(content_buffer, 1, strlen(content_buffer), fp);
        for(uint8_t i = 0; i < od_result->object_num; i++)
        {
            sprintf(content_buffer, "%d,%d,%d,%d,%d,%d,%d\n", i, od_result->object[i].center_x, od_result->object[i].center_y,
                od_result->object[i].width, od_result->object[i].height, od_result->object[i].object_type, od_result->object[i].confidence_level);
            fwrite(content_buffer, 1, strlen(content_buffer), fp);
        }
        fclose(fp);
    }
#elif defined PLATFORM_ARDUINO

#else   // other platform...

#endif
}

void setup()
{
    char display_buffer[120];

#ifdef PLATFORM_RASPI
    if(!interface_spi_init()) {
        GENERAL_PRINT("Cannot initialize SPI!\n");
        while(1);   // stop executing
    }

#elif defined PLATFORM_ARDUINO
    // initialize print method when using platform Arduino
    Serial.begin(115200);

    // initialize SPI interface (default Arduino SPI object)
    SPI.begin();
    if(!interface_spi_init(NULL)) {
        GENERAL_PRINT("Cannot initialize SPI!\n");
        while(1);   // stop executing
    }

    /** // or you can use custom SPIClass object:
    * SPIClass *custom_spi = new SPIClass(VSPI);
    * custom_spi->begin(SCK, MISO, MOSI);
    * if(!interface_spi_init(custom_spi)) {
    *     GENERAL_PRINT("Cannot initialize SPI!\n");
    *     while(1);   // stop executing
    * }
    */

#else   // other platform...

#endif

    // initialize user button pin as input
    interface_gpio_input(USER_BUTTON_PIN);

    // register the function when JPEG recieved in OD_JPEG_MODE or S_MOTION_OD_JPEG_MODE
    ai_module_register_save_jpeg_func(Platform_JPEG_Save);

    // initialize AI module
    while(!ai_module_init(PIN_CS, PIN_RST))
    {
        GENERAL_PRINT("AI Module cannot be initialized!\n");
        usleep(200000);
    }
    GENERAL_PRINT("AI Module initialized successfully!\n");

    // display constant PartID
    sprintf(display_buffer, "AI Module Part ID = 0x%02X, 0x%02X\n", PART_ID_MSB_CONST_VAL, PART_ID_LSB_CONST_VAL);
    GENERAL_PRINT(display_buffer);
    // retrieve PartID from AI module
    uint8_t part_id_msb, part_id_lsb;
    part_id_msb = interface_spi_read(PIN_CS, R_PART_ID_MSB);
    part_id_lsb = interface_spi_read(PIN_CS, R_PART_ID_LSB);
    sprintf(display_buffer, "Retrieved Part ID = 0x%02X, 0x%02X\n", part_id_msb, part_id_lsb);
    GENERAL_PRINT(display_buffer);

    // set 21 object type OD event triggering threshold values of AI module
    ai_module_set_od_threshold(ai_module_od_thresholds);

    // user settings for operation mode/JPEG settings
    prepare_user_setting_variable(&user_setting);
    // set user setting to AI module
    ai_module_set_jpeg_quality(user_setting.jpeg_quality_value);
    ai_module_switch_mode(user_setting.operation_mode);
}

void loop()
{
    char display_buffer[120];
    static uint32_t rec_counter = 0;

    switch(ai_module_get_mode())
    {
    case IDLE_MODE:
    break;

    default:
    {
        // detect whether there is any event triggered
        struct od_data_struct od_event;
        bool is_obj_detected = ai_module_process_event(&od_event); // event polling mode

        // read OD information if OD event triggered
        if(is_obj_detected)
        {
            sprintf(display_buffer, "AI Module Detected Objects: %d\n", od_event.object_num);
            GENERAL_PRINT(display_buffer);
            if(od_event.object_num > 0)
            {
                GENERAL_PRINT("Object Index\tCenterX\tCenterY\tWidth\tHeight\tType\tConf. Level\n");
                for(int i = 0; i < od_event.object_num; i++)
                {
                    sprintf(display_buffer, "%d\t\t%d\t%d\t%d\t%d\t%d\t%d\n", i, od_event.object[i].center_x, od_event.object[i].center_y,
                        od_event.object[i].width, od_event.object[i].height, od_event.object[i].object_type,
                        od_event.object[i].confidence_level);
                    GENERAL_PRINT(display_buffer);
                }
                GENERAL_PRINT("\n");

                // do other operations when detected the objects...

            }
        }
    }
    break;
    }

    // detect whether user pressed the button with debounce
    static bool btn_last_state = false;
    static uint8_t debounce_counter = 0;
    bool user_button_state = interface_digital_read(USER_BUTTON_PIN);
    if(user_button_state != btn_last_state)
    {
        usleep(20000);  // delay for 20 ms
        user_button_state = interface_digital_read(USER_BUTTON_PIN);
        if(user_button_state != btn_last_state)
        {
            debounce_counter += 1;
            if(debounce_counter >= 2)
            {
                // change the mode
                switch(user_setting.operation_mode)
                {
                case IDLE_MODE:
                    GENERAL_PRINT("Switching to OD_MODE...\n");
                    user_setting.operation_mode = OD_MODE;
                break;
                case OD_MODE:
                    GENERAL_PRINT("Switching to S_MOTION_OD_MODE...\n");
                    user_setting.operation_mode = S_MOTION_OD_MODE;
                break;
                case S_MOTION_OD_MODE:
                    GENERAL_PRINT("Switching to S_MOTION_OD_JPEG_MODE...\n");
                    user_setting.operation_mode = S_MOTION_OD_JPEG_MODE;
                    rec_counter = 0;
                break;
                case S_MOTION_OD_JPEG_MODE:
                    GENERAL_PRINT("Switching to IDLE_MODE...\n");
                    user_setting.operation_mode = IDLE_MODE;
                break;
                }

                // update with new user operation mode
                ai_module_switch_mode(user_setting.operation_mode);
                debounce_counter = 0;
            }
            btn_last_state = user_button_state;
        }
    }

    // do other operations in main loop...

    usleep(10000);  // delay for 10 milliseconds
}

#ifndef PLATFORM_ARDUINO
// implement function main() if host platform is not Arduino
int main()
{
    setup();

    while(true) loop();

    return 0;
}
#endif