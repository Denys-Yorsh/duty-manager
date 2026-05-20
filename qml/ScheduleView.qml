import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

/**
 * @file ScheduleView.qml
 * @brief Вертикальне відображення графіка нарядів з об'єднанням заголовків та покращеною логікою заміни.
 */
Page {
    id: root

    // --- Властивості ---
    readonly property var months: ["Січень", "Лютий", "Березень", "Квітень", "Травень", "Червень", "Липень", "Серпень", "Вересень", "Жовтень", "Листопад", "Грудень"]

    property int currentMonth: new Date().getMonth() + 1
    property int currentYear: new Date().getFullYear()

    property bool isCurrentSchedule: {
        var now = new Date();
        return currentMonth === (now.getMonth() + 1) && currentYear === now.getFullYear();
    }

    property bool isPersonnelSelected: false
    property int selectedDay: -1
    property int selectedDutyId: -1
    property int selectedSubIndex: -1
    property string selectedPersonName: ""

    /**
     * @brief Статична ширина колонки для відповідності стилю PersonnelView
     */
    property int preferredColumnWidth: 150

    /**
     * @property calculatedTableWidth
     * @brief Розрахункова ширина всієї таблиці.
     */
    readonly property int calculatedTableWidth: 60 + (scheduleModel.count * root.preferredColumnWidth)

    // --- Моделі ---
    ListModel {
        id: scheduleModel
    }
    property var assignments: ({})

    // --- Функції ---

    function openAutoGenDialog() {
        autoGenDialog.open();
    }
    function openEditScheduleDialog() {
        if (isCurrentSchedule)
            editScheduleDialog.open();
    }
    function openShowScheduleDialog() {
        yearsModel.clear();
        var years = ScheduleController.getAvailableYears();
        for (var i = 0; i < years.length; i++)
            yearsModel.append({
                year: years[i]
            });
        showScheduleDialog.open();
    }

    /**
     * @brief Відкриває діалог заміни в/с з попереднім оновленням статусів.
     */
    function openReplacePersonnelDialog() {
        if (isPersonnelSelected) {
            // Оновлюємо статуси в PersonnelController перед відкриттям списку
            PersonnelController.updateAllPersonnelStatuses();

            replacePersonnelModel.clear();
            var list = ScheduleController.getAvailablePersonnel();
            for (var i = 0; i < list.length; i++)
                replacePersonnelModel.append(list[i]);
            replacePersonnelDialog.open();
        }
    }

    function exportExcel() {
        excelSaveDialog.open();
    }
    function exportPdf() {
        pdfSaveDialog.open();
    }

    function refreshSchedule() {
        scheduleModel.clear();
        var struct = ScheduleController.getScheduleStructure();
        for (var i = 0; i < struct.length; i++) {
            scheduleModel.append(struct[i]);
        }

        var data = ScheduleController.getScheduleData(root.currentMonth, root.currentYear);
        var newAssignments = {};
        for (var j = 0; j < data.length; j++) {
            var key = data[j].dutyTypeId + "_" + data[j].day + "_" + data[j].subIndex;
            newAssignments[key] = {
                name: data[j].shortName,
                fullName: data[j].fullName,
                id: data[j].personId
            };
        }
        root.preferredColumnWidth = 150; // Фіксована ширина для стилю картки
        assignments = newAssignments;
        isPersonnelSelected = false;
        selectedDay = -1;
        selectedDutyId = -1;
        selectedSubIndex = -1;
    }

    Component.onCompleted: refreshSchedule()

    Connections {
        target: ScheduleController
        function onScheduleChanged() {
            refreshSchedule();
        }
        function onExportFinished(success, message) {
            exportResultText.text = message;
            exportResultDialog.isError = !success;
            exportResultDialog.open();
        }
    }

    function isWeekend(day) {
        var d = new Date(root.currentYear, root.currentMonth - 1, day).getDay();
        return d === 0 || d === 6;
    }

    // --- Компоненти дизайну ---
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

    // --- Верстка ---
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 15
        spacing: 0

        /**
         * @brief Основний контейнер таблиці у стилі картки
         */
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "white"
            border.color: "#E0E0E0" // М'яка межа
            border.width: 1
            radius: 12 // Закруглені кути
            clip: true

            ScrollView {
                anchors.fill: parent
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOn
                ScrollBar.vertical.policy: ScrollBar.AlwaysOn

                ColumnLayout {
                    id: verticalTable
                    spacing: 0
                    // Якщо нарядів немає (ширина 60), примусово розтягуємо таблицю на 1/3 екрану.
                    // Коли наряди є — вона стає своєї повної розрахункової ширини.
                    width: root.calculatedTableWidth <= 60 ? (root.width / 3) : root.calculatedTableWidth

                    // Рядок 1: Місяць та Рік (Шапка таблиці)
                    Rectangle {
                        height: 45
                        Layout.fillWidth: true
                        color: "#FAFAFA" // Пастельний колір шапки
                        border.color: "#EFEFEF"
                        border.width: 1
                        Text {
                            anchors.centerIn: parent
                            text: root.months[root.currentMonth - 1].toUpperCase() + " " + root.currentYear
                            font.bold: true
                            font.pixelSize: 16
                            color: "#2C3E50" // Темно-синій для тексту
                        }
                    }

                    // Рядок 2: Загальні заголовки (Дні та Назва наряду)
                    RowLayout {
                        spacing: 0
                        Rectangle {
                            width: 60
                            height: 40
                            color: "#FDF5E6" // Пастельний кремовий
                            border.color: "#EFEFEF"
                            border.width: 1
                            Text {
                                anchors.centerIn: parent
                                text: "Дні"
                                font.bold: true
                                color: "#2C3E50"
                            }
                        }
                        Rectangle {
                            height: 40
                            Layout.fillWidth: true
                            color: "#FDF5E6"
                            border.color: "#EFEFEF"
                            border.width: 1
                            Text {
                                anchors.centerIn: parent
                                text: "Назва наряду"
                                font.bold: true
                                color: "#2C3E50"
                            }
                        }
                    }

                    // Рядок 3: Заголовки конкретних видів нарядів
                    RowLayout {
                        spacing: 0
                        Rectangle {
                            width: 60
                            height: 40
                            color: "#FAFAFA"
                            border.color: "#EFEFEF"
                            border.width: 1
                        }

                        Rectangle {
                            width: root.calculatedTableWidth - 60
                            height: 40
                            color: "#FAFAFA"
                            border.color: "#EFEFEF"
                            border.width: 1
                            visible: scheduleModel.count === 0
                        }

                        Repeater {
                            model: scheduleModel
                            delegate: Rectangle {
                                width: model.isFirstInGroup ? (root.preferredColumnWidth * model.groupSize) : 0
                                height: 40
                                color: "#FAFAFA"
                                border.color: "#EFEFEF"
                                border.width: 1
                                visible: model.isFirstInGroup
                                Text {
                                    anchors.centerIn: parent
                                    text: model.dutyName
                                    font.bold: true
                                    wrapMode: Text.WordWrap
                                    horizontalAlignment: Text.AlignHCenter
                                    width: parent.width - 10
                                    color: "#2C3E50"
                                }
                            }
                        }
                    }

                    // Рядки: Дані по днях (Ефект зебри)
                    Repeater {
                        model: new Date(root.currentYear, root.currentMonth, 0).getDate()
                        delegate: RowLayout {
                            property int day: index + 1
                            spacing: 0

                            /**
                             * @brief Колонка з номером дня
                             */
                            Rectangle {
                                width: 60
                                height: 40
                                // Колір залежить від вихідного або вибраного дня
                                color: (root.selectedDay === day) ? "#FFF3E0" : (isWeekend(day) ? "#F5F5F5" : (index % 2 === 0 ? "#FFFFFF" : "#FAFAFA"))
                                border.color: "#F5F5F5"
                                border.width: 1
                                Text {
                                    anchors.centerIn: parent
                                    text: day
                                    font.bold: true
                                    color: (root.selectedDay === day) ? "#F39200" : "#333333"
                                }
                            }

                            Rectangle {
                                width: root.calculatedTableWidth - 60
                                height: 40
                                color: index % 2 === 0 ? "#FFFFFF" : "#FAFAFA"
                                border.color: "#F5F5F5"
                                border.width: 1
                                visible: scheduleModel.count === 0
                            }

                            /**
                             * @brief Дані призначень (Зебра та виділення)
                             */
                            Repeater {
                                model: scheduleModel
                                delegate: Rectangle {
                                    width: root.preferredColumnWidth
                                    height: 40
                                    // Логіка кольору: Виділена ячейка -> Вихідний -> Зебра
                                    color: (root.selectedDay === day && root.selectedDutyId === model.dutyTypeId && root.selectedSubIndex === model.subIndex)
                                           ? "#FFF3E0"
                                           : (isWeekend(day) ? "#F5F5F5" : (parent.day % 2 !== 0 ? "#FFFFFF" : "#FAFAFA"))
                                    border.color: "#F5F5F5"
                                    border.width: 1

                                    Text {
                                        anchors.centerIn: parent
                                        text: assignments[model.dutyTypeId + "_" + day + "_" + model.subIndex] ? assignments[model.dutyTypeId + "_" + day + "_" + model.subIndex].name : ""
                                        font.pixelSize: 12
                                        font.bold: (root.selectedDay === day && root.selectedDutyId === model.dutyTypeId && root.selectedSubIndex === model.subIndex)
                                        color: (root.selectedDay === day && root.selectedDutyId === model.dutyTypeId && root.selectedSubIndex === model.subIndex) ? "#F39200" : "#333333"
                                    }

                                    MouseArea {
                                        anchors.fill: parent
                                        onClicked: {
                                            root.selectedDay = day;
                                            root.selectedDutyId = model.dutyTypeId;
                                            root.selectedSubIndex = model.subIndex;
                                            var ass = assignments[model.dutyTypeId + "_" + day + "_" + model.subIndex];
                                            if (ass) {
                                                root.selectedPersonName = ass.fullName;
                                            } else {
                                                root.selectedPersonName = "Порожньо";
                                            }
                                            root.isPersonnelSelected = true;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // --- Модальні вікна ---

    // 1. Авто-генерація
    Dialog {
        id: autoGenDialog
        width: 400
        padding: 25
        modal: true
        x: Math.round((parent.width - width) / 2)
        y: Math.round((parent.height - height) / 2)
        background: Rectangle {
            color: "white"
            radius: 12
            border.color: "#dcdcdc"
        }
        header: Label {
            text: "Авто-генерація"
            font.pixelSize: 22
            font.bold: true
            color: "#2C3E50"
            padding: 20
            horizontalAlignment: Text.AlignHCenter
        }
        contentItem: ColumnLayout {
            spacing: 20
            ComboBox {
                id: autoGenMonth
                Layout.fillWidth: true
                model: root.months
                currentIndex: root.currentMonth - 1
            }
            ComboBox {
                id: autoGenYear
                Layout.fillWidth: true
                model: {
                    var years = [];
                    for (var i = 2026; i <= 2070; i++)
                        years.push(i);
                    return years;
                }
                currentIndex: root.currentYear - 2026 >= 0 ? root.currentYear - 2026 : 0
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: 15
                DialogButton {
                    text: "Відміна"
                    onClicked: autoGenDialog.close()
                }
                DialogButton {
                    text: "Примінити"
                    onClicked: {
                        // 1. Запускаем генерацию в C++
                        ScheduleController.generateSchedule(autoGenMonth.currentIndex + 1, autoGenYear.currentValue);

                        // 2. Обновляем месяц и год в интерфейсе
                        root.currentMonth = autoGenMonth.currentIndex + 1;
                        root.currentYear = autoGenYear.currentValue;

                        // 3. ИСПРАВЛЕНО: Принудительно заставляем таблицу обновиться и показать ПИБ
                        root.refreshSchedule();

                        // 4. Закрываем окно
                        autoGenDialog.close();
                    }
                }
            }
        }
    }

    // 2. Змінити графік
    Dialog {
        id: editScheduleDialog
        width: 400
        padding: 25
        modal: true
        x: Math.round((parent.width - width) / 2)
        y: Math.round((parent.height - height) / 2)
        background: Rectangle {
            color: "white"
            radius: 12
            border.color: "#dcdcdc"
        }
        header: Label {
            text: "Змінити графік"
            font.pixelSize: 22
            font.bold: true
            color: "#2C3E50"
            padding: 20
            horizontalAlignment: Text.AlignHCenter
        }
        contentItem: ColumnLayout {
            spacing: 20
            Label {
                text: "Виберіть день:"
                font.bold: true
            }
            ComboBox {
                id: editDayCombo
                Layout.fillWidth: true
                model: {
                    var days = [];
                    var count = new Date(root.currentYear, root.currentMonth, 0).getDate();
                    for (var i = 1; i <= count; i++)
                        days.push(i);
                    return days;
                }
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: 15
                DialogButton {
                    text: "Відміна"
                    onClicked: editScheduleDialog.close()
                }
                DialogButton {
                    text: "Примінити"
                    onClicked: {
                        ScheduleController.generateSchedule(root.currentMonth, root.currentYear, editDayCombo.currentValue);
                        editScheduleDialog.close();
                    }
                }
            }
        }
    }

    // 3. Показати графік
    Dialog {
        id: showScheduleDialog
        width: 400
        padding: 25
        modal: true
        x: Math.round((parent.width - width) / 2)
        y: Math.round((parent.height - height) / 2)
        background: Rectangle {
            color: "white"
            radius: 12
            border.color: "#dcdcdc"
        }
        header: Label {
            text: "Показати графік"
            font.pixelSize: 22
            font.bold: true
            color: "#2C3E50"
            padding: 20
            horizontalAlignment: Text.AlignHCenter
        }
        contentItem: ColumnLayout {
            spacing: 20
            ComboBox {
                id: showYearCombo
                Layout.fillWidth: true
                textRole: "year"
                model: ListModel {
                    id: yearsModel
                }
                onActivated: {
                    monthsModel.clear();
                    var ms = ScheduleController.getAvailableMonths(currentText);
                    for (var i = 0; i < ms.length; i++)
                        monthsModel.append({
                            mIdx: ms[i],
                            mName: root.months[ms[i] - 1]
                        });
                }
            }
            ComboBox {
                id: showMonthCombo
                Layout.fillWidth: true
                textRole: "mName"
                model: ListModel {
                    id: monthsModel
                }
                enabled: showYearCombo.currentIndex !== -1
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: 15
                DialogButton {
                    text: "Відміна"
                    onClicked: showScheduleDialog.close()
                }
                DialogButton {
                    text: "Примінити"
                    onClicked: {
                        root.currentYear = parseInt(showYearCombo.currentText);
                        root.currentMonth = monthsModel.get(showMonthCombo.currentIndex).mIdx;
                        refreshSchedule();
                        showScheduleDialog.close();
                    }
                }
            }
        }
    }

    // 4. Замінити в/с
    Dialog {
        id: replacePersonnelDialog
        width: 400
        padding: 25
        modal: true
        x: Math.round((parent.width - width) / 2)
        y: Math.round((parent.height - height) / 2)
        background: Rectangle {
            color: "white"
            radius: 12
            border.color: "#dcdcdc"
        }
        header: Label {
            text: "Замінити в/с"
            font.pixelSize: 22
            font.bold: true
            color: "#2C3E50"
            padding: 20
            horizontalAlignment: Text.AlignHCenter
        }
        contentItem: ColumnLayout {
            spacing: 20
            Label {
                text: "Поточний: " + root.selectedPersonName
                font.bold: true
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
            ComboBox {
                id: replacePersonCombo
                Layout.fillWidth: true
                textRole: "name"
                valueRole: "id"
                model: ListModel {
                    id: replacePersonnelModel
                }
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: 15
                DialogButton {
                    text: "Відміна"
                    onClicked: replacePersonnelDialog.close()
                }
                DialogButton {
                    text: "Примінити"
                    onClicked: {
                        var oldId = assignments[root.selectedDutyId + "_" + root.selectedDay + "_" + root.selectedSubIndex] ? assignments[root.selectedDutyId + "_" + root.selectedDay + "_" + root.selectedSubIndex].id : -1;
                        ScheduleController.assignPerson(root.selectedDay, root.currentMonth, root.currentYear, root.selectedDutyId, replacePersonCombo.currentValue, oldId);
                        replacePersonnelDialog.close();
                    }
                }
            }
        }
    }

    // 5. Результат експорту
    Dialog {
        id: exportResultDialog
        width: 400
        padding: 25
        modal: true
        property bool isError: false
        x: Math.round((parent.width - width) / 2)
        y: Math.round((parent.height - height) / 2)
        background: Rectangle {
            color: "white"
            radius: 12
            border.color: "#dcdcdc"
        }
        header: Label {
            text: exportResultDialog.isError ? "Помилка" : "Успіх"
            font.pixelSize: 22
            font.bold: true
            color: exportResultDialog.isError ? "#E74C3C" : "black"
            padding: 20
            horizontalAlignment: Text.AlignHCenter
        }
        contentItem: ColumnLayout {
            spacing: 25
            Text {
                id: exportResultText
                font.pixelSize: 16
                horizontalAlignment: Text.AlignHCenter
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
            }
            DialogButton {
                text: "Зрозуміло"
                onClicked: exportResultDialog.close()
            }
        }
    }

    // --- Діалоги збереження файлів ---
    FileDialog {
        id: excelSaveDialog
        title: "Зберегти як Excel"
        fileMode: FileDialog.SaveFile
        nameFilters: ["Excel files (*.xlsx)"]
        onAccepted: ScheduleController.exportToExcel(root.currentMonth, root.currentYear, selectedFile.toString().replace("file:///", ""))
    }

    FileDialog {
        id: pdfSaveDialog
        title: "Зберегти як PDF"
        fileMode: FileDialog.SaveFile
        nameFilters: ["PDF files (*.pdf)"]
        onAccepted: ScheduleController.exportToPdf(root.currentMonth, root.currentYear, selectedFile.toString().replace("file:///", ""))
    }
}
