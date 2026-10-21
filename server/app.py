import asyncio
import json
import os

from fastapi import FastAPI, Request, WebSocket, WebSocketDisconnect
from fastapi.responses import HTMLResponse, JSONResponse
from fastapi.staticfiles import StaticFiles
from fastapi.templating import Jinja2Templates

from server.config import load_config, save_config
from server.conversations import (
    add_message,
    create_conversation,
    delete_conversation,
    get_conversation,
    list_conversations,
)
from server.ollama_client import list_models, pull_model, stream_chat

app = FastAPI(title="MyAI")

BASE_DIR = os.path.join(os.path.dirname(__file__), "..")
app.mount("/static", StaticFiles(directory=os.path.join(BASE_DIR, "static")), name="static")
templates = Jinja2Templates(directory=os.path.join(BASE_DIR, "templates"))


@app.get("/", response_class=HTMLResponse)
async def index(request: Request):
    return templates.TemplateResponse("index.html", {"request": request})


@app.get("/api/conversations")
async def api_list_conversations():
    return list_conversations()


@app.post("/api/conversations")
async def api_create_conversation(request: Request):
    body = await request.json()
    conv = create_conversation(body.get("title"))
    return conv


@app.get("/api/conversations/{conv_id}")
async def api_get_conversation(conv_id: str):
    conv = get_conversation(conv_id)
    if not conv:
        return JSONResponse({"error": "not found"}, status_code=404)
    return conv


@app.delete("/api/conversations/{conv_id}")
async def api_delete_conversation(conv_id: str):
    delete_conversation(conv_id)
    return {"ok": True}


@app.get("/api/config")
async def api_get_config():
    return load_config()


@app.post("/api/config")
async def api_save_config(request: Request):
    body = await request.json()
    config = load_config()
    config.update(body)
    save_config(config)
    return config


@app.get("/api/models")
async def api_list_models():
    models = await list_models()
    return {"models": models}


@app.post("/api/models/pull")
async def api_pull_model(request: Request):
    body = await request.json()
    model_name = body.get("model", "")
    if not model_name:
        return JSONResponse({"error": "model name required"}, status_code=400)
    ok = await pull_model(model_name)
    return {"ok": ok}


@app.websocket("/ws/chat/{conv_id}")
async def ws_chat(websocket: WebSocket, conv_id: str):
    await websocket.accept()

    try:
        while True:
            data = await websocket.receive_text()
            msg = json.loads(data)
            user_content = msg.get("content", "")

            if not user_content.strip():
                continue

            conv = get_conversation(conv_id)
            if not conv:
                conv = create_conversation()
                conv_id = conv["id"]
                await websocket.send_text(json.dumps({
                    "type": "conv_id",
                    "conv_id": conv_id,
                }))

            add_message(conv_id, "user", user_content)

            chat_messages = [
                {"role": m["role"], "content": m["content"]}
                for m in conv["messages"]
            ]
            chat_messages.append({"role": "user", "content": user_content})

            await websocket.send_text(json.dumps({"type": "start"}))

            async def send_token(token):
                await websocket.send_text(json.dumps({
                    "type": "token",
                    "content": token,
                }))

            try:
                full_response = await stream_chat(chat_messages, send_token)
                add_message(conv_id, "assistant", full_response)
                await websocket.send_text(json.dumps({"type": "done"}))
            except Exception as e:
                await websocket.send_text(json.dumps({
                    "type": "error",
                    "content": str(e),
                }))

    except WebSocketDisconnect:
        pass
