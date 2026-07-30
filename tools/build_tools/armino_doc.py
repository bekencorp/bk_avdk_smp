#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
ARMINO 文档构建工具，内置 Markdown → RST 转换。
"""

from __future__ import annotations

import re
import sys
import textwrap
import unicodedata
from dataclasses import dataclass, field
from enum import Enum, auto
from typing import Callable, Iterable, List, Optional, Sequence, Tuple

import os
import shutil
import subprocess
import filecmp


# ---------------------------------------------------------------------------
# 常量
# ---------------------------------------------------------------------------

# RST 标题下划线字符（按层级循环使用）
RST_TITLE_CHARS = "=-~^\"'#+*"

# 围栏代码块
FENCE_PATTERN = re.compile(
    r"^(?P<indent>[ \t]*)(?P<fence>`{3,}|~{3,})"
    r"(?P<info>[ \t]*[^\n`~]*)?\s*$"
)

# ATX 标题 (# ## ...)
ATX_HEADER_PATTERN = re.compile(
    r"^(?P<indent>[ \t]*)(?P<hashes>#{1,6})\s+"
    r"(?P<title>.+?)\s*(?:#+\s*)?$"
)

# Setext 标题下划线
SETEXT_UNDERLINE_PATTERN = re.compile(
    r"^(?P<indent>[ \t]*)(?P<underline>=+|-+)\s*$"
)

# 无序列表
UNORDERED_LIST_PATTERN = re.compile(
    r"^(?P<indent>[ \t]*)(?P<marker>[-*+])\s+(?P<content>.+)$"
)

# 有序列表
ORDERED_LIST_PATTERN = re.compile(
    r"^(?P<indent>[ \t]*)(?P<num>\d+)\.\s+(?P<content>.+)$"
)

# 任务列表
TASK_LIST_PATTERN = re.compile(
    r"^(?P<indent>[ \t]*)(?P<marker>[-*+])\s+"
    r"\[(?P<checked>[ xX])\]\s+(?P<content>.+)$"
)

# 引用块
BLOCKQUOTE_PATTERN = re.compile(r"^(?P<indent>[ \t]*)(?P<mark>>+)\s?(?P<content>.*)$")

# 水平线
HR_PATTERN = re.compile(r"^(?P<indent>[ \t]*)([-*_]){3,}\s*$")

# 表格分隔行
TABLE_SEPARATOR_PATTERN = re.compile(
    r"^\|?\s*:?-{3,}:?\s*(\|\s*:?-{3,}:?\s*)+\|?\s*$"
)

# 表格行
TABLE_ROW_PATTERN = re.compile(r"^\|.+\|$")

# 定义列表 (term\n: definition)
DEFINITION_PATTERN = re.compile(r"^(?P<indent>[ \t]*)(?P<term>[^:]+):\s*$")

# 脚注定义 [^id]: text
FOOTNOTE_DEF_PATTERN = re.compile(
    r"^(?P<indent>[ \t]*)\[\^(?P<id>[^\]]+)\]:\s*(?P<content>.*)$"
)

# 脚注引用 [^id]
FOOTNOTE_REF_PATTERN = re.compile(r"\[\^(?P<id>[^\]]+)\]")

# HTML 块开始/结束（简单处理）
HTML_BLOCK_START = re.compile(r"^\s*<(?P<tag>\w+)[^>]*>\s*$", re.IGNORECASE)
HTML_BLOCK_END = re.compile(r"^\s*</(?P<tag>\w+)>\s*$", re.IGNORECASE)

# 自动链接 <http://...>
AUTOLINK_PATTERN = re.compile(r"<((?:https?|ftp|mailto):[^>\s]+)>")

# 参考式链接定义 [ref]: url "title"
REF_LINK_DEF_PATTERN = re.compile(
    r"^(?P<indent>[ \t]*)\[(?P<label>[^\]]+)\]:\s+"
    r"(?P<url><[^>]+>|[^\s]+)\s*(?P<title>\"[^\"]*\"|'[^']*'|\([^)]*\))?\s*$"
)

# 缩进代码块检测（4空格或tab）
INDENTED_CODE_PATTERN = re.compile(r"^(?:    |\t)(?P<code>.*)$")

# RST 块级语法（Markdown 中嵌入 RST 时原样保留）
RST_BLOCK_PATTERN = re.compile(r"^(?P<indent>[ \t]*)\.\.(?P<body>.*)$")

# RST 行内语法（转换 MD 时需保护，避免被误处理）
RST_INLINE_HYPERLINK = re.compile(r"`[^`\n]+?\s+<[^>\n]+>`_")
RST_INLINE_ROLE = re.compile(r":[a-zA-Z][\w.-]*:`[^`\n]*?`")
RST_INLINE_FOOTNOTE_REF = re.compile(r"\[\#[^\]]+\]_")
RST_INLINE_SUBSTITUTION = re.compile(r"\|[\w\-]+\|")

# RST 字面量块（段落以 :: 结尾 + 缩进内容）
RST_LITERAL_MARKER = re.compile(r"::\s*$")

# RST 嵌套转换
MAX_RST_NEST_DEPTH = 32

# 内容为代码/原始文本、不做 MD 转换的 RST 指令
RST_CODE_DIRECTIVES = frozenset({
    "code-block",
    "sourcecode",
    "literalinclude",
    "parsed-literal",
    "highlight",
    "program",
    "console",
    "code",
})

RST_DIRECTIVE_OPTION = re.compile(r"^(:[\w-]+:)(\s*)(.*)$")
RST_LIST_TABLE_ROW = re.compile(r"^(\*\s+-|-)(\s*)(.*)$")
RST_FOOTNOTE_OPENER = re.compile(r"^(\s*\[\#[^\]]+\]\s+)(.*)$")
RST_SUBSTITUTION_OPENER = re.compile(r"^(\s*\|[^|]+\|\s+\w+::\s*)(.*)$")
RST_HYPERLINK_OPENER = re.compile(r"^(\s*_[^:\s]+:\s*)(.*)$")


class BlockState(Enum):
    NORMAL = auto()
    CODE_FENCE = auto()
    BLOCKQUOTE = auto()
    LIST = auto()
    TABLE = auto()
    HTML_BLOCK = auto()
    FOOTNOTE_DEF = auto()


@dataclass
class ConversionOptions:
    """转换选项。"""

    default_code_language: str = ""
    wrap_width: int = 0  # 0 表示不自动换行
    strict: bool = False  # True 时对未知结构发出警告
    preserve_html: bool = True  # 保留无法转换的 HTML
    use_grid_tables: bool = True
    title_char_offset: int = 0  # 标题字符起始偏移（用于嵌套文档）
    # MD 管道表格 → list-table 时：None 表示按表格结构自动推断
    list_table_header_rows: Optional[int] = None
    list_table_widths: Optional[Tuple[str, ...]] = None


@dataclass
class ConversionResult:
    """转换结果。"""

    text: str
    warnings: List[str] = field(default_factory=list)


@dataclass
class ListNode:
    """列表树节点，用于渲染 RST 嵌套列表。"""

    indent: int
    content: str
    ordered: bool
    task: bool
    number: int
    children: List[ListNode] = field(default_factory=list)


class InlineConverter:
    """行内 Markdown 元素转换。"""

    def __init__(self, options: ConversionOptions):
        self.options = options
        self._ref_links: dict[str, Tuple[str, str]] = {}
        self._footnotes: dict[str, str] = {}
        self.warnings: List[str] = []

    def register_ref_link(self, label: str, url: str, title: str = "") -> None:
        key = label.strip().lower()
        self._ref_links[key] = (url, title)

    def register_footnote(self, footnote_id: str, content: str) -> None:
        self._footnotes[footnote_id] = content

    def convert(self, text: str, escape_rst: bool = False) -> str:
        """转换行内 Markdown。"""
        if not text:
            return ""

        try:
            # 保护已有 RST / MD 行内标记，再执行 MD → RST 转换
            protected, rst_spans = self._protect_rst_spans(text)
            protected, code_spans = self._protect_code_spans(protected)
            result = protected
            result = self._convert_footnote_refs(result)
            result = self._convert_strikethrough(result)
            result = self._convert_bold_italic(result)
            result = self._convert_autolinks(result)
            result = self._convert_images(result)
            result = self._convert_links(result)
            result = self._convert_ref_links(result)
            result = self._convert_html_tags(result)
            result = self._restore_code_spans(result, code_spans)
            result = self._restore_rst_spans(result, rst_spans)
            result = self._finalize_rst_inline(result)
            if escape_rst:
                result = self._escape_rst_text(result)
            return result
        except Exception as exc:
            self.warnings.append(f"行内转换失败，保留原文: {exc}")
            return text

    def _protect_rst_spans(self, text: str) -> Tuple[str, List[str]]:
        """保护 Markdown 中已有的 RST 行内语法。"""
        spans: List[str] = []
        patterns = (
            RST_INLINE_HYPERLINK,
            RST_INLINE_ROLE,
            RST_INLINE_FOOTNOTE_REF,
            RST_INLINE_SUBSTITUTION,
        )

        for pattern in patterns:
            def repl(match: re.Match, _spans=spans) -> str:
                _spans.append(match.group(0))
                return f"\x00RST{len(_spans) - 1}\x00"

            text = pattern.sub(repl, text)
        return text, spans

    @staticmethod
    def _restore_rst_spans(text: str, spans: List[str]) -> str:
        for idx, span in enumerate(spans):
            text = text.replace(f"\x00RST{idx}\x00", span)
        return text

    def _protect_code_spans(self, text: str) -> Tuple[str, List[str]]:
        """保护行内代码，返回占位文本与代码列表。"""
        spans: List[str] = []

        def repl(match: re.Match) -> str:
            original = match.group(0)
            inner = match.group(1)
            escaped = inner.replace("\\", "\\\\").replace("`", "\\`")
            rst_code = f"``{escaped}``"
            spans.append(rst_code)
            return f"\x00CODE{len(spans) - 1}\x00"

        protected = re.sub(r"(?<![`])``([^`]+)``(?![`])", repl, text)
        protected = re.sub(r"(?<![`\\])`([^`\n]+)`(?![`])", repl, protected)
        return protected, spans

    @staticmethod
    def _literal_code_inner(code: str) -> str:
        """从 ``literal`` 形式提取内容。"""
        if code.startswith("``") and code.endswith("``") and len(code) >= 4:
            return code[2:-2]
        return code

    @staticmethod
    def _restore_code_spans(text: str, spans: List[str]) -> str:
        for idx, code in enumerate(spans):
            placeholder = f"\x00CODE{idx}\x00"
            # **`code`** → :strong:`code`（勿还原为 :strong:``code```）
            strong_wrapped = f":strong:`{placeholder}`"
            if strong_wrapped in text:
                inner = InlineConverter._literal_code_inner(code)
                text = text.replace(strong_wrapped, f":strong:`{inner}`")
                continue
            text = text.replace(placeholder, code)
        return text

    def _convert_images(self, text: str) -> str:
        """![alt](url) -> :raw-html:`<img ...>` 占位，块级会单独处理；行内用 link"""

        def repl(match: re.Match) -> str:
            alt = match.group(1) or ""
            url = match.group(2)
            title = match.group(3) or ""
            alt_clean = alt.replace("\n", " ")
            if title:
                return f"`:raw-html:`<img src=\"{url}\" alt=\"{alt_clean}\" title=\"{title}\">``"
            return f"`:raw-html:`<img src=\"{url}\" alt=\"{alt_clean}\">``"

        # 带 title 的图片
        text = re.sub(
            r"!\[([^\]]*)\]\(([^\s\)\"]+)(?:\s+\"([^\"]*)\")?\)",
            repl,
            text,
        )
        return text

    def _convert_links(self, text: str) -> str:
        """[text](url) -> `text <url>`_"""

        def repl(match: re.Match) -> str:
            link_text = match.group(1)
            url = match.group(2)
            title = match.group(3)
            converted_text = self.convert(link_text)
            if title:
                return f"`{converted_text} <{url}>`_ ({title})"
            return f"`{converted_text} <{url}>`_"

        text = re.sub(
            r"(?<!!)\[([^\]]+)\]\(([^\s\)\"]+)(?:\s+\"([^\"]*)\")?\)",
            repl,
            text,
        )
        return text

    def _convert_ref_links(self, text: str) -> str:
        """[text][ref] 和 [text][]"""

        def repl_full(match: re.Match) -> str:
            link_text = match.group(1)
            label = (match.group(2) or link_text).strip().lower()
            if label not in self._ref_links:
                if self.options.strict:
                    self.warnings.append(f"未找到参考链接定义: [{label}]")
                return match.group(0)
            url, title = self._ref_links[label]
            converted_text = self.convert(link_text)
            if title:
                clean_title = title.strip("\"'")
                return f"`{converted_text} <{url}>`_ ({clean_title})"
            return f"`{converted_text} <{url}>`_"

        def repl_short(match: re.Match) -> str:
            link_text = match.group(1)
            label = link_text.strip().lower()
            if label not in self._ref_links:
                if self.options.strict:
                    self.warnings.append(f"未找到参考链接定义: [{label}]")
                return match.group(0)
            url, title = self._ref_links[label]
            converted_text = self.convert(link_text)
            if title:
                clean_title = title.strip("\"'")
                return f"`{converted_text} <{url}>`_ ({clean_title})"
            return f"`{converted_text} <{url}>`_"

        text = re.sub(r"\[([^\]]+)\]\[([^\]]*)\]", repl_full, text)
        text = re.sub(r"\[([^\]]+)\]\[\]", repl_short, text)
        return text

    def _convert_autolinks(self, text: str) -> str:
        def repl(match: re.Match) -> str:
            url = match.group(1)
            return f"`{url} <{url}>`_"

        return AUTOLINK_PATTERN.sub(repl, text)

    def _convert_strikethrough(self, text: str) -> str:
        """~~text~~ → 删除线文本（docutils 无标准 strike role，保留正文）。"""

        def repl(match: re.Match) -> str:
            return self.convert(match.group(1))

        return re.sub(r"~~([^~]+)~~", repl, text)

    @staticmethod
    def _escape_role_text(text: str) -> str:
        """转义 RST role 反引号内容中的特殊字符。"""
        return text.replace("\\", "\\\\").replace("`", "\\`")

    @classmethod
    def _to_rst_strong(cls, text: str, *, emphatic: bool = False) -> str:
        """将粗体文本转为 RST :strong: role（比 **...** 更适合同段多处粗体）。"""
        escaped = cls._escape_role_text(text)
        if emphatic:
            escaped = cls._escape_role_text(f"*{text}*")
        return f":strong:`{escaped}`"

    def _convert_bold_italic(self, text: str) -> str:
        """处理 **bold**, __bold__, *italic*, _italic_, ***bold italic***"""

        # ***bold italic*** / ___both___
        def repl_bold_italic(match: re.Match) -> str:
            inner = match.group(1)
            return self._to_rst_strong(inner, emphatic=True)

        text = re.sub(r"\*\*\*([^*]+)\*\*\*", repl_bold_italic, text)
        text = re.sub(r"___([^_]+)___", repl_bold_italic, text)

        # **bold** / __bold__  → :strong:`...`（避免 docutils 合并相邻 **...**）
        def repl_bold(match: re.Match) -> str:
            return self._to_rst_strong(match.group(1))

        text = re.sub(r"\*\*([^*]+)\*\*", repl_bold, text)
        text = re.sub(r"__([^_]+)__", repl_bold, text)

        # *italic* — 避免误匹配列表标记
        def repl_italic_star(match: re.Match) -> str:
            inner = match.group(1)
            if not inner.strip():
                return match.group(0)
            return f"*{inner}*"

        text = re.sub(r"(?<!\*)\*([^*\n]+)\*(?!\*)", repl_italic_star, text)

        def repl_italic_under(match: re.Match) -> str:
            inner = match.group(1)
            if not inner.strip():
                return match.group(0)
            return f"*{inner}*"

        # 仅匹配非标识符语境下的 _italic_（避免 bk_pp_process 被误转换）
        text = re.sub(r"(?<![\w])_([^_\n]+)_(?![\w])", repl_italic_under, text)
        return text

    _RST_INLINE_ROLE_END = re.compile(r"(:[\w-]+:`[^`]*`)(?=[^\s、，])")

    @classmethod
    def _finalize_rst_inline(cls, text: str) -> str:
        """调整行内 markup 间距，满足 docutils 解析要求。"""
        if not text:
            return text
        # role 前若紧贴文字/标点，需空格（如 其:strong:` → 其 :strong:`）
        text = re.sub(r"(?<=[^\s`])(:[\w-]+:)", r" \1", text)
        # role 后若紧贴中文/括号等，需空格
        text = cls._RST_INLINE_ROLE_END.sub(r"\1 ", text)
        # 行内字面量 ``...`` 后紧贴中文/全角括号时，需空格
        text = re.sub(
            r"(``(?:[^`]|`(?!`))*``)(?=[\u4e00-\u9fff（）【】「」『』《》〈〉])",
            r"\1 ",
            text,
        )
        # 兜底：**:strong:``code``` → :strong:`code`
        text = re.sub(
            r":strong:`(?:``)+([^`]+?)(?:``)+`",
            r":strong:`\1`",
            text,
        )
        # 超链接结尾 `_` 后若紧跟非空白字符，需转义 `_`（如 `>_`（...）
        text = re.sub(r"(<[^>\n]+>)`_([^\s])", r"\1`\\_\2", text)
        return text

    def _convert_footnote_refs(self, text: str) -> str:
        def repl(match: re.Match) -> str:
            fid = match.group("id")
            return f" [#{fid}]_"

        return FOOTNOTE_REF_PATTERN.sub(repl, text)

    def _convert_html_tags(self, text: str) -> str:
        """保留或简化常见 HTML 标签。"""
        if not self.options.preserve_html:
            return re.sub(r"<[^>]+>", "", text)

        # <br>, <br/> -> 换行标记
        text = re.sub(r"<br\s*/?>", "\n", text, flags=re.IGNORECASE)

        # <kbd>key</kbd> -> 行内字面量（docutils 标准语法）
        def kbd_repl(match: re.Match) -> str:
            inner = match.group(1).replace("`", "\\`")
            return f"``{inner}``"

        text = re.sub(
            r"<kbd>([^<]+)</kbd>",
            kbd_repl,
            text,
            flags=re.IGNORECASE,
        )

        # <code>code</code> 已在 inline code 处理
        text = re.sub(
            r"<code>([^<]+)</code>",
            lambda m: f"``{m.group(1)}``",
            text,
            flags=re.IGNORECASE,
        )

        return text

    @staticmethod
    def _escape_rst_text(text: str) -> str:
        """转义 RST 特殊字符（在非指令上下文）。"""
        # 仅转义独立出现的特殊字符
        special = r"\*`|<>"
        result = []
        i = 0
        while i < len(text):
            ch = text[i]
            if ch in special and (i == 0 or text[i - 1] != "\\"):
                result.append("\\" + ch)
            else:
                result.append(ch)
            i += 1
        return "".join(result)


class MarkdownToRstConverter:
    """Markdown 到 RST 的主转换器。"""

    def __init__(self, options: Optional[ConversionOptions] = None):
        self.options = options or ConversionOptions()
        self.inline = InlineConverter(self.options)
        self.warnings: List[str] = []

    @staticmethod
    def _convert_translation_links(text: str) -> str:
        """将 MD 语言切换链接转为 Sphinx :link_to_translation: role。"""
        text = re.sub(
            r"^\s*[*-]\s+\[English\]\(\./README\.md\)",
            r":link_to_translation:`en:[English]`",
            text,
            flags=re.MULTILINE,
        )
        text = re.sub(
            r"^\s*[*-]\s+\[中文\]\(\./README_CN\.md\)",
            r":link_to_translation:`zh_CN:[中文]`",
            text,
            flags=re.MULTILINE,
        )
        text = re.sub(
            r"\[English\]\(\./README\.md\)",
            r":link_to_translation:`en:[English]`",
            text,
        )
        text = re.sub(
            r"\[中文\]\(\./README_CN\.md\)",
            r":link_to_translation:`zh_CN:[中文]`",
            text,
        )
        return text

    def convert(self, markdown_text: str) -> ConversionResult:
        """将 Markdown 文本转换为 RST。"""
        if markdown_text is None:
            return ConversionResult(text="", warnings=["输入为 None，返回空字符串"])

        try:
            # 统一换行符
            text = markdown_text.replace("\r\n", "\n").replace("\r", "\n")
            text = self._convert_translation_links(text)
            lines = text.split("\n")

            # 预处理：收集参考链接和脚注定义
            lines = self._extract_definitions(lines)

            output_lines: List[str] = []
            i = 0
            state = BlockState.NORMAL
            pending_setext_title: Optional[Tuple[str, str]] = None

            while i < len(lines):
                line = lines[i]

                try:
                    # 围栏代码块
                    fence_match = FENCE_PATTERN.match(line)
                    if fence_match and state != BlockState.CODE_FENCE:
                        block_lines, i = self._parse_code_fence(lines, i)
                        output_lines.extend(block_lines)
                        continue

                    if state == BlockState.CODE_FENCE:
                        block_lines, i = self._parse_code_fence(lines, i)
                        output_lines.extend(block_lines)
                        continue

                    # 空行
                    if not line.strip():
                        output_lines.append("")
                        pending_setext_title = None
                        i += 1
                        continue

                    # Setext 标题（需要先缓存上一行）
                    setext_match = SETEXT_UNDERLINE_PATTERN.match(line)
                    if setext_match and pending_setext_title:
                        title_line, _ = pending_setext_title
                        underline = setext_match.group("underline")
                        char = "=" if underline.startswith("=") else "-"
                        output_lines.pop()  # 移除已写入的 title 行
                        output_lines.extend(self._make_title(title_line, char))
                        pending_setext_title = None
                        i += 1
                        continue

                    # ATX 标题
                    atx_match = ATX_HEADER_PATTERN.match(line)
                    if atx_match:
                        level = len(atx_match.group("hashes"))
                        title = atx_match.group("title").strip()
                        title = self.inline.convert(title)
                        char = self._title_char(level)
                        output_lines.extend(self._make_title(title, char))
                        pending_setext_title = None
                        i += 1
                        continue

                    # 水平线
                    if HR_PATTERN.match(line):
                        output_lines.append("")
                        output_lines.append("----")
                        output_lines.append("")
                        pending_setext_title = None
                        i += 1
                        continue

                    # 表格
                    if self._is_table_start(lines, i):
                        table_lines, i = self._parse_table(lines, i)
                        output_lines.extend(table_lines)
                        pending_setext_title = None
                        continue

                    # Admonition（须在普通引用块之前）
                    admonition = self._try_admonition(lines, i)
                    if admonition:
                        ad_lines, i = admonition
                        output_lines.extend(ad_lines)
                        pending_setext_title = None
                        continue

                    # 引用块
                    bq_match = BLOCKQUOTE_PATTERN.match(line)
                    if bq_match and bq_match.group("content") is not None:
                        bq_lines, i = self._parse_blockquote(lines, i)
                        output_lines.extend(bq_lines)
                        pending_setext_title = None
                        continue

                    # 任务列表
                    task_match = TASK_LIST_PATTERN.match(line)
                    if task_match:
                        list_lines, i = self._parse_list(lines, i, task=True)
                        output_lines.extend(list_lines)
                        pending_setext_title = None
                        continue

                    # 无序/有序列表
                    if UNORDERED_LIST_PATTERN.match(line) or ORDERED_LIST_PATTERN.match(line):
                        list_lines, i = self._parse_list(lines, i)
                        output_lines.extend(list_lines)
                        pending_setext_title = None
                        continue

                    # RST 块（指令 / 锚点 / 脚注 / 注释等，优先于缩进代码块）
                    if self._is_rst_block_line(line):
                        rst_block = self._parse_rst_block(lines, i)
                        if rst_block:
                            block_lines, i = rst_block
                            output_lines.extend(block_lines)
                            pending_setext_title = None
                            continue

                    # RST 字面量块（段落以 :: 结尾）
                    literal_block = self._parse_rst_literal_block(lines, i)
                    if literal_block:
                        block_lines, i = literal_block
                        output_lines.extend(block_lines)
                        pending_setext_title = None
                        continue

                    # 独立图片行 ![alt](url)
                    img_line = self._parse_standalone_image(line)
                    if img_line:
                        output_lines.extend(img_line)
                        pending_setext_title = None
                        i += 1
                        continue

                    # 缩进代码块
                    if self._is_indented_code_block(lines, i):
                        code_lines, i = self._parse_indented_code(lines, i)
                        output_lines.extend(code_lines)
                        pending_setext_title = None
                        continue

                    # 普通段落
                    para_lines, i = self._parse_paragraph(lines, i)
                    converted = [self.inline.convert(pl) for pl in para_lines]

                    # 检查下一行是否为 Setext 下划线（不前进 i，留给下一轮处理）
                    if i < len(lines):
                        next_setext = SETEXT_UNDERLINE_PATTERN.match(lines[i])
                        if next_setext:
                            pending_setext_title = (converted[0], lines[i])
                            output_lines.append(converted[0])
                            continue

                    output_lines.extend(converted)
                    pending_setext_title = None

                except Exception as exc:
                    msg = f"第 {i + 1} 行处理失败，保留原文: {exc}"
                    self.warnings.append(msg)
                    if self.options.strict:
                        self.warnings.append(f"  原文: {line!r}")
                    output_lines.append(line)
                    i += 1

            # 追加脚注
            footnote_block = self._render_footnotes()
            if footnote_block:
                output_lines.append("")
                output_lines.extend(footnote_block)

            result_text = self._post_process("\n".join(output_lines))
            all_warnings = self.warnings + self.inline.warnings
            return ConversionResult(text=result_text, warnings=all_warnings)

        except Exception as exc:
            self.warnings.append(f"转换过程发生严重错误: {exc}")
            return ConversionResult(text=markdown_text, warnings=self.warnings)

    def _extract_definitions(self, lines: List[str]) -> List[str]:
        """提取参考链接定义和脚注定义，从正文中移除。"""
        result: List[str] = []
        i = 0
        while i < len(lines):
            line = lines[i]

            ref_match = REF_LINK_DEF_PATTERN.match(line)
            if ref_match:
                label = ref_match.group("label")
                url = ref_match.group("url").strip("<>")
                title = (ref_match.group("title") or "").strip("\"'()")
                self.inline.register_ref_link(label, url, title)
                i += 1
                continue

            fn_match = FOOTNOTE_DEF_PATTERN.match(line)
            if fn_match:
                fid = fn_match.group("id")
                content = fn_match.group("content")
                # 支持多行脚注
                cont_lines = [content] if content else []
                i += 1
                while i < len(lines) and lines[i].startswith("    "):
                    cont_lines.append(lines[i][4:])
                    i += 1
                full_content = " ".join(cont_lines).strip()
                self.inline.register_footnote(fid, full_content)
                continue

            result.append(line)
            i += 1
        return result

    def _title_char(self, level: int) -> str:
        idx = (level - 1 + self.options.title_char_offset) % len(RST_TITLE_CHARS)
        return RST_TITLE_CHARS[idx]

    def _make_title(self, title: str, char: str) -> List[str]:
        """生成 RST 标题。"""
        # 中文等宽字符按显示宽度计算，避免下划线短于标题
        width = self._display_width(title)
        underline = char * max(width, 3)
        return [title, underline, ""]

    def _parse_code_fence(
        self, lines: Sequence[str], start: int
    ) -> Tuple[List[str], int]:
        """解析围栏代码块。"""
        opening = lines[start]
        fence_match = FENCE_PATTERN.match(opening)
        if not fence_match:
            return [self.inline.convert(opening)], start + 1

        fence = fence_match.group("fence")
        fence_char = fence[0]
        fence_len = len(fence)
        info = (fence_match.group("info") or "").strip()
        language = info.split()[0] if info else self.options.default_code_language

        code_lines: List[str] = []
        i = start + 1
        while i < len(lines):
            closing_match = FENCE_PATTERN.match(lines[i])
            if closing_match:
                close_fence = closing_match.group("fence")
                if close_fence[0] == fence_char and len(close_fence) >= fence_len:
                    break
            code_lines.append(lines[i])
            i += 1

        i += 1  # 跳过 closing fence

        result: List[str] = [""]
        if language:
            result.append(f".. code-block:: {language}")
        else:
            result.append(".. code-block:: text")
        result.append("")
        for cl in code_lines:
            result.append(f"   {cl}" if cl else "   ")
        result.append("")
        return result, i

    def _parse_fence_as_rst_body(
        self,
        lines: Sequence[str],
        start: int,
        mode: str,
        depth: int,
    ) -> Tuple[List[str], int]:
        """RST 块体内的 MD 围栏：作为缩进正文，避免破坏外层 directive。"""
        opening = lines[start]
        fence_match = FENCE_PATTERN.match(opening)
        if not fence_match:
            return [self._convert_rst_body_line(opening, mode, depth)], start + 1

        fence = fence_match.group("fence")
        fence_char = fence[0]
        fence_len = len(fence)
        body_lines: List[str] = []
        i = start + 1
        while i < len(lines):
            closing_match = FENCE_PATTERN.match(lines[i])
            if closing_match:
                close_fence = closing_match.group("fence")
                if close_fence[0] == fence_char and len(close_fence) >= fence_len:
                    break
            body_lines.append(lines[i])
            i += 1
        i += 1

        result: List[str] = []
        for bl in body_lines:
            if bl.strip():
                converted = self._convert_nested_inline(bl.strip(), depth)
                result.append(f"   {converted}")
            elif result:
                result.append("")
        return result, i

    def _is_table_start(self, lines: Sequence[str], index: int) -> bool:
        if index + 1 >= len(lines):
            return False
        row = lines[index].strip()
        sep = lines[index + 1].strip()
        if not row.startswith("|") or not row.endswith("|"):
            return False
        return bool(TABLE_SEPARATOR_PATTERN.match(sep))

    @staticmethod
    def _split_md_table_cells(line: str) -> List[str]:
        """拆分 Markdown 管道表格行，忽略 `` \\| `` 与行内 `` ` `` 中的 ``|``。"""
        body = line.strip()
        if body.startswith("|"):
            body = body[1:]
        if body.endswith("|"):
            body = body[:-1]

        cells: List[str] = []
        current: List[str] = []
        in_backticks = False
        i = 0
        while i < len(body):
            ch = body[i]
            if ch == "\\" and i + 1 < len(body) and body[i + 1] == "|":
                current.append("|")
                i += 2
                continue
            if ch == "`":
                in_backticks = not in_backticks
                current.append(ch)
                i += 1
                continue
            if ch == "|" and not in_backticks:
                cells.append("".join(current).strip())
                current = []
                i += 1
                continue
            current.append(ch)
            i += 1
        cells.append("".join(current).strip())
        return cells

    @staticmethod
    def _sanitize_grid_cell(cell: str) -> str:
        """Grid table 单元格内 ``|`` 会被当作列分隔符，替换为全角竖线。"""
        if "|" not in cell:
            return cell
        return cell.replace("|", "｜")

    def _parse_table(
        self, lines: Sequence[str], start: int
    ) -> Tuple[List[str], int]:
        """解析 Markdown 表格并转为 RST grid/list table。"""
        rows: List[List[str]] = []
        i = start

        while i < len(lines):
            line = lines[i].strip()
            if not line:
                break
            if TABLE_SEPARATOR_PATTERN.match(line):
                i += 1
                continue
            if not (line.startswith("|") and line.endswith("|")):
                break
            cells = self._split_md_table_cells(line)
            converted_cells = [self.inline.convert(c) for c in cells]
            rows.append(converted_cells)
            i += 1

        if not rows:
            return [lines[start]], start + 1

        if self.options.use_grid_tables:
            table_rst = self._to_grid_table(rows)
        else:
            table_rst = self._to_list_table(rows)

        return table_rst + [""], i

    def _to_grid_table(self, rows: List[List[str]]) -> List[str]:
        """转为 RST grid table。"""
        if not rows:
            return []

        col_count = max(len(r) for r in rows)
        normalized = [r + [""] * (col_count - len(r)) for r in rows]

        # 计算列宽（考虑 unicode 显示宽度）
        widths = [3] * col_count
        for row in normalized:
            for ci, cell in enumerate(row):
                widths[ci] = max(widths[ci], self._display_width(cell))

        def border(sep_left: str, sep_mid: str, sep_right: str, fill: str) -> str:
            parts = [sep_left]
            for w in widths:
                parts.append(fill * (w + 2))
                parts.append(sep_mid)
            return "".join(parts[:-1]) + sep_right

        def format_row(row: List[str]) -> str:
            cells = []
            for ci, cell in enumerate(row):
                safe_cell = self._sanitize_grid_cell(cell)
                pad = widths[ci] - self._display_width(safe_cell)
                cells.append(" " + safe_cell + " " * (pad + 1))
            return "|" + "|".join(cells) + "|"

        result: List[str] = []
        result.append(border("+", "+", "+", "-"))
        result.append(format_row(normalized[0]))
        result.append(border("+", "+", "+", "="))
        for row in normalized[1:]:
            result.append(format_row(row))
            result.append(border("+", "+", "+", "-"))
        return result

    def _infer_list_table_header_rows(self, rows: List[List[str]]) -> int:
        """推断 list-table 的 header-rows（MD 管道表格首行通常为表头）。"""
        if not rows:
            return 0
        return 1 if len(rows) > 1 else 0

    def _resolve_list_table_header_rows(self, rows: List[List[str]]) -> Optional[int]:
        """返回应写入的 header-rows；显式配置优先，None 表示不写该选项。"""
        if self.options.list_table_header_rows is not None:
            return max(0, self.options.list_table_header_rows)
        inferred = self._infer_list_table_header_rows(rows)
        return inferred if inferred > 0 else None

    def _list_table_directive_options(
        self, col_count: int, header_rows: Optional[int]
    ) -> List[str]:
        """生成 list-table 指令选项行（未配置则不输出对应项）。"""
        options: List[str] = []
        if header_rows is not None:
            options.append(f"   :header-rows: {header_rows}")
        if self.options.list_table_widths is not None:
            widths = list(self.options.list_table_widths)
            if len(widths) < col_count:
                widths.extend(["1"] * (col_count - len(widths)))
            elif len(widths) > col_count:
                widths = widths[:col_count]
            options.append(f"   :widths: {' '.join(widths)}")
        return options

    def _to_list_table(self, rows: List[List[str]]) -> List[str]:
        """转为 RST list-table。"""
        if not rows:
            return []

        col_count = max(len(r) for r in rows)
        header_rows = self._resolve_list_table_header_rows(rows)

        result = [".. list-table::"]
        result.extend(self._list_table_directive_options(col_count, header_rows))
        result.append("")

        for row in rows:
            padded = row + [""] * (col_count - len(row))
            result.append("   * - " + "\n     - ".join(padded))
        return result

    @staticmethod
    def _display_width(text: str) -> int:
        """估算文本显示宽度（东亚字符计 2）。"""
        width = 0
        for ch in text:
            if unicodedata.east_asian_width(ch) in ("F", "W"):
                width += 2
            else:
                width += 1
        return width

    def _parse_blockquote(
        self, lines: Sequence[str], start: int
    ) -> Tuple[List[str], int]:
        """解析引用块。"""
        quoted: List[str] = []
        i = start
        while i < len(lines):
            match = BLOCKQUOTE_PATTERN.match(lines[i])
            if not match:
                break
            content = match.group("content")
            quoted.append(content)
            i += 1

        converted = [self.inline.convert(q) for q in quoted]
        result = ["", ".. epigraph::", ""]
        for qline in converted:
            result.append(f"   {qline}")
        result.append("")
        return result, i

    @staticmethod
    def _build_list_tree(
        flat_items: Sequence[Tuple[int, str, bool, bool, int]]
    ) -> List[ListNode]:
        """将扁平列表项按缩进构建为树。"""
        roots: List[ListNode] = []
        stack: List[ListNode] = []

        for indent, content, ordered, is_task, number in flat_items:
            node = ListNode(
                indent=indent,
                content=content,
                ordered=ordered,
                task=is_task,
                number=number,
            )
            while stack and indent <= stack[-1].indent:
                stack.pop()
            if stack:
                stack[-1].children.append(node)
            else:
                roots.append(node)
            stack.append(node)

        return roots

    def _render_list_tree(self, nodes: Sequence[ListNode], depth: int = 0) -> List[str]:
        """递归渲染列表树为 RST（每深一层增加 3 空格缩进）。"""
        lines: List[str] = []
        indent_str = "   " * depth

        for idx, node in enumerate(nodes):
            converted = self.inline.convert(node.content)
            if node.ordered:
                marker = f"{node.number}. "
            else:
                marker = "- "
            lines.append(f"{indent_str}{marker}{converted}")
            if node.children:
                # RST 要求：列表项与子列表之间必须有空行（尤其有序项）
                lines.append("")
                lines.extend(self._render_list_tree(node.children, depth + 1))
                # 含子列表的项与同级下一项之间也需空行
                if idx + 1 < len(nodes):
                    lines.append("")

        return lines

    @staticmethod
    def _peek_next_non_blank(lines: Sequence[str], index: int) -> int:
        """跳过空行，返回下一非空行索引（若无则 len(lines)）。"""
        j = index
        while j < len(lines) and not lines[j].strip():
            j += 1
        return j

    @staticmethod
    def _is_indented_fence_line(line: str) -> bool:
        """判断是否为列表项内缩进的围栏代码块起始行。"""
        match = FENCE_PATTERN.match(line)
        if not match:
            return False
        return bool(match.group("indent"))

    @staticmethod
    def _is_indented_rst_block_line(line: str) -> bool:
        """判断是否为列表项内缩进的 RST 块级 ``..`` 行。"""
        if not line.startswith("  ") and not line.startswith("\t"):
            return False
        return MarkdownToRstConverter._is_rst_block_line(line)

    @staticmethod
    def _dedent_block_lines(lines: Sequence[str], dedent: int) -> List[str]:
        """将块内各行左移指定列数（用于列表项内嵌 RST 块归一化）。"""
        if dedent <= 0:
            return list(lines)
        prefix = " " * dedent
        dedented: List[str] = []
        for line in lines:
            if line.startswith(prefix):
                dedented.append(line[dedent:])
            else:
                dedented.append(line)
        return dedented

    def _parse_list_embedded_rst_block(
        self, lines: Sequence[str], start: int
    ) -> Tuple[List[str], int]:
        """解析列表项内缩进的 RST 块，并归一化到列 0 起始的标准 RST 结构。"""
        match = RST_BLOCK_PATTERN.match(lines[start])
        dedent = len(match.group("indent").expandtabs(4)) if match else 0
        parsed = self._parse_rst_block(lines, start, depth=0)
        if not parsed:
            return [lines[start].rstrip()], start + 1
        block_lines, i = parsed
        if dedent > 0:
            block_lines = self._dedent_block_lines(block_lines, dedent)
        return block_lines, i

    def _list_line_breaks_block(
        self,
        items: Sequence[Tuple[int, str, bool, bool, int]],
        line: str,
    ) -> bool:
        """空行后若遇到不同类型的顶层列表，应断开为独立块（docutils 要求）。"""
        if not items:
            return False
        last_indent, _, last_ordered, last_task, _ = items[-1]
        task_match = TASK_LIST_PATTERN.match(line)
        ul_match = UNORDERED_LIST_PATTERN.match(line)
        ol_match = ORDERED_LIST_PATTERN.match(line)
        if task_match:
            new_indent = len(task_match.group("indent").expandtabs(4))
            new_kind = "task"
        elif ul_match:
            new_indent = len(ul_match.group("indent").expandtabs(4))
            new_kind = "ul"
        elif ol_match:
            new_indent = len(ol_match.group("indent").expandtabs(4))
            new_kind = "ol"
        else:
            return False
        if new_indent > last_indent:
            return False
        last_kind = "task" if last_task else ("ol" if last_ordered else "ul")
        return new_kind != last_kind

    def _parse_list(
        self,
        lines: Sequence[str],
        start: int,
        task: bool = False,
    ) -> Tuple[List[str], int]:
        """解析有序/无序/任务列表。

        列表项内的 MD 围栏代码块输出为独立 ``.. code-block::``，
        不合并进列表文字，也不走行内 `` ` `` 占位符逻辑。
        列表项内缩进的 RST 块（``.. note::``、``.. list-table::`` 等）
        交给 ``_parse_rst_block()`` 递归处理，并归一化缩进。
        """
        items: List[Tuple[int, str, bool, bool, int]] = []
        result: List[str] = [""]
        i = start

        def flush_items() -> None:
            if not items:
                return
            tree = self._build_list_tree(items)
            result.extend(self._render_list_tree(tree))
            items.clear()

        def parse_indented_fence() -> None:
            nonlocal i
            flush_items()
            block_lines, i = self._parse_code_fence(lines, i)
            result.extend(block_lines)

        def parse_indented_rst_block() -> None:
            nonlocal i
            flush_items()
            block_lines, i = self._parse_list_embedded_rst_block(lines, i)
            result.extend(block_lines)

        while i < len(lines):
            line = lines[i]
            if not line.strip():
                nxt = self._peek_next_non_blank(lines, i + 1)
                if nxt >= len(lines):
                    i = nxt
                    break
                if self._is_list_line(lines[nxt]):
                    if items and self._list_line_breaks_block(items, lines[nxt]):
                        break
                    i += 1
                    continue
                if self._is_indented_fence_line(lines[nxt]):
                    i = nxt
                    parse_indented_fence()
                    continue
                if self._is_indented_rst_block_line(lines[nxt]):
                    i = nxt
                    parse_indented_rst_block()
                    continue
                break

            task_match = TASK_LIST_PATTERN.match(line)
            if task_match:
                indent = len(task_match.group("indent").expandtabs(4))
                checked = task_match.group("checked").lower() == "x"
                content = task_match.group("content")
                prefix = "[x] " if checked else "[ ] "
                items.append((indent, prefix + content, False, True, 0))
                i += 1
                continue

            ul_match = UNORDERED_LIST_PATTERN.match(line)
            if ul_match:
                indent = len(ul_match.group("indent").expandtabs(4))
                items.append((indent, ul_match.group("content"), False, False, 0))
                i += 1
                continue

            ol_match = ORDERED_LIST_PATTERN.match(line)
            if ol_match:
                indent = len(ol_match.group("indent").expandtabs(4))
                number = int(ol_match.group("num"))
                items.append((indent, ol_match.group("content"), True, False, number))
                i += 1
                continue

            # 续行（缩进正文）；围栏 / RST 块单独解析
            if items and (line.startswith("  ") or line.startswith("\t")):
                if self._is_indented_fence_line(line):
                    parse_indented_fence()
                    continue
                if self._is_indented_rst_block_line(line):
                    parse_indented_rst_block()
                    continue
                stripped = line.strip()
                last_indent, last_content, ordered, is_task, number = items[-1]
                items[-1] = (
                    last_indent,
                    last_content + " " + stripped,
                    ordered,
                    is_task,
                    number,
                )
                i += 1
                continue
            break

        flush_items()
        result.append("")
        return result, i

    def _parse_standalone_image(self, line: str) -> Optional[List[str]]:
        """独立行的图片。"""
        match = re.match(
            r"^!\[([^\]]*)\]\(([^\s\)\"]+)(?:\s+\"([^\"]*)\")?\)\s*$",
            line.strip(),
        )
        if not match:
            return None

        alt = match.group(1) or ""
        url = match.group(2)
        if url.startswith("./"):
            url = url[2:]
        title = match.group(3) or ""

        result = ["", f".. image:: {url}"]
        if title and alt:
            result.append(f"   :alt: {alt} — {title}")
        elif title:
            result.append(f"   :alt: {title}")
        else:
            result.append(f"   :alt: {alt}")
        result.append("")
        return result

    @staticmethod
    def _is_rst_block_line(line: str) -> bool:
        """判断是否为 RST 块级 ``..`` 行。"""
        match = RST_BLOCK_PATTERN.match(line)
        if not match:
            return False
        body = match.group("body")
        if not body or not body.strip():
            return True
        if body.startswith(" "):
            return True
        stripped = body.lstrip()
        return (
            stripped.startswith("_")
            or stripped.startswith("|")
            or stripped.startswith("[")
            or "::" in stripped
        )

    @staticmethod
    def _rst_body_line(line: str) -> Optional[str]:
        """提取 RST 块体缩进行内容（统一为 3 空格缩进），非缩进行返回 None。"""
        if line.startswith("    "):
            return line[4:].rstrip()
        if line.startswith("   "):
            return line[3:].rstrip()
        if line.startswith("\t"):
            return line.lstrip("\t").rstrip()
        return None

    @staticmethod
    def _line_indent(line: str) -> str:
        return line[: len(line) - len(line.lstrip(" "))]

    @staticmethod
    def _rst_directive_name(body: str) -> str:
        """从 ``..`` 行 body 提取指令名。"""
        stripped = body.strip()
        sub_match = re.match(r"\|[^|]+\|\s+([\w-]+)::", stripped)
        if sub_match:
            return sub_match.group(1).lower()
        name_match = re.match(r"([\w-]+)::", stripped)
        if name_match:
            return name_match.group(1).lower()
        return ""

    @classmethod
    def _rst_block_convert_mode(cls, body: str) -> str:
        """判断 RST 块体的 MD 转换模式。"""
        stripped = body.strip()
        if not stripped:
            return "comment"
        if stripped.startswith("_") and ":" in stripped and not stripped.endswith("::"):
            return "structural"
        if stripped.startswith("[") or stripped.startswith("|"):
            return "structural"
        name = cls._rst_directive_name(body)
        if name in RST_CODE_DIRECTIVES:
            return "code"
        if name == "list-table":
            return "list-table"
        if name:
            return "prose"
        return "structural"

    def _convert_nested_inline(self, text: str, depth: int) -> str:
        """转换嵌套层中的 Markdown 行内语法。"""
        if not text:
            return text
        if depth >= MAX_RST_NEST_DEPTH:
            self.warnings.append(f"RST 嵌套深度超过 {MAX_RST_NEST_DEPTH}，保留原文")
            return text
        return self.inline.convert(text)

    def _convert_rst_opener_line(self, line: str, depth: int) -> str:
        """转换 ``..`` 起始行中可能嵌套的 Markdown。"""
        match = RST_BLOCK_PATTERN.match(line)
        if not match:
            return line.rstrip()

        indent = match.group("indent")
        body = match.group("body")

        for pattern in (RST_FOOTNOTE_OPENER, RST_SUBSTITUTION_OPENER, RST_HYPERLINK_OPENER):
            body_match = pattern.match(body)
            if body_match:
                prefix, content = body_match.groups()
                if content.strip():
                    converted = self._convert_nested_inline(content, depth)
                    return f"{indent}..{prefix}{converted}"
                return line.rstrip()

        return line.rstrip()

    def _convert_rst_body_line(self, line: str, mode: str, depth: int) -> str:
        """转换 RST 块体单行：保留结构，转换其中嵌套的 Markdown。"""
        if mode == "code":
            return line.rstrip()

        indent = self._line_indent(line)
        stripped = line.lstrip(" ")

        if mode == "list-table":
            row_match = RST_LIST_TABLE_ROW.match(stripped)
            if row_match:
                prefix, spacing, cell = row_match.groups()
                return indent + prefix + spacing + self._convert_nested_inline(cell, depth)

        opt_match = RST_DIRECTIVE_OPTION.match(stripped)
        if opt_match:
            key, spacing, value = opt_match.groups()
            if value:
                return indent + key + spacing + self._convert_nested_inline(value, depth)
            return line.rstrip()

        if mode in ("prose", "list-table", "structural"):
            return indent + self._convert_nested_inline(stripped, depth)

        return line.rstrip()

    @staticmethod
    def _is_directive_option_line(line: str) -> bool:
        return bool(RST_DIRECTIVE_OPTION.match(line.lstrip(" ")))

    @staticmethod
    def _is_list_table_row_line(line: str) -> bool:
        return bool(RST_LIST_TABLE_ROW.match(line.lstrip(" ")))

    def _should_skip_blank_in_rst_block(
        self,
        mode: str,
        result: List[str],
        next_line: str,
    ) -> bool:
        """list-table 中空行会破坏 bullet list 或把 option 误判为 content。"""
        if mode != "list-table":
            return False

        if self._is_directive_option_line(next_line):
            if not result:
                return True
            prev = result[-1]
            if prev.strip().startswith(".."):
                return True
            if self._is_directive_option_line(prev):
                return True

        if result:
            prev = result[-1]
            if self._is_list_table_row_line(prev) and self._is_list_table_row_line(next_line):
                return True
            if prev.lstrip(" ").startswith("- ") and self._is_list_table_row_line(next_line):
                return True

        return False

    def _ensure_blank_before_list_table_rows(self, result: List[str]) -> None:
        """option 与 bullet list 之间保留 exactly one 空行。"""
        if not result:
            return
        prev = result[-1]
        if not self._is_directive_option_line(prev):
            return
        if len(result) >= 2 and result[-2] == "":
            return
        result.append("")

    def _parse_rst_block(
        self, lines: Sequence[str], start: int, depth: int = 0
    ) -> Optional[Tuple[List[str], int]]:
        """解析 Markdown 中嵌入的 RST 块，并递归处理多层嵌套及块内 MD。"""
        if depth >= MAX_RST_NEST_DEPTH:
            self.warnings.append(
                f"第 {start + 1} 行：RST 嵌套深度超过 {MAX_RST_NEST_DEPTH}，保留原文"
            )
            return None

        if not self._is_rst_block_line(lines[start]):
            return None

        match = RST_BLOCK_PATTERN.match(lines[start])
        if not match:
            return None

        body = match.group("body").rstrip()
        mode = self._rst_block_convert_mode(body)
        opener = self._convert_rst_opener_line(lines[start], depth)
        result: List[str] = [opener]
        i = start + 1

        while i < len(lines):
            line = lines[i]

            if not line.strip():
                if i + 1 < len(lines):
                    nxt = lines[i + 1]
                    if self._is_rst_block_line(nxt):
                        break
                    if self._rst_body_line(nxt) is not None:
                        if self._should_skip_blank_in_rst_block(mode, result, nxt):
                            i += 1
                            continue
                        result.append("")
                        i += 1
                        continue
                if len(result) > 1:
                    break
                if not self._should_skip_blank_in_rst_block(mode, result, ""):
                    result.append("")
                i += 1
                continue

            if self._is_rst_block_line(line):
                nested = self._parse_rst_block(lines, i, depth + 1)
                if nested:
                    nested_lines, i = nested
                    if len(result) == 1:
                        result.append("")
                    result.extend(nested_lines)
                    continue
                break

            if mode != "code" and FENCE_PATTERN.match(line):
                if len(result) == 1:
                    result.append("")
                fence_lines, i = self._parse_fence_as_rst_body(lines, i, mode, depth)
                result.extend(fence_lines)
                continue

            body_line = self._rst_body_line(line)
            if body_line is not None:
                if mode == "list-table" and self._is_list_table_row_line(line):
                    self._ensure_blank_before_list_table_rows(result)
                elif len(result) == 1 and mode != "list-table":
                    result.append("")
                elif (
                    len(result) == 1
                    and mode == "list-table"
                    and not self._is_directive_option_line(line)
                ):
                    result.append("")
                result.append(self._convert_rst_body_line(line, mode, depth))
                i += 1
                continue
            break

        if mode == "list-table" and result and result[-1] != "":
            result.append("")
        elif mode != "list-table":
            result.append("")
        return result, i

    def _parse_rst_literal_block(
        self, lines: Sequence[str], start: int
    ) -> Optional[Tuple[List[str], int]]:
        """解析 RST 字面量块（段落以 :: 结尾，后跟缩进内容）。"""
        line = lines[start]
        if self._is_rst_block_line(line):
            return None
        if not RST_LITERAL_MARKER.search(line.rstrip()):
            return None

        j = start + 1
        while j < len(lines) and not lines[j].strip():
            j += 1
        if j >= len(lines) or self._rst_body_line(lines[j]) is None:
            return None

        result = [line.rstrip(), ""]
        i = start + 1
        while i < len(lines):
            if not lines[i].strip():
                if i + 1 < len(lines) and self._rst_body_line(lines[i + 1]) is not None:
                    result.append("")
                    i += 1
                    continue
                break
            body_line = self._rst_body_line(lines[i])
            if body_line is None:
                break
            result.append(lines[i].rstrip())
            i += 1

        result.append("")
        return result, i

    def _parse_rst_directive(
        self, lines: Sequence[str], start: int
    ) -> Optional[Tuple[List[str], int]]:
        """兼容旧接口，委托给 _parse_rst_block。"""
        return self._parse_rst_block(lines, start)

    def _is_indented_code_block(self, lines: Sequence[str], index: int) -> bool:
        if index >= len(lines):
            return False
        line = lines[index]
        if not INDENTED_CODE_PATTERN.match(line):
            return False
        # RST 块后的缩进内容是块体，不是 MD 代码块
        prev = index - 1
        while prev >= 0 and not lines[prev].strip():
            prev -= 1
        if prev >= 0:
            if self._is_rst_block_line(lines[prev]):
                return False
            if RST_LITERAL_MARKER.search(lines[prev].rstrip()):
                return False
        if index == 0:
            return True
        return not lines[index - 1].strip()

    def _parse_indented_code(
        self, lines: Sequence[str], start: int
    ) -> Tuple[List[str], int]:
        code_lines: List[str] = []
        i = start
        while i < len(lines):
            match = INDENTED_CODE_PATTERN.match(lines[i])
            if not match:
                if not lines[i].strip():
                    break
                if not lines[i].startswith("    ") and not lines[i].startswith("\t"):
                    break
                match = INDENTED_CODE_PATTERN.match(lines[i])
                if not match:
                    break
            code_lines.append(match.group("code") if match else lines[i].strip())
            i += 1

        result = ["", ".. code-block:: text", ""]
        for cl in code_lines:
            result.append(f"   {cl}")
        result.append("")
        return result, i

    def _try_admonition(
        self, lines: Sequence[str], start: int
    ) -> Optional[Tuple[List[str], int]]:
        """尝试解析 admonition（> **Note:** 或 ::: warning）。"""
        line = lines[start].strip()

        # GitHub 风格 ::: note
        directive_match = re.match(r"^:::\s*(\w+)\s*(.*)$", line)
        if directive_match:
            kind = directive_match.group(1).lower()
            rst_kind = self._map_admonition(kind)
            body_lines: List[str] = []
            i = start + 1
            while i < len(lines):
                if lines[i].strip() == ":::":
                    break
                body_lines.append(lines[i])
                i += 1
            i += 1
            converted = [self.inline.convert(bl) for bl in body_lines if bl.strip()]
            result = [f".. {rst_kind}::", ""]
            result.extend(f"   {bl}" for bl in converted)
            result.append("")
            return result, i

        # 引用块中的 **Note:** 形式
        bq_match = BLOCKQUOTE_PATTERN.match(lines[start])
        if bq_match:
            content = bq_match.group("content")
            admon_match = re.match(
                r"^\*\*(Note|Warning|Tip|Important|Caution|Attention|Hint|Error):\*\*\s*(.*)$",
                content,
                re.IGNORECASE,
            )
            if admon_match:
                kind = self._map_admonition(admon_match.group(1))
                first_line = admon_match.group(2)
                body = [first_line] if first_line else []
                i = start + 1
                while i < len(lines):
                    m = BLOCKQUOTE_PATTERN.match(lines[i])
                    if not m:
                        break
                    body.append(m.group("content"))
                    i += 1
                converted = [self.inline.convert(b) for b in body if b.strip()]
                result = [f".. {kind}::", ""]
                result.extend(f"   {bl}" for bl in converted)
                result.append("")
                return result, i

        return None

    @staticmethod
    def _map_admonition(kind: str) -> str:
        mapping = {
            "note": "note",
            "info": "note",
            "tip": "tip",
            "hint": "tip",
            "important": "important",
            "warning": "warning",
            "caution": "caution",
            "attention": "caution",
            "error": "error",
            "danger": "error",
        }
        return mapping.get(kind.lower(), "note")

    def _is_list_line(self, line: str) -> bool:
        return bool(
            UNORDERED_LIST_PATTERN.match(line)
            or ORDERED_LIST_PATTERN.match(line)
            or TASK_LIST_PATTERN.match(line)
        )

    def _parse_paragraph(
        self, lines: Sequence[str], start: int
    ) -> Tuple[List[str], int]:
        """解析普通段落（直到空行或块级元素）。"""
        para: List[str] = [lines[start]]
        i = start + 1
        while i < len(lines):
            line = lines[i]
            if not line.strip():
                break
            if self._is_block_start(lines, i):
                break
            para.append(line.strip())
            i += 1
        # 合并软换行
        merged = " ".join(para)
        return [merged], i

    def _is_block_start(self, lines: Sequence[str], index: int) -> bool:
        line = lines[index]
        checks: List[Callable[[], bool]] = [
            lambda: bool(FENCE_PATTERN.match(line)),
            lambda: bool(ATX_HEADER_PATTERN.match(line)),
            lambda: bool(SETEXT_UNDERLINE_PATTERN.match(line)),
            lambda: bool(HR_PATTERN.match(line)),
            lambda: self._is_table_start(lines, index),
            lambda: bool(RST_BLOCK_PATTERN.match(line)) and self._is_rst_block_line(line),
            lambda: bool(BLOCKQUOTE_PATTERN.match(line)),
            lambda: self._is_list_line(line),
            lambda: bool(self._parse_standalone_image(line)),
            lambda: self._is_indented_code_block(lines, index),
        ]
        return any(c() for c in checks)

    def _render_footnotes(self) -> List[str]:
        if not self.inline._footnotes:
            return []
        result: List[str] = []
        for fid, content in self.inline._footnotes.items():
            converted = self.inline.convert(content)
            result.append(f".. [#{fid}] {converted}")
        return result

    def _post_process(self, text: str) -> str:
        """后处理：清理多余空行等。"""
        # 最多保留两个连续空行
        text = re.sub(r"\n{4,}", "\n\n\n", text)
        # 去除文件首尾空白
        text = text.strip() + "\n"
        if self.options.wrap_width > 0:
            text = self._wrap_paragraphs(text, self.options.wrap_width)
        return text

    @staticmethod
    def _wrap_paragraphs(text: str, width: int) -> str:
        """对普通段落自动换行（不影响指令块和代码块）。"""
        lines = text.split("\n")
        result: List[str] = []
        in_block = False

        for line in lines:
            if line.startswith(".. ") or line.startswith("   ") or line.startswith("|"):
                in_block = line.startswith("   ") or line.startswith("|")
                result.append(line)
                continue
            if not line.strip():
                in_block = False
                result.append(line)
                continue
            if not in_block and len(line) > width:
                result.append(textwrap.fill(line, width=width))
            else:
                result.append(line)
        return "\n".join(result)


def validate_rst_with_docutils(
    rst_text: str,
    source_path: str = "<string>",
    *,
    max_level: int = 3,
) -> List[str]:
    """用 docutils 解析 RST，返回 ERROR/WARNING 消息（level >= 2）。"""
    try:
        from docutils.core import publish_doctree
    except ImportError:
        return ["docutils 未安装，跳过校验"]

    from io import StringIO

    stream = StringIO()
    publish_doctree(
        rst_text,
        source_path=source_path,
        settings_overrides={
            "warning_stream": stream,
            "report_level": max_level,
        },
    )
    return [line for line in stream.getvalue().splitlines() if line.strip()]


def convert_markdown_to_rst(
    markdown_text: str,
    *,
    default_code_language: str = "",
    wrap_width: int = 0,
    strict: bool = False,
    preserve_html: bool = True,
    use_grid_tables: bool = True,
    list_table_header_rows: Optional[int] = None,
    list_table_widths: Optional[Sequence[str]] = None,
) -> str:
    """
    便捷函数：将 Markdown 字符串转换为 RST 字符串。

    参数:
        markdown_text: Markdown 源文本
        default_code_language: 无语言标识的代码块默认语言
        wrap_width: 段落换行宽度，0 表示不换行
        strict: 严格模式，记录更多警告
        preserve_html: 是否保留 HTML 标签
        use_grid_tables: True 使用 grid table，False 使用 list-table
        list_table_header_rows: list-table 表头行数，None 则按 MD 表格结构自动推断
        list_table_widths: list-table 列宽，None 则不输出 :widths: 选项

    返回:
        RST 格式字符串
    """
    widths_tuple = tuple(list_table_widths) if list_table_widths else None
    options = ConversionOptions(
        default_code_language=default_code_language,
        wrap_width=wrap_width,
        strict=strict,
        preserve_html=preserve_html,
        use_grid_tables=use_grid_tables,
        list_table_header_rows=list_table_header_rows,
        list_table_widths=widths_tuple,
    )
    converter = MarkdownToRstConverter(options)
    result = converter.convert(markdown_text)
    return result.text


def convert_file(
    input_path: str,
    output_path: Optional[str] = None,
    encoding: str = "utf-8",
    **kwargs,
) -> ConversionResult:
    """从文件读取 Markdown 并转换，可选写入输出文件。"""
    try:
        with open(input_path, "r", encoding=encoding) as f:
            source = f.read()
    except UnicodeDecodeError:
        with open(input_path, "r", encoding="gbk", errors="replace") as f:
            source = f.read()

    options = ConversionOptions(
        default_code_language=kwargs.get("default_code_language", ""),
        wrap_width=kwargs.get("wrap_width", 0),
        strict=kwargs.get("strict", False),
        preserve_html=kwargs.get("preserve_html", True),
        use_grid_tables=kwargs.get("use_grid_tables", True),
        list_table_header_rows=kwargs.get("list_table_header_rows"),
        list_table_widths=(
            tuple(kwargs["list_table_widths"])
            if kwargs.get("list_table_widths")
            else None
        ),
    )
    converter = MarkdownToRstConverter(options)
    result = converter.convert(source)

    if output_path:
        with open(output_path, "w", encoding=encoding, newline="\n") as f:
            f.write(result.text)

    return result



# ---------------------------------------------------------------------------
# 文档构建
# ---------------------------------------------------------------------------

def _run_md2rst(src_file_path, dst_file_path):
    try:
        result = convert_file(src_file_path, dst_file_path)
    except Exception as exc:
        raise RuntimeError(f"md2rst failed for {src_file_path}: {exc}") from exc
    for warning in result.warnings:
        print(warning)

def _cjk_display_width(text: str) -> int:
    """Return reST title underline width (wide/fullwidth chars count as 2)."""
    import unicodedata
    width = 0
    for ch in text:
        if unicodedata.east_asian_width(ch) in ('W', 'F'):
            width += 2
        else:
            width += 1
    return width + 4

def translate_md2rst(src_path, dst_path, lan):
    src_file = ""
    dst_file = ""

    if lan == 'en':
        src_file = "README.md"
        dst_file = "index.rst"

    elif lan == 'zh_CN':
        src_file = "README_CN.md"
        dst_file = "index.rst"
    else:
        return

    # 检查源文件是否存在
    if not os.path.isfile(os.path.join(src_path, src_file)):
        return

    src_file_path = os.path.join(src_path, src_file)
    dst_file_path = os.path.join(dst_path, dst_file)
    os.makedirs(dst_path, exist_ok=True)

    _run_md2rst(src_file_path, dst_file_path)

def write_projects_index(dst_path, title, entries):
    index_path = os.path.join(dst_path, "index.rst")
    if os.path.exists(index_path):
        return

    normalized_entries = sorted(set(entries))
    if not normalized_entries:
        return

    lines = [
        title,
        "=" * _cjk_display_width(title),
        "",
        ".. toctree::",
        "   :maxdepth: 1",
        "",
    ]
    for entry in normalized_entries:
        lines.append(f"   {entry}/index")
    lines.append("")

    with open(index_path, "w", encoding="utf-8") as index_file:
        index_file.write("\n".join(lines))

def get_projects_index_title(src_path, lan):
    dirname = os.path.basename(src_path)
    if dirname == "projects":
        return "Projects" if lan == "en" else "项目示例"

    title = dirname.replace("_", " ").replace("-", " ")
    return title.title() if lan == "en" else title

def run_cmd(cmd):
    p = subprocess.Popen(cmd, shell=True)
    p.wait()
    return p

MARKDOWN_IMAGE_REF_PATTERN = re.compile(
    r"!\[[^\]]*\]\(([^\s\)\"]+)(?:\s+\"[^\"]*\")?\)"
)
RST_IMAGE_REF_PATTERN = re.compile(r"^\.\.\s+image::\s+(\S+)")

def _normalize_doc_asset_path(path: str) -> str:
    if path.startswith("./"):
        return path[2:]
    return path

def _is_local_doc_asset(path: str) -> bool:
    if not path or path.startswith(("http://", "https://", "data:", "/")):
        return False
    normalized = _normalize_doc_asset_path(path).replace("\\", "/")
    return ".." not in normalized.split("/")

def _collect_referenced_doc_assets(readme_path: str, rst_path: str) -> List[str]:
    asset_paths = set()

    if os.path.isfile(readme_path):
        with open(readme_path, "r", encoding="utf-8") as readme_file:
            readme_text = readme_file.read()
        for match in MARKDOWN_IMAGE_REF_PATTERN.finditer(readme_text):
            url = match.group(1)
            if _is_local_doc_asset(url):
                asset_paths.add(_normalize_doc_asset_path(url))

    if os.path.isfile(rst_path):
        with open(rst_path, "r", encoding="utf-8") as rst_file:
            rst_text = rst_file.read()
        for line in rst_text.splitlines():
            match = RST_IMAGE_REF_PATTERN.match(line.strip())
            if not match:
                continue
            url = match.group(1)
            if _is_local_doc_asset(url):
                asset_paths.add(_normalize_doc_asset_path(url))

    return sorted(asset_paths)

def _common_static_dir(doc_lan_dir: str) -> str:
    """Return ap/docs/common/_static from a language doc dir (e.g. ap/docs/bk7259/zh_CN)."""
    ap_docs_root = os.path.abspath(os.path.join(doc_lan_dir, "..", ".."))
    return os.path.join(ap_docs_root, "common", "_static")

def _static_image_ref(dst_path: str, static_dir: str, image_basename: str) -> str:
    rel_static = os.path.relpath(static_dir, dst_path).replace("\\", "/")
    return f"{rel_static}/{image_basename}"

def _rewrite_rst_image_paths(rst_path: str, path_rewrites: dict) -> None:
    if not os.path.isfile(rst_path) or not path_rewrites:
        return

    with open(rst_path, "r", encoding="utf-8") as rst_file:
        lines = rst_file.readlines()

    new_lines = []
    for line in lines:
        match = RST_IMAGE_REF_PATTERN.match(line.strip())
        if match:
            old_path = match.group(1)
            normalized = _normalize_doc_asset_path(old_path)
            new_path = path_rewrites.get(normalized) or path_rewrites.get(old_path)
            if new_path:
                indent = line[: len(line) - len(line.lstrip())]
                new_lines.append(f"{indent}.. image:: {new_path}\n")
                continue
        new_lines.append(line)

    with open(rst_path, "w", encoding="utf-8") as rst_file:
        rst_file.writelines(new_lines)

def _files_are_identical(path_a: str, path_b: str) -> bool:
    """Return True when two files have the same content."""
    return filecmp.cmp(path_a, path_b, shallow=False)

def _pick_static_image_basename(
    static_dir: str,
    src_file: str,
    image_basename: str,
    project_name: str,
) -> str:
    """Pick a unique basename under static_dir, reusing or renaming on collision."""
    stem, ext = os.path.splitext(image_basename)
    candidates = [image_basename, f"{project_name}_{image_basename}"]

    suffix = 2
    while len(candidates) < 128:
        candidates.append(f"{project_name}_{stem}_{suffix}{ext}")
        suffix += 1

    for candidate in candidates:
        dst_file = os.path.join(static_dir, candidate)
        if not os.path.isfile(dst_file):
            return candidate
        if _files_are_identical(src_file, dst_file):
            return candidate

    raise RuntimeError(
        f"unable to find unused static image name for {src_file} under {static_dir}"
    )

def _copy_referenced_doc_assets(src_path, dst_path, readme_path, rst_path, static_dir=None):
    """Copy only image assets referenced by README / generated RST."""
    asset_paths = _collect_referenced_doc_assets(readme_path, rst_path)
    path_rewrites = {}
    project_name = os.path.basename(os.path.normpath(src_path))

    for rel_path in asset_paths:
        src_file = os.path.join(src_path, rel_path)
        if not os.path.isfile(src_file):
            print(f"warning: referenced doc asset not found: {src_file}")
            continue

        image_basename = os.path.basename(rel_path)
        if static_dir:
            os.makedirs(static_dir, exist_ok=True)
            image_basename = _pick_static_image_basename(
                static_dir, src_file, image_basename, project_name
            )
            dst_file = os.path.join(static_dir, image_basename)
            if not os.path.isfile(dst_file) or not _files_are_identical(src_file, dst_file):
                shutil.copy2(src_file, dst_file)
            new_ref = _static_image_ref(dst_path, static_dir, image_basename)
            path_rewrites[_normalize_doc_asset_path(rel_path)] = new_ref
        else:
            dst_file = os.path.join(dst_path, rel_path)
            os.makedirs(os.path.dirname(dst_file), exist_ok=True)
            shutil.copy2(src_file, dst_file)

    if static_dir:
        _rewrite_rst_image_paths(rst_path, path_rewrites)

def copy_projects_doc(src_path, dst_path, lan, static_dir=None):
    print(f"copy_projects_doc: {src_path} -> {dst_path}")
    if not os.path.isdir(src_path):
        return 0

    child_doc_dirs = []
    for item in sorted(os.listdir(src_path)):
        if item == '.git':
            continue
        item_path = os.path.join(src_path, item)
        item_dst_path = os.path.join(dst_path, item)
        if os.path.isdir(item_path):
            if copy_projects_doc(item_path, item_dst_path, lan, static_dir):
                child_doc_dirs.append(item)

    readme_name = "README.md" if lan == 'en' else "README_CN.md"
    readme_path = os.path.join(src_path, readme_name)
    projects_rst_path = os.path.join(src_path, "projects.rst")
    has_local_doc = os.path.isfile(readme_path) or os.path.isfile(projects_rst_path)

    if not has_local_doc and not child_doc_dirs:
        return 0

    run_cmd(f'mkdir -p {dst_path}')
    if os.path.isfile(readme_path):
        translate_md2rst(src_path, dst_path, lan)
        _copy_referenced_doc_assets(
            src_path,
            dst_path,
            readme_path,
            os.path.join(dst_path, "index.rst"),
            static_dir,
        )
    elif os.path.isfile(projects_rst_path):
        shutil.copyfile(projects_rst_path, os.path.join(dst_path, "index.rst"))

    write_projects_index(dst_path, get_projects_index_title(src_path, lan), child_doc_dirs)
    return 1

def build_lan_doc(doc_path, target, lan):
    # 无论路径是否包含ap/docs，都确保lan_dir被定义
    lan_dir = f'{doc_path}/{lan}'

    if "ap/docs" in doc_path:
        print("cp/docs not support")
        print(f"doc_path: {doc_path}")
        print(f"target: {target}")
        print(f"lan: {lan}")
        armino_path = os.getenv('ARMINO_PATH')
        print(f"armino_path: {armino_path}")
        print(f"lan_dir: {lan_dir}")
        run_cmd(f'rm -rf {lan_dir}/examples/projects')
        if target in ('bk7236', 'bk7258', 'bk7259'):
            static_dir = _common_static_dir(lan_dir)
            copy_projects_doc(
                f'{lan_dir}/../../../../projects',
                f'{lan_dir}/examples/projects',
                lan,
                static_dir,
            )

    # clean build space (use absolute paths; no chdir so zh/en can build in parallel)
    run_cmd(f'rm -rf {doc_path}/{lan}/_build')
    run_cmd(f'rm -rf {doc_path}/{lan}/xml')
    run_cmd(f'rm -rf {doc_path}/{lan}/xml_in')
    run_cmd(f'rm -rf {doc_path}/{lan}/man')
    run_cmd(f'rm -rf {doc_path}/{lan}/__pycache__')

    p = run_cmd(f'make -C {lan_dir} arminodocs -j8')
    if p.returncode:
        print("make doc failed!")
        raise RuntimeError(f"make arminodocs failed for {lan} ({lan_dir})")
    run_cmd(f'mkdir -p {doc_path}/build/{lan}')
    run_cmd(f'cp -r {lan_dir}/_build/* {doc_path}/build/{lan}/')

def build_with_target(clean, target, doc_build_path):
    cur_dir_is_docs_dir = True
    saved_dir = os.getcwd()
    if 'ARMINO_PATH' in os.environ:
        armino_path = os.getenv('ARMINO_PATH')
        DOCS_PATH = f"{armino_path}/docs/{target}"
        cur_path = os.getcwd()
        if cur_path != DOCS_PATH:
            cur_dir_is_docs_dir = False
        print(f'DOCS_PATH set to {DOCS_PATH}')
    else:
        #print('ARMINO_PATH env is not set, set DOCS_PATH to current dir')
        DOCS_PATH = f"{os.getcwd()}/docs/{target}"

    build_dir = doc_build_path
    if (clean):
        run_cmd(f'rm -rf {build_dir}')
        run_cmd(f'rm -rf {DOCS_PATH}/en/_build')
        run_cmd(f'rm -rf {DOCS_PATH}/en/xml')
        run_cmd(f'rm -rf {DOCS_PATH}/en/xml_in')
        run_cmd(f'rm -rf {DOCS_PATH}/en/man')
        run_cmd(f'rm -rf {DOCS_PATH}/zh_CN/_build')
        run_cmd(f'rm -rf {DOCS_PATH}/zh_CN/xml')
        run_cmd(f'rm -rf {DOCS_PATH}/zh_CN/xml_in')
        run_cmd(f'rm -rf {DOCS_PATH}/zh_CN/man')
        run_cmd(f'rm -rf {DOCS_PATH}/__pycache__')
        if (target == 'bk7259'):
            run_cmd(f'rm -rf {DOCS_PATH}/en/projects')
            run_cmd(f'rm -rf {DOCS_PATH}/zh_CN/projects')
        return

    if not os.path.exists(build_dir):
        run_cmd(f'mkdir -p {build_dir}')

    build_lan_doc(DOCS_PATH, target, 'zh_CN')
    build_lan_doc(DOCS_PATH, target, 'en')

    if cur_dir_is_docs_dir == False:
        run_cmd(f'rm -rf {build_dir}/{target}')
        run_cmd(f'cp -rf {DOCS_PATH}/build/ {build_dir}/{target}')
        run_cmd(f'rm -rf {build_dir}/{target}/*/inc')

    os.chdir(saved_dir)

def build_doc_internal(clean, target):
    if 'ARMINO_AVDK_DIR' in os.environ:
        sdk_path = os.getenv('ARMINO_AVDK_DIR')
    else:
        raise RuntimeError("not find env ARMINO_AVDK_DIR")

    if 'ARMINO_DIR' in os.environ:
        armino_path = os.getenv('ARMINO_DIR')
    else:
        raise RuntimeError("not find env ARMINO_DIR")

    sub_doc_name = os.path.basename(os.getcwd())
    doc_build_path = sdk_path + f"/build/doc/{sub_doc_name}_doc"
    if not os.path.exists(doc_build_path):
        os.makedirs(doc_build_path)

    if (target == "all"):
        build_with_target(clean, "bk7259", doc_build_path)
    else:
        build_with_target(clean, target, doc_build_path)

    run_cmd(f'cp {armino_path}/docs/version.json {doc_build_path}/version.json')

def build_doc(target):
    build_doc_internal(False, target)

def main(argv):
    if (len(argv) > 1 and argv[1] == "clean"):
        target = "all"
        if (len(argv) == 3):
            target = sys.argv[2]
        build_doc_internal(True, target)
    else:
        build_doc_internal(False, "all")

if __name__ == "__main__":
    main(sys.argv)
