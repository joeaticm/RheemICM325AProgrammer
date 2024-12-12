# Attila: NFC Reader/Writer program
### for Nucleo-L053R8 with X-Nucleo-NFC06A1 add on board

The NFC Reader/Writer, Codename: Attila, is designed to work with ICM's universal test fixtures (UTF) via USB, but can also plug into a PC and communicate via a serial terminal program such as Realterm. The software is designed to accept single-byte commands from from a UTF or serial terminal program while the Reader/Writer constantly scans for NFC devices within its antenna range. If an NFC device is detected, the Reader/Writer will process the last command it received and energize an onboard LED signifying which type of NFC device was found.

## Usage

The Reader/Writer is constantly polling for serial communications with the **baud rate set to 19200**. When it receives a command from the connected UTF or PC, the device becomes "armed" until a tag enters the antenna range. The command byte is processed according to the table below once a compatible tag is detected.
The intended operation sequence for production is as follows:
* UTF sends Command `A` to the Reader/Writer, which verifies that the Unit Under Test (UUT) can accept NFC write operations.
* UTF sends Command `B` and attempts to read a specific response from the NFC chip's memory, which verifies that the UUT's processor can communicate with the NFC chip.
* If the prior commands pass, UTF sends command `C` to initialize the UUT to Factory Default Settings.

|Command|Effect|
|---|---|
| A (0x41) | Check for a 4 byte Marker (`@ICM`) in Block 55 (Addresses 220-223) of the detected NFC-V tag. If the marker is present, then the Reader/Writer will de-initialize the NFC Chip, returning it to the state it was in when it left ST-Micro's Factory. Regardless of the presence of the Marker, the Reader/Writer will then write the ASCII values for the word `TEST` into Block 60 (Addresses 240-243) of the detected NFC-V tag. The Reader/Writer will transmit the status `PASS` or `FAIL` in ASCII over the serial connection.|
| B (0x42) | Read Block 60 (Addresses 240-243) of the detected NFC-V tag. It will then compare data found there to a set of data containing the ASCII values for the word `PASS`. The Reader/Writer will transmit the status `PASS` or `FAIL` in ASCII over the serial connection. NOTE: Command `B` is currently not used for the ICM325A as the software engineer for the ICM325A was unable to figure out how to write `PASS` into Block 60 using the I2C Connection. It is kept around in case this feature is added to future products.|
| C (0x43) | Write the Factory Configuration Data into the NFC-V tag in antenna range. The data always includes the Configuration and Area Passwords, Factory Default Configuration Settings and Checksum, CC File, and NDEF's to open the related ICM Smartphone App for both Android and iOS devices. The exact values are different depending on the product; data is hard coded for each model and a #define is used to build different firmware versions for different product test fixtures. As each test fixture is physically unique and can only be used to test and program one specific type of Product, this ensures that an operator or script error cannot accidently load the wrong Factory Configuration Data into a product. After the configuration is written, the Reader/Writer will write a 4 byte Marker `0x40 0x49 0x43 0x4D`, or "@ICM", in Block 55 (Addresses 220-223). This Marker is used by Command `A` to check if a unit has already been initialized at the ICM factory so it can be reset prior to re-testing.
| D (0x44) | De-Initialize (zero out) the NFC-V tag in antenna range. It will first check that the tag was previously Initialized with Command `C`, then erase the tag and transmit `PASS` over serial. If it does not find the marker from Command C, it will transmit `FAIL` over serial and the tag will not be erased. This command is meant for development use, not on the factory floor; command `A` will automatically run the de-init procedure if needed.|
| ? (0x3F) | Query the Reader/Writer board for the current configuration that will be programmed into units by command `C`. The device will reply with the model name (in ASCII). To change to a different model's configuration, the Reader/Writer itself must be reprogrammed with the appropriate binary (since the parameters are decided at compile time).|

The defininitions for each different product type is located in the icm_models.h file.

There is also a debug output feature that is intended to be used with a serial terminal program such as Realterm. It can be toggled with a #define called DEBUG_OUTPUT, and is specified by CubeIDE in the "Debug" build configuration.

This software was developed by modifying existing code provided by ST-Micro. As such not all of the module names are accurate,
	there is plenty of bloat (I tried to shrink it as best I could), uncommented and hard to explain code, etc. Furthermore, the
	necessary configuration file used to reconfigure pin-outs, clock speeds, and other settings of the IC was not included with this
	code. It is still possible to reconfigure these settings but to do so will require more intimate knowledge of this IC and some
	reverse engineering.

## Setup & Installation
### Git Repository
Clone this repository to your preferred location for working on source code. Then `cd` into it and run the following command:
```
git config --local include.path ../.gitconfig
```
This enables some custom repo settings that must be consistent across all users. For security reasons, git requires the user to execute this step manually on the command line.

### STM32CubeIDE
STM32CubeIDE is based on the Eclipse integrated development environment, and bundles multiple ST-specific development tools into one package. This document will refer to it as "CubeIDE" for short.
1. Download & install STM32CubeIDE from [ST's official website](https://www.st.com/en/development-tools/stm32cubeide.html).
   *  This project was created and compiled with version 1.11.2. Newer versions *should* work as well, but v1.11.2 is confirmed to work in case the latest version fails to build the project correctly.
2. On first run, CubeIDE will prompt to select a Workspace. You may use the default location if you have no other CubeIDE projects, or create an empty folder to use as a new workspace.
   * **DO NOT** create or select a workspace folder inside the project repository!
   * The Workspace is where CubeIDE stores user settings that are not part of any specific project.
3. When the Workspace loads, select `Window → Show View → Project Explorer` from the menu bar.
4. From the menu bar, select `File → Import...` then expand the `General` group and choose `Existing Projects into Workspace`. Press the "Next" button.
5. Select the bubble next to `Select Root Directory:` and browse to the repository on your system. The `Projects:` field should populate with one project (L053R8-NFC06A1-Attila).
6. Ensure the `Copy projects into workspace` option is **unchecked**, then press the Finish button.

## Making Changes to the Source Code
Eclipse (and by extension, CubeIDE) relies on its own strict project hierarchy for storing files. Most of it is automatically generated & managed by the IDE, so there are only a few relevant source files to edit when updates are needed. Drivers and Middlewares provided by ST should NOT be modified.

## Version History
2023-03-14	v1.22
- Added Command `?` (0x3F)
- moved product parameter definitions to icm_models.h

2023-03-08	v1.21
- Updated URL to fix iPhone launcher shortcut

2023-02-10	v1.20
- Added De-Virginized Marker functionality

2023-02-07	v1.10
- Added Command `D` (0x44)

2023-02-07	v1.00
- Initial Release to factory floor

2022-12-08	v0.20 BETA
- Added #define's for all 3 Products along with updated factory default configuration data

2022-09-22	v0.10 BETA
- This version is a proof of concept and can accept hex commands from a terminal program such as Realterm to execute the
- Factory Default NFC configuration of an ICM-UFPT-5, Codename: Bullshark. It is not a complete, finished release.