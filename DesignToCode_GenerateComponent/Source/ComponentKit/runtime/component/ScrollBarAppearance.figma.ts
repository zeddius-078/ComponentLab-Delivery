// url=https://www.figma.com/design/w92BKW1EcutKmTXkwzNHbf/Design-Components-Build?node-id=471-477
// source=ScrollBarAppearance.h
// component=commonWidget::configureFigmaScrollBar
import figma from 'figma'

const instance = figma.selectedInstance
const orientation = instance.getEnum('Orientation', {
  Horizontal: 'Horizontal',
  Vertical: 'Vertical',
})
const state = instance.getEnum('State', {
  Default: 'Default',
  Hover: 'Hover',
  Dragging: 'Dragging',
  Disabled: 'Disabled',
})
const showLineButtons = instance.getBoolean('Show line buttons', {
  true: 'true',
  false: 'false',
})

export default {
  example: figma.code`
QMap<QString, QString> properties{
    {"Orientation", "${orientation}"},
    {"State", "${state}"},
    {"Show line buttons", "${showLineButtons}"},
};
commonWidget::configureFigmaScrollBar(scrollBar, properties);
  `,
  imports: [
    '#include "ScrollBarAppearance.h"',
    '#include <QMap>',
    '#include <QScrollBar>',
  ],
  id: 'componentlab-scroll-bar',
  metadata: { nestable: true, props: {} },
}
