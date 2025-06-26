# Cohda MK6 OBU CARMA Setup

The Cohda MK6 OBU can be configured using the [mk6-rsu-obu-setup.sh](mk6-rsu-obu-setup.sh) script.
We have observed occasional clearing of the configuration, so a separate script will be run to ensure the configurations exist.


## Setup:

1. Download the three scripts in this [docs](/docs/) directory.
2. Using a test PC, open a terminal and navigate to the folder where the files were saved. Example:
```
cd ~/Downloads
```
3. Copy the files to the OBU:
```
scp mk6-rsu-obu-setup.sh rsu@192.168.88.40:/tmp
scp check-mk6-conf.sh rsu@192.168.88.40:/tmp
scp rc.local rsu@192.168.88.40:/tmp
Password: rsuadmin
```
4.	Login to the OBU and gain administrator privileges:
```
ssh rsu@192.168.88.40
Password: rsuadmin
sudo -i
Password: rsuadmin
```
5.	Save the firmware to the /mnt/src directory:
```
cp /tmp/*.sh /mnt/rw/
cp /tmp/rc.local /mnt/rw/
```
6.	Run the setup script:
```
/mnt/src/mk6-rsu-obu-setup.sh
```
7.	Reboot the OBU after the script is done executing:
```
reboot
```
8.	OBU is setup to forward to CARMA. Additionally, the rc.local script will automatically run the configuration check script at every boot of the device.
