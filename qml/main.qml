import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/**
 * @file main.qml
 * @brief Головне вікно програми з бічною навігацією та стеком контенту.
 */
ApplicationWindow {
    id: window
    visible: true
    width: 1200
    height: 800
    title: "Графік нарядів ВЧ"

    // Визначення глобальних кольорів та стилів
    readonly property color primaryColor: "#F39200"
    readonly property color textColor: "#333333"
    readonly property color secondaryTextColor: "#7F8C8D"
    readonly property color borderColor: "#DCDCDC"

    // Посилання на об'єкти для доступу з інших компонентів
    property var personnelListView: null

    background: Rectangle {
        color: "#f0f2f5"
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // Бічна панель навігації у стилі плаваючої карти (звужена)
        Rectangle {
            Layout.fillHeight: true
            Layout.preferredWidth: 230
            Layout.margins: 15
            color: "white"
            radius: 8
            border.color: "#dddddd"
            border.width: 1
            clip: true

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 5
                spacing: 0

                // Список пунктів меню з прокруткою
                ScrollView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    Layout.topMargin: 10

                    ColumnLayout {
                        id: menuColumn
                        width: parent.width
                        spacing: 0

                        ButtonGroup {
                            id: navGroup
                        }

                        Repeater {
                            model: ListModel {
                                id: navModel
                                ListElement {
                                    title: "Особовий склад"
                                    sIdx: 0
                                    expanded: true
                                }
                                ListElement {
                                    title: "Види нарядів"
                                    sIdx: 1
                                    expanded: false
                                }
                                ListElement {
                                    title: "Графік нарядів"
                                    sIdx: 2
                                    expanded: false
                                }
                                ListElement {
                                    title: "Статистика"
                                    sIdx: 3
                                    expanded: false
                                }
                            }

                            delegate: ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 0

                                // Основна кнопка розділу
                                NavButton {
                                    text: model.title
                                    checked: contentStack.currentIndex === model.sIdx
                                    onClicked: {
                                        contentStack.currentIndex = model.sIdx;
                                        // Перемикання стану розгортання незалежно для кожного розділу
                                        navModel.setProperty(index, "expanded", !model.expanded);
                                    }
                                }

                                // Контейнер для підпунктів з анімацією згортання
                                ColumnLayout {
                                    id: subMenuContainer
                                    Layout.fillWidth: true
                                    Layout.leftMargin: 35 // Зміщення додаткових розділів вправо
                                    spacing: 2
                                    clip: true

                                    property bool isExpanded: model.expanded

                                    Layout.preferredHeight: isExpanded ? -1 : 0
                                    opacity: isExpanded ? 1 : 0
                                    visible: opacity > 0

                                    Behavior on Layout.preferredHeight {
                                        NumberAnimation {
                                            duration: 250
                                            easing.type: Easing.InOutQuad
                                        }
                                    }
                                    Behavior on opacity {
                                        NumberAnimation {
                                            duration: 200
                                        }
                                    }

                                    // Підпункти для "Особовий склад"
                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        spacing: 2
                                        visible: model.sIdx === 0

                                        ActionButton {
                                            text: "Додати в/с"
                                            onClicked: {
                                                /* Перемикання на вкладку "Особовий склад" та відкриття діалогу додавання */
                                                contentStack.currentIndex = model.sIdx;
                                                personnelView.openAddDialog();
                                            }
                                        }
                                        ActionButton {
                                            text: "Змінити в/с"
                                            enabled: !!PersonnelController && PersonnelController.selectedPersonId !== -1
                                            onClicked: {
                                                /* Перемикання на вкладку "Особовий склад" та відкриття діалогу редагування */
                                                contentStack.currentIndex = model.sIdx;
                                                personnelView.openEditDialog();
                                            }
                                        }
                                        ActionButton {
                                            text: "Видалити в/с"
                                            enabled: !!PersonnelController && PersonnelController.selectedPersonId !== -1
                                            onClicked: {
                                                /* Перемикання на вкладку "Особовий склад" та відкриття діалогу видалення */
                                                contentStack.currentIndex = model.sIdx;
                                                personnelView.openDeleteDialog();
                                            }
                                        }

                                        // Вкладене меню статусів
                                        ColumnLayout {
                                            Layout.fillWidth: true
                                            spacing: 2

                                            ActionButton {
                                                id: statusBaseBtn
                                                text: "Статус в/с"
                                                enabled: !!PersonnelController && PersonnelController.selectedPersonId !== -1
                                                property bool subExpanded: false
                                                onClicked: {
                                                    /* Перемикання на вкладку та розгортання меню статусів */
                                                    contentStack.currentIndex = model.sIdx;
                                                    subExpanded = !subExpanded;
                                                }
                                            }

                                            ColumnLayout {
                                                Layout.fillWidth: true
                                                Layout.leftMargin: 15
                                                spacing: 2
                                                visible: statusBaseBtn.subExpanded && statusBaseBtn.enabled

                                                ActionButton {
                                                    text: "Додати статус"
                                                    onClicked: {
                                                        /* Перемикання на вкладку та додавання статусу */
                                                        contentStack.currentIndex = model.sIdx;
                                                        personnelView.openStatusMainDialog("add");
                                                    }
                                                }
                                                ActionButton {
                                                    text: "Змінити статус"
                                                    onClicked: {
                                                        /* Перемикання на вкладку та зміна статусу */
                                                        contentStack.currentIndex = model.sIdx;
                                                        personnelView.openStatusMainDialog("edit");
                                                    }
                                                }
                                                ActionButton {
                                                    text: "Перегляд статусів"
                                                    onClicked: {
                                                        /* Перемикання на вкладку та перегляд статусів */
                                                        contentStack.currentIndex = model.sIdx;
                                                        personnelView.openStatusMainDialog("view");
                                                    }
                                                }
                                                ActionButton {
                                                    text: "Видалити статус"
                                                    onClicked: {
                                                        /* Перемикання на вкладку та видалення статусу */
                                                        contentStack.currentIndex = model.sIdx;
                                                        personnelView.openStatusMainDialog("delete");
                                                    }
                                                }
                                            }
                                        }
                                    }

                                    // Підпункти для "Види нарядів"
                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        spacing: 2
                                        visible: model.sIdx === 1

                                        ActionButton {
                                            text: "Додати наряд"
                                            onClicked: {
                                                /* Перемикання на вкладку "Види нарядів" та відкриття діалогу додавання */
                                                contentStack.currentIndex = model.sIdx;
                                                dutyTypesView.openAddDialog();
                                            }
                                        }
                                        ActionButton {
                                            text: "Редагувати наряд"
                                            enabled: !!DutyTypesController && DutyTypesController.selectedDutyTypeId !== -1
                                            onClicked: {
                                                /* Перемикання на вкладку "Види нарядів" та відкриття діалогу редагування */
                                                contentStack.currentIndex = model.sIdx;
                                                dutyTypesView.openEditDialog();
                                            }
                                        }
                                        ActionButton {
                                            text: "Видалити наряд"
                                            enabled: !!DutyTypesController && DutyTypesController.selectedDutyTypeId !== -1
                                            onClicked: {
                                                /* Перемикання на вкладку "Види нарядів" та відкриття діалогу видалення */
                                                contentStack.currentIndex = model.sIdx;
                                                dutyTypesView.openDeleteDialog();
                                            }
                                        }
                                    }

                                    // Підпункти для "Графік нарядів"
                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        spacing: 2
                                        visible: model.sIdx === 2

                                        ActionButton {
                                            text: "Авто-генерація"
                                            onClicked: {
                                                /* Перемикання на вкладку "Графік нарядів" та запуск авто-генерації */
                                                contentStack.currentIndex = model.sIdx;
                                                scheduleView.openAutoGenDialog();
                                            }
                                        }
                                        ActionButton {
                                            text: "Змінити графік"
                                            onClicked: {
                                                /* Перемикання на вкладку "Графік нарядів" та відкриття діалогу зміни */
                                                contentStack.currentIndex = model.sIdx;
                                                scheduleView.openEditScheduleDialog();
                                            }
                                        }
                                        ActionButton {
                                            text: "Показати графік"
                                            onClicked: {
                                                /* Перемикання на вкладку "Графік нарядів" та відкриття діалогу вибору графіка */
                                                contentStack.currentIndex = model.sIdx;
                                                scheduleView.openShowScheduleDialog();
                                            }
                                        }
                                        ActionButton {
                                            text: "Замінити в/с"
                                            enabled: scheduleView.isPersonnelSelected
                                            onClicked: {
                                                /* Перемикання на вкладку "Графік нарядів" та відкриття діалогу заміни в/с */
                                                contentStack.currentIndex = model.sIdx;
                                                scheduleView.openReplacePersonnelDialog();
                                            }
                                        }
                                        ActionButton {
                                            text: "Експорт Excel"
                                            onClicked: {
                                                /* Перемикання на вкладку та експорт у Excel */
                                                contentStack.currentIndex = model.sIdx;
                                                scheduleView.exportExcel();
                                            }
                                        }
                                        ActionButton {
                                            text: "Експорт PDF"
                                            onClicked: {
                                                /* Перемикання на вкладку та експорт у PDF */
                                                contentStack.currentIndex = model.sIdx;
                                                scheduleView.exportPdf();
                                            }
                                        }
                                    }

                                    // Підпункти для "Статистика"
                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        spacing: 2
                                        visible: model.sIdx === 3

                                        ActionButton {
                                            text: "Відобразити статистику"
                                            onClicked: {
                                                /* Перемикання на вкладку "Статистика" та відкриття діалогу */
                                                contentStack.currentIndex = model.sIdx;
                                                statisticsView.openShowStatsDialog();
                                            }
                                        }
                                        ActionButton {
                                            text: "Експорт Excel"
                                            onClicked: {
                                                /* Перемикання на вкладку та експорт статистики у Excel */
                                                contentStack.currentIndex = model.sIdx;
                                                statisticsView.exportExcel();
                                            }
                                        }
                                        ActionButton {
                                            text: "Експорт PDF"
                                            onClicked: {
                                                /* Перемикання на вкладку та експорт статистики у PDF */
                                                contentStack.currentIndex = model.sIdx;
                                                statisticsView.exportPdf();
                                            }
                                        }
                                    }
                                }

                                Item {
                                    Layout.preferredHeight: 5
                                }
                            }
                        }
                    }
                }

                Item {
                    Layout.fillHeight: true
                }

                // Інформація про версію
                Text {
                    text: "v2.0.0"
                    color: secondaryTextColor
                    font.pixelSize: 11
                    Layout.alignment: Qt.AlignHCenter
                    Layout.bottomMargin: 10
                }
            }
        }

        // Основний контент програми
        StackLayout {
            id: contentStack
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: 0

            PersonnelView {
                id: personnelView
                Component.onCompleted: window.personnelListView = listView
            }
            DutyTypesView {
                id: dutyTypesView
            }
            ScheduleView {
                id: scheduleView
            }
            StatisticsView {
                id: statisticsView
            }
        }
    }

    // --- Кастомні компоненти ---

    /**
     * @component NavButton
     * @brief Основна кнопка навігації для бічної панелі.
     */
    component NavButton: Button {
        Layout.fillWidth: true
        Layout.preferredHeight: 50
        checkable: true
        ButtonGroup.group: navGroup

        contentItem: Text {
            text: parent.text
            color: parent.checked ? primaryColor : (parent.hovered ? primaryColor : textColor)
            font.pixelSize: 16
            font.bold: parent.checked
            verticalAlignment: Text.AlignVCenter
            leftPadding: 15
            Behavior on color {
                ColorAnimation {
                    duration: 150
                }
            }
        }

        background: Rectangle {
            color: parent.hovered ? "#f8f9fa" : "transparent"
            radius: 6
            anchors.fill: parent
            anchors.margins: 4

            Rectangle {
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                height: parent.height * 0.6
                width: 4
                radius: 2
                color: primaryColor
                visible: parent.parent.checked
            }
        }
    }

    /**
     * @component ActionButton
     * @brief Кнопка для додаткових дій у підменю.
     */
    component ActionButton: Button {
        Layout.fillWidth: true
        Layout.preferredHeight: 32
        Layout.rightMargin: 10
        flat: true
        hoverEnabled: enabled

        contentItem: Text {
            text: parent.text
            color: parent.enabled ? (parent.hovered ? primaryColor : textColor) : "#BDC3C7"
            font.pixelSize: 13
            verticalAlignment: Text.AlignVCenter
            leftPadding: 10
            Behavior on color {
                ColorAnimation {
                    duration: 150
                }
            }
        }

        background: Rectangle {
            color: parent.hovered ? "#f0f2f5" : "transparent"
            radius: 4
        }
    }
}
