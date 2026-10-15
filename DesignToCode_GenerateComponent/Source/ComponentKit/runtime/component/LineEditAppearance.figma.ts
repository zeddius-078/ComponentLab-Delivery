// url=https://www.figma.com/design/w92BKW1EcutKmTXkwzNHbf/Design-Components-Build?node-id=1116-18
// source=LineEditAppearance.h
// component=commonWidget::configureFigmaLineEdit
import figma from 'figma'

const instance = figma.selectedInstance
const size = instance.getEnum('Size', {
  Small: 'Small',
  Medium: 'Medium',
  Large: 'Large',
})
const state = instance.getEnum('State', {
  Default: 'Default',
  Hover: 'Hover',
  Focus: 'Focus',
  Filled: 'Filled',
  Error: 'Error',
  Disabled: 'Disabled',
})
const value = instance.getString('Value')
const placeholder = instance.getString('Placeholder')
const showPrefix = instance.getBoolean('Show prefix', { true: 'true', false: 'false' })
const showSuffix = instance.getBoolean('Show suffix', { true: 'true', false: 'false' })
const allowClear = instance.getBoolean('Allow clear', { true: 'true', false: 'false' })

export default {
  example: figma.code`
QMap<QString, QString> properties{
    {"Size", "${size}"},
    {"State", "${state}"},
    {"Show prefix", "${showPrefix}"},
    {"Show suffix", "${showSuffix}"},
    {"Allow clear", "${allowClear}"},
};
lineEdit->setText(QString::fromUtf8("${value}"));
lineEdit->setPlaceholderText(QString::fromUtf8("${placeholder}"));
commonWidget::configureFigmaLineEdit(lineEdit, properties);
// Prefix icon, Suffix icon and Clear icon remain native QAction identities;
// the current SDK contract maps their visibility, not Figma instance swaps.
  `,
  imports: [
    '#include "LineEditAppearance.h"',
    '#include <QLineEdit>',
    '#include <QMap>',
  ],
  id: 'componentlab-line-edit',
  metadata: { nestable: true, props: {} },
}
