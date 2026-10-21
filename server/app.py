"""
Trident Auth API — runs on the VPS, validates loader keys.
"""

from flask import Flask, request, jsonify
import db
import os

app = Flask(__name__)

API_SECRET = os.environ.get("TRIDENT_API_SECRET", "change-me-in-production")

@app.route("/api/auth", methods=["POST"])
def auth():
    data = request.get_json(silent=True)
    if not data or "key" not in data or "hwid" not in data:
        return jsonify({"status": "error", "message": "Missing key or hwid"}), 400

    ok, result = db.validate_key(data["key"], data["hwid"])
    if ok:
        return jsonify({
            "status": "ok",
            "expires": result
        })
    else:
        return jsonify({"status": "error", "message": result}), 403

@app.route("/api/status", methods=["GET"])
def status():
    return jsonify({"status": "ok", "service": "trident-auth"})

if __name__ == "__main__":
    from waitress import serve
    port = int(os.environ.get("TRIDENT_PORT", "8888"))
    print(f"[Trident Auth] Listening on 0.0.0.0:{port}")
    serve(app, host="0.0.0.0", port=port)
