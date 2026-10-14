"""Phase 2: check rules against saved DSL component definitions and actual Qt headers."""
# 静态契约检查：使用保存的 Login 样本，核对组件身份、变体值域及目标头文件存在。
# 运行时 API 的实际执行由 ComponentTests.cpp 检查；本脚本直接运行时会刷新结果 JSON。
# 检查使用 assert，执行时保留断言，使用 python -B 而不加 -O/-OO。
import hashlib
import json
import os
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SDK = Path(os.environ.get(
    'GENERATED_COMPONENT_SDK_ROOT',
    ROOT.parent.parent / 'DesignToCode_GenerateComponent' / 'build' / 'sdk',
)).resolve()
raw = json.loads((ROOT / 'HtmlDsl/pipeline/design-inputs/node-12-382-full/canonical.raw.json').read_bytes())
mapping = json.loads((ROOT / 'HtmlDsl/pipeline/mapping/component-mapping.json').read_bytes())
definitions = {n['guid']: n for n in raw['pixComponentTreeDslNodes']}
rules = {n['designComponentKey']: n for n in mapping['rules'] if 'designComponentKey' in n}

# 公共图片/原理图资源只能通过 GenerateComponent SDK 的稳定语义 ID 消费。
# 此处验证映射结构；实际解析、摘要和 Profile 渲染由 ComponentTests.cpp 覆盖。
public_assets = mapping['publicAssetMappings']
assert public_assets['missing'] == 'UNMAPPED_RESOURCE'
asset_entries = public_assets['entries']
assert set(asset_entries) == {
    'image.foundation.geometric-preview',
    'model.foundation.schematic',
}
for asset_id, entry in asset_entries.items():
    assert entry['assetId'] == asset_id
    assert entry['ownership'] == 'shared'
    assert entry['designIdentity'] in {'ComponentLab/ImageAsset', 'ComponentLab/ModelAsset'}
    assert entry['defaultState'] in {'Loaded', 'Ready'}
    assert entry['fit'] and set(entry['fit']) <= {'Contain', 'Cover'}
    # GenerateLayout 不得再维护 SDK 公共资产的具体文件/QRC 路径。
    assert not ({'source', 'path', 'qrcPath', 'resourcePath'} & set(entry))
assert (SDK / 'include/MyDesignSystem/VisualAssetCatalog.h').is_file()
# 每条规则必须且只能声明一种稳定身份，防止 set id 与旧 key 同时存在时产生隐式优先级。
identities = []
for rule in mapping['rules']:
    identity_fields = [key for key in ('designComponentKey', 'designComponentSetId', 'designIdentityId') if key in rule]
    assert len(identity_fields) == 1, rule['designComponentName']
    identities.append((identity_fields[0], rule[identity_fields[0]], rule['designComponentName']))
assert len(identities) == len(set(identities))
results = []
# 仅遍历当前根节点的直接子实例，按组件集 key 找规则，再核对母版引用。
for instance in raw['pixTreeNodes'][0]['childNode']:
    # 仅组件实例具备母版身份与变体属性；普通布局或文本节点交给其他转换规则处理。
    if instance['type'] != 'INSTANCE':
        continue
    family = definitions[instance['mainStateGroup']]
    rule = rules[family['componentKey']]
    assert family['name'] == rule['designComponentName']
    master = next(n for n in family['childNode'] if n['guid'] == instance['mainComponent'])
    assert master['componentKey'] == instance['componentKey']
    properties = {p['name']: p['value'] for p in instance['propRefMap']['variant']}
    # state、Profile 分别走状态表和配置值，其余属性走各自映射值域。
    for key, value in properties.items():
        # 按属性职责选择核对位置：状态表、统一 Profile 或普通属性值域，避免不同命名空间混用。
        if key == 'state':
            assert value in rule['stateMappings']
        elif key == 'Profile':
            assert value == mapping['profile']
        else:
            assert value in rule['propertyMappings'][key]['values']
    # 这里只检查头文件路径存在，方法签名和参数类型仍需编译验证。
    header = SDK / 'include/MyDesignSystem' / (rule['qtClass'].split('::')[-1]+'.h')
    assert header.is_file()
    results.append({'instance': instance['guid'], 'family': family['name'],
                    'qtClass': rule['qtClass'], 'properties': properties})
# 全部断言通过才写 PASS；失败抛错时旧文件可能仍在，应结合命令退出码判断。
report = {'phase': 2, 'status': 'PASS', 'instances': results,
          'publicAssets': sorted(asset_entries),
          'mappingSha256': hashlib.sha256((ROOT/'HtmlDsl/pipeline/mapping/component-mapping.json').read_bytes()).hexdigest(),
          'scope': 'identity, property domains and public asset ownership; native API execution verified in Phase 3'}
(ROOT.parent/'build/reports/component/mapping-result.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
print(json.dumps(report))
