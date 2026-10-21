import sqlite3
import os
import secrets
import string
import time

DB_PATH = os.path.join(os.path.dirname(__file__), "keys.db")

def get_conn():
    conn = sqlite3.connect(DB_PATH)
    conn.row_factory = sqlite3.Row
    conn.execute("PRAGMA journal_mode=WAL")
    return conn

def init_db():
    conn = get_conn()
    conn.execute("""
        CREATE TABLE IF NOT EXISTS keys (
            key         TEXT PRIMARY KEY,
            hwid        TEXT DEFAULT '',
            created_at  INTEGER NOT NULL,
            expires_at  INTEGER NOT NULL,
            created_by  TEXT DEFAULT '',
            note        TEXT DEFAULT '',
            used        INTEGER DEFAULT 0
        )
    """)
    conn.commit()
    conn.close()

def generate_key(duration_seconds, created_by="", note=""):
    alphabet = string.ascii_uppercase + string.digits
    key = "TRIDENT-" + "-".join(
        "".join(secrets.choice(alphabet) for _ in range(5))
        for _ in range(4)
    )
    now = int(time.time())
    expires = 0 if duration_seconds == 0 else now + duration_seconds
    conn = get_conn()
    conn.execute(
        "INSERT INTO keys (key, created_at, expires_at, created_by, note) VALUES (?, ?, ?, ?, ?)",
        (key, now, expires, created_by, note)
    )
    conn.commit()
    conn.close()
    return key, expires

def validate_key(key, hwid):
    conn = get_conn()
    row = conn.execute("SELECT * FROM keys WHERE key = ?", (key,)).fetchone()
    if not row:
        conn.close()
        return False, "Invalid key"

    now = int(time.time())
    if row["expires_at"] != 0 and now > row["expires_at"]:
        conn.close()
        return False, "Key expired"

    if row["hwid"] and row["hwid"] != hwid:
        conn.close()
        return False, "HWID mismatch — key bound to another machine"

    if not row["hwid"]:
        conn.execute("UPDATE keys SET hwid = ?, used = 1 WHERE key = ?", (hwid, key))
        conn.commit()
    elif not row["used"]:
        conn.execute("UPDATE keys SET used = 1 WHERE key = ?", (key,))
        conn.commit()

    conn.close()
    return True, row["expires_at"]

def delete_key(key):
    conn = get_conn()
    cur = conn.execute("DELETE FROM keys WHERE key = ?", (key,))
    conn.commit()
    conn.close()
    return cur.rowcount > 0

def reset_hwid(key):
    conn = get_conn()
    cur = conn.execute("UPDATE keys SET hwid = '', used = 0 WHERE key = ?", (key,))
    conn.commit()
    conn.close()
    return cur.rowcount > 0

def get_key_info(key):
    conn = get_conn()
    row = conn.execute("SELECT * FROM keys WHERE key = ?", (key,)).fetchone()
    conn.close()
    return dict(row) if row else None

def list_keys(limit=50):
    conn = get_conn()
    rows = conn.execute("SELECT * FROM keys ORDER BY created_at DESC LIMIT ?", (limit,)).fetchall()
    conn.close()
    return [dict(r) for r in rows]

def count_keys():
    conn = get_conn()
    row = conn.execute("SELECT COUNT(*) as c FROM keys").fetchone()
    active = conn.execute(
        "SELECT COUNT(*) as c FROM keys WHERE (expires_at = 0 OR expires_at > ?)",
        (int(time.time()),)
    ).fetchone()
    conn.close()
    return row["c"], active["c"]

init_db()
