import json
import os
import time

VAULT_FILENAME = "vault.json"


def _path(project_dir):
    return os.path.join(project_dir, VAULT_FILENAME)


def load_vault(project_dir):
    path = _path(project_dir)
    if not os.path.exists(path):
        return []
    try:
        with open(path, "r", encoding="utf-8") as f:
            data = json.load(f)
        return data if isinstance(data, list) else []
    except (json.JSONDecodeError, OSError):
        return []


def save_vault(project_dir, listings):
    with open(_path(project_dir), "w", encoding="utf-8") as f:
        json.dump(listings, f, indent=2)


def add_listing(project_dir, username, price, notes=""):
    listings = load_vault(project_dir)
    if any(l["username"].lower() == username.lower() for l in listings):
        return listings, False
    listings.insert(0, {
        "username": username,
        "price": price,
        "notes": notes,
        "added": time.strftime("%Y-%m-%d"),
    })
    save_vault(project_dir, listings)
    return listings, True


def add_many(project_dir, usernames, notes=""):
    # one read + one write, instead of add_listing's per-name round trip
    listings = load_vault(project_dir)
    have = {l["username"].lower() for l in listings}
    today = time.strftime("%Y-%m-%d")
    added = 0
    for name in usernames:
        name = (name or "").strip().lstrip("@")
        if name and name.lower() not in have:
            have.add(name.lower())
            listings.insert(0, {"username": name, "price": "", "notes": notes, "added": today})
            added += 1
    if added:
        save_vault(project_dir, listings)
    return added


def remove_listing(project_dir, username):
    listings = load_vault(project_dir)
    listings = [l for l in listings if l["username"] != username]
    save_vault(project_dir, listings)
    return listings
