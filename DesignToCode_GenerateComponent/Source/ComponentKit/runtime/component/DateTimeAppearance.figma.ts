// url=https://www.figma.com/design/w92BKW1EcutKmTXkwzNHbf/Design-Components-Build?node-id=465-189
// source=DateTimeAppearance.h
// component=commonWidget::configureFigmaTimePicker
import figma from 'figma'

const instance = figma.selectedInstance
const format = instance.getEnum('Format', {
  '24h': '24h',
  '12h': '12h',
})
const state = instance.getEnum('State', {
  Default: 'Default',
  Focus: 'Focus',
  ReadOnly: 'ReadOnly',
  Disabled: 'Disabled',
})
const showSeconds = instance.getBoolean('Show seconds', { true: 'true', false: 'false' })

export default {
  example: figma.code`
QMap<QString, QString> properties{
    {"Format", "${format}"},
    {"State", "${state}"},
    {"Show seconds", "${showSeconds}"},
};
commonWidget::configureFigmaTimePicker(timePicker, properties);
// Hour, Minute and Second are preview text; QTime remains the runtime authority.
  `,
  imports: [
    '#include "DateTimeAppearance.h"',
    '#include <QMap>',
    '#include <QTimeEdit>',
  ],
  id: 'componentlab-time-picker',
  metadata: { nestable: true, props: {} },
}
