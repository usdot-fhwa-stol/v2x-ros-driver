# Overview
This [v2x_decoder_forwarder.py](v2x_decoder_forwarder.py) script is used to receive, decode, and forward SAE J2735 V2X Messages as a JSON string to a remote server.
Accepted inputs are UPER-encoded using the SAE J2735 2024-09 ASN.1 and are received via a UDP server. Current outputs are decoded BSM and SDSM JSON strings and are forwarded to a user-defined UDP server. The script may be extended to add other V2X message types.

## Requirements
* pycrate>=0.7.11
```
pip3 install pycrate
```
To upgrade pycrate:
```
pip3 install pycrate --upgrade
```

## Usage
1. Run the script:
```
./v2x_decoder_forwarder.py -h
```
