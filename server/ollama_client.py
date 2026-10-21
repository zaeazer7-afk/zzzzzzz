import httpx
import json
from typing import AsyncIterator

from server.config import load_config


async def stream_chat(messages: list[dict], on_token) -> str:
    config = load_config()
    url = f"{config['ollama_url']}/api/chat"

    payload = {
        "model": config["model"],
        "messages": [
            {"role": "system", "content": config["system_prompt"]},
            *messages,
        ],
        "stream": True,
        "options": {
            "temperature": config["temperature"],
            "num_predict": config["max_tokens"],
            "num_ctx": config["context_length"],
        },
    }

    full_response = ""
    async with httpx.AsyncClient(timeout=300.0) as client:
        async with client.stream("POST", url, json=payload) as resp:
            resp.raise_for_status()
            async for line in resp.aiter_lines():
                if not line.strip():
                    continue
                chunk = json.loads(line)
                if "message" in chunk and "content" in chunk["message"]:
                    token = chunk["message"]["content"]
                    full_response += token
                    await on_token(token)
                if chunk.get("done"):
                    break

    return full_response


async def list_models() -> list[str]:
    config = load_config()
    url = f"{config['ollama_url']}/api/tags"
    try:
        async with httpx.AsyncClient(timeout=10.0) as client:
            resp = await client.get(url)
            resp.raise_for_status()
            data = resp.json()
            return [m["name"] for m in data.get("models", [])]
    except Exception:
        return []


async def pull_model(model_name: str, on_progress=None) -> bool:
    config = load_config()
    url = f"{config['ollama_url']}/api/pull"
    payload = {"name": model_name, "stream": True}

    try:
        async with httpx.AsyncClient(timeout=600.0) as client:
            async with client.stream("POST", url, json=payload) as resp:
                resp.raise_for_status()
                async for line in resp.aiter_lines():
                    if not line.strip():
                        continue
                    chunk = json.loads(line)
                    if on_progress and "status" in chunk:
                        await on_progress(chunk["status"])
        return True
    except Exception:
        return False
