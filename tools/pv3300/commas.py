# How to use: python3 commas.py <repo> <output folder>
# Makes the DataFix for spaces around commas in all songbooks: exactly one space after a comma.
# Works on the songs as they are after the earlier DataFixes, so it has to be applied after them.
import glob, os, re, shutil, sqlite3, sys, tempfile

repo, outdir = sys.argv[1], sys.argv[2]
NAME = '2026-10-25-songs-commas.sql'


def apply_fixes(db, paths):
    # Like applyDataFixes() in source/main.cpp
    db.isolation_level = None
    for path in paths:
        lines = [l for l in open(path, encoding='utf-8').read().split('\n') if not l.strip().startswith('--')]
        db.execute('BEGIN')
        for s in re.split(r';\s*\n', '\n'.join(lines)):
            if s.strip():
                db.execute(s)
        db.execute('COMMIT')


# A stray comma, not a missing space: removed instead
SPECIAL = {'предаться , воле': 'предаться воле'}


def fix_line(line):
    keep = line
    for old, new in SPECIAL.items():
        line = line.replace(old, new)
    line = re.sub(r'(\S)[ \t ]+,', r'\1,', line)          # no space before a comma
    line = re.sub(r',,+', ',', line)                          # "идти,,"
    line = re.sub(r',\.(\s*)$', r'.\1', line)                 # "взирал,." at the end
    line = re.sub(r',\.\s+', ', ', line)                      # "Святой,. небесный"
    line = re.sub(r'\(2 р,\)', '(2 р.)', line)
    line = re.sub(r',-(?=[^\s-])', ', - ', line)              # ",- word" with no space after the dash
    line = re.sub(r',-', ', -', line)                         # ",-" -> ", -"
    line = re.sub(r',[ \t ]{2,}(?=\S)', ', ', line)      # more than one space
    line = re.sub(r',\t(?=\S)', ', ', line)
    # Any other comma right before a letter, a quote or a bracket; not ":,:" (repeat) or "1,5"
    line = re.sub(r'(?<!:),(?![:\d])(?=[^\s,.!?;:)\]»"\'])', ', ', line)
    return line if line != keep else keep


def lit(s):
    return ' || char(10) || '.join("'" + p.replace("'", "''") + "'" for p in s.split('\n'))


tmp = tempfile.mkdtemp()
db_path = os.path.join(tmp, 'spData.sqlite')
shutil.copy(os.path.join(repo, 'spData.sqlite'), db_path)
db = sqlite3.connect(db_path)
earlier = sorted(p for p in glob.glob(os.path.join(repo, 'DataFixes', '*.sql')) if os.path.basename(p) < NAME)
apply_fixes(db, earlier)

books = dict(db.execute('SELECT id, name FROM Songbooks'))
fixes, expected = [], {}
for sid, sb, num, title, text in db.execute('SELECT id, songbook_id, number, title, song_text FROM Songs'):
    new_lines = [fix_line(l) for l in (text or '').split('\n')]
    new_text = '\n'.join(new_lines)
    new_title = fix_line(title or '')
    if new_text == text and new_title == (title or ''):
        continue
    expected[sid] = (new_text, new_title)
    for old, new in sorted({(o, n) for o, n in zip(text.split('\n'), new_lines) if o != n}, key=lambda p: -len(p[0])):
        fixes.append((books[sb], num, 'song_text', old, new))
    if new_title != (title or ''):
        fixes.append((books[sb], num, 'title', title, new_title))

with open(os.path.join(outdir, NAME), 'w', encoding='utf-8') as f:
    f.write('-- Spaces around commas in all songbooks (2026-10-25): exactly one space after a comma ("ручья,-" ->\n'
            '-- "ручья, -", "себя,(как" -> "себя, (как"), none before it; "идти,," and ",." fixed.\n'
            '-- Repeat signs ":,:" and numbers like "1,5" stay. Each fix replaces one line of one song and is\n'
            '-- skipped when that line was changed since.\n')
    for book, num, field, old, new in fixes:
        f.write(f"UPDATE Songs SET {field} = replace({field}, {lit(old)}, {lit(new)}) "
                f"WHERE songbook_id = (SELECT id FROM Songbooks WHERE name = {lit(book)}) AND number = {num} "
                f"AND instr({field}, {lit(old)}) > 0;\n")

# Check: applied to the database, the songs become exactly what was meant, and nothing else changes
before = dict(((r[0]), (r[1], r[2])) for r in db.execute('SELECT id, song_text, title FROM Songs'))
apply_fixes(db, [os.path.join(outdir, NAME)])
after = dict(((r[0]), (r[1], r[2])) for r in db.execute('SELECT id, song_text, title FROM Songs'))
wrong = [i for i in after if after[i] != expected.get(i, before[i])]
print(f'{len(fixes)} line fixes in {len(expected)} songs; songs not as expected: {wrong[:10]}')
shutil.rmtree(tmp)
