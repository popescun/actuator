#!/usr/bin/env python3
"""Build refman.ind from refman.idx -- a stand-in for makeindex.

Doxygen's LaTeX output ends with an alphabetical index that \\printindex pulls
from refman.ind. That file is normally produced by makeindex, which ships with a
full TeX distribution. When the PDF is built with a lightweight engine such as
tectonic there is no makeindex, and the failure is silent: the PDF builds and
looks complete, but its index is empty.

Usage, from the directory holding refman.tex:

    tectonic -X compile refman.tex --outdir .   # pass 1 -- writes refman.idx
    python3 tools/make_index.py                 # this script -- writes refman.ind
    tectonic -X compile refman.tex --outdir .   # pass 2 -- embeds the index

refman.idx does not exist until pass 1 has run; it is a LaTeX output, not a
doxygen one.

Input lines look like:

    \\indexentry{sort@{display}!sort2@{display2}|hyperpage}{12}

The sort key precedes '@', the rendered text follows it in braces, and '!'
separates an item from its subitem.
"""

import collections
import re
import sys

ENTRY = re.compile(r'\\indexentry\{(.*)\|hyperpage\}\{(\d+)\}\s*$')
DISPLAY = re.compile(r'^(.*?)@\{(.*)\}$')


def split_levels(key):
    """Split an index key on '!', ignoring separators nested inside braces."""
    parts, depth, current = [], 0, ''
    for char in key:
        if char == '{':
            depth += 1
        elif char == '}':
            depth -= 1
        if char == '!' and depth == 0:
            parts.append(current)
            current = ''
        else:
            current += char
    parts.append(current)
    return parts


def read_entries(path):
    entries = []
    for line in open(path, encoding='utf-8'):
        match = ENTRY.match(line)
        if not match:
            continue
        page = int(match.group(2))
        levels = []
        for part in split_levels(match.group(1)):
            shown = DISPLAY.match(part)
            levels.append(shown.group(2) if shown else part)
        entries.append((levels, page))
    return entries


def build_tree(entries):
    """Group entries by item, then subitem, keeping page lists unique."""
    tree = collections.OrderedDict()
    for levels, page in sorted(entries, key=lambda e: [l.lower() for l in e[0]]):
        node = tree.setdefault(levels[0], {'pages': [], 'subs': collections.OrderedDict()})
        pages = node['pages'] if len(levels) == 1 else node['subs'].setdefault(levels[1], [])
        if page not in pages:
            pages.append(page)
    return tree


def render(tree):
    hyperpages = lambda pages: ', '.join(f'\\hyperpage{{{p}}}' for p in sorted(pages))
    lines, previous_letter = ['\\begin{theindex}'], None
    for item, node in tree.items():
        letter = item[0].lower()
        if previous_letter is not None and letter != previous_letter:
            lines.append('\n  \\indexspace')
        previous_letter = letter
        pages = f', {hyperpages(node["pages"])}' if node['pages'] else ''
        lines.append(f'  \\item {item}{pages}')
        for subitem, subpages in node['subs'].items():
            lines.append(f'    \\subitem {subitem}, {hyperpages(subpages)}')
    lines.append('\n\\end{theindex}')
    return '\n'.join(lines) + '\n'


def main():
    idx = sys.argv[1] if len(sys.argv) > 1 else 'refman.idx'
    ind = sys.argv[2] if len(sys.argv) > 2 else 'refman.ind'
    try:
        entries = read_entries(idx)
    except FileNotFoundError:
        sys.exit(f'{idx} not found -- run the first LaTeX pass before this script')
    if not entries:
        sys.exit(f'{idx} holds no \\indexentry lines')
    tree = build_tree(entries)
    open(ind, 'w', encoding='utf-8').write(render(tree))
    print(f'{ind}: {len(entries)} entries -> {len(tree)} items')


if __name__ == '__main__':
    main()
