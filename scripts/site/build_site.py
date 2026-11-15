# -*- coding: utf-8 -*-
"""Gop docs/*.md cua project thanh 1 trang HTML (so tay doi thi).

Dung (tu thu muc goc project, sau khi da chay scripts/build_host.ps1 de co build/*.svg):
    python -X utf8 scripts/site/build_site.py . index.html            -> trang doc lap (GitHub Pages)
    python -X utf8 scripts/site/build_site.py . out.html --fragment   -> chi phan noi dung (khong co <html>/<head>)
"""
import html
import os
import re
import sys

ROOT = sys.argv[1]
OUT = sys.argv[2]
FRAGMENT = '--fragment' in sys.argv[3:]
TEMPLATE = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'template.html')

DOCS = [
    ('00-lo-trinh-du-an.md', 'lo-trinh', 'Lộ trình'),
    ('01-tong-hop-the-le.md', 'the-le', 'Thể lệ'),
    ('02-phan-cung.md', 'phan-cung', 'Phần cứng'),
    ('03-firmware-nap-va-van-hanh.md', 'firmware', 'Nạp & vận hành'),
    ('04-calib-va-tinh-chinh.md', 'calib', 'Calib'),
    ('05-thuat-toan.md', 'thuat-toan', 'Thuật toán'),
    ('06-test-tren-may.md', 'test', 'Test trên máy'),
    ('07-nap-ban-do-san.md', 'ban-do', 'Nạp bản đồ'),
    ('08-chien-luoc-thi.md', 'chien-luoc', 'Chiến lược thi'),
    ('09-tham-khao.md', 'tham-khao', 'Tham khảo'),
]
SLUG = {f: s for f, s, _ in DOCS}


def slugify(text, used):
    t = re.sub(r'<[^>]+>', '', text).lower()
    t = re.sub(r'[^\w\s-]', '', t, flags=re.U)
    t = re.sub(r'\s+', '-', t.strip())[:48] or 'muc'
    base, i = t, 2
    while t in used:
        t = f'{base}-{i}'
        i += 1
    used.add(t)
    return t


def link_target(url):
    """Tra ve (href, is_external) hoac (None, None) neu la file trong repo (khong mo duoc tu trang)."""
    if url.startswith('http://') or url.startswith('https://'):
        return url, True
    base = url.split('#')[0]
    name = os.path.basename(base)
    if name in SLUG:
        return '#' + SLUG[name], False
    return None, None


def inline(text):
    codes = []

    def keep_code(m):
        codes.append('<code>' + html.escape(m.group(1)) + '</code>')
        return f'\x00{len(codes) - 1}\x00'

    text = re.sub(r'`([^`]+)`', keep_code, text)
    text = text.replace('\\*', '\x01')
    text = html.escape(text, quote=False)

    def link(m):
        label, url = m.group(1), m.group(2)
        href, ext = link_target(html.unescape(url))
        if href is None:
            return label  # file trong repo: chi giu chu
        if ext:
            return f'<a href="{href}" target="_blank" rel="noopener">{label}</a>'
        return f'<a href="{href}">{label}</a>'

    text = re.sub(r'\[([^\]]+)\]\(([^)\s]+)\)', link, text)
    text = re.sub(r'\*\*(.+?)\*\*', r'<strong>\1</strong>', text)
    text = re.sub(r'(?<![\w*])\*(?!\s)(.+?)(?<!\s)\*(?![\w*])', r'<em>\1</em>', text)
    text = text.replace('\x01', '*')
    text = re.sub(r'\x00(\d+)\x00', lambda m: codes[int(m.group(1))], text)
    return text


def split_row(line):
    line = line.strip()
    if line.startswith('|'):
        line = line[1:]
    if line.endswith('|'):
        line = line[:-1]
    cells, cur, in_code = [], '', False
    for ch in line:
        if ch == '`':
            in_code = not in_code
        if ch == '|' and not in_code:
            cells.append(cur.strip())
            cur = ''
        else:
            cur += ch
    cells.append(cur.strip())
    return cells


LIST_RE = re.compile(r'^(\s*)([-*]|\d+\.)\s+(.*)$')


def render_list(lines):
    """lines: cac dong cua 1 khoi danh sach (co the long 1-2 cap)."""
    out = []
    stack = []  # (indent, tag)

    def open_list(indent, ordered):
        tag = 'ol' if ordered else 'ul'
        out.append(f'<{tag}>')
        stack.append((indent, tag))

    for raw in lines:
        m = LIST_RE.match(raw)
        if not m:
            # dong noi tiep cua muc truoc
            if out and out[-1].endswith('</li>'):
                out[-1] = out[-1][:-5] + ' ' + inline(raw.strip()) + '</li>'
            continue
        indent = len(m.group(1).expandtabs(4))
        ordered = m.group(2)[0].isdigit()
        body = m.group(3)
        while stack and indent < stack[-1][0]:
            out.append(f'</{stack.pop()[1]}></li>')
        if not stack or indent > stack[-1][0]:
            if stack and out and out[-1].endswith('</li>'):
                out[-1] = out[-1][:-5]  # mo danh sach con ben trong muc cha
            open_list(indent, ordered)
        task = re.match(r'^\[( |x|X)\]\s+(.*)$', body)
        if task:
            checked = ' checked' if task.group(1).lower() == 'x' else ''
            out.append(f'<li class="task"><input type="checkbox" disabled{checked}> <span>{inline(task.group(2))}</span></li>')
        else:
            out.append(f'<li>{inline(body)}</li>')
    while stack:
        tag = stack.pop()[1]
        out.append(f'</{tag}>' + ('</li>' if stack else ''))
    return '\n'.join(out)


def md_to_html(md, used):
    lines = md.replace('\r\n', '\n').split('\n')
    out, toc = [], []
    i, n = 0, len(lines)
    title = ''
    while i < n:
        line = lines[i]
        s = line.strip()
        if not s:
            i += 1
            continue
        if s.startswith('```'):
            lang = s[3:].strip()
            buf = []
            i += 1
            while i < n and not lines[i].strip().startswith('```'):
                buf.append(lines[i])
                i += 1
            i += 1
            code = html.escape('\n'.join(buf))
            cls = ' class="cmd"' if lang == 'bash' else ''
            btn = '<button class="copy" type="button" aria-label="Sao chép lệnh">Copy</button>' if lang == 'bash' else ''
            out.append(f'<div class="code"{cls}>{btn}<pre><code>{code}</code></pre></div>')
            continue
        m = re.match(r'^(#{1,4})\s+(.*)$', s)
        if m:
            level, text = len(m.group(1)), inline(m.group(2))
            if level == 1:
                title = text
                i += 1
                continue
            hid = slugify(text, used)
            if level == 2:
                toc.append((hid, re.sub(r'<[^>]+>', '', text)))
            out.append(f'<h{level} id="{hid}">{text}</h{level}>')
            i += 1
            continue
        if re.match(r'^-{3,}$', s):
            out.append('<hr>')
            i += 1
            continue
        if s.startswith('|') and i + 1 < n and re.match(r'^\|?\s*:?-{3,}', lines[i + 1].strip()):
            head = split_row(s)
            i += 2
            rows = []
            while i < n and lines[i].strip().startswith('|'):
                rows.append(split_row(lines[i]))
                i += 1
            t = ['<div class="table"><table><thead><tr>']
            t += [f'<th>{inline(c)}</th>' for c in head]
            t.append('</tr></thead><tbody>')
            for r in rows:
                t.append('<tr>' + ''.join(f'<td>{inline(c)}</td>' for c in r) + '</tr>')
            t.append('</tbody></table></div>')
            out.append(''.join(t))
            continue
        if s.startswith('>'):
            buf = []
            while i < n and lines[i].strip().startswith('>'):
                buf.append(lines[i].strip()[1:].strip())
                i += 1
            body = inline(' '.join(buf))
            cls = 'note warn' if '⚠' in body else 'note'
            out.append(f'<aside class="{cls}">{body}</aside>')
            continue
        if LIST_RE.match(line):
            buf = []
            while i < n and lines[i].strip() and (LIST_RE.match(lines[i]) or lines[i].startswith('   ')):
                buf.append(lines[i])
                i += 1
            out.append(render_list(buf))
            continue
        # doan van
        buf = []
        while i < n and lines[i].strip() and not re.match(r'^(#{1,4}\s|```|\||>|-{3,}$)', lines[i].strip()) \
                and not LIST_RE.match(lines[i]):
            buf.append(lines[i].strip())
            i += 1
        out.append('<p>' + inline(' '.join(buf)) + '</p>')
    return title, '\n'.join(out), toc


BASE_CSS = """<style>
:root { color-scheme: light; }
body { margin: 0; }
img, svg { max-width: 100%; }
[hidden] { display: none !important; }
</style>"""


def standalone(page):
    """Boc thanh tai lieu HTML day du: <!doctype>, <head> (title, font, style), <body>."""
    cut = page.index('</style>') + len('</style>')
    head, body = page[:cut], page[cut:]
    head = re.sub(r'<meta name="claude:[^>]*>\n?', '', head)
    meta = [
        '<!doctype html>',
        '<html lang="vi">',
        '<head>',
        '<meta charset="utf-8">',
        '<meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover">',
        '<meta name="color-scheme" content="light dark">',
        '<meta name="description" content="Sổ tay đội thi Micromouse MYSTERIOUS MAZE 2026 (HUTECH): '
        'phần cứng, firmware STM32, calib, thuật toán, chiến lược thi.">',
        BASE_CSS,
        head.strip(),
        '</head>',
        '<body>',
        body.strip(),
        '</body>',
        '</html>',
    ]
    return '\n'.join(meta) + '\n'


def main():
    sections, nav = [], []
    for fname, slug, short in DOCS:
        md = open(os.path.join(ROOT, 'docs', fname), encoding='utf-8').read()
        used = set()
        title, body, toc = md_to_html(md, used)
        # tien to id theo trang de khong trung giua cac trang
        body = re.sub(r' id="([^"]+)"', lambda m: f' id="{slug}--{m.group(1)}"', body)
        num = fname[:2]
        toc_html = ''
        if len(toc) >= 3:
            toc_html = '<nav class="toc" aria-label="Trong trang này"><span>Trong trang này</span>' + ''.join(
                f'<button type="button" data-jump="{slug}--{h}">{html.escape(t)}</button>' for h, t in toc) + '</nav>'
        sections.append(
            f'<section class="doc" id="view-{slug}" data-view="{slug}" hidden>'
            f'<header class="doc-head"><span class="doc-num">{num}</span><h1>{title}</h1></header>'
            f'{toc_html}<div class="prose">{body}</div>'
            f'</section>')
        nav.append((slug, num, short))

    def load_svg(name):
        s = open(os.path.join(ROOT, 'build', name), encoding='utf-8').read()
        s = s.replace(" class='anim'", '')
        s = re.sub(r"<style>.*?</style>", '', s, flags=re.S)
        s = re.sub(r"<svg xmlns='http://www.w3.org/2000/svg' width='[\d.]+' height='[\d.]+'",
                   "<svg xmlns='http://www.w3.org/2000/svg' role='img'", s, count=1)
        return s

    tpl = open(TEMPLATE, encoding='utf-8').read()
    nav_html = ''.join(
        f'<a href="#{slug}" data-nav="{slug}"><span class="n">{num}</span>{html.escape(short)}</a>' for slug, num, short in nav)
    page = (tpl.replace('{{NAV}}', nav_html)
               .replace('{{SECTIONS}}', '\n'.join(sections))
               .replace('{{SVG_SEARCH}}', load_svg('btc_search.svg'))
               .replace('{{SVG_MAP}}', load_svg('btc_search_map.svg')))
    if not FRAGMENT:
        page = standalone(page)
    open(OUT, 'w', encoding='utf-8', newline='\n').write(page)
    print('Da ghi', OUT, len(page) // 1024, 'KB')


main()
