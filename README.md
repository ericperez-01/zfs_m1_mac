# zfs_m1_mac
Open Zfs for M1 Mac

This was an initial attempt to convert the OpenZFS on OSX for the M1 Mac. After updating the values for the arm64 architecture, this version is compiling fine on my M1 Mac. However it is failing to run on the M1 Macs due to how Apple is now handling access to the kernel. A more complete rewrite of the OpenZFS code will be necessary in order to make it work with the new restrictions.  
