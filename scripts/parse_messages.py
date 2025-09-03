#!/usr/bin/python3
import socket
import J2735_202409

# Set for incoming and outgoing data
INBOUND_PORT = 5398  # must match port defined in sender device
FORWARD_IP = "127.0.0.1"
FORWARD_PORT = 5400

def grab_payload(data: bytes):
    """
    Extract payload containing a BSM or SDSM.

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
    print("Script is running, waiting for data...")

    # Cache the MessageFrame outside the loop
    MessageFrame = J2735_202409.MessageFrame.MessageFrame

    # Reuse both inbound and forward sockets
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock, \
         socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as forward_sock:
        sock.bind(("0.0.0.0", INBOUND_PORT))
        sock.settimeout(1.0)  # allow periodic wakeups

        try:
            while True:
                try:
                    data, addr = sock.recvfrom(65536)
                except socket.timeout:
                    continue  # check for Ctrl+C and keep looping

                payload = grab_payload(data)
                if not payload:
                    # print("No valid payload found.")
                    continue

                # print(f"Received payload from {addr} of size {len(payload)}")

                try:
                    # Decode using cached class
                    MessageFrame.from_uper(payload)
                    parsed = MessageFrame()
                except Exception as e:
                    # print(f"Decoding error: {e}")
                    continue

                # print(parsed)
                if parsed["messageId"] == 20:  # BSM
                    core = parsed["value"][1]["coreData"]
                    parsed_msg = {
                        "id": core["id"].hex(),
                        "lat": core["lat"],
                        "long": core["long"],
                        "elev": core["elev"],
                        "speed": core["speed"],
                        "heading": core["heading"],
                    }
                    print(parsed_msg)
                elif parsed["messageId"] == 41:  # SDSM
                    objectList = []
                    sourceID = parsed["value"][1]["sourceID"].hex()
                    refPos = parsed["value"][1]["refPos"]
                    objects = parsed["value"][1]["objects"]
                    for obj in objects:
                        pos = obj["detObjCommon"]["pos"]
                        obj_data = {
                            "id": obj["detObjCommon"]["objectID"],
                            "objType": obj["detObjCommon"]["objType"],
                            "offsetX": pos["offsetX"],
                            "offsetY": pos["offsetY"],
                            "offsetZ": pos["offsetZ"],
                            "speed": obj["detObjCommon"]["speed"],
                            "heading": obj["detObjCommon"]["heading"],
                        }
                        objectList.append(obj_data)
                    parsed_msg = {
                        "sourceID": sourceID,
                        "refPos": {
                            "lat": refPos["lat"],
                            "long": refPos["long"],
                            "elev": refPos["elevation"],
                        },
                        "objects": objectList,
                    }
                    print(parsed_msg)

                # Reuse the forward socket for sending
                try:
                    forward_sock.sendto(parsed_msg.encode(), (FORWARD_IP, FORWARD_PORT))
                except Exception as e:
                    print(f"Error forwarding JSON: {e}")

        except KeyboardInterrupt:
            print("\nKeyboard interrupt, shutting down")

if __name__ == "__main__":
    main()
