# Sync via browser (Kermit server)

F1 -> Sync -> "Sync via browser" turns Draftling into a Kermit file server on
its serial console, so a computer connected to the device's USB port
can list, download, upload, rename and delete the files of the current
project: the project of the document being edited, or the one shown or
selected in the file browser. The client sees the project folder (its
documents and `git.cfg`) as the top folder. No WiFi is needed.

It is available only on boards whose USB port is wired straight to
the ESP32-S3's built-in USB -- the same boards that offer "SD card via
USB" (see "Supported boards" below).

Kermit is a long-established file-transfer protocol for serial lines
(<https://www.kermitproject.org>). Any Kermit client that can talk to a
Kermit server works: C-Kermit (`kermit`), Kermit 95, or a web page
that implements the protocol over the browser's Web Serial API.

## Starting and stopping

1. Press F1, open Sync (letter Y) and select "Sync via browser:
   <project>..." (letter B; the
   row names the project that will be served) and choose
   **Start** (or **Cancel** to go back to the menu).
2. Draftling saves every open document, closes the editor and shows
   the "Sync via browser" dialog. If no computer is connected to the
   device's USB port, the dialog first asks you to connect one with a
   USB cable; as soon as the computer has detected the device, it
   asks you to open
   <https://bridge.draftling.art/> in Chrome, Edge or Opera on the
   computer connected to the device's USB port and to follow the
   instructions there (the bridge is
   [Draftbridge](https://github.com/clackups/draftbridge)). Any other
   Kermit client works as well. The dialog shows the progress of each
   transfer.
3. When you are done, select **Finished** in the dialog (Enter or Esc,
   or tap it on a touchscreen). The server stops and the file list is
   refreshed with whatever changed.

If the USB connection goes away (the cable is unplugged, or the
computer goes to sleep), the dialog says so. Plug the cable in again
to continue, or select Finished.

While a computer is connected, the device does not go to sleep: the
standby timer, the power button and Ctrl+P are ignored. While no
computer is connected (before the cable is plugged in, or after the
connection is lost), the normal standby timeout applies again. The
dialog cannot be closed except with Finished.

Start is refused while "SD card via USB" or a Git sync is running,
when no SD card is mounted, and while a new (never saved) document
has unsaved changes -- save or discard it first. While the server is
running, the editor, Git sync, Draftbox, "Format SD card" and
"SD card via USB" are not available.

While the server runs, the console carries Kermit packets instead of
the firmware's log messages: all console output is switched off, and
it comes back when the server is stopped.

## Supported boards

Supported: Xteink X4 Pro and X4 Classic, LilyGO T5 E-Paper S3 Pro /
Pro Lite / H752, M5Stack PaperS3 and PaperMono, Waveshare
ESP32-S3-ePaper-3.97, ESP32-S3-RLCD-4.2, ESP32-S3-Touch-LCD-7,
ESP32-S3-Touch-LCD-3.49 and ESP32-S3-LCD-3.16, the four Freenove
FNK0104 boards, and the Viewe UEDX24320028E-WB-A.

Every other board has no "Sync via browser" menu item. That includes
the boards whose USB port goes through a USB-to-UART bridge chip, such
as the Seeed reTerminal E1001, the Seeed reTerminal Sticky and the
Elecrow CrowPanel 5.79": the bridge's control lines are wired to the
chip's reset and boot-mode pins, and a computer opening the port can
reset the device or hold it in reset.

## Connecting

When you select Start, the device's USB port switches over: the serial
port the device normally shows (its log console) disappears and a new
serial port named "Draftling" appears (`/dev/ttyACM0` on Linux,
`/dev/cu.usbmodem*` on macOS, a COM port on Windows). Connect to that
one; the speed setting does not matter. When you select Finished, the
"Draftling" port goes away and the console port comes back.

The console port itself cannot be used for this: the chip's built-in
USB serial port resets the device when a program sets its control
lines the way the browser does when it opens a port.

Example with C-Kermit on Linux:

```
kermit -l /dev/ttyACM0 -b 115200
C-Kermit> set carrier-watch off
C-Kermit> set flow none
C-Kermit> remote directory
C-Kermit> get notes.md
C-Kermit> send chapter-3.md
C-Kermit> finish
```

## Supported commands

| Client command | What the server does |
|----------------|----------------------|
| `send` | Stores the file(s) on the SD card (a file with the same name is replaced only once the transfer has completed) |
| `get` | Sends the named file(s); `*` and `?` wildcards are allowed in the file name |
| `remote directory` | Lists a directory (name, size, date) |
| `remote cd`, `remote pwd` | Changes / shows the current directory; `/` is the top of the SD card |
| `remote delete` | Deletes the named file(s); wildcards are allowed |
| `remote rename` | Renames a file |
| `remote mkdir`, `remote rmdir` | Creates / removes a directory |
| `remote space` | Shows the SD card's size and free space |
| `remote type` | Shows a file's contents |
| `remote help` | Lists these commands |
| `finish`, `bye` | Acknowledged; the server keeps running until you select Finished on the device |

The client cannot reach anything outside the SD card.

## Notes

- Files are sent and stored byte for byte. When a client sends a file
  in text mode (C-Kermit does this for files it recognises as text),
  its CR LF line endings are turned back into the LF line endings
  Draftling uses.
- C-Kermit sends file names in upper case unless told
  `set file names literal`; a name without any lower-case letters is
  stored in lower case.
- The protocol supports long packets (up to about 9 KB), block checks
  1, 2 and 3 (CRC-16), repeat-count compression, 8th-bit prefixing
  and attribute packets (file type, size and date). Sliding windows,
  streaming and transfer recovery are not supported.
- Stopping the server during a transfer cancels it; a partially
  received file is deleted.
