# How to use: python3 build.py <folder with fixes.py> <repo> <output folder>
# Makes the DataFixes SQL for "Песнь Возрождения 3300" from the lists in fixes.py and a review table.
import sqlite3, re, sys, shutil, html
sys.path.insert(0, sys.argv[1]); from fixes import *
repo, outdir = sys.argv[2], sys.argv[3]
SB = "(SELECT id FROM Songbooks WHERE name = 'Песнь Возрождения 3300')"
db = sqlite3.connect('file:' + repo + '/spData.sqlite?mode=ro', uri=True)
songs = {}
for n, t in db.execute("select number, song_text from Songs where songbook_id=1"):
    songs.setdefault(n, []).append(t)

def lit(s):
    # SQL text literal; line breaks as char(10) so a statement never spans lines
    parts = s.split('\n')
    return ' || char(10) || '.join("'" + p.replace("'", "''") + "'" for p in parts)

def stmt(n, old, new):
    return (f"UPDATE Songs SET song_text = replace(song_text, {lit(old)}, {lit(new)}) "
            f"WHERE songbook_id = {SB} AND number = {n} AND instr(song_text, {lit(old)}) > 0;")

problems = []
def check(n, old):
    hits = sum(t.count(old) for t in songs.get(n, []))
    if hits == 0: problems.append(f'{n}: not found: {old!r}')
    return hits

# Item 1: double spaces become line fixes
double = []
for n in DOUBLE_SPACE_SONGS:
    for t in songs[n]:
        for line in t.split('\n'):
            fixed = re.sub(r'(?<=\S) {2,}(?=\S)', ' ', line)
            if fixed != line: double.append((n, line, fixed))
auto = AUTO_LABELS + AUTO_DIGITS + AUTO_SPLITS + AUTO_PUNCT + double
for n, old, new in auto + TYPOS: check(n, old)

head1 = """-- Mechanical fixes in the songbook "Песнь Возрождения 3300" (2026-10-25):
-- the digit 3 typed in place of the letter З (and back), Latin letters that look like Russian ones
-- in hymn 627 (so search finds it), words torn by a space or by a hyphen left from the printed book,
-- spaces around punctuation, double spaces, labels "Куплет1" and "Привпев".
-- Each fix replaces a piece of one hymn and is skipped when that piece was changed since.
"""
head2 = """-- Typos in the songbook "Песнь Возрождения 3300", checked by the church media team (2026-10-25):
-- misspelt words, letters misread when the book was scanned ("н" for "и" or "в"), names of God
-- written with a small letter. Each fix is skipped when that piece of the hymn was changed since.
"""
with open(outdir + '/2026-10-25-pv3300-mechanical.sql', 'w') as f:
    f.write(head1)
    for n, old, new in auto: f.write(stmt(n, old, new) + '\n')
    latin = 'song_text'
    for a, b in LATIN_MAP.items(): latin = f"replace({latin}, '{a}', '{b}')"
    f.write(f"UPDATE Songs SET song_text = {latin} WHERE songbook_id = {SB} AND number = {LATIN_SONG} "
            f"AND song_text GLOB '*[A-Za-z]*';\n")
with open(outdir + '/2026-10-25-pv3300-typos.sql', 'w') as f:
    f.write(head2)
    for n, old, new in TYPOS: f.write(stmt(n, old, new) + '\n')

# Review table for item 2
def ctx(n, old):
    for t in songs[n]:
        if old in t:
            i = t.index(old); s = t.rfind('\n', 0, i) + 1; e = t.find('\n', i + len(old)); e = len(t) if e < 0 else e
            return t[s:e].strip()
    return ''
with open(outdir + '/typos-review.md', 'w') as f:
    f.write('# Опечатки в «Песни Возрождения 3300»: на проверку\n\n')
    f.write(f'{len(TYPOS)} исправлений. Изменённые слова выделены жирным. Если какое-то исправление неверно, '
            'напишите номер гимна, и оно не войдёт в обновление.\n\n')
    f.write('| № | Было | Станет |\n|---|---|---|\n')
    for n, old, new in TYPOS:
        line = ctx(n, old).replace('\n', ' / ')
        o, w = old.strip('\n').replace('\n', ' / '), new.strip('\n').replace('\n', ' / ')
        f.write(f'| {n} | {line.replace(o, "**" + o + "**")} | {line.replace(o, "**" + w + "**")} |\n')
print('mechanical:', len(auto) + 1, 'typos:', len(TYPOS), 'double-space lines:', len(double))
print('\n'.join(problems) or 'all pieces found')
