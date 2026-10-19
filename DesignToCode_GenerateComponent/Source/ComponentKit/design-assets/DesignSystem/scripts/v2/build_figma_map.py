"""Build the full Figma variable-group <-> component map for the V2 architecture.

Input : DesignSystem/config/figma-audit-v2.json (MCP read-only audit snapshot)
Output: DesignSystem/config/figma-component-map-v2.json

For every audited variable binding the generator finds the canonical V2 token:
1. curated name-pattern table (runtime variable names -> canonical paths);
2. fallback: exact resolved-value match over the canonical context;
3. otherwise: isolated (family F antd leftovers) or unmapped (registered).

Each binding records the design value and the canonical resolved value, so the
delta list is exactly the pending Figma rebind work.
"""
from __future__ import annotations

import json
import re
import sys
from pathlib import Path
from typing import Any

# Compatibility entry point: historical callers may still invoke this module,
# but the runtime map must always be produced from V2 bindings/recipes.  Keep
# the old audit implementation below as read-only migration history; it is no
# longer allowed to write the runtime map.
if __name__ == "__main__":
    from v2.build_runtime_map import write_map
    root = Path(__file__).resolve().parents[3]
    result = write_map(root, root / "DesignSystem/config/figma-component-map-v2.json")
    print(f"figma-component-map-v2: {len(result['components'])} V2 components")
    raise SystemExit(0)

SCRIPTS = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(SCRIPTS))

from v2.model import TokenModel
from v2.resolver import Resolver
from v2.values import qt_value

ROOT = SCRIPTS.parents[1]
AUDIT = ROOT / "DesignSystem/config/figma-audit-v2.json"
OUT = ROOT / "DesignSystem/config/figma-component-map-v2.json"

HEX = re.compile(r"^#([0-9A-Fa-f]{6,8})$")

# name pattern -> canonical path (pattern is searched case-insensitively)
CURATED = [
    (re.compile(r"qtlab-semantic-color-text-primary$", re.I), "semantic.color.text.primary"),
    (re.compile(r"qtlab-semantic-color-text-secondary$", re.I), "semantic.color.text.secondary"),
    (re.compile(r"qtlab-semantic-color-text-muted$", re.I), "semantic.color.text.muted"),
    (re.compile(r"qtlab-semantic-color-text-disabled$", re.I), "semantic.color.text.disabled"),
    (re.compile(r"qtlab-semantic-color-text-invert$", re.I), "semantic.color.text.invert"),
    (re.compile(r"qtlab-semantic-color-action-primary$", re.I), "semantic.color.interactive.brand.default"),
    (re.compile(r"qtlab-semantic-color-action-primary-bg$", re.I), "semantic.color.interactive.brand.subtle"),
    (re.compile(r"qtlab-semantic-color-background-surface$", re.I), "semantic.color.background.page"),
    (re.compile(r"qtlab-semantic-color-background-surface-muted$", re.I), "semantic.color.background.surface"),
    (re.compile(r"qtlab-semantic-color-background-elevated$", re.I), "semantic.color.background.surface-raised"),
    (re.compile(r"qtlab-semantic-color-border-subtle$", re.I), "semantic.color.border.secondary"),
    (re.compile(r"qtlab-semantic-color-status-danger$", re.I), "semantic.color.status.danger.default"),
    (re.compile(r"qtlab-semantic-color-status-warning$", re.I), "semantic.color.status.warning.default"),
    (re.compile(r"qtlab-semantic-color-status-warning-bg$", re.I), "semantic.color.status.warning.soft"),
    (re.compile(r"qtlab-semantic-dimension-control-height-xs$", re.I), "adapter.qt.dimension.control-height.xs"),
    (re.compile(r"qtlab-semantic-dimension-control-height-sm$", re.I), "adapter.qt.dimension.control-height.sm"),
    (re.compile(r"qtlab-semantic-dimension-control-height-md$", re.I), "adapter.qt.dimension.control-height.md"),
    (re.compile(r"qtlab-semantic-dimension-control-height-lg$", re.I), "adapter.qt.dimension.control-height.lg"),
    (re.compile(r"qtlab-component-basic-button-size-(sm|md|lg)-height$", re.I), "adapter.qt.button.size.\\1.height"),
    (re.compile(r"qtlab-base-stoken-adapter-button-xs-icon-container$", re.I), "adapter.qt.button.iconContainer.XS"),
    (re.compile(r"qtlab-base-stoken-adapter-button-s-icon-container$", re.I), "adapter.qt.button.iconContainer.S"),
    (re.compile(r"qtlab-base-stoken-adapter-button-m-icon-container$", re.I), "adapter.qt.button.iconContainer.M"),
    (re.compile(r"qtlab-base-stoken-adapter-button-l-icon-container$", re.I), "adapter.qt.button.iconContainer.L"),
    (re.compile(r"menu-hover$", re.I), "semantic.color.background.hover"),
    (re.compile(r"menu-disabled$", re.I), "semantic.color.text.disabled"),
    (re.compile(r"menu-danger$", re.I), "semantic.color.status.danger.default"),
    (re.compile(r"menu-dangersurface$", re.I), "semantic.color.status.danger.subtle"),
    (re.compile(r"menu-divider$", re.I), "semantic.color.border.secondary"),
    (re.compile(r"menu-border$", re.I), "semantic.color.border.secondary"),
    (re.compile(r"menu-active$", re.I), "semantic.color.interactive.brand.default"),
    (re.compile(r"menu-indicator$", re.I), "semantic.color.interactive.brand.default"),
    (re.compile(r"menu-secondary$", re.I), "semantic.color.text.muted"),
    (re.compile(r"menu-surface$", re.I), "semantic.color.background.page"),
    (re.compile(r"menu-text$", re.I), "semantic.color.text.primary"),
    (re.compile(r"input-state-focus-border$", re.I), "semantic.color.interactive.brand.focus-ring"),
    (re.compile(r"input-state-default-border$", re.I), "semantic.color.border.default"),
    (re.compile(r"input-state-default-placeholder$", re.I), "semantic.color.text.tertiary"),
    (re.compile(r"input-state-default-background$", re.I), "semantic.color.background.page"),
    (re.compile(r"input-state-hover-border$", re.I), "semantic.color.border.hover"),
    (re.compile(r"input-state-focus-outline$", re.I), "semantic.color.interactive.brand.subtle"),
    (re.compile(r"input-state-default-text$", re.I), "semantic.color.text.primary"),
    (re.compile(r"input-state-disabled-background$", re.I), "semantic.color.background.disabled"),
    (re.compile(r"input-state-disabled-border$", re.I), "semantic.color.border.secondary"),
    (re.compile(r"input-state-error-border$", re.I), "semantic.color.status.danger.default"),
    (re.compile(r"input-state-error-background$", re.I), "semantic.color.status.danger.subtle"),
    (re.compile(r"input-state-disabled-text$", re.I), "semantic.color.text.disabled"),
    (re.compile(r"input-radius$", re.I), "semantic.radius.control-default"),
    (re.compile(r"input-size-(sm|md|lg)-padding-inline$", re.I), "component.input.size.\\1.container.padding-inline"),
    (re.compile(r"input-size-(sm|md|lg)-height$", re.I), "component.input.size.\\1.container.visual-height"),
    (re.compile(r"input-size-md-font-size$", re.I), "component.input.base.default.content.font-size"),
    (re.compile(r"select-default-placeholder-color$", re.I), "semantic.color.text.tertiary"),
    (re.compile(r"select-default-content-color$", re.I), "semantic.color.text.primary"),
    (re.compile(r"select-disabled-content-color$", re.I), "semantic.color.text.disabled"),
    (re.compile(r"select-default-container-background$", re.I), "semantic.color.background.page"),
    (re.compile(r"select-hover-border-color$", re.I), "semantic.color.border.hover"),
    (re.compile(r"progress-state-default-text$", re.I), "semantic.color.text.primary"),
    (re.compile(r"progress-state-default-indicator$", re.I), "semantic.color.interactive.brand.default"),
    (re.compile(r"progress-state-disabled-text$", re.I), "semantic.color.text.disabled"),
    (re.compile(r"progress-state-default-background$", re.I), "semantic.color.background.page"),
    (re.compile(r"progress-radius$", re.I), "semantic.radius.control-default"),
    (re.compile(r"progress-border-width$", re.I), "semantic.border-width.boundary-default"),
    (re.compile(r"progress-state-default-border$", re.I), "semantic.color.border.default"),
    (re.compile(r"progress-state-disabled-background$", re.I), "semantic.color.background.disabled"),
    (re.compile(r"progress-state-disabled-border$", re.I), "semantic.color.border.secondary"),
    (re.compile(r"progress-height$", re.I), "adapter.qt.dimension.control-height.md"),
    (re.compile(r"list-item-selected-hover-background$", re.I), "semantic.color.interactive.brand.soft"),
    (re.compile(r"list-item-selected-background$", re.I), "semantic.color.interactive.brand.soft"),
    (re.compile(r"list-focus-outline$", re.I), "semantic.color.interactive.brand.default"),
    (re.compile(r"list-selected-text$", re.I), "semantic.color.text.primary"),
    (re.compile(r"list-disabled-text$", re.I), "semantic.color.text.disabled"),
    (re.compile(r"list-item-disabled-background$", re.I), "semantic.color.background.disabled"),
    (re.compile(r"list-item-hover-background$", re.I), "semantic.color.background.hover"),
    (re.compile(r"list-muted$", re.I), "semantic.color.text.muted"),
    (re.compile(r"list-text$", re.I), "semantic.color.text.primary"),
    (re.compile(r"list-background$", re.I), "semantic.color.background.page"),
    (re.compile(r"list-border$", re.I), "semantic.color.border.secondary"),
    (re.compile(r"list-radius$", re.I), "semantic.radius.card-default"),
    (re.compile(r"list-border-width$", re.I), "semantic.border-width.boundary-default"),
    (re.compile(r"list-focus-width$", re.I), "semantic.border-width.boundary-default"),
    (re.compile(r"tree-header-background$", re.I), "semantic.color.background.disabled"),
    (re.compile(r"tree-header-text$", re.I), "semantic.color.text.primary"),
    (re.compile(r"tree-branch$", re.I), "semantic.color.text.muted"),
    (re.compile(r"tree-branch-border$", re.I), "semantic.color.border.secondary"),
    (re.compile(r"table-header-split$", re.I), "semantic.color.border.secondary"),
    (re.compile(r"table-row-border$", re.I), "semantic.color.border.secondary"),
    (re.compile(r"table-row-hover-background$", re.I), "semantic.color.background.hover"),
    (re.compile(r"table-row-selected-hover-background$", re.I), "semantic.color.interactive.brand.soft"),
    (re.compile(r"table-row-selected-background$", re.I), "semantic.color.interactive.brand.soft"),
    (re.compile(r"table-header-text$", re.I), "semantic.color.text.primary"),
    (re.compile(r"table-text$", re.I), "semantic.color.text.primary"),
    (re.compile(r"table-selected-text$", re.I), "semantic.color.text.primary"),
    (re.compile(r"table-muted$", re.I), "semantic.color.text.muted"),
    (re.compile(r"table-row-background$", re.I), "semantic.color.background.page"),
    (re.compile(r"table-header-bg$", re.I), "semantic.color.background.disabled"),
    (re.compile(r"table-border-width$", re.I), "semantic.border-width.boundary-default"),
    (re.compile(r"pagination-selected-item-background$", re.I), "semantic.color.interactive.brand.default"),
    (re.compile(r"pagination-selected-item-label-color$", re.I), "semantic.color.text.invert"),
    (re.compile(r"pagination-base-default-item-label-color$", re.I), "semantic.color.text.primary"),
    (re.compile(r"pagination-base-default-navigation-icon-color$", re.I), "semantic.color.interactive.brand.default"),
    (re.compile(r"pagination-base-default-navigation-background$", re.I), "semantic.color.background.page"),
    (re.compile(r"pagination-base-default-navigation-border-color$", re.I), "semantic.color.border.default"),
    (re.compile(r"pagination-base-default-item-radius$", re.I), "semantic.radius.control-default"),
    (re.compile(r"pagination-base-default-navigation-radius$", re.I), "semantic.radius.control-default"),
    (re.compile(r"pagination-base-default-border-width$", re.I), "semantic.border-width.boundary-default"),
    (re.compile(r"spin-trail$", re.I), "semantic.color.border.secondary"),
    (re.compile(r"spin-indicator$", re.I), "semantic.color.interactive.brand.default"),
    (re.compile(r"spin-tip-text$", re.I), "semantic.color.text.primary"),
    (re.compile(r"spin-disabled-text$", re.I), "semantic.color.text.disabled"),
    (re.compile(r"dialog-critical$", re.I), "semantic.color.status.danger.default"),
    (re.compile(r"dialog-warning$|message-dialog-warning$", re.I), "semantic.color.status.warning.default"),
    (re.compile(r"dialog-error$|message-error$|notification-error$|alert-error$", re.I), "semantic.color.status.danger.default"),
    (re.compile(r"message-warning$|notification-warning$|alert-warning$", re.I), "semantic.color.status.warning.default"),
    (re.compile(r"result-status-error$", re.I), "semantic.color.status.danger.default"),
    (re.compile(r"result-status-warning$", re.I), "semantic.color.status.warning.default"),
    (re.compile(r"result-status-info$", re.I), "semantic.color.interactive.brand.default"),
    (re.compile(r"alert-errorsurface$|message-dialog-errorsurface$|notification-errorsurface$", re.I), "semantic.color.status.danger.subtle"),
    (re.compile(r"errorsurface$", re.I), "semantic.color.status.danger.subtle"),
    (re.compile(r"warningsurface$", re.I), "semantic.color.status.warning.soft"),
    (re.compile(r"conflict-surface$", re.I), "semantic.color.status.danger.subtle"),
    (re.compile(r"conflict$", re.I), "semantic.color.status.danger.default"),
    (re.compile(r"recording$", re.I), "semantic.color.interactive.brand.default"),
    (re.compile(r"command-link-hover-background$", re.I), "semantic.color.interactive.brand.soft"),
    (re.compile(r"command-link-pressed-border$", re.I), "semantic.color.interactive.brand.pressed"),
    (re.compile(r"command-link-focus-border$", re.I), "semantic.color.interactive.brand.focus-ring"),
    (re.compile(r"command-link-accent$", re.I), "semantic.color.interactive.brand.default"),
    (re.compile(r"command-link-radius$", re.I), "semantic.radius.control-default"),
    (re.compile(r"command-link-foreground$", re.I), "semantic.color.text.primary"),
    (re.compile(r"command-link-description$", re.I), "semantic.color.text.muted"),
    (re.compile(r"command-link-checked-description$", re.I), "semantic.color.text.muted"),
    (re.compile(r"command-link-background$", re.I), "semantic.color.background.page"),
    (re.compile(r"command-link-border$", re.I), "semantic.color.border.default"),
    (re.compile(r"command-link-disabled-foreground$", re.I), "semantic.color.text.disabled"),
    (re.compile(r"command-link-disabled-background$", re.I), "semantic.color.background.disabled"),
    (re.compile(r"command-link-disabled-border$", re.I), "semantic.color.border.secondary"),
    (re.compile(r"slider-rail$|slider-disabled-rail$", re.I), "semantic.color.border.secondary"),
    (re.compile(r"slider-handle$", re.I), "semantic.color.background.page"),
    (re.compile(r"slider-disabled-track$|slider-disabled-handle$|slider-disabled-hub$", re.I), "semantic.color.background.disabled"),
    (re.compile(r"slider-secondary$", re.I), "semantic.color.text.muted"),
    (re.compile(r"scrollbar-groove$|scrollbar-disabled-groove$|scrollbar-page-hover$", re.I), "semantic.color.border.secondary"),
    (re.compile(r"scrollbar-thumb$", re.I), "semantic.color.text.muted"),
    (re.compile(r"scrollbar-line-surface$", re.I), "semantic.color.background.page"),
    (re.compile(r"scrollbar-thumb-hover$", re.I), "semantic.color.border.default"),
    (re.compile(r"scrollbar-thumb-dragging$", re.I), "semantic.color.interactive.brand.default"),
    (re.compile(r"scrollbar-disabled-thumb$", re.I), "semantic.color.background.disabled"),
    (re.compile(r"chip-background$", re.I), "semantic.color.background.page"),
    (re.compile(r"chip-selected-background$", re.I), "semantic.color.background.surface"),
    (re.compile(r"chip-border$|chip-disabled-border$", re.I), "semantic.color.border.secondary"),
    (re.compile(r"chip-text$", re.I), "semantic.color.text.primary"),
    (re.compile(r"chip-pressed-background$|chip-disabled-background$", re.I), "semantic.color.background.disabled"),
    (re.compile(r"status-bar-accent$|status-bar-temporary$", re.I), "semantic.color.interactive.brand.default"),
    (re.compile(r"status-bar-muted$|status-bar-grip$", re.I), "semantic.color.text.muted"),
    (re.compile(r"status-bar-text$", re.I), "semantic.color.text.primary"),
    (re.compile(r"status-bar-surface$", re.I), "semantic.color.background.page"),
    (re.compile(r"status-bar-border$", re.I), "semantic.color.border.secondary"),
    (re.compile(r"shortcut-input-placeholder$|shortcut-input-helper$", re.I), "semantic.color.text.muted"),
    (re.compile(r"shortcut-input-border$", re.I), "semantic.color.border.default"),
    (re.compile(r"shortcut-input-surface$", re.I), "semantic.color.background.page"),
    (re.compile(r"shortcut-input-foreground$", re.I), "semantic.color.text.primary"),
    (re.compile(r"shortcut-input-focus-outline$", re.I), "semantic.color.interactive.brand.subtle"),
    (re.compile(r"shortcut-input-focus-border$", re.I), "semantic.color.interactive.brand.focus-ring"),
    (re.compile(r"shortcut-input-disabled-surface$", re.I), "semantic.color.background.disabled"),
    (re.compile(r"shortcut-input-disabled-text$", re.I), "semantic.color.text.disabled"),
    (re.compile(r"shortcut-input-disabled-border$", re.I), "semantic.color.border.secondary"),
    (re.compile(r"text-edit-cursor$|plain-text-edit-cursor$|rich-text-edit-cursor$", re.I), "semantic.color.interactive.brand.default"),
    (re.compile(r"focus-border$", re.I), "semantic.color.interactive.brand.focus-ring"),
    (re.compile(r"text-edit-radius$|plain-text-edit-radius$|rich-text-edit-radius$", re.I), "semantic.radius.control-default"),
    (re.compile(r"open-gl-viewport-axis-x$|open-gl-viewport-fallback-text$", re.I), "semantic.color.status.danger.default"),
    (re.compile(r"open-gl-viewport-axis-y$", re.I), "semantic.color.status.warning.default"),
    (re.compile(r"open-gl-viewport-axis-z$|open-gl-viewport-wireframe$", re.I), "semantic.color.interactive.brand.default"),
    (re.compile(r"open-gl-viewport-fallback-background$", re.I), "semantic.color.status.danger.subtle"),
    (re.compile(r"open-gl-viewport-overlay-text$", re.I), "semantic.color.text.muted"),
    (re.compile(r"empty-title$|result-title$|statistic-title$|statistic-value$|integration-card-title$|metric-card-title$|metric-card-value$|navbar-title$", re.I), "semantic.color.text.primary"),
    (re.compile(r"empty-description$|result-description$|statistic-affix$|statistic-empty$|integration-card-description$|metric-card-meta$|navbar-meta$", re.I), "semantic.color.text.muted"),
    (re.compile(r"empty-illustration-accent$", re.I), "semantic.color.interactive.brand.default"),
    (re.compile(r"empty-illustration-stroke$", re.I), "semantic.color.border.default"),
    (re.compile(r"statistic-error$", re.I), "semantic.color.status.danger.default"),
    (re.compile(r"statistic-loading$", re.I), "semantic.color.border.secondary"),
    (re.compile(r"toolbar-handle$|toolbar-focus$", re.I), "semantic.color.interactive.brand.default"),
    (re.compile(r"toolbar-separator$|toolbar-border$", re.I), "semantic.color.border.secondary"),
    (re.compile(r"toolbar-surface$", re.I), "semantic.color.background.page"),
    (re.compile(r"toolbar-text$", re.I), "semantic.color.text.primary"),
    (re.compile(r"toolbar-hover$", re.I), "semantic.color.background.hover"),
    (re.compile(r"tool-button-base-default-foreground-color$", re.I), "semantic.color.text.primary"),
    (re.compile(r"tool-button-base-disabled-border-color$", re.I), "semantic.color.border.secondary"),
    (re.compile(r"tool-button-menu-split-divider-color$", re.I), "semantic.color.border.secondary"),
    (re.compile(r"tabs-accent$|tabs-focus$|tabs-selected$", re.I), "semantic.color.interactive.brand.default"),
    (re.compile(r"tabs-hover$", re.I), "semantic.color.background.hover"),
    (re.compile(r"tabs-surface$|tabs-pane$", re.I), "semantic.color.background.page"),
    (re.compile(r"tabs-text$", re.I), "semantic.color.text.primary"),
    (re.compile(r"tabs-border$", re.I), "semantic.color.border.secondary"),
    (re.compile(r"tabs-disabled$", re.I), "semantic.color.text.disabled"),
    (re.compile(r"toolbox-accent$|toolbox-focus$", re.I), "semantic.color.interactive.brand.default"),
    (re.compile(r"toolbox-hover$|toolbox-selected$", re.I), "semantic.color.background.hover"),
    (re.compile(r"toolbox-surface$|toolbox-pane$", re.I), "semantic.color.background.page"),
    (re.compile(r"toolbox-text$", re.I), "semantic.color.text.primary"),
    (re.compile(r"toolbox-border$", re.I), "semantic.color.border.secondary"),
    (re.compile(r"toolbox-disabled$", re.I), "semantic.color.text.disabled"),
    (re.compile(r"mdi-accent$|mdi-active$", re.I), "semantic.color.interactive.brand.default"),
    (re.compile(r"mdi-hover$", re.I), "semantic.color.background.hover"),
    (re.compile(r"mdi-workspace$|mdi-surface$", re.I), "semantic.color.background.page"),
    (re.compile(r"mdi-text$", re.I), "semantic.color.text.primary"),
    (re.compile(r"mdi-border$", re.I), "semantic.color.border.secondary"),
    (re.compile(r"mdi-disabled$", re.I), "semantic.color.text.disabled"),
    (re.compile(r"flow-accent$|flow-active$", re.I), "semantic.color.interactive.brand.default"),
    (re.compile(r"flow-hover$", re.I), "semantic.color.background.hover"),
    (re.compile(r"flow-pane$|flow-surface$", re.I), "semantic.color.background.page"),
    (re.compile(r"flow-text$", re.I), "semantic.color.text.primary"),
    (re.compile(r"flow-border$", re.I), "semantic.color.border.secondary"),
    (re.compile(r"flow-disabled$", re.I), "semantic.color.text.disabled"),
    (re.compile(r"splitter-accent$|splitter-active$|groupbox-active$|groupbox-accent$|groupbox-focus$|widgetframe-focus$|widgetframe-accent$|dockwidget-accent$", re.I), "semantic.color.interactive.brand.default"),
    (re.compile(r"splitter-hover$|groupbox-hover$|widgetframe-separator$|scrollarea-hover$", re.I), "semantic.color.background.hover"),
    (re.compile(r"splitter-surface$|splitter-pane$|groupbox-pane$|groupbox-surface$|widgetframe-pane$|widgetframe-surface$|scrollarea-surface$|dockwidget-surface$|dockwidget-pane$", re.I), "semantic.color.background.page"),
    (re.compile(r"splitter-text$|groupbox-text$|widgetframe-text$|scrollarea-text$|dockwidget-text$", re.I), "semantic.color.text.primary"),
    (re.compile(r"splitter-border$|groupbox-border$|widgetframe-border$|scrollarea-border$|dockwidget-border$", re.I), "semantic.color.border.secondary"),
    (re.compile(r"splitter-disabled$|groupbox-disabled$|widgetframe-disabled$|dockwidget-disabled$", re.I), "semantic.color.text.disabled"),
    (re.compile(r"layout-text$", re.I), "semantic.color.text.primary"),
    (re.compile(r"layout-muted$", re.I), "semantic.color.text.muted"),
    (re.compile(r"layout-surface$", re.I), "semantic.color.background.page"),
    (re.compile(r"layout-border$", re.I), "semantic.color.border.secondary"),
    (re.compile(r"layout-accent$", re.I), "semantic.color.interactive.brand.default"),
    (re.compile(r"form-accent$", re.I), "semantic.color.interactive.brand.default"),
    (re.compile(r"form-text$", re.I), "semantic.color.text.primary"),
    (re.compile(r"form-muted$", re.I), "semantic.color.text.muted"),
    (re.compile(r"form-surface$", re.I), "semantic.color.background.page"),
    (re.compile(r"form-border$", re.I), "semantic.color.border.secondary"),
    (re.compile(r"form-error$", re.I), "semantic.color.status.danger.default"),
    (re.compile(r"form-warning$", re.I), "semantic.color.status.warning.default"),
    (re.compile(r"dialog-accent$|dialog-focus$|message-dialog-focus$|system-picker-accent$|system-picker-focus$|system-picker-selected$|specialized-dialog-focus$|specialized-dialog-selected$|specialized-dialog-accent$", re.I), "semantic.color.interactive.brand.default"),
    (re.compile(r"dialog-surface$|message-dialog-surface$|system-picker-pane$|system-picker-surface$|specialized-dialog-surface$|specialized-dialog-pane$", re.I), "semantic.color.background.page"),
    (re.compile(r"dialog-text$|message-dialog-text$|system-picker-text$|specialized-dialog-text$", re.I), "semantic.color.text.primary"),
    (re.compile(r"dialog-border$|message-dialog-border$|system-picker-border$|specialized-dialog-border$", re.I), "semantic.color.border.secondary"),
    (re.compile(r"dialog-muted$|message-dialog-muted$|system-picker-muted$|specialized-dialog-muted$", re.I), "semantic.color.text.muted"),
    (re.compile(r"message-dialog-information$|message-dialog-question$|message-info$|message-loading$|notification-info$|alert-info$", re.I), "semantic.color.interactive.brand.default"),
    (re.compile(r"message-text$|notification-text$", re.I), "semantic.color.text.primary"),
    (re.compile(r"message-surface$|notification-surface$|alert-surface$", re.I), "semantic.color.background.page"),
    (re.compile(r"notification-border$|alert-border$", re.I), "semantic.color.border.secondary"),
    (re.compile(r"notification-muted$|alert-muted$", re.I), "semantic.color.text.muted"),
    (re.compile(r"alert-success$|message-success$|notification-success$|result-status-success$", re.I), "semantic.color.status.success.default"),
    (re.compile(r"statistic-error$", re.I), "semantic.color.status.danger.default"),
    (re.compile(r"alert-infosurface$|message-infosurface$", re.I), "semantic.color.background.surface"),
    (re.compile(r"alert-successsurface$", re.I), "semantic.color.status.success.default"),
    (re.compile(r"dial-ring$|dial-disabled-ring$", re.I), "semantic.color.border.secondary"),
    (re.compile(r"dial-notch$", re.I), "semantic.color.border.default"),
    (re.compile(r"dial-indicator$|dial-foreground$", re.I), "semantic.color.text.primary"),
    (re.compile(r"dial-hub$", re.I), "semantic.color.background.page"),
    (re.compile(r"dial-disabled-indicator$", re.I), "semantic.color.text.disabled"),
    (re.compile(r"dial-disabled-hub$", re.I), "semantic.color.background.disabled"),
    (re.compile(r"text-browser-content-link$", re.I), "semantic.color.interactive.brand.default"),
    (re.compile(r"text-browser-content-code-background$", re.I), "semantic.color.background.surface"),
    (re.compile(r"text-browser-state-default-text$|lcd-number-state-default-digit$|label-state-default-text$|calendar-text$|graphics-view-text$", re.I), "semantic.color.text.primary"),
    (re.compile(r"text-browser-state-disabled-text$|lcd-number-state-disabled-digit$|label-state-disabled-text$|calendar-disabled-text$|graphics-view-disabled-text$", re.I), "semantic.color.text.disabled"),
    (re.compile(r"calendar-item-today-border$|calendar-item-selected-background$|graphics-view-item-stroke$|graphics-view-selection-stroke$", re.I), "semantic.color.interactive.brand.default"),
    (re.compile(r"calendar-item-selected-text$", re.I), "semantic.color.text.invert"),
    (re.compile(r"calendar-header-weekday-text$|calendar-item-outside-text$", re.I), "semantic.color.text.muted"),
    (re.compile(r"calendar-grid-border$", re.I), "semantic.color.border.secondary"),
    (re.compile(r"calendar-default-background$", re.I), "semantic.color.background.page"),
    (re.compile(r"calendar-default-border$", re.I), "semantic.color.border.default"),
    (re.compile(r"calendar-disabled-background$", re.I), "semantic.color.background.disabled"),
    (re.compile(r"calendar-disabled-border$", re.I), "semantic.color.border.secondary"),
    (re.compile(r"calendar-radius$|text-browser-radius$|graphics-view-radius$|lcd-number-radius$", re.I), "semantic.radius.control-default"),
    (re.compile(r"calendar-border-width$|text-browser-border-width$|graphics-view-border-width$|lcd-number-border-width$", re.I), "semantic.border-width.boundary-default"),
    (re.compile(r"text-browser-default-background$|graphics-view-default-background$|lcd-number-default-background$", re.I), "semantic.color.background.page"),
    (re.compile(r"text-browser-default-border$|graphics-view-default-border$|lcd-number-default-border$", re.I), "semantic.color.border.default"),
    (re.compile(r"text-browser-disabled-background$|graphics-view-disabled-background$|lcd-number-disabled-background$", re.I), "semantic.color.background.disabled"),
    (re.compile(r"text-browser-disabled-border$|graphics-view-disabled-border$|lcd-number-disabled-border$", re.I), "semantic.color.border.secondary"),
    (re.compile(r"graphics-view-item-fill$", re.I), "semantic.color.interactive.brand.subtle"),
    (re.compile(r"graphics-view-scene-background$", re.I), "semantic.color.background.surface"),
    (re.compile(r"spin-box-affix$|double-spin-box-affix$|spin-box-divider$|double-spin-box-divider$|spin-box-control-background$|double-spin-box-control-background$", re.I), "semantic.color.background.surface"),
    (re.compile(r"spin-box-foreground$|double-spin-box-foreground$|date-picker-foreground$|time-picker-foreground$|date-time-picker-foreground$", re.I), "semantic.color.text.primary"),
    (re.compile(r"spin-box-background$|double-spin-box-background$|date-picker-background$|time-picker-background$|date-time-picker-background$", re.I), "semantic.color.background.page"),
    (re.compile(r"spin-box-border$|double-spin-box-border$|date-picker-border$|time-picker-border$|date-time-picker-border$", re.I), "semantic.color.border.default"),
    (re.compile(r"spin-box-focus-border$|double-spin-box-focus-border$|date-picker-focus-border$|time-picker-focus-border$|date-time-picker-focus-border$", re.I), "semantic.color.interactive.brand.focus-ring"),
    (re.compile(r"spin-box-read-only-background$|double-spin-box-read-only-background$|date-picker-read-only-background$|time-picker-read-only-background$|date-time-picker-read-only-background$|spin-box-disabled-background$|double-spin-box-disabled-background$|date-picker-disabled-background$|time-picker-disabled-background$|date-time-picker-disabled-background$", re.I), "semantic.color.background.disabled"),
    (re.compile(r"spin-box-disabled-foreground$|double-spin-box-disabled-foreground$|date-picker-disabled-foreground$|time-picker-disabled-foreground$|date-time-picker-disabled-foreground$", re.I), "semantic.color.text.disabled"),
    (re.compile(r"spin-box-disabled-border$|double-spin-box-disabled-border$|date-picker-disabled-border$|time-picker-disabled-border$|date-time-picker-disabled-border$", re.I), "semantic.color.border.secondary"),
    (re.compile(r"column-view-divider$", re.I), "semantic.color.border.secondary"),
    (re.compile(r"integration-card-background$|metric-card-background$|navbar-background$|sidebar-background$", re.I), "semantic.color.background.page"),
    (re.compile(r"integration-card-border$|metric-card-border$|navbar-border$|sidebar-border$|card-border$", re.I), "semantic.color.border.secondary"),
    (re.compile(r"card-title-text$", re.I), "semantic.color.text.primary"),
    (re.compile(r"card-body-text$", re.I), "semantic.color.text.muted"),
    (re.compile(r"card-background$", re.I), "semantic.color.background.page"),
    (re.compile(r"card-soft-background$", re.I), "semantic.color.background.surface"),
    (re.compile(r"card-border-width$", re.I), "semantic.border-width.boundary-default"),
    (re.compile(r"sidebar-nav-item-active-text$", re.I), "semantic.color.interactive.brand.default"),
    (re.compile(r"sidebar-nav-item-active-bg$", re.I), "semantic.color.background.surface"),
    (re.compile(r"sidebar-nav-item-text$", re.I), "semantic.color.text.primary"),
    (re.compile(r"sidebar-nav-item-radius$", re.I), "semantic.radius.control-default"),
    (re.compile(r"skeleton-title-height$", re.I), "semantic.size.height.md"),
    (re.compile(r"skeleton-line-height$", re.I), "semantic.typography.caption.sm.line-height"),
    (re.compile(r"column-view-background$", re.I), "semantic.color.background.page"),
    (re.compile(r"column-view-preview-background$", re.I), "semantic.color.background.surface"),
    (re.compile(r"undo-view-clean-marker$", re.I), "semantic.color.interactive.brand.default"),
    (re.compile(r"undo-view-empty-text$", re.I), "semantic.color.text.muted"),
]


def norm_color(value):
    if isinstance(value, str) and value.startswith("#"):
        m = HEX.match(value)
        if m:
            hexbody = m.group(1)
            if len(hexbody) == 8:
                return "#" + hexbody[:6], int(hexbody[6:8], 16)
            return "#" + hexbody.upper(), 255
    return None


def norm_number(value):
    if isinstance(value, (int, float)):
        return float(value)
    return None


def main() -> int:
    from v2.output_guard import require_build_workspace
    require_build_workspace(OUT)
    audit = json.loads(AUDIT.read_text(encoding="utf-8"))
    model = TokenModel(ROOT / "DesignSystem/source")
    context = model.context("standard", "light")
    resolved = Resolver(model, context).resolve_all()

    # adapter-scope targets (computed Qt adapter values, not canonical leaves)
    from v2.adapters.qt_runtime import adapter_values
    adapter_index = adapter_values(resolved)

    def target_value(target: str):
        if target in resolved:
            return qt_value(resolved[target])
        if target in adapter_index:
            return adapter_index[target]
        return None

    # canonical value index: normalized value -> list of paths (semantic first)
    value_index: dict[tuple, list[str]] = {}
    for path, record in resolved.items():
        value = qt_value(record)
        key = None
        color = norm_color(value)
        if color:
            key = ("color", color[0])
        else:
            number = norm_number(value)
            if number is not None:
                key = ("number", number)
        if key is not None:
            value_index.setdefault(key, []).append(path)

    def curated_lookup(name: str) -> str | None:
        for pattern, canonical in CURATED:
            match = pattern.search(name)
            if match:
                target = match.expand(canonical) if "\\" in canonical else canonical
                if target.startswith("component."):
                    target = (target.replace(".sm.", ".small.").replace(".md.", ".medium.")
                              .replace(".lg.", ".large."))
                return target
        return None

    components = []
    stats = {"aligned": 0, "alignedPrimitive": 0, "delta": 0, "unmapped": 0,
             "isolated": 0, "bindings": 0}
    for page_id, page in sorted(audit["pages"].items()):
        bindings = []
        for family, entries in sorted(page.get("bindings", {}).items()):
            for name, value in sorted(entries.items()):
                if family == "F":
                    bindings.append({"variable": name, "family": "F", "value": value,
                                     "status": "isolated",
                                     "reason": "antd 遗留变量，不属于 ComponentLab token 体系"})
                    stats["isolated"] += 1
                    continue
                target = curated_lookup(name)
                if target is not None and target_value(target) is None:
                    target = None
                if target is None:
                    key = None
                    color = norm_color(value)
                    if color:
                        key = ("color", color[0])
                        candidates = [p for p in value_index.get(key, [])
                                      if p.startswith("semantic.")]
                        target = candidates[0] if candidates else (
                            value_index.get(key, [None])[0])
                    else:
                        number = norm_number(value)
                        if number is not None:
                            for prefix in ("semantic.size.", "primitive.dimension.",
                                           "primitive.spacing."):
                                candidates = [p for p in
                                              value_index.get(("number", number), [])
                                              if p.startswith(prefix)]
                                if candidates:
                                    target = candidates[0]
                                    break
                if target is None or target_value(target) is None:
                    bindings.append({"variable": name, "family": family, "value": value,
                                     "status": "unmapped",
                                     "reason": "设计自有几何/色值，V2 无对应角色，需裁决"})
                    stats["unmapped"] += 1
                    continue
                canonical_value = target_value(target)
                aligned = norm_color(value) and norm_color(canonical_value) and \
                    norm_color(value)[0] == norm_color(canonical_value)[0] or \
                    (norm_number(value) is not None and norm_number(canonical_value) is not None
                     and norm_number(value) == norm_number(canonical_value))
                if aligned:
                    stats["aligned"] += 1
                    status = "aligned"
                else:
                    stats["delta"] += 1
                    status = "delta"
                bindings.append({
                    "variable": name, "family": family, "value": value,
                    "canonicalPath": target, "canonicalValue": canonical_value,
                    "status": status,
                    "note": "" if aligned else "Figma 侧需按 V2 规范值重绑定（见 rebind plan）",
                })
                stats["bindings"] += 1
        components.append({
            "componentId": page["component"],
            "figmaPageId": page_id,
            "variantAxes": page.get("variants", {}),
            "bindings": bindings,
        })
    payload = {
        "$description": "Figma 变量组与组件映射全量替换（V2 架构）。aligned=设计与规范值一致；delta=Figma 侧待重绑定；unmapped=需人工裁决（设计自有值）；isolated=antd 遗留。变体轴与状态值即组件状态表来源。",
        "figmaFileKey": audit["figmaFileKey"],
        "generatedFrom": "DesignSystem/config/figma-audit-v2.json",
        "canonicalSourceRevision": model.source_revision()[:12],
        "statistics": stats,
        "components": components,
    }
    OUT.write_text(json.dumps(payload, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(f"figma-component-map-v2: {len(components)} components, "
          f"bindings={stats['bindings']} aligned={stats['aligned']} "
          f"delta={stats['delta']} unmapped={stats['unmapped']} isolated={stats['isolated']}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
