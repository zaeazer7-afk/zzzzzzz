/* ═══════════════════════════════════════════
   API BRIDGE — real pywebview bridge, or a mock
   backend when the page is opened in a browser.
═══════════════════════════════════════════ */

const sleep = (ms) => new Promise((r) => setTimeout(r, ms));

// mirror of generators.FORMATS for the browser preview only
const MOCK_SWEEPS = {
  semi3l: [365248, 300000], semi3n: [8000, 7600], word4: [4068, 3967], semi3m: [1040, 1000], semi3c: [56160, 50000],
  all_dot: [2340, 1300], all_4n: [10000, 581], all_4l: [456976, 448522], all_4c: [1212640, 1197085], rsemi3: [93312, 93312],
};
// Roblox: which formats exist there, with their measured Roblox odds
const MOCK_ROBLOX = {
  rsemi3: 0, word4: 0, all_4n: 0, all_4l: 0, all_4c: 0, rsemi4: 32, "5c": 26, "5l": 0, "5p": 0, "5n": 0, "3m": 0, "3s": 0,
  xny: 97, compound: 89, stretched: 8, hashtag: 40, misspell: 25, split: 0, mood: 32, twisted: 30, alphanum: 54, stylized: 5,
  phrase: null, duo: null, wordnum: null, brand: null, mash: null, laugh: null, l4n: null, longnum: null,
};
const MOCK_ROBLOX_ONLY = new Set(["rsemi3", "rsemi4"]);
const MOCK_FORMATS = [
  ["rsemi3", "Every 4-character name", "Semi 3", "ab_c", "3 letters/digits + one _ in the middle (ab_c, 9h_4, m_8m)", 0, ["every4"]],
  ["rsemi4", "Semi 4L / 4C", "Semi 4", "ab_cd", "4 letters/digits + one _ in the middle (ab_cd, a_bcd, 4k_9x)", null, ["semi4"]],
  ["semi3l", "Every 4-character name", "Semi 3L", "mfm_", "3 letters/digits + one _ or . anywhere (mfm_, m8m., o8o., 9h_4)", 0, ["every4"]],
  ["semi3n", "Every 4-character name", "Semi 3N", "093_", "3 digits + one _ or . anywhere (093_, 09.3, _093)", 0, ["every4"]],
  ["word4", "Every 4-character name", "4L word", "wbot", "a letter or digit + short word (wbot, xgod, 1pro)", 0, ["every4"]],
  ["all_dot", "Every 4-character name", ".10a", ".10a", "every . + 10–99 + letter (.10a … .99z)", 0, ["every4"]],
  ["all_4n", "Every 4-character name", "4N", "2190", "every 4-digit number (0000 … 9999)", 0, ["every4"]],
  ["all_4l", "Every 4-character name", "4L", "zuxk", "every 4-letter name (aaaa … zzzz)", 0, ["every4"]],
  ["all_4c", "Every 4-character name", "4C", "cm3a", "every 4-character mix of letters and digits", 0, ["every4"]],
  ["4c_sym", "Semi 4L / 4C", "Semi 4C", "t_n4y", "4 letters or digits + one _ or .", 84, ["best", "semi4"]],
  ["4l_sym", "Semi 4L / 4C", "Semi 4L", "kt_io", "4 letters + one _ or .", 56, ["best", "semi4"]],
  ["4n_mid", "Semi 4N", "Semi 4N", "21.90", "4 digits with a _ or . in the middle", 55, ["best", "semi4", "numbers"]],
  ["4n_start", "Semi 4N", "Semi 4N start", "_2190", "4 digits that start with _ or .", 4, ["semi4", "numbers"]],
  ["4n_end", "Semi 4N", "Semi 4N end", "4316_", "4 digits that end with _ or .", 1, ["semi4", "numbers"]],
  ["5c", "5 characters", "5C", "guj31", "5 letters and digits", 82, ["best"]],
  ["5l", "5 characters", "5L", "hnupo", "5 letters", 68, ["best"]],
  ["5p", "5 characters", "5P", "lajub", "5 letters you can say out loud", 12, []],
  ["5n", "5 characters", "5N", "21904", "5 digits", 0, []],
  ["3m", "3 characters", "3M", "m8m", "mirror, no symbol (6w6, k3k)", 0, []],
  ["3s", "3 characters", "3C", "ka1", "3 letters and digits mixed, no symbol", 0, []],
  ["semi3m", "3 characters", "Semi 3M", "m8m.", "mirror + . or _ at the end (m8m., 6w6_)", 0, ["every4"]],
  ["semi3c", "3 characters", "Semi 3C", "ka1.", "3 letters/digits mixed + . or _ at the end (ka1., 9xx_)", 0, ["every4"]],
  ["xny", "Words", "XnY", "paidnbroken", "word + n + word", null, ["words"]],
  ["compound", "Words", "Compound", "stuckinloops", "two-part phrase", null, ["words"]],
  ["stretched", "Words", "Stretched", "numbbbbbb", "word with a stretched letter", null, ["words"]],
  ["hashtag", "Words", "Hashtag", "hashtagempty", "tag / no / im + phrase", null, ["words"]],
  ["misspell", "Words", "Misspell", "beautifyul", "misspelled word", null, ["words"]],
  ["split", "Words", "Split", "g_uap", "word split by _ . or -", null, ["words"]],
  ["mood", "Words", "Mood", "wockmode", "word + mode / way / set…", null, ["words"]],
  ["twisted", "Words", "Twisted", "fragmenteds", "word with a twisted -ing / -ed ending", null, ["words"]],
  ["alphanum", "Words", "AlphaNum", "1x1y1", "patterns like 1x1y1 or 0x hex", null, ["words"]],
  ["stylized", "Words", "Stylized", "cigareto", "a word with a stylized ending (cigareto, chocolato, velveto)", null, ["words"]],
  ["phrase", "Phrases & mashes", "Phrase", "aimingtoohardsorry", "a short sentence (ifeelsorryforyouxx, nevergondisrespectme, stopyoufreak81)", null, ["mix"]],
  ["duo", "Phrases & mashes", "Duo", "loudrapmusic", "two words (loudrapmusic, forcepassive, seasonmvp)", null, ["mix"]],
  ["wordnum", "Phrases & mashes", "Word + num", "assault5", "a word + 1-4 digits (assault5, static915, loser872)", null, ["mix"]],
  ["brand", "Phrases & mashes", "Brand", "balenciaga8917", "a fashion brand + digits, or a phrase around one (maisonmargiela82, devillarpsbalenciaga)", null, ["mix"]],
  ["mash", "Phrases & mashes", "Key mash", "xcvbjxxzcd", "keyboard mash (xcvbjxxzcd, asdjrt, xzxzxzxzxcx, zzzxxxlol8)", null, ["mix"]],
  ["laugh", "Phrases & mashes", "Laugh mash", "hahaxdlol89", "laughs stuck together (hahaxdlol89, xoxolol19)", null, ["mix"]],
  ["l4n", "Phrases & mashes", "1L + 4N", "s8274", "a letter + 4 digits, either way round (s8274, 1829s)", null, ["mix"]],
  ["longnum", "Phrases & mashes", "Long number", "74627521", "6 to 9 digits (74627521)", null, ["mix"]],
  ["dotcom", "Phrases & mashes", ".com", "famousguys.com", "a word or brand + .com / .gg / .net (famousguys.com)", null, ["mix"]],
].map(([id, group, code, example, desc, odds, picks]) => {
  const f = { id, group, code, example, desc, odds, picks };
  if (MOCK_SWEEPS[id]) Object.assign(f, { sweep: true, size: MOCK_SWEEPS[id][0], left: MOCK_SWEEPS[id][1] });
  return f;
});

function makeMockApi() {
  let vault = [];
  let running = false;
  let queue = [];
  let retries = {};
  const push = (type, data) => queue.push({ type, data });
  const randName = () => {
    const c = "abcdefghijklmnopqrstuvwxyz0123456789";
    let n = "";
    for (let i = 0; i < 4 + Math.floor(Math.random() * 2); i++) n += c[Math.floor(Math.random() * c.length)];
    return n;
  };
  const fakeFile = (label) => ({ ok: true, path: `C:\\demo\\exports\\${label}.txt`, file: `exports\\${label}_demo.txt` });

  async function fakeRun(list, count) {
    running = true;
    push("status", `rotating gateway | ${count || "endless"} usernames | 50 workers`);
    for (let i = 1; (!count || i <= count) && running; i++) {
      const name = list ? list[i - 1] : randName();
      push("checking", { username: name, total: count });
      await sleep(160);
      if (!running) break;
      if (Math.random() < 0.2) {
        const why = Math.random() < 0.6 ? "no answer" : "rate limited";
        retries[why] = (retries[why] || 0) + 1;
      }
      const roll = Math.random();
      const d = { username: name, n: i, total: count, fmt: list ? undefined : "4C+" };
      if (roll < 0.3) push("valid", d);
      else if (roll < 0.93) push("taken", d);
      else push("retry", { ...d, reason: "no answer" });
    }
    push(running ? "done" : "stopped", running ? "Done! mock run finished" : "Stopped");
    running = false;
  }

  return {
    get_formats: async (platform) => (platform === "roblox"
      ? MOCK_FORMATS.filter((f) => f.id in MOCK_ROBLOX).map((f) => ({
        ...f, odds: MOCK_ROBLOX[f.id],
        picks: f.picks.filter((p) => p !== "best").concat(MOCK_ROBLOX[f.id] >= 50 ? ["best"] : []),
      }))
      : MOCK_FORMATS.filter((f) => !MOCK_ROBLOX_ONLY.has(f.id))),
    get_state: async () => ({ proxies_text: "http://user:pass@dy01.example.com:30000", vault, running: false }),
    save_proxies: async (text) => ({ count: text.split("\n").filter((x) => x.trim()).length }),
    load_proxies: async () => "http://user:pass@dy01.example.com:30000",
    test_proxies: async (text) => {
      await sleep(900);
      return text.split("\n").filter((x) => x.trim()).map((p) => ({
        proxy: p.replace(/:[^:@/]+@/, ":•••@"), ok: Math.random() > 0.2, status: 200,
        ms: 400 + Math.floor(Math.random() * 3000), rotating: true,
      }));
    },
    start_checking: async (mode, formats, count, proxiesText, workers, skip, listText) => {
      queue = [];
      retries = {};
      const list = mode === "list" ? listText.split("\n").filter(Boolean) : null;
      const total = list ? list.length : count;
      fakeRun(list, total);
      return { ok: true, total };
    },
    stop_checking: async () => { running = false; return { ok: true }; },
    poll_events: async () => { const e = queue; queue = []; return { events: e, running, retries: { ...retries } }; },
    copy_text: async (t) => { try { await navigator.clipboard.writeText(t); } catch (_) {} return { ok: true }; },
    generate_to_file: async (formats, count) => ({ ...fakeFile("names"), count }),
    save_names: async (names, label) => ({ ...fakeFile(label), count: names.length }),
    reveal: async () => ({ ok: true }),
    get_all_hits: async () => ["xkdt", "g4110", "21.90", "kt_io", "5947_", "hnupo", "guj31", "t_n4y"],
    vault_list: async () => vault,
    vault_add: async (username, price, notes) => {
      if (vault.find((v) => v.username.toLowerCase() === username.toLowerCase())) return { added: false };
      vault.unshift({ username, price, notes, added: new Date().toISOString().slice(0, 10) });
      return { added: true };
    },
    vault_add_many: async (names) => {
      let added = 0;
      names.forEach((n) => {
        if (!vault.find((v) => v.username === n)) { vault.unshift({ username: n, price: "", notes: "", added: "" }); added++; }
      });
      return { added };
    },
    vault_remove: async (username) => { vault = vault.filter((v) => v.username !== username); return { ok: true }; },
    vault_import_valid: async () => ({ added: 0 }),
    vault_copy_all: async () => ({ count: vault.length }),
  };
}

let API = null;
function api() { return API; }

// fake data is only for previewing the page in a browser (index.html?preview);
// the app itself must never fall back to it, however slow the engine starts
const PREVIEW = new URLSearchParams(location.search).has("preview");

function waitForBridge() {
  const ready = () => window.pywebview && window.pywebview.api && typeof window.pywebview.api.get_state === "function";
  return new Promise((resolve) => {
    if (ready()) return resolve(window.pywebview.api);
    if (PREVIEW) {
      document.body.appendChild(el("div", "preview-banner", "PREVIEW — fake data, not real results"));
      return resolve(makeMockApi());
    }
    const timer = setInterval(() => {
      if (ready()) { clearInterval(timer); resolve(window.pywebview.api); }
    }, 200);
    window.addEventListener("pywebviewready", () => { clearInterval(timer); resolve(window.pywebview.api); }, { once: true });
    setTimeout(() => { if (!API) setStatus("connecting to the checker engine…", "var(--yellow)"); }, 3000);
  });
}

/* ═══════════════════════════════════════════
   HELPERS
═══════════════════════════════════════════ */

const $ = (id) => document.getElementById(id);
const fmtNum = (n) => Number(n).toLocaleString();
const plural = (n, word) => `${fmtNum(n)} ${word}${n === 1 ? "" : "s"}`;

function el(tag, cls, text) {
  const e = document.createElement(tag);
  if (cls) e.className = cls;
  if (text !== undefined) e.textContent = text;
  return e;
}

const ICON_COPY = '<svg viewBox="0 0 24 24"><rect x="9" y="9" width="11" height="11" rx="2"/><path d="M5 15V5a2 2 0 0 1 2-2h8"/></svg>';
const ICON_PLUS = '<svg viewBox="0 0 24 24"><path d="M12 5v14M5 12h14"/></svg>';

function iconBtn(svg, title, fn) {
  const b = el("button", "icon-btn");
  b.title = title;
  b.innerHTML = svg;
  b.addEventListener("click", fn);
  return b;
}

function toast(text, kind, action) {
  const t = el("div", "toast " + (kind || ""));
  t.appendChild(el("span", null, text));
  const dismiss = () => {
    if (!t.isConnected || t.classList.contains("out")) return;
    t.classList.add("out");
    setTimeout(() => t.remove(), 260);
  };
  if (action) {
    const b = el("button", null, action.label);
    b.addEventListener("click", () => { action.fn(); dismiss(); });
    t.appendChild(b);
  }
  $("toasts").appendChild(t);
  setTimeout(dismiss, action ? 6000 : 2400);
}

function fmtTime(s) {
  s = Math.max(0, Math.floor(s));
  const h = Math.floor(s / 3600), m = Math.floor((s % 3600) / 60), sec = s % 60;
  const mm = String(m).padStart(2, "0"), ss = String(sec).padStart(2, "0");
  return h ? `${h}:${mm}:${ss}` : `${mm}:${ss}`;
}

function nowStamp() {
  const d = new Date();
  return [d.getHours(), d.getMinutes(), d.getSeconds()].map((n) => String(n).padStart(2, "0")).join(":");
}

function prefGet(k) { try { return localStorage.getItem(k); } catch (_) { return null; } }
function prefSet(k, v) { try { localStorage.setItem(k, v); } catch (_) {} }

function segSelect(seg, attr, value) {
  seg.querySelectorAll("button").forEach((b, i) => {
    const on = b.dataset[attr] === value;
    b.classList.toggle("active", on);
    if (on) seg.style.setProperty("--i", i);
  });
}

/* ═══════════════════════════════════════════
   STATE
═══════════════════════════════════════════ */

const WORDS_ODDS = 55;   // word formats ran ~55% free as a group
const HIT_CARDS = 500;   // cards drawn on the Hits page; copy/save still use every name

const state = {
  page: "scan",
  platform: "discord",    // discord | roblox — each keeps its own formats, files and hits
  kind: "gen",            // gen = find new names, list = check my list
  formats: [],
  selected: new Set(),
  running: false,
  stopping: false,
  workers: 50,
  total: 0,
  stats: { valid: 0, taken: 0, retry: 0 },
  doneTimes: [],
  startedAt: 0,
  retries: {},
  hits: [],               // last run: { name, fmt, at }
  runPlatform: "discord", // which site the last run checked
  hitsPlatform: "discord",
  hitsView: "session",
  allHits: null,
  hitsShown: [],
  hitsSort: "new",
  hitsQuery: "",
};

const PLATFORM_NAME = { discord: "Discord", roblox: "Roblox" };
const VALID_FILE = { discord: "valid.txt", roblox: "roblox_valid.txt" };

const SCAN_PAGE = {
  discord: ["Discord", "Find free Discord usernames — pick what to look for, choose how many, press Start"],
  roblox: ["Roblox", "Find free Roblox usernames — 3–20 letters or digits, at most one _ in the middle"],
};
const PAGES = {
  hits: ["Hits", "Free {platform} names — save them to a .txt, copy them or re-check them"],
  proxies: ["Proxies", "Rotating gateways get a fresh IP per check — no cooldowns"],
  vault: ["Vault", "Names you're holding or selling"],
};

/* ═══════════════════════════════════════════
   NAV — Discord and Roblox are their own tabs
═══════════════════════════════════════════ */

function showPage(key) {
  state.page = key;
  document.querySelectorAll(".nav-item").forEach((n) => n.classList.toggle("active",
    n.dataset.page === key && (!n.dataset.platform || n.dataset.platform === state.platform)));
  document.querySelectorAll(".page").forEach((p) => p.classList.toggle("active", p.id === "page-" + key));
  const [title, sub] = key === "scan" ? SCAN_PAGE[state.platform] : PAGES[key];
  $("page-title").textContent = title;
  $("page-sub").textContent = sub.replace("{platform}", PLATFORM_NAME[state.hitsPlatform]);
  const viewing = key === "scan" ? state.platform : key === "hits" ? state.hitsPlatform : "discord";
  document.body.classList.toggle("theme-roblox", viewing === "roblox");
  if (key === "vault") refreshVault();
  if (key === "hits") {
    if (state.hitsView === "all") state.allHits = null;
    renderHitsPage();
  }
}

async function openPlatform(platform) {
  if (platform !== state.platform) {
    if (state.running) {
      toast(`Stop the ${PLATFORM_NAME[state.platform]} scan first`, "bad");
      return false;
    }
    await setPlatform(platform);
  }
  showPage("scan");
  return true;
}

document.querySelectorAll(".nav-item").forEach((n) => n.addEventListener("click", () => {
  if (n.dataset.platform) return openPlatform(n.dataset.platform);
  if (n.dataset.page === "hits") setHitsPlatform(state.platform, false);
  showPage(n.dataset.page);
}));
$("conn-card").addEventListener("click", () => showPage("proxies"));

/* ═══════════════════════════════════════════
   PLATFORM — Discord / Roblox
═══════════════════════════════════════════ */

const formatsKey = () => "nv-formats-" + state.platform;
const vaultNote = (platform) => (platform === "roblox" ? "Roblox" : "");

async function setPlatform(platform) {
  state.platform = platform;
  prefSet("nv-platform", platform);
  state.allHits = null;
  await loadFormats();
}

// formats, saved picks and "N left" counts all depend on the platform
async function loadFormats() {
  state.formats = await api().get_formats(state.platform);
  renderFormats();
  let saved = null;
  try { saved = JSON.parse(prefGet(formatsKey()) || "null"); } catch (_) {}
  const known = new Set(state.formats.map((f) => f.id));
  const restored = Array.isArray(saved) ? saved.filter((id) => known.has(id)) : [];
  document.querySelectorAll("#quick-picks button").forEach((b) => (b.hidden = !pickIds(b.dataset.pick).length));
  if (restored.length) {
    state.selected = new Set(restored);
    syncFormatUI();
  } else {
    applyPick("best");
  }
}

/* ═══════════════════════════════════════════
   SETUP — find vs. list
═══════════════════════════════════════════ */

function setKind(kind) {
  state.kind = kind;
  segSelect($("kind-seg"), "kind", kind);
  $("controls").classList.toggle("mode-list", kind === "list");
  prefSet("nv-kind", kind);
  syncGoButton();
  updateEstimate();
}
document.querySelectorAll("#kind-seg button").forEach((b) => b.addEventListener("click", () => setKind(b.dataset.kind)));

/* ═══════════════════════════════════════════
   SETUP — formats
═══════════════════════════════════════════ */

const oddsOf = (f) => (f.odds === null ? WORDS_ODDS : f.odds);
const oddsClass = (o) => (o >= 40 ? "good" : o >= 5 ? "low" : "dead");

function groupNote(fmts) {
  if (fmts.every((f) => f.sweep)) return "each name checked once";
  if (fmts.every((f) => f.odds === null)) return `≈${WORDS_ODDS}% free as a group`;
  if (fmts.every((f) => f.odds === 0)) return "all taken in tests";
  return "";
}

function compactNum(n) {
  if (n >= 1e6) return (n / 1e6).toFixed(1).replace(/\.0$/, "") + "m";
  if (n >= 1e4) return Math.round(n / 1e3) + "k";
  if (n >= 1e3) return (n / 1e3).toFixed(1).replace(/\.0$/, "") + "k";
  return String(n);
}

function fmtDuration(mins) {
  if (mins < 1) return "under a minute";
  if (mins < 90) return `~${Math.round(mins)} min`;
  return `~${(mins / 60).toFixed(mins < 600 ? 1 : 0)} hours`;
}

// measured: Discord ~1,000/min at 50 parallel; Roblox looks up 100 names per request, ~4,800/min
const checksPerMin = () => state.workers * (state.platform === "roblox" ? 60 : 20);

function renderFormats() {
  const box = $("fmt-groups");
  box.innerHTML = "";
  const groups = [...new Set(state.formats.map((f) => f.group))];
  groups.forEach((g) => {
    const fmts = state.formats.filter((f) => f.group === g);
    const wrap = el("div", "fmt-group");
    const head = el("div", "fmt-group-head");
    const title = el("span", "fmt-group-title", g);
    const note = groupNote(fmts);
    if (note) title.appendChild(el("em", null, note));
    const all = el("button", "link-btn", "all");
    all.title = `Pick every ${g} format`;
    all.addEventListener("click", () => toggleGroup(fmts));
    head.append(title, all);

    const chips = el("div", "fmt-chips");
    fmts.forEach((f) => chips.appendChild(fmtChip(f)));
    wrap.append(head, chips);
    box.appendChild(wrap);
  });
}

function fmtChip(f) {
  const c = el("button", "fmt-chip" + (f.odds === 0 && !f.sweep ? " dead" : ""));
  c.dataset.id = f.id;
  c.appendChild(el("span", "ex", f.example));
  if (f.sweep) c.appendChild(el("span", "odds left", f.left ? `${compactNum(f.left)} left` : "done"));
  else if (f.odds !== null) c.appendChild(el("span", "odds " + oddsClass(f.odds), `${f.odds}%`));
  c.addEventListener("click", () => toggleFormat(f.id));
  c.addEventListener("mouseenter", () => showFmtInfo(f));
  c.addEventListener("mouseleave", () => showFmtInfo(null));
  return c;
}

function renderPicked() {
  const box = $("picked");
  box.innerHTML = "";
  const sel = state.formats.filter((f) => state.selected.has(f.id));
  if (!sel.length) {
    box.appendChild(el("div", "none", "Nothing picked — tap a quick pick, or customize below"));
    return;
  }
  sel.forEach((f) => {
    const c = fmtChip(f);
    c.classList.add("on");
    c.title = "Tap to remove";
    box.appendChild(c);
  });
}

function setCustomize(open) {
  $("customize").hidden = !open;
  $("btn-customize").classList.toggle("open", open);
  prefSet("nv-custom", open ? "1" : "0");
}
$("btn-customize").addEventListener("click", () => setCustomize($("customize").hidden));

function syncFormatUI() {
  document.querySelectorAll("#fmt-groups .fmt-chip").forEach((c) => c.classList.toggle("on", state.selected.has(c.dataset.id)));
  renderPicked();
  document.querySelectorAll("#quick-picks button").forEach((b) => b.classList.toggle("active", pickMatches(b.dataset.pick)));
  prefSet(formatsKey(), JSON.stringify([...state.selected]));
  showFmtInfo(null);
  updateEstimate();
}

function toggleFormat(id) {
  if (state.selected.has(id)) state.selected.delete(id); else state.selected.add(id);
  syncFormatUI();
}

function toggleGroup(fmts) {
  const allOn = fmts.every((f) => state.selected.has(f.id));
  fmts.forEach((f) => (allOn ? state.selected.delete(f.id) : state.selected.add(f.id)));
  syncFormatUI();
}

const pickIds = (key) => state.formats.filter((f) => f.picks.includes(key)).map((f) => f.id);

function applyPick(key) {
  state.selected = new Set(pickIds(key));
  syncFormatUI();
}

function pickMatches(key) {
  const ids = pickIds(key);
  return ids.length === state.selected.size && ids.every((id) => state.selected.has(id));
}

document.querySelectorAll("#quick-picks button").forEach((b) => b.addEventListener("click", () => applyPick(b.dataset.pick)));
$("btn-clear-formats").addEventListener("click", () => { state.selected.clear(); syncFormatUI(); });

function showFmtInfo(f) {
  const box = $("fmt-info");
  box.innerHTML = "";
  if (f) {
    box.append(el("b", null, f.code), `${f.desc} — e.g. `, el("code", null, f.example));
    if (f.sweep) box.append(` · ${fmtNum(f.left)} of ${fmtNum(f.size)} not checked yet · free ones are very rare`);
    else box.append(f.odds === null ? "" : ` · ${f.odds}% free in tests`);
    return;
  }
  const n = state.selected.size;
  box.append(n
    ? `${plural(n, "format")} picked. Hover one to see what it makes.`
    : "Quick picks choose a few formats for you. Customize shows all 26.");
}

/* ═══════════════════════════════════════════
   SETUP — amount + estimate
═══════════════════════════════════════════ */

// "" / "0" / "∞" / "inf" → endless (0); otherwise any positive number, 10k / 1m allowed
function parseAmount(raw) {
  const s = (raw || "").trim().toLowerCase().replace(/[,_\s]/g, "");
  if (!s || s === "0" || s === "∞" || s === "inf" || s === "infinite") return 0;
  const m = s.match(/^(\d+(?:\.\d+)?)(k|m)?$/);
  if (!m) return null;
  const n = Math.round(parseFloat(m[1]) * (m[2] === "k" ? 1e3 : m[2] === "m" ? 1e6 : 1));
  return n >= 1 ? n : null;
}

document.querySelectorAll(".presets button").forEach((b) =>
  b.addEventListener("click", () => { $("count-field").value = b.dataset.n; updateEstimate(); }));
$("count-field").addEventListener("input", updateEstimate);

function updateEstimate() {
  const box = $("estimate");
  box.className = "estimate";
  box.textContent = "";
  const sel = state.formats.filter((f) => state.selected.has(f.id));
  const amount = parseAmount($("count-field").value);
  if (!sel.length) return;
  if (amount === null) { box.classList.add("bad"); box.textContent = "Type a number, like 500, 10k or ∞"; return; }
  if (sel.every((f) => f.sweep)) {
    const left = sel.reduce((sum, f) => sum + f.left, 0);
    const n = amount ? Math.min(amount, left) : left;
    box.classList.add("meh");
    box.textContent = !left ? "Every name in these styles has been checked already"
      : `${amount && amount < left ? `Checks ${fmtNum(n)} of the ${fmtNum(left)} left` : `Checks all ${fmtNum(left)} left`}`
        + ` · ${fmtDuration(n / checksPerMin())} · expect very few free`;
    return;
  }
  const avg = sel.reduce((sum, f) => sum + oddsOf(f), 0) / sel.length;
  if (avg < 1) {
    box.classList.add("bad");
    box.textContent = "These formats are almost all taken — expect few or no hits";
    return;
  }
  box.classList.add(avg >= 40 ? "good" : "meh");
  box.textContent = amount
    ? `≈ ${fmtNum(Math.round((amount * avg) / 100))} of ${fmtNum(amount)} should be free`
    : `≈ ${Math.round(avg)}% of names should be free`;
}

/* ═══════════════════════════════════════════
   SETUP — my list
═══════════════════════════════════════════ */

function listNames() {
  const seen = new Set();
  $("list-box").value.replace(/,/g, "\n").split("\n").forEach((l) => {
    const n = l.trim().replace(/^@/, "").toLowerCase();
    if (n) seen.add(n);
  });
  return [...seen];
}

function updateListCount() { $("list-count").textContent = plural(listNames().length, "name"); }
$("list-box").addEventListener("input", updateListCount);

$("btn-open-txt").addEventListener("click", () => $("file-input").click());
$("file-input").addEventListener("change", async (e) => {
  const file = e.target.files[0];
  e.target.value = "";
  if (!file) return;
  const text = (await file.text()).trim();
  const box = $("list-box");
  box.value = box.value.trim() ? `${box.value.trim()}\n${text}` : text;
  updateListCount();
  toast(`Loaded ${file.name}`, "good");
});
$("btn-clear-list").addEventListener("click", () => { $("list-box").value = ""; updateListCount(); });

/* ═══════════════════════════════════════════
   SETUP — speed & options
═══════════════════════════════════════════ */

const WORKER_NOTES = { 10: "gentle", 25: "fast", 50: "max speed" };

function setWorkers(n) {
  state.workers = n;
  document.querySelectorAll("#speed-seg button").forEach((b) => b.classList.toggle("active", +b.dataset.w === n));
  const note = $("workers-note");
  note.textContent = WORKER_NOTES[n];
  note.classList.toggle("hot", n === 50);
  prefSet("nv-workers", String(n));
  updateProxyInfo();
  updateEstimate();
}
document.querySelectorAll("#speed-seg button").forEach((b) => b.addEventListener("click", () => setWorkers(+b.dataset.w)));
$("skip-checked").addEventListener("change", (e) => prefSet("nv-skip", e.target.checked ? "1" : "0"));
$("rare-alert").addEventListener("change", (e) => prefSet("nv-rare", e.target.checked ? "1" : "0"));

// a free 4-character name is rare and gets sniped fast — make it impossible to miss
let audioCtx = null;
function primeAudio() {
  if (!$("rare-alert").checked) return;
  try {
    audioCtx = audioCtx || new AudioContext();
    audioCtx.resume();
  } catch (_) { /* no audio device */ }
}

function chime() {
  if (!audioCtx) return;
  const now = audioCtx.currentTime;
  [880, 1175, 1568].forEach((freq, i) => {
    const t = now + i * 0.11;
    const osc = audioCtx.createOscillator();
    const gain = audioCtx.createGain();
    osc.type = "sine";
    osc.frequency.value = freq;
    gain.gain.setValueAtTime(0.0001, t);
    gain.gain.exponentialRampToValueAtTime(0.22, t + 0.02);
    gain.gain.exponentialRampToValueAtTime(0.0001, t + 0.4);
    osc.connect(gain).connect(audioCtx.destination);
    osc.start(t);
    osc.stop(t + 0.45);
  });
}

function rareHit(name) {
  if (name.length > 4 || !$("rare-alert").checked) return;
  chime();
  toast(`${name.length}-character name is free: ${name} — claim it fast`, "good",
        { label: "Copy", fn: () => copyText(name) });
}
$("advanced").addEventListener("toggle", () => prefSet("nv-adv", $("advanced").open ? "1" : "0"));

function setStatus(text, color) {
  const s = $("gen-status");
  s.textContent = text;
  s.style.color = color || "";
}

function fail(msg) {
  setStatus(msg, "var(--red)");
  toast(msg, "bad");
}

// one main button: Start while idle, Stop while a scan runs
function syncGoButton() {
  const btn = $("btn-generate");
  btn.classList.toggle("running", state.running);
  btn.disabled = state.stopping;
  btn.querySelector(".go-label").textContent = state.stopping ? "STOPPING…"
    : state.running ? "STOP" : state.kind === "list" ? "CHECK LIST" : "START SCAN";
}

function setRunning(on) {
  state.running = on;
  if (!on) state.stopping = false;
  syncGoButton();
  $("btn-recheck").disabled = on || !state.hitsShown.length;
  $("live-pill").classList.toggle("on", on);
  $("live-text").textContent = on ? "LIVE" : "IDLE";
  $("hero").classList.toggle("running", on);
}

/* ═══════════════════════════════════════════
   HERO / SCRAMBLE
═══════════════════════════════════════════ */

const GLYPHS = "abcdefghijklmnopqrstuvwxyz0123456789_.#$%&";
let scrambleRAF = 0;

function scrambleTo(text, cls) {
  const nameEl = $("reveal-name");
  nameEl.className = "hero-name " + (cls || "");
  cancelAnimationFrame(scrambleRAF);
  const start = performance.now();
  const dur = 260;
  function frame(t) {
    const p = Math.min(1, (t - start) / dur);
    const settled = Math.floor(p * text.length);
    let out = text.slice(0, settled);
    for (let i = settled; i < text.length; i++) out += GLYPHS[Math.floor(Math.random() * GLYPHS.length)];
    nameEl.textContent = out;
    if (p < 1) scrambleRAF = requestAnimationFrame(frame);
  }
  scrambleRAF = requestAnimationFrame(frame);
}

function setHero(name, chipText, kind, kicker) {
  scrambleTo(name, kind ? "c-" + kind : "");
  const chip = $("reveal-chip");
  chip.textContent = chipText;
  chip.className = "chip" + (kind ? " k-" + kind : "");
  if (kicker) $("hero-kicker").textContent = kicker;
}

function flashHit() {
  const h = $("hero");
  h.classList.remove("flash-valid");
  void h.offsetWidth;
  h.classList.add("flash-valid");
}

/* ═══════════════════════════════════════════
   STATS
═══════════════════════════════════════════ */

function resetStats() {
  state.stats = { valid: 0, taken: 0, retry: 0 };
  state.retries = {};
  state.doneTimes = [];
  renderStats();
  renderRetries();
}

// retries = { reason: count } — bad exit IPs that were retried on a fresh IP
function renderRetries() {
  const entries = Object.entries(state.retries).sort((a, b) => b[1] - a[1]);
  const total = entries.reduce((sum, [, n]) => sum + n, 0);
  const foot = $("foot-retry");
  foot.textContent = `${fmtNum(total)} retried`;
  foot.title = total
    ? "Retried on a fresh IP: " + entries.map(([why, n]) => `${fmtNum(n)} ${why}`).join(", ")
    : "Bad exit IPs are retried on a fresh IP automatically";
}

function bump(id, value) {
  const n = $(id);
  n.textContent = fmtNum(value);
  n.classList.remove("bump");
  void n.offsetWidth;
  n.classList.add("bump");
}

function renderStats() {
  const s = state.stats;
  const done = s.valid + s.taken + s.retry;
  $("stat-valid").textContent = fmtNum(s.valid);
  $("stat-taken").textContent = fmtNum(s.taken);
  $("stat-retry").textContent = fmtNum(s.retry);
  const pct = (v) => (done ? (v / done) * 100 : 0) + "%";
  $("bar-valid").style.width = pct(s.valid);
  $("bar-taken").style.width = pct(s.taken);
  $("bar-retry").style.width = pct(s.retry);
  const answered = s.valid + s.taken;
  $("foot-valid").textContent = answered ? `${Math.round((s.valid / answered) * 100)}% hit rate` : "hit rate —";
  $("foot-taken").textContent = `${fmtNum(done)} checked`;
}

function setRing(done, total) {
  const C = 326.73;
  const ring = $("ring-fg").parentElement;
  if (!total) {
    // endless run: keep a quarter arc spinning instead of a percentage
    $("ring-fg").style.strokeDashoffset = state.running ? C * 0.75 : C;
    ring.classList.toggle("spin", state.running);
    $("ring-pct").textContent = "∞";
    $("ring-sub").textContent = `${fmtNum(done)} checked`;
    return;
  }
  ring.classList.remove("spin");
  const p = done / total;
  $("ring-fg").style.strokeDashoffset = C * (1 - p);
  $("ring-pct").textContent = Math.round(p * 100) + "%";
  $("ring-sub").textContent = `${fmtNum(done)} / ${fmtNum(total)}`;
}

const queuedText = (total) => (total ? `${fmtNum(total)} names queued` : "endless — runs until you press Stop");
const progressText = (n, total) => (total ? `${fmtNum(n)} / ${fmtNum(total)} checked` : `${fmtNum(n)} checked`);

function tickSpeed() {
  const now = performance.now();
  state.doneTimes = state.doneTimes.filter((t) => now - t < 30000);
  const windowSec = Math.min(30, Math.max(1, (now - state.startedAt) / 1000));
  const perMin = state.running ? Math.round((state.doneTimes.length / windowSec) * 60) : 0;
  $("stat-speed").textContent = fmtNum(perMin);
  const s = state.stats;
  const remaining = state.total - (s.valid + s.taken + s.retry);
  $("stat-eta").textContent = !state.total && state.running ? "ETA ∞"
    : state.running && perMin > 0 ? "ETA " + fmtTime((remaining / perMin) * 60) : "ETA —";
  if (state.running) $("run-timer").textContent = fmtTime((now - state.startedAt) / 1000);
}
setInterval(tickSpeed, 500);

/* ═══════════════════════════════════════════
   ACTIVITY FEED
═══════════════════════════════════════════ */

const TAGS = { "row-available": "HIT", "row-taken": "TAKEN", "row-retry": "FAIL", "row-status": "SYS", "row-error": "ERROR" };

function feed(text, cls) {
  const list = $("history-list");
  const row = el("div", "row " + cls);
  row.title = text;
  row.append(el("span", "ts", nowStamp()), el("span", "tag", TAGS[cls] || ""), el("span", "msg", text));
  list.prepend(row);
  while (list.children.length > 400) list.lastChild.remove();
}

document.querySelectorAll("#feed-filters button").forEach((b) =>
  b.addEventListener("click", () => {
    document.querySelectorAll("#feed-filters button").forEach((x) => x.classList.toggle("active", x === b));
    $("history-list").dataset.filter = b.dataset.f;
  }));

/* ═══════════════════════════════════════════
   HITS — shared actions
═══════════════════════════════════════════ */

async function copyText(text, label) {
  await api().copy_text(text);
  toast(label || `Copied ${text}`, "good");
}

async function copyNames(names) {
  if (!names.length) return toast("Nothing to copy yet");
  await copyText(names.join("\n"), `Copied ${plural(names.length, "name")}`);
}

async function saveNames(names, label) {
  if (!names.length) return toast("Nothing to save yet");
  const r = await api().save_names(names, label);
  if (!r.ok) return toast(r.error || "Couldn't save the file", "bad");
  toast(`Saved ${plural(r.count, "name")} to ${r.file}`, "good", { label: "Show file", fn: () => api().reveal(r.path) });
}

async function addToVault(name, platform) {
  const r = await api().vault_add(name, "", vaultNote(platform));
  toast(r.added ? `${name} added to Vault` : `${name} is already in Vault`, r.added ? "good" : "");
}

const sessionNames = () => state.hits.map((h) => h.name);

/* ═══════════════════════════════════════════
   HITS — scanner panel
═══════════════════════════════════════════ */

const GUIDE_HTML = `
  <div class="guide">
    <div class="guide-row"><span class="step-n">1</span><span>Pick what to look for — <b>Best odds</b> is a good start</span></div>
    <div class="guide-row"><span class="step-n">2</span><span>Choose how many — <b>∞</b> keeps going until you press Stop</span></div>
    <div class="guide-row"><span class="step-n">3</span><span>Press <b>Start</b> — free names show up right here</span></div>
  </div>`;

function hitRow(h) {
  const row = el("div", "hit-row");
  row.appendChild(el("span", "nm", h.name));
  if (h.fmt) row.appendChild(el("span", "fmt-tag", h.fmt));
  row.append(iconBtn(ICON_COPY, "Copy", () => copyText(h.name)),
             iconBtn(ICON_PLUS, "Add to Vault", () => addToVault(h.name, state.runPlatform)));
  return row;
}

function renderHitsMini() {
  const box = $("hits-mini");
  box.innerHTML = "";
  if (!state.hits.length) { box.innerHTML = GUIDE_HTML; return; }
  state.hits.slice(0, 80).forEach((h) => box.appendChild(hitRow(h)));
}

function updateHitCounts() {
  const n = state.hits.length;
  $("hits-count").textContent = fmtNum(n);
  const badge = $("nav-hits-badge");
  badge.textContent = fmtNum(n);
  badge.classList.toggle("zero", n === 0);
}

function clearSessionHits() {
  state.hits = [];
  renderHitsMini();
  updateHitCounts();
}

function addHit(name, fmt) {
  const h = { name, fmt, at: nowStamp() };
  state.hits.unshift(h);
  const box = $("hits-mini");
  if (state.hits.length === 1) box.innerHTML = "";
  box.prepend(hitRow(h));
  while (box.children.length > 80) box.lastChild.remove();
  updateHitCounts();
  if (state.page === "hits" && state.hitsView === "session") scheduleHitsRender();
}

$("btn-save-hits-mini").addEventListener("click", () => saveNames(sessionNames(), "available"));
$("btn-copy-hits-mini").addEventListener("click", () => copyNames(sessionNames()));

/* ═══════════════════════════════════════════
   HITS — page
═══════════════════════════════════════════ */

let hitsRenderTimer = 0;
function scheduleHitsRender() {
  if (hitsRenderTimer) return;
  hitsRenderTimer = setTimeout(() => { hitsRenderTimer = 0; renderHitsPage(); }, 350);
}

async function hitItems() {
  if (state.hitsView === "session") {
    if (state.runPlatform !== state.hitsPlatform) return [];
    return state.hits.map((h) => ({ name: h.name, meta: `${h.fmt ? h.fmt + " · " : ""}found ${h.at}` }));
  }
  if (!state.allHits) state.allHits = await api().get_all_hits(state.hitsPlatform);
  return state.allHits.map((n) => ({ name: n, meta: `in ${VALID_FILE[state.hitsPlatform]}` }));
}

function setHitsPlatform(platform, render = true) {
  state.hitsPlatform = platform;
  segSelect($("hits-platform"), "platform", platform);
  state.allHits = null;
  if (render) showPage("hits");
}
document.querySelectorAll("#hits-platform button").forEach((b) =>
  b.addEventListener("click", () => setHitsPlatform(b.dataset.platform)));

async function renderHitsPage() {
  let items = await hitItems();
  const q = state.hitsQuery.trim().toLowerCase();
  if (q) items = items.filter((i) => i.name.includes(q));
  if (state.hitsSort === "short") items = [...items].sort((a, b) => a.name.length - b.name.length || a.name.localeCompare(b.name));
  else if (state.hitsSort === "az") items = [...items].sort((a, b) => a.name.localeCompare(b.name));
  state.hitsShown = items.map((i) => i.name);

  const n = items.length;
  $("hits-note").textContent = plural(n, "name") + (n > HIT_CARDS ? ` · showing the first ${HIT_CARDS}` : "");
  $("btn-recheck").textContent = n ? `Re-check ${fmtNum(n)}` : "Re-check";
  ["btn-save-hits", "btn-copy-hits", "btn-hits-vault"].forEach((id) => ($(id).disabled = !n));
  $("btn-recheck").disabled = !n || state.running;

  const grid = $("hits-grid");
  grid.innerHTML = "";
  if (!n) {
    const site = PLATFORM_NAME[state.hitsPlatform];
    grid.appendChild(el("div", "empty big",
      q ? "No names match that search."
        : state.hitsView === "all" ? `${VALID_FILE[state.hitsPlatform]} is empty — run a ${site} scan first.`
        : state.runPlatform !== state.hitsPlatform && state.hits.length
          ? `Your last run was on ${PLATFORM_NAME[state.runPlatform]} — pick All time to see every ${site} name.`
          : `Nothing from a ${site} run yet — start a scan on the ${site} tab and free names land here.`));
    return;
  }
  items.slice(0, HIT_CARDS).forEach((h, i) => {
    const card = el("div", "hit-card");
    card.style.animationDelay = Math.min(i, 20) * 20 + "ms";
    card.append(el("div", "nm", h.name), el("div", "meta", `${h.meta} · ${h.name.length} chars`));
    const acts = el("div", "acts");
    const c = el("button", "btn", "Copy");
    c.addEventListener("click", () => copyText(h.name));
    const v = el("button", "btn", "+ Vault");
    v.addEventListener("click", () => addToVault(h.name, state.hitsPlatform));
    acts.append(c, v);
    card.appendChild(acts);
    grid.appendChild(card);
  });
}

document.querySelectorAll("#hits-view button").forEach((b) => b.addEventListener("click", () => {
  state.hitsView = b.dataset.view;
  segSelect($("hits-view"), "view", state.hitsView);
  if (state.hitsView === "all") state.allHits = null;
  renderHitsPage();
}));
$("hits-search").addEventListener("input", (e) => { state.hitsQuery = e.target.value; scheduleHitsRender(); });
$("hits-sort").addEventListener("change", (e) => { state.hitsSort = e.target.value; renderHitsPage(); });

$("btn-save-hits").addEventListener("click", () =>
  saveNames(state.hitsShown, state.hitsView === "all" ? "all_hits" : "available"));
$("btn-copy-hits").addEventListener("click", () => copyNames(state.hitsShown));
$("btn-recheck").addEventListener("click", () => recheck(state.hitsShown));
$("btn-hits-vault").addEventListener("click", async () => {
  const r = await api().vault_add_many(state.hitsShown, vaultNote(state.hitsPlatform));
  toast(r.added ? `Added ${plural(r.added, "name")} to Vault` : "They're all in Vault already", r.added ? "good" : "");
});

async function recheck(names) {
  if (state.running) return toast("Stop the current scan first", "bad");
  if (!names.length) return;
  // re-check on the site these names came from
  if (!(await openPlatform(state.hitsPlatform))) return;
  $("list-box").value = names.join("\n");
  updateListCount();
  setKind("list");
  onStart();
}

/* ═══════════════════════════════════════════
   RUN
═══════════════════════════════════════════ */

async function onStart() {
  if (state.running) return;
  const listMode = state.kind === "list";
  let count;
  let formats = [];
  let listText = "";

  if (listMode) {
    const names = listNames();
    if (!names.length) return fail("Add some names to check first");
    count = names.length;
    listText = names.join("\n");
  } else {
    count = parseAmount($("count-field").value);
    if (count === null) return fail("Amount must be a number, 10k, 1m or ∞");
    formats = [...state.selected];
    if (!formats.length) return fail("Pick at least one format");
  }

  const proxiesText = $("proxy-box").value;
  const direct = !proxiesText.trim();
  primeAudio();   // browsers only allow sound that starts from a click

  $("history-list").innerHTML = "";
  resetStats();
  clearSessionHits();
  state.runPlatform = state.platform;
  state.total = count;
  state.startedAt = performance.now();
  setRunning(true);
  setRing(0, count);
  setHero("starting", direct ? "DIRECT" : "BOOTING", "checking", direct ? "// direct connection, no proxy" : "// spinning up workers");
  $("reveal-progress").textContent = queuedText(count);
  setStatus(count ? `${fmtNum(count)} queued` : "endless run");

  const r = await api().start_checking(listMode ? "list" : "gen", formats, count, proxiesText,
                                       state.workers, $("skip-checked").checked, listText, state.platform);
  if (!r.ok) {
    setRunning(false);
    setRing(0, 0);
    setHero("namevault", "IDLE", "", "// awaiting target");
    return fail(r.error || "Couldn't start");
  }
  if (r.total && count && r.total < count) {
    state.total = r.total;
    setRing(0, r.total);
    $("reveal-progress").textContent = queuedText(r.total);
    toast(`Only ${fmtNum(r.total)} unchecked names left in these formats`);
  }
}

async function onStop() {
  if (!state.running || state.stopping) return;
  state.stopping = true;
  syncGoButton();
  setStatus("stopping — finishing the checks already in flight…", "var(--yellow)");
  await api().stop_checking();
}

async function onMakeList() {
  let count = parseAmount($("count-field").value);
  if (!state.selected.size) return fail("Pick at least one format");
  const sel = state.formats.filter((f) => state.selected.has(f.id));
  if (count === 0 && sel.every((f) => f.sweep)) {
    count = Math.min(1e6, sel.reduce((sum, f) => sum + f.left, 0));   // ∞ = everything left in the sweep
  }
  if (!count) return fail("Pick a number (not ∞) to save a list");
  const btn = $("btn-make-list");
  btn.disabled = true;
  try {
    const r = await api().generate_to_file([...state.selected], count, $("skip-checked").checked, state.platform);
    if (!r.ok) return fail(r.error || "Couldn't make the list");
    toast(`Saved ${plural(r.count, "name")} to ${r.file} (not checked)`, "good",
          { label: "Show file", fn: () => api().reveal(r.path) });
  } finally {
    btn.disabled = false;
  }
}

$("btn-generate").addEventListener("click", () => (state.running ? onStop() : onStart()));
$("btn-make-list").addEventListener("click", onMakeList);
document.addEventListener("keydown", (e) => {
  if (e.key === "Enter" && e.ctrlKey) { e.preventDefault(); onStart(); }
  if (e.key === "Escape") onStop();
});

function finish() {
  setRunning(false);
  const s = state.stats;
  setRing(s.valid + s.taken + s.retry, state.total);
  tickSpeed();
  state.allHits = null;
  if (state.page === "hits") renderHitsPage();
  refreshFormats();
}

// sweep chips show "N left", which changes as names get checked
async function refreshFormats() {
  state.formats = await api().get_formats(state.platform);
  renderFormats();
  syncFormatUI();
}

function runEndToast(prefix) {
  const n = state.stats.valid;
  toast(`${prefix} — ${plural(n, "available name")}`, n ? "good" : "",
        n ? { label: "Save .txt", fn: () => saveNames(sessionNames(), "available") } : null);
}

function handleEvent({ type, data }) {
  const s = state.stats;
  switch (type) {
    case "checking":
      setHero(data.username, "CHECKING", "checking", `// probing ${state.platform}`);
      break;

    case "valid":
    case "taken": {
      const valid = type === "valid";
      if (valid) { s.valid++; bump("stat-valid", s.valid); } else { s.taken++; }
      state.doneTimes.push(performance.now());
      renderStats();
      setRing(data.n, data.total);
      $("reveal-progress").textContent = progressText(data.n, data.total);
      if (valid) {
        setHero(data.username, "AVAILABLE", "valid", "// hit");
        flashHit();
        addHit(data.username, data.fmt);
        feed(data.username, "row-available");
        rareHit(data.username);
      } else {
        feed(data.username, "row-taken");
      }
      break;
    }

    case "retry":
    case "invalid":
      s.retry++;
      state.doneTimes.push(performance.now());
      renderStats();
      setRing(data.n, data.total);
      feed(type === "invalid" ? `${data.username} — not allowed by Discord`
        : `${data.username} — gave up after 5 tries (${data.reason || "no answer"})`, "row-retry");
      break;

    case "status":
      setStatus(data);
      feed(data, "row-status");
      break;

    case "log":
      feed(data, "row-status");
      break;

    case "error":
      feed(String(data), "row-error");
      setHero("error", "ERROR", "taken", "// run crashed");
      setStatus(String(data), "var(--red)");
      toast("Run crashed: " + data, "bad");
      finish();
      break;

    case "stopped":
      feed(data, "row-retry");
      setHero("stopped", "STOPPED", "warn", "// halted by user");
      setStatus(data, "var(--yellow)");
      finish();
      runEndToast("Stopped");
      break;

    case "done":
      feed(data, "row-status");
      setHero("complete", "DONE", "done", "// scan finished");
      setStatus(data, "var(--green)");
      finish();
      runEndToast("Done");
      break;
  }
}

async function pump() {
  for (;;) {
    try {
      const r = await api().poll_events();
      if (state.running && r.retries) { state.retries = r.retries; renderRetries(); }
      r.events.forEach(handleEvent);
      // thread died without a done/stopped event
      if (state.running && !r.running && !r.events.length) {
        await sleep(300);
        const again = await api().poll_events();
        again.events.forEach(handleEvent);
        if (state.running && !again.running) finish();
      }
    } catch (_) { /* bridge hiccup — try again next tick */ }
    await sleep(state.running ? 120 : 500);
  }
}

/* ═══════════════════════════════════════════
   PROXIES
═══════════════════════════════════════════ */

function proxyLines() { return $("proxy-box").value.split("\n").map((l) => l.trim()).filter(Boolean); }

// mirrors checker_core.normalize_proxy: user:pass@host:port, host:port:user:pass, user:pass:host:port
function hostOf(line) {
  const s = line.trim().replace(/^[a-z0-9]+:\/\//i, "");
  const tail = s.includes("@") ? s.split("@").pop() : "";
  if (/^[^:]+:\d+$/.test(tail)) return tail.split(":")[0];
  const p = s.split(":");
  const looksHost = (h) => h.includes(".") || h === "localhost";
  if (p.length >= 4 && /^\d+$/.test(p[1]) && looksHost(p[0])) return p[0];
  if (p.length >= 4 && /^\d+$/.test(p[p.length - 1]) && looksHost(p[p.length - 2])) return p[p.length - 2];
  return p[0] || "";
}
const isIp = (h) => /^\d{1,3}(\.\d{1,3}){3}$/.test(h) || h.includes("[");

function updateProxyInfo() {
  const lines = proxyLines();
  const unique = [...new Set(lines)];
  const rotating = unique.filter((l) => !isIp(hostOf(l)));
  const schemes = [...new Set(unique.map((l) => (l.match(/^([a-z0-9]+):\/\//i) || [, "http"])[1].toLowerCase()))];

  $("proxy-count").textContent = `${lines.length} lines · ${unique.length} unique`;

  let mode, sub, ok;
  if (!lines.length) { mode = "Direct"; sub = "no proxies — slow, 1 at a time"; ok = false; }
  else if (rotating.length === unique.length) { mode = "Rotating"; sub = hostOf(unique[0]); ok = true; }
  else { mode = `${unique.length} static`; sub = `${unique.length - rotating.length} fixed IPs`; ok = true; }
  $("conn-label").textContent = mode;
  $("conn-sub").textContent = sub;
  $("conn-dot").classList.toggle("ok", ok);

  const workers = !lines.length ? "1 (direct)" : String(state.workers);
  const d = $("proxy-detect");
  d.innerHTML = "";
  [
    ["Mode", mode, ok ? "good" : "warn"],
    ["Protocols", schemes.join(", ") || "—", ""],
    ["Rotating gateways", String(rotating.length), rotating.length ? "good" : ""],
    ["Parallel checks", workers, ""],
    ["Roblox sign-up check", "from this PC", ""],
  ].forEach(([k, v, cls]) => {
    const r = el("div", "detect-row");
    r.append(el("span", null, k), el("b", cls, v));
    d.appendChild(r);
  });
}
$("proxy-box").addEventListener("input", updateProxyInfo);

$("btn-save-proxies").addEventListener("click", async () => {
  const r = await api().save_proxies($("proxy-box").value);
  toast(`Saved ${plural(r.count, "proxy line")}`, "good");
});
$("btn-load-saved").addEventListener("click", async () => {
  $("proxy-box").value = await api().load_proxies("proxies.txt");
  updateProxyInfo();
  toast("Loaded proxies.txt");
});
$("btn-load-working").addEventListener("click", async () => {
  $("proxy-box").value = await api().load_proxies("proxies_working.txt");
  updateProxyInfo();
  toast("Loaded proxies_working.txt");
});

$("btn-test-proxies").addEventListener("click", async () => {
  const btn = $("btn-test-proxies");
  const box = $("test-results");
  if (!proxyLines().length) { toast("No proxies to test", "bad"); return; }
  btn.disabled = true;
  box.innerHTML = '<div class="empty"><span class="spinner"></span>&nbsp; probing discord…</div>';
  try {
    const res = await api().test_proxies($("proxy-box").value);
    box.innerHTML = "";
    const good = res.filter((r) => r.ok).length;
    res.forEach((r, i) => {
      const row = el("div", "test-row " + (r.ok ? "ok" : "bad"));
      row.style.animationDelay = i * 30 + "ms";
      row.append(el("span", "d"), el("span", "p", r.proxy), el("span", "ms", r.ok ? r.ms + "ms" : (r.status || "dead")));
      const lat = el("div", "lat");
      const fill = el("span");
      fill.style.width = r.ok ? Math.min(100, (r.ms / 8000) * 100) + "%" : "0";
      lat.appendChild(fill);
      row.appendChild(lat);
      box.appendChild(row);
    });
    toast(`${good}/${res.length} proxies reached Discord`, good ? "good" : "bad");
  } catch (e) {
    box.innerHTML = '<div class="empty">Test failed</div>';
  } finally {
    btn.disabled = false;
  }
});

/* ═══════════════════════════════════════════
   VAULT
═══════════════════════════════════════════ */

async function refreshVault() {
  const listings = await api().vault_list();
  const box = $("vault-list");
  box.innerHTML = "";
  if (!listings.length) { box.innerHTML = '<div class="empty big">Vault is empty — add a name above or import from valid.txt.</div>'; return; }
  listings.forEach((l, i) => {
    const item = el("div", "vault-item");
    item.style.animationDelay = Math.min(i, 20) * 20 + "ms";
    const info = el("div", "info");
    info.append(el("div", "uname", "@" + l.username), el("div", "sub", (l.notes ? l.notes + " · " : "") + "added " + (l.added || "")));
    const price = el("span", "price" + (l.price ? "" : " none"), l.price ? "$" + l.price : "no price");
    const rm = el("button", "btn btn-danger", "Remove");
    rm.addEventListener("click", async () => {
      await api().vault_remove(l.username);
      toast(`Removed ${l.username}`);
      refreshVault();
    });
    item.append(el("div", "av", l.username[0] ? l.username[0].toUpperCase() : "?"), info, price,
                iconBtn(ICON_COPY, "Copy", () => copyText(l.username)), rm);
    box.appendChild(item);
  });
}

$("btn-vault-add").addEventListener("click", async () => {
  const username = $("vault-username").value.trim().replace(/^@/, "");
  if (!username) { toast("Enter a username", "bad"); return; }
  const r = await api().vault_add(username, $("vault-price").value.trim(), $("vault-notes").value.trim());
  if (!r.added) { toast("Already listed"); return; }
  ["vault-username", "vault-price", "vault-notes"].forEach((id) => ($(id).value = ""));
  toast("Listing added", "good");
  refreshVault();
});
$("btn-vault-import").addEventListener("click", async () => {
  const r = await api().vault_import_valid();
  toast(`Imported ${fmtNum(r.added)} new`, "good");
  refreshVault();
});
$("btn-vault-copy").addEventListener("click", async () => {
  const r = await api().vault_copy_all();
  toast(`Copied ${plural(r.count, "listing")}`, "good");
});

/* ═══════════════════════════════════════════
   CURSOR TRAIL
═══════════════════════════════════════════ */

(function cursorTrail() {
  const canvas = $("fx");
  const ctx = canvas.getContext("2d");
  let w, h;
  const resize = () => { w = canvas.width = innerWidth; h = canvas.height = innerHeight; };
  addEventListener("resize", resize);
  resize();
  const pts = [];
  addEventListener("mousemove", (e) => {
    pts.push({ x: e.clientX, y: e.clientY, t: performance.now() });
    if (pts.length > 24) pts.shift();
  });
  (function tick() {
    ctx.clearRect(0, 0, w, h);
    const now = performance.now();
    while (pts.length && now - pts[0].t > 320) pts.shift();
    for (let i = 1; i < pts.length; i++) {
      const a = pts[i - 1], b = pts[i];
      const age = (now - b.t) / 320;
      ctx.beginPath();
      ctx.moveTo(a.x, a.y);
      ctx.lineTo(b.x, b.y);
      ctx.strokeStyle = `rgba(150,135,255,${Math.max(0, 1 - age) * 0.45})`;
      ctx.lineWidth = Math.max(1, 3.5 * (1 - age));
      ctx.lineCap = "round";
      ctx.shadowBlur = 10;
      ctx.shadowColor = "rgba(124,108,255,0.9)";
      ctx.stroke();
    }
    requestAnimationFrame(tick);
  })();
})();

/* ═══════════════════════════════════════════
   INIT
═══════════════════════════════════════════ */

(async function init() {
  API = await waitForBridge();
  const s = await api().get_state();
  // picks saved before Roblox existed belong to Discord
  if (prefGet("nv-formats") && !prefGet("nv-formats-discord")) prefSet("nv-formats-discord", prefGet("nv-formats"));
  state.platform = prefGet("nv-platform") === "roblox" ? "roblox" : "discord";
  setHitsPlatform(state.platform, false);
  await loadFormats();
  showPage("scan");   // opens on the last tab used (Discord or Roblox)

  $("proxy-box").value = s.proxies_text || "";
  setWorkers([10, 25, 50].includes(+prefGet("nv-workers")) ? +prefGet("nv-workers") : 50);
  if (prefGet("nv-skip") === "0") $("skip-checked").checked = false;
  if (prefGet("nv-rare") === "0") $("rare-alert").checked = false;
  if (prefGet("nv-adv") === "1") $("advanced").open = true;
  setCustomize(prefGet("nv-custom") === "1");
  setKind(prefGet("nv-kind") === "list" ? "list" : "gen");
  setRunning(!!s.running);
  if (s.running) state.startedAt = performance.now();
  renderHitsMini();
  updateHitCounts();
  pump();
})();
