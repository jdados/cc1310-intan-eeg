# Basic information
This repository contains the CC1310 program for the Tx Coil project. It's capable of transmitting the RF signal under OOK modulation (default) or continously (for power measurements).
The code is based on a LaunchPad example project rfPacketTx. The sub-folder of the repo is a complete Code Composer project that can be imported into local IDE and flashed onto the uC.
# Programming the device
The CC1310 LaunchPad can be used to program the microcontroller on a custom PCB. 
1. Disconnect the XDS debugger circuit and the uC circuit on the LaunchPad from each other by removing on-board header jumpers for the JTAG TCK, TMS, and RST lines.
2. Now use jumper wires to connect the JTAG TCK, TMS, and RST lines between the LaunchPad debugger and our custom board.
3. Connect the LaunchPad to your computer using the provided USB, and open the firmware in CCS. Build the project and make sure it was imported correctly.
4. Click "debug" in CCS. Now the code has been flashed onto the uC on the custom PCB. You can step through the code, pause and use breakpoints like normally.
