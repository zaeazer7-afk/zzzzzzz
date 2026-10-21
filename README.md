# MyAI

Your own self-hosted AI assistant. Runs locally via Ollama — no API keys, no rate limits, no filters. You own the model and the system prompt.

## Setup (Windows)

### 1. Install Ollama

Download and install from [ollama.com](https://ollama.com/download).

### 2. Pull a model

Open a terminal and run:

```bash
ollama pull qwen2.5-coder:7b
```

This downloads a 7B coding model that fits your 1660 Super (6GB VRAM). Takes ~5 minutes on a decent connection.

Other models you can try:
- `ollama pull llama3.1:8b` — general purpose all-rounder
- `ollama pull deepseek-coder-v2:16b` — stronger coder, needs 12GB+ VRAM
- `ollama pull codellama:7b` — Meta's code model

### 3. Install Python dependencies

```bash
pip install -r requirements.txt
```

### 4. Run

```bash
python run.py
```

Open **http://localhost:8888** in your browser.

## Features

- **Chat UI** — clean dark interface with streaming responses
- **Conversation history** — saved locally as JSON, pick up where you left off
- **Multiple models** — swap models in Settings, pull new ones from the UI
- **Custom system prompt** — fully editable, controls how the AI behaves
- **Code highlighting** — syntax-highlighted code blocks with copy button
- **WebSocket streaming** — tokens appear as they generate, no waiting
- **No external calls** — everything runs on your machine

## Customization

Click **Settings** in the sidebar to change:
- **Model** — pick from any model you've pulled
- **System prompt** — controls the AI's behavior and personality
- **Temperature** — higher = more creative, lower = more precise
- **Max tokens** — how long responses can be

## Architecture

```
server/
  app.py              — FastAPI routes + WebSocket handler
  ollama_client.py    — Ollama API client with streaming
  config.py           — config load/save
  conversations.py    — conversation CRUD (JSON files)
templates/
  index.html          — main page
static/
  style.css           — dark theme UI
  app.js              — frontend logic
data/                 — conversations + config (auto-created)
run.py                — launcher
```
