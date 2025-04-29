## Cohda MK6 OBU CARMA Setup
The Cohda MK6 OBU can be configured using the [mk6-rsu-obu-setup.sh](mk6-rsu-obu-setup.sh) script.


# Setup:
1. Download the rsu/obu setup script.
2. Using the test PC, open a terminal and navigate to the folder where the firmware was saved. Example: 
```
cd ~/Downloads
```
3. Copy the folder: 
```
scp mk6-rsu-obu-setup.sh rsu@192.168.88.40:/tmp
Password: rsuadmin
```
4.	Log into the OBU and gain administrator privileges:
```
ssh rsu@192.168.88.40
Password: rsuadmin
sudo -i 
Password: rsuadmin
```
5.	Save the firmware to the /mnt/src directory:
```
cp /tmp/mk6-rsu-obu-setup.sh /mnt/src/
```
6.	Run the setup script:
```
/mnt/src/mk6-rsu-obu-setup.sh
```
7.	Reboot the OBU after the script is done executing:
```
reboot
```
8.	OBU is setup to forward to CARMA.
