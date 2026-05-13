# Cohda MK6 OBU Setup for V2X-ROS-Driver

The Cohda MK6 OBU can be configured using the [mk6-rsu-obu-setup.sh](mk6-rsu-obu-setup.sh) script.
We have observed occasional clearing of the configuration, so a separate script will be run to ensure the configurations exist.

Additionally, SecurityEnable is defaulted to 0 (disabled). To enable security (signed messages), manually run [mk6-rsu-obu-setup.sh](mk6-rsu-obu-setup.sh) with 1 as an argument.

**Note**: This driver assumes the OBU IPv4 address is set to: 192.168.88.40/24.

## Setup

1. Download the three scripts in this [docs](/docs/) directory.
2. Connect the OBU to the same local network as a PC running the V2X-ROS-Driver.
3. Using the PC, open a terminal and navigate to the folder where the files were saved. Example:

```sh
cd ~/Downloads
```

4. Copy the files to the OBU:

```sh
scp mk6-rsu-obu-setup.sh rsu@192.168.88.40:/tmp
scp check-mk6-conf.sh rsu@192.168.88.40:/tmp
scp rc.local rsu@192.168.88.40:/tmp
Password: rsuadmin
```

5. Login to the OBU and gain administrator privileges:

```sh
ssh rsu@192.168.88.40
Password: rsuadmin
sudo -i
Password: rsuadmin
```

6. Save the firmware to the /mnt/src directory:

```sh
cp /tmp/*.sh /mnt/rw/
cp /tmp/rc.local /mnt/rw/
```

7.  Make the scripts executable

```sh
chmod +x /mnt/rw/*.sh
chmod +x /mnt/rw/rc.local
```

8.  Run the setup script (with security disabled):

```sh
/mnt/rw/mk6-rsu-obu-setup.sh 0
```

9. Reboot the OBU after the script is done executing:

```sh
reboot
```

OBU is setup to forward to the PC running V2X-ROS-Driver. Additionally, the rc.local script will automatically run the configuration check script at every boot of the device.
