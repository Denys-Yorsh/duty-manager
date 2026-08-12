import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/**
 * @file PersonnelView.qml
 * @brief Сторінка управління особовим складом.
 * Дозволяє переглядати, додавати, редагувати та видаляти військовослужбовців,
 * а також керувати їх статусами через транзакційну модель.
 */
Page {
    id: root

    // --- Властивості та стани ---
    property alias listView: listView
    property bool isEditing: false
    property int editId: -1
    property bool isEditingStatus: false
    property string statusMode: "view" // add, edit, delete, view
    property int editingStatusId: -1

    // --- Моделі даних ---
    ListModel { id: personnelModel }

    /**
     * @model statusModel
     * @brief Локальна модель статусів для модального вікна.
     * Дозволяє редагувати список без негайного збереження в БД.
     */
    ListModel { id: statusModel }

    // --- Функції управління особовим складом ---

    function refreshData() {
        var selectedId = PersonnelController.selectedPersonId
        personnelModel.clear()
        var data = PersonnelController.getPersonnelList()
        var newIndex = -1
        for (var i = 0; i < data.length; i++) {
            personnelModel.append(data[i])
            if (data[i].id === selectedId) {
                newIndex = i
            }
        }
        listView.currentIndex = newIndex
    }

    function openAddDialog() {
        isEditing = false
        addDialog.title = "Новий в/с"
        nameField.text = ""
        posField.text = ""
        addDialog.open()
    }

    function openEditDialog() {
        var currentItem = personnelModel.get(listView.currentIndex)
        if (currentItem) {
            isEditing = true
            editId = currentItem.id
            addDialog.title = "Редагувати в/с"
            nameField.text = currentItem.fullName
            posField.text = currentItem.position
            rankCombo.currentIndex = findRankIndex(currentItem.rankName)
            addDialog.open()
        }
    }

    function openDeleteDialog() {
        var currentItem = personnelModel.get(listView.currentIndex)
        if (currentItem) {
            confirmDeleteDialog.personId = currentItem.id
            confirmDeleteDialog.open()
        }
    }

    function findRankIndex(rankName) {
        for (var i = 0; i < ranksModel.count; i++) {
            if (ranksModel.get(i).name === rankName) return i
        }
        return 0
    }

    // --- Функції управління статусами (Транзакційна логіка) ---

    /**
     * @brief Відкриває основне вікно статусів у вибраному режимі.
     * Завантажує актуальні дані з БД у локальну модель.
     */
    function openStatusMainDialog(mode) {
        statusMode = mode || "view"
        statusModel.clear()
        var data = PersonnelController.getStatuses(PersonnelController.selectedPersonId)
        for (var i = 0; i < data.length; i++) {
            statusModel.append(data[i])
        }
        statusListView.currentIndex = -1
        statusMainDialog.open()
    }

    function openAddStatus() {
        isEditingStatus = false
        statusTypeCombo.currentIndex = 0
        startStatusField.text = ""
        endStatusField.text = ""
        notesStatusField.text = ""
        addStatusDialog.open()
    }

    function openEditStatus() {
        var currentItem = statusModel.get(statusListView.currentIndex)
        if (currentItem) {
            isEditingStatus = true
            editingStatusId = (currentItem.id !== undefined) ? currentItem.id : -1
            statusTypeCombo.currentIndex = statusTypeCombo.find(currentItem.statusName)
            startStatusField.text = currentItem.startDate
            endStatusField.text = currentItem.endDate
            notesStatusField.text = currentItem.notes
            addStatusDialog.open()
        }
    }

    /**
     * @brief Зберігає всі зміни статусів із локальної моделі в БД.
     */
    function applyStatusChanges() {
        var statusesToSave = []
        for (var i = 0; i < statusModel.count; i++) {
            var item = statusModel.get(i)
            statusesToSave.push({
                "statusName": item.statusName,
                "startDate": item.startDate,
                "endDate": item.endDate,
                "notes": item.notes
            })
        }
        if (PersonnelController.saveStatuses(PersonnelController.selectedPersonId, statusesToSave)) {
            statusMainDialog.close()
            refreshData()
        }
    }

    // --- Життєвий цикл та сигнали ---

    Component.onCompleted: refreshData()

    Connections {
        target: PersonnelController
        function onPersonnelChanged() { refreshData() }
    }

    // --- Спільні компоненти ---

    /**
     * @component DialogButton
     * @brief Уніфікована кнопка для використання в діалогах.
     */
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

    /**
     * @component DateSelector
     * @brief Текстове поле з випадаючим календарем для вибору дати.
     */
    component DateSelector: TextField {
        id: dateField
        placeholderText: "РРРР-ММ-ДД"
        readOnly: true
        Layout.fillWidth: true
        height: 40
        leftPadding: 10
        background: Rectangle { radius: 8; border.color: dateField.activeFocus ? "#F39200" : "#BDC3C7"; border.width: 1 }

        MouseArea { anchors.fill: parent; onClicked: datePopup.open() }

        Popup {
            id: datePopup
            y: dateField.height; width: 250; height: 300; padding: 10
            background: Rectangle { radius: 10; border.color: "#dcdcdc"; color: "white" }
            ColumnLayout {
                anchors.fill: parent
                MonthGrid {
                    id: grid; month: new Date().getMonth(); year: new Date().getFullYear(); Layout.fillWidth: true; Layout.fillHeight: true
                    onClicked: (date) => { dateField.text = Qt.formatDate(date, "yyyy-MM-dd"); datePopup.close() }
                }
                RowLayout {
                    Layout.fillWidth: true
                    Button { text: "<"; onClicked: { if (grid.month === 0) { grid.month = 11; grid.year-- } else { grid.month-- } } }
                    Label { text: Qt.formatDate(new Date(grid.year, grid.month, 1), "MMMM yyyy"); Layout.fillWidth: true; horizontalAlignment: Text.AlignHCenter }
                    Button { text: ">"; onClicked: { if (grid.month === 11) { grid.month = 0; grid.year++ } else { grid.month++ } } }
                }
            }
        }
    }

    // --- Головна верстка сторінки ---

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 15
        spacing: 15

        // Основна таблиця особового складу
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
                model: personnelModel
                clip: true
                spacing: 0
                focus: true
                currentIndex: -1

                // === ВОТ ЭТОТ БЛОК ДОБАВЛЕН ДЛЯ ПОЛОСЫ ПРОКРУТКИ ===
                ScrollBar.vertical: ScrollBar {
                    id: vScrollBar
                    active: true
                    policy: ScrollBar.AsNeeded

                    contentItem: Rectangle {
                        implicitWidth: 6
                        radius: 3
                        color: vScrollBar.pressed ? "#BDC3C7" : (vScrollBar.hovered ? "#BDC3C7" : "#BDC3C7")
                    }
                }
                // =================================================

                headerPositioning: ListView.OverlayHeader
                header: Rectangle {
                    width: listView.width; height: 40; color: "#f2f2f2"; z: 2
                    RowLayout {
                        anchors.fill: parent; spacing: 0
                        Text { text: "№"; font.bold: true; Layout.preferredWidth: 60; horizontalAlignment: Text.AlignHCenter }
                        Text { text: "Звання"; font.bold: true; Layout.minimumWidth: 150; Layout.preferredWidth: 150; leftPadding: 10 }
                        Text { text: "ПІБ"; font.bold: true; Layout.fillWidth: true; leftPadding: 10 }
                        Text { text: "Посада"; font.bold: true; Layout.preferredWidth: 200; leftPadding: 10 }
                        Text { text: "Примітки"; font.bold: true; Layout.preferredWidth: 150; leftPadding: 10 }
                    }
                }

                delegate: ItemDelegate {
                    id: delegateItem
                    width: listView.width
                    height: 40
                    onClicked: {
                        listView.currentIndex = index
                        PersonnelController.selectedPersonId = model.id
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
                            text: rankName
                            Layout.minimumWidth: 150 // Жестко запрещаем сжимать колонку меньше 150px!
                            Layout.preferredWidth: 150
                            leftPadding: 10
                            font.bold: true
                            elide: Text.ElideRight // Если звание будет совсем гигантским, оно аккуратно уйдет в три точки, не ломая верстку
                            color: delegateItem.ListView.isCurrentItem ? "#F39200" : "black"
                        }
                        Text {
                            text: fullName
                            Layout.fillWidth: true
                            leftPadding: 10
                            color: delegateItem.ListView.isCurrentItem ? "#F39200" : "black"
                        }
                        Text {
                            text: position
                            Layout.preferredWidth: 200
                            leftPadding: 10
                            color: delegateItem.ListView.isCurrentItem ? "#F39200" : "black"
                        }
                        Text {
                            text: notes || "в наявності"
                            Layout.preferredWidth: 150
                            leftPadding: 10
                            color: delegateItem.ListView.isCurrentItem ? "#F39200" : "black"
                            elide: Text.ElideRight
                        }
                    }
                }
            }
        }
    }

    // --- Діалогові вікна ---

    // Діалог додавання/редагування військовослужбовця
    Dialog {
        id: addDialog
        width: 450
        padding: 20
        modal: true
        focus: true
        x: Math.round((parent.width - width) / 2)
        y: Math.round((parent.height - height) / 2)

        background: Rectangle { color: "white"; radius: 12; border.color: "#dcdcdc"; border.width: 1 }

        header: Label {
            text: addDialog.title
            font.pixelSize: 22; font.bold: true; color: "#2C3E50"; padding: 20; horizontalAlignment: Text.AlignHCenter
        }

        contentItem: ColumnLayout {
            spacing: 20

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 5
                Label { text: "ПІБ:"; font.bold: true; color: "#34495E" }
                TextField {
                    id: nameField; placeholderText: "Прізвище, ім'я, по батькові"
                    Layout.fillWidth: true; Layout.preferredHeight: 40
                    background: Rectangle { radius: 8; border.color: nameField.activeFocus ? "#F39200" : "#BDC3C7"; border.width: 1 }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 5
                Label { text: "Звання:"; font.bold: true; color: "#34495E" }
                ComboBox {
                    id: rankCombo; Layout.fillWidth: true; Layout.preferredHeight: 40
                    textRole: "name"
                    model: ListModel { id: ranksModel }
                    Component.onCompleted: {
                        var ranks = PersonnelController.getRanks()
                        for (var i = 0; i < ranks.length; i++) ranksModel.append(ranks[i])
                    }
                    background: Rectangle { radius: 8; border.color: rankCombo.activeFocus ? "#F39200" : "#BDC3C7"; border.width: 1 }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 5
                Label { text: "Посада:"; font.bold: true; color: "#34495E" }
                TextField {
                    id: posField; placeholderText: "Посада"
                    Layout.fillWidth: true; Layout.preferredHeight: 40
                    background: Rectangle { radius: 8; border.color: posField.activeFocus ? "#F39200" : "#BDC3C7"; border.width: 1 }
                }
            }

            RowLayout {
                Layout.fillWidth: true; spacing: 15; Layout.topMargin: 10
                DialogButton { text: "Скасувати"; onClicked: addDialog.close() }
                DialogButton {
                    text: "Зберегти"
                    onClicked: {
                        if (nameField.text !== "") {
                            if (isEditing) PersonnelController.updatePerson(editId, rankCombo.currentText, nameField.text, posField.text, "в наявності")
                            else PersonnelController.addPerson(rankCombo.currentText, nameField.text, posField.text, "в наявності")
                            addDialog.close()
                        }
                    }
                }
            }
        }
    }

    // Вікно підтвердження видалення військовослужбовця
    Dialog {
        id: confirmDeleteDialog
        property int personId: -1
        width: 400
        padding: 25
        modal: true
        x: Math.round((parent.width - width) / 2)
        y: Math.round((parent.height - height) / 2)

        background: Rectangle { color: "white"; radius: 12; border.color: "#dcdcdc" }

        header: Label {
            text: "Видалення"
            font.pixelSize: 22; font.bold: true; color: "#2C3E50"; padding: 20; horizontalAlignment: Text.AlignHCenter
        }

        contentItem: ColumnLayout {
            spacing: 25
            Label {
                text: "Ви впевнені, що хочете видалити цього військовослужбовця?"
                font.pixelSize: 16; wrapMode: Text.WordWrap; Layout.fillWidth: true; horizontalAlignment: Text.AlignHCenter; color: "#34495E"
            }
            RowLayout {
                Layout.fillWidth: true; spacing: 15
                DialogButton { text: "Відміна"; onClicked: confirmDeleteDialog.close() }
                DialogButton {
                    text: "Видалити"
                    onClicked: {
                        PersonnelController.deletePerson(confirmDeleteDialog.personId)
                        confirmDeleteDialog.close()
                    }
                }
            }
        }
    }

    // Головне вікно керування статусами (Транзакційне)
    Dialog {
        id: statusMainDialog
        width: root.width * 0.9
        height: root.height * 0.7
        padding: 20
        modal: true
        focus: true
        x: Math.round((parent.width - width) / 2)
        y: Math.round((parent.height - height) / 2)

        background: Rectangle { color: "white"; radius: 12; border.color: "#dcdcdc"; border.width: 1 }

        header: Label {
            text: "Статуси: " + (!!PersonnelController ? PersonnelController.currentSelectedPersonName : "")
            font.pixelSize: 22; font.bold: true; padding: 20; color: "#2C3E50"; horizontalAlignment: Text.AlignHCenter
        }

        contentItem: ColumnLayout {
            spacing: 20

            Rectangle {
                Layout.fillWidth: true; Layout.fillHeight: true; border.color: "#dddddd"; radius: 8; clip: true

                ListView {
                    id: statusListView
                    anchors.fill: parent
                    anchors.margins: 1
                    model: statusModel
                    clip: true
                    currentIndex: -1
                    headerPositioning: ListView.OverlayHeader
                    header: Rectangle {
                        width: parent.width; height: 40; color: "#f2f2f2"; z: 2
                        RowLayout {
                            anchors.fill: parent; spacing: 0
                            Text { text: "№"; font.bold: true; Layout.preferredWidth: 40; horizontalAlignment: Text.AlignHCenter }
                            Text { text: "Статус"; font.bold: true; Layout.preferredWidth: 150; leftPadding: 10 }
                            Text { text: "Початок"; font.bold: true; Layout.preferredWidth: 120; leftPadding: 10 }
                            Text { text: "Кінець"; font.bold: true; Layout.preferredWidth: 120; leftPadding: 10 }
                            Text { text: "Примітки"; font.bold: true; Layout.fillWidth: true; leftPadding: 10 }
                        }
                    }

                    delegate: ItemDelegate {
                        id: statusDelegate
                        width: statusListView.width
                        height: 40
                        onClicked: statusListView.currentIndex = index
                        background: Rectangle { color: "white"; border.color: "#eeeeee" }
                        contentItem: RowLayout {
                            anchors.fill: parent; spacing: 0
                            Text {
                                text: index + 1
                                Layout.preferredWidth: 40
                                horizontalAlignment: Text.AlignHCenter
                                color: statusDelegate.ListView.isCurrentItem ? "#F39200" : "black"
                            }
                            Text {
                                text: statusName
                                Layout.preferredWidth: 150
                                leftPadding: 10
                                color: statusDelegate.ListView.isCurrentItem ? "#F39200" : "black"
                            }
                            Text {
                                text: startDate
                                Layout.preferredWidth: 120
                                leftPadding: 10
                                color: statusDelegate.ListView.isCurrentItem ? "#F39200" : "black"
                            }
                            Text {
                                text: endDate
                                Layout.preferredWidth: 120
                                leftPadding: 10
                                color: statusDelegate.ListView.isCurrentItem ? "#F39200" : "black"
                            }
                            Text {
                                text: notes
                                Layout.fillWidth: true
                                leftPadding: 10
                                elide: Text.ElideRight
                                color: statusDelegate.ListView.isCurrentItem ? "#F39200" : "black"
                            }
                        }
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 15

                // Кнопка Відміна - не відображається в режимі перегляду
                DialogButton {
                    text: "Відміна"
                    visible: statusMode !== "view"
                    onClicked: statusMainDialog.close()
                }

                // Кнопка Редагувати статус - для режимів add та edit
                DialogButton {
                    text: "Редагувати статус"
                    visible: statusMode === "add" || statusMode === "edit"
                    enabled: statusListView.currentIndex !== -1
                    onClicked: openEditStatus()
                }

                // Кнопка Додати статус - тільки в режимі add
                DialogButton {
                    text: "Додати статус"
                    visible: statusMode === "add"
                    onClicked: openAddStatus()
                }

                // Кнопка Видалити статус - тільки в режимі delete
                DialogButton {
                    text: "Видалити статус"
                    visible: statusMode === "delete"
                    enabled: statusListView.currentIndex !== -1
                    onClicked: confirmDeleteStatusDialog.open()
                }

                // Кнопка Примінити - не відображається в режимі перегляду
                DialogButton {
                    text: "Примінити"
                    visible: statusMode !== "view"
                    onClicked: applyStatusChanges()
                }

                // Кнопка Закрити - тільки для режиму перегляду
                DialogButton {
                    text: "Закрити"
                    visible: statusMode === "view"
                    onClicked: statusMainDialog.close()
                }
            }
        }
    }

    // Діалог підтвердження видалення СТАТУСУ (локально)
    Dialog {
        id: confirmDeleteStatusDialog
        width: 400
        padding: 25
        modal: true
        x: Math.round((parent.width - width) / 2)
        y: Math.round((parent.height - height) / 2)

        background: Rectangle { color: "white"; radius: 12; border.color: "#dcdcdc" }

        header: Label {
            text: "Видалення статусу"
            font.pixelSize: 22; font.bold: true; color: "#2C3E50"; padding: 20; horizontalAlignment: Text.AlignHCenter
        }

        contentItem: ColumnLayout {
            spacing: 25
            Label {
                text: "Ви дійсно хочете видалити вибраний статус?"
                font.pixelSize: 16; wrapMode: Text.WordWrap; Layout.fillWidth: true; horizontalAlignment: Text.AlignHCenter; color: "#34495E"
            }
            RowLayout {
                Layout.fillWidth: true; spacing: 15
                DialogButton {
                    text: "Ні"
                    onClicked: confirmDeleteStatusDialog.close()
                }
                DialogButton {
                    text: "Так"
                    onClicked: {
                        statusModel.remove(statusListView.currentIndex)
                        confirmDeleteStatusDialog.close()
                    }
                }
            }
        }
    }

    // Діалог додавання/редагування статусу в локальну модель
    Dialog {
        id: addStatusDialog
        width: 450
        padding: 20
        modal: true
        x: Math.round((parent.width - width) / 2)
        y: Math.round((parent.height - height) / 2)

        background: Rectangle { color: "white"; radius: 12; border.color: "#dcdcdc" }

        header: Label {
            text: isEditingStatus ? "Редагувати статус" : "Додати статус"
            font.pixelSize: 22; font.bold: true; color: "#2C3E50"; padding: 20; horizontalAlignment: Text.AlignHCenter
        }

        contentItem: ColumnLayout {
            spacing: 15

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 5
                Label {
                    text: "Тип статусу:"
                    font.bold: true
                    color: "#34495E"
                }
                ComboBox {
                    id: statusTypeCombo
                    Layout.fillWidth: true
                    Layout.preferredHeight: 40
                    model: ["Відпустка", "Лікарняний", "Відрядження", "Інше"]
                    background: Rectangle { radius: 8; border.color: "#BDC3C7" }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 5
                Label {
                    text: "Початок:"
                    font.bold: true
                    color: "#34495E"
                }
                DateSelector {
                    id: startStatusField
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 5
                Label {
                    text: "Кінець:"
                    font.bold: true
                    color: "#34495E"
                }
                DateSelector {
                    id: endStatusField
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 5
                Label {
                    text: "Примітки:"
                    font.bold: true
                    color: "#34495E"
                }
                TextArea {
                    id: notesStatusField
                    Layout.fillWidth: true
                    Layout.preferredHeight: 80
                    placeholderText: "Додаткова інформація"
                    wrapMode: TextArea.Wrap
                    padding: 10
                    background: Rectangle {
                        radius: 8
                        border.color: notesStatusField.activeFocus ? "#F39200" : "#BDC3C7"
                        border.width: 1
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 15
                Layout.topMargin: 10

                DialogButton {
                    text: "Скасувати"
                    onClicked: addStatusDialog.close()
                }

                DialogButton {
                    text: isEditingStatus ? "Змінити" : "Додати"
                    onClicked: {
                        if (isEditingStatus) {
                            statusModel.set(statusListView.currentIndex, {
                                "statusName": statusTypeCombo.currentText,
                                "startDate": startStatusField.text,
                                "endDate": endStatusField.text,
                                "notes": notesStatusField.text
                            })
                        } else {
                            statusModel.append({
                                "statusName": statusTypeCombo.currentText,
                                "startDate": startStatusField.text,
                                "endDate": endStatusField.text,
                                "notes": notesStatusField.text
                            })
                        }
                        addStatusDialog.close()
                    }
                }
            }
        }
    }
}
