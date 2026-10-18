import random
import string
import argparse

# ── word+name combos (like "fuckalex", "stupidmike") ──

PREFIXES = [
    "fuck", "hate", "kill", "slap", "drop", "kick", "smack", "dumb",
    "stupid", "ugly", "fat", "sad", "mad", "bad", "big", "lil",
    "not", "fake", "real", "only", "just", "its", "im", "ur",
    "the", "that", "this", "king", "lord", "sir", "god", "don",
    "pro", "noob", "bot", "old", "new", "dark", "cold", "lost",
    "dead", "sick", "raw", "low", "top", "best", "last", "next",
]

NAMES = [
    "cats", "dogs", "fire", "ice", "rain", "snow", "wolf", "bear",
    "hawk", "crow", "fish", "frog", "dust", "wind", "storm", "rock",
    "moon", "star", "sun", "sky", "void", "dark", "light", "dawn",
    "dusk", "haze", "fog", "mist", "sand", "dirt", "clay", "rust",
    "gold", "iron", "acid", "salt", "bone", "blood", "rage", "fury",
    "doom", "pain", "fear", "hope", "luck", "fate", "chaos", "peace",
    "war", "riot", "myth", "lore", "hex", "vibe", "edge", "blade",
    "fist", "grip", "hook", "trap", "maze", "gate", "wall", "tower",
    "code", "data", "ping", "lag", "spam", "troll", "grief", "sweat",
    "drip", "flex", "grind", "cash", "coin", "loot", "gems", "crown",
]

SUFFIXES = [
    "", "", "", "", "", "",  # bias toward no suffix
    "x", "o", "z", "69", "420", "99", "1", "0",
    "xd", "lol", "fr", "rn", "gg", "tv", "yt",
]


def gen_wordname():
    prefix = random.choice(PREFIXES)
    name = random.choice(NAMES)
    suffix = random.choice(SUFFIXES)
    return prefix + name + suffix


# ── keyboard mash / repeated patterns (like "zxczxczxc", "asdasdasd") ──

CHUNKS = [
    "zxc", "asd", "qwe", "wer", "sdf", "xcv", "dfg", "fgh",
    "cvb", "bnm", "rty", "tyu", "uio", "ghj", "hjk", "jkl",
    "poi", "lkj", "mnb", "vcx", "dsa", "ewq", "ytr", "iop",
    "was", "die", "sad", "mad", "lol", "brr", "grr", "hmm",
    "zzz", "xxx", "aaa", "eee", "oof", "uwu", "owo", "xox",
]


def gen_mash():
    chunk = random.choice(CHUNKS)
    repeats = random.randint(3, 6)
    result = chunk * repeats
    if random.random() < 0.3:
        result = result[:random.randint(8, len(result))]
    return result


# ── random long lowercase (like "jfkdlsajfkdsa") ──

def gen_random_long():
    length = random.randint(8, 18)
    return ''.join(random.choice(string.ascii_lowercase) for _ in range(length))


# ── combined word spam (like "ihatethisgame", "getoutofhere") ──

WORDS_A = [
    "i", "u", "we", "they", "he", "she", "who", "why", "how",
    "get", "got", "let", "dont", "cant", "wont", "stop", "go",
    "come", "take", "make", "give", "put", "run", "try", "eat",
]

WORDS_B = [
    "out", "up", "off", "away", "here", "there", "now", "back",
    "hate", "love", "want", "need", "like", "miss", "know", "see",
    "this", "that", "the", "my", "your", "his", "her", "our",
]

WORDS_C = [
    "game", "life", "world", "name", "face", "shit", "crap", "mess",
    "kid", "dog", "cat", "bot", "noob", "bro", "dude", "man",
    "thing", "stuff", "place", "time", "day", "way", "boy", "girl",
]


def gen_phrase():
    parts = [random.choice(WORDS_A), random.choice(WORDS_B)]
    if random.random() < 0.7:
        parts.append(random.choice(WORDS_C))
    return ''.join(parts)


GENERATORS = {
    "wordname": gen_wordname,
    "mash": gen_mash,
    "random": gen_random_long,
    "phrase": gen_phrase,
}


def generate_batch(count, formats=None):
    if formats is None:
        formats = list(GENERATORS.keys())
    seen = set()
    results = []
    gen_funcs = [GENERATORS[f] for f in formats]
    while len(results) < count:
        name = random.choice(gen_funcs)()
        if name not in seen and 2 <= len(name) <= 32:
            seen.add(name)
            results.append(name)
    return results


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Long/mash Discord username generator")
    parser.add_argument("-n", "--count", type=int, default=200,
                        help="number of usernames (default: 200)")
    parser.add_argument("-f", "--formats", nargs="+",
                        choices=list(GENERATORS.keys()), default=None,
                        help="formats: wordname mash random phrase (default: all)")
    parser.add_argument("-o", "--output", default="users_long.txt",
                        help="output file (default: users_long.txt)")
    args = parser.parse_args()

    usernames = generate_batch(args.count, args.formats)

    with open(args.output, "w") as f:
        for name in usernames:
            f.write(name + "\n")

    print(f"Generated {len(usernames)} usernames -> {args.output}")
    for fmt in (args.formats or list(GENERATORS.keys())):
        label = {"wordname": "Word+Name", "mash": "Keyboard Mash",
                 "random": "Random Long", "phrase": "Word Phrase"}[fmt]
        sample = [u for u in usernames if True][:3]
        print(f"  {label}: included")
