const $ = (sel) => document.querySelector(sel);
const state = { convId: null, ws: null, generating: false };

// --- Conversations ---

async function loadConversations() {
    const res = await fetch("/api/conversations");
    const convs = await res.json();
    const list = $("#convList");
    list.innerHTML = "";
    convs.forEach((c) => {
        const el = document.createElement("div");
        el.className = `conv-item${c.id === state.convId ? " active" : ""}`;
        el.innerHTML = `
            <span class="conv-title">${esc(c.title)}</span>
            <button class="delete-btn" data-id="${c.id}">&times;</button>
        `;
        el.addEventListener("click", (e) => {
            if (e.target.classList.contains("delete-btn")) return;
            openConversation(c.id);
        });
        el.querySelector(".delete-btn").addEventListener("click", async () => {
            await fetch(`/api/conversations/${c.id}`, { method: "DELETE" });
            if (state.convId === c.id) {
                state.convId = null;
                showWelcome();
            }
            loadConversations();
        });
        list.appendChild(el);
    });
}

async function openConversation(id) {
    const res = await fetch(`/api/conversations/${id}`);
    if (!res.ok) return;
    const conv = await res.json();
    state.convId = id;

    const msgs = $("#messages");
    msgs.innerHTML = "";
    conv.messages.forEach((m) => appendMessage(m.role, m.content));

    $("#welcome").style.display = "none";
    msgs.style.display = "block";
    scrollBottom();
    connectWS();
    loadConversations();
}

async function newChat() {
    const res = await fetch("/api/conversations", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ title: "New Chat" }),
    });
    const conv = await res.json();
    state.convId = conv.id;

    $("#messages").innerHTML = "";
    $("#messages").style.display = "block";
    $("#welcome").style.display = "none";
    connectWS();
    loadConversations();
}

function showWelcome() {
    $("#messages").style.display = "none";
    $("#welcome").style.display = "flex";
}

// --- WebSocket ---

function connectWS() {
    if (state.ws) {
        state.ws.close();
    }
    if (!state.convId) return;

    const proto = location.protocol === "https:" ? "wss:" : "ws:";
    state.ws = new WebSocket(`${proto}//${location.host}/ws/chat/${state.convId}`);

    state.ws.onmessage = (event) => {
        const data = JSON.parse(event.data);

        if (data.type === "conv_id") {
            state.convId = data.conv_id;
            loadConversations();
        } else if (data.type === "start") {
            appendMessage("assistant", "");
            state.generating = true;
            updateStatus("Generating...");
        } else if (data.type === "token") {
            appendToken(data.content);
        } else if (data.type === "done") {
            finishGeneration();
        } else if (data.type === "error") {
            appendToken(`\n\n[Error: ${data.content}]`);
            finishGeneration();
        }
    };

    state.ws.onerror = () => updateStatus("Connection error");
    state.ws.onclose = () => updateStatus("Disconnected");
}

// --- Messages ---

function appendMessage(role, content) {
    const msgs = $("#messages");
    const div = document.createElement("div");
    div.className = `message ${role}`;

    const avatar = role === "user" ? "Y" : "AI";
    const sender = role === "user" ? "You" : "MyAI";

    div.innerHTML = `
        <div class="message-header">
            <div class="avatar ${role === "user" ? "user" : "ai"}">${avatar}</div>
            <span class="message-sender">${sender}</span>
        </div>
        <div class="message-content">${formatContent(content)}</div>
    `;
    msgs.appendChild(div);
    scrollBottom();
}

function appendToken(token) {
    const msgs = document.querySelectorAll(".message.assistant");
    if (msgs.length === 0) return;
    const last = msgs[msgs.length - 1];
    const content = last.querySelector(".message-content");

    if (!content.dataset.raw) content.dataset.raw = "";
    content.dataset.raw += token;
    content.innerHTML = formatContent(content.dataset.raw);
    scrollBottom();
}

function finishGeneration() {
    state.generating = false;
    updateStatus("Ready");
    $("#sendBtn").disabled = false;

    // add copy buttons to code blocks
    document.querySelectorAll(".message-content pre").forEach((pre) => {
        if (pre.querySelector(".copy-btn")) return;
        const btn = document.createElement("button");
        btn.className = "copy-btn";
        btn.textContent = "Copy";
        btn.addEventListener("click", () => {
            const code = pre.querySelector("code");
            navigator.clipboard.writeText(code ? code.textContent : pre.textContent);
            btn.textContent = "Copied!";
            setTimeout(() => (btn.textContent = "Copy"), 1500);
        });
        pre.style.position = "relative";
        pre.appendChild(btn);
    });

    loadConversations();
}

function formatContent(text) {
    if (!text) return "";
    let html = esc(text);

    // code blocks: ```lang\ncode\n```
    html = html.replace(/```(\w*)\n([\s\S]*?)```/g, (_, lang, code) => {
        return `<pre><code class="language-${lang || "text"}">${code.trim()}</code></pre>`;
    });

    // inline code
    html = html.replace(/`([^`]+)`/g, "<code>$1</code>");

    // bold
    html = html.replace(/\*\*(.+?)\*\*/g, "<strong>$1</strong>");

    // italic
    html = html.replace(/\*(.+?)\*/g, "<em>$1</em>");

    // line breaks (outside pre)
    html = html.replace(/\n/g, "<br>");

    // fix line breaks inside pre
    html = html.replace(/<pre>([\s\S]*?)<\/pre>/g, (match) => {
        return match.replace(/<br>/g, "\n");
    });

    return html;
}

function esc(s) {
    const d = document.createElement("div");
    d.textContent = s;
    return d.innerHTML;
}

function scrollBottom() {
    const container = $("#chatContainer");
    container.scrollTop = container.scrollHeight;
}

function updateStatus(text) {
    $("#status").textContent = text;
}

// --- Send ---

async function send() {
    const input = $("#userInput");
    const text = input.value.trim();
    if (!text || state.generating) return;

    if (!state.convId) {
        await newChat();
    }

    appendMessage("user", text);
    input.value = "";
    input.style.height = "auto";
    $("#sendBtn").disabled = true;

    if (state.ws && state.ws.readyState === WebSocket.OPEN) {
        state.ws.send(JSON.stringify({ content: text }));
    } else {
        connectWS();
        setTimeout(() => {
            if (state.ws && state.ws.readyState === WebSocket.OPEN) {
                state.ws.send(JSON.stringify({ content: text }));
            }
        }, 500);
    }
}

// --- Settings ---

async function loadSettings() {
    const res = await fetch("/api/config");
    const config = await res.json();

    $("#cfgOllamaUrl").value = config.ollama_url || "";
    $("#cfgSystemPrompt").value = config.system_prompt || "";
    $("#cfgTemp").value = config.temperature || 0.7;
    $("#tempVal").textContent = config.temperature || 0.7;
    $("#cfgMaxTokens").value = config.max_tokens || 4096;
    $("#modelBadge").textContent = config.model || "qwen2.5-coder:7b";

    await refreshModels(config.model);
}

async function refreshModels(selectedModel) {
    const res = await fetch("/api/models");
    const data = await res.json();
    const select = $("#cfgModel");
    select.innerHTML = "";
    data.models.forEach((m) => {
        const opt = document.createElement("option");
        opt.value = m;
        opt.textContent = m;
        if (m === selectedModel) opt.selected = true;
        select.appendChild(opt);
    });
    if (data.models.length === 0) {
        const opt = document.createElement("option");
        opt.textContent = "No models found — pull one first";
        select.appendChild(opt);
    }
}

async function saveSettings() {
    const config = {
        ollama_url: $("#cfgOllamaUrl").value,
        model: $("#cfgModel").value,
        system_prompt: $("#cfgSystemPrompt").value,
        temperature: parseFloat($("#cfgTemp").value),
        max_tokens: parseInt($("#cfgMaxTokens").value),
    };
    await fetch("/api/config", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify(config),
    });
    $("#modelBadge").textContent = config.model;
    $("#settingsModal").classList.remove("open");
}

// --- Init ---

document.addEventListener("DOMContentLoaded", () => {
    loadConversations();
    loadSettings();

    // New chat
    $("#newChatBtn").addEventListener("click", newChat);

    // Send
    $("#sendBtn").addEventListener("click", send);
    $("#userInput").addEventListener("keydown", (e) => {
        if (e.key === "Enter" && !e.shiftKey) {
            e.preventDefault();
            send();
        }
    });

    // Auto-resize textarea
    $("#userInput").addEventListener("input", function () {
        this.style.height = "auto";
        this.style.height = Math.min(this.scrollHeight, 200) + "px";
    });

    // Quick action buttons
    document.querySelectorAll(".quick-btn").forEach((btn) => {
        btn.addEventListener("click", () => {
            $("#userInput").value = btn.dataset.prompt + " ";
            $("#userInput").focus();
        });
    });

    // Settings
    $("#settingsBtn").addEventListener("click", () => {
        loadSettings();
        $("#settingsModal").classList.add("open");
    });
    $("#closeSettings").addEventListener("click", () => {
        $("#settingsModal").classList.remove("open");
    });
    $("#settingsModal").addEventListener("click", (e) => {
        if (e.target === $("#settingsModal")) {
            $("#settingsModal").classList.remove("open");
        }
    });
    $("#saveSettings").addEventListener("click", saveSettings);
    $("#refreshModels").addEventListener("click", () => refreshModels());
    $("#cfgTemp").addEventListener("input", function () {
        $("#tempVal").textContent = this.value;
    });

    // Pull model
    $("#pullModelBtn").addEventListener("click", async () => {
        const name = $("#pullModelName").value.trim();
        if (!name) return;
        $("#pullStatus").textContent = "Pulling... this may take a while";
        const res = await fetch("/api/models/pull", {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify({ model: name }),
        });
        const data = await res.json();
        if (data.ok) {
            $("#pullStatus").textContent = "Done!";
            refreshModels(name);
        } else {
            $("#pullStatus").textContent = "Failed to pull model";
        }
    });
});
