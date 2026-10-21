import json
import os
import time
import uuid

DATA_DIR = os.path.join(os.path.dirname(__file__), "..", "data", "conversations")


def _ensure_dir():
    os.makedirs(DATA_DIR, exist_ok=True)


def create_conversation(title=None):
    _ensure_dir()
    conv_id = str(uuid.uuid4())[:8]
    conv = {
        "id": conv_id,
        "title": title or f"Chat {conv_id}",
        "created": time.time(),
        "messages": [],
    }
    _save(conv_id, conv)
    return conv


def get_conversation(conv_id):
    path = os.path.join(DATA_DIR, f"{conv_id}.json")
    if not os.path.exists(path):
        return None
    with open(path, "r") as f:
        return json.load(f)


def list_conversations():
    _ensure_dir()
    convs = []
    for fname in os.listdir(DATA_DIR):
        if fname.endswith(".json"):
            path = os.path.join(DATA_DIR, fname)
            with open(path, "r") as f:
                data = json.load(f)
                convs.append({
                    "id": data["id"],
                    "title": data["title"],
                    "created": data["created"],
                    "message_count": len(data["messages"]),
                })
    convs.sort(key=lambda c: c["created"], reverse=True)
    return convs


def add_message(conv_id, role, content):
    conv = get_conversation(conv_id)
    if not conv:
        conv = create_conversation()
        conv_id = conv["id"]
    conv["messages"].append({
        "role": role,
        "content": content,
        "timestamp": time.time(),
    })
    _save(conv_id, conv)
    return conv


def delete_conversation(conv_id):
    path = os.path.join(DATA_DIR, f"{conv_id}.json")
    if os.path.exists(path):
        os.remove(path)


def _save(conv_id, conv):
    _ensure_dir()
    path = os.path.join(DATA_DIR, f"{conv_id}.json")
    with open(path, "w") as f:
        json.dump(conv, f, indent=2)
