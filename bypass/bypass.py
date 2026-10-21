#!/usr/bin/env python3
"""
Bypass for unlocknow.net / launcherleaks.net / Linkvertise redirect links.
Both unlocknow and launcherleaks redirect to Linkvertise — this tool
follows the chain and extracts the final destination URL.
"""

import re
import sys
import base64
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

WRAPPER_DOMAINS = {"unlocknow", "launcherleaks"}


def follow_to_linkvertise(url: str) -> str:
    """Follow unlocknow.net / launcherleaks.net redirects until we hit Linkvertise or a final URL."""
    try:
        # First try without following redirects to grab Location header
        resp = SESSION.get(url, allow_redirects=False, timeout=15)
        if resp.status_code in (301, 302, 303, 307, 308):
            location = resp.headers.get("Location", "")
            if location:
                return location

        # Follow full redirect chain
        resp = SESSION.get(url, allow_redirects=True, timeout=15)
        if resp.url != url:
            return resp.url

        # Scrape the page for the Linkvertise URL
        body = resp.text

        lv_matches = re.findall(r'(https?://linkvertise\.com/\S+)', body)
        if lv_matches:
            return lv_matches[0].strip('"').strip("'").rstrip("\\")

        # JS redirects
        js_matches = re.findall(
            r'(?:window\.location|location\.href|location\.replace)\s*[\(=]\s*["\']?(https?://[^"\';\s\)]+)',
            body
        )
        if js_matches:
            return js_matches[0]

        # href links that aren't the wrapper site itself
        href_matches = re.findall(r'href="(https?://[^"]+)"', body)
        for href in href_matches:
            parsed = urlparse(href)
            if parsed.hostname and not any(w in parsed.hostname for w in WRAPPER_DOMAINS):
                return href

    except Exception as e:
        print(f"[!] Error following redirect: {e}")

    return url


def bypass_linkvertise(url: str) -> str | None:
    """Extract the destination URL from a Linkvertise link."""

    # Method 1: bypass API services
    for api_fn in [_try_bypass_vip, _try_bypass_city]:
        dest = api_fn(url)
        if dest:
            return dest

    # Method 2: direct scrape
    dest = _try_scrape_linkvertise(url)
    if dest:
        return dest

    # Method 3: URL-encoded destination in the link itself
    dest = _try_url_decode(url)
    if dest:
        return dest

    return None


def _try_bypass_vip(url: str) -> str | None:
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


def _try_scrape_linkvertise(url: str) -> str | None:
    try:
        resp = SESSION.get(url, timeout=15)
        body = resp.text

        # JSON embedded destination
        patterns = [
            r'"destination"\s*:\s*"(https?://[^"]+)"',
            r'"targetUrl"\s*:\s*"(https?://[^"]+)"',
            r'"longurl"\s*:\s*"(https?://[^"]+)"',
            r'"target"\s*:\s*"(https?://[^"]+)"',
            r'"url"\s*:\s*"(https?://[^"]+)"',
            r'data-url="(https?://[^"]+)"',
        ]
        for pat in patterns:
            m = re.search(pat, body)
            if m:
                found = unquote(m.group(1))
                if "linkvertise" not in found:
                    return found

        # Base64 encoded strings in the page
        b64_matches = re.findall(r'[A-Za-z0-9+/]{20,}={0,2}', body)
        for match in b64_matches:
            decoded = _try_b64_decode(match)
            if decoded and decoded.startswith("http") and "linkvertise" not in decoded:
                return decoded

        # File hosting links
        hosting_pattern = (
            r'href="(https?://(?:mega\.nz|mediafire\.com|drive\.google\.com|'
            r'cdn\.discordapp\.com|github\.com|dropbox\.com|gofile\.io|'
            r'pixeldrain\.com|anonfiles\.com|workink\.click|'
            r'filesupload\.org|catbox\.moe|krakenfiles\.com)[^"]*)"'
        )
        matches = re.findall(hosting_pattern, body, re.IGNORECASE)
        if matches:
            return unquote(matches[0])

    except Exception:
        pass
    return None


def _try_url_decode(url: str) -> str | None:
    parsed = urlparse(url)

    # Check query params for encoded URLs
    params = parse_qs(parsed.query)
    for values in params.values():
        for val in values:
            decoded = _try_b64_decode(val)
            if decoded and decoded.startswith("http"):
                return decoded
            if val.startswith("http") and "linkvertise" not in val:
                return unquote(val)

    # Check path segments
    for part in parsed.path.strip("/").split("/"):
        decoded = _try_b64_decode(part)
        if decoded and decoded.startswith("http"):
            return decoded

    return None


def _try_b64_decode(s: str) -> str | None:
    for decode_fn in [base64.b64decode, base64.urlsafe_b64decode]:
        for candidate in [s, s + "=", s + "=="]:
            try:
                decoded = decode_fn(candidate).decode("utf-8", errors="ignore")
                if decoded.startswith("http"):
                    return decoded
            except Exception:
                pass
    return None


def bypass(url: str) -> str | None:
    """Main entry point — handles any supported link type."""
    parsed = urlparse(url)
    host = (parsed.hostname or "").lower()

    # unlocknow.net and launcherleaks.net both redirect to Linkvertise
    if any(w in host for w in WRAPPER_DOMAINS):
        linkvertise_url = follow_to_linkvertise(url)
        if "linkvertise" in linkvertise_url:
            return bypass_linkvertise(linkvertise_url)
        # If it redirected to something else entirely, that's the destination
        if not any(w in linkvertise_url for w in WRAPPER_DOMAINS):
            return linkvertise_url
        return None

    if "linkvertise" in host:
        return bypass_linkvertise(url)

    # Unknown domain — try all methods
    result = bypass_linkvertise(url)
    return result


def main():
    if len(sys.argv) < 2:
        print("Usage: python bypass.py <url>")
        print()
        print("Supported sites:")
        print("  - linkvertise.com")
        print("  - unlocknow.net (redirects to Linkvertise)")
        print("  - launcherleaks.net (redirects to Linkvertise)")
        print()
        print("Example: python bypass.py https://unlocknow.net/abc123")
        sys.exit(1)

    url = sys.argv[1]
    if not url.startswith("http"):
        url = "https://" + url

    print(f"[*] Bypassing: {url}")

    result = bypass(url)

    if result:
        print(f"[+] Destination: {result}")
    else:
        print("[-] Could not extract destination URL.")
        print("    The link structure may have changed. Try updating the script.")


if __name__ == "__main__":
    main()
