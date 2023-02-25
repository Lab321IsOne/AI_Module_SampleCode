/** InstAI Co. (Public Version)
    Description: This is the sample code for complete API (OD/S-Motion OD/S_MOTION_JPEG_OD) for InstAI C-series AI Module
    Modified Date: Feb 25, 2023
    Remark: this C/C++ Library only supports single AI module connected to the host
*/

#ifndef AI_MODULE_H
#define AI_MODULE_H

#include "interface.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
// include headers for function save_jpeg()
#include <dirent.h>
#include <errno.h>

//-- Constant values
#define MAX_OD_SUPPORT_TYPES    21
#define MAX_OD_SUPPORT_OBJECTS  30

//-- Host platform dependency value
#define AI_MODULE_BUFFER_SIZE 30 * 1024

//-- Registers
#define R_PART_ID_LSB 0x00
#define R_PART_ID_MSB 0x01
//-- Constant check value for R_PART_ID_LSB/R_PART_ID_MSB register
#define PART_ID_LSB_CONST_VAL 0x80
#define PART_ID_MSB_CONST_VAL 0x76

//-- Enumerations
/**
    @brief: options of AI module modes
    @remark: host changes the mode of AI module by calling function ai_module_switch_mode()
        or retrieve current mode of AI module by calling function ai_module_get_mode()
*/
enum AI_MODULE_MODE
{
    IDLE_MODE = 0x00,
    OD_MODE = 0x04,
    S_MOTION_OD_MODE = 0x0E,
    OD_JPEG_MODE = 0x12,
    S_MOTION_OD_JPEG_MODE = 0x14
};

/**
    @brief: set save JPEG quality
    @remark: host can change JPEG quality saved in SRAM of AI module by calling function ai_module_set_jpeg_quality()
*/
enum JPEG_QUALITY
{
    JPEG_QUALITY_LOW_VAL = 0x80,
    JPEG_QUALITY_DEFAULT_MEDIUM_VAL = 0x40,
    JPEG_QUALITY_HIGH_VAL = 0x20
};

//-- Structures
/**
    @brief: data structure of each detected object attributes
    @remark: the detected object attributes could be obtained by calling function ai_module_process_event()
        when AI module is triggered in OD_MODE, S_MOTION_OD_MODE or S_MOTION_JPEG_OD_MODE
        these attributes are stored in the field "object" of data structure "od_data_struct"
*/
struct od_object_unit_struct {
    uint32_t center_x;
    uint32_t center_y;
    uint32_t width;
    uint32_t height;
    uint8_t object_type;        // object type ranges from 2 to 22
    uint8_t confidence_level;
};

/**
    @brief: data structure of triggered OD event information
    @remark: the object detected information could be obtained by calling function ai_module_process_event()
        when AI module is triggered in OD_MODE, S_MOTION_OD_MODE or S_MOTION_JPEG_OD_MODE
*/
struct od_data_struct {
    uint8_t object_num;
    uint8_t reserve;
    struct od_object_unit_struct object[MAX_OD_SUPPORT_OBJECTS];
};

//-- Function Pointer
/**
    @brief: function pointer which points to custom function to save retrieved JPEG data to specific platform
    @parameter:
        jpeg_data:  (value provided by the function) provides retrieved JPEG data from AI module (including JPEG header and contents)
        jpeg_size:  (value provided by the function) provides the size of JPEG data
        od_result:  (value provided by the function) provides the OD results if AI module is in OD_MODE, S_MOTION_OD_MODE,
                    OD_JPEG_MODE or S_MOTION_OD_JPEG_MODE
    @remark: 
*/
typedef void (*FunPtr_SaveJPEG)(uint8_t *jpeg_data, size_t jpeg_size, struct od_data_struct *od_result);

/**
    @brief: initialize AI module
    @parameter:
        ai_module_pin_cs: specify digital pin number of AI Module's SPI chip select Pin
        ai_module_pin_rst: specify digital pin number of AI Module's RST Pin
    @return:
        return true if AI module initialized successfully
        otherwise, return false
    @remark: function ai_module_init() must be called after function interface_init()
        and ensure interface_init() returned true (platform GPIO & SPI initialized successfully)
*/
bool ai_module_init(uint8_t ai_module_pin_cs, uint8_t ai_module_pin_rst);

/**
    @brief: set OD event triggering threshold values of each type of objects
    @parameter:
        th_values: the array of OD event triggering threshold values of 21 object types
    @return:
        (NONE)
    @remark: 21 object type OD event triggering threshold should be provided by InstAI Co.
        set the OD event triggering threshold values once AI module successfully initialized
*/
void ai_module_set_od_threshold(const uint8_t *th_values);

/**
    @brief: set JPEG quality saved in SRAM of AI module
    @parameter:
        jpeg_quality: one of the quality options defined in enumeration JPEG_QUALITY
    @return:
        (NONE)
    @remark: JPEG quality should be set after AI module initialization
*/
void ai_module_set_jpeg_quality(enum JPEG_QUALITY jpeg_quality);
/**
    @brief: register save JPEG function implemented on your platform
    @parameter:
        user_save_jpeg_func:    provide the function defined on your platform with prototype:
                                void [Custom_Function_Name](uint8_t *jpeg_data, size_t jpeg_size, struct od_data_struct *od_result);
    @return:
        (NONE)
    @sa
        Data Type "Function Pointer": FunPtr_SaveJPEG
*/
void ai_module_register_save_jpeg_func(FunPtr_SaveJPEG user_save_jpeg_func);

/**
    @brief: switch the mode of AI module
    @parameter:
        mode: one of the modes defined in enumeration AI_MODULE_MODE
    @return:
        (NONE)
*/
void ai_module_switch_mode(enum AI_MODULE_MODE mode);
/**
    @brief: get the current mode of AI module
    @parameter:
        (NONE)
    @return:
        current mode of AI module defined in enumeration AI_MODULE_MODE
*/
enum AI_MODULE_MODE ai_module_get_mode();

/**
    @brief: process OD event / JPEG event if any event(s) triggered, OD event results would be stored in the given parameter od_data
        captured image would also be saved if AI module is in OD_JPEG_MODE or S_MOTION_OD_JPEG_MODE
    @parameter:
        od_data: give the variable with type "od_data_struct" to store the detected OD event information and object attributes
    @return:
        in OD_MODE / S_MOTION_OD_MODE / OD_JPEG_MODE or S_MOTION_OD_JPEG_MODE:
            return true if AI module detected interested object(s), host can retrieve the results stored in given parameter od_data
            return false if AI module has not detected any interested objects, and the content of given parameter od_data would not be changed
*/
bool ai_module_process_event(struct od_data_struct *od_data);

#endif // AI_MODULE_H
