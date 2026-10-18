import asyncio
import aiohttp
from aiohttp_socks import ProxyConnector
import itertools
import json
import uuid
import time
import random
import base64
import string
import ipaddress
from urllib.parse import quote, urlparse

REQUEST_TIMEOUT = 5   # static proxies: good checks land in ~2-3.5s; past 5s retry elsewhere
ROTATING_TIMEOUT = 3.5  # Geonode residential: 90% answer within ~2s, the rest are hung exits
WORKERS = 5           # default parallel checks for static proxy lists
ROTATING_WORKERS = 10 # default for rotating gateways (fresh IP per connection)
MAX_WORKERS = 50
COOLDOWN = 20         # 429 cooldown per proxy
PROXY_RACE_SIZE = 3   # proxies tried at once per username, per round
MAX_ROUNDS = 5        # up to 15 different proxies per username — "no matter what"

URL = "https://discord.com/api/v9/unique-username/username-attempt-unauthed"

BROWSERS = [
    ("Firefox", "121.0", "Gecko/20100101 Firefox/121.0"),
    ("Firefox", "122.0", "Gecko/20100101 Firefox/122.0"),
    ("Chrome", "120.0.0.0", "AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36"),
    ("Chrome", "121.0.0.0", "AppleWebKit/537.36 (KHTML, like Gecko) Chrome/121.0.0.0 Safari/537.36"),
]

LOCALES = [
    ("fr", "fr,fr-FR;q=0.8,en-US;q=0.5,en;q=0.3", "Europe/Paris"),
    ("en-US", "en-US,en;q=0.9", "America/New_York"),
    ("en-GB", "en-GB,en;q=0.9", "Europe/London"),
    ("de", "de,de-DE;q=0.9,en;q=0.5", "Europe/Berlin"),
]

BUILD_NUMBERS = [260101, 260200, 260305, 261012, 261100]


def random_fingerprint():
    num = random.randint(1100000000000000000, 1300000000000000000)
    chars = string.ascii_letters + string.digits + "_-"
    suffix = ''.join(random.choice(chars) for _ in range(27))
    return f"{num}.{suffix}"


def build_super_properties():
    browser_name, browser_ver, ua_suffix = random.choice(BROWSERS)
    build = random.choice(BUILD_NUMBERS)
    locale_code, _, _ = random.choice(LOCALES)
    if "Firefox" in browser_name:
        ua = f"Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:{browser_ver}) {ua_suffix}"
    else:
        ua = f"Mozilla/5.0 (Windows NT 10.0; Win64; x64) {ua_suffix}"
    props = {
        "os": "Windows", "browser": browser_name, "device": "",
        "system_locale": locale_code, "browser_user_agent": ua,
        "browser_version": browser_ver, "os_version": "10",
        "referrer": "", "referring_domain": "",
        "referrer_current": "", "referring_domain_current": "",
        "release_channel": "stable", "client_build_number": build,
        "client_event_source": None,
    }
    return base64.b64encode(json.dumps(props).encode()).decode()


def build_headers():
    _, accept_lang, timezone = random.choice(LOCALES)
    locale_code = accept_lang.split(",")[0]
    return {
        "Accept": "*/*",
        "Accept-Language": accept_lang,
        "Accept-Encoding": "gzip, deflate, br",
        "X-Super-Properties": build_super_properties(),
        "X-Fingerprint": random_fingerprint(),
        "X-Discord-Locale": locale_code,
        "X-Discord-Timezone": timezone,
        "X-Debug-Options": "bugReporterEnabled",
        "Origin": "https://discord.com",
        "Referer": "https://discord.com/register",
        "Sec-Fetch-Dest": "empty",
        "Sec-Fetch-Mode": "cors",
        "Sec-Fetch-Site": "same-origin",
        "TE": "trailers",
        "Content-Type": "application/json",
        "Cookie": f"__dcfduid={uuid.uuid4()}; __sdcfduid={uuid.uuid4()}{uuid.uuid4()}",
    }


def _looks_like_host(s):
    return "." in s or s == "localhost"


def normalize_proxy(line):
    """Turn any common proxy line into a URL. Accepts scheme://user:pass@host:port,
    user:pass@host:port, host:port:user:pass, user:pass:host:port and host:port."""
    line = line.strip()
    if not line or "://" in line:
        return line
    tail = line.rsplit("@", 1)[1].split(":") if "@" in line else []
    at_form = len(tail) == 2 and tail[1].isdigit()  # user:pass@host:port
    if not at_form:
        parts = line.split(":")
        creds = None
        if len(parts) >= 4 and parts[1].isdigit() and _looks_like_host(parts[0]):
            host, port, creds = parts[0], parts[1], (parts[2], ":".join(parts[3:]))
        elif len(parts) >= 4 and parts[-1].isdigit() and _looks_like_host(parts[-2]):
            host, port, creds = parts[-2], parts[-1], (parts[0], ":".join(parts[1:-2]))
        if creds:
            user, pw = (quote(c, safe="") for c in creds)
            line = f"{user}:{pw}@{host}:{port}"
    return "http://" + line


def is_rotating(proxy):
    # a hostname gateway (e.g. dy01.glorycloud.com) rotates exit IPs per
    # connection; a bare IP is one fixed exit that Discord can rate-limit
    host = urlparse(proxy).hostname or ""
    try:
        ipaddress.ip_address(host)
        return False
    except ValueError:
        return True


class ProxyPool:
    def __init__(self, proxies):
        self.proxies = list(proxies)
        self.rotating = {p for p in self.proxies if is_rotating(p)}
        self.timeout = ROTATING_TIMEOUT if self.all_rotating else REQUEST_TIMEOUT
        self.cooldowns = {}
        self.lock = asyncio.Lock()
        self.last_used = None

    @property
    def all_rotating(self):
        return bool(self.proxies) and all(p in self.rotating for p in self.proxies)

    async def get(self, stop_event=None):
        for _ in range(60):
            if stop_event and stop_event.is_set():
                return None
            async with self.lock:
                now = time.time()
                available = [
                    p for p in self.proxies
                    if now >= self.cooldowns.get(p, 0) and p != self.last_used
                ]
                if not available:
                    available = [p for p in self.proxies if now >= self.cooldowns.get(p, 0)]
                if available:
                    proxy = random.choice(available)
                    self.last_used = proxy
                    if proxy not in self.rotating:
                        self.cooldowns[proxy] = now + 1.0
                    return proxy
            await asyncio.sleep(0.5)
        return random.choice(self.proxies) if self.proxies else None

    async def cooldown(self, proxy, duration=COOLDOWN):
        if proxy in self.rotating:
            return
        async with self.lock:
            self.cooldowns[proxy] = time.time() + duration

    async def remove(self, proxy):
        async with self.lock:
            if proxy in self.proxies:
                self.proxies.remove(proxy)

    async def status(self):
        async with self.lock:
            now = time.time()
            avail = sum(1 for p in self.proxies if now >= self.cooldowns.get(p, 0))
            return avail, len(self.proxies)


def safe_emit(callback, msg_type, data):
    if not callback:
        return
    try:
        callback(msg_type, data)
    except Exception:
        pass


async def interruptible_sleep(seconds, stop_event=None):
    """asyncio.sleep that bails out early the moment stop is requested,
    instead of a long uninterruptible wait ignoring the Stop button."""
    remaining = seconds
    while remaining > 0:
        if stop_event and stop_event.is_set():
            return
        chunk = min(0.25, remaining)
        await asyncio.sleep(chunk)
        remaining -= chunk


async def _request(username, proxy, timeout=REQUEST_TIMEOUT):
    connector = ProxyConnector.from_url(proxy) if proxy else None
    timeout = aiohttp.ClientTimeout(total=timeout)
    async with aiohttp.ClientSession(connector=connector) as session:
        async with session.post(
            URL,
            headers=build_headers(),
            data=json.dumps({"username": username}),
            timeout=timeout,
        ) as resp:
            body = await resp.text()
            return resp.status, body


def _swallow(task):
    if not task.cancelled():
        task.exception()


async def _with_deadline(coro, timeout):
    # aiohttp's timeout doesn't cover the proxy handshake, and a stuck
    # handshake can ignore cancellation — so wait_for() (which waits for the
    # cancel to land) could still pin a worker for a minute. Abandon the
    # task at the deadline instead and let it die in the background.
    task = asyncio.ensure_future(coro)
    try:
        done, _ = await asyncio.wait({task}, timeout=timeout)
    except asyncio.CancelledError:
        task.cancel()
        task.add_done_callback(_swallow)
        raise
    if not done:
        task.cancel()
        task.add_done_callback(_swallow)
        return None
    try:
        return task.result()
    except asyncio.CancelledError:
        raise
    except Exception:
        # any proxy/socket/handshake failure (timeouts, resets, malformed
        # SOCKS replies, TLS errors, etc.) just counts as a dead attempt
        return None


async def make_request(username, proxy, timeout=REQUEST_TIMEOUT):
    return await _with_deadline(_request(username, proxy, timeout), timeout) or (None, None)


async def _try_one_proxy(username, pool, stop_event=None):
    proxy = await pool.get(stop_event)
    if proxy is None:
        return None, None, None
    try:
        status, body = await make_request(username, proxy, pool.timeout)
    except asyncio.CancelledError:
        raise
    except Exception:
        status, body = None, None
    return proxy, status, body


async def check_one(username, pool, sem, stats, results_lock, counter, total, callback=None, stop_event=None):
    async with sem:
        safe_emit(callback, "checking", {"username": username, "total": total})

        # race a batch of different proxies at once for THIS username —
        # keep firing fresh batches until one gets through, then move on.
        # racing beats going one-at-a-time (each dead/slow proxy costs up
        # to REQUEST_TIMEOUT seconds; doing that sequentially for dozens
        # of proxies is what made this slow) without abandoning a username
        # after a single unlucky proxy.
        last_reason = "no proxy available"
        for round_num in range(1, MAX_ROUNDS + 1):
            if stop_event and stop_event.is_set():
                return None

            avail, alive = await pool.status()
            if alive == 0:
                break

            # racing exists for flaky static lists; a rotating gateway already
            # gives a fresh IP on retry, and every extra request costs data
            batch_n = 1 if pool.all_rotating else min(PROXY_RACE_SIZE, alive)
            results = await asyncio.gather(
                *[_try_one_proxy(username, pool, stop_event) for _ in range(batch_n)],
                return_exceptions=True,
            )

            if stop_event and stop_event.is_set():
                return None

            taken = None
            invalid = False
            reason = "no answer"
            for r in results:
                if isinstance(r, BaseException):
                    continue
                proxy, status, body = r

                if proxy is None:
                    continue
                if status == 400 and body and "50035" in body:
                    invalid = True  # Discord rejects the name itself (banned word / bad format)
                elif status is None:
                    # a hung or dropped connection — for residential
                    # rotating proxies, one bad request doesn't mean
                    # the exit IP is dead, so just cool it down briefly
                    # instead of removing it from the pool entirely
                    await pool.cooldown(proxy, 8)
                elif status == 429:
                    reason = "rate limited"
                    await pool.cooldown(proxy, COOLDOWN)
                elif status == 200:
                    try:
                        this_taken = json.loads(body).get("taken", True)
                    except Exception:
                        reason = "bad reply"
                        await pool.cooldown(proxy)
                        continue
                    if taken is None:
                        taken = this_taken  # first clean success in this batch wins
                else:
                    reason = f"HTTP {status}"
                    await pool.cooldown(proxy, 10)

            if taken is not None:
                async with results_lock:
                    counter[0] += 1
                    n = counter[0]

                avail, alive = await pool.status()
                payload = {
                    "username": username, "n": n, "total": total,
                    "avail": avail, "alive": alive,
                }
                if not taken:
                    stats["available"] += 1
                    safe_emit(callback, "valid", payload)
                else:
                    stats["taken"] += 1
                    safe_emit(callback, "taken", payload)
                return None

            if invalid:
                async with results_lock:
                    counter[0] += 1
                    n = counter[0]
                stats["retry"] += 1
                safe_emit(callback, "invalid", {"username": username, "n": n, "total": total})
                return None

            # a bad exit IP is routine with residential proxies — report it
            # quietly and try again, instead of a log line per attempt
            last_reason = reason
            safe_emit(callback, "retrying", {"username": username, "round": round_num, "reason": reason})

        async with results_lock:
            counter[0] += 1
            n = counter[0]
        stats["retry"] += 1
        safe_emit(callback, "retry", {"username": username, "n": n, "total": total, "reason": last_reason})
        return username


def _amount(total):
    return f"{total} usernames" if total else "endless"


async def run_direct(usernames, callback=None, stop_event=None, total=0):
    """No proxies at all — just check straight from this machine's own
    connection, one username at a time, respecting Discord's Retry-After
    on a 429 since there's no other identity to fall back on."""
    safe_emit(callback, "status", f"No proxies — checking directly | {_amount(total)}")

    stats = {"available": 0, "taken": 0, "retry": 0}

    for n, username in enumerate(usernames, start=1):
        if stop_event and stop_event.is_set():
            safe_emit(callback, "stopped", f"Stopped | {stats['available']} available | {stats['taken']} taken")
            return

        safe_emit(callback, "checking", {"username": username, "total": total})
        status, body = await make_request(username, None)

        if status == 429:
            retry_after = 2.0
            try:
                retry_after = float(json.loads(body).get("retry_after", 2.0))
            except Exception:
                pass
            wait = min(retry_after, 20)
            safe_emit(callback, "log", f"[{username}] 429 rate limited | waiting {wait:.1f}s")
            await interruptible_sleep(wait, stop_event)
            if stop_event and stop_event.is_set():
                safe_emit(callback, "stopped", f"Stopped | {stats['available']} available | {stats['taken']} taken")
                return
            status, body = await make_request(username, None)  # one retry after waiting it out

        if status == 200:
            try:
                taken = json.loads(body).get("taken", True)
            except Exception:
                taken = None
            if taken is not None:
                payload = {"username": username, "n": n, "total": total, "avail": 1, "alive": 1}
                if not taken:
                    stats["available"] += 1
                    safe_emit(callback, "valid", payload)
                else:
                    stats["taken"] += 1
                    safe_emit(callback, "taken", payload)
                await asyncio.sleep(0.3)  # be a good citizen — pace requests from one IP
                continue

        stats["retry"] += 1
        bad_name = status == 400 and body and "50035" in body
        safe_emit(callback, "invalid" if bad_name else "retry", {"username": username, "n": n, "total": total})
        await asyncio.sleep(0.3)

    safe_emit(callback, "done", f"Done! {stats['available']} available | {stats['taken']} taken | {stats['retry']} failed")


# ═══════════════════════════════════════════════════════
#  ROBLOX
# ═══════════════════════════════════════════════════════
# Roblox is checked in two steps:
#   1. users.roblox.com looks up 100 names per request through the proxy;
#      any name that belongs to an account is taken.
#   2. the rest go through Roblox's sign-up check (auth.roblox.com), which
#      also knows reserved / old / filtered names. Geonode blocks that host
#      ("464 Target Forbidden"), so this step runs from this PC directly.

ROBLOX_LOOKUP = "https://users.roblox.com/v1/usernames/users"
ROBLOX_VALIDATE = "https://auth.roblox.com/v1/usernames/validate"
ROBLOX_BATCH = 100
ROBLOX_TIMEOUT = 8


def _browser_ua():
    browser_name, browser_ver, ua_suffix = random.choice(BROWSERS)
    if browser_name == "Firefox":
        return f"Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:{browser_ver}) {ua_suffix}"
    return f"Mozilla/5.0 (Windows NT 10.0; Win64; x64) {ua_suffix}"


def _roblox_headers():
    return {
        "User-Agent": _browser_ua(),
        "Accept": "application/json",
        "Accept-Language": random.choice(LOCALES)[1],
        "Content-Type": "application/json",
        "Origin": "https://www.roblox.com",
        "Referer": "https://www.roblox.com/",
    }


async def _roblox_lookup(names, proxy):
    """→ (status, {lowercase names that belong to an account} or None)"""
    connector = ProxyConnector.from_url(proxy) if proxy else None
    async with aiohttp.ClientSession(connector=connector) as session:
        async with session.post(
            ROBLOX_LOOKUP,
            headers=_roblox_headers(),
            data=json.dumps({"usernames": names, "excludeBannedUsers": False}),
            timeout=aiohttp.ClientTimeout(total=ROBLOX_TIMEOUT),
        ) as resp:
            if resp.status != 200:
                return resp.status, None
            data = json.loads(await resp.text()).get("data", [])
            return 200, {d["requestedUsername"].lower() for d in data}


async def _roblox_validate(name):
    """→ (status, code) — code 0 = free, 1 = taken, anything else = not allowed"""
    params = {"request.username": name, "request.birthday": "2000-01-01T00:00:00.000Z",
              "request.context": "Signup"}
    async with aiohttp.ClientSession() as session:
        async with session.get(ROBLOX_VALIDATE, params=params, headers=_roblox_headers(),
                               timeout=aiohttp.ClientTimeout(total=ROBLOX_TIMEOUT)) as resp:
            if resp.status != 200:
                return resp.status, None
            return 200, json.loads(await resp.text()).get("code")


async def run_roblox(usernames, proxies, callback=None, stop_event=None, workers=None, total=0):
    names = iter(usernames)
    proxies = list(dict.fromkeys(normalize_proxy(p) for p in proxies if p.strip()))
    pool = ProxyPool(proxies) if proxies else None
    lookups = max(1, min(int(workers or 10), MAX_WORKERS) // 10)   # 100 names each
    verifiers = max(2, min(int(workers or 10), MAX_WORKERS) // 10)  # direct sign-up checks
    via = ("rotating gateway" if pool.all_rotating else f"{len(proxies)} proxies") if pool else "direct"
    safe_emit(callback, "status", f"Roblox | lookups via {via} | sign-up check from this PC | {_amount(total)}")

    stats = {"available": 0, "taken": 0, "retry": 0}
    counter = [0]
    queue = asyncio.Queue(maxsize=lookups * ROBLOX_BATCH * 2)
    # shared back-off when Roblox rate-limits this PC: short at first, longer
    # only while 429s keep coming back to back, reset by any clean answer
    throttle = {"until": 0.0, "streak": 0}

    def stopped():
        return bool(stop_event and stop_event.is_set())

    def finish(name, kind, reason=None):
        counter[0] += 1
        payload = {"username": name, "n": counter[0], "total": total}
        if kind == "free":
            stats["available"] += 1
            safe_emit(callback, "valid", payload)
        elif kind == "taken":
            stats["taken"] += 1
            safe_emit(callback, "taken", payload)
        elif kind == "invalid":
            stats["retry"] += 1
            safe_emit(callback, "invalid", payload)
        else:
            stats["retry"] += 1
            payload["reason"] = reason
            safe_emit(callback, "retry", payload)

    async def lookup_worker():
        while not stopped():
            batch = list(itertools.islice(names, ROBLOX_BATCH))
            if not batch:
                return
            safe_emit(callback, "checking", {"username": batch[0], "total": total})
            found, reason = None, "no answer"
            for attempt in range(1, MAX_ROUNDS + 1):
                if stopped():
                    return
                proxy = await pool.get(stop_event) if pool else None
                res = await _with_deadline(_roblox_lookup(batch, proxy), ROBLOX_TIMEOUT)
                if res and res[0] == 200:
                    found = res[1]
                    break
                reason = "no answer" if res is None else "rate limited" if res[0] == 429 else f"HTTP {res[0]}"
                safe_emit(callback, "retrying", {"username": batch[0], "round": attempt, "reason": reason})
                if pool:
                    await pool.cooldown(proxy, 8)
                else:
                    await interruptible_sleep(2 * attempt, stop_event)
            if found is None:
                for n in batch:
                    finish(n, "retry", f"lookup {reason}")
                continue
            for n in batch:
                if n.lower() in found:
                    finish(n, "taken")
                else:
                    await queue.put(n)

    async def verify_worker():
        while True:
            name = await queue.get()
            if name is None:
                return
            if stopped():
                continue  # drain without checking
            safe_emit(callback, "checking", {"username": name, "total": total})
            reason = "no answer"
            for attempt in range(1, 7):
                wait = throttle["until"] - time.monotonic()
                if wait > 0:
                    await interruptible_sleep(wait, stop_event)
                if stopped():
                    break
                res = await _with_deadline(_roblox_validate(name), ROBLOX_TIMEOUT)
                if res and res[0] == 200 and res[1] is not None:
                    throttle["streak"] = 0
                    finish(name, "free" if res[1] == 0 else "taken" if res[1] == 1 else "invalid")
                    break
                if res and res[0] == 429:
                    reason = "rate limited"
                    throttle["streak"] += 1
                    pause = min(30, 2 ** min(throttle["streak"], 5))
                    throttle["until"] = max(throttle["until"], time.monotonic() + pause)
                else:
                    reason = "no answer" if res is None else f"HTTP {res[0]}"
                    await interruptible_sleep(1, stop_event)
                safe_emit(callback, "retrying", {"username": name, "round": attempt, "reason": reason})
            else:
                finish(name, "retry", reason)

    try:
        checkers = [asyncio.ensure_future(verify_worker()) for _ in range(verifiers)]
        await asyncio.gather(*[lookup_worker() for _ in range(lookups)], return_exceptions=True)
        for _ in checkers:
            await queue.put(None)
        await asyncio.gather(*checkers, return_exceptions=True)
    except asyncio.CancelledError:
        raise
    except Exception as ex:
        safe_emit(callback, "log", f"Run crashed: {ex}")

    if stopped():
        safe_emit(callback, "stopped", f"Stopped | {stats['available']} available | {stats['taken']} taken")
    else:
        safe_emit(callback, "done", f"Done! {stats['available']} available | {stats['taken']} taken | {stats['retry']} skipped")


async def run_checker(usernames, proxies, callback=None, stop_event=None, workers=None, total=None, platform="discord"):
    # usernames may be a list or an endless iterator; total=0 means endless
    if total is None:
        total = len(usernames) if hasattr(usernames, "__len__") else 0
    if platform == "roblox":
        await run_roblox(usernames, proxies, callback, stop_event, workers, total)
        return
    names = iter(usernames)

    if not proxies:
        await run_direct(names, callback, stop_event, total)
        return

    # duplicate lines add nothing but extra racing requests
    proxies = list(dict.fromkeys(normalize_proxy(p) for p in proxies if p.strip()))
    pool = ProxyPool(proxies)
    if workers:
        workers = max(1, min(int(workers), MAX_WORKERS))
    else:
        workers = ROTATING_WORKERS if pool.all_rotating else WORKERS
    kind = "rotating gateway" if pool.all_rotating else f"{len(proxies)} proxies"
    safe_emit(callback, "status", f"{kind} | {_amount(total)} | {workers} workers")

    stats = {"available": 0, "taken": 0, "retry": 0}

    try:
        sem = asyncio.Semaphore(workers)
        results_lock = asyncio.Lock()
        counter = [0]

        # a fixed set of workers pulls names on demand, so the amount can
        # be anything (or endless) without building millions of tasks
        async def worker():
            while not (stop_event and stop_event.is_set()):
                u = next(names, None)
                if u is None:
                    return
                await check_one(u, pool, sem, stats, results_lock, counter, total, callback, stop_event)

        await asyncio.gather(*[worker() for _ in range(workers)], return_exceptions=True)
    except asyncio.CancelledError:
        raise
    except Exception as ex:
        safe_emit(callback, "log", f"Run crashed: {ex}")

    if stop_event and stop_event.is_set():
        if callback:
            callback("stopped", f"Stopped | {stats['available']} available | {stats['taken']} taken")
        return

    if callback:
        callback("done", f"Done! {stats['available']} available | {stats['taken']} taken | {stats['retry']} skipped")
