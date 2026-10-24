# Makes a PDF from a Markdown document in docs/ (the guide for the media team and similar).
# Needs the "markdown" Python package and Google Chrome:
#   python3 -m venv /tmp/md && /tmp/md/bin/pip install markdown==3.7
#   /tmp/md/bin/python tools/md2pdf.py "docs/Что нового.md"
import os, re, subprocess, sys, tempfile
import markdown

CHROME = '/Applications/Google Chrome.app/Contents/MacOS/Google Chrome'
CSS = """
@page { size: A4; margin: 16mm 15mm 16mm 15mm; }
body { font-family: -apple-system, 'Helvetica Neue', 'Segoe UI', Arial, sans-serif; font-size: 10.5pt; line-height: 1.5; color: #1d2330; }
h1 { font-size: 24pt; margin: 0 0 2px; color: #14306b; letter-spacing: -0.3px; }
h1 + p em { color: #5a6578; font-style: normal; font-size: 10pt; }
h2 { font-size: 14.5pt; color: #14306b; margin: 22px 0 6px; padding-bottom: 4px; border-bottom: 2px solid #dfe6f3; break-after: avoid; }
h2 + * { break-before: avoid; }
p, li { margin: 4px 0; }
ul, ol { padding-left: 20px; margin: 4px 0 8px; }
strong { color: #0f1a2e; }
table { border-collapse: collapse; width: 100%; margin: 8px 0 10px; font-size: 9.8pt; break-inside: auto; }
tr { break-inside: avoid; }
th, td { border: 1px solid #d5dceb; padding: 5px 8px; text-align: left; vertical-align: top; }
th { background: #eef2fa; color: #14306b; }
tr:nth-child(even) td { background: #f8f9fc; }
blockquote { margin: 8px 0; padding: 7px 12px; background: #fff8e6; border-left: 4px solid #f0b429; border-radius: 3px; }
blockquote p { margin: 0; }
code { font-family: Menlo, Consolas, monospace; font-size: 9pt; background: #eef1f6; padding: 1px 4px; border-radius: 3px; }
pre { background: #1f2633; color: #e8ecf3; padding: 8px 12px; border-radius: 5px; break-inside: avoid; }
pre code { background: none; color: inherit; padding: 0; font-size: 9.5pt; }
hr { border: none; border-top: 1px solid #dfe6f3; margin: 18px 0; }
"""

source = sys.argv[1]
text = open(source, encoding='utf-8').read()
title = re.search(r'^# (.+)$', text, re.M)
title = title.group(1) if title else os.path.splitext(os.path.basename(source))[0]
body = markdown.markdown(text, extensions=['tables', 'fenced_code'])
page = (f'<!doctype html><html lang="ru"><head><meta charset="utf-8"><title>{title}</title>'
        f'<style>{CSS}</style></head><body>{body}</body></html>')
with tempfile.NamedTemporaryFile('w', suffix='.html', delete=False, encoding='utf-8') as f:
    f.write(page)
pdf = os.path.splitext(source)[0] + '.pdf'
subprocess.run([CHROME, '--headless=new', '--disable-gpu', '--no-pdf-header-footer',
                '--print-to-pdf=' + os.path.abspath(pdf), 'file://' + f.name],
               check=True, stderr=subprocess.DEVNULL)
os.unlink(f.name)
print(pdf)
