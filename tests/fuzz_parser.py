#!/usr/bin/env python3
"""Parser fuzzer for EPL.

LLMs write wrong code; the parser has to reject it cleanly. This mutates every
EPL program in the repo (and generates random keyword soup), runs
`epl --check` on the result, and reports any input that makes the parser
crash, hang, or run out of memory. A clean rejection (exit 1) is fine.

`--check` only parses, so fuzzed programs never execute: no `run`, no file
writes, no sockets.

    tests/fuzz_parser.py                       # 2000 iterations, random seed
    tests/fuzz_parser.py --iterations 20000 --seed 7
    tests/fuzz_parser.py --replay case.EP      # re-check one saved input
    EPL_BIN=/path/to/epl tests/fuzz_parser.py  # fuzz a different binary

Failures are minimised (line by line, then word by word) and saved to --out.
Exit status is 1 if any failure was found.
"""

import argparse
import os
import random
import re
import resource
import subprocess
import sys
import tempfile
import time
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
EPL = Path(os.environ.get("EPL_BIN", ROOT / "build" / "epl"))
MEM_LIMIT = 512 * 1024 * 1024
TIMEOUT = 3.0

def load_corpus():
    files = []
    for sub in ("build", "examples", "tests/cases"):
        for p in sorted((ROOT / sub).rglob("*")):
            if p.suffix.lower() in (".ep", ".epf") and p.is_file():
                files.append(p.read_text(errors="replace"))
    return files

def load_vocabulary():
    """Every keyword and phrase the lexer knows, plus punctuation and literals."""
    src = (ROOT / "build" / "lexer.c").read_text(errors="replace")
    words = set(re.findall(r'\{\s*"([^"\\]+)"\s*,\s*TOKEN_', src))
    words |= {"!", ",", "[", "]", "(", ")", "+", "-", "*", "/", ":", ".",
              "0", "1", "-3", "2.5", '"text"', '""', "x", "items", "#", "###"}
    return sorted(words)

def mutate(rng, text, corpus, vocab):
    lines = text.split("\n")
    for _ in range(rng.randint(1, 4)):
        if not lines:
            lines = [""]
        op = rng.randrange(9)
        i = rng.randrange(len(lines))
        words = lines[i].split(" ")
        if op == 0:
            if words:
                del words[rng.randrange(len(words))]
            lines[i] = " ".join(words)
        elif op == 1:
            words.insert(rng.randint(0, len(words)), rng.choice(vocab))
            lines[i] = " ".join(words)
        elif op == 2:
            if words:
                words[rng.randrange(len(words))] = rng.choice(vocab)
            lines[i] = " ".join(words)
        elif op == 3:
            del lines[i]
        elif op == 4:
            lines.insert(i, lines[i])
        elif op == 5:
            j = rng.randrange(len(lines))
            lines[i], lines[j] = lines[j], lines[i]
        elif op == 6:
            lines = lines[:i]
        elif op == 7:
            other = rng.choice(corpus).split("\n")
            lines.insert(i, rng.choice(other))
        else:
            cut = rng.randint(0, len(lines[i]))
            lines[i] = lines[i][:cut]
    return "\n".join(lines)

def keyword_soup(rng, vocab):
    out = []
    for _ in range(rng.randint(1, 12)):
        out.append(" ".join(rng.choice(vocab) for _ in range(rng.randint(1, 8))))
    return "\n".join(out) + "\n"

def limit_memory():
    resource.setrlimit(resource.RLIMIT_AS, (MEM_LIMIT, MEM_LIMIT))

def check(source, workdir):
    """Return None if the parser behaved, else a short reason string."""
    fd, path = tempfile.mkstemp(suffix=".EP", dir=workdir)
    with os.fdopen(fd, "w") as f:
        f.write(source)
    try:
        for attempt in range(3):
            try:
                r = subprocess.run([str(EPL), "--check", path], capture_output=True,
                                   timeout=TIMEOUT, preexec_fn=limit_memory)
                break
            except PermissionError:
                if attempt == 2:
                    raise
                time.sleep(1)
    except subprocess.TimeoutExpired:
        return "hang (timeout)"
    finally:
        os.unlink(path)
    err = r.stderr.decode(errors="replace")
    if r.returncode < 0:
        return f"crash (signal {-r.returncode})"
    if "out of memory" in err:
        return "out of memory"
    if r.returncode not in (0, 1):
        return f"unexpected exit {r.returncode}"
    return None

def minimise(source, reason, workdir):
    """Drop lines, then words, while the same failure still reproduces."""
    def still(s):
        return check(s, workdir) == reason

    for sep in ("\n", " "):
        parts = source.split(sep)
        i = 0
        while i < len(parts):
            trial = parts[:i] + parts[i + 1:]
            if trial and still(sep.join(trial)):
                parts = trial
            else:
                i += 1
        source = sep.join(parts)
    return source

def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--iterations", type=int, default=2000)
    ap.add_argument("--seed", type=int, default=None)
    ap.add_argument("--jobs", type=int, default=os.cpu_count() or 2)
    ap.add_argument("--out", default=None,
                    help="directory for minimised failures (default: a temp dir)")
    ap.add_argument("--replay", help="check a single saved input and exit")
    args = ap.parse_args()

    if not EPL.exists():
        sys.exit(f"error: {EPL} not found — run 'make' in build/ first")

    workdir = tempfile.mkdtemp(prefix="epl-fuzz-")

    if args.replay:
        reason = check(Path(args.replay).read_text(errors="replace"), workdir)
        print(reason or "ok")
        sys.exit(1 if reason else 0)

    seed = args.seed if args.seed is not None else random.randrange(1 << 30)
    rng = random.Random(seed)
    corpus = load_corpus()
    vocab = load_vocabulary()
    print(f"fuzz: {len(corpus)} corpus files, {len(vocab)} words, "
          f"{args.iterations} iterations, seed {seed}")

    inputs = []
    for n in range(args.iterations):
        if n % 5 == 4:
            inputs.append(keyword_soup(rng, vocab))
        else:
            inputs.append(mutate(rng, rng.choice(corpus), corpus, vocab))

    with ThreadPoolExecutor(max_workers=args.jobs) as pool:
        results = list(pool.map(lambda s: check(s, workdir), inputs))

    failures = {}
    for src, reason in zip(inputs, results):
        if reason and reason not in failures:
            failures[reason] = src

    if not failures:
        print(f"fuzz: ok — {args.iterations} inputs, no crashes, hangs or OOM")
        return

    out = Path(args.out or tempfile.mkdtemp(prefix="epl-fuzz-failures-"))
    out.mkdir(parents=True, exist_ok=True)
    bad = sum(1 for r in results if r)
    print(f"fuzz: {bad} failing inputs, {len(failures)} distinct kinds")
    for k, (reason, src) in enumerate(failures.items()):
        small = minimise(src, reason, workdir)
        path = out / f"failure_{k}.EP"
        path.write_text(small)
        print(f"\n── {reason}  →  {path}")
        print("   " + small.replace("\n", "\n   "))
    sys.exit(1)

if __name__ == "__main__":
    main()
