# InstAI C-Series AI Module Sample Code
This sample code demonstrates how to communicate with C-Series AI Module in C/C++ on your host.

And currently it's compatible with Raspberry Pi platform and Arduino framework.
## Introduction to C-Series AI Module
InstAI C-Series AI Module provides the following object detection(OD) operation modes:
1. **Idle Mode (IDLE_MODE)**: When AI Module is in idle state, it would not respond to host even when the interested objects were detected
2. **Object Detection Mode (OD_MODE)**: When any of the interested objects were detected, respond OD results to host
3. **Sensor Motion Object Detection Mode (S_MOTION_OD_MODE)**: Switch to sensor motion detection state upon entering this mode. When sensor detects the difference of captured image, NPU would be powered up, started to inference the image and respond the OD results. When NPU not detected any of the interested objects for more than 5 seconds. The AI Module would switch back to sensor motion detection state to save power consumption.
4. **Object Detection JPEG Mode (OD_JPEG_MODE)**: When any of the interested objects were detected, AI module would not only respond OD results, but JPEG images from sensor.
5. **Sensor Motion Object Detection JPEG Mode (S_MOTION_OD_JPEG_MODE)**: This mode is the combination of S_MOTION_OD_MODE and OD_JPEG_MODE. When there is no motion detected by sensor, AI Module powers down NPU. Otherwise, AI Module powers up NPU and started inferencing the captured images. If any of the insterested objects were detected, the OD results and the corresponded JPEG image would be responded to the host.

The OD result contains the following information:
1. Total object number detected in the latest frame.
2. The attributes of each detected object including:
   1. the object's coordinate position (center X, center Y) relative to the frame resolution (320x240 pixels)
   2. the object's size (width, height) in pixel
   3. the object's type
   4. the object's confidence level

Here is the operation flow chart of manipulating AI Module:

## Sample Code Files Hierarchy and Usage Description
Here is the graph presents the file hierarchy of this sample code:


1.	**Hardware Layer (interface.h & interface.cpp)**:
    If your host platform is either on Raspberry Pi or on Arduino, you can define either options below in the header file interface.h
    * For Raspberry Pi:
    ```C
    #define PLATFORM_RASPI
    ```
    * For Arduino
    ```C
    #define PLATFORM_ARDUINO
    ```
    
    If your host platform differs from both of the platforms above, remove the platform definition in the file interface.h and finish implementing the platform-dependent hardware functions in the source code interface.h and interface.cpp.
