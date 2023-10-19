ble_emrg_build.bin
-------------------

    Dump of MCU flash with the latest firmware


fwDump.bat + jLinkDumpScript.jlink
---------------------------------------

    These 2 files work with Jlink and help to get full dump of the MCU flash. It helps to get 
    BIN file which may be used in production to flash new PCBs

    In order to make a dump:

        - connect the board to JLINK (see assembling drawing, there is a connection schematic for JLINK) 

        - connect a battery to the board 

        - run fwDump.bat

    If there is no any problem with the hardware, the flash dump will be saved in ble_emrg_build.bin file


fwUpload.bat + jLinkUploadScript.jlink
---------------------------------------

    These 2 files work with Jlink and help to simplify flashing process of the hardware.

    In order to flash the board:

        - connect the board to JLINK (see assembling drawing, there is a connection schematic for JLINK) 

        - connect a battery to the board 

        - run fwUpload.bat

    If there is no any problem with the hardware, the PCB will be flashed automatically within 10 sec. or so.