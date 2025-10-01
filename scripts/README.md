# Overview
This [v2x_decoder_forwarder.py](v2x_decoder_forwarder.py) script is used to receive, decode, and forward SAE J2735 V2X Messages as a JSON string to a remote server.
Accepted inputs are UPER-encoded using the SAE J2735 2024-09 ASN.1 and are received via a UDP server. Current outputs are decoded BSM and SDSM JSON strings and are forwarded to a user-defined UDP server. The script may be extended to add other V2X message types.

## Requirements
* pycrate>=0.7.11
* j2735_202409
```sh
pip3 install pycrate
```sh
To upgrade pycrate:
```sh
pip3 install pycrate --upgrade
```

Download the [j2735_202409 wheel](https://github.com/usdot-fhwa-stol/j2735decoder/blob/develop/wheels/j2735_202409-0.1.0-py3-none-any.whl) file and install it.
```sh
pip3 install j2735_202409*.whl
```

## Usage
1. Run the script:
```
./v2x_decoder_forwarder.py -h
```
