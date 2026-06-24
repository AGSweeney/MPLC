#!/usr/bin/env python3
"""Add BSD-3-Clause copyright header to MPLC project source files."""

# Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
# SPDX-License-Identifier: BSD-3-Clause

from __future__ import annotations

import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

COPYRIGHT_MARK = "Copyright (c) 2026 Adam G. Sweeney"

C_BLOCK = (
    "/*\n"
    " * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>\n"
    " * SPDX-License-Identifier: BSD-3-Clause\n"
    " */\n"
)

HASH_BLOCK = (
    "# Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>\n"
    "# SPDX-License-Identifier: BSD-3-Clause\n"
)

SKIP_DIR_NAMES = {
    ".git",
    "build",
    "build_local",
    "obj",
    "CMakeFiles",
    "Testing",
    "mplc_studio_autogen",
    ".metadata",
    "node_modules",
}

SKIP_EXTENSIONS = {
    ".exe",
    ".obj",
    ".o",
    ".lib",
    ".a",
    ".dll",
    ".so",
    ".bin",
    ".mplc",
    ".png",
    ".ico",
    ".gif",
    ".jpg",
    ".jpeg",
    ".webp",
    ".pdf",
    ".recipe",
    ".tlog",
    ".stamp",
    ".depend",
    ".list",
    ".log",
    ".yaml",
    ".yml",
    ".sln",
    ".vcxproj",
    ".filters",
    ".ninja",
    ".deps",
    ".d",
    ".s19",
    ".map",
    ".elf",
    ".hex",
}

TEXT_EXTENSIONS = {
    ".c",
    ".h",
    ".cpp",
    ".hpp",
    ".cc",
    ".cxx",
    ".cmake",
    ".md",
    ".py",
    ".ps1",
    ".st",
    ".il",
    ".xml",
    ".json",
    ".rc",
    ".qrc",
    ".txt",
    ".svg",
    ".rule",
    ".gitignore",
}

SPECIAL_NAMES = {"CMakeLists.txt", ".gitignore", "LICENSE", "makefile", "Makefile"}


def should_skip_dir(path: str) -> bool:
    parts = path.replace("\\", "/").split("/")
    return any(part in SKIP_DIR_NAMES for part in parts)


def pick_header(path: str, first_line: str) -> str | None:
    ext = os.path.splitext(path)[1].lower()
    base = os.path.basename(path)

    if base == "LICENSE":
        return None
    if ext in {".md"} or base.endswith(".md"):
        return None
    if ext in {".cmake", ".py", ".ps1", ".qrc", ".rule"} or base == "CMakeLists.txt":
        return HASH_BLOCK
    if ext == ".gitignore" or base == ".gitignore":
        return HASH_BLOCK
    if ext in {".st", ".il", ".xml", ".json", ".txt"}:
        return HASH_BLOCK
    if ext == ".svg":
        return (
            "<!-- Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com> "
            "SPDX-License-Identifier: BSD-3-Clause -->\n"
        )
    if ext in {".c", ".h", ".cpp", ".hpp", ".cc", ".cxx", ".rc"}:
        return C_BLOCK
    return None


def insert_header(content: str, header: str, path: str) -> str:
    if content.startswith("#NB_COPYRIGHT#"):
        lines = content.splitlines(keepends=True)
        return lines[0] + header + "".join(lines[1:])
    if content.startswith("#!"):
        lines = content.splitlines(keepends=True)
        first = lines[0]
        rest = "".join(lines[1:])
        if rest.startswith("\n"):
            return first + header + rest
        return first + "\n" + header + rest
    return header + ("\n" if content and not content.startswith("\n") else "") + content


def process_file(path: str) -> bool:
    try:
        with open(path, "r", encoding="utf-8", newline="") as f:
            content = f.read()
    except (UnicodeDecodeError, OSError):
        return False

    if COPYRIGHT_MARK in content:
        return False

    header = pick_header(path, content.split("\n", 1)[0] if content else "")
    if header is None:
        return False

    new_content = insert_header(content, header, path)
    with open(path, "w", encoding="utf-8", newline="") as f:
        f.write(new_content)
    return True


def main() -> int:
    updated: list[str] = []
    skipped_binary = 0

    for dirpath, dirnames, filenames in os.walk(ROOT):
        dirnames[:] = [d for d in dirnames if d not in SKIP_DIR_NAMES]
        if should_skip_dir(dirpath):
            dirnames.clear()
            continue

        for name in filenames:
            path = os.path.join(dirpath, name)
            rel = os.path.relpath(path, ROOT)
            ext = os.path.splitext(name)[1].lower()

            if ext in SKIP_EXTENSIONS:
                skipped_binary += 1
                continue
            if name.startswith("moc_") and ext == ".cpp":
                continue
            if name in {"mocs_compilation.cpp", "CMakeCCompilerId.c", "CMakeCXXCompilerId.cpp"}:
                continue
            if ext not in TEXT_EXTENSIONS and name not in SPECIAL_NAMES:
                continue

            if process_file(path):
                updated.append(rel)

    print(f"Updated {len(updated)} files")
    for p in sorted(updated):
        print(f"  {p}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
