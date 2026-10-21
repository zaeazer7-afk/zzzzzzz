"""
Trident Discord Bot — manages license keys.

Environment variables:
    DISCORD_TOKEN       — bot token
    ADMIN_ROLE_ID       — Discord role ID that can manage keys (optional, defaults to server owner)
    TRIDENT_PORT        — API port (default 8888)
"""

import discord
from discord.ext import commands
import db
import os
import time
import threading

DISCORD_TOKEN = os.environ.get("DISCORD_TOKEN", "")
ADMIN_ROLE_ID = int(os.environ.get("ADMIN_ROLE_ID", "0"))
CUSTOMER_ROLE_ID = int(os.environ.get("CUSTOMER_ROLE_ID", "0"))

DURATIONS = {
    "1h":       3600,
    "1d":       86400,
    "3d":       259200,
    "7d":       604800,
    "14d":      1209600,
    "30d":      2592000,
    "90d":      7776000,
    "lifetime": 0,
}

intents = discord.Intents.default()
intents.message_content = True
intents.members = True
bot = commands.Bot(command_prefix="!", intents=intents)

def is_admin(ctx):
    if ctx.author.id == ctx.guild.owner_id:
        return True
    if ADMIN_ROLE_ID:
        return any(r.id == ADMIN_ROLE_ID for r in ctx.author.roles)
    return False

def fmt_time(ts):
    if ts == 0:
        return "Lifetime"
    return time.strftime("%Y-%m-%d %H:%M UTC", time.gmtime(ts))

def fmt_remaining(ts):
    if ts == 0:
        return "Lifetime"
    rem = ts - int(time.time())
    if rem <= 0:
        return "Expired"
    d = rem // 86400
    h = (rem % 86400) // 3600
    if d > 0:
        return f"{d}d {h}h"
    m = (rem % 3600) // 60
    return f"{h}h {m}m"

@bot.event
async def on_ready():
    print(f"[Bot] Logged in as {bot.user}")

@bot.command(name="genkey")
async def genkey(ctx, duration: str = "30d", *, note: str = ""):
    if not is_admin(ctx):
        await ctx.reply("No permission.")
        return

    dur = duration.lower()
    if dur not in DURATIONS:
        await ctx.reply(f"Invalid duration. Options: {', '.join(DURATIONS.keys())}")
        return

    key, expires = db.generate_key(DURATIONS[dur], str(ctx.author), note)

    embed = discord.Embed(title="Key Generated", color=0x7B2FBE)
    embed.add_field(name="Key", value=f"```{key}```", inline=False)
    embed.add_field(name="Duration", value=dur, inline=True)
    embed.add_field(name="Expires", value=fmt_time(expires), inline=True)
    if note:
        embed.add_field(name="Note", value=note, inline=False)

    try:
        await ctx.author.send(embed=embed)
        await ctx.reply("Key sent to your DMs.")
    except discord.Forbidden:
        await ctx.reply(embed=embed)

@bot.command(name="genkeys")
async def genkeys(ctx, count: int = 1, duration: str = "30d"):
    if not is_admin(ctx):
        await ctx.reply("No permission.")
        return

    if count < 1 or count > 25:
        await ctx.reply("Count must be 1-25.")
        return

    dur = duration.lower()
    if dur not in DURATIONS:
        await ctx.reply(f"Invalid duration. Options: {', '.join(DURATIONS.keys())}")
        return

    keys = []
    for _ in range(count):
        key, _ = db.generate_key(DURATIONS[dur], str(ctx.author))
        keys.append(key)

    text = "\n".join(keys)
    embed = discord.Embed(title=f"{count} Keys Generated ({dur})", color=0x7B2FBE)
    embed.description = f"```\n{text}\n```"

    try:
        await ctx.author.send(embed=embed)
        await ctx.reply(f"{count} keys sent to your DMs.")
    except discord.Forbidden:
        await ctx.reply(embed=embed)

@bot.command(name="delkey")
async def delkey(ctx, key: str):
    if not is_admin(ctx):
        await ctx.reply("No permission.")
        return

    info = db.get_key_info(key)
    if not info:
        await ctx.reply("Key not found.")
        return

    discord_id = info.get("note", "")
    db.delete_key(key)

    if discord_id and CUSTOMER_ROLE_ID and ctx.guild:
        try:
            member = await ctx.guild.fetch_member(int(discord_id))
            role = ctx.guild.get_role(CUSTOMER_ROLE_ID)
            if member and role:
                await member.remove_roles(role)
        except (discord.Forbidden, discord.NotFound, ValueError):
            pass

    await ctx.reply(f"Key `{key}` deleted.")

@bot.command(name="resetkey")
async def resetkey(ctx, key: str):
    if not is_admin(ctx):
        await ctx.reply("No permission.")
        return

    if db.reset_hwid(key):
        await ctx.reply(f"HWID reset for `{key}`. Key can bind to a new machine.")
    else:
        await ctx.reply("Key not found.")

@bot.command(name="keyinfo")
async def keyinfo(ctx, key: str):
    if not is_admin(ctx):
        await ctx.reply("No permission.")
        return

    info = db.get_key_info(key)
    if not info:
        await ctx.reply("Key not found.")
        return

    embed = discord.Embed(title="Key Info", color=0x7B2FBE)
    embed.add_field(name="Key", value=f"```{info['key']}```", inline=False)
    embed.add_field(name="Status", value="Active" if (info["expires_at"] == 0 or info["expires_at"] > time.time()) else "Expired", inline=True)
    embed.add_field(name="Remaining", value=fmt_remaining(info["expires_at"]), inline=True)
    embed.add_field(name="HWID", value=info["hwid"] or "Not bound", inline=True)
    embed.add_field(name="Created", value=fmt_time(info["created_at"]), inline=True)
    embed.add_field(name="Expires", value=fmt_time(info["expires_at"]), inline=True)
    embed.add_field(name="Created By", value=info["created_by"] or "—", inline=True)
    await ctx.reply(embed=embed)

@bot.command(name="keys")
async def keys_list(ctx):
    if not is_admin(ctx):
        await ctx.reply("No permission.")
        return

    total, active = db.count_keys()
    all_keys = db.list_keys(25)

    embed = discord.Embed(title="License Keys", color=0x7B2FBE)
    embed.description = f"**{active}** active / **{total}** total"

    for k in all_keys:
        status = "Active" if (k["expires_at"] == 0 or k["expires_at"] > time.time()) else "Expired"
        hwid = "Bound" if k["hwid"] else "Unbound"
        embed.add_field(
            name=k["key"],
            value=f"{status} | {fmt_remaining(k['expires_at'])} | {hwid}",
            inline=False
        )

    await ctx.reply(embed=embed)

@bot.command(name="redeem")
async def redeem(ctx, key: str):
    info = db.get_key_info(key)
    if not info:
        await ctx.reply("Invalid key.")
        return

    now = time.time()
    if info["expires_at"] != 0 and now > info["expires_at"]:
        await ctx.reply("This key has expired.")
        return

    if info["hwid"] and info["used"]:
        discord_id = info.get("note", "")
        if discord_id and discord_id != str(ctx.author.id):
            await ctx.reply("This key is already bound to another user.")
            return

    db.bind_discord(key, str(ctx.author.id))

    if CUSTOMER_ROLE_ID and ctx.guild:
        role = ctx.guild.get_role(CUSTOMER_ROLE_ID)
        if role:
            try:
                await ctx.author.add_roles(role)
            except discord.Forbidden:
                await ctx.reply("Key redeemed but I couldn't assign the role — check my permissions.")
                return

    remaining = fmt_remaining(info["expires_at"])
    embed = discord.Embed(title="Key Redeemed", color=0x00FF00)
    embed.add_field(name="Remaining", value=remaining, inline=True)
    embed.description = "You now have access. Use your key in the loader."
    await ctx.reply(embed=embed)

    try:
        await ctx.message.delete()
    except discord.Forbidden:
        pass

@bot.command(name="help_keys")
async def help_keys(ctx):
    embed = discord.Embed(title="Trident Key Commands", color=0x7B2FBE)
    embed.add_field(name="!redeem <key>", value="Redeem a key and get the Trident Internal role", inline=False)
    embed.add_field(name="!genkey [duration] [note]", value="Generate a key (1h/1d/3d/7d/14d/30d/90d/lifetime) [Admin]", inline=False)
    embed.add_field(name="!genkeys [count] [duration]", value="Generate multiple keys (max 25) [Admin]", inline=False)
    embed.add_field(name="!delkey <key>", value="Delete a key and remove role [Admin]", inline=False)
    embed.add_field(name="!resetkey <key>", value="Reset HWID binding [Admin]", inline=False)
    embed.add_field(name="!keyinfo <key>", value="Show key details [Admin]", inline=False)
    embed.add_field(name="!keys", value="List all keys [Admin]", inline=False)
    await ctx.reply(embed=embed)


def start_api():
    from app import app
    from waitress import serve
    port = int(os.environ.get("TRIDENT_PORT", "8888"))
    print(f"[Auth API] Listening on 0.0.0.0:{port}")
    serve(app, host="0.0.0.0", port=port)

if __name__ == "__main__":
    if not DISCORD_TOKEN:
        print("[!] Set DISCORD_TOKEN environment variable")
        exit(1)

    api_thread = threading.Thread(target=start_api, daemon=True)
    api_thread.start()

    bot.run(DISCORD_TOKEN)
