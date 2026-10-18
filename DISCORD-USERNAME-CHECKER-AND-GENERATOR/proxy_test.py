import requests
import sys
import time
from concurrent.futures import ThreadPoolExecutor, as_completed

PROXYPATH = "proxies.txt"
OUTPUTPATH = "proxies_working.txt"
TEST_URL = "https://httpbin.org/ip"
TIMEOUT = 8
WORKERS = 30


def load_proxies(path):
    try:
        with open(path, "r") as f:
            lines = [line.strip() for line in f if line.strip()]
    except FileNotFoundError:
        return []
    # entries like "user:pass@host:port" have no scheme -- requests rejects
    # those outright (InvalidProxyURL), which the bare except below would
    # silently count as "dead". default bare entries to http://.
    return [p if "://" in p else f"http://{p}" for p in lines]


def test_proxy(proxy):
    try:
        resp = requests.get(
            TEST_URL,
            proxies={"http": proxy, "https": proxy},
            timeout=TIMEOUT,
        )
        if resp.status_code == 200:
            return proxy, True, resp.elapsed.total_seconds()
    except Exception:
        pass
    return proxy, False, 0


def main():
    proxies = load_proxies(PROXYPATH)
    if not proxies:
        print(f"No proxies found in {PROXYPATH}")
        return

    print(f"Testing {len(proxies)} proxies with {WORKERS} threads...")
    print(f"Timeout: {TIMEOUT}s per proxy\n")

    working = []
    tested = 0
    start = time.time()

    with ThreadPoolExecutor(max_workers=WORKERS) as pool:
        futures = {pool.submit(test_proxy, p): p for p in proxies}
        for future in as_completed(futures):
            tested += 1
            proxy, alive, latency = future.result()
            if alive:
                working.append((proxy, latency))
                print(f"[{tested}/{len(proxies)}] [OK {latency:.1f}s] {proxy}")
            else:
                print(f"[{tested}/{len(proxies)}] [DEAD] {proxy}", end="\r")

    elapsed = time.time() - start
    working.sort(key=lambda x: x[1])

    with open(OUTPUTPATH, "w") as f:
        for proxy, _ in working:
            f.write(proxy + "\n")

    print(f"\n\nDone in {elapsed:.0f}s")
    print(f"Working: {len(working)}/{len(proxies)}")
    print(f"Saved to {OUTPUTPATH}")


if __name__ == "__main__":
    main()
