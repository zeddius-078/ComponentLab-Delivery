// url=https://www.figma.com/design/w92BKW1EcutKmTXkwzNHbf/Design-Components-Build?node-id=467-195
// source=SliderAppearance.h
// component=commonWidget::configureFigmaSlider
import figma from 'figma'

const instance = figma.selectedInstance
const orientation = instance.getEnum('Orientation', {
  Horizontal: 'Horizontal',
  Vertical: 'Vertical',
})
const state = instance.getEnum('State', {
  Default: 'Default',
  Focus: 'Focus',
  Dragging: 'Dragging',
  Disabled: 'Disabled',
})
const valueLabel = instance.getString('Value label')
const showValue = instance.getBoolean('Show value', { true: 'true', false: 'false' })
const showTicks = instance.getBoolean('Show ticks', { true: 'true', false: 'false' })

export default {
  example: figma.code`
QMap<QString, QString> properties{
    {"Orientation", "${orientation}"},
    {"State", "${state}"},
    {"Value label", "${valueLabel}"},
    {"Show value", "${showValue}"},
    {"Show ticks", "${showTicks}"},
};
commonWidget::configureFigmaSlider(slider, properties);
  `,
  imports: [
    '#include "SliderAppearance.h"',
    '#include <QMap>',
    '#include <QSlider>',
  ],
  id: 'componentlab-slider',
  metadata: { nestable: true, props: {} },
}
