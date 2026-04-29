# TripWIreTool
C++ tripwire monitoring tool with decoy files and HTML logging

## Overview
TripWireTool is a C++ Windows-based defensive monitoring tool built in Visual Studio. It acts as a lightweight tripwire system by automatically generating decoy files, monitoring directories for suspicious file activity, and logging detected events.

The tool detects:
- File creation
- File modification
- File deletion
- File renaming

It logs events to both:
- a plain text log file
- an HTML log file for easier visualization

## Features
- Automatically creates decoy/tripwire files in a default monitored folder
- Allows users to add additional folders to monitor
- Detects file system changes in real time
- Writes events to `tripwire_log.txt`
- Writes events to `tripwire_log.html`
- Provides a simple defensive/deceptive monitoring capability for Windows hosts

## Technologies Used
- C++
- Visual Studio
- Windows API (`ReadDirectoryChangesW`)
- HTML for log visualization

## How It Works
When the program starts, it creates a `DecoyFiles` directory and generates several default decoy files. It then begins monitoring that folder and any additional user-supplied folders for suspicious file activity. When activity is detected, the event is written to both a text log and an HTML log.

## Example Decoy Files
- `payroll_backup.txt`
- `admin_passwords.txt`
- `confidential_notes.txt`
- `finance_records.txt`

## How to Run
1. Open the project in Visual Studio
2. Build the C++ console application
3. Run the program
4. Type `DONE` to monitor only the default decoy folder, or enter an existing folder path to add another folder to monitor
5. Trigger file events such as create, modify, rename, or delete
6. Review the output in:
   - `tripwire_log.txt`
   - `tripwire_log.html`

## Notes
- Avoid monitoring a folder that contains the program’s own log files, or repeated logging events may occur.
- A separate test folder is recommended for demonstrations.

## Example Use Case
This project can be used as a lightweight defensive countermeasure to detect possible file tampering or suspicious host activity. It also incorporates deception by generating decoy files automatically.
