#!/usr/bin/env python3

import os
import shutil
import subprocess
import sys
import re
from typing import Match

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

class MarkdownToRST:
    """Markdown → RST 转换器"""

    def __init__(self, src_path=None, dst_path=None):
        """初始化转换器"""
        self.src_path = src_path  # 源文件路径
        self.dst_path = dst_path  # 目标文件路径

    def convert_file(self, src_file, dst_file):
        """将指定的Markdown文件转换为RST文件"""
        # 构建完整的文件路径
        src_file_path = os.path.join(self.src_path, src_file) if self.src_path else src_file
        dst_file_path = os.path.join(self.dst_path, dst_file) if self.dst_path else dst_file

        # 确保目标目录存在
        if self.dst_path:
            os.makedirs(self.dst_path, exist_ok=True)

        # 读取源文件
        with open(src_file_path, "r", encoding="utf-8") as f:
            md_text = f.read()

        # 转换内容
        rst_text = self.convert(md_text)

        # 写入目标文件
        with open(dst_file_path, "w", encoding="utf-8") as f:
            f.write(rst_text)

    def convert(self, text: str) -> str:
        """将 Markdown 文本转换为 reStructuredText (RST)"""
        # 首先清理文本，移除可能导致问题的内容
        text = self._clean_text_before_conversion(text)
        text = self._fix_rst_warning_blocks(text)
        text = self._convert_backtick_links(text)
        # 代码块须在表格转换之前，避免 ``` 内的 | 行被误判为 Markdown 表格
        text = self._convert_codeblocks(text)
        text = self._convert_markdown_tables(text)

        # 然后按行处理其他格式
        lines = text.splitlines()
        processed_lines = []
        in_codeblock = False

        for line in lines:
            stripped = line.strip()
            if stripped.startswith('.. code-block::'):
                in_codeblock = True
                processed_lines.append(line)
                continue
            if in_codeblock:
                processed_lines.append(line)
                if stripped == '':
                    in_codeblock = False
                continue
            # 检查是否已经包含reST语法
            if self._contains_rst_syntax(line):
                processed_lines.append(line)
            else:
                processed_lines.append(self._convert_markdown_line(line))

        # 将处理后的行重新组合为文本
        text = '\n'.join(processed_lines)
        text = self._fix_nested_list_indent(text)
        text = self._ensure_blank_before_directives(text)
        return self._ensure_document_title(text)

    def _convert_markdown_line(self, line: str) -> str:
        """Markdown 行内格式转换（反引号区间内不做 emphasis 转换）。"""
        if ':link_to_translation:' in line:
            line = self._convert_link_to_translation(line)
            line = self._convert_unordered_list(line)
            return line
        line = self._normalize_md_emphasis_in_backticks(line)
        parts = line.split('`')
        for i in range(0, len(parts), 2):
            segment = parts[i]
            segment = self._convert_bold(segment)
            segment = self._convert_italic(segment)
            segment = self._convert_headings(segment)
            segment = self._convert_images(segment)
            segment = self._convert_link_to_translation(segment)
            segment = self._convert_links(segment)
            segment = self._convert_unordered_list(segment)
            segment = self._convert_ordered_list(segment)
            parts[i] = segment
        for i in range(1, len(parts), 2):
            parts[i] = re.sub(r'\*\*', '', parts[i])
        line = '`'.join(parts)
        line = self._convert_inline_code(line)
        line = self._sanitize_rst_line(line)
        return line

    def _normalize_md_emphasis_in_backticks(self, line: str) -> str:
        """README 中 `` `**bold**` `` / `` `**text` `` 等混用统一为普通反引号内容。"""
        line = re.sub(r'`\[([^\]]+)\]\(([^)]+)\)`', r'[\1](\2)', line)
        line = re.sub(r'`\*\*(.+?)\*\*`', r'`\1`', line)
        line = re.sub(r'`\*\*([^*`]+)`', r'`\1`', line)
        line = re.sub(r'`([^*`]+)\*\*`', r'`\1`', line)
        return line

    def _sanitize_rst_line(self, line: str) -> str:
        """修正 README 转 RST 后易触发 docutils 警告的 inline markup。"""
        line = re.sub(r'\*\*(`[^`]+`)\*\*', r'\1', line)
        line = re.sub(r'\*\*(``[^`]+``)\*\*', r'\1', line)
        line = re.sub(r'(``[^`]+``)\*\*', r'\1', line)
        line = re.sub(r'>`_（', r'>`_ （', line)
        line = re.sub(r'(``[^`]+``)（', r'\1 （', line)
        line = re.sub(r'\*\*(\.\w+)\*\*', r'``\1``', line)
        line = re.sub(r'\*\*([^*`]+_[^*`]+)\*\*', r'``\1``', line)
        # 项目 README 的 **bold** 在 RST 中易与 （、` 等混用出错，去掉强调标记保留正文
        line = re.sub(r'\*\*([^*]+)\*\*', r'\1', line)
        line = re.sub(r'\*\*', '', line)
        return line

    def _fix_nested_list_indent(self, text: str) -> str:
        """修正 Markdown 嵌套列表转 RST 后的缩进与空行。

        RST 要求：父级 ``-`` 与缩进子列表之间不能有空行（否则会被当成 block quote）；
        从缩进子列表回到顶层 ``-`` 时则需要空行分隔。
        """
        lines = text.splitlines()
        out = []
        for i, line in enumerate(lines):
            is_top_level_item = bool(
                re.match(r'^[-*]\s', line) or re.match(r'^\d+\.', line)
            )

            # 去掉父级列表项与缩进子项之间误插的空行
            if (
                line == ''
                and out
                and out[-1] != ''
                and not re.match(r'^  ', out[-1])
                and i + 1 < len(lines)
                and re.match(r'^  +- ', lines[i + 1])
            ):
                continue

            # 缩进子列表结束后回到顶层列表项前补空行
            if is_top_level_item and out and out[-1] != '':
                if re.match(r'^  ', out[-1]):
                    out.append('')

            out.append(line)
        return '\n'.join(out)

    def _ensure_blank_before_directives(self, text: str) -> str:
        """段落与 .. directive 之间补空行，避免 code-block 等紧贴正文。"""
        return re.sub(
            r'(\S)\n(\.\. (?:code-block|warning|note|important|caution|tip)::)',
            r'\1\n\n\2',
            text,
        )

    def _ensure_document_title(self, text: str) -> str:
        """首行非 Markdown 标题时，提升为 RST 文档标题（避免 toctree no title）。"""
        lines = text.splitlines()
        idx = 0
        while idx < len(lines) and not lines[idx].strip():
            idx += 1
        if idx >= len(lines):
            return text
        first = lines[idx].strip()
        if first.startswith('#') or first.startswith('..') or first.startswith(':link_to_translation:'):
            return text
        if idx + 1 < len(lines) and re.match(r'^[=\-~^`\'",.:*+#_]+$', lines[idx + 1].strip()):
            return text
        width = _cjk_display_width(first)
        lines.insert(idx + 1, '=' * width)
        lines.insert(idx + 2, '')
        return '\n'.join(lines)

    def _convert_markdown_tables(self, text: str) -> str:
        """将 Markdown 表格包进 code-block，避免 |------| 被当成 RST 替换引用。"""
        lines = text.splitlines()
        result = []
        i = 0
        while i < len(lines):
            line = lines[i]
            # 已缩进行（含 code-block 内容）不参与 Markdown 表格识别
            if re.match(r'^\s', line):
                result.append(line)
                i += 1
                continue
            if '|' in line and i + 1 < len(lines) and re.match(r'^\s*\|?[\s:\-|]+\|', lines[i + 1]):
                table_lines = [line]
                i += 1
                while i < len(lines) and '|' in lines[i]:
                    table_lines.append(lines[i])
                    i += 1
                result.append('.. code-block:: text')
                result.append('')
                for tl in table_lines:
                    result.append('   ' + tl.rstrip())
                result.append('')
                continue
            result.append(line)
            i += 1
        return '\n'.join(result)

    def _fix_rst_warning_blocks(self, text: str) -> str:
        """把 .. warning:: 后面误接 markdown 围栏代码块的内容改成 RST 缩进段落。"""
        def repl(m: Match) -> str:
            body = m.group(1).strip('\n')
            out = ['.. warning::', '']
            for bl in body.splitlines():
                out.append('   ' + bl)
            return '\n'.join(out)

        return re.sub(
            r'\.\. warning::\s*\n+```\w*\n(.*?)```',
            repl,
            text,
            flags=re.DOTALL,
        )

    def _convert_backtick_links(self, text: str) -> str:
        """`[text](url)` -> `text <url>`_（README 里常见反引号包裹的链接）。"""
        return re.sub(
            r'`\[([^\]]+)\]\(([^)]+)\)`',
            r'`\1 <\2>`_',
            text,
        )

    def _convert_link_to_translation(self, text: str) -> str:
        # 确保使用正确的反引号格式，避免出现双反引号
        # 匹配无序列表中的语言链接
        text = re.sub(r'^\s*[*-]\s+\[English\]\(\.\/README\.md\)', r':link_to_translation:`en:[English]`', text, flags=re.MULTILINE)
        text = re.sub(r'^\s*[*-]\s+\[中文\]\(\.\/README_CN\.md\)', r':link_to_translation:`zh_CN:[中文]`', text, flags=re.MULTILINE)
        # 匹配普通文本中的语言链接
        text = re.sub(r'\[English\]\(\.\/README\.md\)', r':link_to_translation:`en:[English]`', text)
        text = re.sub(r'\[中文\]\(\.\/README_CN\.md\)', r':link_to_translation:`zh_CN:[中文]`', text)
        return text


    def _contains_rst_syntax(self, line: str) -> bool:
        """检查一行文本是否已经包含reST语法"""
        # 检查常见的reST语法模式
        patterns = [
            # 检查以..开头的指令（如.. image::, .. code-block::等）
            r'^\s*\.\.',
            # 检查以:开头的指令（如:link_to_translation:）
            r'^\s*:',
            r':link_to_translation:',
            # 检查rst链接格式 `link text <url>`_
            r'`[^`]+ <[^>]+>`_',
            # 检查行内代码 ``code``
            r'``[^`]+``',
            # 检查toctree指令
            r'\.\.\s+toctree\s*::',
            # 检查note等提示框指令
            r'\.\.\s+(note|warning|tip|important|caution)\s*::'
        ]

        # 如果包含任何一个reST语法模式，则返回True
        for pattern in patterns:
            if re.search(pattern, line):
                return True

        return False

    def _convert_headings(self, text: str) -> str:
        def repl(m: Match) -> str:
            level = len(m.group(1))
            title = m.group(2).strip()
            width = _cjk_display_width(title)
            if level == 1:
                underline = "=" * width
            elif level == 2:
                underline = "-" * width
            elif level == 3:
                underline = "," * width
            elif level == 4:
                underline = "." * width
            elif level == 5:
                underline = "*" * width
            else:
                underline = "~" * width
            return f"{title}\n{underline}\n"
        # 先处理标准的# 标题格式
        text = re.sub(r"^(#{1,6})\s+(.*)$", repl, text, flags=re.MULTILINE)
        # 再处理特殊的#. 标题格式
        text = re.sub(r"^(#)\.\s+(.*)$", repl, text, flags=re.MULTILINE)
        return text

    def _convert_images(self, text: str) -> str:
        def repl(match):
            alt_text = match.group(1)
            image_path = match.group(2)

            # 如果指定了源路径和目标路径，则调整图片相对路径
            if self.src_path and self.dst_path:
                # 判断是否为绝对路径
                if not os.path.isabs(image_path):
                    # 构建图片的绝对路径
                    absolute_image_path = os.path.join(self.src_path, image_path)
                    # 计算相对于目标路径的相对路径
                    relative_image_path = os.path.relpath(absolute_image_path, self.dst_path)
                    # 确保路径使用正斜杠（RST标准）
                    image_path = relative_image_path.replace(os.sep, '/')

            return f".. image:: {image_path}\n   :alt: {alt_text}"

        return re.sub(r'!\[(.*?)\]\((.*?)\)', repl, text)

    def _convert_links(self, text: str) -> str:
        text = re.sub(r'\[(.*?)\]\((.*?)\)', r'`\1 <\2>`_', text)
        # 参考：`link`_ 中冒号紧贴反引号会触发 literal 解析错误
        text = re.sub(r'([：:])(`[^`]+ <)', r'\1 \2', text)
        return text

    def _convert_bold(self, text: str) -> str:
        return re.sub(r'(\*\*|__)(.*?)\1', r'**\2**', text)

    def _convert_italic(self, text: str) -> str:
        """
        *italic* 或 _italic_ -> *italic*
        避免误伤普通下划线，如 link_to
        """
        # 只匹配被空格或行首行尾包围的 _xxx_
        text = re.sub(r'(?<!\w)\*(?!\*)(.+?)(?<!\*)\*(?!\w)', r'*\1*', text)
        text = re.sub(r'(?<!\w)_(?!_)(.+?)(?<!_)_(?!\w)', r'*\1*', text)
        return text

    def _convert_codeblocks(self, text: str) -> str:
        """转换Markdown代码块为reST代码块"""
        def repl(m: Match) -> str:
            lang = m.group(1).strip() if m.group(1) else ""
            code = m.group(2)
            rst = f".. code-block:: {lang}\n\n"
            for line in code.splitlines():
                # 确保代码行正确缩进，避免reST语法错误
                rst += f"   {line}\n"
            return rst + "\n"

        # 使用更健壮的正则表达式匹配代码块
        # 使用DOTALL标志使.匹配换行符，使用贪婪模式确保完整匹配代码块
        return re.sub(r"```(\w*)\n(.*?)```", repl, text, flags=re.DOTALL)

    def _clean_text_before_conversion(self, text: str) -> str:
        """在转换前清理文本，移除可能导致问题的内容"""
        # 移除编辑器相关的标记或注释
        text = re.sub(r'用户\d+\s+复制\s+删除\s+', '', text)
        # 移除多余的空行
        text = re.sub(r'\n{3,}', '\n\n', text)
        return text

    def _convert_inline_code(self, text: str) -> str:
        links = []

        def stash_link(m: Match) -> str:
            links.append(m.group(0))
            return f'\x00RSTLINK{len(links) - 1}\x00'

        text = re.sub(r'`[^`]+ <[^>]+>`_', stash_link, text)

        def repl(m: Match) -> str:
            inner = m.group(1)
            if re.match(r'^(en|zh_CN):\[', inner):
                return m.group(0)
            stripped = re.sub(r'^\*\*+|\*\*+$', '', inner)
            if stripped != inner:
                return f'``{stripped}``'
            return f'``{inner}``'

        text = re.sub(r'`([^`]+)`', repl, text)
        for idx, link in enumerate(links):
            text = text.replace(f'\x00RSTLINK{idx}\x00', link)
        return text

    def _fix_unbalanced_strong(self, text: str) -> str:
        """Remove orphan ** markers left from malformed README emphasis."""
        while True:
            stars = [m.start() for m in re.finditer(r'\*\*', text)]
            if len(stars) % 2 == 0:
                break
            text = text[:stars[-1]] + text[stars[-1] + 2:]
        return text

    def _convert_unordered_list(self, text: str) -> str:
        return re.sub(r'^[\-\*]\s+', r'- ', text, flags=re.MULTILINE)

    def _convert_ordered_list(self, text: str) -> str:
        # 保持原始数字格式，不转换为#.格式
        return text

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

    # 使用类的方法进行文件转换
    converter = MarkdownToRST(src_path, dst_path)
    converter.convert_file(src_file, dst_file)

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
    ret = p.wait()
    return p

def copy_projects_doc(src_path, dst_path, lan):
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
            if copy_projects_doc(item_path, item_dst_path, lan):
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
            copy_projects_doc(f'{lan_dir}/../../../../projects', f'{lan_dir}/examples/projects', lan)

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
