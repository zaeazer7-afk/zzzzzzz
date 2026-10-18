import random
import string
import argparse

LETTERS = string.ascii_lowercase
DIGITS = string.digits
CONSONANTS = "bcdfghjklmnpqrstvwxz"
VOWELS = "aeiou"


def gen_4l():
    # 4 letters, phonetic consonant/vowel mix for cleaner combos
    pattern = random.choice([
        lambda: random.choice(CONSONANTS) + random.choice(VOWELS) + random.choice(CONSONANTS) + random.choice(CONSONANTS),
        lambda: random.choice(CONSONANTS) + random.choice(CONSONANTS) + random.choice(VOWELS) + random.choice(CONSONANTS),
        lambda: random.choice(CONSONANTS) + random.choice(CONSONANTS) + random.choice(CONSONANTS) + random.choice(VOWELS),
        lambda: ''.join(random.choice(CONSONANTS) for _ in range(4)),
    ])
    return pattern()


def gen_4c():
    # 4-char alpha-numeric mix, must have at least 1 letter and 1 digit
    while True:
        result = ''.join(random.choice(LETTERS + DIGITS) for _ in range(4))
        has_letter = any(c in LETTERS for c in result)
        has_digit = any(c in DIGITS for c in result)
        if has_letter and has_digit:
            return result


def gen_3c_mirror():
    # symmetrical: NLN (6w6) or LNL (z9z)
    if random.random() < 0.5:
        n = random.choice(DIGITS)
        return n + random.choice(LETTERS) + n
    else:
        l = random.choice(LETTERS)
        return l + random.choice(DIGITS) + l


def gen_3c_semi():
    # mixed 3-char: LNL, NLN, LLN, NLL, LNN, NNL
    patterns = [
        lambda: random.choice(LETTERS) + random.choice(DIGITS) + random.choice(LETTERS),
        lambda: random.choice(DIGITS) + random.choice(LETTERS) + random.choice(DIGITS),
        lambda: random.choice(LETTERS) + random.choice(LETTERS) + random.choice(DIGITS),
        lambda: random.choice(DIGITS) + random.choice(LETTERS) + random.choice(LETTERS),
        lambda: random.choice(LETTERS) + random.choice(DIGITS) + random.choice(DIGITS),
        lambda: random.choice(DIGITS) + random.choice(DIGITS) + random.choice(LETTERS),
    ]
    return random.choice(patterns)()


def gen_3c_deco():
    # rare-feel: a 3-char mirror/semi name with a . or _ inserted at a
    # random position — start, between any two chars, or end
    base = gen_3c_mirror() if random.random() < 0.5 else gen_3c_semi()
    deco = random.choice([".", "_"])
    pos = random.randint(0, len(base))  # 0..3 inclusive
    return base[:pos] + deco + base[pos:]


GENERATORS = {
    "4l": gen_4l,
    "4c": gen_4c,
    "3m": gen_3c_mirror,
    "3s": gen_3c_semi,
    "3d": gen_3c_deco,
}


def generate_batch(count, formats=None):
    if formats is None:
        formats = list(GENERATORS.keys())
    seen = set()
    results = []
    gen_funcs = [GENERATORS[f] for f in formats]
    while len(results) < count:
        name = random.choice(gen_funcs)()
        if name not in seen:
            seen.add(name)
            results.append(name)
    return results


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Discord username generator")
    parser.add_argument("-n", "--count", type=int, default=200, help="number of usernames to generate (default: 200)")
    parser.add_argument("-f", "--formats", nargs="+", choices=list(GENERATORS.keys()), default=None,
                        help="formats to use: 4l 4c 3m 3s 3d (default: all)")
    parser.add_argument("-o", "--output", default="users.txt", help="output file (default: users.txt)")
    args = parser.parse_args()

    usernames = generate_batch(args.count, args.formats)

    with open(args.output, "w") as f:
        for name in usernames:
            f.write(name + "\n")

    print(f"Generated {len(usernames)} usernames -> {args.output}")
    labels = {
        "4l": "4-Letter", "4c": "4-Char Mixed", "3m": "3-Char Mirror",
        "3s": "3-Char Semi", "3d": "3-Char + . or _",
    }
    for fmt in (args.formats or list(GENERATORS.keys())):
        print(f"  {labels[fmt]}: included")
