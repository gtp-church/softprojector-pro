-- Corrections of the church's Bible translations.
-- SoftProjector applies this file once when it starts (see applyDataFixes in source/main.cpp).

-- The translations are found by name, since their ids may differ between databases
CREATE TEMP TABLE fix_bibles AS
SELECT CAST((SELECT id FROM BibleVersions WHERE abbreviation = 'RST') AS TEXT) AS rst,
       CAST((SELECT id FROM BibleVersions WHERE bible_name = 'New Russian Translation') AS TEXT) AS nrt;

-- Synodal: 3 and 4 Kings have 22 and 25 chapters; the chapter list showed an extra empty chapter
UPDATE BibleBooks SET chapter_count = 22
WHERE bible_id = (SELECT rst FROM fix_bibles) AND id = 11 AND chapter_count = 23;
UPDATE BibleBooks SET chapter_count = 25
WHERE bible_id = (SELECT rst FROM fix_bibles) AND id = 12 AND chapter_count = 26;

-- Synodal: the titles of psalms 50, 51, 53 and 59 take verses 1 and 2, and both verses had the
-- same id, so verse 2 was shown with the text of verse 1
UPDATE BibleVerse SET verse_id = verse_id || 'b'
WHERE bible_id = (SELECT rst FROM fix_bibles) AND book = '19' AND verse = 2
  AND verse_id IN ('B019C051V000', 'B019C052V000', 'B019C054V000', 'B019C060V000');

-- New Russian Translation: Russian title and abbreviation
UPDATE BibleVersions SET bible_name = 'Новый русский перевод', abbreviation = 'НРП'
WHERE id = (SELECT nrt FROM fix_bibles);

-- New Russian Translation: book names and order as in the Synodal translation
CREATE TEMP TABLE fix_books AS
SELECT n.id AS id, r.book_name AS book_name, n.chapter_count AS chapter_count, r.rowid AS ord
FROM BibleBooks n JOIN BibleBooks r ON r.id = n.id AND r.bible_id = (SELECT rst FROM fix_bibles)
WHERE n.bible_id = (SELECT nrt FROM fix_bibles);
DELETE FROM BibleBooks
WHERE bible_id = (SELECT nrt FROM fix_bibles) AND id IN (SELECT id FROM fix_books);
INSERT INTO BibleBooks (bible_id, id, book_name, chapter_count)
SELECT (SELECT nrt FROM fix_bibles), id, book_name, chapter_count FROM fix_books ORDER BY ord;

-- New Russian Translation: remove the "<n>[1]</n>" markers that were shown on the screen
UPDATE BibleVerse
SET verse_text = trim(replace(replace(replace(verse_text, '<n>[1]</n>', ''), '  ', ' '), '  ', ' '))
WHERE bible_id = (SELECT nrt FROM fix_bibles) AND verse_text LIKE '%<n>[1]</n>%';

-- New Russian Translation: verse ids as in the Synodal translation. Translations shown side by side
-- are matched by verse id; in the psalms the ids followed another numbering, so the second
-- translation showed the next psalm or nothing at all.
CREATE TEMP TABLE fix_ids AS
SELECT book, chapter, verse, MIN(rowid) AS rid FROM BibleVerse
WHERE bible_id = (SELECT rst FROM fix_bibles)
GROUP BY book, chapter, verse;
CREATE INDEX temp.fix_ids_place ON fix_ids (book, chapter, verse);
UPDATE BibleVerse SET verse_id = (
    SELECT v.verse_id FROM fix_ids f JOIN BibleVerse v ON v.rowid = f.rid
    WHERE f.book = BibleVerse.book AND f.chapter = BibleVerse.chapter AND f.verse = BibleVerse.verse)
WHERE bible_id = (SELECT nrt FROM fix_bibles)
  AND EXISTS (SELECT 1 FROM fix_ids f
              WHERE f.book = BibleVerse.book AND f.chapter = BibleVerse.chapter AND f.verse = BibleVerse.verse);

DROP TABLE fix_ids;
DROP TABLE fix_books;
DROP TABLE fix_bibles;
