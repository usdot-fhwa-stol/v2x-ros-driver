# Cohda MK6 OBU Setup for V2X-ROS-Driver during ITSWC-2025

The Cohda MK6 OBU can be configured using the [mk6-rsu-obu-setup.sh](mk6-rsu-obu-setup.sh) script.
We have observed occasional clearing of the configuration, so a separate script will be run to ensure the configurations exist.

Additionally, SecurityEnable is defaulted to 0 (disabled). To enable security (signed messages), manually run [mk6-rsu-obu-setup.sh](mk6-rsu-obu-setup.sh) with 1 as an argument.

**Note**: This driver assumes the OBU IPv4 address is set to: 192.168.88.40/24.

**ITS WC Note**: Follow these steps in order. Use the FLIR calibration setup to calibrate the flir sensors, then follow the final setup steps to set the final configurations for the demos.

## FLIR-calibration setup
1. Using the PC, open a terminal and clear any possibly saved scripts from your PC:
```
rm ~/Downloads/mk6-rsu-obu-setup*
```
2. Download the typical [mk6-rsu-obu-setup.sh](https://github.com/usdot-fhwa-stol/v2x-ros-driver/blob/develop/docs/mk6-rsu-obu-setup.sh) script.
2. Copy the typical setup script to the OBU:
```
scp ~/Downloads/mk6-rsu-obu-setup.sh rsu@192.168.88.40:/tmp/
```
3. Login to the OBU and gain admin access:
```
ssh rsu@192.168.88.40
Password: rsuadmin
sudo -i
Password: rsuadmin
```
3. Remove all current scripts in /mnt/rw/:
```
rm /mnt/rw/mk6-rsu-obu-setup.sh
rm /mnt/rw/check-mk6-conf.sh
rm /mnt/rw/rc.local
```
4.	Save the script to the /mnt/rw/ directory:
```
mv /mnt/rw/mk6-rsu-obu-setup.sh /mnt/rw/
```
5. Make the scripts executable:
```
chmod +x /mnt/rw/mk6-rsu-obu-setup.sh
```
6.	Run the setup script (with security disabled):
```
/mnt/rw/mk6-rsu-obu-setup.sh 0
```
7. Calibration setup complete. Continue with FLIR calibration, then follow the Final Setup steps.


## Final Setup:

1. Connect the OBU to the same local network as a PC running the V2X-ROS-Driver.
2. Using the PC, open a terminal and clear any possibly saved scripts from your PC:
```
rm ~/Downloads/mk6-rsu-obu-setup*
rm ~/Downloads/check-mk6-conf*
rm ~/Downloads/rc.local*
```
3. Download the three scripts in this [docs](/docs/) directory.
4. Copy the files to the OBU:
```
scp ~/Downloads/mk6-rsu-obu-setup.sh rsu@192.168.88.40:/tmp/
scp ~/Downloads/check-mk6-conf.sh rsu@192.168.88.40:/tmp/
scp ~/Downloads/rc.local rsu@192.168.88.40:/tmp/
Password: rsuadmin
```
5.	Login to the OBU and gain administrator privileges:
```
ssh rsu@192.168.88.40
Password: rsuadmin
sudo -i
Password: rsuadmin
```
6.	Save the scripts to the /mnt/rw directory:
```
mv /tmp/*.sh /mnt/rw/
mv /tmp/rc.local /mnt/rw/
```
7. Make the scripts executable
```
chmod +x /mnt/rw/*.sh
chmod +x /mnt/rw/rc.local
```
8.	Run the setup script (with security disabled):
```
/mnt/rw/mk6-rsu-obu-setup.sh 0
```
9.	Reboot the OBU after the script is done executing:
```
reboot
```

OBU is setup to forward to the PC running V2X-ROS-Driver. Additionally, the rc.local script will automatically run the configuration check script at every boot of the device.
