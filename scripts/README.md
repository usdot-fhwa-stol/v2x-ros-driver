# Overview
This [parse_messages.py](parse_messages.py) script is used to receive, decode, and forward SAE J2735 V2X Messages as a JSON string to a remote server.
Accepted inputs are UPER-encoded using the SAE J2735 2024-09 ASN.1. Current outputs are decoded BSM and SDSM JSON strings, but the script may be extended to add more V2X message types.

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
./parse_messages.py -h
```
