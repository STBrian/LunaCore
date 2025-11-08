import socket
import struct 
import argparse
import time

def send_string(s: socket.socket, data: str):
    length = len(data.encode("utf-8"))
    s.sendall(struct.pack("<I", length))
    s.sendall(data.encode("utf-8"))

def recv_all(conn, size):
    data = bytearray()
    while len(data) < size:
        packet = conn.recv(size - len(data))
        if not packet:
            raise ConnectionError("Connection closed")
        data.extend(packet)
        #print(f"Progress: {len(data) / size * 100:.2f}")
    return data

def parse_hex(hexs: str) -> int:
    value = 0
    try:
        value = int(hexs, 16)
    except:
        raise ValueError("Invalid hex value!")
    return value

def parse_int(ints: str) -> int:
    value = 0
    try:
        value = int(ints, 10)
    except:
        raise ValueError("Invalid 32 value!")
    return value

def parse_float(floats: str) -> float:
    value = 0
    try:
        value = float(floats)
    except:
        raise ValueError("Invalid float value!")
    return value

def start_console(host: str, port: int):
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect((host, port))

        cmd = ""
        while cmd != "exit":
            cmd = input("> ").strip()
            cmdParts = cmd.split()
            cmdName = cmdParts[0].lower()

            if cmdName == "exit":
                s.sendall(struct.pack("<I", 0x5AB1E))
                cmd = "exit"
                continue

            try:
                if cmdName == "ping":
                    current_time = time.time()
                    s.sendall(struct.pack("<I", 0))
                    recv_all(s, 4)
                    print(f"Ping in {time.time() - current_time} seconds")
                elif cmdName == "write32":
                    if len(cmdParts) != 3:
                        raise Exception("Invalid command usage!")
                    s.sendall(struct.pack("<I", 1))
                    s.sendall(struct.pack("<I", parse_hex(cmdParts[1])))
                    s.sendall(struct.pack("<I", parse_int(cmdParts[2])))
                elif cmdName == "read32":
                    if len(cmdParts) != 2:
                        raise Exception("Invalid command usage!")
                    s.sendall(struct.pack("<I", 2))
                    s.sendall(struct.pack("<I", parse_hex(cmdParts[1])))
                    value = struct.unpack("<I", recv_all(s, 4))[0]
                    print(f"{value:08X}")
                elif cmdName == "writef":
                    if len(cmdParts) != 3:
                        raise Exception("Invalid command usage!")
                    s.sendall(struct.pack("<I", 1))
                    s.sendall(struct.pack("<I", parse_hex(cmdParts[1])))
                    s.sendall(struct.pack("<f", parse_float(cmdParts[2])))
                elif cmdName == "storfile":
                    if len(cmdParts) != 3:
                        raise Exception("Invalid command usage!")
                    dstPath = cmdParts[1]
                    sourcePath = cmdParts[2]
                    dstPathEnc = bytearray(dstPath.encode("utf-8"))
                    dstPathEnc.append(0)
                    lenDstPath = len(dstPathEnc)
                    with open(sourcePath, "rb") as f:
                        data = f.read()
                    s.sendall(struct.pack("<I", 3))
                    s.sendall(struct.pack("<I", lenDstPath))
                    s.sendall(dstPathEnc)
                    s.sendall(struct.pack("<I", len(data)))
                    s.sendall(data)
                else:
                    print("Invalid command!")
            except Exception as e:
                print(f"{e}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description = "Send scripts to LunaCore plugin")
    parser.add_argument("-a", "--address", help="The host address that LunaCore shows", required=True)
    args = parser.parse_args()
    host = args.address

    port = 5431
    if host != "":
        start_console(host, port)