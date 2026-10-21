#!/usr/bin/env python3
"""
Bypass for unlocknow.net / Linkvertise redirect links.
Extracts the final destination URL without going through ad walls.
"""

import re
import sys
import base64
import json
from urllib.parse import urlparse, parse_qs, unquote

import requests

SESSION = requests.Session()
SESSION.headers.update({
    "User-Agent": "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 "
                  "(KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36",
    "Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8",
    "Accept-Language": "en-US,en;q=0.9",
    "Referer": "https://www.google.com/",
})


def bypass_linkvertise(url: str) -> str | None:
    """Attempt multiple methods to extract the destination from a Linkvertise link."""

    # Method 1: thebypasser.com API
    dest = _try_thebypasser(url)
    if dest:
        return dest

    # Method 2: bypass.city API
    dest = _try_bypass_city(url)
    if dest:
        return dest

    # Method 3: direct page scrape + base64 decode
    dest = _try_direct_scrape(url)
    if dest:
        return dest

    # Method 4: lootlink/rekonise pattern
    dest = _try_loot_pattern(url)
    if dest:
        return dest

    return None


def bypass_unlocknow(url: str) -> str | None:
    """
    unlocknow.net wraps a Linkvertise or direct URL.
    Extract the inner link and bypass that.
    """
    try:
        resp = SESSION.get(url, allow_redirects=False, timeout=15)

        # Check Location header for redirect
        if resp.status_code in (301, 302, 303, 307, 308):
            redirect_url = resp.headers.get("Location", "")
            if redirect_url:
                if "linkvertise" in redirect_url:
                    return bypass_linkvertise(redirect_url)
                return redirect_url

        # Check page content for encoded URLs
        body = resp.text

        # Look for base64 encoded destination
        b64_matches = re.findall(r'[A-Za-z0-9+/]{20,}={0,2}', body)
        for match in b64_matches:
            decoded = _try_b64_decode(match)
            if decoded and decoded.startswith("http"):
                return decoded

        # Look for linkvertise URLs in the page
        lv_matches = re.findall(r'https?://linkvertise\.com/\S+', body)
        if lv_matches:
            return bypass_linkvertise(lv_matches[0].strip('"').strip("'"))

        # Look for any target URL pattern
        url_matches = re.findall(
            r'(?:href|url|redirect|destination|target)\s*[=:]\s*["\']?(https?://[^"\'\s>]+)',
            body, re.IGNORECASE
        )
        for u in url_matches:
            parsed = urlparse(u)
            if parsed.hostname and "unlocknow" not in parsed.hostname and "linkvertise" not in parsed.hostname:
                return u

        # Follow JS redirects
        js_redirects = re.findall(
            r'(?:window\.location|location\.href|location\.replace)\s*[\(=]\s*["\']?(https?://[^"\';\s\)]+)',
            body
        )
        for u in js_redirects:
            if "unlocknow" not in u and "linkvertise" not in u:
                return u
            if "linkvertise" in u:
                return bypass_linkvertise(u)

        # Try following all redirects
        resp2 = SESSION.get(url, allow_redirects=True, timeout=15)
        if resp2.url != url:
            final = resp2.url
            if "linkvertise" in final:
                return bypass_linkvertise(final)
            if "unlocknow" not in final:
                return final

    except Exception as e:
        print(f"[!] Error fetching unlocknow URL: {e}")

    return None


def _try_thebypasser(url: str) -> str | None:
    try:
        resp = SESSION.post(
            "https://api.bypass.vip/bypass",
            json={"url": url},
            timeout=30,
        )
        if resp.status_code == 200:
            data = resp.json()
            dest = data.get("destination") or data.get("result") or data.get("bypassed")
            if dest and dest.startswith("http"):
                return dest
    except Exception:
        pass
    return None


def _try_bypass_city(url: str) -> str | None:
    try:
        resp = SESSION.post(
            "https://api.bypass.city/bypass",
            json={"url": url},
            timeout=30,
        )
        if resp.status_code == 200:
            data = resp.json()
            dest = data.get("destination") or data.get("result")
            if dest and dest.startswith("http"):
                return dest
    except Exception:
        pass
    return None


def _try_direct_scrape(url: str) -> str | None:
    try:
        resp = SESSION.get(url, timeout=15)
        body = resp.text

        # Look for the target URL in data attributes or JSON
        json_matches = re.findall(r'\{[^{}]*"url"\s*:\s*"(https?://[^"]+)"[^{}]*\}', body)
        for u in json_matches:
            parsed = urlparse(u)
            if parsed.hostname and "linkvertise" not in parsed.hostname:
                return unquote(u)

        # Look for base64 in query params
        parsed_url = urlparse(url)
        params = parse_qs(parsed_url.query)
        for key, values in params.items():
            for val in values:
                decoded = _try_b64_decode(val)
                if decoded and decoded.startswith("http"):
                    return decoded

        # Look for encoded URL in path
        path_parts = parsed_url.path.strip("/").split("/")
        for part in path_parts:
            decoded = _try_b64_decode(part)
            if decoded and decoded.startswith("http"):
                return decoded

    except Exception:
        pass
    return None


def _try_loot_pattern(url: str) -> str | None:
    try:
        resp = SESSION.get(url, timeout=15)
        body = resp.text

        # LootLinks / Rekonise pattern: destination in a script tag
        patterns = [
            r'"destination"\s*:\s*"(https?://[^"]+)"',
            r'"targetUrl"\s*:\s*"(https?://[^"]+)"',
            r'"longurl"\s*:\s*"(https?://[^"]+)"',
            r'"target"\s*:\s*"(https?://[^"]+)"',
            r'data-url="(https?://[^"]+)"',
        ]
        for pat in patterns:
            m = re.search(pat, body)
            if m:
                found = unquote(m.group(1))
                if "linkvertise" not in found:
                    return found
    except Exception:
        pass
    return None


def _try_b64_decode(s: str) -> str | None:
    for candidate in [s, s + "=", s + "=="]:
        try:
            decoded = base64.b64decode(candidate).decode("utf-8", errors="ignore")
            if decoded.startswith("http"):
                return decoded
        except Exception:
            pass
    # Try URL-safe base64
    for candidate in [s, s + "=", s + "=="]:
        try:
            decoded = base64.urlsafe_b64decode(candidate).decode("utf-8", errors="ignore")
            if decoded.startswith("http"):
                return decoded
        except Exception:
            pass
    return None


def bypass(url: str) -> str | None:
    """Main entry point — detects the link type and bypasses it."""
    parsed = urlparse(url)
    host = (parsed.hostname or "").lower()

    if "unlocknow" in host:
        return bypass_unlocknow(url)
    elif "linkvertise" in host:
        return bypass_linkvertise(url)
    else:
        # Try generic bypass
        result = bypass_linkvertise(url)
        if not result:
            result = bypass_unlocknow(url)
        return result


def main():
    if len(sys.argv) < 2:
        print("Usage: python bypass.py <url>")
        print("Example: python bypass.py https://unlocknow.net/abc123")
        sys.exit(1)

    url = sys.argv[1]
    print(f"[*] Bypassing: {url}")

    result = bypass(url)

    if result:
        print(f"[+] Destination: {result}")
    else:
        print("[-] Could not extract destination URL.")
        print("    The link structure may have changed. Try updating the script.")


if __name__ == "__main__":
    main()
