import itertools
import math
import random
import re
import string

# ═══════════════════════════════════════════════════════
#  SHORT GENERATORS (3-4 char)
# ═══════════════════════════════════════════════════════

LETTERS = string.ascii_lowercase
DIGITS = string.digits
VOWELS = "aeiou"


class Sweep:
    """Every name of one style, each exactly once, in a scrambled order —
    without building the list (4 letters alone is 456,976 names)."""

    def __init__(self, alphabets, pattern, prefix="", size=None, keep=None):
        self.alphabets = alphabets          # allowed characters, one string per position
        self.prefix = prefix
        self.keep = keep
        self.regex = re.compile(pattern)
        self.space = math.prod(len(a) for a in alphabets)
        self.size = size or self.space      # names that pass `keep`

    def matches(self, name):
        return bool(self.regex.fullmatch(name))

    def names(self):
        n = self.space
        step = int(n * 0.618) | 1           # coprime step visits every index once
        while math.gcd(step, n) != 1:
            step += 2
        i = random.randrange(n)
        for _ in range(n):
            i = (i + step) % n
            rest, chars = i, []
            for a in reversed(self.alphabets):
                rest, r = divmod(rest, len(a))
                chars.append(a[r])
            name = self.prefix + "".join(reversed(chars))
            if self.keep is None or self.keep(name):
                yield name


def _letter_and_digit(name):
    return any(c in LETTERS for c in name) and any(c in DIGITS for c in name)


ALNUM = LETTERS + DIGITS


def _one_symbol(name):
    return sum(c in "_." for c in name) == 1

# short words people stick a letter or digit in front of (wbot, xgod, 1pro)
SHORT_WORDS = list(dict.fromkeys("""
bot god pro max vip ace dev fox ice kid sky sun war zen ash cat dog owl ray rex sad tea toy web win
zip zap axe bee bug cap cry eye fly fun gem gun hex hug jet joy key lab lit log lux map mob net oak odd orb pet
pie pop pug raw red rip rot run sea sin six ski spy tax top two van vex wax way wet yak zoo ego emo fig fit hot
jam jaw kin lad lip mad mix mug nap nut ore pal pin pit ram rap rat rib rim rod rub sip sly sob tag tan tin tub
vow wig yen zag""".split()))


def gen_3m():
    if random.random() < 0.5:
        n = random.choice(DIGITS)
        return n + random.choice(LETTERS) + n
    else:
        l = random.choice(LETTERS)
        return l + random.choice(DIGITS) + l


def gen_3s():
    patterns = [
        lambda: random.choice(LETTERS) + random.choice(DIGITS) + random.choice(LETTERS),
        lambda: random.choice(DIGITS) + random.choice(LETTERS) + random.choice(DIGITS),
        lambda: random.choice(LETTERS) + random.choice(LETTERS) + random.choice(DIGITS),
        lambda: random.choice(DIGITS) + random.choice(LETTERS) + random.choice(LETTERS),
        lambda: random.choice(LETTERS) + random.choice(DIGITS) + random.choice(DIGITS),
        lambda: random.choice(DIGITS) + random.choice(DIGITS) + random.choice(LETTERS),
    ]
    return random.choice(patterns)()


PRONOUNCE_C = "bcdfghjklmnprstvwz"


def _with_symbol(base):
    i = random.randint(0, len(base))
    return base[:i] + random.choice("_.") + base[i:]


def gen_4l_sym():
    return _with_symbol(''.join(random.choice(LETTERS) for _ in range(4)))


def gen_4c_sym():
    return _with_symbol(''.join(random.choice(LETTERS + DIGITS) for _ in range(4)))


def gen_5p():
    c, v = PRONOUNCE_C, VOWELS
    return random.choice(c) + random.choice(v) + random.choice(c) + random.choice(v) + random.choice(c)


def gen_5l():
    return ''.join(random.choice(LETTERS) for _ in range(5))


def gen_5c():
    while True:
        result = ''.join(random.choice(LETTERS + DIGITS) for _ in range(5))
        if any(ch in LETTERS for ch in result):
            return result


def _digits(n):
    return ''.join(random.choice(DIGITS) for _ in range(n))


def gen_5n():
    return _digits(5)


def gen_4n_mid():
    d = _digits(4)
    i = random.randint(1, 3)
    return d[:i] + random.choice("_.") + d[i:]


def gen_4n_start():
    return random.choice("_.") + _digits(4)


def gen_4n_end():
    return _digits(4) + random.choice("_.")

# ═══════════════════════════════════════════════════════
#  AESTHETIC / LONG GENERATORS
# ═══════════════════════════════════════════════════════

# ── XnY: word + "n" + word ──

XNY_LEFT = [
    "paid", "racks", "profit", "funded", "lost", "stuck", "numb",
    "rich", "broke", "dead", "cold", "dark", "burnt", "scarred",
    "blessed", "cursed", "woke", "tired", "drained", "faded",
    "hollow", "empty", "toxic", "frozen", "shattered", "ruined",
    "haunted", "blinded", "trapped", "wasted", "loaded", "stacked",
    "dripped", "flexed", "ghosted", "played", "used", "torn",
    "lonely", "silent", "chained", "bruised", "shaded", "clouded",
    "blurred", "muted", "buried", "fried", "flooded", "starved",
    "poisoned", "bitter", "pressed", "bent", "cracked", "sealed",
    "sunken", "quiet", "distant", "far", "hidden", "guarded",
    "afraid", "restless", "sleepless", "worn", "spent", "wounded",
    "mad", "sane", "chosen", "marked", "known", "seen", "heard",
    "given", "taken", "held", "kept", "left", "moved", "raised",
    "born", "shown", "told", "asked", "warned", "saved", "chased",
    "mocked", "missed", "watched", "tested", "tempted", "traded",
]

XNY_RIGHT = [
    "broken", "empty", "numb", "betrayal", "paranoia", "lonely",
    "forgotten", "damaged", "shattered", "scarred", "ruined",
    "haunted", "hollow", "faded", "wasted", "lost", "gone",
    "buried", "silenced", "abandoned", "detached", "displaced",
    "conflicted", "unforgiven", "regretful", "restless", "reckless",
    "ruthless", "heartless", "merciless", "relentless", "careless",
    "hopeless", "pointless", "worthless", "endless", "mindless",
    "loveless", "faceless", "nameless", "clueless", "flawless",
    "harmless", "priceless", "spineless", "toothless", "voiceless",
    "aimless", "boundless", "colorless", "fearless", "graceless",
    "joyless", "lifeless", "motionless", "noiseless", "painless",
    "restless", "sightless", "sleepless", "soundless", "speechless",
    "stainless", "tasteless", "thoughtless", "timeless", "tireless",
    "useless", "weightless", "wireless", "wordless", "goalless",
    "rageful", "spiteful", "wistful", "fateful", "hateful",
    "grateful", "hurtful", "pitiful", "shameful", "blameless",
]


def gen_xny():
    connector = random.choice(["n", "n", "n", "n", "in", "an", "on"])
    return random.choice(XNY_LEFT) + connector + random.choice(XNY_RIGHT)


# ── compound phrases ──

PHRASE_STARTS = [
    "stuckin", "donttrust", "dontlookback", "cantfeel", "cantgo",
    "neverlooked", "ioncur", "loyaltyover", "tearsdont", "mindoutof",
    "quietlysuffer", "processin", "misreadin", "echoesof", "shadowsof",
    "voicesof", "wavesof", "memoryof", "tracesof", "weightin",
    "sinkin", "driftin", "fallin", "burnin", "runnin", "hidin",
    "chasin", "losin", "searchin", "reachin", "breakin", "fakin",
    "watchin", "waitin", "prayin", "hopin", "drownin", "bleedin",
    "dyingfor", "cravin", "missin", "lackin", "aching", "ridin",
    "walkin", "movin", "goin", "leavin", "returnin", "endin",
    "thinkin", "dreamin", "wonderin", "wanderin", "listenin",
    "watchingover", "rememberin", "forgetin", "regrettin", "livin",
    "lyin", "cryin", "sighin", "dyin", "callin", "answerin",
    "showin", "hidingfrom", "runningfrom", "escapin", "avoidin",
    "battlin", "fightin", "strugglin", "holdon", "letgo", "letit",
]

PHRASE_ENDS = [
    "loops", "silence", "anymore", "errythang", "ever", "back",
    "pain", "life", "sync", "replay", "circles", "darkness",
    "absence", "nothing", "emptiness", "regret", "remorse",
    "myself", "thewall", "nowhere", "theabyss", "thedeep",
    "thecold", "thevoid", "theend", "forever", "again",
    "insilence", "alone", "therain", "thenight", "reality",
    "peace", "chaos", "vibes", "vibrations", "energy", "aura",
    "spirit", "soul", "heart", "mind", "body", "essence",
    "dreams", "nightmares", "shadows", "reflections", "memories",
    "moments", "fragments", "pieces", "traces", "echoes", "whispers",
    "myshadow", "myghost", "mydemons", "myangels", "mythoughts",
    "mydreams", "myfears", "mypain", "mytruth", "mylies", "mystory",
    "thepast", "thefuture", "themoment", "thechance", "thesignal",
    "themirror", "thecycle", "thepattern", "thesystem", "thehustle",
]


def gen_compound():
    connector = random.choice(["", "", "", "", "the", "my", "our", "a"])
    if connector:
        return random.choice(PHRASE_STARTS) + connector + random.choice(PHRASE_ENDS)
    return random.choice(PHRASE_STARTS) + random.choice(PHRASE_ENDS)


# ── stretched letters ──

STRETCH_WORDS = [
    "numb", "fool", "foolish", "cold", "dead", "lost", "sad",
    "empty", "pain", "rage", "dark", "broke", "hollow", "tired",
    "drained", "faded", "gone", "done", "burnt", "scarred",
    "trapped", "stuck", "blind", "mute", "deaf", "weak",
    "sleep", "cry", "bleed", "scream", "whisper", "silent",
    "quiet", "loud", "wild", "tame", "free", "chain", "rust",
    "dust", "ash", "smoke", "burn", "drown", "sink", "fall",
    "rise", "float", "drift", "break", "crash", "shatter",
    "melt", "freeze", "boil", "steam", "fade", "glow", "shine",
    "shadow", "ghost", "haunt", "curse", "bless", "pray", "wish",
    "hope", "fear", "hurt", "heal", "kill", "die", "live",
    "wake", "dream", "wonder", "wander", "leave", "stay", "run",
    "hide", "seek", "find", "lose", "win", "beg", "give", "take",
]


def gen_stretched():
    word = random.choice(STRETCH_WORDS)
    stretch_style = random.choice(["last", "vowel", "double"])
    if stretch_style == "last":
        last = word[-1]
        repeats = random.randint(4, 12)
        return word + last * repeats
    elif stretch_style == "vowel":
        # find the last vowel in the word and stretch it
        for i in range(len(word) - 1, -1, -1):
            if word[i] in "aeiou":
                repeats = random.randint(3, 8)
                return word[:i] + word[i] * (repeats + 1) + word[i+1:]
        last = word[-1]
        return word + last * random.randint(4, 12)
    else:
        # doubled ending pattern like "coldddd_"
        last = word[-1]
        repeats = random.randint(3, 9)
        suffix = random.choice(["", "", "_", "."])
        return word + last * repeats + suffix


# ── hashtag + phrase ──

HASHTAG_PHRASES = [
    "moveinsilence", "empty", "nofeelings", "detached", "overit",
    "dontcare", "numbtoitall", "nocap", "realones", "lonelyboy",
    "grindmode", "stackmode", "traplife", "noregrets", "coldworld",
    "nobodyknows", "staylow", "ghosted", "invisible", "forgotten",
    "untouchable", "unbothered", "unfazed", "unbroken", "unspoken",
    "damaged", "disposable", "replaceable", "expendable", "broken",
    "painistemporary", "trustnoone", "ridealone", "nochill",
    "solo", "byself", "alone", "lonewolf", "silent", "quiet",
    "loud", "wild", "free", "chained", "tired", "faded",
    "wavy", "drippy", "iced", "blessed", "cursed", "chosen",
    "outcast", "outsider", "unknown", "underground", "offgrid",
    "offline", "afk", "muted", "blocked", "banned", "canceled",
    "hated", "loved", "envied", "faked", "used", "played",
    "lost", "gone", "back", "here", "there", "nowhere",
    "everywhere", "somewhere", "anywhere", "always", "never",
    "forever", "yesterday", "tomorrow", "later", "soon",
    "stayreal", "keepgoing", "dontquit", "onegoal", "winonly",
    "prayup", "manifest", "focused", "lockedin", "dialedin",
]

HASHTAG_PREFIXES = ["hashtag", "tag", "the", "team", "gang", "no", "im", "we", "just"]


def gen_hashtag():
    return random.choice(HASHTAG_PREFIXES) + random.choice(HASHTAG_PHRASES)


# ── misspelled words ──

MISSPELL_PAIRS = [
    ("beautiful", "beautifyul"), ("remorse", "remorsis"),
    ("consistent", "concistent"), ("detaching", "detatchin"),
    ("embarrassment", "embarassments"), ("desperate", "desparate"),
    ("separate", "seprate"), ("disappear", "dissapear"),
    ("conscious", "concious"), ("occurrence", "occurence"),
    ("privilege", "privelege"), ("necessary", "neccessary"),
    ("acknowledge", "acknowlege"), ("existence", "existense"),
    ("independent", "independant"), ("persistence", "persistance"),
    ("maintenance", "maintenence"), ("consequence", "consequense"),
    ("intelligence", "intelligense"), ("interference", "interferense"),
    ("significance", "significanse"), ("irrelevant", "irrelevent"),
    ("temperament", "temperment"), ("environment", "enviroment"),
    ("achievement", "achievment"), ("management", "managment"),
    ("development", "developement"), ("measurement", "measurment"),
    ("abandonment", "abandonement"), ("replacement", "replacment"),
    ("fulfillment", "fullfillment"), ("involvement", "involvment"),
    ("commitment", "comitment"), ("agreement", "agreament"),
    ("statement", "statment"), ("appointment", "apointment"),
    ("adjustment", "adjustement"), ("attachment", "atachment"),
    ("disappoint", "dissapoint"), ("misunderstand", "misundrstand"),
    ("weird", "wierd"), ("receive", "recieve"), ("believe", "beleive"),
    ("achieve", "acheive"), ("relieve", "releive"), ("perceive", "percieve"),
    ("definitely", "definately"), ("separate", "seperate"),
    ("recommend", "recomend"), ("accommodate", "accomodate"),
    ("committed", "commited"), ("beginning", "begining"),
    ("occasionally", "ocasionally"), ("professional", "profesional"),
    ("occurred", "occured"), ("preferred", "prefered"),
    ("guarantee", "garantee"), ("apparent", "apparant"),
    ("calendar", "calender"), ("cemetery", "cemetary"),
    ("column", "colomn"), ("familiar", "familliar"),
    ("foreign", "foriegn"), ("grateful", "greatful"),
    ("harass", "harrass"), ("hierarchy", "heirarchy"),
    ("liaison", "liason"), ("license", "licence"),
    ("mischievous", "mischievious"), ("nuclear", "nucular"),
    ("original", "orginal"), ("possess", "posess"),
]


def gen_misspell():
    word = random.choice(MISSPELL_PAIRS)[1]
    # sometimes append a small tail for variety
    tail = random.random()
    if tail < 0.15:
        word += str(random.randint(0, 99))
    elif tail < 0.20:
        word += random.choice(["_", "."])
    return word


# ── underscore split ──

SPLIT_WORDS = [
    "guap", "funds", "racks", "bands", "stacks", "drip", "flex",
    "trap", "grind", "pain", "void", "cold", "dark", "numb",
    "rage", "fury", "doom", "fate", "lost", "gone", "dead",
    "blur", "haze", "dust", "rust", "burn", "torn", "scar",
    "bled", "woke", "fake", "real", "lone", "myth", "grim",
    "vibe", "mood", "soul", "mind", "core", "edge", "curve",
    "wave", "tide", "shore", "cliff", "peak", "storm", "calm",
    "faith", "hope", "trust", "love", "hate", "fear", "peace",
    "light", "shade", "flash", "spark", "flame", "smoke", "ash",
    "beat", "rhythm", "pulse", "flow", "stream", "river", "pond",
    "shine", "gloss", "glow", "glare", "sheen", "matte", "raw",
    "silk", "steel", "iron", "gold", "silver", "brass", "chrome",
]


def gen_split():
    word = random.choice(SPLIT_WORDS)
    pos = random.randint(1, len(word) - 1)
    sep = random.choice(["_", "_", "_", ".", "-"])
    return word[:pos] + sep + word[pos:]


# ── mood combos: word+mode/way/set ──

MOOD_FRONTS = [
    "wock", "trap", "cold", "dark", "grim", "ghost", "void",
    "numb", "pain", "rage", "grief", "dread", "bleak", "stark",
    "harsh", "raw", "grit", "scar", "burn", "rust", "dust",
    "night", "shade", "frost", "storm", "wraith", "silent",
    "bitter", "hollow", "vacant", "barren", "blunt", "rigid",
    "lush", "vivid", "dim", "faint", "bright", "sharp", "keen",
    "swift", "slow", "soft", "hard", "thick", "thin", "pure",
    "wild", "tame", "free", "bound", "loose", "tight", "open",
    "closed", "hidden", "seen", "known", "lost", "found",
    "iron", "steel", "gold", "silver", "brass", "copper", "clay",
    "smoke", "flame", "ember", "spark", "flash", "beam", "ray",
    "shark", "wolf", "raven", "hawk", "lion", "tiger", "bear",
    "moon", "sun", "star", "sky", "cloud", "mist", "rain",
    "lone", "silent", "still", "quiet", "loud", "clear", "murky",
]

MOOD_BACKS = [
    "mode", "way", "set", "funds", "archive", "heart", "mind",
    "gang", "club", "life", "world", "zone", "side", "lane",
    "path", "code", "wave", "state", "shift", "drift", "phase",
    "flow", "edge", "grind", "stack", "vault", "reign", "pulse",
    "cult", "crew", "wave", "era", "age", "time", "hour",
    "day", "night", "dawn", "dusk", "hour", "season",
    "kingdom", "empire", "realm", "domain", "space", "place",
    "aura", "essence", "spirit", "soul", "core", "root", "seed",
    "crown", "throne", "temple", "shrine", "altar", "vault",
    "bloom", "burst", "surge", "rush", "storm", "calm", "peak",
    "boy", "girl", "child", "kid", "one", "soul", "self",
    "shift", "swap", "trade", "deal", "flip", "spin", "turn",
]


def gen_mood():
    name = random.choice(MOOD_FRONTS) + random.choice(MOOD_BACKS)
    # sometimes tail with digits or a symbol for extra variety
    tail_style = random.random()
    if tail_style < 0.15:
        name += str(random.randint(0, 999))
    elif tail_style < 0.20:
        name += random.choice(["_", "."])
    return name


# ── -ing/-ed/-s twisted words ──

TWIST_BASES = [
    "fragment", "dismember", "disintegrat", "coordinat", "deplet",
    "process", "dissolv", "deteriorat", "eliminat", "evaporat",
    "decompos", "obliterat", "annihilat", "decapitat",
    "manipulat", "incapacitat", "disorient", "destabiliz",
    "disconnect", "decompress", "deconstruct", "disassembl",
    "overwhelm", "overpower", "underestimat", "misinterpret",
    "reconsid", "reorganiz", "reinvent", "reassembl", "recompos",
    "reevaluat", "regenerat", "reformulat", "reintegrat",
    "hypnotiz", "paralyz", "traumatiz", "energiz", "symboliz",
    "criticiz", "romanticiz", "materiliz", "visualiz", "verbaliz",
    "vandal", "torment", "torture", "punish", "banish", "vanish",
    "sabotag", "provoke", "disturb", "corrupt", "distort", "poison",
    "murder", "haunt", "scare", "terror", "shatter", "smother",
    "consume", "devour", "swallow", "choke", "drown", "burn",
    "slither", "linger", "wither", "shiver", "quiver", "whimper",
]

TWIST_SUFFIXES = ["ing", "ings", "ed", "eds", "in", "ins", "er", "ers", "or", "ors"]


def gen_twisted():
    return random.choice(TWIST_BASES) + random.choice(TWIST_SUFFIXES)


# ── alphanumeric patterns ──

def gen_alphanum():
    patterns = [
        lambda: f"{random.randint(0,9)}x{random.randint(0,9)}y{random.randint(0,9)}",
        lambda: f"{random.randint(0,9)}x{random.randint(0,9)}x{random.randint(0,9)}",
        lambda: f"0x" + ''.join(random.choice("0123456789abcdef") for _ in range(random.randint(4, 8))),
        lambda: ''.join(random.choice(LETTERS + DIGITS) for _ in range(random.randint(5, 8))),
    ]
    return random.choice(patterns)()


# ── stylized words: cigarette → cigareto ──

STYLE_WORDS = """
cigarette cassette roulette silhouette baguette palette omelette croquette vignette marionette gazette
rosette brunette corvette coquette etiquette pipette barrette statuette majorette serviette
velvet rocket pocket jacket planet magnet secret comet ticket bracelet scarlet violet helmet puppet
carpet basket bucket locket sonnet cabinet closet blanket trumpet garnet gadget budget target tablet
chocolate nicotine caffeine adrenaline gasoline limousine sunshine moonshine perfume champagne vampire
sapphire empire fortune lemonade marmalade cascade parade grenade arcade serenade masquerade silence
romance essence cologne migraine hurricane cyclone ozone nightmare paradise satellite dynamite marble
purple castle candle jungle puzzle bubble trouble ripple needle riddle smoke
diamond pistol poison venom crystal phantom mirror thunder silver ember cinder crimson saturn neon
dragon demon lotus chaos velour liquor nectar sugar caramel cinnamon tobacco bourbon absinthe
honey cherry whiskey brandy candy money misty ruby ivory mercury galaxy fury envy frenzy jelly
vanilla tequila karma plasma cobra aurora nova vodka salsa cola guava papaya banana lava
espresso casino inferno tornado volcano piano mango avocado mosquito flamingo domino tuxedo studio
radio cameo rodeo lotto
""".split()

STYLE_ENDINGS = [
    # (word ending, letters to drop, new endings)
    ("ette", 4, ["eto", "eta", "ito", "ita", "ete", "ino", "ina"]),
    ("et", 0, ["o", "a", "ito", "ina", "ino"]),
    ("e", 1, ["o", "a", "i", "ito", "ina"]),
    ("y", 1, ["o", "i", "ita", "ino"]),
    ("a", 1, ["o", "i", "ita", "ina"]),
    ("o", 1, ["a", "i", "ito", "ino"]),
    ("", 0, ["o", "a", "ito", "ina"]),
]


def _stylize(word):
    for end, drop, endings in STYLE_ENDINGS:
        if word.endswith(end):
            stem = word[:len(word) - drop]
            out = []
            for new in endings:
                if stem[-1] == new[0]:
                    new = new[1:]  # radio → radito, not radiito
                if len(stem + new) >= 5 and stem + new != word:
                    out.append(stem + new)
            return out
    return []


STYLED = sorted({name for word in STYLE_WORDS for name in _stylize(word)})


def gen_stylized():
    return random.choice(STYLED)


# ═══════════════════════════════════════════════════════
#  FORMAT REGISTRY
# ═══════════════════════════════════════════════════════

BOTH = ("discord", "roblox")


def _fmt(fid, group, code, example, desc, odds, gen, picks=(), platforms=("discord",), roblox_odds=None):
    return {"id": fid, "group": group, "code": code, "example": example, "desc": desc,
            "odds": {"discord": odds, "roblox": roblox_odds}, "gen": gen, "picks": list(picks),
            "platforms": list(platforms)}


def roblox_ok(name):
    # Roblox: 3-20 characters, a-z 0-9 _, at most one _, never first or last
    return (3 <= len(name) <= 20 and re.fullmatch(r"[a-z0-9_]+", name) is not None
            and name[0] != "_" and name[-1] != "_" and name.count("_") <= 1)


VALID_FOR = {"discord": None, "roblox": roblox_ok}


def gen_semi4_roblox():
    base = "".join(random.choice(ALNUM) for _ in range(4))
    i = random.randint(1, 3)
    return base[:i] + "_" + base[i:]


# odds = % free when 50-100 of each were checked against Discord (Oct 2026);
# roblox_odds = % free in 40-name samples against Roblox (Oct 2026).
# None = not measured on its own; Discord's word formats ran ~55% free as a group.
FORMATS = [
    # "semi" = a 3-character name plus one _ or . anywhere (mfm_, m8m., 9h_4, .ka1)
    _fmt("semi3l", "Every 4-character name", "Semi 3L", "mfm_", "3 letters/digits + one _ or . anywhere (mfm_, m8m., o8o., 9h_4)", 0,
         Sweep([ALNUM + "_."] * 4, r"(?=.{4}$)(?=.*[a-z])[a-z0-9]*[_.][a-z0-9]*",
               size=(36 ** 3 - 10 ** 3) * 4 * 2, keep=lambda n: _one_symbol(n) and any(c in LETTERS for c in n)),
         ("every4",)),
    _fmt("semi3n", "Every 4-character name", "Semi 3N", "093_", "3 digits + one _ or . anywhere (093_, 09.3, _093)", 0,
         Sweep([DIGITS + "_."] * 4, r"(?=.{4}$)\d*[_.]\d*", size=10 ** 3 * 4 * 2, keep=lambda n: _one_symbol(n)),
         ("every4",)),
    _fmt("rsemi3", "Every 4-character name", "Semi 3", "ab_c", "3 letters/digits + one _ in the middle (ab_c, 9h_4, m_8m)", 0,
         Sweep([ALNUM, ALNUM + "_", ALNUM + "_", ALNUM], r"[a-z0-9](?:[a-z0-9]_|_[a-z0-9])[a-z0-9]",
               size=2 * 36 ** 3, keep=lambda n: n.count("_") == 1), ("every4",), platforms=("roblox",), roblox_odds=0),
    _fmt("word4", "Every 4-character name", "4L word", "wbot", "a letter or digit + short word (wbot, xgod, 1pro)", 0,
         Sweep([ALNUM, SHORT_WORDS], "[a-z0-9](?:" + "|".join(SHORT_WORDS) + ")"), ("every4",), platforms=BOTH, roblox_odds=0),
    _fmt("all_dot", "Every 4-character name", ".10a", ".10a", "every . + 10–99 + letter (.10a … .99z)", 0,
         Sweep(["123456789", DIGITS, LETTERS], r"\.[1-9]\d[a-z]", prefix="."), ("every4",)),
    _fmt("all_4n", "Every 4-character name", "4N", "2190", "every 4-digit number (0000 … 9999)", 0,
         Sweep([DIGITS] * 4, r"\d{4}"), ("every4",), platforms=BOTH, roblox_odds=0),
    _fmt("all_4l", "Every 4-character name", "4L", "zuxk", "every 4-letter name (aaaa … zzzz)", 0,
         Sweep([LETTERS] * 4, r"[a-z]{4}"), ("every4",), platforms=BOTH, roblox_odds=0),
    _fmt("all_4c", "Every 4-character name", "4C", "cm3a", "every 4-character mix of letters and digits", 0,
         Sweep([ALNUM] * 4, r"(?=.*[a-z])(?=.*\d)[a-z\d]{4}",
               size=36 ** 4 - 26 ** 4 - 10 ** 4, keep=_letter_and_digit), ("every4",), platforms=BOTH, roblox_odds=0),

    _fmt("4c_sym", "Semi 4L / 4C", "Semi 4C", "t_n4y", "4 letters or digits + one _ or .", 84, gen_4c_sym, ("best", "semi4")),
    _fmt("4l_sym", "Semi 4L / 4C", "Semi 4L", "kt_io", "4 letters + one _ or .", 56, gen_4l_sym, ("best", "semi4")),
    _fmt("rsemi4", "Semi 4L / 4C", "Semi 4", "ab_cd", "4 letters/digits + one _ in the middle (ab_cd, a_bcd, 4k_9x)", None,
         gen_semi4_roblox, ("semi4",), platforms=("roblox",), roblox_odds=32),

    _fmt("4n_mid", "Semi 4N", "Semi 4N", "21.90", "4 digits with a _ or . in the middle", 55, gen_4n_mid, ("best", "semi4", "numbers")),
    _fmt("4n_start", "Semi 4N", "Semi 4N start", "_2190", "4 digits that start with _ or .", 4, gen_4n_start, ("semi4", "numbers")),
    _fmt("4n_end", "Semi 4N", "Semi 4N end", "4316_", "4 digits that end with _ or .", 1, gen_4n_end, ("semi4", "numbers")),

    _fmt("5c", "5 characters", "5C", "guj31", "5 letters and digits", 82, gen_5c, ("best",), platforms=BOTH, roblox_odds=26),
    _fmt("5l", "5 characters", "5L", "hnupo", "5 letters", 68, gen_5l, ("best",), platforms=BOTH, roblox_odds=0),
    _fmt("5p", "5 characters", "5P", "lajub", "5 letters you can say out loud", 12, gen_5p, platforms=BOTH, roblox_odds=0),
    _fmt("5n", "5 characters", "5N", "21904", "5 digits", 0, gen_5n, platforms=BOTH, roblox_odds=0),

    _fmt("3m", "3 characters", "3M", "m8m", "mirror, no symbol (6w6, k3k)", 0, gen_3m, platforms=BOTH, roblox_odds=0),
    _fmt("3s", "3 characters", "3C", "ka1", "3 letters and digits mixed, no symbol", 0, gen_3s, platforms=BOTH, roblox_odds=0),
    _fmt("semi3m", "3 characters", "Semi 3M", "m8m.", "mirror + . or _ at the end (m8m., 6w6_)", 0,
         Sweep([ALNUM] * 3 + ["._"], r"(?:([a-z])\d\1|(\d)[a-z]\2)[._]", size=2 * 26 * 10 * 2,
               keep=lambda n: n[0] == n[2] and (n[0] in DIGITS) != (n[1] in DIGITS)), ("every4",)),
    _fmt("semi3c", "3 characters", "Semi 3C", "ka1.", "3 letters/digits mixed + . or _ at the end (ka1., 9xx_)", 0,
         Sweep([ALNUM] * 3 + ["._"], r"(?=.*[a-z])(?=.*\d)[a-z0-9]{3}[._]",
               size=(36 ** 3 - 26 ** 3 - 10 ** 3) * 2, keep=_letter_and_digit), ("every4",)),

    _fmt("xny", "Words", "XnY", "paidnbroken", "word + n + word", None, gen_xny, ("words",), platforms=BOTH, roblox_odds=97),
    _fmt("compound", "Words", "Compound", "stuckinloops", "two-part phrase", None, gen_compound, ("words",),
         platforms=BOTH, roblox_odds=89),
    _fmt("stretched", "Words", "Stretched", "numbbbbbb", "word with a stretched letter", None, gen_stretched, ("words",),
         platforms=BOTH, roblox_odds=8),
    _fmt("hashtag", "Words", "Hashtag", "hashtagempty", "tag / no / im + phrase", None, gen_hashtag, ("words",),
         platforms=BOTH, roblox_odds=40),
    _fmt("misspell", "Words", "Misspell", "beautifyul", "misspelled word", None, gen_misspell, ("words",),
         platforms=BOTH, roblox_odds=25),
    _fmt("split", "Words", "Split", "g_uap", "word split by _ . or -", None, gen_split, ("words",), platforms=BOTH, roblox_odds=0),
    _fmt("mood", "Words", "Mood", "wockmode", "word + mode / way / set…", None, gen_mood, ("words",), platforms=BOTH, roblox_odds=32),
    _fmt("twisted", "Words", "Twisted", "fragmenteds", "word with a twisted -ing / -ed ending", None, gen_twisted, ("words",),
         platforms=BOTH, roblox_odds=30),
    _fmt("alphanum", "Words", "AlphaNum", "1x1y1", "patterns like 1x1y1 or 0x hex", None, gen_alphanum, ("words",),
         platforms=BOTH, roblox_odds=54),
    _fmt("stylized", "Words", "Stylized", "cigareto", "a word with a stylized ending (cigareto, chocolato, velveto)", None,
         gen_stylized, ("words",), platforms=BOTH, roblox_odds=5),
]
FORMAT_BY_ID = {f["id"]: f for f in FORMATS}


def stream(gens, exclude=frozenset(), origin=None, valid=None):
    """Supply of fresh names from {format_id: generator or Sweep}.

    Random formats stop once they run dry (e.g. 3M has only 520 combos).
    Sweeps run smallest first and end after every combination is covered.
    `valid` drops names a platform doesn't allow (Roblox: no ".", one "_").
    Records which format made each name in `origin` when given."""
    seen = set()
    randoms = [(fid, g) for fid, g in gens.items() if not isinstance(g, Sweep)]
    sweeps = [(fid, g.names()) for fid, g in sorted(
        ((fid, g) for fid, g in gens.items() if isinstance(g, Sweep)), key=lambda x: x[1].size)]
    misses = 0
    while randoms or sweeps:
        if sweeps and (not randoms or random.random() < 1 / (len(randoms) + 1)):
            fid, it = sweeps[0]
            name = next(it, None)
            if name is None:
                sweeps.pop(0)
                continue
            if name in exclude or (valid and not valid(name)):
                continue  # checked in an earlier run, or not allowed here
        else:
            fid, fn = random.choice(randoms)
            name = fn()
            if (name in seen or name in exclude or not 2 <= len(name) <= 32
                    or (valid and not valid(name))):
                misses += 1
                if misses >= 20000:
                    randoms = []
                continue
            seen.add(name)
            misses = 0
        if origin is not None:
            origin[name] = fid
        yield name


def generate(count, gens, exclude=frozenset(), origin=None, valid=None):
    return list(itertools.islice(stream(gens, exclude, origin, valid), count))
