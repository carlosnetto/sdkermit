# sdkermit
An old implementation of Kermit protocol and VT100 terminal emulator

SDKermit is a legacy VT100 terminal emulator and implementation of the Kermit file transfer protocol, designed for IBM PC-compatible computers running MS-DOS.

## Historical Context

This software represents a piece of history from **Matera Systems** (formerly known as **Software Design**). Before focusing on payments and core banking solutions, Software Design developed utilities like SDKermit to facilitate communication between PCs and mainframes or Unix servers via RS232 serial ports.

The "SD" in SDKermit stands for **Software Design**. This repository serves as a museum of what the company achieved in the early stages of its existence.

## Key Use Cases

SDKermit was widely used in Brazil during the era of serial communications:

*   **Unix Server Connectivity:** It was frequently used to connect PCs to Unix servers, particularly **IBM AIX** systems.
*   **Telephone Switch Management:** A distinct advantage of SDKermit was its support for **25-line screen display**. This feature made it the tool of choice for accessing and configuring specific models of telephone switches that required a full 25-line terminal interface.

## Features

*   **Terminal Emulation:** Full VT100 and VT52 emulation.
*   **File Transfer:** Robust implementation of the Kermit protocol for sending and receiving files.
*   **Hardware Interface:** Direct interaction with PC RS232 serial ports (UART) and BIOS video services.
*   **User Interface:** A text-mode, menu-driven interface for configuration and operation.

## Technical Details

The codebase is written in C and targets the MS-DOS environment. It utilizes direct hardware manipulation for performance, including:
*   Direct writing to video memory for terminal display.
*   Interrupt-driven serial communication handling (8250/16550 UARTs).
*   BIOS interrupts for system services.

## Status

This is a "dead" system, preserved for historical and educational purposes. It reflects the state of software engineering and telecommunications integration in Brazil during the dominance of MS-DOS and serial terminals.
