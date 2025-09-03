#!/usr/bin/env python3
import argparse
import socket
import J2735_202409

# Monkey-patch imports to control JSON encoding behavior used by to_jer()
import pycrate_asn1rt.asnobj
import pycrate_core.elt as _core_elt
import pycrate_asn1rt.codecs as _asn_codecs

# Ensure JER JSON preserves insertion order (disable alphabetical sorting)
# i.e., pycrate defaults to JSONEncoder(sort_keys=True). Override it here.
try:
    # Recreate the encoder with sort_keys disabled
    _core_elt.JsonEnc = _core_elt.JSONEncoder(sort_keys=False)
    # Propagate to modules that cached the encoder
    _asn_codecs.JsonEnc = _core_elt.JsonEnc
    pycrate_asn1rt.asnobj.JsonEnc = _core_elt.JsonEnc
except Exception:
    pass

# Set for incoming and outgoing data
INBOUND_IP = "0.0.0.0"
INBOUND_PORT = 5398
FORWARD_IP = "127.0.0.1"
FORWARD_PORT = 5400

# Socket/UDP buffering
# Linux will cap SO_RCVBUF based on net.core.rmem_max and may double the value internally.
# Target ~20x the expected 16383 max bytes size for V2X messages.
SO_RCVBUF_SIZE = 16384 * 20 # 16384 for multiples in KiB
# Receive size from the socket (expecting max 16383 bytes)
RECVFROM_SIZE = 16384

def grab_payload(data: bytes):
    """
    Extract payload containing a BSM or SDSM. May be extended to support additional message types.

    Parameters
    ----------
    data : bytes
        The raw UDP payload data.

    Returns
    -------
    bytes
        The extracted payload, or None if not found.
    """
    # BSM and SDSM DSRCmsgIDs as bytes
    msg_ids = (b"\x00\x14", b"\x00\x29")
    for id in msg_ids:
        idx = data.find(id)
        if idx != -1:
            return data[idx:]
    return None

def main():
    parser = argparse.ArgumentParser(description='Decode and forward SAE J2735 V2X Messages as they are received over UDP.')
    parser.add_argument('--ip', help='IP address to receive UDP data.', type=str, default=INBOUND_IP)
    parser.add_argument('--port', help='Port to receive UDP data.', type=int, default=INBOUND_PORT)
    parser.add_argument('--fwd_ip', help='IP address to forward decoded JSON data.', type=str, default=FORWARD_IP)
    parser.add_argument('--fwd_port', help='Port to forward decoded JSON data.', type=int, default=FORWARD_PORT)
    args = parser.parse_args()

    print(f"Waiting for data at {args.ip}:{args.port}...")

    # Cache the MessageFrame outside the loop
    MessageFrame = J2735_202409.MessageFrame.MessageFrame

    # Reuse both inbound and forward sockets
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock, \
         socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as forward_sock:
        # Increase kernel receive buffer to help absorb bursts
        try:
            sock.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, SO_RCVBUF_SIZE)
        except Exception:
            pass
        sock.bind((args.ip, args.port))
        sock.settimeout(1.0)  # allow periodic wakeups

        try:
            while True:
                try:
                    data = sock.recvfrom(RECVFROM_SIZE)[0]
                except socket.timeout:
                    continue  # check for Ctrl+C and keep looping

                payload = grab_payload(data)
                if not payload:
                    continue

                try:
                    # Decode using cached MessageFrame
                    MessageFrame.from_uper(payload)
                    decoded_msg = MessageFrame.to_jer()
                except Exception as e:
                    continue
                print(decoded_msg)

                # Send the decoded message to the forward socket
                try:
                    forward_sock.sendto(decoded_msg.encode(), (args.fwd_ip, args.fwd_port))
                except Exception as e:
                    print(f"Error forwarding JSON: {e}")

        except KeyboardInterrupt:
            print("\nKeyboard interrupt, shutting down")

if __name__ == "__main__":
    main()
