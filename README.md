# btrepkey-win

This small utility program allows a Bluetooth device to be paired to the current Linux operating system without breaking the pairing between the same device and a Windows operating system installed on the same machine. 

For the program to work, several conditions must be met before executing it:

  - the device must be paired with the Windows operating system
  - the device must have been previously paired with the Linux operating system
  - `chntpw` must be installed
  - the program must be executed with root privilege (eg. using `sudo`)
