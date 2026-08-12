import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/**
 * @file DutyTypesView.qml
 * @brief Сторінка управління видами нарядів.
 * Представляє список нарядів у вигляді таблиці з можливістю вибору.
 */
Page {
    id: root

    // --- Властивості та стани ---
    property alias listView: listView
    property bool isEditing: false
    property int editId: -1

    // --- Моделі даних ---
    ListModel { id: dutyTypesModel }

    // --- Функції управління ---

    /**
     * @brief Оновлює список видів нарядів з контролера.
     */
    function refreshData() {
        var selectedId = DutyTypesController.selectedDutyTypeId
        dutyTypesModel.clear()
        var data = DutyTypesController.getDutyTypes()
        var newIndex = -1
        for (var i = 0; i < data.length; i++) {
            dutyTypesModel.append(data[i])
            if (data[i].id === selectedId) {
                newIndex = i
            }
        }
        listView.currentIndex = newIndex
    }

    /**
     * @brief Відкриває діалог для додавання нового наряду.
     */
    function openAddDialog() {
        isEditing = false
        addDutyDialog.title = "Новий вид наряду"
        dutyNameField.text = ""
        minRankCombo.currentIndex = 0
        maxRankCombo.currentIndex = ranksModel.count - 1
        restDaysSpin.value = 1
        personCountSpin.value = 1
        addDutyDialog.open()
    }

    /**
     * @brief Відкриває діалог для редагування вибраного наряду.
     */
    function openEditDialog() {
        var currentItem = dutyTypesModel.get(listView.currentIndex)
        if (currentItem) {
            isEditing = true
            editId = currentItem.id
            addDutyDialog.title = "Редагувати наряд"
            dutyNameField.text = currentItem.name
            minRankCombo.currentIndex = findRankIndex(currentItem.minRank)
            maxRankCombo.currentIndex = findRankIndex(currentItem.maxRank)
            restDaysSpin.value = currentItem.restDays
            personCountSpin.value = currentItem.personCount
            addDutyDialog.open()
        }
    }

    /**
     * @brief Відкриває вікно підтвердження видалення.
     */
    function openDeleteDialog() {
        var currentItem = dutyTypesModel.get(listView.currentIndex)
        if (currentItem) {
            confirmDeleteDialog.dutyId = currentItem.id
            confirmDeleteDialog.open()
        }
    }

    /**
     * @brief Допоміжна функція для пошуку індексу звання.
     */
    function findRankIndex(rankName) {
        for (var i = 0; i < ranksModel.count; i++) {
            if (ranksModel.get(i).name === rankName) return i
        }
        return 0
    }

    // --- Життєвий цикл та сигнали ---

    Component.onCompleted: refreshData()

    Connections {
        target: DutyTypesController
        function onDutyTypesChanged() { refreshData() }
    }

    // --- Спільні компоненти ---

    component DialogButton: Button {
        id: dbBtn
        Layout.fillWidth: true
        Layout.preferredHeight: 40
        hoverEnabled: enabled
        background: Rectangle {
            radius: 8
            color: !dbBtn.enabled ? "white" : (dbBtn.hovered ? "#F39200" : "white")
            border.color: !dbBtn.enabled ? "#E0E0E0" : (dbBtn.hovered ? "#F39200" : "#BDC3C7")
            border.width: 1
        }
        contentItem: Text {
            text: dbBtn.text
            font.bold: true
            color: !dbBtn.enabled ? "#BDC3C7" : (dbBtn.hovered ? "white" : "#2C3E50")
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
    }

    // --- Головна верстка ---

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 15
        spacing: 15

        // Таблиця видів нарядів (дизайн як у особового складу)
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "white"
            border.color: "#dddddd"
            radius: 8
            clip: true

            ListView {
                id: listView
                anchors.fill: parent
                anchors.margins: 1
                model: dutyTypesModel
                clip: true
                spacing: 0
                focus: true
                currentIndex: -1

                headerPositioning: ListView.OverlayHeader
                header: Rectangle {
                    width: listView.width; height: 40; color: "#f2f2f2"; z: 2
                    RowLayout {
                        anchors.fill: parent; spacing: 0
                        Text { text: "№"; font.bold: true; Layout.preferredWidth: 60; horizontalAlignment: Text.AlignHCenter }
                        Text { text: "Назва наряду"; font.bold: true; Layout.fillWidth: true; leftPadding: 10 }
                        Text { text: "Мін. звання"; font.bold: true; Layout.minimumWidth: 150; Layout.preferredWidth: 150; leftPadding: 10 }
                        Text { text: "Макс. звання"; font.bold: true; Layout.preferredWidth: 150; leftPadding: 10 }
                        Text { text: "Дні відп."; font.bold: true; Layout.preferredWidth: 100; horizontalAlignment: Text.AlignHCenter }
                        Text { text: "К-сть осіб"; font.bold: true; Layout.preferredWidth: 100; horizontalAlignment: Text.AlignHCenter }
                    }
                }

                delegate: ItemDelegate {
                    id: delegateItem
                    width: listView.width
                    height: 40
                    onClicked: {
                        listView.currentIndex = index
                        DutyTypesController.selectedDutyTypeId = model.id
                    }

                    background: Rectangle {
                        color: "white"
                        border.color: "#eeeeee"
                        border.width: 1
                    }

                    contentItem: RowLayout {
                        anchors.fill: parent
                        spacing: 0

                        Text {
                            text: index + 1
                            Layout.preferredWidth: 60
                            horizontalAlignment: Text.AlignHCenter
                            color: delegateItem.ListView.isCurrentItem ? "#F39200" : "black"
                        }
                        Text {
                            text: name
                            Layout.fillWidth: true
                            leftPadding: 10
                            color: delegateItem.ListView.isCurrentItem ? "#F39200" : "black"
                        }
                        Text {
                            text: minRank || "Не обм."
                            Layout.minimumWidth: 150 // ДОБАВЛЯЕМ: Жесткий лимит против сжатия колонки!
                            Layout.preferredWidth: 150
                            leftPadding: 10
                            elide: Text.ElideRight   // ДОБАВЛЯЕМ: Чтобы длинное звание уходило в три точки, а не сжималось
                            color: delegateItem.ListView.isCurrentItem ? "#F39200" : "black"
                        }
                        Text {
                            text: maxRank || "Не обм."
                            Layout.preferredWidth: 150
                            leftPadding: 10
                            color: delegateItem.ListView.isCurrentItem ? "#F39200" : "black"
                        }
                        Text {
                            text: restDays
                            Layout.preferredWidth: 100
                            horizontalAlignment: Text.AlignHCenter
                            color: delegateItem.ListView.isCurrentItem ? "#F39200" : "black"
                        }
                        Text {
                            text: personCount
                            Layout.preferredWidth: 100
                            horizontalAlignment: Text.AlignHCenter
                            color: delegateItem.ListView.isCurrentItem ? "#F39200" : "black"
                        }
                    }
                }
            }
        }
    }

    // --- Діалоги ---

    // Діалог додавання/редагування наряду
    Dialog {
        id: addDutyDialog; width: 450; padding: 20; modal: true; focus: true
        x: Math.round((parent.width - width) / 2); y: Math.round((parent.height - height) / 2)
        background: Rectangle { color: "white"; radius: 12; border.color: "#dcdcdc"; border.width: 1 }
        header: Label { text: addDutyDialog.title; font.pixelSize: 22; font.bold: true; color: "#2C3E50"; padding: 20; horizontalAlignment: Text.AlignHCenter }
        contentItem: ColumnLayout {
            spacing: 15

            ColumnLayout {
                Layout.fillWidth: true; spacing: 5
                Label { text: "Назва наряду:"; font.bold: true; color: "#34495E" }
                TextField {
                    id: dutyNameField; Layout.fillWidth: true; Layout.preferredHeight: 40
                    background: Rectangle { radius: 8; border.color: dutyNameField.activeFocus ? "#F39200" : "#BDC3C7"; border.width: 1 }
                }
            }

            RowLayout {
                spacing: 15
                ColumnLayout {
                    Layout.fillWidth: true; spacing: 5
                    Label { text: "Мін. звання:"; font.bold: true; color: "#34495E" }
                    ComboBox {
                        id: minRankCombo; Layout.fillWidth: true; Layout.preferredHeight: 40; textRole: "name"
                        model: ListModel { id: ranksModel }
                        Component.onCompleted: { var rs = PersonnelController.getRanks(); for (var i = 0; i < rs.length; i++) ranksModel.append(rs[i]) }
                        background: Rectangle { radius: 8; border.color: "#BDC3C7" }
                    }
                }
                ColumnLayout {
                    Layout.fillWidth: true; spacing: 5
                    Label { text: "Макс. звання:"; font.bold: true; color: "#34495E" }
                    ComboBox {
                        id: maxRankCombo; Layout.fillWidth: true; Layout.preferredHeight: 40; textRole: "name"
                        model: ranksModel
                        background: Rectangle { radius: 8; border.color: "#BDC3C7" }
                    }
                }
            }

            RowLayout {
                spacing: 15
                ColumnLayout {
                    Layout.fillWidth: true; spacing: 5
                    Label { text: "Дні відпочинку:"; font.bold: true; color: "#34495E" }
                    SpinBox { id: restDaysSpin; from: 0; to: 10; Layout.fillWidth: true; Layout.preferredHeight: 40 }
                }
                ColumnLayout {
                    Layout.fillWidth: true; spacing: 5
                    Label { text: "К-сть осіб:"; font.bold: true; color: "#34495E" }
                    SpinBox { id: personCountSpin; from: 1; to: 10; Layout.fillWidth: true; Layout.preferredHeight: 40 }
                }
            }

            RowLayout {
                Layout.fillWidth: true; spacing: 15; Layout.topMargin: 10
                DialogButton { text: "Скасувати"; onClicked: addDutyDialog.close() }
                DialogButton {
                    text: "Зберегти"
                    onClicked: {
                        if (dutyNameField.text !== "") {
                            if (isEditing) DutyTypesController.updateDutyType(editId, dutyNameField.text, minRankCombo.currentText, maxRankCombo.currentText, restDaysSpin.value, personCountSpin.value)
                            else DutyTypesController.addDutyType(dutyNameField.text, minRankCombo.currentText, maxRankCombo.currentText, restDaysSpin.value, personCountSpin.value)
                            addDutyDialog.close()
                        }
                    }
                }
            }
        }
    }

    // Підтвердження видалення наряду
    Dialog {
        id: confirmDeleteDialog; property int dutyId: -1; width: 400; padding: 25; modal: true
        x: Math.round((parent.width - width) / 2); y: Math.round((parent.height - height) / 2)
        background: Rectangle { color: "white"; radius: 12; border.color: "#dcdcdc" }
        header: Label { text: "Видалення наряду"; font.pixelSize: 22; font.bold: true; color: "#2C3E50"; padding: 20; horizontalAlignment: Text.AlignHCenter }
        contentItem: ColumnLayout {
            spacing: 25
            Label { text: "Ви впевнені, що хочете видалити цей вид наряду?"; font.pixelSize: 16; wrapMode: Text.WordWrap; Layout.fillWidth: true; horizontalAlignment: Text.AlignHCenter; color: "#34495E" }
            RowLayout {
                Layout.fillWidth: true; spacing: 15
                DialogButton { text: "Відміна"; onClicked: confirmDeleteDialog.close() }
                DialogButton { text: "Видалити"; onClicked: { DutyTypesController.deleteDutyType(confirmDeleteDialog.dutyId); confirmDeleteDialog.close() } }
            }
        }
    }
}
