import asyncio
import aiohttp
from aiohttp_socks import ProxyConnector
import json
import uuid
import time
import random
import base64
import string

USERPATH = "users.txt"
VALIDPATH = "valid.txt"
PROXYPATH = "proxies_working.txt"
CACHEPATH = "checked.txt"

REQUEST_TIMEOUT = 5
VERIFY_TIMEOUT = 4
WORKERS = 15
COOLDOWN = 20

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


def load_lines(path):
    try:
        with open(path, "r") as f:
            return [line.strip() for line in f if line.strip()]
    except FileNotFoundError:
        return []


def load_cache():
    return set(load_lines(CACHEPATH))


# ── proxy pool: never reuse same proxy back-to-back, instant drop on hang ──

class ProxyPool:
    def __init__(self, proxies):
        self.proxies = list(proxies)
        self.cooldowns = {}
        self.lock = asyncio.Lock()
        self.last_used = None

    async def get(self):
        for _ in range(60):
            async with self.lock:
                now = time.time()
                available = [
                    p for p in self.proxies
                    if now >= self.cooldowns.get(p, 0) and p != self.last_used
                ]
                if not available:
                    # allow last_used if it's the only option
                    available = [p for p in self.proxies if now >= self.cooldowns.get(p, 0)]
                if available:
                    proxy = random.choice(available)
                    self.last_used = proxy
                    self.cooldowns[proxy] = now + 1.0
                    return proxy
            await asyncio.sleep(0.5)
        return random.choice(self.proxies)

    async def cooldown(self, proxy, duration=COOLDOWN):
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


# ── verify proxies against discord with strict timeout ──

async def verify_one(proxy, sem, verified, count, total):
    async with sem:
        try:
            connector = ProxyConnector.from_url(proxy)
            async with aiohttp.ClientSession(connector=connector) as session:
                async with session.get(
                    "https://discord.com",
                    timeout=aiohttp.ClientTimeout(total=VERIFY_TIMEOUT),
                ) as resp:
                    count[0] += 1
                    if resp.status < 500:
                        verified.append(proxy)
                    print(f"  {count[0]}/{total} | {len(verified)} alive", end="\r")
        except Exception:
            count[0] += 1
            print(f"  {count[0]}/{total} | {len(verified)} alive", end="\r")


async def verify_proxies(proxies):
    print(f"Pre-checking {len(proxies)} proxies (timeout {VERIFY_TIMEOUT}s)...")
    verified = []
    count = [0]
    sem = asyncio.Semaphore(40)
    await asyncio.gather(*[verify_one(p, sem, verified, count, len(proxies)) for p in proxies])
    print(f"\n{len(verified)} proxies passed pre-check\n")
    return verified


# ── single request with strict timeout, instant drop on hang ──

async def make_request(username, proxy):
    try:
        connector = ProxyConnector.from_url(proxy)
        timeout = aiohttp.ClientTimeout(total=REQUEST_TIMEOUT)
        async with aiohttp.ClientSession(connector=connector) as session:
            async with session.post(
                URL,
                headers=build_headers(),
                data=json.dumps({"username": username}),
                timeout=timeout,
            ) as resp:
                body = await resp.text()
                return resp.status, body
    except (asyncio.TimeoutError, aiohttp.ClientError, OSError, ConnectionError):
        return None, None


# ── check one username: new proxy every attempt, exponential backoff on 429 ──

async def check_one(username, pool, sem, stats, file_lock, counter, total):
    async with sem:
        backoff = 1.0

        for attempt in range(1, 9):
            proxy = await pool.get()
            status, body = await make_request(username, proxy)

            # proxy hung or crashed — drop it instantly, next proxy
            if status is None:
                await pool.remove(proxy)
                alive = (await pool.status())[1]
                print(f"    [{username}] proxy dead, dropped | {alive} alive | attempt {attempt}")
                continue

            # rate limited — read Retry-After, exponential backoff, bench proxy
            if status == 429:
                retry_after = 1.0
                try:
                    retry_after = float(json.loads(body).get("retry_after", 1.0))
                except Exception:
                    pass
                wait = min(retry_after * backoff, 20)
                await pool.cooldown(proxy, COOLDOWN)
                avail, alive = await pool.status()
                print(f"    [{username}] 429 | wait {wait:.1f}s | {avail}/{alive} proxies | attempt {attempt}")
                await asyncio.sleep(wait)
                backoff = min(backoff * 2, 8)
                continue

            # success
            if status == 200:
                try:
                    taken = json.loads(body).get("taken", True)
                except Exception:
                    await pool.cooldown(proxy)
                    continue

                async with file_lock:
                    counter[0] += 1
                    n = counter[0]
                    with open(CACHEPATH, "a") as f:
                        f.write(username + "\n")

                avail, alive = await pool.status()
                if not taken:
                    stats["available"] += 1
                    print(f"  [{n}/{total}] >>>> VALID: {username} <<<< | {avail}/{alive}")
                    async with file_lock:
                        with open(VALIDPATH, "a") as f:
                            f.write(username + "\n")
                else:
                    stats["taken"] += 1
                    print(f"  [{n}/{total}] [TAKEN] {username} | {avail}/{alive}")
                return None

            # other http error — bench proxy, try next
            await pool.cooldown(proxy, 10)
            print(f"    [{username}] HTTP {status} | attempt {attempt}")
            continue

        # all attempts exhausted
        async with file_lock:
            counter[0] += 1
            n = counter[0]
        avail, alive = await pool.status()
        stats["retry"] += 1
        print(f"  [{n}/{total}] [RETRY] {username} | {avail}/{alive}")
        return username


async def run_pass(usernames, pool, stats, pass_num):
    print(f"=== Pass {pass_num}: {len(usernames)} usernames ===\n")
    sem = asyncio.Semaphore(WORKERS)
    file_lock = asyncio.Lock()
    counter = [0]

    tasks = []
    for u in usernames:
        tasks.append(asyncio.ensure_future(
            check_one(u, pool, sem, stats, file_lock, counter, len(usernames))
        ))
        await asyncio.sleep(random.uniform(0.05, 0.1))

    results = await asyncio.gather(*tasks)
    return [r for r in results if r is not None]


async def main():
    usernames = load_lines(USERPATH)
    raw_proxies = load_lines(PROXYPATH)

    if not usernames:
        print(f"No usernames in {USERPATH}")
        return
    if not raw_proxies:
        print(f"No proxies in {PROXYPATH}")
        return

    # skip already checked
    cache = load_cache()
    before = len(usernames)
    usernames = [u for u in usernames if u not in cache]
    if before != len(usernames):
        print(f"Skipped {before - len(usernames)} cached usernames\n")
    if not usernames:
        print("All usernames checked. Run gen.py for new ones.")
        return

    # pre-check: strict 4s timeout, only proxies that reach discord survive
    proxies = await verify_proxies(raw_proxies)
    if not proxies:
        print("No proxies can reach Discord.")
        return

    print(f"{len(usernames)} usernames | {len(proxies)} proxies | {WORKERS} workers")
    print(f"Request timeout: {REQUEST_TIMEOUT}s | Cooldown: {COOLDOWN}s\n")

    pool = ProxyPool(proxies)
    stats = {"available": 0, "taken": 0, "retry": 0}
    remaining = usernames

    for pass_num in range(1, 4):
        if not remaining:
            break
        remaining = await run_pass(remaining, pool, stats, pass_num)
        alive = (await pool.status())[1]
        if alive == 0:
            print("\nAll proxies dead. Get fresh proxies.")
            break
        if remaining and pass_num < 3:
            print(f"\n  {len(remaining)} need retry | {alive} proxies alive | waiting 20s...\n")
            await asyncio.sleep(20)

    print(f"\nDone! {stats['available']} available | {stats['taken']} taken | {len(remaining)} failed")
    if stats["available"] > 0:
        print(f"Valid usernames saved to {VALIDPATH}")


if __name__ == "__main__":
    asyncio.run(main())
