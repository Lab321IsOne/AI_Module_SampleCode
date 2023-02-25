/** InstAI Co. (Public Version)
    Description: This is the sample code for complete API (OD/S-Motion OD/OD_JPEG/S_MOTION_OD_JPEG) for InstAI C-series AI Module
    Modified Date: Feb 25, 2023
    Remark: this C/C++ Library only supports single AI module connected to the host
*/
#include "ai_module.h"

//-- Registers
#define R_FW_POWER_ON_READY_0 0x03
#define R_INTO_STATUS 0x04
#define R_CPU_RESET_ENL 0x0A
#define R_RPT_SRAM_DATA_REG 0x0F
#define R_OP_MODE_HOST 0x10
#define R_OP_HOST_REQ 0x21
#define R_OP_HOST_PARA 0x22
#define R_OP_HOST_PARA0_REG 0x23
#define R_OP_HOST_PARA1_REG 0x24
#define CPU_VALID_CONTROL 0x3B
#define R_JPEG_QUALITY 0x69
#define BANK_SEL	0x7F
//#define T_INDEX_LOW_BYTE_REG R_OP_HOST_PARA0_REG
//#define T_INDEX_HIGH_BYTE_REG R_OP_HOST_PARA1_REG

//-- Parameters for R_OP_HOST_REQ register
#define REQ_DATA_INIT 0x03
#define REQ_DATA_REQUEST 0x04
#define REQ_STATE_CLR 0x05

//-- Constant values
#define TINDEX_DEFAULT 1024
#define DATA_DESCRIPTION_SIZE 32

//-- define AI Module Interrupt values
enum AI_MODULE_EVENT
{
    READY_EVENT = 0x01,
    OD_EVENT = 0x02,
    JPEG_EVENT = 0x40
};

//-- Structure
struct data_description_struct
{
    uint32_t total_loop;
    uint32_t total_length;
    uint32_t max_size_per_packet;
    uint32_t t1_motion_frame;
    uint32_t t2_start_frame;
    uint32_t t3_end_frame;
    uint32_t t4_current_frame;
    uint32_t t5_od_frame;
};

/* ---- internal commands function prototypes declaration ---- */
void reset();
void control_command(uint8_t command);
void function_read_sram_data(uint8_t *array, int32_t length);
void read_data_description(struct data_description_struct *description);
void read_data(struct data_description_struct *description, uint8_t *data);
void clear_event(uint8_t event_type);
void set_parameter_Event(uint8_t event_type);
void set_parameter_Tindex(uint32_t T_index);
void parse_od(uint8_t  *data, struct od_data_struct *od);
// check if the event happens during handling JPEG
void recheck_event_before_clear_jpeg(uint8_t e, struct od_data_struct *od_data);
void handle_event(uint8_t e, uint8_t recheck_which_event, struct od_data_struct *od_data);

//-- Global variables
uint8_t pin_cs, pin_rst;
struct data_description_struct data_description;
uint8_t data_buffer[AI_MODULE_BUFFER_SIZE] = { 0 };
FunPtr_SaveJPEG save_jpeg_func = NULL;  // function pointer to store user_jpeg_save_func

bool ai_module_init(uint8_t ai_module_pin_cs, uint8_t ai_module_pin_rst)
{
    // initialize pin numbers
    pin_cs = ai_module_pin_cs;
    pin_rst = ai_module_pin_rst;

    uint32_t partid_value = 0;

    // initialize GPIO
    // initialize CS & RST pin as OUTPUT digital pin
    interface_gpio_output(pin_cs);
    interface_gpio_output(pin_rst);

    // initialize CS & RST pin states
    interface_digital_write(pin_rst, HIGH);
    interface_digital_write(pin_cs, HIGH);
    usleep(1000);
    interface_digital_write(pin_cs, LOW);
    usleep(1000);
    interface_digital_write(pin_cs, HIGH);
    usleep(1000);

    interface_spi_write(pin_cs, BANK_SEL, 0x00);			// Switch to bank 0
    partid_value = interface_spi_read(pin_cs, R_PART_ID_LSB) + (interface_spi_read(pin_cs, R_PART_ID_MSB) << 8);

    if (partid_value != (PART_ID_LSB_CONST_VAL + (PART_ID_MSB_CONST_VAL << 8)))
        return false;

    int32_t counter = 0;
    uint8_t power_on_ready = 0, temp_value = 0;
    uint8_t event_into_status = 0;

    // reset the module before wake up
    reset();

    interface_spi_write(pin_cs, BANK_SEL, 0x00);
    interface_spi_write(pin_cs, CPU_VALID_CONTROL, 0x01);	// CPU on
    interface_spi_write(pin_cs, R_CPU_RESET_ENL, 0x01);

    while (1)
    {
        if (counter > 1000)
            return false;

        temp_value = interface_spi_read(pin_cs, R_FW_POWER_ON_READY_0);
        power_on_ready = (temp_value & 0x01);
        if (power_on_ready == 0x01)
            break;
        usleep(10000);
        counter++;
    }

    counter = 0;
    while (1)
    {
        if (counter > 1000)
            return false;

        event_into_status = interface_spi_read(pin_cs, R_INTO_STATUS);
        if (event_into_status == READY_EVENT)
        {
            clear_event(READY_EVENT);
            break;
        }

        usleep(10000);
        counter++;
    }
    usleep(100000);
    return true;
}

void ai_module_set_od_threshold(const uint8_t *th_values)
{
    interface_spi_write(pin_cs, BANK_SEL, 14);  // switch to bank 14
    for(uint8_t i = 0; i < MAX_OD_SUPPORT_TYPES; i++)
        interface_spi_write(pin_cs, 74 + i, th_values[i]);
    interface_spi_write(pin_cs, BANK_SEL, 0);   // switch to bank 0
}

void reset()
{
    // back to ready state
    ai_module_switch_mode(IDLE_MODE);

    interface_digital_write(pin_rst, LOW);
    usleep(10000);
    interface_digital_write(pin_rst, HIGH);
    usleep(50000);
}

void ai_module_set_jpeg_quality(enum JPEG_QUALITY jpeg_quality)
{
    interface_spi_write(pin_cs, BANK_SEL, 0); // switch to bank 0
    interface_spi_write(pin_cs, R_JPEG_QUALITY, (int8_t)jpeg_quality);
}

void ai_module_register_save_jpeg_func(FunPtr_SaveJPEG user_save_jpeg_func)
{
    save_jpeg_func = user_save_jpeg_func;
}

void control_command(uint8_t command)
{
    interface_spi_write(pin_cs, BANK_SEL, 0); // switch to bank 0
    interface_spi_write(pin_cs, R_OP_HOST_REQ, command);			// Write REQ_DATA_INIT (0x03) to R_OP_HOST_REQ (0x21) register
    while (interface_spi_read(pin_cs, R_OP_HOST_REQ) != 0);			// Wait for PAG7681LS handled the request
}

void ai_module_switch_mode(enum AI_MODULE_MODE mode)
{
    interface_spi_write(pin_cs, BANK_SEL, 0); // switch to bank 0
    // remeber to switch to IDLE_MODE before changing to any other operation mode
    interface_spi_write(pin_cs, R_OP_MODE_HOST, IDLE_MODE);
    usleep(100000);
    if(mode == IDLE_MODE)
        return;
    interface_spi_write(pin_cs, R_OP_MODE_HOST, (uint8_t)mode);
    usleep(300000);
}

void function_read_sram_data(uint8_t * array, int32_t length)
{
    int32_t i = 0;
    for (i = 0; i < length; i++)
        array[i] = interface_spi_read(pin_cs, R_RPT_SRAM_DATA_REG);
}

void read_data_description(struct data_description_struct *description)
{
    int32_t i = 0;
    uint8_t  data_description_array[DATA_DESCRIPTION_SIZE] = { 0 };

    control_command(REQ_DATA_INIT);			// Write REQ_DATA_INIT (0x03) to R_OP_HOST_REQ (0x21) register

    function_read_sram_data(data_description_array, DATA_DESCRIPTION_SIZE);

    memset(description, 0, sizeof(struct data_description_struct));

    for (i = 0; i < 4; i++)
        description->total_loop += (data_description_array[i] << (8 * i));
    for (i = 4; i < 8; i++)
        description->total_length += (data_description_array[i] << (8 * (i - 4)));
    for (i = 8; i < 12; i++)
        description->max_size_per_packet += (data_description_array[i] << (8 * (i - 8)));
    for (i = 12; i < 16; i++)
        description->t1_motion_frame += (data_description_array[i] << (8 * (i - 12)));
    for (i = 16; i < 20; i++)
        description->t2_start_frame += (data_description_array[i] << (8 * (i - 16)));
    for (i = 20; i < 24; i++)
        description->t3_end_frame += (data_description_array[i] << (8 * (i - 20)));
    for (i = 24; i < 28; i++)
        description->t4_current_frame += (data_description_array[i] << (8 * (i - 24)));
    for (i = 28; i < 32; i++)
        description->t5_od_frame += (data_description_array[i] << (8 * (i - 28)));
}

void read_data(struct data_description_struct* description, uint8_t * data)
{
    uint32_t last_length = description->total_length;
    uint32_t temp_length = 0, offset = 0;

    while (last_length != 0)
    {
        control_command(REQ_DATA_REQUEST);

        if (last_length > description->max_size_per_packet)	// readout length can't exceed
            temp_length = description->max_size_per_packet;	// internal SRAM size (max_size_per_packet)
        else
            temp_length = last_length;

        function_read_sram_data(&data[offset], temp_length);
        last_length -= temp_length;
        offset += temp_length;
    }
}

void clear_event(uint8_t event_type)
{
    interface_spi_write(pin_cs, R_OP_HOST_PARA, event_type); 			// Write event_type to R_OP_HOST_PARA register
    control_command(REQ_STATE_CLR);
}

void set_parameter_Event(uint8_t event_type) 			        // Write event_type to R_OP_HOST_PARA register
{ interface_spi_write(pin_cs, R_OP_HOST_PARA, event_type); }

void set_parameter_Tindex(uint32_t T_index)
{
    interface_spi_write(pin_cs, R_OP_HOST_PARA0_REG, T_index & 0xff);
    interface_spi_write(pin_cs, R_OP_HOST_PARA1_REG, (T_index >> 8) & 0xff);
}

void parse_od(uint8_t *data, struct od_data_struct* od)
{
    if(od == NULL)
        return;

    int32_t i = 0;

    memset(od, 0, sizeof(struct od_data_struct));
    od->object_num = data[0];
    od->reserve = data[1];

    for (i = 0; i < od->object_num; i++)
    {
        od->object[i].center_x = data[2 + (10 * i)] + (data[2 + (10 * i) + 1] << 8);
        od->object[i].center_y = data[2 + (10 * i) + 2] + (data[2 + (10 * i) + 3] << 8);
        od->object[i].width = data[2 + (10 * i) + 4] + (data[2 + (10 * i) + 5] << 8);
        od->object[i].height = data[2 + (10 * i) + 6] + (data[2 + (10 * i) + 7] << 8);
        od->object[i].object_type = data[2 + (10 * i) + 8];
        od->object[i].confidence_level = data[2 + (10 * i) + 9];
    }
}

void recheck_event_before_clear_jpeg(uint8_t e, struct od_data_struct *od_data)
{
    uint8_t event_into_status = interface_spi_read(pin_cs, R_INTO_STATUS);

    if ((event_into_status & e) == e)
    {
        switch (e)
        {
            case OD_EVENT:
                set_parameter_Event(OD_EVENT);
                read_data_description(&data_description);
                memset(data_buffer, 0, AI_MODULE_BUFFER_SIZE);
                read_data(&data_description, data_buffer);
                parse_od(data_buffer, od_data);

                break;
            default:
                break;
        }
    }
}

void handle_event(uint8_t e, uint8_t recheck_which_event, struct od_data_struct *od_data)
{
    switch (e)
    {
        case READY_EVENT:
            clear_event(READY_EVENT);
            break;
        case OD_EVENT:
            set_parameter_Event(OD_EVENT);
            read_data_description(&data_description);
            memset(data_buffer, 0, AI_MODULE_BUFFER_SIZE);
            read_data(&data_description, data_buffer);
            clear_event(OD_EVENT);
            if(od_data != NULL)
                parse_od(data_buffer, od_data);
            break;
        case JPEG_EVENT:
            set_parameter_Tindex(TINDEX_DEFAULT);
            set_parameter_Event(JPEG_EVENT);
            read_data_description(&data_description);
            memset(data_buffer, 0, AI_MODULE_BUFFER_SIZE);
            read_data(&data_description, data_buffer);

            // call the user JPEG saving function to save the frame makes OD triggered before clear the JPEG event
            if(save_jpeg_func != NULL)
                save_jpeg_func(data_buffer, data_description.total_length, od_data);
            
            if(recheck_which_event != 0)
                recheck_event_before_clear_jpeg(recheck_which_event, od_data);

            clear_event(JPEG_EVENT);
            break;
        default:
            break;
    }
}

enum AI_MODULE_MODE ai_module_get_mode()
{
    enum AI_MODULE_MODE mode = (enum AI_MODULE_MODE)interface_spi_read(pin_cs, R_OP_MODE_HOST);
    return mode;
}

bool ai_module_process_event(struct od_data_struct *od_data)
{
    uint8_t event_into_status = 0;

    interface_spi_write(pin_cs, BANK_SEL, 0); // switch to bank 0
    event_into_status = interface_spi_read(pin_cs, R_INTO_STATUS);

    switch (event_into_status)
    {
        case READY_EVENT:
            handle_event(READY_EVENT, 0, NULL);
            break;
        case OD_EVENT:
            handle_event(OD_EVENT, 0, od_data);
            return true;
        case JPEG_EVENT:
            handle_event(JPEG_EVENT, 0, NULL);
            break;
        case (JPEG_EVENT | OD_EVENT): // 0x42
            handle_event(OD_EVENT, 0, od_data);
            handle_event(JPEG_EVENT, OD_EVENT, od_data);
            return true;
        default:
            break;
    }
    return false;
}