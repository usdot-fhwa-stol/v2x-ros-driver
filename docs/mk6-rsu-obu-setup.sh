#!/bin/bash
#ddk 2025-04-29

##############################################################################
# Default settings, no change needed if running this script locally
##############################################################################
#set -x 
export ID0="rsu"
export PW0="rsuadmin"
export MIB_DIR="/home/duser/vm_share/fw_Release/docs/RSU/mibs/"
export HOSTNAME=$(cat /etc/hostname)
export SUT_IPV6_ADDR="::1"
export IP="udp6:[$SUT_IPV6_ADDR]:161"
export TX_MODE=0
export CHAN=183

##############################################################################
# Detecting Environment
##############################################################################
_detect_host()
{
  if [ "$HOSTNAME" == "MKx-SDK" ]; then
    export DIR="$MIB_DIR"
  elif [[ $HOSTNAME =~ MK[5-6] ]]; then
    FW_REV=$(fim -l | grep AR | awk '{print $3}')
    echo "Running $FW_REV"
    DIR="/mnt/rw/rsu1609/snmp/mibs/"
  else
    echo "Host not recognized"
    exit 1
  fi
  export RW_AUTH_ARGS="-Le -t10 -r3 -v3 -lauthPriv -M $DIR -m RSU-MIB -u $ID0 -A $PW0 -X $PW0 -aSHA -xAES $IP"
}

##############################################################################
# Local setup (file manipulation and settings)
##############################################################################

_edit_stack_conf()
{
#delete prior edits
sed -i '0,/board/I!d' /mnt/rw/rsu1609/conf/stack.conf
echo " " >> /mnt/rw/rsu1609/conf/stack.conf
sync

if [[ $HOSTNAME =~ MK5 ]]; then
  echo "TxALogEnableFlag           = 1"     >> /mnt/rw/rsu1609/conf/stack.conf
  echo "RxALogEnableFlag           = 1"     >> /mnt/rw/rsu1609/conf/stack.conf
fi

if [[ $CHAN == 180 ]]; then
  echo "WSMP_LLIF                  = 6"     >> /mnt/rw/rsu1609/conf/stack.conf
  echo "BSMPartITxRandOffset_ms    = 5"     >> /mnt/rw/rsu1609/conf/stack.conf
  echo "TxALogEnableFlag               = 1" >> /mnt/rw/rsu1609/conf/stack.conf
  echo "TxBLogEnableFlag               = 1" >> /mnt/rw/rsu1609/conf/stack.conf
  echo "RxALogEnableFlag               = 1" >> /mnt/rw/rsu1609/conf/stack.conf
  echo "RxBLogEnableFlag               = 1" >> /mnt/rw/rsu1609/conf/stack.conf
  echo "Cohda_P1609PC5RxLogEnableFlag  = 0" >> /mnt/rw/rsu1609/conf/stack.conf
  echo "Cohda_P1609PC5TxLogEnableFlag  = 0" >> /mnt/rw/rsu1609/conf/stack.conf
fi
  echo "WSATxEnable                = 0"     >> /mnt/rw/rsu1609/conf/stack.conf
  echo "WBSS_WSA_RepeatRate        = 5"     >> /mnt/rw/rsu1609/conf/stack.conf

  echo "Cohda_PCAP_LoggingDisabled = 0"     >> /mnt/rw/rsu1609/conf/stack.conf
  echo "Cohda_DebugLevel           = 6"     >> /mnt/rw/rsu1609/conf/stack.conf
  echo "Cohda_DebugTimeLevel       = 1"     >> /mnt/rw/rsu1609/conf/stack.conf
  echo "Cohda_DebugInfoLevel       = 2"     >> /mnt/rw/rsu1609/conf/stack.conf

  echo "SecurityEnable             = 0"     >> /mnt/rw/rsu1609/conf/stack.conf
  echo "SendUnsecuredDot2Header    = 1"     >> /mnt/rw/rsu1609/conf/stack.conf
  echo "BSMUnsecurePSID            = 0x20"  >> /mnt/rw/rsu1609/conf/stack.conf

  echo "Cohda_Crypto_TestCountryCode = 840" >> /mnt/rw/rsu1609/conf/stack.conf
  echo "Cohda_Crypto_AeroLogging     = all" >> /mnt/rw/rsu1609/conf/stack.conf
  echo "Cohda_Crypto_SSPEVA   = $SSPEVA"    >> /mnt/rw/rsu1609/conf/stack.conf
  echo "Cohda_Crypto_SSPSPAT  = $SSPSPAT"   >> /mnt/rw/rsu1609/conf/stack.conf
  echo "Cohda_Crypto_SSPMAP   = $SSPMAP"    >> /mnt/rw/rsu1609/conf/stack.conf
  echo "WSMP_ChannelNumber         = $CHAN" >> /mnt/rw/rsu1609/conf/stack.conf
  echo "ContinuousChanNum          = $CHAN" >> /mnt/rw/rsu1609/conf/stack.conf
  echo "ForcedSerChanNum           = $CHAN" >> /mnt/rw/rsu1609/conf/stack.conf
  echo "ForcedcontrolChanNum       = $CHAN" >> /mnt/rw/rsu1609/conf/stack.conf
}

_manually_manipulate_rsu_files()
{
if [[ $HOSTNAME =~ MK[5-6] ]]; then
  echo
  echo "Performing manual setup that cannot be accomplished via SNMP"

  echo "stopping application(s)"
  systemctl disable cw-mkx-application.service && sync && systemctl stop cw-mkx-application
  /opt/cohda/application/rc.local stop &>/dev/null
  sync

  #clean, add SNMP user
  sed -i '0,/agentXTimeout/I!d' /mnt/rw/rsu1609/snmp/snmpd.conf
  sync
  net-snmp-config --create-snmpv3-user -A $PW0 -X $PW0 -a SHA -x AES $ID0 

  _edit_stack_conf
  rm -rf /mnt/rw/rsu1609/conf/user.conf 
  sync

  echo "starting application(s)"
  /opt/cohda/application/rc.local start &>/dev/null
  systemctl enable cw-mkx-application.service && sync && systemctl start cw-mkx-application
  _detect_rsu1609_running
fi
}


##############################################################################
# Helper Functions
##############################################################################
_detect_rsu1609_running()
{
  sleep 5
  RSU_NUM_PROCS=$(ps -A | egrep 'rsu1609|rsu-monitor' | wc -l)
  until [ "$RSU_NUM_PROCS" = "2" ]; do
    echo "Waiting for 'rsu1609', 'rsu-monitor'..."
    sleep 1
    RSU_NUM_PROCS=$(ps -A | egrep 'rsu1609|rsu-monitor' | wc -l)
    done

  sleep 1
  sync  
  echo "rsu1609 successfully started!"
}

_set_standby()
{
  until snmpget $RW_AUTH_ARGS rsuMode.0 | grep -q 'standby(2)'; do
  snmpset $RW_AUTH_ARGS rsuMode.0 i 2 1>/dev/null
  sleep 1
  done
}

_set_operate()
{
  until snmpget $RW_AUTH_ARGS rsuMode.0 | grep -q 'operate(4)'; do
  snmpset $RW_AUTH_ARGS rsuMode.0 i 4 1>/dev/null
  sleep 1
  done
}

_SYNC()
{
  echo "syncing..."
  if [ "$HOSTNAME" == "MKx-SDK" ]; then
    sshpass -p $PW0 ssh $ID0@$SUT_IPV6_ADDR -p 22 "sync && sync"
  else
    sync && sync
  fi
}

_walk_the_mib()
{
echo " "
echo "${FUNCNAME[0]} "
snmpwalk $RW_AUTH_ARGS iso.0.15628.4.1 
}

_configure_IMF_simulation()
{
find . -name "cw_*.txt" 2>/dev/null | xargs sed -i "s/Signature=.*/Signature=False/"
find . -name "cw_*.txt" 2>/dev/null | xargs sed -i "s/TxChannel.*/TxChannel=${CHAN}/"
find . -name "cw_*.txt" 2>/dev/null | xargs sed -i "s/TxMode.*/TxMode=CONT/"
sync
}


##############################################################################
# NMEA Forward
##############################################################################
#192.168.1.101 = 0xC0.0xA8.0x01.0x65
FW_NMEA_ADDR=0x000000000000000000000000C0A80165
FW_NMEA_PORT1="14998"

_set_NMEA_forward()
{
echo
echo "${FUNCNAME[0]} "
snmpset $RW_AUTH_ARGS \
rsuGpsOutputPort.0 i $FW_NMEA_PORT1 \
rsuGpsOutputAddress.0 x $FW_NMEA_ADDR \
rsuGpsOutputInterface.0 s eth0 \
rsuGpsOutputInterval.0 i 1
}


##############################################################################
# WSMFwdRx table
##############################################################################
#192.168.88.10 = 0xC0.0xA8.0x58.0x0A
WSMFWD_ADDR=0x000000000000000000000000C0A8580A
WSMFWD_PORT="5398"
WSMFWD_PSID1="0x20"
WSMFWD_PSID2="0x8002"
WSMFWD_PSID3="0x8003"
WSMFWD_PSID4="0x27"
WSMFWD_PSID5="0x8010"
WSMFWD_RSSI="-100"
WSMFWD_STRT="07E901010000"
WSMFWD_STOP="07EE01010000"

_disable_WSMFwdRx()
{
echo " "
echo "${FUNCNAME[0]} "
snmpset $RW_AUTH_ARGS \
rsuDsrcFwdEnable.1 i 0 
}

_destroy_WSMFwdRx()
{
echo " "
echo "${FUNCNAME[0]} "
snmpset $RW_AUTH_ARGS \
rsuDsrcFwdStatus.4 i 6 \
rsuDsrcFwdStatus.3 i 6 \
rsuDsrcFwdStatus.2 i 6 \
rsuDsrcFwdStatus.1 i 6
}

_set_WSMFwdRx1()
{
echo
echo "${FUNCNAME[0]} "
snmpset $RW_AUTH_ARGS \
rsuDsrcFwdPsid.1 x $WSMFWD_PSID1 \
rsuDsrcFwdDestIpAddr.1 x $WSMFWD_ADDR \
rsuDsrcFwdDestPort.1 i $WSMFWD_PORT \
rsuDsrcFwdProtocol.1 i 2 \
rsuDsrcFwdRssi.1 i $WSMFWD_RSSI \
rsuDsrcFwdMsgInterval.1 i 1 \
rsuDsrcFwdDeliveryStart.1 x $WSMFWD_STRT \
rsuDsrcFwdDeliveryStop.1 x $WSMFWD_STOP \
rsuDsrcFwdEnable.1 i 1 \
rsuDsrcFwdStatus.1 i 4
}

_set_WSMFwdRx2()
{
echo
echo "${FUNCNAME[0]} "
snmpset $RW_AUTH_ARGS \
rsuDsrcFwdPsid.2 x $WSMFWD_PSID2 \
rsuDsrcFwdDestIpAddr.2 x $WSMFWD_ADDR \
rsuDsrcFwdDestPort.2 i $WSMFWD_PORT \
rsuDsrcFwdProtocol.2 i 2 \
rsuDsrcFwdRssi.2 i $WSMFWD_RSSI \
rsuDsrcFwdMsgInterval.2 i 1 \
rsuDsrcFwdDeliveryStart.2 x $WSMFWD_STRT \
rsuDsrcFwdDeliveryStop.2 x $WSMFWD_STOP \
rsuDsrcFwdEnable.2 i 1 \
rsuDsrcFwdStatus.2 i 4
}

_set_WSMFwdRx3()
{
echo
echo "${FUNCNAME[0]} "
snmpset $RW_AUTH_ARGS \
rsuDsrcFwdPsid.3 x $WSMFWD_PSID3 \
rsuDsrcFwdDestIpAddr.3 x $WSMFWD_ADDR \
rsuDsrcFwdDestPort.3 i $WSMFWD_PORT \
rsuDsrcFwdProtocol.3 i 2 \
rsuDsrcFwdRssi.3 i $WSMFWD_RSSI \
rsuDsrcFwdMsgInterval.3 i 1 \
rsuDsrcFwdDeliveryStart.3 x $WSMFWD_STRT \
rsuDsrcFwdDeliveryStop.3 x $WSMFWD_STOP \
rsuDsrcFwdEnable.3 i 1 \
rsuDsrcFwdStatus.3 i 4
}

_set_WSMFwdRx4()
{
echo
echo "${FUNCNAME[0]} "
snmpset $RW_AUTH_ARGS \
rsuDsrcFwdPsid.4 x $WSMFWD_PSID4 \
rsuDsrcFwdDestIpAddr.4 x $WSMFWD_ADDR \
rsuDsrcFwdDestPort.4 i $WSMFWD_PORT \
rsuDsrcFwdProtocol.4 i 2 \
rsuDsrcFwdRssi.4 i $WSMFWD_RSSI \
rsuDsrcFwdMsgInterval.4 i 1 \
rsuDsrcFwdDeliveryStart.4 x $WSMFWD_STRT \
rsuDsrcFwdDeliveryStop.4 x $WSMFWD_STOP \
rsuDsrcFwdEnable.4 i 1 \
rsuDsrcFwdStatus.4 i 4
}

_set_WSMFwdRx5()
{
echo
echo "${FUNCNAME[0]} "
snmpset $RW_AUTH_ARGS \
rsuDsrcFwdPsid.5 x $WSMFWD_PSID5 \
rsuDsrcFwdDestIpAddr.5 x $WSMFWD_ADDR \
rsuDsrcFwdDestPort.5 i $WSMFWD_PORT \
rsuDsrcFwdProtocol.5 i 2 \
rsuDsrcFwdRssi.5 i $WSMFWD_RSSI \
rsuDsrcFwdMsgInterval.5 i 1 \
rsuDsrcFwdDeliveryStart.5 x $WSMFWD_STRT \
rsuDsrcFwdDeliveryStop.5 x $WSMFWD_STOP \
rsuDsrcFwdEnable.5 i 1 \
rsuDsrcFwdStatus.5 i 4
}


##############################################################################
# Immediate Forward table 
##############################################################################

_destroy_IMF()
{
echo
echo "SNMP: Destroy IMF Table"
snmpset $RW_AUTH_ARGS \
rsuIFMStatus.5 i 6 2>/dev/null \
rsuIFMStatus.4 i 6 2>/dev/null \
rsuIFMStatus.3 i 6 2>/dev/null \
rsuIFMStatus.2 i 6 2>/dev/null \
rsuIFMStatus.1 i 6 2>/dev/null
}

_set_IMF1()
{
echo
echo "${FUNCNAME[0]} "
snmpset $RW_AUTH_ARGS \
rsuIFMPsid.1 x 000020 \
rsuIFMDsrcMsgId.1 i 20 \
rsuIFMTxMode.1 i $TX_MODE \
rsuIFMTxChannel.1 i $CHAN \
rsuIFMEnable.1 i 1 \
rsuIFMStatus.1 i 4
}

_set_IMF2()
{
echo
echo "${FUNCNAME[0]} "
snmpset $RW_AUTH_ARGS \
rsuIFMPsid.2 x 0000bfee \
rsuIFMDsrcMsgId.2 i 240 \
rsuIFMTxMode.2 i $TX_MODE \
rsuIFMTxChannel.2 i $CHAN \
rsuIFMEnable.2 i 1 \
rsuIFMStatus.2 i 4
}
 
_set_IMF3()
{
echo
echo "${FUNCNAME[0]} "
snmpset $RW_AUTH_ARGS \
rsuIFMPsid.3 x 0000bfee \
rsuIFMDsrcMsgId.3 i 241 \
rsuIFMTxMode.3 i $TX_MODE \
rsuIFMTxChannel.3 i $CHAN \
rsuIFMEnable.3 i 1 \
rsuIFMStatus.3 i 4
}

_set_IMF4()
{
echo
echo "${FUNCNAME[0]} "
snmpset $RW_AUTH_ARGS \
rsuIFMPsid.4 x 0000bfee \
rsuIFMDsrcMsgId.4 i 243 \
rsuIFMTxMode.4 i $TX_MODE \
rsuIFMTxChannel.4 i $CHAN \
rsuIFMEnable.4 i 1 \
rsuIFMStatus.4 i 4
}

_set_IMF5()
{
echo
echo "${FUNCNAME[0]} "
snmpset $RW_AUTH_ARGS \
rsuIFMPsid.5 x 00008003 \
rsuIFMDsrcMsgId.5 i 244 \
rsuIFMTxMode.5 i $TX_MODE \
rsuIFMTxChannel.5 i $CHAN \
rsuIFMEnable.5 i 1 \
rsuIFMStatus.5 i 4
}

##############################################################################
# Main
##############################################################################
echo " "
_configure_IMF_simulation
_detect_host
_manually_manipulate_rsu_files
_SYNC

_set_standby
_destroy_WSMFwdRx
_set_operate
_set_standby
_set_WSMFwdRx1
_set_WSMFwdRx2
_set_WSMFwdRx3
_set_WSMFwdRx4
_set_WSMFwdRx5
_set_operate

_set_standby
_destroy_IMF
_set_standby
_set_IMF1
_set_IMF2
_set_IMF3
_set_IMF4
_set_IMF5
_set_operate

_walk_the_mib

# Connect to ISS SCMS Server
# systemd-resolve -4 -i eth0 --set-dns=8.8.8.8
# ssh -4 -v -p 8892 ra.preprod.v2x.isscms.com 2>&1 | grep Conn
# systemd-resolve -6 -i eth0 --set-dns=2001:4860:4860::8888
# ssh -6 -v -p 8892 ra.preprod.v2x.isscms.com 2>&1 | grep Conn

exit 0
