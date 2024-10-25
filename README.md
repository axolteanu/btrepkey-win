# btrepkey-win

This small utility program allows a Bluetooth device to be paired to the current Linux operating system without breaking the pairing between the same device and a Windows operating system installed on the same machine. 

To compile, simple run `make`.

Before executing the program, several conditions must be met:

  - the device must be paired with the Windows operating system installed on your machine
  - the device must have been previously paired with the current Linux operating system
  - there must be a configuration file called `btrepkey-win.conf` in the `/etc` directory containing the property `WPATH` set to the path to the Windows partition (must end in `/`)
    - e.g. `WPATH=/media/windows/`
  - `chntpw` must be installed

The program must be executed as root (e.g. with `sudo`).
