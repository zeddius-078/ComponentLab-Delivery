"""隔离修改原始 sToken，再调用既有生成链和公开 SDK；不修改正式源或交付文件。"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

PROJECT = Path(__file__).resolve().parents[3]


def hashes(root):
    return {str(p.relative_to(root)): hashlib.sha256(p.read_bytes()).hexdigest()
            for p in root.rglob('*') if p.is_file() and '__pycache__' not in p.parts}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--component-root', type=Path, required=True)
    parser.add_argument('--probe', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    report = args.report.resolve()
    if not report.is_relative_to((PROJECT / 'build/reports').resolve()):
        raise ValueError('REPORT_OUTSIDE_TOOL')
    report.mkdir(parents=True, exist_ok=True)
    source = args.component_root / 'Source/ComponentKit/design-assets'
    original = hashes(source)
    tool_dir = args.component_root / 'build/cache/GeneratedComponentSDK'
    env = dict(os.environ, QT_QPA_PLATFORM='offscreen')
    result = {'status': 'RUNNING', 'scope': 'Primitive -> Alias/Profile -> SDK generated artifacts -> page button pixel',
              'sourceToken': '--color/highlight/purple-100', 'phases': {}}

    def run(command, name):
        process = subprocess.run([str(x) for x in command], env=env, capture_output=True, timeout=120)
        (report / (name + '.txt')).write_bytes(process.stdout + b'\n' + process.stderr)
        if process.returncode:
            raise RuntimeError(f'{name}: exit {process.returncode}')
        return process.stdout.decode('utf-8-sig')

    try:
        with tempfile.TemporaryDirectory(prefix='token-roundtrip-', dir=PROJECT / 'build/cache') as temporary:
            root = Path(temporary) / 'assets'
            shutil.copytree(source, root, ignore=shutil.ignore_patterns('__pycache__'))
            token_file = root / 'DesignSystem/imports/stoken.json'
            baseline_bytes = token_file.read_bytes()
            source_json = json.loads(baseline_bytes.decode('utf-8-sig'))
            primitive = next(mode['参数'] for entry in source_json for mode in entry.get('01.primitive', []) if '参数' in mode)
            old_value = primitive[result['sourceToken']]
            result['originalValue'] = old_value
            result['testValue'] = 'rgba(12, 123, 99, 1)'
            baseline_generated = None
            for phase in ['baseline', 'changed', 'restored']:
                if phase == 'changed':
                    primitive[result['sourceToken']] = result['testValue']
                    token_file.write_text(json.dumps(source_json, ensure_ascii=False, indent=2), encoding='utf-8')
                elif phase == 'restored':
                    token_file.write_bytes(baseline_bytes)
                run([sys.executable, '-B', source / 'tools/export_tokens.py', '--root', root], phase + '-resolve')
                for family in ['button', 'checkbox', 'forms']:
                    run([tool_dir / f'd2c_{family}_token_compiler.exe', root, root / 'generated' / family], phase + '-' + family)
                manifest = json.loads((root / 'generated/button/manifest.json').read_bytes())
                profile = next(p for p in manifest['profiles'] if p['profile'] == 'light-compact')
                observed = json.loads(run([args.probe, root / 'generated', report / (phase + '.png')], phase + '-probe'))
                snapshot = hashes(root / 'generated')
                result['phases'][phase] = {'runtime': observed, 'palettePrimary': profile['palette']['primary'],
                    'sourceSha256': hashlib.sha256(token_file.read_bytes()).hexdigest(),
                    'qssSha256': snapshot[str(Path('button/light-compact/Z_PushButton.qss'))]}
                if phase == 'baseline':
                    baseline_generated = snapshot
                elif phase == 'restored' and snapshot != baseline_generated:
                    raise AssertionError('RESTORED_GENERATED_BYTES_DIFFER')
            baseline = result['phases']['baseline']['runtime']
            changed = result['phases']['changed']['runtime']
            restored = result['phases']['restored']['runtime']
            if baseline['pixel'] != '#6235f7' or changed['pixel'] != '#0c7b63':
                raise AssertionError(f'PIXEL_CHAIN_MISMATCH: {baseline} -> {changed}')
            if restored != baseline:
                raise AssertionError('RESTORED_RUNTIME_DIFFER')
            if any(changed[key] != baseline[key] for key in ['buttonWidth', 'buttonHeight', 'enabled']):
                raise AssertionError('COLOR_CHANGE_ALTERED_LAYOUT_OR_STATE')
            if result['phases']['baseline']['qssSha256'] == result['phases']['changed']['qssSha256']:
                raise AssertionError('QSS_DID_NOT_CHANGE')
            result['restoredGeneratedBytesIdentical'] = True
            result['status'] = 'PASS'
    except Exception as error:
        result['status'] = 'FAIL'
        result['error'] = str(error)
        raise
    finally:
        result['originalSourceUnchanged'] = hashes(source) == original
        if not result['originalSourceUnchanged']:
            result['status'] = 'FAIL'
        (report / 'token-roundtrip.json').write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding='utf-8')
    if result['status'] != 'PASS':
        raise AssertionError('TOKEN_ROUNDTRIP_FAILED')
    print(json.dumps(result, ensure_ascii=False))


if __name__ == '__main__':
    main()
