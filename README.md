# btrepkey-win

This small utility program allows a Bluetooth device to be paired to the current Linux operating system without breaking the pairing between the same device and a Windows operating system installed on the same machine. 

For the program to work, several conditions must be met before executing it:

  - the device must be paired with the Windows operating system
  - the device must have been previously paired with the Linux operating system
  - there must be a config. file called `btrepkey-win` in the `/etc` folder containing the setting `WPATH` set to the path to the Windows partition `e.g. WPATH=/media/windows/`
  - `chntpw` must be installed
  - the program must be executed as root (e.g. with `sudo`)
