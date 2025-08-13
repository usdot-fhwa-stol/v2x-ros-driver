#!/bin/bash

ID0="rsu"
PW0="rsuadmin"
DIR="/mnt/rw/rsu1609/snmp/mibs/"
IP="udp6:[::1]:161"
RW_AUTH_ARGS="-Le -t10 -r3 -v3 -lauthPriv -M $DIR -m RSU-MIB -u $ID0 -A $PW0 -X $PW0 -aSHA -xAES $IP"

##############################################################################
# Functions
##############################################################################
_detect_rsu1609_running()
{
  sleep 10
  RSU_NUM_PROCS=$(ps -A | egrep 'rsu1609' | wc -l)
  until [ "$RSU_NUM_PROCS" = "1" ]; do
    echo "Waiting for 'rsu1609'..."
    sleep 1
    RSU_NUM_PROCS=$(ps -A | egrep 'rsu1609' | wc -l)
    done

  sleep 1
  sync
  echo "rsu1609 successfully started!"
}

##############################################################################
# Main
##############################################################################
_detect_rsu1609_running

# Define all expected values
declare -A EXPECTED

EXPECTED["RSU-MIB::rsuIFMIndex.1"]="INTEGER: 1"
EXPECTED["RSU-MIB::rsuIFMIndex.2"]="INTEGER: 2"
EXPECTED["RSU-MIB::rsuIFMIndex.3"]="INTEGER: 3"
EXPECTED["RSU-MIB::rsuIFMIndex.4"]="INTEGER: 4"
EXPECTED["RSU-MIB::rsuIFMPsid.1"]="STRING: 20"
EXPECTED["RSU-MIB::rsuIFMPsid.2"]="STRING: bfee"
EXPECTED["RSU-MIB::rsuIFMPsid.3"]="STRING: 8003"
EXPECTED["RSU-MIB::rsuIFMPsid.4"]="STRING: 8010"
EXPECTED["RSU-MIB::rsuIFMDsrcMsgId.1"]="INTEGER: 0"
EXPECTED["RSU-MIB::rsuIFMDsrcMsgId.2"]="INTEGER: 0"
EXPECTED["RSU-MIB::rsuIFMDsrcMsgId.3"]="INTEGER: 0"
EXPECTED["RSU-MIB::rsuIFMDsrcMsgId.4"]="INTEGER: 0"
EXPECTED["RSU-MIB::rsuIFMTxMode.1"]="INTEGER: cont(0)"
EXPECTED["RSU-MIB::rsuIFMTxMode.2"]="INTEGER: cont(0)"
EXPECTED["RSU-MIB::rsuIFMTxMode.3"]="INTEGER: cont(0)"
EXPECTED["RSU-MIB::rsuIFMTxMode.4"]="INTEGER: cont(0)"
EXPECTED["RSU-MIB::rsuIFMTxChannel.1"]="INTEGER: 183"
EXPECTED["RSU-MIB::rsuIFMTxChannel.2"]="INTEGER: 183"
EXPECTED["RSU-MIB::rsuIFMTxChannel.3"]="INTEGER: 183"
EXPECTED["RSU-MIB::rsuIFMTxChannel.4"]="INTEGER: 183"
EXPECTED["RSU-MIB::rsuIFMEnable.1"]="INTEGER: on(1)"
EXPECTED["RSU-MIB::rsuIFMEnable.2"]="INTEGER: on(1)"
EXPECTED["RSU-MIB::rsuIFMEnable.3"]="INTEGER: on(1)"
EXPECTED["RSU-MIB::rsuIFMEnable.4"]="INTEGER: on(1)"
EXPECTED["RSU-MIB::rsuIFMStatus.1"]="INTEGER: active(1)"
EXPECTED["RSU-MIB::rsuIFMStatus.2"]="INTEGER: active(1)"
EXPECTED["RSU-MIB::rsuIFMStatus.3"]="INTEGER: active(1)"
EXPECTED["RSU-MIB::rsuIFMStatus.4"]="INTEGER: active(1)"
EXPECTED["RSU-MIB::rsuDsrcFwdIndex.1"]="INTEGER: 1"
EXPECTED["RSU-MIB::rsuDsrcFwdIndex.2"]="INTEGER: 2"
EXPECTED["RSU-MIB::rsuDsrcFwdPsid.1"]="STRING: 80100000"
EXPECTED["RSU-MIB::rsuDsrcFwdPsid.2"]="STRING: bfee0000"
EXPECTED["RSU-MIB::rsuDsrcFwdDestIpAddr.1"]="STRING: 0:0:0:0:0:0:c0a8:580a"
EXPECTED["RSU-MIB::rsuDsrcFwdDestIpAddr.2"]="STRING: 0:0:0:0:0:0:c0a8:580a"
EXPECTED["RSU-MIB::rsuDsrcFwdDestPort.1"]="INTEGER: 5398"
EXPECTED["RSU-MIB::rsuDsrcFwdDestPort.2"]="INTEGER: 5398"
EXPECTED["RSU-MIB::rsuDsrcFwdProtocol.1"]="INTEGER: udp(2)"
EXPECTED["RSU-MIB::rsuDsrcFwdProtocol.2"]="INTEGER: udp(2)"
EXPECTED["RSU-MIB::rsuDsrcFwdRssi.1"]="INTEGER: -100"
EXPECTED["RSU-MIB::rsuDsrcFwdRssi.2"]="INTEGER: -100"
EXPECTED["RSU-MIB::rsuDsrcFwdMsgInterval.1"]="INTEGER: 1"
EXPECTED["RSU-MIB::rsuDsrcFwdMsgInterval.2"]="INTEGER: 1"
EXPECTED["RSU-MIB::rsuDsrcFwdDeliveryStart.1"]="Hex-STRING: 07 E9 01 01 00 00"
EXPECTED["RSU-MIB::rsuDsrcFwdDeliveryStart.2"]="Hex-STRING: 07 E9 01 01 00 00"
EXPECTED["RSU-MIB::rsuDsrcFwdDeliveryStop.1"]="Hex-STRING: 07 EE 01 01 00 00"
EXPECTED["RSU-MIB::rsuDsrcFwdDeliveryStop.2"]="Hex-STRING: 07 EE 01 01 00 00"
EXPECTED["RSU-MIB::rsuDsrcFwdEnable.1"]="INTEGER: on(1)"
EXPECTED["RSU-MIB::rsuDsrcFwdEnable.2"]="INTEGER: on(1)"
EXPECTED["RSU-MIB::rsuDsrcFwdStatus.1"]="INTEGER: active(1)"
EXPECTED["RSU-MIB::rsuDsrcFwdStatus.2"]="INTEGER: active(1)"


# Walk and capture output once
SNMP_OUT="$(snmpwalk $RW_AUTH_ARGS iso.0.15628.4.1.5; snmpwalk $RW_AUTH_ARGS iso.0.15628.4.1.7)"

FAILED=0
PASSED=0

echo "Checking SNMP table values..."
for KEY in "${!EXPECTED[@]}"; do
    EXPECTED_VAL="${EXPECTED[$KEY]}"
    # Use grep to check if line exists
    if echo "$SNMP_OUT" | grep -Fq "$KEY = $EXPECTED_VAL"; then
        echo "PASS: $KEY = $EXPECTED_VAL"
        ((PASSED++))
    else
        echo "FAIL: $KEY expected '$EXPECTED_VAL'"
        ((FAILED++))
    fi
done

echo
echo "Summary: $PASSED passed, $FAILED failed"
if [ $FAILED -ne 0 ]; then
    echo "Some SNMP table values did not match the expected configuration."
    echo "Running mk6-rsu-obu-setup.sh to reconfigure..."
    echo "Defaulting to SecurityEnable = 0"
    /mnt/rw/mk6-rsu-obu-setup.sh 0

    exit 1
else
    exit 0
fi
