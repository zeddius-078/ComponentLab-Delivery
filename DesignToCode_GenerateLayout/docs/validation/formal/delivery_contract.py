"""正式包当前字节与 XML 结构检查；不冒充编译、视觉或人工验收。"""
import hashlib
import json
import re
from pathlib import Path
from xml.etree import ElementTree as ET


def verify_page(package: Path, record_path: Path):
    record = json.loads(record_path.read_bytes())
    if record.get('published') is not True or record.get('profile') != 'wide-light':
        raise ValueError('PACKAGE_NOT_PUBLISHED_COMPACT')
    hashes = record.get('outputSha256')
    if not isinstance(hashes, dict) or not hashes:
        raise ValueError('OUTPUT_HASH_MANIFEST_REQUIRED')
    actual = {p.relative_to(package).as_posix(): p for p in package.rglob('*') if p.is_file()}
    if set(hashes) != set(actual):
        raise ValueError('OUTPUT_FILE_SET_CHANGED')
    for name, file in actual.items():
        if not file.resolve().is_relative_to(package.resolve()):
            raise ValueError('OUTPUT_FILE_REDIRECTED')
        if hashlib.sha256(file.read_bytes()).hexdigest() != hashes[name]:
            raise ValueError('OUTPUT_BYTES_CHANGED: ' + name)
        if file.suffix.lower() not in ('.ui', '.h', '.cpp', '.qss', '.qrc', '.svg', '.png'):
            raise ValueError('UNEXPECTED_RUNTIME_FILE: ' + name)
    uis = [p for name, p in actual.items() if name.startswith('form/') and p.suffix == '.ui']
    if len(uis) != 1:
        raise ValueError('EXACTLY_ONE_UI_REQUIRED')
    stem = uis[0].stem
    for name in (f'include/{stem}.h', f'src/{stem}.cpp', f'view/res/qss/{stem}.qss'):
        if name not in actual:
            raise ValueError('MISSING_FOUR_FILE_MEMBER: ' + name)
    tree = ET.parse(uis[0])
    root = tree.getroot().find('widget')
    if root is None:
        raise ValueError('ROOT_WIDGET_REQUIRED')
    geometry = tree.findall('.//property[@name="geometry"]')
    if len(geometry) != 1 or root.find('property[@name="geometry"]') is not geometry[0]:
        raise ValueError('ABSOLUTE_CHILD_GEOMETRY')
    if tree.findall('.//spacer'):
        raise ValueError('UNMAPPED_SPACER')
    for prop in tree.findall('.//property'):
        if prop.get('name') in ('designNodeId', 'designNodeName', 'designNodeType', 'styleSheet'):
            raise ValueError('RUNTIME_TRACE_OR_INLINE_STYLE')
    objects = [*tree.findall('.//widget'), *tree.findall('.//layout')]
    names = [item.get('name', '') for item in objects]
    if len(names) != len(set(names)) or any(not re.fullmatch(r'[a-z][A-Za-z0-9]*', n) for n in names):
        raise ValueError('INVALID_PUBLIC_OBJECT_NAMES')
    header = actual[f'include/{stem}.h'].read_text(encoding='utf-8')
    for name in names:
        if f'get{name[0].upper() + name[1:]}()' not in header:
            raise ValueError('PUBLIC_ACCESSOR_MISSING: ' + name)
    return dict(files=len(actual), objects=len(names), hashes=hashes)


def verify_batch(project: Path, batch: str):
    if not re.fullmatch(r'[A-Za-z0-9][A-Za-z0-9_-]*', batch):
        raise ValueError('INVALID_BATCH')
    directory = project / 'build/delivery' / batch
    pages = ('review', 'result', 'horizontal')
    if not directory.is_dir() or {p.name for p in directory.iterdir()} != set(pages):
        raise ValueError('THREE_COMPLETE_PAGES_REQUIRED')
    return {page: verify_page(directory / page,
        project / 'build/reports/formal-delivery' / batch / page / 'generation-record.json')
        for page in pages}
