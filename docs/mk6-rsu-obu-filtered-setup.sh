#!/bin/bash

##############################################################################
# Default settings, no change needed if running this script locally
##############################################################################
export ID0="rsu"
export PW0="rsuadmin"
export HOSTNAME=$(cat /etc/hostname)
export SUT_IPV6_ADDR="::1"
export IP="udp6:[$SUT_IPV6_ADDR]:161"
export TX_MODE=0
export CHAN=183

# Change the following to the desired start and end dates for the WSMFwdRx table
# Example: 2025-01-01 00:00 UTC = 07E901010000
FW_StartDate="07E901010000"
FW_EndDate="07EE01010000"
# Change IP address and port to remote receiving address (spectra pc)
REMOTE_IP="192.168.88.10"
REMOTE_PORT="5398"

##############################################################################
# Setting Environment
##############################################################################
_set_host()
{
  FW_REV=$(fim -l | grep AR | awk '{print $3}')
  echo "Running $FW_REV"
  DIR="/mnt/rw/rsu1609/snmp/mibs/"

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

echo "Using user input: SecurityEnable = $1"

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

  echo "SecurityEnable             = $1"     >> /mnt/rw/rsu1609/conf/stack.conf

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
  /opt/cohda/application/rc.local stop &>/dev/null
  sync

  #clean, add SNMP user
  sed -i '0,/agentXTimeout/I!d' /mnt/rw/rsu1609/snmp/snmpd.conf
  sync
  net-snmp-config --create-snmpv3-user -A $PW0 -X $PW0 -a SHA -x AES $ID0

  _edit_stack_conf $1
  rm -rf /mnt/rw/rsu1609/conf/user.conf
  sync

  echo "starting application(s)"
  /opt/cohda/application/rc.local start &>/dev/null
  _detect_rsu1609_running
fi
}

# Function to read a valid input for SecurityEnable
# It will keep prompting until a valid input (0 or 1) is provided
read_valid() {
    local val
    while :; do
        read -rp "Enter SecurityEnable (0 or 1) i.e., disabled=0, enabled=1: " val
        case "$val" in
            0|1) printf '%s' "$val"; return 0 ;;
            *)   printf 'Invalid. Try again.\n' >&2 ;;
        esac
    done
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

to_hex()
{
  local hex_ip="000000000000000000000000"

  # Split the IP address into its four octets
  IFS='.' read -r -a octets <<< "$1"

  # Convert each octet to hex and concatenate
  for octet in "${octets[@]}";
  do
      hex_ip+=$(printf '%02X' "$octet")
  done

  echo $hex_ip
}

##############################################################################
# WSMFwdRx table
##############################################################################
# Convert the SUT IPv4 address to hex
# The hex IP address is 32 characters long, with leading zeros
# Example: 192.168.88.10 = 000000000000000000000000c0a8580a
WSMFWD_ADDR=$(to_hex "$REMOTE_IP")
if [ "${#WSMFWD_ADDR}" -ne 32 ]; then
  echo "The submitted IP address is invalid. Exiting..."
  exit 1
fi
echo "Configuring using backhaul IP address: $REMOTE_IP"
echo "IP address in hex: $WSMFWD_ADDR"

_destroy_WSMFwdRx()
{
echo " "
echo "${FUNCNAME[0]} "
for i in {9..1}; do
  snmpset $RW_AUTH_ARGS rsuDsrcFwdStatus.$i i 6 &>/dev/null
done
}

_set_WSMFwdRx() {
  echo
  echo "${FUNCNAME[0]} "

  local psid_list=("0x8010" "0xBFEE")
  local fwd_index=1

  for psid in "${psid_list[@]}"; do
    snmpset $RW_AUTH_ARGS \
      rsuDsrcFwdPsid.$fwd_index        x "$psid" \
      rsuDsrcFwdDestIpAddr.$fwd_index  x "$WSMFWD_ADDR" \
      rsuDsrcFwdDestPort.$fwd_index    i "$REMOTE_PORT" \
      rsuDsrcFwdProtocol.$fwd_index    i 2 \
      rsuDsrcFwdRssi.$fwd_index        i -100 \
      rsuDsrcFwdMsgInterval.$fwd_index i 1 \
      rsuDsrcFwdDeliveryStart.$fwd_index x "$FW_StartDate" \
      rsuDsrcFwdDeliveryStop.$fwd_index  x "$FW_EndDate" \
      rsuDsrcFwdEnable.$fwd_index      i 1 \
      rsuDsrcFwdStatus.$fwd_index      i 4
    echo

    ((fwd_index++))
  done
}

##############################################################################
# Immediate Forward table
##############################################################################

_destroy_IFM()
{
echo
echo "${FUNCNAME[0]} "
for i in {9..1}; do
  snmpset $RW_AUTH_ARGS rsuIFMStatus.$i i 6 &>/dev/null
done
}

_set_IFM() {
  echo
  echo "${FUNCNAME[0]} "

  local psid_list=("0x20" "0xBFEE" "0x8003" "0x8010")
  local ifm_index=1

  for psid in "${psid_list[@]}"; do
    snmpset $RW_AUTH_ARGS \
      rsuIFMPsid.$ifm_index       x "$psid" \
      rsuIFMDsrcMsgId.$ifm_index  i 0 \
      rsuIFMTxMode.$ifm_index     i $TX_MODE \
      rsuIFMTxChannel.$ifm_index  i $CHAN \
      rsuIFMEnable.$ifm_index     i 1 \
      rsuIFMStatus.$ifm_index     i 4
    echo

    ((ifm_index++))
  done
}

##############################################################################
# Main
##############################################################################
echo
_set_host

# If $1 provided, validate it; otherwise prompt. An argument of 0 or 1 for SecurityEnabled (disabled/enabled, respectively) is expected.
if [[ $# -gt 0 ]]; then
    case "$1" in
        0|1) input="$1" ;;
        *)   printf 'Error: arg must be 0 or 1\n' >&2; exit 1 ;;
    esac
else
    input="$(read_valid)"
fi

_manually_manipulate_rsu_files "$input"
_SYNC

_set_standby
_destroy_WSMFwdRx
_set_operate

_set_standby
_set_WSMFwdRx
_set_operate

_set_standby
_destroy_IFM
_set_operate

_set_standby
_set_IFM
_set_operate

_walk_the_mib

exit 0
