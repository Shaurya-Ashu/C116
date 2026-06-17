# C116
A tiny tine mcu . So it is the smallest DEV Board I have built till now. It is powered by STM 32F0 micro controller and offers a built-in ToF sensor , 2 x 2 RGB matrix , 3 GPIO's & UART all under a USB-C connector.

# why I build it
Because it's amazing, just imagine a tiny board of approximately 1 cm x 1 cm can fit STM power dev boadr.
and also I love making small things and I always wanted to build a tiny MCU.

# Zine 
<img width="540" height="828" alt="Frame 4-2" src="https://github.com/user-attachments/assets/01c87d38-8104-43de-9764-c05811f4c034" />


# parts & schematic

I've chosen STM32F042T6Y6TR as the brain of this project , as it offer two of the most important thing I needed for this project i.e. the size (2.6 x 2.7mm) & it offers USB interface .
It comes in a pakage of WLCSP36 and offers 36 ball pins arrange in a matrix of 6 by 6 with pitch of 0.4mm.
With flash memories of 32 Kbytes and an internal clock for usb make it even greater choice for this project .

<img width="391" height="278" alt="Screenshot 2026-06-17 at 3 08 22 AM" src="https://github.com/user-attachments/assets/f555be7a-face-4245-a880-763079e3de14" />


Now for power/programing & also the thing which will decides the pcb size USB-C connector
I used a 16 pin SMD USB-C connector .

<img width="230" height="220" alt="Screenshot 2026-06-17 at 3 08 38 AM" src="https://github.com/user-attachments/assets/fb6af3de-9de5-4132-9850-3856a2ddb338" />


The STM32F04X reqires 3.3v to operate so i used a 3.3v @300mah LDO from texes industries i.e. TLV74333PDQNR
in a pakage size of 1mm by 1mm with two 1uf caps , one at the input and other at the output .

<img width="232" height="106" alt="Screenshot 2026-06-17 at 3 08 50 AM" src="https://github.com/user-attachments/assets/168ae07f-43d6-4b44-b6bc-fe9a4ba95f6f" />


I have integrated a ToF (Time of Flight) sensor VL53L0X from ST and it comes in a pakage size of 4.4 x 2.4 mm
With capability of accurate reading up to 2 m. 
This opens a large number of future projects which could be made with this tiny MCU(C116)

<img width="404" height="141" alt="Screenshot 2026-06-17 at 3 09 12 AM" src="https://github.com/user-attachments/assets/9d42dc11-b675-40c5-acaf-a32242796347" />


There is also a 2 x 2 matrix of RGB LEDs with support for WS2812B protocols.
It comes in a package of 1 BY 1 mm.

<img width="350" height="166" alt="Screenshot 2026-06-17 at 3 09 20 AM" src="https://github.com/user-attachments/assets/fbb0a678-9897-453c-ac5a-572c857994d9" />


For extra pins i have designed some custom footprints according to the PCB dimensions and integrated seven pins in total, which are 3x GPIO's , UART, VCC & GND.

<img width="181" height="96" alt="Screenshot 2026-06-17 at 3 09 48 AM" src="https://github.com/user-attachments/assets/eae718c7-dd47-42e8-a6a4-657199299c3a" />

There are two tiny push buttons for reset and boot .

<img width="340" height="73" alt="Screenshot 2026-06-17 at 3 09 34 AM" src="https://github.com/user-attachments/assets/38f99dce-0214-463c-8385-715cf2560b82" />


# PCB 

It is a 4-layer PCB designed around a footprint of the USB-C connector with dimensions of 12.4 MM by 9.4 MM
Top layer - signals
Inner layer 1 - GND
Inner layer 2 - VCC
Bottom layer - signals


<img width="419" height="487" alt="Screenshot 2026-06-17 at 3 29 42 AM" src="https://github.com/user-attachments/assets/5162669f-5a23-4085-abc4-731ba518d7a6" />

<img width="470" height="483" alt="Screenshot 2026-06-17 at 3 29 53 AM" src="https://github.com/user-attachments/assets/02afe87a-1788-4546-bbe4-837448320508" />

<img width="501" height="373" alt="Screenshot 2026-06-17 at 3 28 11 AM" src="https://github.com/user-attachments/assets/a98be68b-c061-44c7-b58f-a954677433ab" />

<img width="492" height="339" alt="Screenshot 2026-06-17 at 3 28 39 AM" src="https://github.com/user-attachments/assets/9b844bd9-63c0-46e5-9eb6-941116a7e05b" />

# 3D rendering


<img width="2880" height="1226" alt="Untitled_2026-Jun-16_10-40-27PM-000_CustomizedView14234716663_png_alpha" src="https://github.com/user-attachments/assets/88d57346-7040-43f4-b403-e1628270b10e" />
<img width="2880" height="1226" alt="Untitled_2026-Jun-16_10-39-49PM-000_CustomizedView3308345888_png_alpha" src="https://github.com/user-attachments/assets/9677f1f1-13a4-4ad5-a540-0e663ec7b815" />
<img width="2880" height="1226" alt="Untitled v5 Background Removed" src="https://github.com/user-attachments/assets/5fb73936-fb7d-4471-a7e8-919905b521ff" />
<img width="2880" height="1226" alt="Untitled_2026-Jun-16_10-36-46PM-000_CustomizedView8354566009_png_alpha" src="https://github.com/user-attachments/assets/4096cc90-75fb-4ee5-b417-81dca1f0419a" />


# PCB ordering
If you are ordering from JLCPCB you need to select the advanced PCB option with four layer configuration the minimum via size should be 0.2mm and it should filled and caped .

Choose accordingly with your PCB manufacturer.

# Firmware
To upload a code through USB-C we need to bring it in DUF mode and to do so hold the boot button and then press the reset button for a few seconds and release the reset button and then release the boo button , it will bring our MCU into DUF mode.

Use STM32CubeProgrammer for uploding the files .














