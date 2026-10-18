import os
import sys
import time
import random
import string
import asyncio
import itertools
import threading
import logging
import subprocess
from collections import Counter, deque

import webview
import pyperclip

# pywebview/pythonnet's WinForms interop occasionally trips a harmless
# UI Automation recursion bug; keep its error logging quiet.
logging.getLogger("pywebview").setLevel(logging.CRITICAL)

from generators import FORMATS, FORMAT_BY_ID, VALID_FOR, Sweep, generate, stream
from checker_core import run_checker, make_request, normalize_proxy, is_rotating
import vault_store

if getattr(sys, "frozen", False):
    BASE_DIR = os.path.dirname(sys.executable)
    WEB_DIR = os.path.join(sys._MEIPASS, "web")
    PROJECT_DIR = BASE_DIR
    # an exe left in app\dist shares the project's checked.txt / proxies /
    # vault, so it doesn't start from zero and re-check old names
    _repo_root = os.path.abspath(os.path.join(BASE_DIR, "..", ".."))
    if os.path.basename(BASE_DIR).lower() == "dist" and os.path.exists(os.path.join(_repo_root, "checked.txt")):
        PROJECT_DIR = _repo_root
else:
    BASE_DIR = os.path.dirname(os.path.abspath(__file__))
    WEB_DIR = os.path.join(BASE_DIR, "web")
    PROJECT_DIR = os.path.abspath(os.path.join(BASE_DIR, ".."))

EXPORT_DIR = os.path.join(PROJECT_DIR, "exports")
VALID_PATH = os.path.join(PROJECT_DIR, "valid.txt")
CHECKED_PATH = os.path.join(PROJECT_DIR, "checked.txt")
ROBLOX_VALID_PATH = os.path.join(PROJECT_DIR, "roblox_valid.txt")
ROBLOX_CHECKED_PATH = os.path.join(PROJECT_DIR, "roblox_checked.txt")
MAX_FILE_NAMES = 1_000_000


def files_for(platform):
    # a name checked on Discord says nothing about Roblox, so each keeps its own lists
    if platform == "roblox":
        return ROBLOX_VALID_PATH, ROBLOX_CHECKED_PATH
    return VALID_PATH, CHECKED_PATH


def mask_proxy(proxy):
    # never echo the proxy password back into the UI
    if "@" not in proxy:
        return proxy
    scheme, rest = proxy.split("://", 1) if "://" in proxy else ("http", proxy)
    creds, host = rest.rsplit("@", 1)
    user = creds.split(":", 1)[0]
    return f"{scheme}://{user}:•••@{host}"


def read_names(path):
    if not os.path.exists(path):
        return []
    with open(path, encoding="utf-8", errors="replace") as f:
        return [l.strip() for l in f if l.strip()]


def append_name(path, name):
    with open(path, "a", encoding="utf-8") as f:
        f.write(name + "\n")


class Api:
    def __init__(self):
        self.stop_event = None
        self.running = False
        # the page pulls events instead of Python pushing them with
        # evaluate_js — pushing from a worker thread blocks whenever the
        # WinForms UI thread is busy, which froze the app on "starting…"
        self._events = deque()
        self._events_lock = threading.Lock()
        self._retries = Counter()
        self._origin = {}
        self._valid_set = set()
        self._checked_set = set()

    # ── formats ──

    def get_formats(self, platform="discord"):
        checked = set(read_names(files_for(platform)[1]))
        out = []
        for f in FORMATS:
            if platform not in f["platforms"]:
                continue
            d = {k: v for k, v in f.items() if k not in ("gen", "odds", "platforms")}
            d["odds"] = f["odds"][platform]
            # "Best odds" = whatever measured 50%+ free on this platform
            d["picks"] = [p for p in f["picks"] if p != "best"] + (["best"] if (d["odds"] or 0) >= 50 else [])
            if isinstance(f["gen"], Sweep):
                done = sum(1 for n in checked if f["gen"].matches(n))
                d.update(sweep=True, size=f["gen"].size, left=max(0, f["gen"].size - done))
            out.append(d)
        return out

    @staticmethod
    def _selected(format_ids, platform="discord"):
        return {fid: FORMAT_BY_ID[fid]["gen"] for fid in (format_ids or [])
                if fid in FORMAT_BY_ID and platform in FORMAT_BY_ID[fid]["platforms"]}

    # ── initial state ──

    def get_state(self):
        proxies_text = ""
        for fname in ("proxies.txt", "proxies_working.txt"):
            path = os.path.join(PROJECT_DIR, fname)
            if os.path.exists(path):
                with open(path, encoding="utf-8") as f:
                    proxies_text = f.read()
                break
        return {
            "proxies_text": proxies_text,
            "vault": vault_store.load_vault(PROJECT_DIR),
            "running": self.running,
        }

    # ── proxies ──

    def save_proxies(self, text):
        lines = [l.strip() for l in text.splitlines() if l.strip()]
        with open(os.path.join(PROJECT_DIR, "proxies.txt"), "w", encoding="utf-8") as f:
            for p in lines:
                f.write(p + "\n")
        return {"count": len(lines)}

    def load_proxies(self, filename):
        if filename not in ("proxies.txt", "proxies_working.txt"):
            return ""
        path = os.path.join(PROJECT_DIR, filename)
        if not os.path.exists(path):
            return ""
        with open(path, encoding="utf-8") as f:
            return f.read()

    def test_proxies(self, text):
        unique = list(dict.fromkeys(normalize_proxy(l) for l in text.splitlines() if l.strip()))[:25]
        if not unique:
            return []

        async def probe(proxy):
            name = "".join(random.choice(string.ascii_lowercase) for _ in range(10))
            t0 = time.perf_counter()
            status, body = await make_request(name, proxy)
            ms = int((time.perf_counter() - t0) * 1000)
            ok = status == 200 and body is not None and '"taken"' in body
            return {
                "proxy": mask_proxy(proxy),
                "ok": ok,
                "status": status,
                "ms": ms,
                "rotating": is_rotating(proxy),
            }

        async def run_all():
            return await asyncio.gather(*[probe(p) for p in unique])

        return asyncio.run(run_all())

    # ── generate + check ──

    def start_checking(self, mode, formats, count, proxies_text, workers=None, skip_checked=True, list_text="",
                       platform="discord"):
        if self.running:
            return {"ok": False, "error": "already running"}

        proxies = [l.strip() for l in proxies_text.splitlines() if l.strip()]
        valid_path, checked_path = files_for(platform)
        self._checked_set = set(read_names(checked_path))
        self._valid_set = set(read_names(valid_path))
        self._origin = {}
        valid = VALID_FOR[platform]

        if mode == "list":
            # listed names are checked as-is (minus @ and blanks) — re-checking
            # a known name is the point here, so checked.txt isn't skipped
            names = (l.strip().lstrip("@").lower() for l in (list_text or "").replace(",", "\n").splitlines())
            usernames = list(dict.fromkeys(n for n in names if n))
            if not usernames:
                return {"ok": False, "error": "add some names to check first"}
            total = len(usernames)
        else:
            selected = self._selected(formats, platform)
            if not selected:
                return {"ok": False, "error": "pick at least one format"}
            # the stream skips names as they get checked, since it shares this set
            exclude = self._checked_set if skip_checked else frozenset()
            count = max(0, int(count or 0))  # 0 = endless
            sweeps = [g for g in selected.values() if isinstance(g, Sweep)]
            if len(sweeps) == len(selected):
                # a sweep has a known end, so ∞ means "everything that's left"
                left = sum(g.size - sum(1 for n in exclude if g.matches(n)) for g in sweeps)
                if not left:
                    return {"ok": False, "error": "every name in these styles has been checked already"}
                count = min(count, left) if count else left
            if count and count <= 50000:
                usernames = generate(count, selected, exclude, self._origin, valid)
                if not usernames:
                    return {"ok": False, "error": "every name in these formats has been checked already"}
                total = len(usernames)
            else:
                names = stream(selected, exclude, self._origin, valid)
                usernames = itertools.islice(names, count) if count else names
                total = count

        with self._events_lock:
            self._events.clear()
            self._retries.clear()
        self.running = True
        self.stop_event = threading.Event()
        threading.Thread(
            target=self._run_checker_thread,
            args=(usernames, proxies, self.stop_event, workers, total, platform),
            daemon=True,
        ).start()
        return {"ok": True, "total": total}

    def stop_checking(self):
        if self.stop_event:
            self.stop_event.set()
        return {"ok": True}

    def poll_events(self):
        with self._events_lock:
            events = list(self._events)
            self._events.clear()
            retries = dict(self._retries)
        return {"events": events, "running": self.running, "retries": retries}

    def _push(self, msg_type, data):
        with self._events_lock:
            # retries happen constantly on residential proxies — the UI only
            # needs running totals per reason, not one event each
            if msg_type == "retrying":
                self._retries[data["reason"]] += 1
            # only the newest "checking" matters to the UI
            elif msg_type == "checking" and self._events and self._events[-1]["type"] == "checking":
                self._events[-1] = {"type": msg_type, "data": data}
            else:
                self._events.append({"type": msg_type, "data": data})

    def _run_checker_thread(self, usernames, proxies, stop_event, workers=None, total=None, platform="discord"):
        valid_path, checked_path = files_for(platform)

        def emit(msg_type, data):
            if msg_type in ("valid", "taken", "retry", "invalid") and isinstance(data, dict):
                fid = self._origin.pop(data["username"], None)
                if fid:
                    data["fmt"] = FORMAT_BY_ID[fid]["code"]
            if msg_type in ("valid", "taken"):
                name = data["username"]
                if msg_type == "valid" and name not in self._valid_set:
                    self._valid_set.add(name)
                    append_name(valid_path, name)
                if name not in self._checked_set:
                    self._checked_set.add(name)
                    append_name(checked_path, name)
            self._push(msg_type, data)

        loop = asyncio.new_event_loop()
        asyncio.set_event_loop(loop)
        try:
            loop.run_until_complete(run_checker(usernames, proxies, emit, stop_event, workers, total, platform))
        except Exception as ex:
            emit("error", str(ex))
        finally:
            loop.close()
            self.running = False

    # ── files ──

    def _write_export(self, label, names):
        os.makedirs(EXPORT_DIR, exist_ok=True)
        safe = "".join(c for c in label if c.isalnum() or c in "-_") or "names"
        path = os.path.join(EXPORT_DIR, f"{safe}_{time.strftime('%Y-%m-%d_%H%M%S')}.txt")
        with open(path, "w", encoding="utf-8") as f:
            f.write("\n".join(names) + "\n")
        return {"ok": True, "count": len(names), "path": path,
                "file": os.path.relpath(path, PROJECT_DIR)}

    def generate_to_file(self, formats, count, skip_checked=True, platform="discord"):
        selected = self._selected(formats, platform)
        if not selected:
            return {"ok": False, "error": "pick at least one format"}
        count = int(count or 0)
        if not 1 <= count <= MAX_FILE_NAMES:
            return {"ok": False, "error": "pick an amount from 1 to 1,000,000"}
        exclude = set(read_names(files_for(platform)[1])) if skip_checked else frozenset()
        names = generate(count, selected, exclude, valid=VALID_FOR[platform])
        if not names:
            return {"ok": False, "error": "every name in these formats has been checked already"}
        return self._write_export("roblox_names" if platform == "roblox" else "names", names)

    def save_names(self, names, label="available"):
        names = [n.strip() for n in (names or []) if isinstance(n, str) and n.strip()]
        if not names:
            return {"ok": False, "error": "nothing to save"}
        return self._write_export(label, names)

    def reveal(self, path):
        # only ever opens Explorer on files inside the project folder
        path = os.path.abspath(path or "")
        root = os.path.abspath(PROJECT_DIR)
        try:
            inside = os.path.commonpath([path, root]) == root
        except ValueError:
            inside = False
        if not inside or not os.path.isfile(path):
            return {"ok": False}
        subprocess.Popen(f'explorer /select,"{path}"')
        return {"ok": True}

    def get_all_hits(self, platform="discord"):
        # newest first, each name once
        return list(dict.fromkeys(reversed(read_names(files_for(platform)[0]))))

    # ── clipboard ──

    def copy_text(self, text):
        pyperclip.copy(text or "")
        return {"ok": True}

    # ── vault ──

    def vault_list(self):
        return vault_store.load_vault(PROJECT_DIR)

    def vault_add(self, username, price, notes):
        username = (username or "").strip().lstrip("@")
        if not username:
            return {"added": False}
        _, added = vault_store.add_listing(PROJECT_DIR, username, price, notes)
        return {"added": added}

    def vault_add_many(self, names, notes=""):
        return {"added": vault_store.add_many(PROJECT_DIR, names or [], notes)}

    def vault_remove(self, username):
        vault_store.remove_listing(PROJECT_DIR, username)
        return {"ok": True}

    def vault_import_valid(self):
        return {"added": vault_store.add_many(PROJECT_DIR, read_names(VALID_PATH))}

    def vault_copy_all(self):
        listings = vault_store.load_vault(PROJECT_DIR)
        lines = []
        for l in listings:
            price = f"${l['price']}" if l.get("price") else "DM for price"
            line = f"@{l['username']} — {price}"
            if l.get("notes"):
                line += f" ({l['notes']})"
            lines.append(line)
        if lines:
            pyperclip.copy("\n".join(lines))
        return {"count": len(lines)}


def silence_dotnet_popups():
    # swallow WinForms UI-thread exceptions (the UI Automation recursion
    # bug) instead of showing the "Unhandled exception" dialog
    if sys.platform != "win32":
        return
    try:
        from webview.platforms import winforms as wf
        from System.Threading import ThreadExceptionEventHandler
        app = wf.WinForms.Application
        app.SetUnhandledExceptionMode(wf.WinForms.UnhandledExceptionMode.CatchException)
        app.ThreadException += ThreadExceptionEventHandler(lambda sender, args: None)
    except Exception:
        pass


def main():
    api = Api()
    webview.create_window(
        "NameVault",
        url=os.path.join(WEB_DIR, "index.html"),
        js_api=api,
        width=1280,
        height=820,
        min_size=(1080, 700),
        background_color="#07070c",
    )
    silence_dotnet_popups()
    icon_path = os.path.join(BASE_DIR, "icon.ico")
    webview.start(icon=icon_path if os.path.exists(icon_path) else None)


if __name__ == "__main__":
    main()
