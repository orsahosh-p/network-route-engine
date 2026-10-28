"""Run against local binaries; the test owns and terminates its server."""
import socket
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def connect():
    sock = socket.create_connection(("127.0.0.1", 18080), timeout=3)
    return sock, sock.makefile("rb")


def exchange(sock, stream, command, expected):
    sock.sendall(command)
    assert stream.readline() == expected, command


server = subprocess.Popen(
    [str(ROOT / "build/route-server-test")], stdout=subprocess.PIPE, stderr=subprocess.PIPE,
    text=True,
)
try:
    assert server.stdout.readline().strip() == "Network Route Engine Server"
    assert server.stdout.readline().strip() == "Server listening on 127.0.0.1:18080"
    sock, stream = connect()
    with sock, stream:
        sock.sendall(b"PRINT\nPING\n")
        assert stream.readline() == b"GRAPH\n"
        empty = []
        while (record := stream.readline()) != b"END\n":
            assert record
            empty.append(record)
        assert b"nodes: 0\n" in empty
        assert stream.readline() == b"PONG\n"
        exchange(sock, stream, b"PRINT extra\n", b"ERROR\n")
        exchange(sock, stream, b"PING\n", b"PONG\n")
        exchange(sock, stream, b"GET_NODE 1\n", b"ERROR\n")
        sock.sendall(b"ADD_NO")
        exchange(sock, stream, b"DE 1\n", b"OK\n")
        sock.sendall(b"ADD_NODE 2\nADD_EDGE 1 2 10\n")
        assert stream.readline() == b"OK\n"
        assert stream.readline() == b"OK\n"
        sock.sendall(b"PRINT\nGET_NODE 1\n")
        assert stream.readline() == b"GRAPH\n"
        records = []
        while (record := stream.readline()) != b"END\n":
            assert record
            records.append(record)
        assert b"nodes: 2\n" in records
        assert b"Node 1 -> [2, weight=10]\n" in records
        assert b"Node 2 -> [1, weight=10]\n" in records
        assert stream.readline() == b"NODE 1\n"
        sock.sendall(b"GET_NO")
        exchange(sock, stream, b"DE 1\n", b"NODE 1\n")
        sock.sendall(b"GET_NODE 2\nGET_NODE 999\n")
        assert stream.readline() == b"NODE 2\n"
        assert stream.readline() == b"ERROR\n"
        exchange(sock, stream, b"GET_NODE 0001\n", b"NODE 1\n")
        exchange(sock, stream, b"ADD_NODE 18446744073709551615\n", b"OK\n")
        exchange(sock, stream, b"GET_NODE 18446744073709551615\n", b"NODE 18446744073709551615\n")
        for command in (b"GET_NODE", b"GET_NODE ", b"GET_NODE -1", b"GET_NODE +1",
                        b"GET_NODE 1x", b"GET_NODE 1 2", b"GET_NODE 1 ",
                        b"GET_NODE 18446744073709551616"):
            exchange(sock, stream, command + b"\n", b"ERROR\n")
        exchange(sock, stream, b"GET_NODE 1\n", b"NODE 1\n")
        exchange(sock, stream, b"ADD_NODE 1\n", b"ERROR\n")
        exchange(sock, stream, b"ADD_EDGE 2 1 10\n", b"ERROR\n")
        exchange(sock, stream, b"ADD_EDGE 1 999 10\n", b"ERROR\n")
        exchange(sock, stream, b"ADD_NODE -1\n", b"ERROR\n")
        exchange(sock, stream, b"HELLO\n", b"ERROR\n")
        exchange(sock, stream, b"\n", b"ERROR\n")
        # Maximum accepted line: 1023 characters, excluding newline.
        exchange(sock, stream, b"ADD_NODE " + b"0" * 1014 + b"\n", b"OK\n")
        exchange(sock, stream, b"GET_NODE 0\n", b"NODE 0\n")
    for bad in (b"PING\x00ignored\n", b"X" * 1024):
        sock, stream = connect()
        with sock, stream:
            exchange(sock, stream, bad, b"ERROR\n")
            assert stream.readline() == b""
    result = subprocess.run([str(ROOT / "build/route-client-test"), "PING"], capture_output=True, text=True, timeout=3)
    assert result.returncode == 0 and result.stdout == "PONG\n", result
    result = subprocess.run([str(ROOT / "build/route-client-test"), "ADD_NODE 3"], capture_output=True, text=True, timeout=3)
    assert result.returncode == 0 and result.stdout == "OK\n", result
    result = subprocess.run([str(ROOT / "build/route-client-test"), "ADD_NODE 3"], capture_output=True, text=True, timeout=3)
    assert result.returncode == 1 and result.stdout == "ERROR\n", result
    result = subprocess.run([str(ROOT / "build/route-client-test"), "GET_NODE 3"], capture_output=True, text=True, timeout=3)
    assert result.returncode == 0 and result.stdout == "NODE 3\n", result
    result = subprocess.run([str(ROOT / "build/route-client-test"), "GET_NODE 999"], capture_output=True, text=True, timeout=3)
    assert result.returncode == 1 and result.stdout == "ERROR\n", result
    result = subprocess.run(
        [str(ROOT / "build/route-client-test")], input="PING\nADD_NODE 50\nADD_NODE 51\nADD_EDGE 50 51 7\nPRINT\nGET_NODE 50\nBOGUS\nPING\nQUIT\nADD_NODE 99\n",
        capture_output=True, text=True, timeout=3,
    )
    assert result.returncode == 0, result
    assert "========== Graph" in result.stdout and "Node 50 -> [51, weight=7]\n" in result.stdout, result
    assert result.stdout.endswith("NODE 50\nERROR\nPONG\n"), result
    result = subprocess.run([str(ROOT / "build/route-client-test"), "GET_NODE 99"], capture_output=True, text=True, timeout=3)
    assert result.returncode == 1, result
    # Bad input must not discard the next interactive command.
    result = subprocess.run([str(ROOT / "build/route-client-test")], input="X" * 2000 + "\nPING\n",
                            capture_output=True, text=True, timeout=3)
    assert result.returncode == 0 and result.stdout == "PONG\n", result
    # Graph output must not be truncated by the client's 1024-byte line buffer.
    commands = "".join(f"ADD_NODE {i}\n" for i in range(1000, 1200))
    result = subprocess.run([str(ROOT / "build/route-client-test")], input=commands + "PRINT\nPING\n",
                            capture_output=True, text=True, timeout=10)
    assert result.returncode == 0 and result.stdout.endswith("PONG\n"), result
    assert all(f"Node {i} -> (no edges)\n" in result.stdout for i in range(1000, 1200))
    result = subprocess.run([str(ROOT / "build/route-client-test"), "PRINT"], capture_output=True, text=True, timeout=3)
    assert result.returncode == 0 and "Node 1199 -> (no edges)\n" in result.stdout, result
    # One node's adjacency list can exceed a single client read buffer.
    commands = "".join(f"ADD_EDGE 1000 {i} 1\n" for i in range(1001, 1200))
    result = subprocess.run([str(ROOT / "build/route-client-test")], input=commands + "PRINT\nPING\n",
                            capture_output=True, text=True, timeout=10)
    assert result.returncode == 0 and result.stdout.endswith("PONG\n"), result
    assert "[1199, weight=1]" in result.stdout and "[1001, weight=1]" in result.stdout
    print("Network tests passed")
finally:
    server.terminate()
    _, errors = server.communicate(timeout=3)
    assert "ERROR: AddressSanitizer" not in errors, errors
    assert "runtime error:" not in errors, errors
    assert server.returncode in (-15, 0), (server.returncode, errors)
